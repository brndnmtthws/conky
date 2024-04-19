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
#include <optional>
#include <string>
#include <unordered_map>
#ifdef BUILD_X11
#include <X11/Xlib.h>
#endif /* BUILD_X11 */

struct Colour {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t alpha = 0xff;

 public:
  Colour() = default;
  Colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a = UINT8_MAX)
      : red(r), green(g), blue(b), alpha(a) {}
  Colour(const std::string &name);
  Colour(const Colour &) = default;
  Colour(Colour &&) = default;

  void operator=(const Colour &c) {
    red = c.red;
    green = c.green;
    blue = c.blue;
    alpha = c.alpha;
  }

  // Compare two instances.
  bool operator==(const Colour &c) const {
    return c.red == red && c.green == green && c.blue == blue &&
           c.alpha == alpha;
  }

  uint8_t *data() { return reinterpret_cast<uint8_t *>(this); }

  // Express the color as a 32-bit ARGB integer (alpha in MSB).
  uint32_t to_argb32(void) const {
    uint32_t out;
    out = alpha << 24 | red << 16 | green << 8 | blue;
    return out;
  }

  // Construct from a 32-bit ARGB integer (alpha in MSB).
  static Colour from_argb32(uint32_t argb);

  class Hash {
   public:
    size_t operator()(const Colour &c) const { return c.to_argb32(); }
  };

#ifdef BUILD_X11
  unsigned long to_x11_color(Display *display, int screen,
                             bool transparency = false,
                             bool premultiply = false);
#endif /* BUILD_X11 */
};

const Colour ERROR_COLOUR = Colour{UINT8_MAX, 0, 0, UINT8_MAX};

Colour parse_color(const std::string &color);
