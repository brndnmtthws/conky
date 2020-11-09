/*
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
 * Copyright (c) 2005-2019 Brenden Matthews, Philip Kovacs, et. al.
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
#include "colours.h"
#include "logging.h"
#ifdef BUILD_X11
#include "x11.h"

#endif

#define CONST_SCALE 512L
#define CONST_SCALE_HALF (CONST_SCALE / 2)
#define CONST_SCALE2 (CONST_SCALE * 2L)
#define CONST_SCALE4 (CONST_SCALE * 4L)
#define CONST_SCALE6 (CONST_SCALE * 6L)
#define CONST_SCALE60 (CONST_SCALE * 60L)
#define CONST_SCALE120 (CONST_SCALE * 120L)
#define CONST_SCALE180 (CONST_SCALE * 180L)
#define CONST_SCALE240 (CONST_SCALE * 240L)
#define CONST_SCALE300 (CONST_SCALE * 300L)
#define CONST_SCALE360 (CONST_SCALE * 360L)

long to_decimal_scale(long value, long max_value) {
  if (value == 0) {
    return 0;
  } else if (value > 0) {
    return (value * CONST_SCALE + max_value - 1) / max_value;
  }
  return -((abs(value) * CONST_SCALE  + max_value - 1) / max_value);
}

long from_decimal_scale(long value, long max_value) {
  if (value == 0) {
    return 0;
  } else if (value > 0) {
    return (value * max_value + CONST_SCALE_HALF) / CONST_SCALE;
  }
  return -((abs(value) * max_value + CONST_SCALE_HALF) / CONST_SCALE);
}

void scaled_rgb_to_scaled_hsv(long * const rgb, long *hsv) {
  long val = rgb[0] > rgb[1] ? MAX(rgb[0], rgb[2]) : MAX(rgb[1], rgb[2]);
  long cmin = rgb[0] < rgb[1] ? MIN(rgb[0], rgb[2]) : MIN(rgb[1], rgb[2]);
  long delta = val - cmin;

  long hue;

  if (delta == 0) {
    hue = 0;
  } else {
    long d;
    long offset;

    if (rgb[0] == val) {
      d = rgb[1] - rgb[2];
      offset = 0;
    } else if (rgb[1] == val) {
      d = rgb[2] - rgb[0];
      offset = CONST_SCALE2;
    } else {
      d = rgb[0] - rgb[1];
      offset = CONST_SCALE4;
    }
    long h = (CONST_SCALE * d + delta / 2) / delta + offset;
    hue = 60L * ((CONST_SCALE6 + h) % CONST_SCALE6);
  }

  long sat;
  if (val == 0) {
    sat = 0;
  } else {
    sat = (CONST_SCALE * delta + val / 2) / val;
  }

  hsv[0] = hue;
  hsv[1] = sat;
  hsv[2] = val;
}

void scaled_hsv_to_scaled_rgb(long *const hsv, long *rgb) {
  long c = (hsv[2] * hsv[1] + CONST_SCALE_HALF) / CONST_SCALE;

  long hue = hsv[0] % CONST_SCALE360;
  long x = (c *
      (CONST_SCALE - abs(((hue + 30L) / 60L) % CONST_SCALE2 - CONST_SCALE))
      + CONST_SCALE_HALF) / CONST_SCALE;
  long m = hsv[2] - c;

  rgb[0] = m;
  rgb[1] = m;
  rgb[2] = m;

  if (hue < CONST_SCALE60) {
    rgb[0] += c;
    rgb[1] += x;
  } else if (hue < CONST_SCALE120) {
    rgb[0] += x;
    rgb[1] += c;
  } else if (hue < CONST_SCALE180) {
    rgb[1] += c;
    rgb[2] += x;
  } else if (hue < CONST_SCALE240) {
    rgb[1] += x;
    rgb[2] += c;
  } else if (hue < CONST_SCALE300) {
    rgb[2] += c;
    rgb[0] += x;
  } else {
    rgb[2] += x;
    rgb[0] += c;
  }
}

/* this function returns the next colour between two colours in hsv space for a gradient */
unsigned long *do_hsv_gradient(int width,
    unsigned long first_colour,
    unsigned long last_colour) {

  long rgb1[3], rgb2[3], rgb3[3];
  long hsv1[3], hsv2[3];
  long hueDiff, satDiff, valDiff;

  int redshift = (2 * colour_depth / 3 + colour_depth % 3);
  int greenshift = (colour_depth / 3);
  auto *colours =
      static_cast<unsigned long *>(malloc(width * sizeof(unsigned long)));
  int i;

  if (colour_depth == 0) { set_up_gradient(); }

  rgb1[0] = to_decimal_scale((first_colour & redmask) >> redshift, redmask >> redshift);
  rgb1[1] = to_decimal_scale((first_colour & greenmask) >> greenshift, greenmask >> greenshift);
  rgb1[2] = to_decimal_scale(first_colour & bluemask, bluemask);
  rgb2[0] = to_decimal_scale((last_colour & redmask) >> redshift, redmask >> redshift);
  rgb2[1] = to_decimal_scale((last_colour & greenmask) >> greenshift, greenmask >> greenshift);
  rgb2[2] = to_decimal_scale(last_colour & bluemask, bluemask);

  scaled_rgb_to_scaled_hsv(rgb1, hsv1);
  scaled_rgb_to_scaled_hsv(rgb2, hsv2);

  hueDiff = hsv2[0] - hsv1[0];
  // use shortest hue path
  if (hueDiff > CONST_SCALE180) {
    hueDiff = hueDiff - CONST_SCALE360;
  } else if (hueDiff < -CONST_SCALE180) {
    hueDiff = hueDiff + CONST_SCALE360;
  }
  satDiff = hsv2[1] - hsv1[1];
  valDiff = hsv2[2] - hsv1[2];

  colours[0] = first_colour;
  colours[width - 1] = last_colour;

  for (i = 1; i < (width - 1); i++) {
    long k;

    long divisor = width - i;
    k = (hueDiff + divisor / 2) / divisor;
    hueDiff -= k;
    long h = hsv1[0] + k;
    if (h < 0) {
      hsv1[0] = CONST_SCALE360 + h;
    } else {
      hsv1[0] = h;
    }

    k = (satDiff + divisor / 2) / divisor;
    satDiff -= k;
    hsv1[1] += k;

    k = (valDiff + divisor / 2) / divisor;
    valDiff -= k;
    hsv1[2] += k;

    scaled_hsv_to_scaled_rgb(hsv1, rgb3);

    long red3 = from_decimal_scale(rgb3[0], bluemask);
    long green3 = from_decimal_scale(rgb3[1], bluemask);
    long blue3 = from_decimal_scale(rgb3[2], bluemask);

    colours[i] = (red3 << redshift) | (green3 << greenshift) | blue3;

  }
  return colours;
}
