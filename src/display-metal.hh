/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018 François Revol et al.
 * Copyright (C) 2018 Nickolas Pylarinos et al.
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

#ifndef DISPLAY_METAL_HH
#define DISPLAY_METAL_HH

#include <limits>
#include <string>
#include <type_traits>

#include "luamm.hh"
#include "display-output.hh"

namespace conky {

/*
 * A base class for Metal display output.
 */
class display_output_metal : public display_output_base {
 public:
  explicit display_output_metal();

  virtual ~display_output_metal() {}

  // check if available and enabled in settings
  virtual bool detect();
  // connect to DISPLAY and other stuff
  virtual bool initialize();
  virtual bool shutdown();

  virtual bool main_loop_wait(double t);

  virtual void sigterm_cleanup();
  virtual void cleanup();

  // drawing primitives
  virtual void set_foreground_color(long c);

  virtual int calc_text_width(const char *s);

  // GUI interface
  virtual void draw_string_at(int x, int y, const char *s, int w);
  // X11 lookalikes
  virtual void set_line_style(int w, bool solid);
  virtual void set_dashes(char *s);
  virtual void draw_line(int x1, int y1, int x2, int y2);
  virtual void draw_rect(int x, int y, int w, int h);
  virtual void fill_rect(int x, int y, int w, int h);
  virtual void draw_arc(int x, int y, int w, int h, int a1, int a2);
  virtual void move_win(int x, int y);

  virtual void end_draw_stuff();
  virtual void clear_text(int exposures);
  virtual void load_fonts(bool utf8);

  // metal-specific
};

}  // namespace conky

#endif /* DISPLAY_METAL_HH */
