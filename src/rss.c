/* Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2007 Toni Spets
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
#include "prss.h"
#include "ccurl_thread.h"
#include <time.h>
#include <assert.h>

static ccurl_location_t *locations_head = 0;

void rss_free_info(void)
{
	ccurl_location_t *tail = locations_head;

	while (tail) {
		if (tail->result) prss_free((PRSS*)tail->result);	/* clean up old data */
		tail = tail->next;
	}
	ccurl_free_locations(&locations_head);
}

void rss_process_info(char *p, int p_max_size, char *uri, char *action, int
		act_par, int interval, unsigned int nrspaces)
{
	PRSS *data;
	char *str;

	ccurl_location_t *curloc = ccurl_find_location(&locations_head, uri);
	if (!curloc->p_timed_thread) {
		curloc->result = malloc(sizeof(PRSS));
		memset(curloc->result, 0, sizeof(PRSS));
		curloc->process_function = &prss_parse_data;
		ccurl_init_thread(curloc, interval);
		if (!curloc->p_timed_thread) {
			ERR("error setting up RSS thread");
		}
	}

	timed_thread_lock(curloc->p_timed_thread);
	data = (PRSS*)curloc->result;

	if (data == NULL) {
		snprintf(p, p_max_size, "prss: Error reading RSS data\n");
	} else {
		if (strcmp(action, "feed_title") == EQUAL) {
		        if ((str = data->title)) {
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
				if (str[strlen(str) - 1] == '\n') {
					str[strlen(str) - 1] = 0;
				}
				snprintf(p, p_max_size, "%s", str);
			}
		} else if (strcmp(action, "item_desc") == EQUAL) {
			if (act_par < data->item_count) {
				str =
					data->items[act_par].description;
				// remove trailing new line if one exists
				if (str[strlen(str) - 1] == '\n') {
					str[strlen(str) - 1] = 0;
				}
				snprintf(p, p_max_size, "%s", str);
			}
		} else if (strcmp(action, "item_titles") == EQUAL) {
			if (data->item_count > 0) {
				int itmp;
				int show;
				//'tmpspaces' is a string with spaces too be placed in front of each title
				char *tmpspaces = malloc(nrspaces + 1);
				memset(tmpspaces, ' ', nrspaces);
				tmpspaces[nrspaces]=0;

				p[0] = 0;

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
						if (str[strlen(str) - 1] == '\n') {
							str[strlen(str) - 1] = 0;
						}
						strncat(p, tmpspaces, p_max_size);
						strncat(p, str, p_max_size);
					}
				}
				free(tmpspaces);
			}
		}
	}
	timed_thread_unlock(curloc->p_timed_thread);
}

