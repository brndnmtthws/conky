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
#include <type_traits>
#include <variant>

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

/// Parsing result can be either a `Colour`, a none variant, or an error
/// message.
class parse_result {
  std::variant<Colour, std::monostate, std::string> value;

 public:
  parse_result(Colour c) : value(c) {}
  parse_result() : value(std::monostate{}) {}
  parse_result(std::string msg) : value(std::move(msg)) {}

  static parse_result error(const char *format...) {
    va_list args;
    va_start(args, format);
    size_t len = snprintf(nullptr, 0, format, args);
    va_end(args);

    char *buffer = new char[len + 1];
    va_start(args, format);
    snprintf(buffer, len, format, args);
    va_end(args);

    auto value = std::string(buffer);
    delete[] buffer;
    return parse_result(value);
  }

  const bool has_colour() const {
    return std::holds_alternative<Colour>(value);
  }
  const std::optional<Colour> get_colour() const {
    if (std::holds_alternative<Colour>(value)) {
      return std::get<Colour>(std::move(value));
    } else {
      return std::nullopt;
    }
  }
  const std::optional<std::string> get_error() const {
    if (std::holds_alternative<std::string>(value)) {
      return std::get<std::string>(std::move(value));
    } else {
      return std::nullopt;
    }
  }
};

parse_result none() { return parse_result(); }

parse_result parse_color_name(const std::string &name) {
  const rgb *value = color_name_hash::in_word_set(name.c_str(), name.length());
  if (value != nullptr) {
    return Colour{value->red, value->green, value->blue};
  } else {
    return none();
  }
}

parse_result parse_hex_color(const std::string &color) {
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
      if (nib < 0)
        return parse_result::error("invalid hex color: '%s' (%d)",
                                   color.c_str(), color.length());
      // Duplicate the nibble, so "#abc" -> 0xaa, 0xbb, 0xcc
      int val = (nib << 4) + nib;

      argb[skip_alpha + i] = val;
    }
  } else if (len == 6 || len == 8) {
    bool skip_alpha = (len == 6);
    for (size_t i = 0; i + 1 < len; i += 2) {
      int nib1 = hex_nibble_value(name[i]);
      int nib2 = hex_nibble_value(name[i + 1]);
      if (nib1 < 0 || nib2 < 0)
        return parse_result::error("invalid hex color: '%s' (%d)",
                                   color.c_str(), color.length());
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

#define TRY_PARSER(name)                      \
  parse_result value_##name = name(color);    \
  if (value_##name.has_colour()) {            \
    return value_##name.get_colour().value(); \
  } else {                                    \
    auto err = value_##name.get_error();      \
    if (err.has_value()) {                    \
      NORM_ERR(err.value().c_str());          \
      return ERROR_COLOUR;                    \
    }                                         \
  }

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
