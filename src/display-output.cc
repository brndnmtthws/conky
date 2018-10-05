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

#include <config.h>

#include "display-output.hh"

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace conky {
namespace {

typedef std::unordered_map<std::string, display_output_base *> display_outputs_t;

/*
 * We cannot construct this object statically, because order of object
 * construction in different modules is not defined, so register_source could be
 * called before this object is constructed. Therefore, we create it on the
 * first call to register_source.
 */
display_outputs_t *display_outputs;

}  // namespace

/*
 * The selected and active display output.
 */
std::vector<display_output_base *> active_display_outputs;

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
  if (not inserted) {
    throw std::logic_error("Display output with name '" + name +
                           "' already registered");
  }
}

}  // namespace priv

display_output_base::display_output_base(const std::string &name_)
    : name(name_), is_active(false), is_graphical(false), priority(-1) {
  priv::do_register_display_output(name, this);
}


disabled_display_output::disabled_display_output(
                                           const std::string &name,
                                           const std::string &define)
    : display_output_base(name) {
  priority = -2;
  // XXX some generic way of reporting errors? NORM_ERR?
  std::cerr << "Support for display output '" << name
            << "' has been disabled during compilation. Please recompile with '"
            << define << "'" << std::endl;
}

bool initialize_display_outputs() {
  std::vector<display_output_base *> outputs;
  outputs.reserve(display_outputs->size());

  for (auto &output : *display_outputs) {
    outputs.push_back(output.second);
  }
  sort(outputs.begin(), outputs.end(), &display_output_base::priority_compare);


  for (auto output : outputs) {
    if (output->priority < 0)
      continue;
    std::cerr << "Testing display output '" << output->name
              << "'... " << std::endl;
    if (output->detect()) {
      std::cerr << "Detected display output '" << output->name
                << "'... " << std::endl;
      if (output->initialize()) {
        std::cerr << "Initialized display output '" << output->name
                  << "'... " << std::endl;
        output->is_active = true;
        active_display_outputs.push_back(output);
      }
    }
  }
  if (active_display_outputs.size())
    return true;

  std::cerr << "Unable to find a usable display output." << std::endl;
  return true;//false;
}

bool shutdown_display_outputs() {
  bool ret = true;
  for (auto output : active_display_outputs)
    ret = output->shutdown();
  return ret;
}

}  // namespace conky

