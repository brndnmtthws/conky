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
        NORM_ERR("Error parsing the curl result into json");
      }
      std::unique_lock<std::mutex> lock(Base::result_mutex);
      Base::result = tmp;
    } catch (std::runtime_error &e) { NORM_ERR("%s", e.what()); }
  }
public:
  octoprint_cb(uint32_t period, const std::string &url, const std::string& token) :
    Base(period, Base::Tuple(url)) {
      NORM_ERR("creating curl cb with token %s",token.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BEARER);
      curl_easy_setopt(curl, CURLOPT_XOAUTH2_BEARER, token.c_str());
      curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
      Base::result = Json::Value(Json::nullValue);
    }
};

struct octoprint_data {
  char *printer_id;
  int tool_index;
};

void print_octoprint_printer_state(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct octoprint_data *od = static_cast<struct octoprint_data *>(obj->data.opaque);
  if (!od) {
    NORM_ERR("error processing Octoprint data");
    return;
  }

  auto state_json = octoprint_get_last_xfer(od->printer_id, "/api/printer");

  if (state_json.isNull()) {
    strncpy(p,"data not found", p_max_size);
    return;
  }

  strncpy(p, state_json["state"]["text"].asCString(), p_max_size);
}


Json::Value octoprint_get_last_xfer(const std::string &printer_id, const std::string &endpoint) {
  std::string url;
  std::string token;
  double interval;
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
    interval = octoprint_update_interval.get(*state);  
  } else {
    const auto conf_blob = octoprint_config.get(*state);
      Json::StreamWriterBuilder wbuilder;
      wbuilder["indentation"] = "";
      auto conf_str = Json::writeString(wbuilder,conf_blob);
    NORM_ERR("octoprint_config: %s",conf_str.c_str());
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
    token = printer_conf["token"].asString();
    if (token.empty()) {
      NORM_ERR("octoprint: token for printer '%s' not specified",printer_id.c_str());
      return Json::Value(Json::nullValue);
    }
    interval = printer_conf["interval"].asDouble();
  }
  url += endpoint;
  return octoprint_get_last_xfer(url, token, interval);
} 

/* prints result data to text buffer, used by $curl */
Json::Value octoprint_get_last_xfer(const std::string &url, const std::string &token, double interval) {
  //first extract the update rate for the printer id
  uint32_t period = std::max(lround(interval / active_update_interval()), 1l);
  auto cb = conky::register_cb<octoprint_cb>(period, url, token);

  return cb->get_result_copy();
}



void octoprint_parse_arg(struct text_object *obj, const char *arg) {
  NORM_ERR("octoprint_parse_arg got arg '%s'",arg);
  struct octoprint_data *od;
  // null values are perfectly acceptable here
  od = static_cast<struct octoprint_data *>(malloc(sizeof(struct octoprint_data)));
  memset(od, 0, sizeof(struct octoprint_data));
  od->printer_id = strdup("");
  obj->data.opaque = od;
  if (arg == nullptr || strlen(arg) < 1) {
    return;
  }

  //we have some args.
  // %s -> octoprint_config key
  // %d -> tool index
  // %s %d -> octoprint_config key and tool index
  // there is a pathological case where a user could supply a single set of
  //    number-compatible chars, expecting it to be treated as a config key 
  //    and not a tool index. 

  int tool_index;
  int match = sscanf(arg, "%d", &tool_index);
  if (match == 1) {
    NORM_ERR("parsed numeric from arg '%d'",tool_index);
    od->tool_index = tool_index;
    return;
  } 

  int char_count{0};
  match = sscanf(arg, "%*s%n %d", &char_count, &tool_index);
  if (char_count > 0) {
    od->printer_id = strndup(arg,char_count);
    NORM_ERR("parsed string from arg '%s'",od->printer_id);
  }
  if (match > 0) {
    NORM_ERR("parsed numeric from arg '%d'",tool_index);
    od->tool_index = tool_index;
  }
  
}

void octoprint_free_obj_info(struct text_object *obj) {
  struct octoprint_data *od = static_cast<struct octoprint_data *>(obj->data.opaque);
  free_and_zero(od->printer_id);
  free_and_zero(obj->data.opaque);
}
