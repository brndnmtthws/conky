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

#include "conky.h"
#include "logging.h"
#include "ccurl_thread.h"
#include "text_object.h"
#include <cmath>
#include <mutex>

#ifdef DEBUG
#include <assert.h>
#endif /* DEBUG */

#include <curl/easy.h>

/*
 * The following code is the conky curl thread lib, which can be re-used to
 * create any curl-based object (see weather and rss).  Below is an
 * implementation of a curl-only object ($curl) which can also be used as an
 * example.
 */

namespace priv {
	/* callback used by curl for parsing the header data */
	size_t curl_internal::parse_header_cb(void *ptr, size_t size, size_t nmemb, void *data)
	{
		curl_internal *obj = static_cast<curl_internal *>(data);
		const char *value = static_cast<const char *>(ptr);
		size_t realsize = size * nmemb;

		if(realsize > 0 && (value[realsize-1] == '\r' || value[realsize-1] == 0))
			--realsize;

		if (strncmp(value, "Last-Modified: ", 15) == EQUAL) {
			obj->last_modified = std::string(value + 15, realsize - 15);
		} else if (strncmp(value,"ETag: ", 6) == EQUAL) {
			obj->etag = std::string(value + 6, realsize - 6);
		}

		return size*nmemb;
	}

	/* callback used by curl for writing the received data */
	size_t curl_internal::write_cb(void *ptr, size_t size, size_t nmemb, void *data)
	{
		curl_internal *obj = static_cast<curl_internal *>(data);
		const char *value = static_cast<const char *>(ptr);
		size_t realsize = size * nmemb;

		obj->data += std::string(value, realsize);

		return realsize;
	}

	curl_internal::curl_internal(const std::string &url)
		: curl(curl_easy_init())
	{
		if(not curl)
			throw std::runtime_error("curl_easy_init() failed");

		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, parse_header_cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "conky-curl/1.1");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1000);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60);

		// curl's usage of alarm()+longjmp() is a really bad idea for multi-threaded applications
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

	}


	/* fetch our datums */
	void curl_internal::do_work()
	{
		CURLcode res;
		struct headers_ {
			struct curl_slist *h;
		
			headers_() : h(NULL) {}
			~headers_() { curl_slist_free_all(h); }
		} headers;

		data.clear();

		if (not last_modified.empty()) {
			headers.h = curl_slist_append(headers.h, ("If-Modified-Since: " + last_modified).c_str());
			last_modified.clear();
		}
		if (not etag.empty()) {
			headers.h = curl_slist_append(headers.h, ("If-None-Match: " + etag).c_str());
			etag.clear();
		}
		if (headers.h) {
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers.h);
		}

		res = curl_easy_perform(curl);
		if (res == CURLE_OK) {
			long http_status_code;

			if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code) == CURLE_OK) {
				switch (http_status_code) {
					case 200:
						process_data();
						break;
					case 304:
						break;
					default:
						NORM_ERR("curl: no data from server, got HTTP status %ld",
								http_status_code);
						break;
				}
			} else {
				NORM_ERR("curl: no HTTP status from server");
			}
		} else {
			NORM_ERR("curl: could not retrieve data from server");
		}
	}
}

namespace {
	class simple_curl_cb: public curl_callback<std::string> {
		typedef curl_callback<std::string> Base;

	protected:
		virtual void process_data()
		{
			std::lock_guard<std::mutex> lock(result_mutex);
			result = data;
		}
	
	public:
		simple_curl_cb(uint32_t period, const std::string &uri)
			: Base(period, Tuple(uri))
		{}
	};
}

/*
 * This is where the $curl section begins.
 */

struct curl_data {
	char uri[128];
	float interval;
};

/* prints result data to text buffer, used by $curl */
void ccurl_process_info(char *p, int p_max_size, const std::string &uri, int interval)
{
	uint32_t period = std::max(lround(interval/active_update_interval()), 1l);
	auto cb = conky::register_cb<simple_curl_cb>(period, uri);

	strncpy(p, cb->get_result_copy().c_str(), p_max_size);
}

void curl_parse_arg(struct text_object *obj, const char *arg)
{
	int argc;
	struct curl_data *cd;
	float interval = 0;

	cd = (struct curl_data*)malloc(sizeof(struct curl_data));
	memset(cd, 0, sizeof(struct curl_data));

	argc = sscanf(arg, "%127s %f", cd->uri, &interval);
	if (argc < 1) {
		free(cd);
		NORM_ERR("wrong number of arguments for $curl");
		return;
	}
	if (argc == 1)
		cd->interval = 15*60;
	else
		cd->interval = interval > 0 ? interval * 60 : active_update_interval();
	obj->data.opaque = cd;
}

void curl_print(struct text_object *obj, char *p, int p_max_size)
{
	struct curl_data *cd = (struct curl_data *)obj->data.opaque;

	if (!cd || !cd->uri) {
		NORM_ERR("error processing Curl data");
		return;
	}
	ccurl_process_info(p, p_max_size, cd->uri, cd->interval);
}

void curl_obj_free(struct text_object *obj)
{
	free_and_zero(obj->data.opaque);
}
