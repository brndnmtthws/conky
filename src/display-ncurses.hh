/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018 François Revol et al.
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
  virtual bool draw_line_inner_required() { return true; }

  // drawing primitives
  virtual void set_foreground_color(long c);

  virtual void begin_draw_text();
  virtual void end_draw_text();
  virtual void draw_string(const char *s, int w);
  virtual void line_inner_done();

  virtual int getx();
  virtual int gety();
  virtual void gotox(int x);
  virtual void gotoy(int y);
  virtual void gotoxy(int x, int y);

  virtual void flush();

  // ncurses-specific
};

}  // namespace conky

#endif /* DISPLAY_NCURSES_HH */
