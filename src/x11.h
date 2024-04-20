/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
 *  (see AUTHORS)
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

#ifndef CONKY_X11_H
#define CONKY_X11_H

#include "config.h"

#ifndef BUILD_X11
#error x11.h included when BUILD_X11 is disabled
#endif

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

#include <cstdint>
#include <functional>
#include <vector>

// TODO: remove lua requirement from x11_init_window
#include "llua.h"

#include "gui.h"

/* true if use_argb_visual=true and argb visual was found*/
extern bool have_argb_visual;

#define ATOM(a) XInternAtom(display, #a, False)

extern Display *display;

struct conky_x11_window {
  /// XID of x11 root window
  Window root;
  /// XID of Conky window
  Window window;
  /// XID of DE desktop window (or root if none)
  Window desktop;
  Drawable drawable;
  Visual *visual;
  Colormap colourmap;
  GC gc;

  // Mask containing all events captured by conky
  int64_t event_mask;

#ifdef BUILD_XDBE
  XdbeBackBuffer back_buffer;
#else  /*BUILD_XDBE*/
  Pixmap back_buffer;
#endif /*BUILD_XDBE*/
#ifdef BUILD_XFT
  XftDraw *xftdraw;
#endif /*BUILD_XFT*/
#if defined(BUILD_MOUSE_EVENTS) || defined(BUILD_XINPUT)
  // Don't feature gate with BUILD_XINPUT; controls fallback.
  std::int32_t xi_opcode;
#endif /* BUILD_MOUSE_EVENTS || BUILD_XINPUT */

  int width;
  int height;
#ifdef OWN_WINDOW
  int x;
  int y;
#endif
};

extern struct conky_x11_window window;

void init_x11();
void destroy_window(void);
void create_gc(void);
void set_transparent_background(Window win);
void get_x11_desktop_info(Display *current_display, Atom atom);
void set_struts(int);
void x11_init_window(lua::state &l, bool own);
void deinit_x11();

/// @brief Forwards argument event to the top-most window at event positon that
/// isn't conky.
///
/// Calling this function is time sensitive as it will query window at event
/// position **at invocation time**.
/// @param event event to forward
/// @param cookie optional cookie data
void propagate_x11_event(XEvent &event, const void *cookie = nullptr);

/// @brief Returns a list of window values for the given atom.
/// @param display display with which the atom is associated
/// @param window window to query for the atom value
/// @param atom atom to query for
/// @return a list of window values for the given atom
std::vector<Window> x11_atom_window_list(Display *display, Window window,
                                         Atom atom);

/// @brief Tries getting a list of windows ordered from bottom to top.
///
/// Whether the list is correctly ordered depends on WM/DE providing the
/// `_NET_CLIENT_LIST_STACKING` atom. If only `_NET_CLIENT_LIST` is defined,
/// this function assumes the WM/DE is a tiling one without stacking order.
///
/// If neither of the atoms are provided, this function tries traversing the
/// window graph in order to collect windows. In this case, map state of windows
/// is ignored. This also produces a lot of noise for some WM/DEs due to
/// inserted window decorations.
///
/// @param display which display to query for windows @return a (likely) ordered
/// list of windows
std::vector<Window> query_x11_windows(Display *display);

/// @brief Finds the last ascendant of a window (trunk) before root.
///
/// If provided `child` is root or has no windows between root and itself, the
/// `child` is returned.
///
/// @param display display of parent
/// @param child window whose parents to query
/// @return the top level ascendant window
Window query_x11_top_parent(Display *display, Window child);

/// @brief Returns the top-most window overlapping provided screen coordinates.
///
/// @param display display of parent
/// @param x screen X position contained by window
/// @param y screen Y position contained by window
/// @return a top-most window at provided screen coordinates, or root
Window query_x11_window_at_pos(Display *display, int x, int y);

/// @brief Returns a list of windows overlapping provided screen coordinates.
///
/// Vector returned by this function will never contain root because it's
/// assumed to always cover the entire display.
///
/// @param display display of parent
/// @param x screen X position contained by window
/// @param y screen Y position contained by window
/// @param predicate any additional predicates to apply for XWindowAttributes
/// (besides bounds testing).
/// @return a vector of windows at provided screen coordinates
std::vector<Window> query_x11_windows_at_pos(
    Display *display, int x, int y,
    std::function<bool(XWindowAttributes &)> predicate =
        [](XWindowAttributes &a) { return true; });

#ifdef BUILD_XDBE
void xdbe_swap_buffers(void);
#else
void xpmdb_swap_buffers(void);
#endif /* BUILD_XDBE */

#endif /* CONKY_X11_H */
