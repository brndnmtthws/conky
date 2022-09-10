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

#ifndef DISPLAY_OUTPUT_HH
#define DISPLAY_OUTPUT_HH

#include <limits>
#include <string>
#include <type_traits>

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
  int priority;

  explicit display_output_base(const std::string &name_);

  virtual ~display_output_base() {}

  static bool priority_compare(const display_output_base *a,
                               const display_output_base *b) {
    return a->priority > b->priority;
  }

  // check if available and enabled in settings
  virtual bool detect() { return false; };
  // connect to DISPLAY and other stuff
  virtual bool initialize() { return false; };
  virtual bool shutdown() { return false; };

  // drawing primitives
  virtual bool begin_draw_text() { return false; };
  virtual bool end_draw_text() { return false; };
  virtual bool draw_string(const char *s, int w) { return false; };

  friend bool conky::initialize_display_outputs();
  friend bool conky::shutdown_display_outputs();

 protected:
  virtual bool active() { return is_active; };
};

/*
 * The selected and active display outputs.
 */
extern std::vector<display_output_base *> active_display_outputs;

/*
 * Use this to declare a display output that has been disabled during
 * compilation. We can then print a nice error message telling the used which
 * setting to enable.
 */
class disabled_display_output : public display_output_base {
 public:
  const std::string define;
  disabled_display_output(const std::string &name, const std::string &define);
};

}  // namespace conky

#endif /* DISPLAY_OUTPUT_HH */
