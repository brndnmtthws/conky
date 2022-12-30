/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018 François Revol et al.
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2021 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
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

#include "display-output.hh"
#include "logging.h"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace conky {
namespace {

typedef std::unordered_map<std::string, display_output_base *>
    display_outputs_t;

/*
 * We cannot construct this object statically, because order of object
 * construction in different modules is not defined, so register_source could be
 * called before this object is constructed. Therefore, we create it on the
 * first call to register_source.
 */
display_outputs_t *display_outputs;

}  // namespace

// HACK: force the linker to link all the objects in with test enabled
extern void init_console_output();
extern void init_ncurses_output();
extern void init_file_output();
extern void init_http_output();
extern void init_x11_output();
extern void init_wayland_output();

/*
 * The selected and active display output.
 */
std::vector<display_output_base *> active_display_outputs;

/*
 * the list of the only current output, when inside draw_text,
 * else we iterate over each active outputs.
 */
std::vector<conky::display_output_base *> current_display_outputs;

namespace priv {
void do_register_display_output(const std::string &name,
                                display_output_base *output) {
  struct display_output_constructor {
    display_output_constructor() { display_outputs = new display_outputs_t(); }
    ~display_output_constructor() {
      delete display_outputs;
      display_outputs = nullptr;
    }
  };
  static display_output_constructor constructor;

  bool inserted = display_outputs->insert({name, output}).second;
  if (!inserted) {
    throw std::logic_error("Display output with name '" + name +
                           "' already registered");
  }
}

}  // namespace priv

display_output_base::display_output_base(const std::string &name_)
    : name(name_), is_active(false), is_graphical(false), priority(-1) {
  priv::do_register_display_output(name, this);
}

disabled_display_output::disabled_display_output(const std::string &name,
                                                 const std::string &define)
    : display_output_base(name) {
  priority = -2;
  // XXX some generic way of reporting errors? NORM_ERR?
  DBGP(
      "Support for display output '%s' has been disabled during compilation. "
      "Please recompile with '%s'",
      name.c_str(), define.c_str());
}

bool initialize_display_outputs() {
  init_console_output();
  init_ncurses_output();
  init_file_output();
  init_http_output();
  init_x11_output();
  init_wayland_output();

  std::vector<display_output_base *> outputs;
  outputs.reserve(display_outputs->size());

  for (auto &output : *display_outputs) { outputs.push_back(output.second); }
  // Sort display outputs by descending priority, to try graphical ones first.
  sort(outputs.begin(), outputs.end(), &display_output_base::priority_compare);

  int graphical_count = 0;

  for (auto output : outputs) {
    if (output->priority < 0) continue;
    DBGP2("Testing display output '%s'... ", output->name.c_str());
    if (output->detect()) {
      DBGP2("Detected display output '%s'... ", output->name.c_str());

      if (graphical_count && output->graphical()) continue;

      // X11 init needs to draw, so we must add it to the list first.
      active_display_outputs.push_back(output);

      if (output->initialize()) {
        DBGP("Initialized display output '%s'... ", output->name.c_str());

        output->is_active = true;
        if (output->graphical()) graphical_count++;
        /*
         * We only support a single graphical display for now.
         * More than one text display (ncurses + http, ...) should be ok.
         */
        // if (graphical_count)
        // return true;
      } else {
        // failed, so remove from list
        active_display_outputs.pop_back();
      }
    }
  }
  if (active_display_outputs.size()) return true;

  std::cerr << "Unable to find a usable display output." << std::endl;
  return false;
}

bool shutdown_display_outputs() {
  bool ret = true;
  for (auto output : active_display_outputs) {
    output->is_active = false;
    ret = output->shutdown();
  }
  active_display_outputs.clear();
  return ret;
}

}  // namespace conky
