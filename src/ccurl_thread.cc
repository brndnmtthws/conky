/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2010 Brenden Matthews, Philip Kovacs, et. al.
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
#include <mutex>

#ifdef DEBUG
#include <assert.h>
#endif /* DEBUG */

#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

/*
 * The following code is the conky curl thread lib, which can be re-used to
 * create any curl-based object (see weather and rss).  Below is an
 * implementation of a curl-only object ($curl) which can also be used as an
 * example.
 */
typedef struct _ccurl_memory_t {
	char *memory;
	size_t size;
} ccurl_memory_t;

typedef struct _ccurl_headers_t {
	char *last_modified;
	char *etag;
} ccurl_headers_t;

/* finds a location based on uri in the list provided */
ccurl_location_ptr ccurl_find_location(ccurl_location_list &locations, char *uri)
{
	for (ccurl_location_list::iterator i = locations.begin();
			i != locations.end(); i++) {
		if ((*i)->uri &&
				strcmp((*i)->uri, uri) == EQUAL) {
			return *i;
		}
	}
	ccurl_location_ptr next = ccurl_location_ptr(new ccurl_location_t);
	DBGP("new curl location: '%s'", uri);
	next->uri = strndup(uri, text_buffer_size);
	locations.push_back(next);
	return next;
}

/* iterates over the list provided, frees stuff (list item, uri, result) */
void ccurl_free_locations(ccurl_location_list &locations)
{
	for (ccurl_location_list::iterator i = locations.begin();
			i != locations.end(); i++) {
		free_and_zero((*i)->uri);
		free_and_zero((*i)->last_modified);
		free_and_zero((*i)->etag);
		free_and_zero((*i)->result);
		(*i)->p_timed_thread.reset();
	}
	locations.clear();
}

/* callback used by curl for parsing the header data */
size_t ccurl_parse_header_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	const char *value = (const char*)ptr;
	char *end;
	ccurl_headers_t *headers = (ccurl_headers_t*)data;

	if (strncmp(value, "Last-Modified: ", 15) == EQUAL) {
		headers->last_modified = strndup(value + 15, realsize - 15);
		if ((end = strchr(headers->last_modified, '\r')) != NULL) {
			*end = '\0';
		}
	} else if (strncmp(value,"ETag: ", 6) == EQUAL) {
		headers->etag = strndup(value + 6, realsize - 6);
		if ((end = strchr(headers->etag, '\r')) != NULL) {
			*end = '\0';
		}
	}

	return realsize;
}

/* callback used by curl for writing the received data */
size_t ccurl_write_memory_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	ccurl_memory_t *mem = (ccurl_memory_t*)data;

	mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}


/* fetch our datums */
void ccurl_fetch_data(thread_handle &handle, const ccurl_location_ptr &curloc)
{
	CURL *curl = NULL;
	CURLcode res;
	struct curl_slist *headers = NULL;

	// curl temps
	ccurl_memory_t chunk;
	ccurl_headers_t response_headers;

	chunk.memory = NULL;
	chunk.size = 0;
	memset(&response_headers, 0, sizeof(ccurl_headers_t));

	curl = curl_easy_init();
	if (curl) {
		DBGP("reading curl data from '%s'", curloc->uri);
		curl_easy_setopt(curl, CURLOPT_URL, curloc->uri);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, ccurl_parse_header_callback);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *) &response_headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ccurl_write_memory_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "conky-curl/1.1");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1000);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60);

		if (curloc->last_modified) {
			const char *header = "If-Modified-Since: ";
			int len = strlen(header) + strlen(curloc->last_modified) + 1;
			char *str = (char*) malloc(len);
			snprintf(str, len, "%s%s", header, curloc->last_modified);
			headers = curl_slist_append(headers, str);
			free(str);
		}
		if (curloc->etag) {
			const char *header = "If-None-Match: ";
			int len = strlen(header) + strlen(curloc->etag) + 1;
			char *str = (char*) malloc(len);
			snprintf(str, len, "%s%s", header, curloc->etag);
			headers = curl_slist_append(headers, str);
			free(str);
		}
		if (headers) {
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		}

		res = curl_easy_perform(curl);
		if (res == CURLE_OK) {
			long http_status_code;

			if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
						&http_status_code) == CURLE_OK) {
				switch (http_status_code) {
					case 200:
						{
							std::lock_guard<std::mutex> lock(handle.mutex());
							free_and_zero(curloc->last_modified);
							free_and_zero(curloc->etag);
							if (response_headers.last_modified) {
								curloc->last_modified =
									strdup(response_headers.last_modified);
							}
							if (response_headers.etag) {
								curloc->etag = strdup(response_headers.etag);
							}
							curloc->process_function(curloc->result, chunk.memory);
						}
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
			free(chunk.memory);
		} else {
			NORM_ERR("curl: could not retrieve data from server");
		}

		free_and_zero(response_headers.last_modified);
		free_and_zero(response_headers.etag);
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}
}

void ccurl_thread(thread_handle &handle, const ccurl_location_ptr &curloc);

void ccurl_init_thread(const ccurl_location_ptr &curloc, int interval)
{
#ifdef DEBUG
	assert(curloc->result);
#endif /* DEBUG */
	curloc->p_timed_thread = timed_thread::create(std::bind(ccurl_thread,
				std::placeholders::_1, curloc), std::chrono::seconds(interval));

	if (!curloc->p_timed_thread) {
		NORM_ERR("curl thread: error creating timed thread");
	}
}

void ccurl_thread(thread_handle &handle, const ccurl_location_ptr &curloc)
{

	while (1) {
		ccurl_fetch_data(handle, curloc);
		if (handle.test(0)) {
			return;
		}
	}
	/* never reached */
}


/*
 * This is where the $curl section begins.
 */

struct curl_data {
	char uri[128];
	float interval;
};

/* internal location pointer for use by $curl, no touchy */
static ccurl_location_list ccurl_locations;

/* used to free data used by $curl */
void ccurl_free_info(void)
{
	ccurl_free_locations(ccurl_locations);
}

/* straight copy, used by $curl */
static void ccurl_parse_data(char *result, const char *data)
{
	if(result) strncpy(result, data, max_user_text);
}

/* prints result data to text buffer, used by $curl */
void ccurl_process_info(char *p, int p_max_size, char *uri, int interval)
{
	ccurl_location_ptr curloc = ccurl_find_location(ccurl_locations, uri);
	if (!curloc->p_timed_thread) {
		curloc->result = (char*)malloc(max_user_text);
		memset(curloc->result, 0, max_user_text);
		curloc->process_function = std::bind(ccurl_parse_data,
				std::placeholders::_1, std::placeholders::_2);
		ccurl_init_thread(curloc, interval);
		if (!curloc->p_timed_thread) {
			NORM_ERR("error setting up curl thread");
		}
	}


	std::lock_guard<std::mutex> lock(curloc->p_timed_thread->mutex());
	strncpy(p, curloc->result, p_max_size);
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
		cd->interval = interval > 0 ? interval * 60 : update_interval;
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
