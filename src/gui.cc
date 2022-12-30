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
#include "gui.h"
#include "common.h"
#include "config.h"
#include "conky.h"
#include "logging.h"
#include "wl.h"

#ifdef BUILD_IMLIB2
#include "imlib2.h"
#endif /* BUILD_IMLIB2 */
#ifndef OWN_WINDOW
#include <iostream>
#endif

#ifdef BUILD_ARGB
bool have_argb_visual;
#endif /* BUILD_ARGB */

/* basic display attributes */
int display_width;
int display_height;
int screen;

/* workarea where window / text is aligned (from _NET_WORKAREA on X11) */
int workarea[4];

/* Window stuff */
char window_created = 0;

/* local prototypes */
#ifdef BUILD_X11
void x11_init_window(lua::state &l, bool own);
#endif /*BUILD_X11*/

/********************* <SETTINGS> ************************/

priv::colour_setting background_colour("own_window_colour", 0);

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

void colour_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);
  Base::lua_setter(l, init);
  ++s;
}
}  // namespace priv

template <>
conky::lua_traits<alignment>::Map conky::lua_traits<alignment>::map = {
    {"top_left", TOP_LEFT},
    {"top_right", TOP_RIGHT},
    {"top_middle", TOP_MIDDLE},
    {"bottom_left", BOTTOM_LEFT},
    {"bottom_right", BOTTOM_RIGHT},
    {"bottom_middle", BOTTOM_MIDDLE},
    {"middle_left", MIDDLE_LEFT},
    {"middle_middle", MIDDLE_MIDDLE},
    {"middle_right", MIDDLE_RIGHT},
    {"tl", TOP_LEFT},
    {"tr", TOP_RIGHT},
    {"tm", TOP_MIDDLE},
    {"bl", BOTTOM_LEFT},
    {"br", BOTTOM_RIGHT},
    {"bm", BOTTOM_MIDDLE},
    {"ml", MIDDLE_LEFT},
    {"mm", MIDDLE_MIDDLE},
    {"mr", MIDDLE_RIGHT},
    {"none", NONE}};

#ifdef OWN_WINDOW
template <>
conky::lua_traits<window_type>::Map conky::lua_traits<window_type>::map = {
    {"normal", TYPE_NORMAL},
    {"dock", TYPE_DOCK},
    {"panel", TYPE_PANEL},
    {"desktop", TYPE_DESKTOP},
    {"override", TYPE_OVERRIDE}};

template <>
conky::lua_traits<window_hints>::Map conky::lua_traits<window_hints>::map = {
    {"undecorated", HINT_UNDECORATED},
    {"below", HINT_BELOW},
    {"above", HINT_ABOVE},
    {"sticky", HINT_STICKY},
    {"skip_taskbar", HINT_SKIP_TASKBAR},
    {"skip_pager", HINT_SKIP_PAGER}};

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

conky::simple_config_setting<alignment> text_alignment("alignment", BOTTOM_LEFT,
                                                       false);

priv::colour_setting color[10] = {{"color0", 0xffffff}, {"color1", 0xffffff},
                                  {"color2", 0xffffff}, {"color3", 0xffffff},
                                  {"color4", 0xffffff}, {"color5", 0xffffff},
                                  {"color6", 0xffffff}, {"color7", 0xffffff},
                                  {"color8", 0xffffff}, {"color9", 0xffffff}};
priv::colour_setting default_color("default_color", 0xffffff);
priv::colour_setting default_shade_color("default_shade_color", 0x000000);
priv::colour_setting default_outline_color("default_outline_color", 0x000000);

conky::range_config_setting<int> border_inner_margin(
    "border_inner_margin", 0, std::numeric_limits<int>::max(), 3, true);
conky::range_config_setting<int> border_outer_margin(
    "border_outer_margin", 0, std::numeric_limits<int>::max(), 1, true);
conky::range_config_setting<int> border_width("border_width", 0,
                                              std::numeric_limits<int>::max(),
                                              1, true);

#ifdef OWN_WINDOW
conky::simple_config_setting<bool> set_transparent("own_window_transparent",
                                                   false, false);
conky::simple_config_setting<std::string> own_window_class("own_window_class",
                                                           PACKAGE_NAME, false);

conky::simple_config_setting<std::string> own_window_title(
    "own_window_title", PACKAGE_NAME " (" + gethostnamecxx() + ")", false);

conky::simple_config_setting<window_type> own_window_type("own_window_type",
                                                          TYPE_NORMAL, false);
conky::simple_config_setting<uint16_t, window_hints_traits> own_window_hints(
    "own_window_hints", 0, false);

#ifdef BUILD_ARGB
conky::range_config_setting<int> own_window_argb_value("own_window_argb_value",
                                                       0, 255, 255, false);
#endif /* BUILD_ARGB */
#endif /* OWN_WINDOW */
priv::own_window_setting own_window;

/******************** </SETTINGS> ************************/
