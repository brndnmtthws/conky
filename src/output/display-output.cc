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

#include <config.h>

#include <vector>

#include "../logging.h"
#include "display-output.hh"
#include "output-setting.hh"

namespace conky {
/*
 * The selected and active display output.
 *
 * This list contains pointers to output objects that are returned by
 * `output_backends`.
 */
std::vector<display_output_base *> active_display_outputs;

/*
 * the list of the only current output, when inside draw_text,
 * else we iterate over each active outputs.
 */
std::vector<conky::display_output_base *> current_display_outputs;

display_outputs_t &registered_outputs() {
  static display_outputs_t map;
  return map;
}

extern std::set<output_t> resolved_outputs;
std::set<output_t> parse_output_settings();

void initialize_display_outputs() {
  for (auto [t, out] : registered_outputs()) {
    LOG_DEBUG("found display output '{}'", out->name);
  }

  auto selected_outputs = parse_output_settings();
  std::set<output_t> initialized_selection;

  for (auto output_type : selected_outputs) {
    auto primary_it = registered_outputs().find(output_type);
    if (primary_it == registered_outputs().end()) continue;
    auto output = primary_it->second;

    LOG_DEBUG("initializing '{}' display output", output->name);
    if (output->initialize()) {
      LOG_DEBUG("initialized display output '{}'", output->name);
      initialized_selection.insert(output_type);
    }
  }

  if (initialized_selection.empty()) {
    LOG_WARNING(
        "unable to initialize any outputs, falling back to console output");
    initialized_selection.insert(output_t::CONSOLE);
    registered_outputs()[output_t::CONSOLE]->initialize();
  }

  resolved_outputs = initialized_selection;

  for (output_t out : resolved_outputs) {
    active_display_outputs.push_back(registered_outputs()[out]);
  }
}

std::optional<display_output_base *> get_registered_output(output_t output) {
  auto located = registered_outputs().find(output);
  if (located == registered_outputs().end()) return std::nullopt;
  return located->second;
}

bool shutdown_display_outputs() {
  bool ret = true;
  for (auto output : active_display_outputs) { ret = output->shutdown(); }
  active_display_outputs.clear();
  return ret;
}

void set_active_output(conky::output_t output);

}  // namespace conky

void set_display_output(conky::display_output_base *output) {
  conky::current_display_outputs.clear();
  if (output != nullptr) {
    // OLD: std::vector always holding a single output
    conky::current_display_outputs.push_back(output);
    // NEW: thread-local single value
    conky::set_active_output(output->type);
  }
}
