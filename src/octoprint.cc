/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2021 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "octoprint.h"
#include "ccurl_thread.h"
#include "setting.hh"
#include "luamm.hh"

#include <curl/curl.h>
#include "json/json.h"


#include <queue>
#include <cmath>

//single printer configs
conky::simple_config_setting<std::string> octoprint_server("octoprint_server", "",
                                                       false);
conky::simple_config_setting<std::string> octoprint_apikey("octoprint_apikey", "",
                                                       false);
conky::simple_config_setting<double> octoprint_update_interval("octoprint_update_interval", 1,
                                                       false);

// so we can use a lua string to back a JSON value
template <>
struct conky::lua_traits<Json::Value, false, false, false> {
  static const lua::Type type = lua::TSTRING;
  typedef Json::Value Type;

  static inline std::pair<Type, bool> convert(lua::state &l, int index,
                                              const std::string &) {
    auto str = l.tostring(index);
    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( str, root );
    if ( !parsingSuccessful ) {
      return {Json::Value(Json::nullValue), false};
    }
    return {root, true};
  }
};

conky::simple_config_setting<Json::Value> octoprint_config("octoprint_config", Json::Value(Json::nullValue), false);


class octoprint_cb : public curl_callback<Json::Value> {
  typedef curl_callback<Json::Value> Base;

protected:
  virtual void process_data() {
    try {
      Json::Value tmp;
      Json::Reader reader;
      bool parsingSuccessful = reader.parse( data, tmp );
      if ( !parsingSuccessful ) {
        NORM_ERR("Error parsing the curl result into json: %s",data.c_str());
        // Json::StreamWriterBuilder wbuilder;
        // wbuilder["indentation"] = "";
        // auto conf_str = Json::writeString(wbuilder,conf_blob);
      }
      std::unique_lock<std::mutex> lock(Base::result_mutex);
      Base::result = tmp;
    } catch (std::runtime_error &e) { NORM_ERR("%s", e.what()); }
  }
public:
  octoprint_cb(uint32_t period, const std::string &url, const std::string& token) :
    Base(period, Base::Tuple(url)) {
      curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
      curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, token.c_str());
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
      Base::result = Json::Value(Json::nullValue);
    }
};

const Json::Value& r_get(const Json::Value& obj, std::queue<std::string>& keys) {
  if (keys.empty()) {
    return obj;
  }
  if (!obj.isObject() && !obj.isArray())
    throw std::runtime_error("Can only index into array and object types");

  const auto& value = obj.isObject() ? obj[keys.front()] : obj[std::stoi(keys.front())];
  keys.pop();
  return r_get(value, keys);
}

Json::Value recursive_get(const Json::Value& obj, std::queue<std::string>& keys) {
  try {
    //walk using reference, but return a copy of the final object
    return r_get(obj,keys);
  } catch(const std::exception& e) {
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
    auto conf_str = Json::writeString(wbuilder,obj);
    NORM_ERR("Error indexing into JSON blob: %s",conf_str.c_str());
    while (!keys.empty()) {
      NORM_ERR("remaining keys: %s", keys.front().c_str());
      keys.pop();
    }
    return Json::Value(Json::nullValue);
  }
}

Json::Value octoprint_get_last_xfer(const std::string &url, const std::string &token, double interval) {
  //first extract the update rate for the printer id
  uint32_t period = std::max(lround(interval / active_update_interval()), 1l);
  auto cb = conky::register_cb<octoprint_cb>(period, url, token);

  return cb->get_result_copy();
}


Json::Value octoprint_get_last_xfer(const std::string &printer_id, const std::string &endpoint) {
  std::string url;
  std::string token;
  double interval{octoprint_update_interval.get(*state)};
  if (printer_id.empty()) {
    url = octoprint_server.get(*state);
    if (url.empty()) {
      NORM_ERR("octoprint: no default printer url specified");
      return Json::Value(Json::nullValue);
    }
    
    token = octoprint_apikey.get(*state);
    if (token.empty()) {
      NORM_ERR("octoprint: no default api key specified");
      return Json::Value(Json::nullValue);
    }
  } else {
    const auto conf_blob = octoprint_config.get(*state);
      // Json::StreamWriterBuilder wbuilder;
      // wbuilder["indentation"] = "";
      // auto conf_str = Json::writeString(wbuilder,conf_blob);
      // NORM_ERR("octoprint_config: %s",conf_str.c_str());
    if (!conf_blob.isMember(std::string{printer_id})) {
      NORM_ERR("octoprint: could not find printer_id: %s", printer_id.c_str());
      return Json::Value(Json::nullValue);
    }
    const auto& printer_conf = conf_blob[printer_id];
    url = printer_conf["url"].asString();
    if (url.empty()) {
      NORM_ERR("octoprint: url for printer '%s' not specified",printer_id.c_str());
      return Json::Value(Json::nullValue);
    }
    token = printer_conf["apikey"].asString();
    if (token.empty()) {
      NORM_ERR("octoprint: apikey for printer '%s' not specified",printer_id.c_str());
      return Json::Value(Json::nullValue);
    }
    if (printer_conf.isMember("interval"))
      interval = printer_conf["interval"].asDouble();
  }
  url += endpoint;
  return octoprint_get_last_xfer(url, token, interval);
} 



struct octoprint_data {
  char* printer_id;
  char* component_id;
};


Json::Value extract_common(struct text_object *obj, const std::string& endpoint, std::queue<std::string>& query_path) {
  struct octoprint_data *od = static_cast<struct octoprint_data *>(obj->data.opaque);
  if (!od) {
    NORM_ERR("error processing Octoprint data");
    return Json::Value(Json::nullValue);
  }

  const auto state_json = octoprint_get_last_xfer(od->printer_id ? od->printer_id : "", endpoint);
  if (state_json.isNull()) {
    NORM_ERR("no data available for printer '%s' endpoint '%s'", (od->printer_id ? od->printer_id : "<default>"), endpoint.c_str());
    return state_json;
  }
  return recursive_get(state_json, query_path);
}

void print_common(const Json::Value& result, char *p, unsigned int p_max_size) {

  if (result.isNull()) {
    strncpy(p,"", p_max_size);
    return;
  } else if (result.isIntegral()) {
    strncpy(p, std::to_string(result.asLargestInt()).c_str(), p_max_size);
  } else if (result.isDouble()) {
    strncpy(p, std::to_string(result.asDouble()).c_str(), p_max_size);
  } else if (result.isString()) {
    strncpy(p, result.asCString(), p_max_size);
  } else {
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
    strncpy(p, Json::writeString(wbuilder,result).c_str(), p_max_size);
  }
  
}


// /api/currentuser
void print_octoprint_user(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"name"});
  print_common(extract_common(obj, "/api/currentuser", q), p, p_max_size);
}

// /api/version
void print_octoprint_version(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"server"});
  print_common(extract_common(obj, "/api/version", q), p, p_max_size);
}

void print_octoprint_longversion(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"text"});
  print_common(extract_common(obj, "/api/version", q), p, p_max_size);
}

// /api/server
void print_octoprint_safemode(struct text_object *obj, char *p, unsigned int p_max_size) {

  std::queue<std::string> q({"safemode"});
  print_common(extract_common(obj, "/api/server", q), p, p_max_size);
}

// /api/connection
void print_octoprint_conn_baud(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"current", "baudrate"});
  print_common(extract_common(obj, "/api/connection", q), p, p_max_size);
}

void print_octoprint_conn_port(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"current", "port"});
  print_common(extract_common(obj, "/api/connection", q), p, p_max_size);
}

void print_octoprint_conn_state(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"current", "state"});
  print_common(extract_common(obj, "/api/connection", q), p, p_max_size);
}

// /api/files
// /api/files/<location>
// /api/files/<location>/<file>
void print_octoprint_local_used(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"files"});
  //TODO: make recursive
  auto result = extract_common(obj, "/api/files/local", q);

  std::size_t bytes{0};
  for (const auto& file : result) {
    bytes += file["size"].asUInt();
  }

  human_readable(bytes, p, p_max_size);
}

void print_octoprint_local_free(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"free"});
  human_readable(extract_common(obj, "/api/files/local", q).asLargestInt(), p, p_max_size);
}

void print_octoprint_local_total(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"total"});
  human_readable(extract_common(obj, "/api/files/local", q).asLargestInt(), p, p_max_size);
}

void print_octoprint_local_filecount(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"files"});
  auto result = extract_common(obj, "/api/files/local", q);
  strncpy(p, std::to_string(result.size()).c_str(), p_max_size);
}

void print_octoprint_sdcard_filecount(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"files"});
  auto result = extract_common(obj, "/api/files/sdcard", q);
  strncpy(p, std::to_string(result.size()).c_str(), p_max_size);
}

// /api/job
void print_octoprint_job_name(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"job", "file", "name"});
  print_common(extract_common(obj, "/api/job", q), p, p_max_size);
}

void print_octoprint_job_progress(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"progress", "completion"});
  print_common(extract_common(obj, "/api/job", q), p, p_max_size);
}

uint8_t octoprint_job_progress_pct(struct text_object *obj) {
  std::queue<std::string> q({"progress", "completion"});
  return extract_common(obj, "/api/job", q).asUInt();
}

double octoprint_job_progress_barval(struct text_object *obj) {
  std::queue<std::string> q({"progress", "completion"});
  return extract_common(obj, "/api/job", q).asDouble();
}


void print_octoprint_job_time(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"progress", "printTime"});
  print_common(extract_common(obj, "/api/job", q), p, p_max_size);
}

void print_octoprint_job_time_left(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"progress", "printTimeLeft"});
  print_common(extract_common(obj, "/api/job", q), p, p_max_size);
}

void print_octoprint_job_state(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"state"});
  print_common(extract_common(obj, "/api/job", q), p, p_max_size);
}

void print_octoprint_job_user(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"job", "user"});
  print_common(extract_common(obj, "/api/job", q), p, p_max_size);
}

// /plugin/logging/logs
void print_octoprint_logs_used(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"files"});
  auto result = extract_common(obj, "/plugin/logging/logs", q);

  std::size_t bytes{0};
  for (const auto& file : result) {
    bytes += file["size"].asUInt();
  }

  human_readable(bytes, p, p_max_size);
}

void print_octoprint_logs_free(struct text_object *obj, char *p, unsigned int p_max_size){
  std::queue<std::string> q({"free"});
  human_readable(extract_common(obj, "/plugin/logging/logs", q).asLargestInt(), p, p_max_size);
}

void print_octoprint_logs_total(struct text_object *obj, char *p, unsigned int p_max_size){
  std::queue<std::string> q({"total"});
  human_readable(extract_common(obj, "/plugin/logging/logs", q).asLargestInt(), p, p_max_size);
}

// /api/printer  -- use single endpoint for efficiency
void print_octoprint_printer_state(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"state", "text"});
  print_common(extract_common(obj, "/api/printer", q), p, p_max_size);
}

void print_octoprint_printer_error(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"state", "error"});
  print_common(extract_common(obj, "/api/printer", q), p, p_max_size);
}



Json::Value get_temperature(struct text_object *obj, const std::string& field) {
  struct octoprint_data *od = static_cast<struct octoprint_data *>(obj->data.opaque);
  if (!od) {
    NORM_ERR("error processing Octoprint data");
    return Json::Value(Json::nullValue);
  }
  if (!od->component_id) {
    NORM_ERR("octoprint: component_id for temperature not specified");
    return Json::Value(Json::nullValue);
  }
  std::queue<std::string> q({"temperature", od->component_id, field});
  return extract_common(obj, "/api/printer", q);
}

void print_octoprint_temperature(struct text_object *obj, char *p, unsigned int p_max_size) {
  auto temperature = get_temperature(obj, "actual");
  if (temperature.isNull()){
    snprintf(p, p_max_size, "???");
  } else {
    snprintf(p, p_max_size, "%3.1f", temperature.asDouble());
  }
}

void print_octoprint_target_temp(struct text_object *obj, char *p, unsigned int p_max_size) {
  auto temperature = get_temperature(obj, "target");
  if (temperature.isNull()){
    snprintf(p, p_max_size, "???");
  } else {
    snprintf(p, p_max_size, "%3.1f", temperature.asDouble());
  }
}


double octoprint_temperature(struct text_object *obj) {
  return get_temperature(obj,"actual").asDouble();
}

double octoprint_target_temp(struct text_object *obj) {
  return get_temperature(obj,"target").asDouble();
}


void print_octoprint_sdcard_ready(struct text_object *obj, char *p, unsigned int p_max_size) {
  std::queue<std::string> q({"sd", "ready"});
  print_common(extract_common(obj, "/api/printer", q), p, p_max_size);
}

//tries to match the specific component patterns
int check_for_component(const char* arg) {
  int char_count{0};
  if (sscanf(arg, "tool%*d%n ", &char_count) != EOF && char_count) {
  } else if (sscanf(arg, "bed%n ", &char_count) != EOF && char_count) {
  } else if (sscanf(arg, "chamber%n ", &char_count) != EOF && char_count) {
  }
  return char_count;
}

//tries to parse a string which may contain printer and/or component specifier
void octoprint_parse_arg(struct text_object *obj, const char *arg) {
  struct octoprint_data *od;
  od = static_cast<struct octoprint_data *>(malloc(sizeof(struct octoprint_data)));
  memset(od, 0, sizeof(struct octoprint_data));
  obj->data.opaque = od;
  if (arg == nullptr || strlen(arg) < 1) {
    //leave pointers as null and return
    return;
  }

  int char_count{0};
  const char* sep = strrchr(arg, ':');
  if (sep && (char_count = check_for_component(sep+1))) {
    od->component_id = strndup(sep+1,char_count);
    od->printer_id = strndup(arg, sep-arg);
  } else if (char_count = check_for_component(arg)) {
    od->component_id = strndup(arg,char_count);
  } else {
    od->printer_id = strdup(arg);
  }
  
}

void octoprint_free_obj_info(struct text_object *obj) {
  struct octoprint_data *od = static_cast<struct octoprint_data *>(obj->data.opaque);
  free_and_zero(od->printer_id);
  free_and_zero(od->component_id);
  free_and_zero(obj->data.opaque);
}
