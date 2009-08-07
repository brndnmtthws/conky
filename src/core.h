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

#ifndef _CONKY_CORE_H_
#define _CONKY_CORE_H_

#include "config.h"	/* defines */

/* alignments */
enum alignment {
	TOP_LEFT = 1,
	TOP_RIGHT,
	TOP_MIDDLE,
	BOTTOM_LEFT,
	BOTTOM_RIGHT,
	BOTTOM_MIDDLE,
	MIDDLE_LEFT,
	MIDDLE_RIGHT,
	NONE
};

#ifdef X11
#define TO_X 1
#endif /* X11 */
#define TO_STDOUT 2
#define TO_STDERR 4
#define OVERWRITE_FILE 8
#define APPEND_FILE 16
#ifdef NCURSES
#define TO_NCURSES 32
#endif /* NCURSES */

void read_exec(const char *data, char *buf, const int size);
void set_default_configurations(conky_context *ctx);
void set_update_interval(conky_context *ctx, double interval);

/* update_text() generates new text and clears old text area */
void update_text(conky_context *ctx);
void update_text_area(conky_context *ctx);
void draw_stuff(conky_context *ctx);
char load_config_file(conky_context *ctx, const char *f);
void extract_variable_text(conky_context *ctx, const char *p);

#ifdef X11
void clear_text(conky_context *ctx, int exposures);
enum alignment string_to_alignment(const char *s);
void load_config_file_x11(conky_context *ctx, const char *);
void X11_create_window(conky_context *ctx);
#endif /* X11 */

void convert_escapes(char *buf);

void reload_config(conky_context *ctx);

void clean_up(conky_context *ctx, void *memtofree1, void *memtofree2);

#endif /* _CONKY_CORE_H_ */
