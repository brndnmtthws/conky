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

#ifndef DISPLAY_X11_HH
#define DISPLAY_X11_HH

#include <limits>
#include <string>
#include <type_traits>

#include "luamm.hh"
#include "display-output.hh"

namespace conky {

/*
 * A base class for X11 display output.
 */
class display_output_x11 : public display_output_base {
 public:
  explicit display_output_x11();

  virtual ~display_output_x11() {}

  // check if available and enabled in settings
  virtual bool detect();
  // connect to DISPLAY and other stuff
  virtual bool initialize();
  virtual bool shutdown();

  // X11-specific
};

}  // namespace conky

#endif /* DISPLAY_X11_HH */
