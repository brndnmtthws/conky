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

#include "config.h"

#include "timeinfo.h"

#include "conky.h"
#include "text_object.h"
#include <locale.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "logging.h"

#include <memory>

struct tztime_s {
	char *tz;	/* timezone variable */
	char *fmt;	/* time display formatting */
};

conky::simple_config_setting<bool> times_in_seconds("times_in_seconds", false, false);

void scan_time(struct text_object *obj, const char *arg)
{
	obj->data.opaque = strndup(arg ? arg : "%F %T", text_buffer_size);
}

void scan_tztime(struct text_object *obj, const char *arg)
{
	char buf1[256], buf2[256], *fmt, *tz;
	struct tztime_s *ts;

	fmt = tz = NULL;
	if (arg) {
		int nArgs = sscanf(arg, "%255s %255[^\n]", buf1, buf2);

		switch (nArgs) {
			case 2:
				fmt = buf2;
			case 1:
				tz = buf1;
		}
	}

	ts = (tztime_s*) malloc(sizeof(struct tztime_s));
	memset(ts, 0, sizeof(struct tztime_s));
	ts->fmt = strndup(fmt ? fmt : "%F %T", text_buffer_size);
	ts->tz = tz ? strndup(tz, text_buffer_size) : NULL;
	obj->data.opaque = ts;
}

void print_time(struct text_object *obj, char *p, int p_max_size)
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	setlocale(LC_TIME, "");
	strftime(p, p_max_size, (char *)obj->data.opaque, tm);
}

void print_utime(struct text_object *obj, char *p, int p_max_size)
{
	time_t t = time(NULL);
	struct tm *tm = gmtime(&t);

	setlocale(LC_TIME, "");
	strftime(p, p_max_size, (char *)obj->data.opaque, tm);
}

void print_tztime(struct text_object *obj, char *p, int p_max_size)
{
	char *oldTZ = NULL;
	time_t t;
	struct tm *tm;
	struct tztime_s *ts = (tztime_s*) obj->data.opaque;

	if (!ts)
		return;

	if (ts->tz) {
		oldTZ = getenv("TZ");
		setenv("TZ", ts->tz, 1);
		tzset();
	}
	t = time(NULL);
	tm = localtime(&t);

	setlocale(LC_TIME, "");
	strftime(p, p_max_size, ts->fmt, tm);
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
	free_and_zero(obj->data.opaque);
}

void free_tztime(struct text_object *obj)
{
	struct tztime_s *ts = (tztime_s*) obj->data.opaque;

	if (!ts)
		return;

	free_and_zero(ts->tz);
	free_and_zero(ts->fmt);

	free_and_zero(obj->data.opaque);
}

/* a safer asprintf()
 * - no need to check for errors
 * - exit conky on memory allocation failure
 * - XXX: no return value at all, otherwise this
 *        could be used globally */
#define safe_asprintf(bufp, ...) { \
	int __v; \
	if ((__v = asprintf(bufp, __VA_ARGS__)) == -1) { \
		fprintf(stderr, "%s: memory allocation failed\n", __func__); \
		exit(__v); \
	} \
}

//all chars after the ending " and between the seconds and the starting " are silently
//ignored, this is wanted behavior, not a bug, so don't "fix" this.
static void do_format_time(struct text_object *obj, char *p, unsigned int p_max_size) {
	double seconds;
	char *currentchar, *temp;
	unsigned int output_length = 0;
	int minutes, hours, days, weeks;
	char show_minutes = 0, show_hours = 0, show_days = 0, show_weeks = 0, hidestring;

	if (not times_in_seconds.get(*state)) {
		NORM_ERR("Enable \"times_in_seconds\" to use $format_time");
		return;
	}

	errno = 0;
	seconds = strtod(obj->data.s, &currentchar);
	if(errno == 0 && obj->data.s != currentchar) {
		while(*currentchar != 0 && *currentchar != '"') {
			currentchar++;
		}
		if(*currentchar != 0) {
			currentchar++;
			minutes = seconds / 60;
			seconds -= minutes * 60;
			hours = minutes / 60;
			minutes %= 60;
			days = hours / 24;
			hours %= 24;
			weeks = days / 7;
			days %= 7;
			for(temp = currentchar; *temp != 0 && *temp != '"'; temp++) {
				if(*temp=='\\') {
					switch(*(temp+1)) {
					case '\\':
						temp++;
						break;
					case 'w':
						show_weeks = 1;
						break;
					case 'd':
						show_days = 1;
						break;
					case 'h':
						show_hours = 1;
						break;
					case 'm':
						show_minutes = 1;
						break;
					}
				}
			}
			if(show_weeks == 0) days += weeks * 7;
			if(show_days == 0) hours += days * 24;
			if(show_hours == 0) minutes += hours * 60;
			if(show_minutes == 0) seconds += minutes * 60;
			hidestring = 0;
			while(output_length < p_max_size - 1) {
				if(*currentchar != 0 && *currentchar != '"') {
					temp = NULL;
					if(*currentchar == '\\' && hidestring == 0) {
						currentchar++;
						switch(*currentchar){
						case 'w':
							safe_asprintf(&temp, "%d", weeks);
							break;
						case 'd':
							safe_asprintf(&temp, "%d", days);
							break;
						case 'h':
							safe_asprintf(&temp, "%d", hours);
							break;
						case 'm':
							safe_asprintf(&temp, "%d", minutes);
							break;
						case 's':
							safe_asprintf(&temp, "%d", (int) seconds);
							break;
						case 'S':
							currentchar++;
							if(*currentchar >= '0' && *currentchar <= '9') {
								safe_asprintf(&temp, "%.*f", (*currentchar) - '0', seconds);
							} else if(*currentchar == 'x') {
								if(seconds == (int) seconds ) {
									safe_asprintf(&temp, "%d", (int) seconds);
								} else {
									safe_asprintf(&temp, "%.9f", seconds);
									while(*(temp + strlen(temp) - 1) == '0' || *(temp + strlen(temp) - 1) == '.') {
										*(temp + strlen(temp) - 1) = 0;
									}
								}
							}else{
								currentchar--;
								NORM_ERR("$format_time needs a digit behind 'S' to specify precision");
							}
							break;
						case '\\':
						case '(':
						case ')':
							p[output_length] = *currentchar;
							output_length++;
							break;
						default:
							NORM_ERR("$format_time doesn't have a special char '%c'", *currentchar);
						}
					} else if(*currentchar == '(') {
						for(temp = currentchar + 1; *temp != 0 && *temp != ')'; temp++) {
							if(*(temp-1) == '\\') {
								switch(*temp) {
								case 'w':
									if(weeks == 0) hidestring = 1;
									break;
								case 'd':
									if(days == 0) hidestring = 1;
									break;
								case 'h':
									if(hours == 0) hidestring = 1;
									break;
								case 'm':
									if(minutes == 0) hidestring = 1;
									break;
								case 's':
								case 'S':
									if(seconds == 0) hidestring = 1;
									break;
								}
							}
						}
						temp = NULL;
					} else if(*currentchar == ')') {
						hidestring = 0;
					} else if(hidestring == 0) {
						p[output_length] = *currentchar;
						output_length++;
					}
					if(temp) {
						if(output_length + strlen(temp) < p_max_size - 1) {
							strcpy(p + output_length, temp);
							output_length += strlen(temp);
						} else NORM_ERR("The format string for $format_time is too long");
						free(temp);
					}
					currentchar++;
				} else break;
			}
			p[output_length] = 0;
		} else {
			NORM_ERR("$format_time needs a output-format starting with a \"-char as 2nd argument");
		}
	} else {
		NORM_ERR("$format_time didn't receive a time in seconds as first argument");
	}
}

void print_format_time(struct text_object *obj, char *p, int p_max_size)
{
	std::unique_ptr<char []> buf(new char[max_user_text]);

	generate_text_internal(buf.get(), max_user_text, *obj->sub);
	obj->data.s = buf.get();
	do_format_time(obj, p, p_max_size);
}

