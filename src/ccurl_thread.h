/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef _CURL_THREAD_H_
#define _CURL_THREAD_H_

#include <curl/curl.h>

#include "update-cb.hh"

namespace priv {
// factored out stuff that does not depend on the template parameters
struct curl_internal {
  std::string last_modified;
  std::string etag;
  std::string data;
  CURL *curl;

  static size_t parse_header_cb(void *ptr, size_t size, size_t nmemb,
                                void *data);
  static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data);

  void do_work();

  // called by do_work() after downloading data from the uri
  // it should populate the result variable
  virtual void process_data() = 0;

  curl_internal(const std::string &url);
  virtual ~curl_internal() {
    if (curl) curl_easy_cleanup(curl);
  }
};
}  // namespace priv

/*
 * Curl callback class template
 * the key is an url
 */
template <typename Result, typename... Keys>
class curl_callback : public conky::callback<Result, std::string, Keys...>,
                      protected priv::curl_internal {
  typedef conky::callback<Result, std::string, Keys...> Base1;
  typedef priv::curl_internal Base2;

 protected:
  virtual void work() {
    DBGP("reading curl data from '%s'", std::get<0>(Base1::tuple).c_str());
    do_work();
  }

 public:
  curl_callback(uint32_t period, const typename Base1::Tuple &tuple)
      : Base1(period, false, tuple), Base2(std::get<0>(tuple)) {}
};

/* $curl exports begin */

/* runs instance of $curl */
void ccurl_process_info(char *p, int p_max_size, const std::string &uri,
                        int interval);

void curl_parse_arg(struct text_object *, const char *);
void curl_print(struct text_object *, char *, int);
void curl_obj_free(struct text_object *);

/* $curl exports end */

#endif /* _CURL_THREAD_H_ */
