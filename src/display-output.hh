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

#ifndef DISPLAY_OUTPUT_HH
#define DISPLAY_OUTPUT_HH

#include <string.h>
#include <cmath>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

#include "colours.h"
#include "logging.h"
#include "luamm.hh"

namespace conky {

bool initialize_display_outputs();

bool shutdown_display_outputs();

/*
 * A base class for all display outputs.
 * API consists of two functions:
 * - get_number should return numeric representation of the data (if available).
 * This can then be used when drawing graphs, bars, ... The default
 * implementation returns NaN.
 * - get_text should return textual representation of the data. This is used
 * when simple displaying the value of the data source. The default
 * implementation converts get_number() to a string, but you can override to
 * return anything (e.g. add units)
 */
class display_output_base {
 private:
  // copying is a REALLY bad idea
  display_output_base(const display_output_base &) = delete;
  display_output_base &operator=(const display_output_base &) = delete;

 public:
  const std::string name;
  bool is_active = false;
  bool is_graphical = false;

  explicit display_output_base(const std::string &name) : name(name){};

  virtual ~display_output_base() {}

  // check if available and enabled in settings
  virtual bool detect() { return false; }
  // connect to DISPLAY and other stuff
  virtual bool initialize() { return false; }
  virtual bool shutdown() { return false; }

  virtual bool graphical() { return is_graphical; };
  virtual bool draw_line_inner_required() { return is_graphical; }

  virtual bool main_loop_wait(double /*t*/) { return false; }

  virtual void sigterm_cleanup() {}
  virtual void cleanup() {}

  // drawing primitives
  virtual void set_foreground_color(Colour /*c*/) {}

  virtual int calc_text_width(const char *s) { return strlen(s); }

  virtual void begin_draw_text() {}
  virtual void end_draw_text() {}
  virtual void draw_string(const char * /*s*/, int /*w*/) {}
  virtual void line_inner_done() {}

  // GUI interface
  virtual void draw_string_at(int /*x*/, int /*y*/, const char * /*s*/,
                              int /*w*/) {}
  // X11 lookalikes
  virtual void set_line_style(int /*w*/, bool /*solid*/) {}
  virtual void set_dashes(char * /*s*/) {}
  virtual void draw_line(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/) {}
  virtual void draw_rect(int /*x*/, int /*y*/, int /*w*/, int /*h*/) {}
  virtual void fill_rect(int /*x*/, int /*y*/, int /*w*/, int /*h*/) {}
  virtual void draw_arc(int /*x*/, int /*y*/, int /*w*/, int /*h*/, int /*a1*/,
                        int /*a2*/) {}
  virtual void move_win(int /*x*/, int /*y*/) {}
  virtual float get_dpi_scale() { return 1.0; };

  virtual void begin_draw_stuff() {}
  virtual void end_draw_stuff() {}
  virtual void clear_text(int /*exposures*/) {}

  // font stuff
  virtual int font_height(unsigned int) { return 0; }
  virtual int font_ascent(unsigned int) { return 0; }
  virtual int font_descent(unsigned int) { return 0; }
  virtual void setup_fonts(void) {}
  virtual void set_font(unsigned int) {}
  virtual void free_fonts(bool /*utf8*/) {}
  virtual void load_fonts(bool /*utf8*/) {}

  // tty interface
  virtual int getx() { return 0; }
  virtual int gety() { return 0; }
  virtual void gotox(int /*x*/) {}
  virtual void gotoy(int /*y*/) {}
  virtual void gotoxy(int /*x*/, int /*y*/) {}

  virtual void flush() {}

  friend bool conky::initialize_display_outputs();
  friend bool conky::shutdown_display_outputs();

 protected:
  virtual bool active() { return is_active; }
};

using display_outputs_t = std::vector<display_output_base *>;

enum class output_t : uint32_t {
  CONSOLE,
  NCURSES,
  FILE,
  HTTP,
  X11,
  WAYLAND,
  OUTPUT_COUNT
};
template <output_t Output>
void register_output(display_outputs_t &outputs);

/*
 * The selected and active display outputs.
 */
extern std::vector<display_output_base *> active_display_outputs;

/*
 * the list of the only current output, when inside draw_text,
 * else we iterate over each active outputs.
 */
extern std::vector<conky::display_output_base *> current_display_outputs;

}  // namespace conky

// XXX: move to namespace?

static inline std::vector<conky::display_output_base *> &display_outputs() {
  if (conky::current_display_outputs.size())
    return conky::current_display_outputs;
  return conky::active_display_outputs;
}

static inline conky::display_output_base *display_output() {
  if (conky::current_display_outputs.size())
    return conky::current_display_outputs[0];
  // XXX; not really what intended yet...
  if (conky::active_display_outputs.size())
    return conky::active_display_outputs[0];
  return nullptr;
}

template <typename T>
inline T dpi_scale(T value) {
  static_assert(std::is_arithmetic_v<T>,
                "dpi_scale value type must be a number");
#ifdef BUILD_GUI
  auto output = display_output();
  if (output) {
    return T(std::round(static_cast<double>(value) * output->get_dpi_scale()));
  }
#endif /* BUILD_GUI */

  return value;
}

static inline void unset_display_output() {
  conky::current_display_outputs.clear();
}

static inline void set_display_output(conky::display_output_base *output) {
  conky::current_display_outputs.clear();
  conky::current_display_outputs.push_back(output);
}

#endif /* DISPLAY_OUTPUT_HH */
