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
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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
 
#include <config.h>

#include "nc.h"

#ifdef BUILD_NCURSES
WINDOW *ncurses_window;
#endif

namespace priv {
void out_to_ncurses_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init && do_convert(l, -1).first) {
    ncurses_window = initscr();
    start_color();
  }

  ++s;
}

void out_to_ncurses_setting::cleanup(lua::state &l) {
  lua::stack_sentry s(l, -1);

  if (do_convert(l, -1).first) endwin();

  l.pop();
}
}  // namespace priv

priv::out_to_ncurses_setting out_to_ncurses;
