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

#include "colours.h"

#include "conky.h"
#include "gui.h"
#include "logging.h"

#ifdef BUILD_X11
#include <string.h>
#include <strings.h>

#include <X11/Xlib.h>
#endif /* BUILD_X11 */
#include "x11-color.h"

Colour Colour::from_argb32(uint32_t argb) {
  Colour out;
  out.alpha = argb >> 24;
  out.red = (argb >> 16) % 256;
  out.green = (argb >> 8) % 256;
  out.blue = argb % 256;
  return out;
}

#ifdef BUILD_X11
unsigned long Colour::to_x11_color(Display *display, int screen,
                                   bool transparency, bool premultiply) {
  static std::unordered_map<Colour, unsigned long, Colour::Hash> x11_pixels;

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
    xcolor.red = this->red * 257;
    xcolor.green = this->green * 257;
    xcolor.blue = this->blue * 257;
    if (XAllocColor(display, DefaultColormap(display, screen), &xcolor) == 0) {
      // NORM_ERR("can't allocate X color");
      return 0;
    }

    /* Save pixel value in the cache to avoid reallocating it */
    x11_pixels[*this] = xcolor.pixel;
    pixel = static_cast<unsigned long>(xcolor.pixel);
  }

  pixel &= 0xffffff;
#ifdef BUILD_ARGB
  if (transparency) {
    if (premultiply)
      pixel = (red * alpha / 255) << 16 | (green * alpha / 255) << 8 |
              (blue * alpha / 255);
    pixel |= ((unsigned long)alpha << 24);
  }
#endif /* BUILD_ARGB */
  return pixel;
}
#endif /* BUILD_X11 */

std::optional<Colour> parse_color_name(const std::string &name) {
  unsigned short r, g, b;
  size_t len = name.length();
  // Parse X11 color names.
  if (OsLookupColor(name.c_str(), len, &r, &g, &b)) {
    return Colour{(uint8_t)r, (uint8_t)g, (uint8_t)b, 0xff};
  } else {
    return std::nullopt;
  }
}

std::optional<Colour> parse_hex_color(const std::string &color) {
  const char *name = color.c_str();
  size_t len = color.length();
  // Skip a leading '#' if present.
  if (name[0] == '#') {
    name++;
    len--;
  }

  static auto hex_nibble_value = [](char c) {
    if (c >= '0' && c <= '9') {
      return c - '0';
    } else if (c >= 'a' && c <= 'f') {
      return c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      return c - 'A' + 10;
    }
    return -1;
  };
  const auto none = [&]() {
    NORM_ERR("can't parse hex color '%s' (%d)", name, len);
    return std::nullopt;
  };

  uint8_t argb[4] = {0xff, 0, 0, 0};
  if (len == 3 || len == 4) {
    bool skip_alpha = (len == 3);
    for (size_t i = 0; i < len; i++) {
      int nib = hex_nibble_value(name[i]);
      if (nib < 0) { return none(); }
      // Duplicate the nibble, so "#abc" -> 0xaa, 0xbb, 0xcc
      int val = (nib << 4) + nib;

      argb[skip_alpha + i] = val;
    }
  } else if (len == 6 || len == 8) {
    bool skip_alpha = (len == 6);
    for (size_t i = 0; i + 1 < len; i += 2) {
      int nib1 = hex_nibble_value(name[i]);
      int nib2 = hex_nibble_value(name[i + 1]);
      if (nib1 < 0 || nib2 < 0) { return none(); }
      int val = (nib1 << 4) + nib2;

      argb[skip_alpha + i / 2] = val;
    }
  } else {
    return none();
  }

  return Colour(argb[1], argb[2], argb[3], argb[0]);
}

Colour parse_color(const std::string &color) {
  std::optional<Colour> result;

#define TRY_PARSER(name)                            \
  std::optional<Colour> value_##name = name(color); \
  if (value_##name.has_value()) { return value_##name.value(); }

  std::optional<Colour> value_parse_color_name = parse_color_name(color);
  if (value_parse_color_name.has_value()) {
    return value_parse_color_name.value();
  }
  std::optional<Colour> value_parse_hex_color = parse_hex_color(color);
  if (value_parse_hex_color.has_value()) {
    return value_parse_hex_color.value();
  }

#undef TRY_PARSER

  return ERROR_COLOUR;
}
