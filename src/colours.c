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
#include "core.h"
#include "logging.h"
#ifdef X11
#include "x11.h"
#endif

/* precalculated: 31/255, and 63/255 */
#define CONST_8_TO_5_BITS 0.12156862745098
#define CONST_8_TO_6_BITS 0.247058823529412

static void set_up_gradient(conky_context *ctx)
{
	int i;
#ifdef X11
	if (ctx->output_methods & TO_X) {
		ctx->colour_depth = DisplayPlanes(display, screen);
	} else
#endif /* X11 */
	{
		ctx->colour_depth = 16;
	}
	if (ctx->colour_depth != 24 && ctx->colour_depth != 16) {
		NORM_ERR("using non-standard colour depth, gradients may look like a "
			"lolly-pop");
	}

	ctx->redmask = 0;
	ctx->greenmask = 0;
	ctx->bluemask = 0;
	for (i = (ctx->colour_depth / 3) - 1; i >= 0; i--) {
		ctx->redmask |= 1 << i;
		ctx->greenmask |= 1 << i;
		ctx->bluemask |= 1 << i;
	}
	if (ctx->colour_depth % 3 == 1) {
		ctx->greenmask |= 1 << (ctx->colour_depth / 3);
	}
	ctx->redmask = ctx->redmask << (2 * ctx->colour_depth / 3 + ctx->colour_depth % 3);
	ctx->greenmask = ctx->greenmask << (ctx->colour_depth / 3);
}

/* adjust colour values depending on colour depth */
unsigned int adjust_colours(conky_context *ctx, unsigned int colour)
{
	double r, g, b;

	if (ctx->colour_depth == 0) {
		set_up_gradient(ctx);
	}
	if (ctx->colour_depth == 16) {
		r = (colour & 0xff0000) >> 16;
		g = (colour & 0xff00) >> 8;
		b =  colour & 0xff;
		colour  = (int) (r * CONST_8_TO_5_BITS) << 11;
		colour |= (int) (g * CONST_8_TO_6_BITS) << 5;
		colour |= (int) (b * CONST_8_TO_5_BITS);
	}
	return colour;
}

/* this function returns the next colour between two colours for a gradient */
unsigned long *do_gradient(conky_context *ctx, int width, unsigned long first_colour, unsigned long last_colour)
{
	int red1, green1, blue1;				// first colour
	int red2, green2, blue2;				// last colour
	int reddiff, greendiff, bluediff;		// difference
	short redshift = (2 * ctx->colour_depth / 3 + ctx->colour_depth % 3);
	short greenshift = (ctx->colour_depth / 3);
	unsigned long *colours = malloc(width * sizeof(unsigned long));
	int i;

	if (ctx->colour_depth == 0) {
		set_up_gradient(ctx);
	}
	red1 = (first_colour & ctx->redmask) >> redshift;
	green1 = (first_colour & ctx->greenmask) >> greenshift;
	blue1 = first_colour & ctx->bluemask;
	red2 = (last_colour & ctx->redmask) >> redshift;
	green2 = (last_colour & ctx->greenmask) >> greenshift;
	blue2 = last_colour & ctx->bluemask;
	reddiff = abs(red1 - red2);
	greendiff = abs(green1 - green2);
	bluediff = abs(blue1 - blue2);
#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic,10) shared(colours)
#endif /* HAVE_OPENMP */
	for (i = 0; i < width; i++) {
		int red3 = 0, green3 = 0, blue3 = 0;	// colour components

		float factor = ((float)(i + 1) / width);

		/* the '+ 0.5' bit rounds our floats to ints properly */
		if (red1 >= red2) {
			red3 = -(factor * reddiff) - 0.5;
		} else if (red1 < red2) {
			red3 = factor * reddiff + 0.5;
		}
		if (green1 >= green2) {
			green3 = -(factor * greendiff) - 0.5;
		} else if (green1 < green2) {
			green3 = factor * greendiff + 0.5;
		}
		if (blue1 >= blue2) {
			blue3 = -(factor * bluediff) - 0.5;
		} else if (blue1 < blue2) {
			blue3 = factor * bluediff + 0.5;
		}
		red3 += red1;
		green3 += green1;
		blue3 += blue1;
		if (red3 < 0) {
			red3 = 0;
		}
		if (green3 < 0) {
			green3 = 0;
		}
		if (blue3 < 0) {
			blue3 = 0;
		}
		if (red3 > ctx->bluemask) {
			red3 = ctx->bluemask;
		}
		if (green3 > ctx->bluemask) {
			green3 = ctx->bluemask;
		}
		if (blue3 > ctx->bluemask) {
			blue3 = ctx->bluemask;
		}
		colours[i] = (red3 << redshift) | (green3 << greenshift) | blue3;
	}
	return colours;
}

