/*
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
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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
#ifndef _TIMEINFO_H
#define _TIMEINFO_H

#include "setting.hh"

extern conky::simple_config_setting<bool> times_in_seconds;

/* since time and utime are quite equal, certain functions
 * are shared in between both text object types. */

/* parse args passed to *time objects */
void scan_time(struct text_object *, const char *);
void scan_tztime(struct text_object *, const char *);

/* print the time */
void print_time(struct text_object *, char *, unsigned int);
void print_utime(struct text_object *, char *, unsigned int);
void print_tztime(struct text_object *, char *, unsigned int);
void print_format_time(struct text_object *obj, char *p, unsigned int p_max_size);

/* free object data */
void free_time(struct text_object *);
void free_tztime(struct text_object *);

#endif /* _TIMEINFO_H */
