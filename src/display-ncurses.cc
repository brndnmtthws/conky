/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2010 Pavel Labath et al.
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

#include "conky.h"
#include "nc.h"
#include "display-ncurses.hh"

#include <iostream>
#include <sstream>
#include <unordered_map>

namespace conky {
namespace {

#ifdef BUILD_NCURSES
conky::display_output_ncurses ncurses_output;
#else
conky::disabled_display_output ncurses_output_disabled("ncurses", "BUILD_NCURSES");
#endif

}  // namespace

namespace priv {


}  // namespace priv

#ifdef BUILD_NCURSES

display_output_ncurses::display_output_ncurses()
    : display_output_console("ncurses") {
  priority = 1;
}

bool display_output_ncurses::detect() {
  if (out_to_ncurses.get(*state)) {
    std::cerr << "Display output '" << name << "' enabled in config." << std::endl;
    return true;
  }
  return false;
}

bool display_output_ncurses::initialize() {
  return false;
}

bool display_output_ncurses::shutdown() {
  return false;
}

#endif /* BUILD_NCURSES */

}  // namespace conky

