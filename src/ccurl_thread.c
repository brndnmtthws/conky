/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
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
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 */

#include "conky.h"
#include "logging.h"
#include "ccurl_thread.h"

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

/* finds a location based on uri in the list provided */
ccurl_location_t *ccurl_find_location(ccurl_location_t **locations_head, char *uri)
{
	ccurl_location_t *tail = *locations_head;
	ccurl_location_t *new = 0;
	while (tail) {
		if (tail->uri &&
				strcmp(tail->uri, uri) == EQUAL) {
			return tail;
		}
		tail = tail->next;
	}
	if (!tail) { /* new location!!!!!!! */
		DBGP("new curl location: '%s'", uri);
		new = malloc(sizeof(ccurl_location_t));
		memset(new, 0, sizeof(ccurl_location_t));
		new->uri = strndup(uri, text_buffer_size);
		tail = *locations_head;
		while (tail && tail->next) {
			tail = tail->next;
		}
		if (!tail) {
			/* omg the first one!!!!!!! */
			*locations_head = new;
		} else {
			tail->next = new;
		}
	}
	return new;
}

/* iterates over the list provided, frees stuff (list item, uri, result) */
void ccurl_free_locations(ccurl_location_t **locations_head)
{
	ccurl_location_t *tail = *locations_head;
	ccurl_location_t *last = 0;

	while (tail) {
		if (tail->uri) free(tail->uri);
		if (tail->result) free(tail->result);
		last = tail;
		tail = tail->next;
		free(last);
	}
	*locations_head = 0;
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
void ccurl_fetch_data(ccurl_location_t *curloc)
{
	CURL *curl = NULL;
	CURLcode res;

	// curl temps
	ccurl_memory_t chunk;

	chunk.memory = NULL;
	chunk.size = 0;

	curl = curl_easy_init();
	if (curl) {
		DBGP("reading curl data from '%s'", curloc->uri);
		curl_easy_setopt(curl, CURLOPT_URL, curloc->uri);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ccurl_write_memory_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "conky-curl/1.0");

		res = curl_easy_perform(curl);
		if (res == CURLE_OK && chunk.size) {
			timed_thread_lock(curloc->p_timed_thread);
			(*curloc->process_function)(curloc->result, chunk.memory);
			timed_thread_unlock(curloc->p_timed_thread);
			free(chunk.memory);
		} else {
			ERR("curl: no data from server");
		}

		curl_easy_cleanup(curl);
	}
}

void *ccurl_thread(void *) __attribute__((noreturn));

void ccurl_init_thread(ccurl_location_t *curloc, int interval)
{
#ifdef DEBUG
	assert(curloc->result);
#endif /* DEBUG */
	curloc->p_timed_thread =
		timed_thread_create(&ccurl_thread,
				(void *)curloc, interval * 1000000);

	if (!curloc->p_timed_thread) {
		ERR("curl thread: error creating timed thread");
	}
	timed_thread_register(curloc->p_timed_thread,
			&curloc->p_timed_thread);
	if (timed_thread_run(curloc->p_timed_thread)) {
		ERR("curl thread: error running timed thread");
	}
}

void *ccurl_thread(void *arg)
{
	ccurl_location_t *curloc = (ccurl_location_t*)arg;

	while (1) {
		ccurl_fetch_data(curloc);
		if (timed_thread_test(curloc->p_timed_thread, 0)) {
			timed_thread_exit(curloc->p_timed_thread);
		}
	}
	/* never reached */
}


/*
 * This is where the $curl section begins.
 */

/* internal location pointer for use by $curl, no touchy */
static ccurl_location_t *ccurl_locations_head = 0;

/* used to free data used by $curl */
void ccurl_free_info(void)
{
	ccurl_free_locations(&ccurl_locations_head);
}

/* straight copy, used by $curl */
void ccurl_parse_data(void *result, const char *data)
{
	strncpy(result, data, max_user_text);
}

/* prints result data to text buffer, used by $curl */
void ccurl_process_info(char *p, int p_max_size, char *uri, int interval)
{
	ccurl_location_t *curloc = ccurl_find_location(&ccurl_locations_head, uri);
	if (!curloc->p_timed_thread) {
		curloc->result = malloc(max_user_text);
		memset(curloc->result, 0, max_user_text);
		curloc->process_function = &ccurl_parse_data;
		ccurl_init_thread(curloc, interval);
		if (!curloc->p_timed_thread) {
			ERR("error setting up curl thread");
		}
	}

	timed_thread_lock(curloc->p_timed_thread);
	strncpy(p, curloc->result, p_max_size);
	timed_thread_unlock(curloc->p_timed_thread);
}

