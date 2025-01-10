/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018-2021 François Revol et al.
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

#ifndef DISPLAY_WAYLAND_HH
#define DISPLAY_WAYLAND_HH

#include <limits>
#include <string>
#include <type_traits>

#include "../content/colours.hh"
#include "../lua/luamm.hh"
#include "display-output.hh"
#include "wl.h"

namespace conky {

/*
 * A base class for Wayland display output.
 */
class display_output_wayland : public display_output_base {
 public:
  explicit display_output_wayland();

  virtual ~display_output_wayland() {}

  // check if available and enabled in settings
  virtual bool detect();
  // connect to DISPLAY and other stuff
  virtual bool initialize();
  virtual bool shutdown();

  virtual bool main_loop_wait(double);

  virtual void sigterm_cleanup();
  virtual void cleanup();

  // drawing primitives
  virtual void set_foreground_color(Colour);

  virtual int calc_text_width(const char *);

  // GUI interface
  virtual void draw_string_at(int, int, const char *, int);
  // X11 lookalikes
  virtual void set_line_style(int, bool);
  virtual void set_dashes(char *);
  virtual void draw_line(int, int, int, int);
  virtual void draw_rect(int, int, int, int);
  virtual void fill_rect(int, int, int, int);
  virtual void draw_arc(int, int, int, int, int, int);
  virtual void move_win(int, int);
  virtual float get_dpi_scale();

  virtual void end_draw_stuff();
  virtual void clear_text(int);

  virtual int font_height(unsigned int);
  virtual int font_ascent(unsigned int);
  virtual int font_descent(unsigned int);
  virtual void setup_fonts(void);
  virtual void set_font(unsigned int);
  virtual void free_fonts(bool);
  virtual void load_fonts(bool);

  // Wayland-specific
};

}  // namespace conky

#endif /* DISPLAY_WAYLAND_HH */
