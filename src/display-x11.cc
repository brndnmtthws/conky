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

#include "display-x11.hh"

#include <iostream>
#include <sstream>
#include <unordered_map>

namespace conky {
namespace {

#ifdef BUILD_X11
conky::display_output_x11 x11_output;
#else
conky::disabled_display_output x11_output_disabled("x11", "BUILD_X11");
#endif

}  // namespace

namespace priv {


}  // namespace priv

#ifdef BUILD_X11

display_output_x11::display_output_x11()
    : display_output_base("x11") {
  priority = 2;
}

bool display_output_x11::detect() {
  if (out_to_x.get(*state)) {
    std::cerr << "Display output '" << name << "' enabled in config." << std::endl;
    return true;
  }
  return false;
}

bool display_output_x11::initialize() {
  return false;
}

bool display_output_x11::shutdown() {
  return false;
}

#endif /* BUILD_X11 */

}  // namespace conky

