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

#ifndef CONKY_WL_SHELL_H
#define CONKY_WL_SHELL_H

#include "config.h"

#ifndef BUILD_WAYLAND
#error wl-shell.h included when BUILD_WAYLAND is disabled
#endif

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>

#include "gui.h"

struct wl_array;
struct wl_output;
struct wl_surface;
struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;
struct zwlr_layer_surface_v1_listener;
struct xdg_surface;
struct xdg_surface_listener;
struct xdg_toplevel;
struct xdg_toplevel_listener;
struct xdg_wm_base;

namespace conky {

/// @brief Edge(s) a surface anchors to / reserves against.
///
/// Aliases the cardinal `alignment` constants, so a `screen_edge` and an
/// `alignment` share an encoding and are (almost) freely castable.
enum class screen_edge : uint8_t {
  NONE = *alignment::NONE,
  TOP = *alignment::TOP_MIDDLE,
  LEFT = *alignment::MIDDLE_LEFT,
  RIGHT = *alignment::MIDDLE_RIGHT,
  BOTTOM = *alignment::BOTTOM_MIDDLE,
};

/// @brief Abstracts a Wayland shell role bound to a `wl_surface`.
///
/// Concrete shells (wlr-layer-shell, xdg-shell, and Iron Man suit AR shell)
/// translate conky's desired geometry into role-specific protocol requests.
class shell_surface {
 public:
  virtual ~shell_surface() = default;

  /// @brief Push the desired logical surface size to the role.
  virtual void set_size(uint32_t width, uint32_t height) = 0;

  /// @brief Reserve screen space along an edge (struts).
  ///
  /// Only honoured by roles that support it (see @ref supports_struts); the
  /// default is a no-op so e.g. xdg toplevels can ignore it.
  ///
  /// @param edge edge(s) to anchor the surface to
  /// @param exclusive_zone perpendicular size to reserve, including the margin
  /// @param margin_x horizontal distance from the anchored edge
  /// @param margin_y vertical distance from the anchored edge
  virtual void reserve_space(screen_edge edge, int exclusive_zone, int margin_x,
                             int margin_y) {
    (void)edge;
    (void)exclusive_zone;
    (void)margin_x;
    (void)margin_y;
  }

  /// @brief Whether the role can reserve screen space (struts/anchoring).
  virtual bool supports_struts() const { return false; }
};

/// @brief Called when the compositor closes the shell surface (toplevel close
/// button, layer surface closed).
using close_shell_handler = void (*)();

struct shell_params {
  close_shell_handler on_close = nullptr;
};

/// @brief wlr-layer-shell role: a surface mounted on a compositor layer, able
/// to anchor to edges and reserve screen space.
class layer_shell_surface final : public shell_surface {
 public:
  struct params : public shell_params {
    wl_surface *surface = nullptr;
    zwlr_layer_shell_v1 *layer_shell = nullptr;
    wl_output *output = nullptr;
    uint32_t layer = 0;  ///< ZWLR_LAYER_SHELL_V1_LAYER_* value
    const char *m_namespace = "conky";
  };

  explicit layer_shell_surface(const params &p);
  ~layer_shell_surface() override;

  void set_size(uint32_t width, uint32_t height) override;
  void reserve_space(screen_edge edge, int exclusive_zone, int margin_x,
                     int margin_y) override;
  bool supports_struts() const override { return true; }

 private:
  static void handle_configure(void *data, zwlr_layer_surface_v1 *surface,
                               uint32_t serial, uint32_t width,
                               uint32_t height);
  static void handle_closed(void *data, zwlr_layer_surface_v1 *surface);
  static const zwlr_layer_surface_v1_listener listener;

  zwlr_layer_surface_v1 *m_layer_surface = nullptr;
  close_shell_handler m_on_close = nullptr;
};

/// @brief xdg-shell role: a normal, compositor-managed toplevel window.
///
/// conky has a fixed content size, so the toplevel is pinned to that size.
class xdg_shell_surface final : public shell_surface {
 public:
  struct params : public shell_params {
    wl_surface *surface = nullptr;
    xdg_wm_base *xdg_shell = nullptr;
    std::string title;
    std::string app_id;
  };

  explicit xdg_shell_surface(const params &p);
  ~xdg_shell_surface() override;

  void set_size(uint32_t width, uint32_t height) override;

 private:
  static void handle_surface_configure(void *data, xdg_surface *surface,
                                       uint32_t serial);
  static void handle_toplevel_configure(void *data, xdg_toplevel *toplevel,
                                        int32_t width, int32_t height,
                                        wl_array *states);
  static void handle_toplevel_close(void *data, xdg_toplevel *toplevel);
  static const xdg_surface_listener surface_listener;
  static const xdg_toplevel_listener toplevel_listener;

  xdg_surface *m_xdg_surface = nullptr;
  xdg_toplevel *m_xdg_toplevel = nullptr;
  close_shell_handler m_on_close = nullptr;
};

/// @brief Constructs a shell role of type @p Shell from its `params`.
template <typename Shell>
std::unique_ptr<shell_surface> create_shell_surface(
    const typename Shell::params &params) {
  static_assert(std::is_base_of_v<shell_surface, Shell>,
                "Shell must derive from conky::shell_surface");
  return std::make_unique<Shell>(params);
}

}  // namespace conky

#endif /* CONKY_WL_SHELL_H */
