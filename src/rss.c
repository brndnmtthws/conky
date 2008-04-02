/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2007 Toni Spets
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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
 * $Id$ */

#include "conky.h"
#include "prss.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

#define MAX_FEEDS 16

struct MemoryStruct {
	char *memory;
	size_t size;
};

typedef struct feed_ {
	char *uri;
	int last_update;
	PRSS *data;
} feed;

int num_feeds = 0;
feed feeds[MAX_FEEDS];

size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *) data;

	mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory) {
		memcpy(&(mem->memory[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = 0;
	}
	return realsize;
}

int rss_delay(int *wait, int delay)
{
	time_t now = time(NULL);

	// make it minutes
	if (delay < 1) {
		delay = 1;
	}
	delay *= 60;

	if (!*wait) {
		*wait = now + delay;
		return 1;
	}

	if (now >= *wait + delay) {
		*wait = now + delay;
		return 1;
	}

	return 0;
}

void init_rss_info(void)
{
	int i;

	for (i = 0; i < MAX_FEEDS; i++) {
		feeds[i].uri = NULL;
		feeds[i].data = NULL;
		feeds[i].last_update = 0;
	}
}

void free_rss_info(void)
{
	int i;

	for (i = 0; i < num_feeds; i++) {
		if (feeds[i].uri != NULL) {
			free(feeds[i].uri);
		}
	}
}

PRSS *get_rss_info(char *uri, int delay)
{
	CURL *curl = NULL;
	CURLcode res;

	// pointers to struct
	feed *curfeed = NULL;
	PRSS *curdata = NULL;
	int *last_update = 0;

	int i;

	// curl temps
	struct MemoryStruct chunk;

	chunk.memory = NULL;
	chunk.size = 0;

	// first seek for the uri in list
	for (i = 0; i < num_feeds; i++) {
		if (feeds[i].uri != NULL) {
			if (!strcmp(feeds[i].uri, uri)) {
				curfeed = &feeds[i];
				break;
			}
		}
	}

	if (!curfeed) {	// new feed
		if (num_feeds == MAX_FEEDS - 1) {
			return NULL;
		}
		curfeed = &feeds[num_feeds];
		curfeed->uri = strndup(uri, text_buffer_size);
		num_feeds++;
	}

	last_update = &curfeed->last_update;
	curdata = curfeed->data;

	if (!rss_delay(last_update, delay)) {
		return curdata;	// wait for delay to pass
	}

	if (curdata != NULL) {
		prss_free(curdata);	// clean up old data
		curdata = NULL;
	}

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, uri);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "conky-rss/1.0");

		res = curl_easy_perform(curl);
		if (chunk.size) {
			curdata = prss_parse_data(chunk.memory);
			free(chunk.memory);
		} else {
			ERR("No data from server");
		}

		curl_easy_cleanup(curl);
	}

	curfeed->data = curdata;

	return curdata;
}
