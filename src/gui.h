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
#pragma once

#include "config.h"

#include "colours.h"
#include "setting.hh"

#include "colour-settings.h"

#ifdef BUILD_X11
#include <X11/Xlib.h>
#include "x11-settings.h"
#endif /* BUILD_X11 */

/// @brief Represents alignment on a single axis.
enum class axis_align : uint8_t {
  /// No alignment
  NONE = 0,
  /// Top or left alignment
  START = 0b01,
  /// Middle alignment
  MIDDLE = 0b10,
  /// Bottom or right alignment
  END = 0b11,
};
constexpr uint8_t operator*(axis_align index) {
  return static_cast<uint8_t>(index);
}

/// @brief Represents alignment on a 2D plane.
///
/// Values are composed of 2 `axis_align` values: 2 bits (at 0x0C) for vertical
/// aligment and 2 least significant bits for horizontal.
enum class alignment : uint8_t {
  NONE = 0,
  NONE_LEFT = 0b0001,
  NONE_MIDDLE = 0b0010,
  NONE_RIGHT = 0b0011,
  TOP_LEFT = 0b0101,
  TOP_MIDDLE = 0b0110,
  TOP_RIGHT = 0b0111,
  MIDDLE_LEFT = 0b1001,
  MIDDLE_MIDDLE = 0b1010,
  MIDDLE_RIGHT = 0b1011,
  BOTTOM_LEFT = 0b1101,
  BOTTOM_MIDDLE = 0b1110,
  BOTTOM_RIGHT = 0b1111,
};
constexpr uint8_t operator*(alignment index) {
  return static_cast<uint8_t>(index);
}

/// @brief Returns the horizontal axis alignment component of `alignment`.
/// @param of 2D alignment to extract axis alignment from
/// @return horizontal `axis_align`
[[nodiscard]] inline axis_align horizontal_alignment(alignment of) {
  return static_cast<axis_align>(static_cast<uint8_t>(of) & 0b11);
}
/// @brief Returns the vertical axis alignment component of `alignment`.
/// @param of 2D alignment to extract axis alignment from
/// @return vertical `axis_align`
[[nodiscard]] inline axis_align vertical_alignment(alignment of) {
  return static_cast<axis_align>((static_cast<uint8_t>(of) >> 2) & 0b11);
}

/// @brief Describes how and where a window should be mounted, as well as its
/// behavior.
///
/// We assume the following order of layers:
/// - Background - behind conky and any other windows, contains icons and
///   desktop menus
/// - Background widgets and docks
/// - Windows
/// - Panels - contains content that covers windows
/// - Override windows - input-override windows on X11, custom overlays, lock
///   screens, etc.
///
/// See also:
/// - [X11 wm-spec `_NET_WM_WINDOW_TYPE` property](
///   https://specifications.freedesktop.org/wm-spec/1.3/ar01s05.html#idm45684324619328)
/// - [wlr-layer-shell layers](
///   https://wayland.app/protocols/wlr-layer-shell-unstable-v1#zwlr_layer_shell_v1:enum:layer)
/// - [xdg-positioner::anchor](
///   https://wayland.app/protocols/xdg-shell#xdg_positioner:enum:anchor)
enum class window_type : uint8_t {
  /// @brief Acts as a normal window - has decorations, above
  /// background, widgets and docks, below panels.
  NORMAL = 0,
  /// @brief Screen background, no decorations, positioned at the very bottom
  /// and behind widgets and docks.
  DESKTOP,
  /// @brief Normal window, always shown above parent window (group).
  ///
  /// See: [Popup](https://wayland.app/protocols/xdg-shell#xdg_popup) XDG shell
  /// surface.
  UTILITY,
  /// @brief No decorations, between windows and background, attached to screen
  /// edge.
  DOCK,
  /// @brief No decorations, above windows, attached to screen edge, reserves
  /// space.
  PANEL,
#ifdef BUILD_X11
  /// @brief On top of everything else, not controlled by WM.
  OVERRIDE,
#endif /* BUILD_X11 */
};
constexpr uint8_t operator*(window_type index) {
  return static_cast<uint8_t>(index);
}

#if defined(BUILD_X11) && defined(OWN_WINDOW)
// Only works in X11 because Wayland doesn't support

/// @brief Hints are used to tell WM how it should treat a window.
///
/// See: [X11 wm-spec `_NET_WM_STATE` property](
/// https://specifications.freedesktop.org/wm-spec/1.3/ar01s05.html#idm45684324611552)
enum class window_hints : uint16_t {
  UNDECORATED = 0,
  BELOW,
  ABOVE,
  STICKY,
  SKIP_TASKBAR,
  SKIP_PAGER
};
constexpr uint8_t operator*(window_hints index) {
  return static_cast<uint8_t>(index);
}

inline void SET_HINT(window_hints &mask, window_hints hint) {
  mask = static_cast<window_hints>(*mask | (1 << (*hint)));
}
inline void SET_HINT(uint16_t &mask, window_hints hint) {
  mask = mask | (1 << (*hint));
}
inline bool TEST_HINT(window_hints mask, window_hints hint) {
  return (*mask & (1 << (*hint))) != 0;
}
inline bool TEST_HINT(uint16_t mask, window_hints hint) {
  return (mask & (1 << (*hint))) != 0;
}
#endif

#ifdef BUILD_X11
extern Display *display;
#endif /* BUILD_X11 */
extern int display_width;
extern int display_height;
extern int screen;
extern int workarea[4];

extern char window_created;

void destroy_window(void);
void create_gc(void);
void set_struts(int);

bool out_to_gui(lua::state &l);

void print_monitor(struct text_object *, char *, unsigned int);
void print_monitor_number(struct text_object *, char *, unsigned int);
void print_desktop(struct text_object *, char *, unsigned int);
void print_desktop_number(struct text_object *, char *, unsigned int);
void print_desktop_name(struct text_object *, char *, unsigned int);

/* Num lock, Scroll lock, Caps Lock */
void print_key_num_lock(struct text_object *, char *, unsigned int);
void print_key_caps_lock(struct text_object *, char *, unsigned int);
void print_key_scroll_lock(struct text_object *, char *, unsigned int);

/* Keyboard layout and mouse speed in percentage */
void print_keyboard_layout(struct text_object *, char *, unsigned int);
void print_mouse_speed(struct text_object *, char *, unsigned int);

#ifdef BUILD_XDBE
void xdbe_swap_buffers(void);
#else
void xpmdb_swap_buffers(void);
#endif /* BUILD_XDBE */

extern conky::simple_config_setting<alignment> text_alignment;

namespace priv {
class own_window_setting : public conky::simple_config_setting<bool> {
  typedef conky::simple_config_setting<bool> Base;

 protected:
  virtual void lua_setter(lua::state &l, bool init);

 public:
  own_window_setting() : Base("own_window", false, false) {}
};
}  // namespace priv

extern conky::simple_config_setting<int> head_index;
extern priv::colour_setting default_shade_color;
extern priv::colour_setting default_outline_color;

extern conky::range_config_setting<int> border_inner_margin;
extern conky::range_config_setting<int> border_outer_margin;
extern conky::range_config_setting<int> border_width;

extern conky::simple_config_setting<bool> forced_redraw;

#ifdef OWN_WINDOW
extern priv::own_window_setting own_window;
extern conky::simple_config_setting<std::string> own_window_title;
#endif /* OWN_WINDOW */

#if defined(OWN_WINDOW) && defined(BUILD_X11)
struct window_hints_traits {
  static const lua::Type type = lua::TSTRING;
  typedef uint16_t Type;
  static std::pair<Type, bool> convert(lua::state &l, int index,
                                       const std::string &name);
};

extern conky::simple_config_setting<std::string> own_window_class;
extern conky::simple_config_setting<window_type> own_window_type;
extern conky::simple_config_setting<uint16_t, window_hints_traits>
    own_window_hints;
#endif /* OWN_WINDOW && BUILD_X11 */

#if defined(OWN_WINDOW) || defined(BUILD_WAYLAND)
extern priv::colour_setting background_colour;
extern conky::simple_config_setting<bool> set_transparent;
#endif /* OWN_WINDOW || BUILD_WAYLAND */

#if defined(BUILD_ARGB) || defined(BUILD_WAYLAND)
extern conky::simple_config_setting<bool> use_argb_visual;
extern conky::range_config_setting<int> own_window_argb_value;
#endif /* BUILD_ARGB || BUILD_WAYLAND */
