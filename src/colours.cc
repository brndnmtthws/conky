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
#include "x11-color.h"

#ifdef BUILD_X11
std::unordered_map<Colour, unsigned long, Colour::Hash> Colour::x11_pixels;
#endif /* BUILD_X11 */

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

Colour error_colour{0xff, 0x00, 0x00, 0xff};

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
