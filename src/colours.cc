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

#include "logging.h"

#include <cstdarg>
#include <cstdio>
#include <optional>

Colour Colour::from_argb32(uint32_t argb) {
  Colour out;
  out.alpha = argb >> 24;
  out.red = (argb >> 16) % 256;
  out.green = (argb >> 8) % 256;
  out.blue = argb % 256;
  return out;
}

#ifdef BUILD_COLOUR_NAME_MAP
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wregister"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"
#include <colour-names.hh>
#pragma clang diagnostic pop
#pragma GCC diagnostic pop
#else /* BUILD_COLOUR_NAME_MAP */
#include "colour-names-stub.hh"
#endif /* BUILD_COLOUR_NAME_MAP */

std::optional<Colour> inline no_colour() { return std::nullopt; }
std::optional<Colour> parse_error(const std::string &color_str,
                                  const char *format...) {
  va_list args;
  va_start(args, format);
  size_t len = snprintf(nullptr, 0, format, args);
  va_end(args);

  char *reason = new char[len + 1];
  va_start(args, format);
  snprintf(reason, len, format, args);
  va_end(args);

  CRIT_ERR("can't parse color '%s' (len: %d): %s", color_str.c_str(),
           color_str.length(), reason);
  delete[] reason;

  return ERROR_COLOUR;
}

std::optional<Colour> parse_color_name(const std::string &name) {
  const rgb *value = color_name_hash::in_word_set(name.c_str(), name.length());
  if (value != nullptr) {
    return Colour{value->red, value->green, value->blue};
  } else {
    return no_colour();
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

  uint8_t argb[4] = {0xff, 0, 0, 0};
  if (len == 3 || len == 4) {
    bool skip_alpha = (len == 3);
    for (size_t i = 0; i < len; i++) {
      int nib = hex_nibble_value(name[i]);
      if (nib < 0) return parse_error(color, "invalid hex value");
      // Duplicate the nibble, so "#abc" -> 0xaa, 0xbb, 0xcc
      int val = (nib << 4) + nib;

      argb[skip_alpha + i] = val;
    }
  } else if (len == 6 || len == 8) {
    bool skip_alpha = (len == 6);
    for (size_t i = 0; i + 1 < len; i += 2) {
      int nib1 = hex_nibble_value(name[i]);
      int nib2 = hex_nibble_value(name[i + 1]);
      if (nib1 < 0 || nib2 < 0) return parse_error(color, "invalid hex value");
      int val = (nib1 << 4) + nib2;

      argb[skip_alpha + i / 2] = val;
    }
  } else {
    return no_colour();
  }

  return Colour(argb[1], argb[2], argb[3], argb[0]);
}

Colour parse_color(const std::string &color) {
  std::optional<Colour> result;

#define TRY_PARSER(name)                            \
  std::optional<Colour> value_##name = name(color); \
  if (value_##name.has_value()) { return value_##name.value(); }

  TRY_PARSER(parse_color_name)
  TRY_PARSER(parse_hex_color)

#undef TRY_PARSER

  NORM_ERR("can't parse color '%s'", color.c_str());
  return ERROR_COLOUR;
}

Colour::Colour(const std::string &name) {
  const auto result = parse_color(name);
  this->red = result.red;
  this->green = result.green;
  this->blue = result.blue;
  this->alpha = result.alpha;
}
