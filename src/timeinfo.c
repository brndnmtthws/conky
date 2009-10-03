/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
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

#include "text_object.h"
#include <locale.h>

void scan_time(struct text_object *obj, const char *arg)
{
	obj->data.s = strndup(arg ? arg : "%F %T", text_buffer_size);
}

void scan_tztime(struct text_object *obj, const char *arg)
{
	char buf1[256], buf2[256], *fmt, *tz;

	fmt = tz = NULL;
	if (arg) {
		int nArgs = sscanf(arg, "%255s %255[^\n]", buf1, buf2);

		switch (nArgs) {
			case 2:
				tz = buf1;
			case 1:
				fmt = buf2;
		}
	}

	obj->data.tztime.fmt = strndup(fmt ? fmt : "%F %T", text_buffer_size);
	obj->data.tztime.tz = tz ? strndup(tz, text_buffer_size) : NULL;
}

void print_time(struct text_object *obj, char *p, int p_max_size)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	setlocale(LC_TIME, "");
	strftime(p, p_max_size, obj->data.s, tm);
}

void print_utime(struct text_object *obj, char *p, int p_max_size)
{
	time_t t = time(NULL);
	struct tm *tm = gmtime(&t);

	setlocale(LC_TIME, "");
	strftime(p, p_max_size, obj->data.s, tm);
}

void print_tztime(struct text_object *obj, char *p, int p_max_size)
{
	char *oldTZ = NULL;
	time_t t;
	struct tm *tm;

	if (obj->data.tztime.tz) {
		oldTZ = getenv("TZ");
		setenv("TZ", obj->data.tztime.tz, 1);
		tzset();
	}
	t = time(NULL);
	tm = localtime(&t);

	setlocale(LC_TIME, "");
	strftime(p, p_max_size, obj->data.tztime.fmt, tm);
	if (oldTZ) {
		setenv("TZ", oldTZ, 1);
		tzset();
	} else {
		unsetenv("TZ");
	}
	// Needless to free oldTZ since getenv gives ptr to static data
}

void free_time(struct text_object *obj)
{
	if (!obj->data.s)
		return;
	free(obj->data.s);
	obj->data.s = NULL;
}

void free_tztime(struct text_object *obj)
{
	if (obj->data.tztime.tz) {
		free(obj->data.tztime.tz);
		obj->data.tztime.tz = NULL;
	}
	if (obj->data.tztime.fmt) {
		free(obj->data.tztime.fmt);
		obj->data.tztime.fmt = NULL;
	}
}
