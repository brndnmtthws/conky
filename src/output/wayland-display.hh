/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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

#ifndef CONKY_WAYLAND_DISPLAY_HH
#define CONKY_WAYLAND_DISPLAY_HH

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace conky {

/// @brief Parsed components of the `display` setting for the Wayland backend.
struct wayland_display_spec {
  /// Wayland socket name to pass to wl_display_connect(), or empty for the
  /// default socket (WAYLAND_DISPLAY).
  std::string socket_name;
  /// Target output selector: wl_output name (e.g. "DP-2"), description, or
  /// 0-based ordinal index. Empty means compositor default (nullptr).
  std::string output_selector;
};

namespace detail {

inline bool is_ascii_digits(std::string_view s) {
  if (s.empty()) { return false; }
  for (char c : s) {
    if (c < '0' || c > '9') { return false; }
  }
  return true;
}

inline std::optional<size_t> parse_nonneg_index(std::string_view s) {
  if (!is_ascii_digits(s)) { return std::nullopt; }
  size_t value = 0;
  for (char c : s) {
    size_t digit = static_cast<size_t>(c - '0');
    if (value > (static_cast<size_t>(-1) - digit) / 10) { return std::nullopt; }
    value = value * 10 + digit;
  }
  return value;
}

}  // namespace detail

/// @brief Parses a Wayland `display` config value into socket + output parts.
///
/// Supported forms:
/// - ""                -> default socket, compositor-chosen output
/// - "DP-2"            -> default socket, output named/described "DP-2"
/// - "1"               -> default socket, output index 1 (or named "1")
/// - "wayland-0"       -> socket "wayland-0", compositor-chosen output
/// - "wayland-0.1"     -> socket "wayland-0", output selector "1"
/// - "wayland-0.DP-2"  -> socket "wayland-0", output selector "DP-2"
/// - ":0"              -> socket "wayland-0", compositor-chosen output
/// - ":0.1"            -> socket "wayland-0", output selector "1"
/// - "/path/to/socket" -> absolute socket path, compositor-chosen output
inline wayland_display_spec parse_wayland_display(std::string_view spec) {
  wayland_display_spec result;
  if (spec.empty()) { return result; }

  if (spec[0] == '/') {
    result.socket_name = std::string(spec);
    return result;
  }

  constexpr std::string_view kWaylandPrefix = "wayland-";
  if (spec.size() > kWaylandPrefix.size() &&
      spec.substr(0, kWaylandPrefix.size()) == kWaylandPrefix) {
    std::string_view rest = spec.substr(kWaylandPrefix.size());
    size_t dot = rest.find('.');
    if (dot == std::string_view::npos) {
      if (detail::is_ascii_digits(rest)) {
        result.socket_name = std::string(spec);
        return result;
      }
    } else if (detail::is_ascii_digits(rest.substr(0, dot))) {
      result.socket_name =
          std::string(spec.substr(0, kWaylandPrefix.size() + dot));
      result.output_selector = std::string(rest.substr(dot + 1));
      return result;
    }
  }

  if (spec.size() > 1 && spec[0] == ':') {
    std::string_view rest = spec.substr(1);
    size_t dot = rest.find('.');
    if (dot == std::string_view::npos) {
      if (detail::is_ascii_digits(rest)) {
        result.socket_name = "wayland-" + std::string(rest);
        return result;
      }
    } else if (detail::is_ascii_digits(rest.substr(0, dot))) {
      result.socket_name = "wayland-" + std::string(rest.substr(0, dot));
      result.output_selector = std::string(rest.substr(dot + 1));
      return result;
    }
  }

  result.output_selector = std::string(spec);
  return result;
}

/// @brief Selects an output index matching @p selector.
///
/// Matching precedence:
/// 1. Exact match on `output.name` (e.g. "DP-2").
/// 2. Exact match on `output.description` (e.g. monitor make/model).
/// 3. 0-based ordinal index into @p outputs.
///
/// @p Container elements may be values or pointer-like (e.g. unique_ptr)
/// exposing `.name` and `.description` strings.
template <typename Container>
std::optional<size_t> select_wayland_output(const Container &outputs,
                                            std::string_view selector) {
  if (selector.empty()) { return std::nullopt; }

  auto get_ptr = [](const auto &entry) {
    using T = std::decay_t<decltype(entry)>;
    if constexpr (std::is_pointer_v<T>) {
      return entry;
    } else if constexpr (requires { entry.operator->(); }) {
      return entry ? entry.operator->() : nullptr;
    } else {
      return &entry;
    }
  };

  for (size_t i = 0; i < outputs.size(); ++i) {
    const auto *item = get_ptr(outputs[i]);
    if (item != nullptr && item->name == selector) { return i; }
  }
  for (size_t i = 0; i < outputs.size(); ++i) {
    const auto *item = get_ptr(outputs[i]);
    if (item != nullptr && item->description == selector) { return i; }
  }
  if (auto idx = detail::parse_nonneg_index(selector);
      idx.has_value() && *idx < outputs.size() &&
      get_ptr(outputs[*idx]) != nullptr) {
    return *idx;
  }
  return std::nullopt;
}

}  // namespace conky

#endif /* CONKY_WAYLAND_DISPLAY_HH */
