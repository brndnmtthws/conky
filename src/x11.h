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

#include <X11/Xatom.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#include <X11/Xlib.h>
#pragma GCC diagnostic pop
#ifdef BUILD_XFT
#include <X11/Xft/Xft.h>
#endif

#ifdef BUILD_XDBE
#include <X11/extensions/Xdbe.h>
#endif

#include "setting.hh"

#ifdef BUILD_ARGB
/* true if use_argb_visual=true and argb visual was found*/
extern bool have_argb_visual;
#endif /* BUILD_ARGB */

#define ATOM(a) XInternAtom(display, #a, False)

#ifdef OWN_WINDOW
enum window_type {
  TYPE_NORMAL = 0,
  TYPE_DOCK,
  TYPE_PANEL,
  TYPE_DESKTOP,
  TYPE_OVERRIDE,
  TYPE_UTILITY
};

enum window_hints {
  HINT_UNDECORATED = 0,
  HINT_BELOW,
  HINT_ABOVE,
  HINT_STICKY,
  HINT_SKIP_TASKBAR,
  HINT_SKIP_PAGER
};

#define SET_HINT(mask, hint) (mask |= (1 << (hint)))
#define TEST_HINT(mask, hint) (mask & (1 << (hint)))
#endif

extern Display *display;

struct conky_x11_window {
  Window root, window, desktop;
  Drawable drawable;
  Visual *visual;
  Colormap colourmap;
  GC gc;

#ifdef BUILD_XDBE
  XdbeBackBuffer back_buffer;
#else  /*BUILD_XDBE*/
  Pixmap back_buffer;
#endif /*BUILD_XDBE*/
#ifdef BUILD_XFT
  XftDraw *xftdraw;
#endif /*BUILD_XFT*/

  int width;
  int height;
#ifdef OWN_WINDOW
  int x;
  int y;
#endif
};

extern struct conky_x11_window window;
extern conky::simple_config_setting<std::string> display_name;

void destroy_window(void);
void create_gc(void);
void set_transparent_background(Window win);
void get_x11_desktop_info(Display *current_display, Atom atom);
void set_struts(int);
void x11_init_window(lua::state &l, bool own);
void deinit_x11();

#ifdef BUILD_XDBE
void xdbe_swap_buffers(void);
#else
void xpmdb_swap_buffers(void);
#endif /* BUILD_XDBE */

namespace priv {
class out_to_x_setting : public conky::simple_config_setting<bool> {
  typedef conky::simple_config_setting<bool> Base;

 protected:
  virtual void lua_setter(lua::state &l, bool init);
  virtual void cleanup(lua::state &l);

 public:
  out_to_x_setting() : Base("out_to_x", true, false) {}
};

#ifdef BUILD_XDBE
class use_xdbe_setting : public conky::simple_config_setting<bool> {
  typedef conky::simple_config_setting<bool> Base;

  bool set_up(lua::state &l);

 protected:
  virtual void lua_setter(lua::state &l, bool init);

 public:
  use_xdbe_setting() : Base("double_buffer", false, false) {}
};

#else
class use_xpmdb_setting : public conky::simple_config_setting<bool> {
  typedef conky::simple_config_setting<bool> Base;

  bool set_up(lua::state &l);

 protected:
  virtual void lua_setter(lua::state &l, bool init);

 public:
  use_xpmdb_setting() : Base("double_buffer", false, false) {}
};
#endif
} /* namespace priv */

extern priv::out_to_x_setting out_to_x;

#ifdef BUILD_XFT
extern conky::simple_config_setting<bool> use_xft;
#endif

#ifdef BUILD_XDBE
extern priv::use_xdbe_setting use_xdbe;
#else
extern priv::use_xpmdb_setting use_xpmdb;
#endif
