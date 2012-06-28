/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
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

#include "config.h"
#include "logging.h"
#include "text_object.h"
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ICONV_CODEPAGE_LENGTH 20

static long iconv_selected;
static long iconv_count = 0;
static char iconv_converting = 0;
static iconv_t **iconv_cd = 0;

int register_iconv(iconv_t *new_iconv)
{
	iconv_cd = (void ***) realloc(iconv_cd, sizeof(iconv_t *) * (iconv_count + 1));
	if (!iconv_cd) {
		CRIT_ERR(NULL, NULL, "Out of memory");
	}
	iconv_cd[iconv_count] = (void **) malloc(sizeof(iconv_t));
	if (!iconv_cd[iconv_count]) {
		CRIT_ERR(NULL, NULL, "Out of memory");
	}
	memcpy(iconv_cd[iconv_count], new_iconv, sizeof(iconv_t));
	iconv_count++;
	return iconv_count;
}

void free_iconv(struct text_object *obj)
{
	long i;

	(void)obj;

	if (!iconv_cd)
		return;

	for (i = 0; i < iconv_count; i++) {
		if (iconv_cd[i]) {
			iconv_close(*iconv_cd[i]);
			free(iconv_cd[i]);
		}
	}
	free(iconv_cd);
	iconv_cd = 0;
}

void iconv_convert(size_t *a, char *buff_in, char *p, size_t p_max_size)
{
	int bytes;
	size_t dummy1, dummy2;
#if defined(__FreeBSD__) || defined(__DragonFly__)
	const char *ptr = buff_in;
#else
	char *ptr = buff_in;
#endif
	char *outptr = p;

	if (*a <= 0 || !iconv_converting || iconv_selected <= 0
			|| iconv_cd[iconv_selected - 1] == (iconv_t) (-1))
		return;

	dummy1 = dummy2 = *a;

	strncpy(buff_in, p, p_max_size);

	iconv(*iconv_cd[iconv_selected - 1], NULL, NULL, NULL, NULL);
	while (dummy1 > 0) {
		bytes = iconv(*iconv_cd[iconv_selected - 1], &ptr, &dummy1,
				&outptr, &dummy2);
		if (bytes == -1) {
			NORM_ERR("Iconv codeset conversion failed");
			break;
		}
	}

	/* It is nessecary when we are converting from multibyte to
	 * singlebyte codepage */
	//a = outptr - p;
	//(*a) = *a - dummy2;
	(*a) = outptr - p;
}

void init_iconv_start(struct text_object *obj, void *free_at_crash, const char *arg)
{
	char iconv_from[ICONV_CODEPAGE_LENGTH];
	char iconv_to[ICONV_CODEPAGE_LENGTH];

	if (iconv_converting) {
		CRIT_ERR(obj, free_at_crash, "You must stop your last iconv conversion before "
				"starting another");
	}
	if (sscanf(arg, "%s %s", iconv_from, iconv_to) != 2) {
		CRIT_ERR(obj, free_at_crash, "Invalid arguments for iconv_start");
	} else {
		iconv_t new_iconv;

		new_iconv = iconv_open(iconv_to, iconv_from);
		if (new_iconv == (iconv_t) (-1)) {
			NORM_ERR("Can't convert from %s to %s.", iconv_from, iconv_to);
		} else {
			obj->data.i = register_iconv(&new_iconv);
			iconv_converting = 1;
		}
	}
}

void init_iconv_stop(void)
{
	iconv_converting = 0;
}

void print_iconv_start(struct text_object *obj, char *p, int p_max_size)
{
	(void)p;
	(void)p_max_size;

	iconv_converting = 1;
	iconv_selected = obj->data.i;
}

void print_iconv_stop(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	(void)p;
	(void)p_max_size;

	iconv_converting = 0;
	iconv_selected = 0;
}
