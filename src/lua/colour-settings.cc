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

#include "colour-settings.hh"

namespace priv {
void colour_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);
  Base::lua_setter(l, init);
  ++s;
}
}  // namespace priv

priv::colour_setting color[COLORS_CUSTOM] = {
    {"color0", white_argb32}, {"color1", white_argb32},
    {"color2", white_argb32}, {"color3", white_argb32},
    {"color4", white_argb32}, {"color5", white_argb32},
    {"color6", white_argb32}, {"color7", white_argb32},
    {"color8", white_argb32}, {"color9", white_argb32}};
priv::colour_setting default_color("default_color", white_argb32);
