/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2007 Toni Spets
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
#include "prss.h"
#include "text_object.h"
#include "ccurl_thread.h"
#include <time.h>
#include <assert.h>
#include <mutex>

struct rss_data {
	char uri[128];
	char action[64];
	int act_par;
	float interval;
	unsigned int nrspaces;
};

static ccurl_location_list locations;

void rss_free_info(void)
{
	ccurl_free_locations(locations);
}

static void rss_process_info(char *p, int p_max_size, char *uri, char *action, int
		act_par, int interval, unsigned int nrspaces)
{
	PRSS *data;
	char *str;

	ccurl_location_ptr curloc = ccurl_find_location(locations, uri);

	assert(act_par >= 0 && action);

	if (!curloc->p_timed_thread) {
		curloc->result = (char*)malloc(sizeof(PRSS));
		memset(curloc->result, 0, sizeof(PRSS));
		curloc->process_function = std::bind(prss_parse_data,
				std::placeholders::_1, std::placeholders::_2);
		ccurl_init_thread(curloc, interval);
		if (!curloc->p_timed_thread) {
			NORM_ERR("error setting up RSS thread");
		}
	}

	std::lock_guard<std::mutex> lock(curloc->p_timed_thread->mutex());
	data = (PRSS*)curloc->result;

	if (data == NULL || data->item_count < 1) {
		*p = 0;
	} else {
		/*
		 * XXX: Refactor this so that we can retrieve any of the fields in the
		 * PRSS struct (in prss.h).
		 */
		if (strcmp(action, "feed_title") == EQUAL) {
			str = data->title;
			if (str && strlen(str) > 0) {
				// remove trailing new line if one exists
				if (str[strlen(str) - 1] == '\n') {
					str[strlen(str) - 1] = 0;
				}
				snprintf(p, p_max_size, "%s", str);
			}
		} else if (strcmp(action, "item_title") == EQUAL) {
			if (act_par < data->item_count) {
				str = data->items[act_par].title;
				// remove trailing new line if one exists
				if (str && strlen(str) > 0) {
					if (str[strlen(str) - 1] == '\n') {
						str[strlen(str) - 1] = 0;
					}
					snprintf(p, p_max_size, "%s", str);
				}
			}
		} else if (strcmp(action, "item_desc") == EQUAL) {
			if (act_par < data->item_count) {
				str =
					data->items[act_par].description;
				// remove trailing new line if one exists
				if (str && strlen(str) > 0) {
					if (str[strlen(str) - 1] == '\n') {
						str[strlen(str) - 1] = 0;
					}
					snprintf(p, p_max_size, "%s", str);
				}
			}
		} else if (strcmp(action, "item_titles") == EQUAL) {
			if (data->item_count > 0) {
				int itmp;
				int show;
				//'tmpspaces' is a string with spaces too be placed in front of each title
				char *tmpspaces = (char*)malloc(nrspaces + 1);
				memset(tmpspaces, ' ', nrspaces);
				tmpspaces[nrspaces]=0;

				if (act_par > data->item_count) {
					show = data->item_count;
				} else {
					show = act_par;
				}
				for (itmp = 0; itmp < show; itmp++) {
					PRSS_Item *item = &data->items[itmp];

					str = item->title;
					if (str) {
						// don't add new line before first item
						if (itmp > 0) {
							strncat(p, "\n", p_max_size);
						}
						/* remove trailing new line if one exists,
						 * we have our own */
						if (strlen(str) > 0 && str[strlen(str) - 1] == '\n') {
							str[strlen(str) - 1] = 0;
						}
						strncat(p, tmpspaces, p_max_size);
						strncat(p, str, p_max_size);
					}
				}
				free(tmpspaces);
			}
		} else {
			NORM_ERR("rss: Invalid action '%s'", action);
		}
	}
}

void rss_scan_arg(struct text_object *obj, const char *arg)
{
	int argc;
	struct rss_data *rd;

	rd = (struct rss_data *)malloc(sizeof(struct rss_data));
	memset(rd, 0, sizeof(struct rss_data));

	argc = sscanf(arg, "%127s %f %63s %d %u", rd->uri, &rd->interval, rd->action,
			&rd->act_par, &rd->nrspaces);
	if (argc < 3) {
		NORM_ERR("wrong number of arguments for $rss");
		free(rd);
		return;
	}
	obj->data.opaque = rd;
}

void rss_print_info(struct text_object *obj, char *p, int p_max_size)
{
	struct rss_data *rd = (struct rss_data *)obj->data.opaque;

	if (!rd) {
		NORM_ERR("error processing RSS data");
		return;
	}
	rss_process_info(p, p_max_size, rd->uri, rd->action,
			rd->act_par, rd->interval, rd->nrspaces);
}

void rss_free_obj_info(struct text_object *obj)
{
	free_and_zero(obj->data.opaque);
}
