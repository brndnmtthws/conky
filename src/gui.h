/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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
#pragma once

#include "config.h"

#ifdef BUILD_X11
#include "x11.h"
#endif /* BUILD_X11 */

#include "colour-settings.h"
#include "colours.h"
#include "setting.hh"

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

/* alignments */
enum alignment {
  TOP_LEFT,
  TOP_RIGHT,
  TOP_MIDDLE,
  BOTTOM_LEFT,
  BOTTOM_RIGHT,
  BOTTOM_MIDDLE,
  MIDDLE_LEFT,
  MIDDLE_MIDDLE,
  MIDDLE_RIGHT,
  NONE
};

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
