/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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
#include <stdio.h>
#include <sys/types.h>
#include "defconfig.h"

#if defined(HAVE_FOPENCOOKIE)
#define COOKIE_LEN_T	size_t
#define COOKIE_RET_T	ssize_t
#else
#define COOKIE_LEN_T	int
#define COOKIE_RET_T	int
#endif

#if defined(HAVE_FOPENCOOKIE) || defined(HAVE_FUNOPEN)
static COOKIE_RET_T
conf_read(void *cookie, char *buf, COOKIE_LEN_T size)
{
	static int col = 0, row = 0;
	COOKIE_LEN_T i = 0;
	const char *conf[] = defconfig;

	(void)cookie;

	while (i < size) {
		if (!(conf[row]))		/* end of rows */
			break;
		if (!(conf[row][col])) {	/* end of line */
			row++;
			col = 0;
			continue;
		}
		buf[i++] = conf[row][col++];
	}
	return i;
}
#endif /* defined(HAVE_FOPENCOOKIE) || defined(HAVE_FUNOPEN) */

#if defined(HAVE_FOPENCOOKIE)
static cookie_io_functions_t conf_cookie;
FILE *conf_cookie_open(void)
{
	conf_cookie.read = &conf_read;
	conf_cookie.write = NULL;
	conf_cookie.seek = NULL;
	conf_cookie.close = NULL;
	return fopencookie(NULL, "r", conf_cookie);
}
#elif defined(HAVE_FUNOPEN)
FILE *conf_cookie_open(void)
{
	return funopen(NULL, &conf_read, NULL, NULL, NULL);
}
#else
FILE *conf_cookie_open(void) { return NULL; }
#endif
