/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
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

#ifndef CONKY_OUTPUT_SETTING_H
#define CONKY_OUTPUT_SETTING_H

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <set>
#include <string_view>

#include <spdlog/fmt/fmt.h>

namespace conky {

/// List of display output backends that are supported by conky.
///
/// **Ordering** (value) determines priority when multiple outputs are selected.
/// However, `output_backend` function is expected to select only a reasonable
/// set of outputs based on settings, compile options and session environment.
enum class output_t : uint32_t {
  // Primary displays (single per process)
  WAYLAND,
  X11,
  NCURSES,

  // Secondary displays
  HTTP,
  FILE,
  CONSOLE,

  OUTPUT_COUNT
};

/// Display output backend **selection** function.
///
/// This function returns a set of backends that should be used for
/// presentation by conky.
///
/// - `output_backend` is the primary selection mechanism, however, if user
///   selects a backend that isn't compiled in, selection becomes ambiguous.
/// - Secondary selection is based on deprecated `out_to_*` settings
///
/// Result of this function is not fully reflective of the truth until backends
/// get initialised.
std::set<output_t> &output_backends();

/// True if the output was enabled in settings.
bool output_enabled(output_t output);

/// True if any of the `outputs` were enabled in settings.
static inline bool output_enabled(std::initializer_list<output_t> outputs) {
  for (output_t output : outputs) {
    if (output_enabled(output)) { return true; }
  }
  return false;
}

/// Returns the output that is being handled by the current thread.
std::optional<output_t> get_active_output();

/// True if the current thread is handling the given output.
bool output_is(output_t output);

/// True if the current thread is handling any of `outputs`.
static inline bool output_is(std::initializer_list<output_t> outputs) {
  for (output_t output : outputs) {
    if (output_is(output)) { return true; }
  }
  return false;
}
}  // namespace conky

template <>
struct fmt::formatter<conky::output_t> : fmt::formatter<std::string_view> {
  auto format(conky::output_t output, fmt::format_context &ctx) const {
    std::string_view name = "none";
    switch (output) {
      case conky::output_t::CONSOLE:
        name = "console";
        break;
      case conky::output_t::NCURSES:
        name = "ncurses";
        break;
      case conky::output_t::FILE:
        name = "file";
        break;
      case conky::output_t::HTTP:
        name = "HTTP";
        break;
      case conky::output_t::X11:
        name = "X11";
        break;
      case conky::output_t::WAYLAND:
        name = "Wayland";
        break;
      case conky::output_t::OUTPUT_COUNT:
        break;
    }
    return fmt::formatter<std::string_view>::format(name, ctx);
  }
};

#endif /* CONKY_OUTPUT_SETTING_H */
