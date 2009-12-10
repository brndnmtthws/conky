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
#ifdef X11
#ifndef _FONTS_H
#define _FONTS_H

#include "x11.h"

#ifdef __cplusplus
extern "C" {
#endif

/* for fonts */
struct font_list {

	char name[DEFAULT_TEXT_BUFFER_SIZE];
	int num;
	XFontStruct *font;

#ifdef XFT
	XftFont *xftfont;
	int font_alpha;
#endif
};

#ifdef XFT

#define font_height() (use_xft ? (fonts[selected_font].xftfont->ascent + \
	fonts[selected_font].xftfont->descent) \
	: (fonts[selected_font].font->max_bounds.ascent + \
	fonts[selected_font].font->max_bounds.descent))
#define font_ascent() (use_xft ? fonts[selected_font].xftfont->ascent \
	: fonts[selected_font].font->max_bounds.ascent)
#define font_descent() (use_xft ? fonts[selected_font].xftfont->descent \
	: fonts[selected_font].font->max_bounds.descent)

#else

#define font_height() (fonts[selected_font].font->max_bounds.ascent + \
	fonts[selected_font].font->max_bounds.descent)
#define font_ascent() fonts[selected_font].font->max_bounds.ascent
#define font_descent() fonts[selected_font].font->max_bounds.descent

#endif

#define MAX_FONTS 256

/* direct access to registered fonts (FIXME: bad encapsulation) */
extern struct font_list *fonts;
extern int selected_font;
extern int font_count;

void setup_fonts(void);
void set_font(void);
int add_font(const char *);
void set_first_font(const char *);
void free_fonts(void);
void load_fonts(void);

#ifdef __cplusplus
}
#endif

#endif /* _FONTS_H */
#endif /* X11 */
