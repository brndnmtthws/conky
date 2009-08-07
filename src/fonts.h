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

#ifdef XFT

#define font_height() (use_xft ? (ctx->fonts[ctx->selected_font].xftfont->ascent + \
	ctx->fonts[ctx->selected_font].xftfont->descent) \
	: (ctx->fonts[ctx->selected_font].font->max_bounds.ascent + \
	ctx->fonts[ctx->selected_font].font->max_bounds.descent))
#define font_ascent() (use_xft ? ctx->fonts[ctx->selected_font].xftfont->ascent \
	: ctx->fonts[ctx->selected_font].font->max_bounds.ascent)
#define font_descent() (use_xft ? ctx->fonts[ctx->selected_font].xftfont->descent \
	: ctx->fonts[ctx->selected_font].font->max_bounds.descent)

#else

#define font_height() (ctx->fonts[ctx->selected_font].font->max_bounds.ascent + \
	ctx->fonts[ctx->selected_font].font->max_bounds.descent)
#define font_ascent() ctx->fonts[ctx->selected_font].font->max_bounds.ascent
#define font_descent() ctx->fonts[ctx->selected_font].font->max_bounds.descent

#endif

#define MAX_FONTS 256


void setup_fonts(conky_context *ctx);
void set_font(conky_context *ctx);
int add_font(conky_context *ctx, const char *);
void set_first_font(conky_context *ctx, const char *);
void free_fonts(conky_context *ctx);
void load_fonts(conky_context *ctx);

#endif /* _FONTS_H */
#endif /* X11 */
