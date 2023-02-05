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
#include "conky.h"
#include "gui.h"
#include "logging.h"
#ifdef BUILD_X11
#include "x11.h"
#endif /*BUILD_X11*/
#include "x11-color.h"

/* precalculated: 31/255, and 63/255 */
#define CONST_8_TO_5_BITS 0.12156862745098
#define CONST_8_TO_6_BITS 0.247058823529412

short colour_depth = 0;
long redmask, greenmask, bluemask;

void set_up_gradient() {
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
  for (int i = (colour_depth / 3) - 1; i >= 0; i--) {
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

static int hex_nibble_value(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  } else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

Colour Colour::from_argb32(uint32_t argb) {
  Colour out;
  out.alpha = argb >> 24;
  out.red = (argb >> 16) % 256;
  out.green = (argb >> 8) % 256;
  out.blue = argb % 256;
  return out;
}

Colour error_colour { 0xff, 0x00, 0x00, 0xff };

Colour parse_color(const char *name) {
  unsigned short r, g, b;
  size_t len = strlen(name);
  if (OsLookupColor(-1, name, len, &r, &g, &b)) {
    Colour out = {(uint8_t)r, (uint8_t)g, (uint8_t)b, 0xff};
    return out;
  }
  if (name[0] == '#') {
    name++;
    len--;
  }
  if (len == 6 || len == 8) {
    bool skip_alpha = (len == 6);
    unsigned char argb[4] = {0xff, 0, 0, 0};
    for (size_t i = 0; i + 1 < len; i += 2) {
      int nib1 = hex_nibble_value(name[i]);
      int nib2 = hex_nibble_value(name[i + 1]);
      if (nib1 < 0 || nib2 < 0) { goto err; }
      int val = (nib1 << 4) + nib2;

      argb[skip_alpha + i / 2] = val;
    }
    Colour out;
    out.alpha = argb[0];
    out.red = argb[1];
    out.green = argb[2];
    out.blue = argb[3];
    return out;
  }
err:
  NORM_ERR("can't parse X color '%s' (%d)", name, len);
  return error_colour;
}

Colour parse_color(const std::string &colour) {
  return parse_color(colour.c_str());
}
