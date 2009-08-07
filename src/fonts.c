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
#include "conky.h"
#include "ctx->fonts.h"
#include "logging.h"

void set_font(conky_context *ctx)
{
#ifdef XFT
	if (use_xft->ctx) return;
#endif /* XFT */
	if (ctx->font_count > -1 && ctx->fonts[selected_font].font) {
		XSetFont(display, window.gc, ctx->fonts[selected_font].font->fid);
	}
}

void setup_fonts(conky_context *ctx)
{
	if ((output_methods & TO_X) == 0) {
		return;
	}
#ifdef XFT
	if (use_xft->ctx) {
		if (window.xftdraw) {
			XftDrawDestroy(window.xftdraw);
			window.xftdraw = 0;
		}
		window.xftdraw = XftDrawCreate(display, window.drawable,
				DefaultVisual(display, screen), DefaultColormap(display, screen));
	}
#endif /* XFT */
	set_font();
}

int add_font(conky_context *ctx, const char *data_in)
{
	if ((output_methods & TO_X) == 0) {
		return 0;
	}
	if (ctx->font_count > MAX_FONTS) {
		CRIT_ERR(NULL, NULL, "you don't need that many ctx->fonts, sorry.");
	}
	ctx->font_count++;
	if (ctx->font_count == 0) {
		if (ctx->fonts != NULL) {
			free(ctx->fonts);
		}
		if ((ctx->fonts = (struct font_list *) malloc(sizeof(struct font_list)))
				== NULL) {
			CRIT_ERR(NULL, NULL, "malloc");
		}
		memset(ctx->fonts, 0, sizeof(struct font_list));
	}
	ctx->fonts = realloc(ctx->fonts, (sizeof(struct font_list) * (ctx->font_count + 1)));
	memset(&ctx->fonts[ctx->font_count], 0, sizeof(struct font_list));
	if (ctx->fonts == NULL) {
		CRIT_ERR(NULL, NULL, "realloc in add_font");
	}
	// must account for null terminator
	if (strlen(data_in) < DEFAULT_TEXT_BUFFER_SIZE) {
		strncpy(ctx->fonts[ctx->font_count].name, data_in, DEFAULT_TEXT_BUFFER_SIZE);
#ifdef XFT
		ctx->fonts[ctx->font_count].font_alpha = 0xffff;
#endif
	} else {
		CRIT_ERR(NULL, NULL, "Oops...looks like something overflowed in add_font().");
	}
	return ctx->font_count;
}

void set_first_font(conky_context *ctx, const char *data_in)
{
	if ((output_methods & TO_X) == 0) {
		return;
	}
	if (ctx->font_count < 0) {
		if ((ctx->fonts = (struct font_list *) malloc(sizeof(struct font_list)))
				== NULL) {
			CRIT_ERR(NULL, NULL, "malloc");
		}
		memset(ctx->fonts, 0, sizeof(struct font_list));
		ctx->font_count++;
	}
	if (strlen(data_in) > 1) {
		strncpy(ctx->fonts[0].name, data_in, DEFAULT_TEXT_BUFFER_SIZE);
#ifdef XFT
		ctx->fonts[0].font_alpha = 0xffff;
#endif
	}
}

void free_fonts(conky_context *ctx)
{
	int i;

	if ((output_methods & TO_X) == 0) {
		return;
	}
	if(ctx->fontloaded == 0) {
		free(ctx->fonts);
		return;
	}
	for (i = 0; i <= ctx->font_count; i++) {
#ifdef XFT
		if (use_xft->ctx) {
			XftFontClose(display, ctx->fonts[i].xftfont);
			ctx->fonts[i].xftfont = 0;
		} else
#endif /* XFT */
		{
			XFreeFont(display, ctx->fonts[i].font);
			ctx->fonts[i].font = 0;
		}
	}
	free(ctx->fonts);
	ctx->fonts = 0;
	ctx->font_count = -1;
	selected_font = 0;
#ifdef XFT
	if (window.xftdraw) {
		XftDrawDestroy(window.xftdraw);
		window.xftdraw = 0;
	}
#endif /* XFT */
}

void load_fonts(conky_context *ctx)
{
	int i;

	if ((output_methods & TO_X) == 0)
		return;
	for (i = 0; i <= ctx->font_count; i++) {
#ifdef XFT
		/* load Xft font */
		if (use_xft->ctx && ctx->fonts[i].xftfont) {
			continue;
		} else if (use_xft->ctx) {
			ctx->fonts[i].xftfont = XftFontOpenName(display, screen,
					ctx->fonts[i].name);
			if (ctx->fonts[i].xftfont) {
				continue;
			}

			NORM_ERR("can't load Xft font '%s'", ctx->fonts[i].name);
			if ((ctx->fonts[i].xftfont = XftFontOpenName(display, screen,
					"courier-12")) != NULL) {
				continue;
			}

			NORM_ERR("can't load Xft font '%s'", "courier-12");

			if ((ctx->fonts[i].font = XLoadQueryFont(display, "fixed")) == NULL) {
				CRIT_ERR(NULL, NULL, "can't load font '%s'", "fixed");
			}
			use_xft->ctx = 0;

			continue;
		}
#endif
		/* load normal font */
		if (!ctx->fonts[i].font && (ctx->fonts[i].font = XLoadQueryFont(display, ctx->fonts[i].name)) == NULL) {
			NORM_ERR("can't load font '%s'", ctx->fonts[i].name);
			if ((ctx->fonts[i].font = XLoadQueryFont(display, "fixed")) == NULL) {
				CRIT_ERR(NULL, NULL, "can't load font '%s'", "fixed");
			}
		}
	}
	ctx->fontloaded = 1;
}
