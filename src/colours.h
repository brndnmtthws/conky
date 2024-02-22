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
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
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
#pragma once

#include <config.h>
#include <cassert>
#include <climits>
#include <memory>
#include <string>
#include <unordered_map>
#ifdef BUILD_X11
#include "x11.h"
#endif /* BUILD_X11 */

struct Colour {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t alpha;

 public:
  // Compare two instances.
  bool operator==(const Colour &c) const {
    return c.red == red && c.green == green && c.blue == blue &&
           c.alpha == alpha;
  }

  // Express the color as a 32-bit ARGB integer (alpha in MSB).
  uint32_t to_argb32(void) const {
    uint32_t out;
    out = alpha << 24 | red << 16 | green << 8 | blue;
    return out;
  }

  // Construct from a 32-bit ARGB integer (alpha in MSB).
  static Colour from_argb32(uint32_t argb);

#ifdef BUILD_X11
  class Hash {
   public:
    size_t operator()(const Colour &c) const { return c.to_argb32(); }
  };

  static std::unordered_map<Colour, unsigned long, Hash> x11_pixels;
  unsigned long to_x11_color(Display *display, int screen,
                             bool premultiply = false) {
    if (display == nullptr) {
      /* cannot work if display is not open */
      return 0;
    }

    unsigned long pixel;

    /* Either get a cached X11 pixel or allocate one */
    if (auto pixel_iter = x11_pixels.find(*this);
        pixel_iter != x11_pixels.end()) {
      pixel = pixel_iter->second;
    } else {
      XColor xcolor{};
      xcolor.red = red * 257;
      xcolor.green = green * 257;
      xcolor.blue = blue * 257;
      if (XAllocColor(display, DefaultColormap(display, screen), &xcolor) ==
          0) {
        // NORM_ERR("can't allocate X color");
        return 0;
      }

      /* Save pixel value in the cache to avoid reallocating it */
      x11_pixels[*this] = xcolor.pixel;
      pixel = static_cast<unsigned long>(xcolor.pixel);
    }

    pixel &= 0xffffff;
#ifdef BUILD_ARGB
    if (have_argb_visual) {
      if (premultiply)
        pixel = (red * alpha / 255) << 16 | (green * alpha / 255) << 8 |
                (blue * alpha / 255);
      pixel |= ((unsigned long)alpha << 24);
    }
#endif /* BUILD_ARGB */
    return pixel;
  }
#endif /* BUILD_X11 */
};

extern Colour error_colour;

Colour parse_color(const std::string &colour);
// XXX: when everyone uses C++ strings, remove this C version
Colour parse_color(const char *);
