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

#ifndef DISPLAY_NCURSES_HH
#define DISPLAY_NCURSES_HH

#include <limits>
#include <string>
#include <type_traits>

#include "luamm.hh"
#include "display-console.hh"

namespace conky {

/*
 * A base class for ncurses display output.
 */
class display_output_ncurses : public display_output_console {
 public:
  explicit display_output_ncurses();

  virtual ~display_output_ncurses() {}

  // check if available and enabled in settings
  virtual bool detect();
  // connect to DISPLAY and other stuff
  virtual bool initialize();
  virtual bool shutdown();

  // drawing primitives
  virtual bool set_foreground_color(long c);

  virtual bool begin_draw_text();
  virtual bool end_draw_text();
  virtual bool draw_string(const char *s, int w);
  virtual void line_inner_done();

  virtual int getx();
  virtual int gety();
  virtual bool gotox(int x);
  virtual bool gotoy(int y);
  virtual bool gotoxy(int x, int y);

  virtual bool flush();

  // ncurses-specific
};

}  // namespace conky

#endif /* DISPLAY_NCURSES_HH */
