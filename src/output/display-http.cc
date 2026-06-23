/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018 François Revol et al.
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
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

#include <config.h>

#include "../conky.h"
#include "display-http.hh"
#include "output-setting.hh"

#include <iostream>
#include <mutex>
#include <sstream>

#include <microhttpd.h>

namespace conky {
namespace {
conky::display_output_http http_output;
}  // namespace

// TODO: cleanup namespace
// namespace priv {

#ifdef MHD_YES
/* older API */
#define MHD_Result int
#endif /* MHD_YES */
/* `presented` is the page served to clients: read by libmicrohttpd's worker
 * thread in sendanswer() and published by the draw thread in end_draw_text(),
 * so every access must hold builder_mutex. The draw thread assembles into
 * `webpage` (which it alone touches) and swaps it in under the lock; this way
 * a request never observes a half-built page or races std::string's buffer
 * management, which previously corrupted the heap and hung the process. */
std::mutex builder_mutex;
std::string presented;
static std::string webpage;
struct MHD_Daemon *httpd;
static conky::simple_config_setting<bool> http_refresh("http_refresh", false,
                                                       true);
static conky::simple_config_setting<unsigned short> http_port("http_port",
                                                              HTTPPORT, true);

MHD_Result sendanswer(void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method, const char *version,
                      const char *upload_data, size_t *upload_data_size,
                      void **con_cls) {
  struct MHD_Response *response;
  {
    /* Copy the page out under the lock; MHD_RESPMEM_MUST_COPY snapshots the
     * bytes so we don't hand MHD a pointer into a string the draw thread may
     * reallocate. */
    std::lock_guard<std::mutex> lock(builder_mutex);
    response = MHD_create_response_from_buffer(
        presented.length(), (void *)presented.c_str(), MHD_RESPMEM_MUST_COPY);
  }
  MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  if (cls || url || method || version || upload_data || upload_data_size ||
      con_cls) {}  // make compiler happy
  return ret;
}

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

std::string html_escape(const std::string &input) {
  std::string escaped;
  escaped.reserve(input.size());

  for (char ch : input) {
    switch (ch) {
      case '&':
        escaped.append("&amp;");
        break;
      case '<':
        escaped.append("&lt;");
        break;
      case '>':
        escaped.append("&gt;");
        break;
      case '"':
        escaped.append("&quot;");
        break;
      case '\'':
        escaped.append("&#39;");
        break;
      default:
        escaped.push_back(ch);
        break;
    }
  }

  return escaped;
}

//}  // namespace priv

display_output_http::display_output_http()
    : display_output_base("http", output_t::HTTP) {
  httpd = NULL;
}

bool display_output_http::initialize() {
  /* warn about old default port */
  if (http_port.get(*state) == 10080) {
    LOG_WARNING(
        "port {} is blocked by browsers like Firefox and Chromium, "
        "you may want to change http_port",
        http_port.get(*state));
  }
  httpd = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, http_port.get(*state),
                           nullptr, NULL, &sendanswer, nullptr, MHD_OPTION_END);

  return true;
}

bool display_output_http::shutdown() {
  if (httpd != nullptr) {
    MHD_stop_daemon(httpd);
    httpd = nullptr;
  }
  return true;
}

void display_output_http::begin_draw_text() {
#define WEBPAGE_START1                                             \
  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" "    \
  "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n<html " \
  "xmlns=\"http://www.w3.org/1999/xhtml\"><head><meta "            \
  "http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\" />"
#define WEBPAGE_START2 \
  "<title>Conky</title></head><body style=\"font-family: monospace\"><p>"
#define WEBPAGE_END "</p></body></html>"
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

void display_output_http::end_draw_text() {
  webpage.append(WEBPAGE_END);
  /* Publish the freshly built page for the HTTP worker thread to serve. */
  std::lock_guard<std::mutex> lock(builder_mutex);
  presented.swap(webpage);
}

void display_output_http::draw_string(const char *s, int) {
  std::string::size_type origlen = webpage.length();
  webpage.append(html_escape(s));
  webpage = string_replace_all(webpage, "\n", "<br />", origlen);
  webpage = string_replace_all(webpage, "  ", "&nbsp;&nbsp;", origlen);
  webpage = string_replace_all(webpage, "&nbsp; ", "&nbsp;&nbsp;", origlen);
  webpage.append("<br />");
}

}  // namespace conky
