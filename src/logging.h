/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
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

#ifdef __cplusplus
extern "C" {
#endif

void clean_up(void *memtofree1, void* memtofree2);

#ifndef _LOGGING_H
#define _LOGGING_H

#define NORM_ERR(...) { \
	fprintf(stderr, PACKAGE_NAME": "); \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, "\n"); \
}

/* critical error */
#define CRIT_ERR(memtofree1, memtofree2, ...) \
	{ NORM_ERR(__VA_ARGS__); clean_up(memtofree1, memtofree2); exit(EXIT_FAILURE); }

/* debugging output */
extern int global_debug_level;
#define __DBGP(level, ...) \
	if (global_debug_level > level) { \
		fprintf(stderr, "DEBUG(%d) [" __FILE__ ":%d]: ", level, __LINE__); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, "\n"); \
	}
#define DBGP(...) __DBGP(0, __VA_ARGS__)
#define DBGP2(...) __DBGP(1, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* _LOGGING_H */
