/*
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
 */

#ifndef _CURL_THREAD_H_
#define _CURL_THREAD_H_

#include "timed_thread.h"

typedef struct _ccurl_location_t {
	char *uri; /* uri of location */
	void *result; /* a pointer to some arbitrary data, will be freed by ccurl_free_info() if non-null */
	timed_thread *p_timed_thread; /* internal thread pointer */
	void (*process_function)(void *, const char *); /* function to call when data is ready to be processed, the first argument will be the result pointer, and the second argument is an internal buffer that shouldn't be mangled */
	struct _ccurl_location_t *next; /* internal list pointer */
} ccurl_location_t;

/* find an existing location for the uri specified */
ccurl_location_t *ccurl_find_location(ccurl_location_t **locations_head, char *uri);
/* free all locations (as well as location->uri and location->result if
 * non-null) */
void ccurl_free_locations(ccurl_location_t **locations_head);
/* initiates a thread at the location specified using the interval in seconds */
void ccurl_init_thread(ccurl_location_t *curloc, int interval);

/* for $curl, free internal list pointer */
void ccurl_free_info(void);
/* runs instance of $curl */
void ccurl_process_info(char *p, int p_max_size, char *uri, int interval);

#endif /* _CURL_THREAD_H_ */

