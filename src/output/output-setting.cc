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

#include "config.h"

#include "output-setting.hh"

#include <initializer_list>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>

#include <spdlog/fmt/bundled/format.h>

#include "../logging.h"
#include "../lua/setting.hh"
#include "../system-details.hh"

extern std::unique_ptr<lua::state> state;  // declared in conky.cc

using conky::output_t;

template <>
conky::lua_traits<output_t>::Map conky::lua_traits<output_t>::map = {
    {"console", output_t::CONSOLE}, {"ncurses", output_t::NCURSES},
    {"file", output_t::FILE},       {"http", output_t::HTTP},
    {"x11", output_t::X11},         {"wayland", output_t::WAYLAND},
};

namespace {

/*
 * Parses a config value into a set of output backends. Accepts either a single
 * backend name (string) or a list of names (table of strings), so both
 * `output_backend = 'x11'` and `output_backend = {'x11', 'ncurses'}` parse.
 * Returns {parsed_set, ok}; on malformed input it logs an error and ok is
 * false. Leaves the stack unchanged.
 */
std::pair<std::set<output_t>, bool> parse_output_backends(
    lua::state &l, int index, const std::string &name) {
  std::set<output_t> result;
  index = l.absindex(index);

  if (l.isnil(index)) { return {result, true}; }

  lua::Type type = l.type(index);
  if (type == lua::TSTRING) {
    auto value = conky::lua_traits<output_t>::convert(l, index, name);
    if (value.second) { result.insert(value.first); }
    return {result, value.second};
  }

  if (type == lua::TTABLE) {
    bool ok = true;
    for (int i = 1;; ++i) {
      l.rawgeti(index, i);
      if (l.isnil(-1)) {
        l.pop();
        break;
      }
      if (l.type(-1) != lua::TSTRING) {
        LOG_ERROR(
            "invalid element of type '{}' in list setting '{}'; expected a "
            "backend name",
            l.type_name(l.type(-1)), name);
        ok = false;
      } else {
        auto value = conky::lua_traits<output_t>::convert(l, -1, name);
        if (value.second) {
          result.insert(value.first);
        } else {
          ok = false;
        }
      }
      l.pop();
    }
    return {result, ok};
  }

  LOG_ERROR(
      "invalid value of type '{}' for setting '{}'; expected a backend name or "
      "a list of backend names",
      l.type_name(type), name);
  return {result, false};
}

/*
 * `output_backend` accepts either a single backend name or a list of names, so
 * it can't use simple_config_setting (which is locked to a single Lua type).
 * We derive from config_setting_template directly and parse the union here.
 */
class output_backend_setting_t
    : public conky::config_setting_template<std::set<output_t>> {
  using Base = conky::config_setting_template<std::set<output_t>>;

 public:
  explicit output_backend_setting_t(const std::string &name) : Base(name) {}

 protected:
  std::set<output_t> getter(lua::state &l) override {
    lua::stack_sentry s(l, -1);
    auto result = parse_output_backends(l, -1, name);
    l.pop();
    return result.first;
  }

  void lua_setter(lua::state &l, bool init) override {
    lua::stack_sentry s(l, -2);

    bool ok = init;
    if (!init) { LOG_ERROR("setting '{}' is not modifiable", name); }
    if (ok) { ok = parse_output_backends(l, -2, name).second; }

    if (ok) {
      l.pop();  // accept the new value, drop the old one
    } else {
      l.replace(-2);  // reject: restore the old value
    }
    ++s;
  }
};

output_backend_setting_t output_backend_setting("output_backend");

/*
 * Deprecated per-backend toggles. They no longer drive output creation -
 * output_backend() does - but remain registered so old configs keep parsing
 * and so the selection heuristic can still read them.
 */

conky::simple_config_setting<bool> out_to_x = conky::deprecated(
    conky::simple_config_setting<bool>("out_to_x", false, false),
    "set `output_backend = 'x11'` instead; out_to_x is only respected on X11 "
    "sessions");

conky::simple_config_setting<bool> out_to_wayland = conky::deprecated(
    conky::simple_config_setting<bool>("out_to_wayland", false, false),
    "set `output_backend = 'wayland'` instead");

conky::simple_config_setting<bool> out_to_ncurses = conky::deprecated(
    conky::simple_config_setting<bool>("out_to_ncurses", false, false),
    "set `output_backend = 'ncurses'` instead");

conky::simple_config_setting<bool> out_to_http = conky::deprecated(
    conky::simple_config_setting<bool>("out_to_http", false, false),
    "set `output_backend = 'http'` instead");
}  // namespace

conky::simple_config_setting<bool> out_to_stdout = conky::deprecated(
    conky::simple_config_setting<bool>("out_to_console", false, false),
    "set `output_backend = 'console'` instead");
extern conky::simple_config_setting<bool> out_to_stderr;

// defined in display-file.cc; used below to infer the file backend
extern conky::simple_config_setting<std::string> overwrite_file;
extern conky::simple_config_setting<std::string> append_file;

namespace {

// Optional backends paired with their build flag and display name. FILE and
// CONSOLE are always available, so they're omitted here and is_built() defaults
// anything unlisted to true.
struct backend_descriptor {
  output_t output;
  std::string_view name;
  bool built;
};
constexpr backend_descriptor BACKENDS[] = {
    {output_t::WAYLAND, "Wayland", BUILD_WAYLAND_V},
    {output_t::X11, "X11", BUILD_X11_V},
    {output_t::NCURSES, "ncurses", BUILD_NCURSES_V},
    {output_t::HTTP, "HTTP", BUILD_HTTP_V},
};

constexpr bool is_built(output_t output) {
  for (const auto &backend : BACKENDS) {
    if (backend.output == output) { return backend.built; }
  }
  return true;  // FILE and CONSOLE are always available
}

// Graphical surfaces and the full-screen terminal UI are mutually exclusive;
// at most one of these may be active at a time.
constexpr output_t PRIMARY_OUTPUTS[] = {output_t::X11, output_t::WAYLAND,
                                        output_t::NCURSES};

bool has_no_primary(std::set<output_t> &outputs) {
  for (output_t primary : PRIMARY_OUTPUTS) {
    if (outputs.contains(primary)) return false;
  }
  return true;
}

// Joins formattable items with ", " using their fmt formatter.
template <typename Range>
std::string join(const Range &items) {
  std::string result;
  for (const auto &item : items) {
    if (!result.empty()) { result += ", "; }
    result += fmt::format("{}", item);
  }
  return result;
}

// Picks the single primary output to keep for the given session, in descending
// preference:
//   - Wayland session: Wayland, X11 (via XWayland), ncurses
//   - X11 session:     X11, ncurses  (Wayland needs a running compositor)
//   - headless:        ncurses
// Only built backends are considered; returns nullopt when none is viable.
std::optional<output_t> choose_primary(const std::set<output_t> &requested,
                                       conky::info::display_session session) {
  using conky::info::display_session;
  static constexpr output_t wayland_prefs[] = {output_t::WAYLAND, output_t::X11,
                                               output_t::NCURSES};
  static constexpr output_t x11_prefs[] = {output_t::X11, output_t::NCURSES};
  static constexpr output_t headless_prefs[] = {output_t::NCURSES};

  std::span<const output_t> prefs = headless_prefs;
  switch (session) {
    case display_session::wayland:
      prefs = wayland_prefs;
      break;
    case display_session::x11:
      prefs = x11_prefs;
      break;
    default:
      break;
  }

  for (output_t output : prefs) {
    if (requested.contains(output) && is_built(output)) { return output; }
  }
  return std::nullopt;
}

// Collapses multiple requested primaries to one, by session viability. Warns
// only when the user picked nothing *but* primaries; combined with other
// outputs (e.g. {x11, wayland, http}) the multi-primary set is an intentional
// "auto-pick a GUI, keep the rest" config and reduces silently.
void reduce_to_single_primary(std::set<output_t> &outputs,
                              conky::info::display_session session) {
  std::set<output_t> requested_primaries;
  for (output_t output : PRIMARY_OUTPUTS) {
    if (outputs.contains(output)) { requested_primaries.insert(output); }
  }
  if (requested_primaries.size() <= 1) { return; }

  if (outputs.size() == requested_primaries.size()) {
    LOG_WARNING(
        "only a single primary output may be used at a time, you picked: {}; "
        "selection will be reduced based on current session",
        join(requested_primaries));
    if (!requested_primaries.contains(output_t::NCURSES)) {
      LOG_INFO(
          "you can omit `output_backend` and `out_to_*` settings to have conky "
          "pick between Wayland/X11 automatically based on active session");
    }
  }

  std::optional<output_t> keep = choose_primary(outputs, session);
  for (output_t output : PRIMARY_OUTPUTS) {
    if (keep != output) { outputs.erase(output); }
  }
}

// Applies deprecated out_to_* hints: the GUI hints only fill a gap (never
// overriding an explicit `output_backend`), while HTTP and console are
// additive.
void apply_legacy_hints(std::set<output_t> &outputs) {
  if (out_to_wayland.get(*state) && has_no_primary(outputs)) {
    outputs.insert(output_t::WAYLAND);
  }
  if (out_to_ncurses.get(*state) && has_no_primary(outputs)) {
    outputs.insert(output_t::NCURSES);
  }
  if (out_to_http.get(*state)) { outputs.insert(output_t::HTTP); }
  if (out_to_stdout.get(*state)) { outputs.insert(output_t::CONSOLE); }
}

// Removes backends this build doesn't support, warning about each dropped one.
void drop_unsupported(std::set<output_t> &outputs) {
  std::set<std::string_view> dropped;
  for (const auto &backend : BACKENDS) {
    if (!backend.built && outputs.erase(backend.output) > 0) {
      dropped.insert(backend.name);
    }
  }
  if (!dropped.empty()) {
    LOG_WARNING(
        "you picked {0} backend(s) which will not be used because conky wasn't "
        "built to support it/them; you need to build and/or install a conky "
        "flavor that supports {0}",
        join(dropped));
  }
}

// Adds outputs implied by output-specific settings.
void apply_from_ambient_settings(std::set<output_t> &outputs) {
  if (overwrite_file.is_set(*state) || append_file.is_set(*state)) {
    LOG_DEBUG("using file backend due to `overwrite_file`/`append_file`");
    outputs.insert(output_t::FILE);
  }
  if (out_to_stderr.get(*state)) {
    LOG_DEBUG("using console backend due to `out_to_stderr` setting");
    outputs.insert(output_t::CONSOLE);
  }
}

// When nothing has been selected yet, infers a default from the session,
// finally falling back to console.
void apply_fallback(std::set<output_t> &outputs, conky::info::system *system) {
#ifdef BUILD_GUI
  if (outputs.empty()) {
    LOG_DEBUG("using system display session info to infer default backend...");
    switch (system->session) {
      case conky::info::display_session::wayland:
        if constexpr (BUILD_WAYLAND_V) {
          outputs.insert(output_t::WAYLAND);
        } else {
          LOG_WARNING(
              "conky wasn't built with Wayland backend, consider using a "
              "different build/installation to use the Wayland backend");
          if (BUILD_X11_V && out_to_x.get(*state)) {
            LOG_INFO(
                "using X11 backend via XWayland due to `out_to_x` setting");
            outputs.insert(output_t::X11);
          }
        }
        break;
      case conky::info::display_session::x11:
        if constexpr (BUILD_X11_V) {
          outputs.insert(output_t::X11);
        } else {
          LOG_WARNING(
              "conky wasn't built with X11 backend, consider using a different "
              "build/installation to use the X11 backend");
        }
        break;
      default:
        LOG_WARNING(
            "conky wasn't able to detect a running graphical session, output "
            "will be printed to console; if you're starting conky from an "
            "autorun file, you should probably add a delay or from a systemd "
            "service with something like:\n"
            "[Unit]\nAfter=graphical-session.target\n\n"
            "Check your compositor/window manager documentation for details.");
        break;
    }
    if (!outputs.empty()) {
      LOG_INFO(
          "using {} output as implicit default for your system due to your "
          "current session",
          *outputs.begin());
      return;
    }
  }
#endif /* BUILD_GUI */

  if (outputs.empty()) {
    LOG_DEBUG("using console output as the fallback");
    outputs.insert(output_t::CONSOLE);
  }
}

}  // namespace

namespace conky {

std::set<output_t> resolved_outputs;

std::set<output_t> parse_output_settings() {
  std::set<output_t> result;
  conky::info::system *system = user_system();

  std::set<output_t> requested = output_backend_setting.get(*state);
  result.insert(requested.begin(), requested.end());

  reduce_to_single_primary(result, system->session);
  apply_legacy_hints(result);
  drop_unsupported(result);
  apply_from_ambient_settings(result);
  apply_fallback(result, system);
  return result;
}

// Resolves which output backends conky should drive, combining (in order) the
// explicit `output_backend` setting, the single-primary constraint, deprecated
// out_to_* hints, the file/console settings and session-based inference.
// Memoized: resolved once, then the same set is returned on every call.
std::set<output_t> &output_backends() {
  if (resolved_outputs.empty()) {
    CRIT_ERR(
        "output_backends are incorrect before backend initialization is "
        "attempted");
  }
  return resolved_outputs;
}

static thread_local output_t active_output = output_t::OUTPUT_COUNT;

static bool outputs_ever_set = false;
void set_active_output(output_t output) {
  outputs_ever_set = true;
  active_output = output;
}

std::optional<output_t> get_active_output() {
  if (active_output == output_t::OUTPUT_COUNT) { return std::nullopt; }
  return active_output;
}

bool output_is(output_t output) {
  if (!outputs_ever_set) {
    CRIT_ERR(
        "attempt to call output_is before any outputs were ever set is "
        "invalid; we don't have enough information yet");
  }
  if (output == output_t::OUTPUT_COUNT ||
      active_output == output_t::OUTPUT_COUNT) {
    return false;
  }
  return active_output == output;
}

bool output_enabled(output_t output) {
  if (resolved_outputs.empty()) {
    CRIT_ERR(
        "attempt to call output_enabled before backend initialization is "
        "attempted is invalid; we don't have enough information yet");
  }
  if (output == output_t::OUTPUT_COUNT) { return false; }
  return resolved_outputs.contains(output);
}

}  // namespace conky
