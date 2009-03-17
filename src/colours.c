/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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
#include "logging.h"
#ifdef X11
#include "x11.h"
#endif

/* precalculated: 31/255, and 63/255 */
#define CONST_8_TO_5_BITS 0.12156862745098
#define CONST_8_TO_6_BITS 0.247058823529412

static short colour_depth = 0;
static long redmask, greenmask, bluemask;

static void set_up_gradient(void)
{
	int i;
#ifdef X11
	if (output_methods & TO_X) {
		colour_depth = DisplayPlanes(display, screen);
	} else
#endif /* X11 */
	{
		colour_depth = 16;
	}
	if (colour_depth != 24 && colour_depth != 16) {
		ERR("using non-standard colour depth, gradients may look like a "
			"lolly-pop");
	}

	redmask = 0;
	greenmask = 0;
	bluemask = 0;
	for (i = (colour_depth / 3) - 1; i >= 0; i--) {
		redmask |= 1 << i;
		greenmask |= 1 << i;
		bluemask |= 1 << i;
	}
	if (colour_depth % 3 == 1) {
		greenmask |= 1 << (colour_depth / 3);
	}
	redmask = redmask << (2 * colour_depth / 3 + colour_depth % 3);
	greenmask = greenmask << (colour_depth / 3);
}

/* adjust color values depending on color depth */
unsigned int adjust_colors(unsigned int color)
{
	double r, g, b;

	if (colour_depth == 0) {
		set_up_gradient();
	}
	if (colour_depth == 16) {
		r = (color & 0xff0000) >> 16;
		g = (color & 0xff00) >> 8;
		b =  color & 0xff;
		color  = (int) (r * CONST_8_TO_5_BITS) << 11;
		color |= (int) (g * CONST_8_TO_6_BITS) << 5;
		color |= (int) (b * CONST_8_TO_5_BITS);
	}
	return color;
}

/* this function returns the next colour between two colours for a gradient */
unsigned long do_gradient(unsigned long first_colour,
		unsigned long last_colour)
{
	int tmp_color = 0;
	int red1, green1, blue1;				// first colour
	int red2, green2, blue2;				// second colour
	int red3 = 0, green3 = 0, blue3 = 0;	// difference
	short redshift = (2 * colour_depth / 3 + colour_depth % 3);
	short greenshift = (colour_depth / 3);

	red1 = (first_colour & redmask) >> redshift;
	green1 = (first_colour & greenmask) >> greenshift;
	blue1 = first_colour & bluemask;
	red2 = (last_colour & redmask) >> redshift;
	green2 = (last_colour & greenmask) >> greenshift;
	blue2 = last_colour & bluemask;
	if (red1 > red2) {
		red3 = -1;
	}
	if (red1 < red2) {
		red3 = 1;
	}
	if (green1 > green2) {
		green3 = -1;
	}
	if (green1 < green2) {
		green3 = 1;
	}
	if (blue1 > blue2) {
		blue3 = -1;
	}
	if (blue1 < blue2) {
		blue3 = 1;
	}
	red1 += red3;
	green1 += green3;
	blue1 += blue3;
	if (red1 < 0) {
		red1 = 0;
	}
	if (green1 < 0) {
		green1 = 0;
	}
	if (blue1 < 0) {
		blue1 = 0;
	}
	if (red1 > bluemask) {
		red1 = bluemask;
	}
	if (green1 > bluemask) {
		green1 = bluemask;
	}
	if (blue1 > bluemask) {
		blue1 = bluemask;
	}
	tmp_color = (red1 << redshift) | (green1 << greenshift) | blue1;
	return tmp_color;
}

/* this function returns the max diff for a gradient */
unsigned long gradient_max(unsigned long first_colour,
		unsigned long last_colour)
{
	int red1, green1, blue1;				// first colour
	int red2, green2, blue2;				// second colour
	int red3 = 0, green3 = 0, blue3 = 0;			// difference
	long redshift, greenshift;
	int max;

	if (colour_depth == 0) {
		set_up_gradient();
	}
	redshift = (2 * colour_depth / 3 + colour_depth % 3);
	greenshift = (colour_depth / 3);

	red1 = (first_colour & redmask) >> redshift;
	green1 = (first_colour & greenmask) >> greenshift;
	blue1 = first_colour & bluemask;
	red2 = (last_colour & redmask) >> redshift;
	green2 = (last_colour & greenmask) >> greenshift;
	blue2 = last_colour & bluemask;
	red3 = abs(red1 - red2);
	green3 = abs(green1 - green2);
	blue3 = abs(blue1 - blue2);
	max = red3;

	if (green3 > max) {
		max = green3;
	}
	if (blue3 > max) {
		max = blue3;
	}
	return max;
}

