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

#ifndef DISPLAY_MOCK_HH
#define DISPLAY_MOCK_HH

#include "colours.h"
#include "display-output.hh"

namespace mock {
/// These are called by Catch2 events, DO NOT use them directly
namespace __internal {
void init_display_output_mock();
void delete_display_output_mock();
}  // namespace __internal

/*
 * A base class for mock display output that emulates a GUI.
 */
class display_output_mock : public conky::display_output_base {
  // Use `mock::get_mock_output`.
  explicit display_output_mock() : conky::display_output_base("mock") {};
  ~display_output_mock() {};

 public:
  float dpi_scale = 1.0;

  // check if available and enabled in settings
  bool detect() { return true; }
  // connect to DISPLAY and other stuff
  bool initialize() { return true; }
  bool shutdown() { return true; }

  bool graphical() { return true; };
  bool draw_line_inner_required() { return true; }

  bool main_loop_wait(double t) { return false; }

  void sigterm_cleanup() {}
  void cleanup() {}

  // drawing primitives
  void set_foreground_color(Colour c) {}

  int calc_text_width(const char *s) { return 0; }

  void begin_draw_text() {}
  void end_draw_text() {}
  void draw_string(const char *s, int w) {}
  void line_inner_done() {}

  // GUI interface
  void draw_string_at(int /*x*/, int /*y*/, const char * /*s*/, int /*w*/) {}
  // X11 lookalikes
  void set_line_style(int /*w*/, bool /*solid*/) {}
  void set_dashes(char * /*s*/) {}
  void draw_line(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/) {}
  void draw_rect(int /*x*/, int /*y*/, int /*w*/, int /*h*/) {}
  void fill_rect(int /*x*/, int /*y*/, int /*w*/, int /*h*/) {}
  void draw_arc(int /*x*/, int /*y*/, int /*w*/, int /*h*/, int /*a1*/,
                int /*a2*/) {}
  void move_win(int /*x*/, int /*y*/) {}
  float get_dpi_scale() { return dpi_scale; };

  void begin_draw_stuff() {}
  void end_draw_stuff() {}
  void clear_text(int /*exposures*/) {}

  // font stuff
  int font_height(unsigned int) { return 0; }
  int font_ascent(unsigned int) { return 0; }
  int font_descent(unsigned int) { return 0; }
  void setup_fonts(void) {}
  void set_font(unsigned int) {}
  void free_fonts(bool /*utf8*/) {}
  void load_fonts(bool /*utf8*/) {}

  // tty interface
  int getx() { return 0; }
  int gety() { return 0; }
  void gotox(int /*x*/) {}
  void gotoy(int /*y*/) {}
  void gotoxy(int /*x*/, int /*y*/) {}

  void flush() {}

 protected:
  bool active() { return true; }

  friend void __internal::init_display_output_mock();
  friend void __internal::delete_display_output_mock();
};

display_output_mock &get_mock_output();
}  // namespace mock

#endif /* DISPLAY_MOCK_HH */
