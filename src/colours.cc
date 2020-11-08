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
#include "logging.h"
#ifdef BUILD_X11
#include "x11.h"
#endif

/* precalculated: 31/255, and 63/255 */
#define CONST_8_TO_5_BITS 0.12156862745098
#define CONST_8_TO_6_BITS 0.247058823529412

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

static short colour_depth = 0;
static long redmask, greenmask, bluemask;

static void set_up_gradient() {
  int i;
#ifdef BUILD_X11
  if (out_to_x.get(*state)) {
    colour_depth = DisplayPlanes(display, screen);
  } else
#endif /* BUILD_X11 */
  {
    colour_depth = 16;
  }
  if (colour_depth != 24 && colour_depth != 16) {
    NORM_ERR(
        "using non-standard colour depth, gradients may look like a "
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
  if (colour_depth % 3 == 1) { greenmask |= 1 << (colour_depth / 3); }
  redmask = redmask << (2 * colour_depth / 3 + colour_depth % 3);
  greenmask = greenmask << (colour_depth / 3);
}

/* adjust colour values depending on colour depth */
unsigned int adjust_colours(unsigned int colour) {
  double r, g, b;

  if (colour_depth == 0) { set_up_gradient(); }
  if (colour_depth == 16) {
    r = (colour & 0xff0000) >> 16;
    g = (colour & 0xff00) >> 8;
    b = colour & 0xff;
    colour = static_cast<int>(r * CONST_8_TO_5_BITS) << 11;
    colour |= static_cast<int>(g * CONST_8_TO_6_BITS) << 5;
    colour |= static_cast<int>(b * CONST_8_TO_5_BITS);
  }
  return colour;
}

/* this function returns the next colour between two colours for a gradient */
unsigned long *do_gradient(int width, unsigned long first_colour,
                           unsigned long last_colour) {
  int red1, green1, blue1;           // first colour
  int red2, green2, blue2;           // last colour
  int reddiff, greendiff, bluediff;  // difference
  short redshift = (2 * colour_depth / 3 + colour_depth % 3);
  short greenshift = (colour_depth / 3);
  auto *colours =
      static_cast<unsigned long *>(malloc(width * sizeof(unsigned long)));
  int i;

  if (colour_depth == 0) { set_up_gradient(); }
  red1 = (first_colour & redmask) >> redshift;
  green1 = (first_colour & greenmask) >> greenshift;
  blue1 = first_colour & bluemask;
  red2 = (last_colour & redmask) >> redshift;
  green2 = (last_colour & greenmask) >> greenshift;
  blue2 = last_colour & bluemask;
  reddiff = abs(red1 - red2);
  greendiff = abs(green1 - green2);
  bluediff = abs(blue1 - blue2);
#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic, 10) shared(colours)
#endif /* HAVE_OPENMP */
  for (i = 0; i < width; i++) {
    int red3 = 0, green3 = 0, blue3 = 0;  // colour components

    float factor = (static_cast<float>(i) / (width - 1));

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
    if (red3 < 0) { red3 = 0; }
    if (green3 < 0) { green3 = 0; }
    if (blue3 < 0) { blue3 = 0; }
    if (red3 > bluemask) { red3 = bluemask; }
    if (green3 > bluemask) { green3 = bluemask; }
    if (blue3 > bluemask) { blue3 = bluemask; }
    colours[i] = (red3 << redshift) | (green3 << greenshift) | blue3;
  }
  return colours;
}

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
unsigned long *do_hsv_gradient(
    int width,
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
  satDiff = hsv2[1] - hsv1[1];
  valDiff = hsv2[2] - hsv1[2];

  colours[0] = first_colour;
  colours[width - 1] = last_colour;

  for (i = 1; i < (width - 1); i++) {
    long k;

    long divisor = width - i;
    k = (hueDiff + divisor / 2) / divisor;
    hueDiff -= k;
    hsv1[0] += k;

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

#ifdef BUILD_X11
long get_x11_color(const char *name) {
  XColor color;

  color.pixel = 0;
  if (XParseColor(display, DefaultColormap(display, screen), name, &color) ==
      0) {
    /* lets check if it's a hex colour with the # missing in front
     * if yes, then do something about it */
    char newname[DEFAULT_TEXT_BUFFER_SIZE];

    newname[0] = '#';
    strncpy(&newname[1], name, DEFAULT_TEXT_BUFFER_SIZE - 1);
    /* now lets try again */
    if (XParseColor(display, DefaultColormap(display, screen), &newname[0],
                    &color) == 0) {
      NORM_ERR("can't parse X color '%s'", name);
      return 0xFF00FF;
    }
  }
  if (XAllocColor(display, DefaultColormap(display, screen), &color) == 0) {
    NORM_ERR("can't allocate X color '%s'", name);
  }

  return static_cast<long>(color.pixel);
}

long get_x11_color(const std::string &colour) {
  return get_x11_color(colour.c_str());
}
#endif
