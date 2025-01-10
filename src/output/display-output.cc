/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018 François Revol et al.
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
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

#include "display-output.hh"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace conky {

inline void log_missing(const char *name, const char *flag) {
  DBGP(
      "%s display output disabled. Enable by recompiling with '%s' "
      "flag enabled.",
      name, flag);
}
#ifndef BUILD_HTTP
template <>
void register_output<output_t::HTTP>(display_outputs_t &outputs) {
  log_missing("HTTP", "BUILD_HTTP");
}
#endif
#ifndef BUILD_NCURSES
template <>
void register_output<output_t::NCURSES>(display_outputs_t &outputs) {
  log_missing("ncurses", "BUILD_NCURSES");
}
#endif
#ifndef BUILD_WAYLAND
template <>
void register_output<output_t::WAYLAND>(display_outputs_t &outputs) {
  log_missing("Wayland", "BUILD_WAYLAND");
}
#endif
#ifndef BUILD_X11
template <>
void register_output<output_t::X11>(display_outputs_t &outputs) {
  log_missing("X11", "BUILD_X11");
}
#endif

/*
 * The selected and active display output.
 */
std::vector<display_output_base *> active_display_outputs;

/*
 * the list of the only current output, when inside draw_text,
 * else we iterate over each active outputs.
 */
std::vector<conky::display_output_base *> current_display_outputs;

bool initialize_display_outputs() {
  std::vector<display_output_base *> outputs;
  outputs.reserve(static_cast<size_t>(output_t::OUTPUT_COUNT));

  // Order of registration is important!
  // - Graphical outputs go before textual (e.g. X11 before NCurses).
  // - Optional outputs go before non-optional (e.g. Wayland before X11).
  // - Newer outputs go before older (e.g. NCurses before (hypothetical) Curses).
  // - Fallbacks go last (in group)
  register_output<output_t::WAYLAND>(outputs);
  register_output<output_t::X11>(outputs);
  register_output<output_t::HTTP>(outputs);
  register_output<output_t::FILE>(outputs);
  register_output<output_t::NCURSES>(outputs);
  register_output<output_t::CONSOLE>(outputs);  // global fallback - always works

  for (auto out : outputs) { NORM_ERR("FOUND: %s", out->name.c_str()); }

  int graphical_count = 0;

  for (auto output : outputs) {
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
  return true;
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
