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
#include "core.h"
#include "logging.h"
#include "specials.h"
#include "text_object.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>


static inline void read_file(const char *data, char *buf, const int size)
{
	FILE *fp;

	memset(buf, 0, size);

	if (!data)
		return;

	fp = fopen(data, "r");
	if(fp) {
		int length;

		length = fread(buf, 1, size - 1, fp);
		fclose(fp);
		buf[length] = '\0';
		if (length > 0 && buf[length - 1] == '\n') {
			buf[length - 1] = '\0';
		}
	} else {
		buf[0] = '\0';
	}
}

static inline unsigned int file_buffer_size(const char *data, const unsigned int maxsize)
{
        struct stat buf;
        if(stat(data, &buf))
                return maxsize;
        if(buf.st_size < 0 || buf.st_size > maxsize)
                return maxsize;
        if(buf.st_size < 10)
                return 10;
        return buf.st_size + 1;
}

void print_cat(struct text_object *obj, char *p, unsigned int p_max_size)
{
	read_file(obj->data.s, p, p_max_size);
}

void print_catp(struct text_object *obj, char *p, unsigned int p_max_size)
{
	const unsigned int sz = file_buffer_size(obj->data.s, text_buffer_size.get(*state));
	char * buf = new char[sz];

	read_file(obj->data.s, buf, sz);

	evaluate(buf, p, p_max_size);

	delete[] buf;
}

void print_cap(struct text_object *obj, char *p, unsigned int p_max_size) {
  unsigned int x = 0;
  int z = 0;
  char buf[p_max_size];
  char *src = obj->data.s;
  char *dest = buf;

  evaluate(obj->data.s, p, p_max_size);
  if (0 != strcmp(p, "")) {
    src = p;
  }

  for (; *src && p_max_size-1 > x; src++, x++) {
    if (0 == z) {
      *dest++ = (toupper((unsigned char) *src));
      z++;
      continue;
    }
    *dest++ = *src;
    if (' ' == *src) z = 0;
  }
  *dest = '\0';
  snprintf(p, p_max_size, "%s", buf);
}


long long int apply_base_multiplier(const char *s, long long int num) {
  long long int base = 1024LL;
  if (*s && (0 == (strcmp(s, "si")))) {
    base = 1000LL;
  }
  return (num * base);
}
