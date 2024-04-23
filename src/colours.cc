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

#include <optional>

Colour Colour::from_argb32(uint32_t argb) {
  Colour out;
  out.alpha = argb >> 24;
  out.red = (argb >> 16) % 256;
  out.green = (argb >> 8) % 256;
  out.blue = argb % 256;
  return out;
}

#include "colour-names.cc"

std::optional<Colour> parse_color_name(const std::string &name) {
  const rgb *value = color_name_hash::in_word_set(name.c_str(), name.length());

  if (value == nullptr) {
    return std::nullopt;
  } else {
    return Colour{value->red, value->green, value->blue};
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

  TRY_PARSER(parse_color_name)
  TRY_PARSER(parse_hex_color)

#undef TRY_PARSER

  return ERROR_COLOUR;
}

Colour::Colour(const std::string &name) {
  const auto result = parse_color(name);
  this->red = result.red;
  this->green = result.green;
  this->blue = result.blue;
  this->alpha = result.alpha;
}
