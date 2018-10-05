/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2010 Pavel Labath et al.
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

#include <config.h>

#include "conky.h"
#include "display-http.hh"

#include <iostream>
#include <sstream>
#include <unordered_map>

#ifdef BUILD_HTTP
#include <microhttpd.h>
#endif /* BUILD_HTTP */


namespace conky {
namespace {

#ifdef BUILD_HTTP
conky::display_output_http http_output;
#else
conky::disabled_display_output http_output_disabled("http", "BUILD_HTTP");
#endif

}  // namespace

// TODO: cleanup namespace
//namespace priv {

#ifdef BUILD_HTTP
std::string webpage;
struct MHD_Daemon *httpd;
static conky::simple_config_setting<bool> http_refresh("http_refresh", false,
                                                       true);

int sendanswer(void *cls, struct MHD_Connection *connection, const char *url,
               const char *method, const char *version, const char *upload_data,
               size_t *upload_data_size, void **con_cls) {
  struct MHD_Response *response = MHD_create_response_from_buffer(
      webpage.length(), (void *)webpage.c_str(), MHD_RESPMEM_PERSISTENT);
  int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  if (cls || url || method || version || upload_data || upload_data_size ||
      con_cls) {}  // make compiler happy
  return ret;
}

class out_to_http_setting : public conky::simple_config_setting<bool> {
  typedef conky::simple_config_setting<bool> Base;

 protected:
  virtual void lua_setter(lua::state &l, bool init) {
    lua::stack_sentry s(l, -2);

    Base::lua_setter(l, init);

    if (init && do_convert(l, -1).first) {
      httpd = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, HTTPPORT, nullptr,
                               NULL, &sendanswer, nullptr, MHD_OPTION_END);
    }

    ++s;
  }

  virtual void cleanup(lua::state &l) {
    lua::stack_sentry s(l, -1);

    if (do_convert(l, -1).first) {
      MHD_stop_daemon(httpd);
      httpd = nullptr;
    }

    l.pop();
  }

 public:
  out_to_http_setting() : Base("out_to_http", false, false) {}
};
static out_to_http_setting out_to_http;

std::string string_replace_all(std::string original, const std::string &oldpart,
                               const std::string &newpart,
                               std::string::size_type start) {
  std::string::size_type i = start;
  int oldpartlen = oldpart.length();
  while (1) {
    i = original.find(oldpart, i);
    if (i == std::string::npos) { break; }
    original.replace(i, oldpartlen, newpart);
  }
  return original;
}

#endif /* BUILD_HTTP */

//}  // namespace priv

#ifdef BUILD_HTTP

display_output_http::display_output_http()
    : display_output_base("http") {
  priority = 1;
  httpd = NULL;
}

bool display_output_http::detect() {
  if (/*priv::*/out_to_http.get(*state)) {
    std::cerr << "Display output '" << name << "' enabled in config." << std::endl;
    return true;
  }
  return false;
}

bool display_output_http::initialize() {
  if (/*priv::*/out_to_http.get(*state)) {
    is_active = true;
    return true;
  }
  return false;
}

bool display_output_http::shutdown() {
  return true;
}

bool display_output_http::begin_draw_text() {
#ifdef BUILD_HTTP
#define WEBPAGE_START1                                             \
  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "    \
  "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n<html " \
  "xmlns=\"http://www.w3.org/1999/xhtml\"><head><meta "            \
  "http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\" />"
#define WEBPAGE_START2 \
  "<title>Conky</title></head><body style=\"font-family: monospace\"><p>"
#define WEBPAGE_END "</p></body></html>"
  if (out_to_http.get(*state)) {
    webpage = WEBPAGE_START1;
    if (http_refresh.get(*state)) {
      webpage.append("<meta http-equiv=\"refresh\" content=\"");
      std::stringstream update_interval_str;
      update_interval_str << update_interval.get(*state);
      webpage.append(update_interval_str.str());
      webpage.append("\" />");
    }
    webpage.append(WEBPAGE_START2);
  }
#endif /* BUILD_HTTP */
  return true;
}

bool display_output_http::end_draw_text() {
  webpage.append(WEBPAGE_END);
  return true;
}

bool display_output_http::draw_string(const char *s, int w) {
  std::string::size_type origlen = webpage.length();
  webpage.append(s);
  webpage = string_replace_all(webpage, "\n", "<br />", origlen);
  webpage = string_replace_all(webpage, "  ", "&nbsp;&nbsp;", origlen);
  webpage = string_replace_all(webpage, "&nbsp; ", "&nbsp;&nbsp;", origlen);
  webpage.append("<br />");
  return true;
}

#endif

}  // namespace conky

