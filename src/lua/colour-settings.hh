/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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

#include "../content/colours.hh"
#include "config.h"
#include "setting.hh"

namespace priv {
struct colour_traits {
  static const lua::Type type = lua::TSTRING;
  typedef Colour Type;

  static inline std::pair<Type, bool> convert(lua::state &l, int index,
                                              const std::string &) {
    return {parse_color(l.tostring(index)), true};
  }
};

class colour_setting
    : public conky::simple_config_setting<Colour, colour_traits> {
  typedef conky::simple_config_setting<Colour, colour_traits> Base;

 protected:
  virtual void lua_setter(lua::state &l, bool init);

 public:
  colour_setting(const std::string &name_, unsigned long default_value_ = 0)
      : Base(name_, Colour::from_argb32(default_value_), true) {}
};
}  // namespace priv

#define COLORS_CUSTOM 10

extern priv::colour_setting color[COLORS_CUSTOM];
extern priv::colour_setting default_color;

const unsigned long white_argb32 = 0xffffffff;
const unsigned long black_argb32 = 0xff000000;
