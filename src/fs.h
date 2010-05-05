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

#ifndef _FS_H
#define _FS_H

#include "conky.h"	/* DEFAULT_TEXT_BUFFER_SIZE */

/* needed here and by fs.c */
struct fs_stat {
	char path[DEFAULT_TEXT_BUFFER_SIZE];
	char type[DEFAULT_TEXT_BUFFER_SIZE];
	long long size;
	long long avail;
	long long free;
	char set;
};

/* forward declare to make gcc happy (fs.h <-> text_object.h include) */
struct text_object;

void init_fs_bar(struct text_object *, const char *);
double fs_barval(struct text_object *);
double fs_free_barval(struct text_object *);

void init_fs(struct text_object *, const char *);
uint8_t fs_free_percentage(struct text_object *);
uint8_t fs_used_percentage(struct text_object *);
void print_fs_free(struct text_object *, char *, int);
void print_fs_size(struct text_object *, char *, int);
void print_fs_used(struct text_object *, char *, int);
void print_fs_type(struct text_object *, char *, int);

int update_fs_stats(void);
struct fs_stat *prepare_fs_stat(const char *path);
void clear_fs_stats(void);

#endif /* _FS_H */
