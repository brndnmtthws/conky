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
 */
#ifndef _SPECIALS_H
#define _SPECIALS_H

/* special stuff in text_buffer */

#define SPECIAL_CHAR '\x01'

#define MAX_GRAPH_DEPTH 512

// don't use spaces in LOGGRAPH or NORMGRAPH if you change them
#define LOGGRAPH "-l"
#define TEMPGRAD "-t"

enum special_types {
	NONSPECIAL = 0,
	HORIZONTAL_LINE = 1,
	STIPPLED_HR,
	BAR,
	FG,
	BG,
	OUTLINE,
	ALIGNR,
	ALIGNC,
	GAUGE,
	GRAPH,
	OFFSET,
	VOFFSET,
	FONT,
	GOTO,
	TAB,
};

struct special_t {
	int type;
	short height;
	short width;
	double arg;
	double *graph;
	double scale;			/* maximum value */
	short show_scale;
	int graph_width;
	int scaled;			/* auto adjust maximum */
	unsigned long first_colour;	// for graph gradient
	unsigned long last_colour;
	short font_added;
	char tempgrad;
};

/* direct access to the registered specials (FIXME: bad encapsulation) */
extern struct special_t *specials;
extern int special_count;

extern int default_bar_width;
extern int default_bar_height;
#ifdef X11
extern int default_graph_width;
extern int default_graph_height;
extern int default_gauge_width;
extern int default_gauge_height;
#endif

/* forward declare to avoid mutual inclusion between specials.h and text_object.h */
struct text_object;

/* max number of specials allowed (TODO: use linked list instead) */
extern int max_specials;

/* scanning special arguments */
const char *scan_bar(struct text_object *, const char *, double);
const char *scan_gauge(struct text_object *, const char *, double);
#ifdef X11
void scan_font(struct text_object *, const char *);
char *scan_graph(struct text_object *, const char *, double);
void scan_tab(struct text_object *, const char *);
void scan_stippled_hr(struct text_object *, const char*);

/* printing specials */
void new_font(struct text_object *, char *, int);
void new_graph(struct text_object *, char *, int, double);
void new_hr(struct text_object *, char *, int);
void new_stippled_hr(struct text_object *, char *, int);
#endif
void new_gauge(struct text_object *, char *, int, double);
void new_bar(struct text_object *, char *, int, double);
void new_fg(struct text_object *, char *, int);
void new_bg(struct text_object *, char *, int);
void new_outline(struct text_object *, char *, int);
void new_offset(struct text_object *, char *, int);
void new_voffset(struct text_object *, char *, int);
void new_alignr(struct text_object *, char *, int);
void new_alignc(struct text_object *, char *, int);
void new_goto(struct text_object *, char *, int);
void new_tab(struct text_object *, char *, int);

#endif /* _SPECIALS_H */
