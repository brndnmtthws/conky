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
#include "gui.h"
#include "../common.h"
#include "../conky.h"
#include "../logging.h"

#ifdef BUILD_X11
#include "../lua/x11-settings.h"
#endif /* BUILD_X11 */

#ifdef BUILD_WAYLAND
#include "wl.h"
#endif /* BUILD_WAYLAND */

// #ifdef BUILD_IMLIB2
// #include "../conky-imlib2.h"
// #endif /* BUILD_IMLIB2 */
#ifndef OWN_WINDOW
#include <iostream>
#endif

/* workarea where window / text is aligned (from _NET_WORKAREA on X11) */
conky::absolute_rect<int> workarea;

/* Window stuff */
char window_created = 0;

/* local prototypes */
#ifdef BUILD_X11
void x11_init_window(lua::state &l, bool own);
#endif /*BUILD_X11*/

/********************* <SETTINGS> ************************/

bool out_to_gui(lua::state &l) {
  bool to_gui = false;
#ifdef BUILD_X11
  to_gui |= out_to_x.get(l);
#endif /* BUILD_X11 */
#ifdef BUILD_WAYLAND
  to_gui |= out_to_wayland.get(l);
#endif /* BUILD_WAYLAND */
  return to_gui;
}

namespace priv {
void own_window_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init) {
    if (do_convert(l, -1).first) {
#ifndef OWN_WINDOW
      std::cerr << "Support for the own_window setting has been "
                   "disabled during compilation\n";
      l.pop();
      l.pushboolean(false);
#endif
    }

    if (out_to_gui(l)) {
#ifdef BUILD_X11
      x11_init_window(l, do_convert(l, -1).first);
#endif /*BUILD_X11*/
    } else {
      // own_window makes no sense when not drawing to X
      l.pop();
      l.pushboolean(false);
    }
  }

  ++s;
}
}  // namespace priv

template <>
conky::lua_traits<alignment>::Map conky::lua_traits<alignment>::map = {
    {"top_left", alignment::TOP_LEFT},
    {"top_right", alignment::TOP_RIGHT},
    {"top_middle", alignment::TOP_MIDDLE},
    {"top", alignment::TOP_MIDDLE},
    {"bottom_left", alignment::BOTTOM_LEFT},
    {"bottom_right", alignment::BOTTOM_RIGHT},
    {"bottom_middle", alignment::BOTTOM_MIDDLE},
    {"bottom", alignment::BOTTOM_MIDDLE},
    {"middle_left", alignment::MIDDLE_LEFT},
    {"left", alignment::MIDDLE_LEFT},
    {"middle_middle", alignment::MIDDLE_MIDDLE},
    {"center", alignment::MIDDLE_MIDDLE},
    {"middle_right", alignment::MIDDLE_RIGHT},
    {"right", alignment::MIDDLE_RIGHT},
    {"tl", alignment::TOP_LEFT},
    {"tr", alignment::TOP_RIGHT},
    {"tm", alignment::TOP_MIDDLE},
    {"bl", alignment::BOTTOM_LEFT},
    {"br", alignment::BOTTOM_RIGHT},
    {"bm", alignment::BOTTOM_MIDDLE},
    {"ml", alignment::MIDDLE_LEFT},
    {"mm", alignment::MIDDLE_MIDDLE},
    {"mr", alignment::MIDDLE_RIGHT},
    {"none", alignment::NONE}};

#ifdef OWN_WINDOW
template <>
conky::lua_traits<window_type>::Map conky::lua_traits<window_type>::map = {
    {"normal", window_type::NORMAL},   {"dock", window_type::DOCK},
    {"panel", window_type::PANEL},     {"desktop", window_type::DESKTOP},
    {"utility", window_type::UTILITY}, {"override", window_type::OVERRIDE}};

template <>
conky::lua_traits<window_hints>::Map conky::lua_traits<window_hints>::map = {
    {"undecorated", window_hints::UNDECORATED},
    {"below", window_hints::BELOW},
    {"above", window_hints::ABOVE},
    {"sticky", window_hints::STICKY},
    {"skip_taskbar", window_hints::SKIP_TASKBAR},
    {"skip_pager", window_hints::SKIP_PAGER}};

std::pair<uint16_t, bool> window_hints_traits::convert(
    lua::state &l, int index, const std::string &name) {
  lua::stack_sentry s(l);
  l.checkstack(1);

  std::string hints = l.tostring(index);
  // add a sentinel to simplify the following loop
  hints += ',';
  size_t pos = 0;
  size_t newpos;
  uint16_t ret = 0;
  while ((newpos = hints.find_first_of(", ", pos)) != std::string::npos) {
    if (newpos > pos) {
      l.pushstring(hints.substr(pos, newpos - pos));
      auto t = conky::lua_traits<window_hints>::convert(l, -1, name);
      if (!t.second) { return {0, false}; }
      SET_HINT(ret, t.first);
      l.pop();
    }
    pos = newpos + 1;
  }
  return {ret, true};
}
#endif

#ifdef OWN_WINDOW
namespace {
// used to set the default value for own_window_title
std::string gethostnamecxx() {
  update_uname();
  return info.uname_s.nodename;
}
}  // namespace
#endif /* OWN_WINDOW */

/*
 * The order of these settings cannot be completely arbitrary. Some of them
 * depend on others, and the setters are called in the order in which they are
 * defined. The order should be: x11_display_name -> out_to_x -> everything
 * colour related -> border_*, own_window_*, etc -> own_window -> double_buffer
 * ->  imlib_cache_size.
 *
 * The settings order can be modified with the settings_ordering vector in
 * setting.cc.
 */

conky::simple_config_setting<alignment> text_alignment("alignment",
                                                       alignment::BOTTOM_LEFT,
                                                       false);

priv::colour_setting default_shade_color("default_shade_color", black_argb32);
priv::colour_setting default_outline_color("default_outline_color",
                                           black_argb32);

conky::range_config_setting<int> border_inner_margin(
    "border_inner_margin", 0, std::numeric_limits<int>::max(), 3, true);
conky::range_config_setting<int> border_outer_margin(
    "border_outer_margin", 0, std::numeric_limits<int>::max(), 1, true);
conky::range_config_setting<int> border_width("border_width", 0,
                                              std::numeric_limits<int>::max(),
                                              1, true);

#ifdef OWN_WINDOW
conky::simple_config_setting<std::string> own_window_title(
    "own_window_title", PACKAGE_NAME " (" + gethostnamecxx() + ")", false);
conky::simple_config_setting<window_type> own_window_type("own_window_type",
                                                          window_type::NORMAL,
                                                          false);
conky::simple_config_setting<std::string> own_window_class("own_window_class",
                                                           PACKAGE_NAME, false);
#endif /* OWN_WINDOW */

#if defined(OWN_WINDOW) && defined(BUILD_X11)
conky::simple_config_setting<uint16_t, window_hints_traits> own_window_hints(
    "own_window_hints", 0, false);
#endif /* OWN_WINDOW && BUILD_X11 */

#if defined(OWN_WINDOW) || defined(BUILD_WAYLAND)
priv::colour_setting background_colour("own_window_colour", 0);
conky::simple_config_setting<bool> set_transparent("own_window_transparent",
                                                   false, false);
#endif /* OWN_WINDOW || BUILD_WAYLAND */

#if defined(BUILD_ARGB) || defined(BUILD_WAYLAND)
conky::simple_config_setting<bool> use_argb_visual("own_window_argb_visual",
                                                   false, false);
conky::range_config_setting<int> own_window_argb_value("own_window_argb_value",
                                                       0, 255, 255, false);
#endif /* BUILD_ARGB || BUILD_WAYLAND */
priv::own_window_setting own_window;

/******************** </SETTINGS> ************************/
