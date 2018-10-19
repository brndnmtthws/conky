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

#include <limits>
#include <string>
#include <type_traits>
#include <string.h>

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
  bool is_active;
  bool is_graphical;
  int priority;

  explicit display_output_base(const std::string &name_);

  virtual ~display_output_base() {}

  static bool priority_compare(const display_output_base *a,
                          const display_output_base *b) {
    return a->priority > b->priority;
  }

  // check if available and enabled in settings
  virtual bool detect() { return false; }
  // connect to DISPLAY and other stuff
  virtual bool initialize() { return false; }
  virtual bool shutdown() { return false; }

  virtual bool graphical() { return is_graphical; };
  virtual bool draw_line_inner_required() { return is_graphical; }

  virtual bool main_loop_wait(double t) { return false; }

  virtual void sigterm_cleanup() { }
  virtual void cleanup() { }

  // drawing primitives
  virtual void set_foreground_color(long c) { }

  virtual int calc_text_width(const char *s) { return strlen(s); }

  virtual void begin_draw_text() { }
  virtual void end_draw_text() { }
  virtual void draw_string(const char *s, int w) { }
  virtual void line_inner_done() { }

  // GUI interface
  virtual void draw_string_at(int x, int y, const char *s, int w) { }
  // X11 lookalikes
  virtual void set_line_style(int w, bool solid) { }
  virtual void set_dashes(char *s) { }
  virtual void draw_line(int x1, int y1, int x2, int y2) { }
  virtual void draw_rect(int x, int y, int w, int h) { }
  virtual void fill_rect(int x, int y, int w, int h) { }
  virtual void draw_arc(int x, int y, int w, int h, int a1, int a2) { }
  virtual void move_win(int x, int y) { }

  virtual void begin_draw_stuff() { }
  virtual void end_draw_stuff() { }
  virtual void swap_buffers() { }
  virtual void clear_text(int exposures) { }
  virtual void load_fonts(bool utf8) { }

  // tty interface
  virtual int getx() { return 0; }
  virtual int gety() { return 0; }
  virtual void gotox(int x) { }
  virtual void gotoy(int y) { }
  virtual void gotoxy(int x, int y) { }

  virtual void flush() { }


  friend bool conky::initialize_display_outputs();
  friend bool conky::shutdown_display_outputs();

protected:
  virtual bool active() { return is_active; }
};

/*
 * The selected and active display outputs.
 */
extern std::vector<display_output_base *> active_display_outputs;

/*
 * the list of the only current output, when inside draw_text,
 * else we iterate over each active outputs.
 */
extern std::vector<conky::display_output_base *> current_display_outputs;

/*
 * Use this to declare a display output that has been disabled during compilation.
 * We can then print a nice error message telling the used which setting to
 * enable.
 */
class disabled_display_output : public display_output_base {
 public:
  const std::string define;
  disabled_display_output(const std::string &name,
                       const std::string &define);
};

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
//XXX; not really what intended yet...
  return conky::active_display_outputs[0];
  //return nullptr;
}

static inline void unset_display_output() {
  conky::current_display_outputs.clear();
}

static inline void set_display_output(conky::display_output_base *output) {
  conky::current_display_outputs.clear();
  conky::current_display_outputs.push_back(output);
}


#endif /* DISPLAY_OUTPUT_HH */
