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
 * Copyright (c) 2005-2021 Brenden Matthews, Philip Kovacs, et. al.
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
#include "hcl_gradient.h"
#include "colours.h"
#include "conky.h"
#include "logging.h"

#ifdef BUILD_X11
#include "x11.h"
#endif /* BUILD_X11 */

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
  return -((std::abs(value) * CONST_SCALE + max_value - 1) / max_value);
}

long from_decimal_scale(long value, long max_value) {
  if (value == 0) {
    return 0;
  } else if (value > 0) {
    return (value * max_value + CONST_SCALE_HALF) / CONST_SCALE;
  }
  return -((std::abs(value) * max_value + CONST_SCALE_HALF) / CONST_SCALE);
}

long cap_scaled_color(long colour) {
  if (colour < 0) {
    return 0;
  } else if (colour > CONST_SCALE) {
    return CONST_SCALE;
  }
  return colour;
}

void scaled_rgb_to_scaled_hcl(long *const rgb, long *hcl) {
  long value =
      rgb[0] > rgb[1] ? std::max(rgb[0], rgb[2]) : std::max(rgb[1], rgb[2]);
  long minimum =
      rgb[0] < rgb[1] ? std::min(rgb[0], rgb[2]) : std::min(rgb[1], rgb[2]);
  long chroma = value - minimum;
  long luma = (2627L * rgb[0] + 6780L * rgb[1] + 593L * rgb[2]) /
              10000L;  // Use Rec.2020 color space
  long hue;

  if (chroma == 0) {
    hue = 0;
  } else {
    long diff;
    long offset;

    if (rgb[0] == value) {
      diff = rgb[1] - rgb[2];
      offset = 0;
    } else if (rgb[1] == value) {
      diff = rgb[2] - rgb[0];
      offset = CONST_SCALE2;
    } else {
      diff = rgb[0] - rgb[1];
      offset = CONST_SCALE4;
    }
    long h = (CONST_SCALE * diff) / chroma + offset;
    hue = 60L * ((CONST_SCALE6 + h) % CONST_SCALE6);
  }

  hcl[0] = hue;
  hcl[1] = chroma * 360L;
  hcl[2] = luma * 360L;
}

void scaled_hcl_to_scaled_rgb(long *const hcl, long *rgb) {
  long hue = hcl[0] % CONST_SCALE360;
  long chroma = hcl[1] / 360L;
  long luma = hcl[2] / 360L;

  long h = hue / 60L;
  long x = (chroma * (CONST_SCALE - std::abs(h % CONST_SCALE2 - CONST_SCALE))) /
           CONST_SCALE;
  long m;

  // use Rec.2020 color space
  if (hue < CONST_SCALE60) {
    m = luma - (2627L * chroma + 6780L * x) / 10000L;
    rgb[0] = rgb[1] = rgb[2] = m;
    rgb[0] += chroma;
    rgb[1] += x;
  } else if (hue < CONST_SCALE120) {
    m = luma - (2627L * x + 6780L * chroma) / 10000L;
    rgb[0] = rgb[1] = rgb[2] = m;
    rgb[0] += x;
    rgb[1] += chroma;
  } else if (hue < CONST_SCALE180) {
    m = luma - (6780L * chroma + 593L * x) / 10000L;
    rgb[0] = rgb[1] = rgb[2] = m;
    rgb[1] += chroma;
    rgb[2] += x;
  } else if (hue < CONST_SCALE240) {
    m = luma - (6780L * x + 593L * chroma) / 10000L;
    rgb[0] = rgb[1] = rgb[2] = m;
    rgb[1] += x;
    rgb[2] += chroma;
  } else if (hue < CONST_SCALE300) {
    m = luma - (2627L * x + 593L * chroma) / 10000L;
    rgb[0] = rgb[1] = rgb[2] = m;
    rgb[2] += chroma;
    rgb[0] += x;
  } else {
    m = luma - (2627L * chroma + 593L * x) / 10000L;
    rgb[0] = rgb[1] = rgb[2] = m;
    rgb[2] += x;
    rgb[0] += chroma;
  }

  rgb[0] = cap_scaled_color(rgb[0]);
  rgb[1] = cap_scaled_color(rgb[1]);
  rgb[2] = cap_scaled_color(rgb[2]);
}

void rgb_to_scaled_rgb(unsigned long colour, long *scaled, int redshift,
                       int greenshift) {
  long red = (colour & redmask) >> redshift;
  long green = (colour & greenmask) >> greenshift;
  long blue = colour & bluemask;

  long red_max = redmask >> redshift;
  long green_max = greenmask >> greenshift;
  long blue_max = bluemask;

  scaled[0] = to_decimal_scale(red, red_max);
  scaled[1] = to_decimal_scale(green, green_max);
  scaled[2] = to_decimal_scale(blue, blue_max);
}

unsigned long scaled_rgb_to_rgb(long *const scaled, int redshift,
                                int greenshift) {
  long red_max = redmask >> redshift;
  long green_max = greenmask >> greenshift;

  long red = from_decimal_scale(scaled[0], red_max);
  long green = from_decimal_scale(scaled[1], green_max);
  long blue = from_decimal_scale(scaled[2], bluemask);

  return (red << redshift) | (green << greenshift) | blue;
}

/* this function returns the next colour between two colours in hcl space for a
 * gradient */
std::unique_ptr<unsigned long[]> do_hcl_gradient(int width,
                                                 unsigned long first_colour,
                                                 unsigned long last_colour) {
  long first_colour_rgb[3];
  long last_colour_rgb[3];
  long temp_colour_rgb[3];

  long first_colour_hcl[3];
  long final_colour_hcl[3];

  long hueDiff, chromaDiff, lumaDiff;

  int redshift = (2 * colour_depth / 3 + colour_depth % 3);
  int greenshift = (colour_depth / 3);

  // Make sure the width is always at least 2
  width = std::max(2, width);

  std::unique_ptr<unsigned long[]> colours(new unsigned long[width]);

  if (colour_depth == 0) { set_up_gradient(); }

  rgb_to_scaled_rgb(first_colour, first_colour_rgb, redshift, greenshift);
  rgb_to_scaled_rgb(last_colour, last_colour_rgb, redshift, greenshift);
  scaled_rgb_to_scaled_hcl(first_colour_rgb, first_colour_hcl);
  scaled_rgb_to_scaled_hcl(last_colour_rgb, final_colour_hcl);

  hueDiff = final_colour_hcl[0] - first_colour_hcl[0];
  chromaDiff = final_colour_hcl[1] - first_colour_hcl[1];
  lumaDiff = final_colour_hcl[2] - first_colour_hcl[2];

  colours[0] = first_colour;
  colours[width - 1] = last_colour;

  long divisor = width - 1;
  long hueDelta = hueDiff / divisor;
  long chromaDelta = chromaDiff / divisor;
  long lumaDelta = lumaDiff / divisor;

  for (int i = 1; i < (width - 1); i++) {
    long h = first_colour_hcl[0] + hueDelta;

    if (h < 0) {
      first_colour_hcl[0] = CONST_SCALE360 + h;
    } else {
      first_colour_hcl[0] = h;
    }
    first_colour_hcl[1] += chromaDelta;
    first_colour_hcl[2] += lumaDelta;

    scaled_hcl_to_scaled_rgb(first_colour_hcl, temp_colour_rgb);
    colours[i] = scaled_rgb_to_rgb(temp_colour_rgb, redshift, greenshift);
  }

  return colours;
}
