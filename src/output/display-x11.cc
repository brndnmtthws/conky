/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018-2021 François Revol et al.
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
#include "display-output.hh"
#include "x11-event.h"

#include "display-x11.hh"

#include <X11/X.h>
#include <functional>
#include <memory>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#include <X11/Xutil.h>
#ifdef BUILD_XFT
#include <X11/Xlib.h>
#endif /* BUILD_XFT */
#pragma GCC diagnostic pop
#ifdef BUILD_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif /* BUILD_XDAMAGE */
#include "../lua/fonts.h"
#ifdef BUILD_IMLIB2
#include "../conky-imlib2.h"
#endif /* BUILD_IMLIB2 */
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#undef COUNT
#include <X11/Xresource.h>

#ifdef BUILD_LUA_CAIRO_XLIB
#include <cairo-xlib.h>
#endif /* BUILD_LUA_CAIRO_XLIB */

#include <cstdint>
#include <iostream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../conky.h"
#include "../content/colours.hh"
#include "../geometry.h"
#include "../logging.h"
#include "../lua/llua.h"
#include "../mouse-events.h"
#include "gui.h"

#include "../lua/x11-settings.h"
#include "x11.h"

// TODO: cleanup externs (move to conky.h ?)
#ifdef OWN_WINDOW
extern int fixed_size, fixed_pos;
#endif                           /* OWN_WINDOW */
extern conky::vec2i text_start;  /* text start position in window */
extern conky::vec2i text_offset; /* offset for start position */
extern conky::vec2i
    text_size; /* initially 1 so no zero-sized window is created */
extern double current_update_time, next_update_time, last_update_time;
void update_text();
extern int need_to_update;
int get_border_total();
extern conky::range_config_setting<int> maximum_width;
extern Colour current_color;
static float screen_dpi = -1;

/* for x_fonts */
struct x_font_list {
  XFontStruct *font;
  XFontSet fontset;

#ifdef BUILD_XFT
  XftFont *xftfont;
  int font_alpha;
#endif

  x_font_list()
      : font(nullptr),
        fontset(nullptr)
#ifdef BUILD_XFT
        ,
        xftfont(nullptr),
        font_alpha(0xffff)
#endif
  {
  }
};

static std::vector<x_font_list> x_fonts; /* indexed by selected_font */

static void X11_create_window();

void update_dpi() {
  // Add XRandR support if used
  // See dunst PR: https://github.com/dunst-project/dunst/pull/608

#ifdef BUILD_XFT
  if (screen_dpi > 0) return;
  if (use_xft.get(*state)) {
    XrmDatabase db = XrmGetDatabase(display);
    if (db != nullptr) {
      char *xrmType;
      XrmValue xrmValue;
      if (XrmGetResource(db, "Xft.dpi", "Xft.dpi", &xrmType, &xrmValue)) {
        screen_dpi = strtof(xrmValue.addr, NULL);
      }
    } else {
      auto dpi = XGetDefault(display, "Xft", "dpi");
      if (dpi) { screen_dpi = strtof(dpi, nullptr); }
    }
  }
#endif /* BUILD_XFT */
  if (screen_dpi > 0) return;
  screen_dpi = static_cast<float>(DisplayWidth(display, screen)) * 25.4 /
               static_cast<float>(DisplayWidthMM(display, screen));
}

static void X11_create_window() {
  if (!window.window) { return; }
  setup_fonts();
  load_fonts(utf8_mode.get(*state));
  update_dpi();
  update_text_area(); /* to position text/window on screen */

#ifdef OWN_WINDOW
  if (own_window.get(*state)) {
    if (fixed_pos == 0) {
      XMoveWindow(display, window.window, window.geometry.x(),
                  window.geometry.y());
    }

    set_transparent_background(&window);
  }
#endif

  create_gc();

  draw_stuff();

  window.repaint_region = XCreateRegion();
#ifdef BUILD_XDAMAGE
  if (XDamageQueryExtension(display, &window.xdamage_event_base,
                            &window.xdamage_error_base) == 0) {
    LOG_WARNING("XDamage extension unavailable");
    window.window_damage = 0;
  } else {
    window.window_damage =
        XDamageCreate(display, window.window, XDamageReportNonEmpty);
    window.damage_region =
        XFixesCreateRegionFromWindow(display, window.window, 0);
    window.damage_scratch =
        XFixesCreateRegionFromWindow(display, window.window, 0);
  }
#endif /* BUILD_XDAMAGE */

  selected_font = 0;
  update_text_area(); /* to get initial size of the window */
}

namespace conky {
namespace {
conky::display_output_x11 x11_output;
}  // namespace

display_output_x11::display_output_x11()
    : display_output_base("x11", output_t::X11) {
  is_graphical = true;
}

bool display_output_x11::initialize() {
  // X global state is set up here rather than in lua config setters, so the
  // settings stay side-effect-free. Order mirrors the settings: open the
  // display, create the window, set up double buffering, then imlib.
  init_x11();
  x11_init_window(*state);

  if (use_double_buffer.get(*state)) {
    if (!x11_set_up_double_buffer(*state)) {
      // double buffering unavailable; reflect that in the value consumers read
      state->pushboolean(false);
      use_double_buffer.lua_set(*state);
    }
    LOG_INFO("drawing to {} buffer",
             use_double_buffer.get(*state) ? "double" : "single");
  }

#ifdef BUILD_IMLIB2
  cimlib_init();
#endif /* BUILD_IMLIB2 */

  X11_create_window();
  update_surface();
  return true;
}

bool display_output_x11::shutdown() {
#ifdef BUILD_IMLIB2
  cimlib_deinit();
#endif /* BUILD_IMLIB2 */
  deinit_x11();
  return true;
}

void process_surface_events(conky::display_output_x11 *surface,
                            Display *display);

bool display_output_x11::main_loop_wait(double t) {
  /* wait for X event or timeout */
  if (!display || !window.gc) return true;

  if (XPending(display) == 0) {
    fd_set fdsr;
    struct timeval tv{};
    int s;
    // t = next_update_time - get_time();

    t = std::min(std::max(t, 0.0), active_update_interval());

    tv.tv_sec = static_cast<long>(t);
    tv.tv_usec = static_cast<long>(t * 1000000) % 1000000;
    FD_ZERO(&fdsr);
    FD_SET(ConnectionNumber(display), &fdsr);

    s = select(ConnectionNumber(display) + 1, &fdsr, nullptr, nullptr, &tv);
    if (s == -1) {
      if (errno != EINTR) { LOG_ERROR("can't select(): {}", strerror(errno)); }
    } else {
      /* timeout */
      if (s == 0) { update_text(); }
    }
  }

  vec2i border_total = vec2i::uniform(get_border_total());
  if (need_to_update != 0) {
#ifdef OWN_WINDOW
    auto old_pos = window.geometry.pos();
#endif

    need_to_update = 0;
    selected_font = 0;
    update_text_area();

#ifdef OWN_WINDOW
    if (own_window.get(*state)) {
      int changed = 0;

      /* resize window if it isn't right size */
      vec2<long> border_size = border_total * 2;
      if ((fixed_size == 0) &&
          (text_size + border_size != window.geometry.size())) {
        window.geometry.set_size(text_size + border_size);
        draw_stuff(); /* redraw everything in our newly sized window */
        XResizeWindow(display, window.window, window.geometry.width(),
                      window.geometry.height()); /* resize window */
        set_transparent_background(&window);
#ifdef BUILD_XDBE
        /* swap buffers */
        swap_x11_buffers();
#else
        if (use_double_buffer.get(*state)) {
          XFreePixmap(display, window.back_buffer);
          unsigned int depth = window.color_depth != 0
                                   ? window.color_depth
                                   : DefaultDepth(display, screen);
          window.back_buffer =
              XCreatePixmap(display, window.window, window.geometry.width(),
                            window.geometry.height(), depth);

          if (window.back_buffer != None) {
            window.drawable = window.back_buffer;
          } else {
            // this is probably reallllly bad
            LOG_ERROR("failed to allocate back buffer for window {:#x} ({}x{})",
                      window.window, window.geometry.width(),
                      window.geometry.height());
          }
          Colour c = get_background_colour_preference(*state);
          unsigned long bg =
              window.color_depth == argb8888_color_depth
                  ? c.to_x11_color(display, screen, window.opacity < 0xff, true)
                  : c.to_x11_color(display, screen, false, false);
          XSetForeground(display, window.gc, bg);
          XFillRectangle(display, window.drawable, window.gc, 0, 0,
                         window.geometry.width(), window.geometry.height());
        }
#endif

        changed++;
        update_surface();
      }

      /* move window if it isn't in right position */
      if ((fixed_pos == 0) && old_pos != window.geometry.pos()) {
        XMoveWindow(display, window.window, window.geometry.x(),
                    window.geometry.y());
        changed++;
      }

      /* update struts */
      if (changed != 0) {
        auto window_type = own_window_type.get(*state);
        // Openbox will implicitly set struts even for window_type::DOCK
        if (window_type == window_type::PANEL) { set_struts(); }
      }
    }
#endif

    /* update lua window globals */
    llua_update_window_table(window.geometry.size(),
                             rect<int>(text_start, text_size));

    clear_text(1);

    if (use_double_buffer.get(*state)) {
      XRectangle rect = conky::rect<int>(text_start - border_total,
                                         text_size + border_total * 2)
                            .to_xrectangle();
      XUnionRectWithRegion(&rect, window.repaint_region, window.repaint_region);
    }
  }

  process_surface_events(this, display);

#ifdef BUILD_XDAMAGE
  if (window.window_damage) {
    XDamageSubtract(display, window.window_damage, window.damage_region, None);
    XFixesSetRegion(display, window.damage_region, nullptr, 0);
  }
#endif /* BUILD_XDAMAGE */

  /* XDBE doesn't seem to provide a way to clear the back buffer
   * without interfering with the front buffer, other than passing
   * XdbeBackground to XdbeSwapBuffers. That means that if we're
   * using XDBE, we need to redraw the text even if it wasn't part of
   * the exposed area. OTOH, if we're not going to call draw_stuff at
   * all, then no swap happens and we can safely do nothing. */

  if (XEmptyRegion(window.repaint_region) == 0) {
    if (use_double_buffer.get(*state)) {
      XRectangle rect = conky::rect<int>(text_start - border_total,
                                         text_size + border_total * 2)
                            .to_xrectangle();
      XUnionRectWithRegion(&rect, window.repaint_region, window.repaint_region);
    }
    XSetRegion(display, window.gc, window.repaint_region);
#ifdef BUILD_XFT
    if (use_xft.get(*state)) {
      XftDrawSetClip(window.xftdraw, window.repaint_region);
    }
#endif
    draw_stuff();
    XDestroyRegion(window.repaint_region);
    window.repaint_region = XCreateRegion();
  }

  // handled
  return true;
}

/// Cached top-level parent of conky's window; invalidated on ReparentNotify.
static Window window_top_parent = None;

#ifdef OWN_WINDOW
static bool test_event_cursor_over_conky(xi_pointer_event &ev) {
  // Fast reject: if cursor is outside our geometry, it's definitely not over
  // this conky instance.  This avoids expensive X11 round-trips
  // (XIQueryPointer + XQueryTree) for the vast majority of events when
  // multiple conky instances are running.  See #1886.

  // Conky only listens for its own window events when `own_window` is set,
  // so this check isn't needed in the most common case.

  bool inside_geometry = window.geometry.contains(ev.pos_absolute);
  if (!inside_geometry) { return false; }

  // AABB test passes, but there could still be a window covering conky

  Window event_window =
      query_x11_window_at_pos(display, ev.pos_absolute, ev.device->master);
  if (window_top_parent == None) {
    window_top_parent = query_x11_top_parent(display, window.window);
  }
  bool same_window =
      query_x11_top_parent(display, event_window) == window_top_parent;

  LOG_TRACE_WITH(({"pos", ev.pos_absolute}, {"geom", window.geometry}),
                 "xi event: type={} inside_geom={} over_conky={}", ev.evtype,
                 inside_geometry, same_window);
  return same_window;
}

#ifdef BUILD_MOUSE_EVENTS
bool handle_event(conky::display_output_x11 *surface, Display *display,
                  xi_pointer_enter ev, conky::x11::event *propagated) {
  if (!own_window.get(*state)) {
    if (!window.cursor_over_window) {
      llua_mouse_hook(mouse_crossing_event(
          mouse_event_t::AREA_ENTER, ev.pos_absolute - window.geometry.pos(),
          ev.pos_absolute));
      window.cursor_over_window = true;
    }

    *propagated = ev;
  } else {
    llua_mouse_hook(mouse_crossing_event(
        mouse_event_t::AREA_ENTER, ev.pos_absolute - window.geometry.pos(),
        ev.pos_absolute));
  }
  return true;
}

bool handle_event(conky::display_output_x11 *surface, Display *display,
                  xi_pointer_leave ev, conky::x11::event *propagated) {
  if (!own_window.get(*state)) {
    if (window.cursor_over_window) {
      llua_mouse_hook(mouse_crossing_event(
          mouse_event_t::AREA_LEAVE, ev.pos_absolute - window.geometry.pos(),
          ev.pos_absolute));
      window.cursor_over_window = false;
    }

    *propagated = ev;
  } else {
    llua_mouse_hook(mouse_crossing_event(
        mouse_event_t::AREA_LEAVE, ev.pos_absolute - window.geometry.pos(),
        ev.pos_absolute));
  }
  return true;
}
#endif /* BUILD_MOUSE_EVENTS */

bool handle_event(conky::display_output_x11 *surface, Display *display,
                  xi_pointer_move ev, conky::x11::event *propagated) {
  bool lua_consumed = false;

#ifdef BUILD_MOUSE_EVENTS
  modifier_state_t mods = x11_modifier_state(ev.mods.effective);

  bool has_move_x = ev.test_valuator(valuator_t::MOVE_X);
  bool has_move_y = ev.test_valuator(valuator_t::MOVE_Y);
  bool has_scroll_x = ev.test_valuator(valuator_t::SCROLL_X);
  bool has_scroll_y = ev.test_valuator(valuator_t::SCROLL_Y);
  bool is_move = has_move_x || has_move_y;
  bool is_scroll = has_scroll_x || has_scroll_y;

  if (!own_window.get(*state)) {
    // Extra checks when conky is mounted on root window with mouse events
    // enabled.

    bool cursor_over_conky = test_event_cursor_over_conky(ev);

    if (is_move) {
      // generate crossing events; these are artificial and crossing events are
      // useless to others - so we never propagate these
      if (cursor_over_conky) {
        if (!window.cursor_over_window) {
          llua_mouse_hook(mouse_crossing_event(
              mouse_event_t::AREA_ENTER,
              ev.pos_absolute - window.geometry.pos(), ev.pos_absolute));
        }
        window.cursor_over_window = true;
      } else if (window.cursor_over_window) {
        llua_mouse_hook(mouse_crossing_event(
            mouse_event_t::AREA_LEAVE, ev.pos_absolute - window.geometry.pos(),
            ev.pos_absolute));
        window.cursor_over_window = false;
      }
    }

    // We only capture mouse events on !own_window when BUILD_MOUSE_EVENTS,
    // in any other case, this check does nothing.
    if (!cursor_over_conky) { return true; }
  }

  LOG_TRACE_WITH(({"move_x", has_move_x}, {"move_y", has_move_y},
                  {"scroll_x", has_scroll_x}, {"scroll_y", has_scroll_y}),
                 "xi motion: is_move={} is_scroll={}", is_move, is_scroll);

  if (is_move) {
    auto window_pos = ev.pos_absolute - window.geometry.pos();
    lua_consumed =
        llua_mouse_hook(mouse_move_event(window_pos, ev.pos_absolute, mods));
  }
  if (is_scroll) {
    scroll_direction_t scroll_direction = scroll_direction_t::UNKNOWN;
    auto vertical = ev.valuator_relative_value(valuator_t::SCROLL_Y);
    double vertical_value = vertical.value_or(0.0);

    if (vertical_value != 0.0) {
      auto *info = ev.valuator_info(valuator_t::SCROLL_Y);
      double increment = (info != nullptr) ? info->increment : 1.0;
      scroll_direction = (vertical_value * increment) < 0.0
                             ? scroll_direction_t::UP
                             : scroll_direction_t::DOWN;
      LOG_TRACE_WITH(
          ({"vertical", vertical_value}, {"increment", increment},
           {"product", vertical_value * increment}),
          "xi scroll dir={}",
          scroll_direction == scroll_direction_t::UP ? "UP" : "DOWN");
    } else {
      auto horizontal = ev.valuator_relative_value(valuator_t::SCROLL_X);
      double horizontal_value = horizontal.value_or(0.0);
      if (horizontal_value != 0.0) {
        auto *info = ev.valuator_info(valuator_t::SCROLL_X);
        double increment = (info != nullptr) ? info->increment : 1.0;
        scroll_direction = (horizontal_value * increment) < 0.0
                               ? scroll_direction_t::LEFT
                               : scroll_direction_t::RIGHT;
      }
    }

    if (scroll_direction != scroll_direction_t::UNKNOWN) {
      auto window_pos = ev.pos_absolute - window.geometry.pos();
      lua_consumed = llua_mouse_hook(mouse_scroll_event(
          window_pos, ev.pos_absolute, scroll_direction, mods));
    }
  }
#endif /* BUILD_MOUSE_EVENTS */

  if (!lua_consumed) { *propagated = std::move(ev); }
  return true;
}

#ifdef BUILD_MOUSE_EVENTS
static void handle_press_release_events(xi_pointer_interact_event &ev,
                                        bool *consumed) {
  modifier_state_t mods = x11_modifier_state(ev.mods.effective);

  LOG_TRACE("xi button: detail={} type={}", ev.detail,
            ev.evtype == XI_ButtonPress ? "press" : "release");
  if (ev.detail >= 4 && ev.detail <= 7) {
    // Fallback: use button 4-7 as scroll if this device has no independent
    // scroll valuators (e.g. Xephyr aliases scroll and move axes).
    // Devices with working scroll valuators handle scroll via XI_Motion.
    bool has_scroll_valuators =
        ev.device->valuator(valuator_t::SCROLL_X).index != SIZE_MAX ||
        ev.device->valuator(valuator_t::SCROLL_Y).index != SIZE_MAX;
    LOG_TRACE("xi button 4-7 fallback: has_scroll_valuators={}",
              has_scroll_valuators);
    if (!has_scroll_valuators && ev.evtype == XI_ButtonPress) {
      scroll_direction_t direction = x11_scroll_direction(ev.detail);
      auto window_pos = ev.pos_absolute - window.geometry.pos();
      *consumed = llua_mouse_hook(
          mouse_scroll_event(window_pos, ev.pos_absolute, direction, mods));
    }
    return;
  }

  auto button = x11_mouse_button_code(ev.detail);
  if (!button.has_value()) return;

  mouse_event_t type = mouse_event_t::PRESS;
  if (ev.evtype == XI_ButtonRelease) { type = mouse_event_t::RELEASE; }

  *consumed = llua_mouse_hook(
      mouse_button_event(type, ev.pos, ev.pos_absolute, button.value(), mods));

  return;
}
#endif /* BUILD_MOUSE_EVENTS */

bool handle_event(conky::display_output_x11 *surface, Display *display,
                  xi_pointer_press ev, conky::x11::event *propagated) {
  bool cursor_over_conky = test_event_cursor_over_conky(ev);
  bool consumed = false;

  if (!cursor_over_conky) { return true; }

#ifdef BUILD_MOUSE_EVENTS
  handle_press_release_events(ev, &consumed);
#endif /* BUILD_MOUSE_EVENTS */

  // If the device supports scroll valuators, then propagating scroll event
  // from 4-7 legacy button events is wrong.
  if (ev.detail >= 4 && ev.detail <= 7) {
    bool has_scroll_valuators =
        ev.device->valuator(valuator_t::SCROLL_X).index != SIZE_MAX ||
        ev.device->valuator(valuator_t::SCROLL_Y).index != SIZE_MAX;
    // ... if it doesn't, then we won't be propagating scroll via
    // xi_pointer_move branch, so we should do it here, BUT only if
    // handle_press_release_events handler didn't set it to true; i.e. user
    // script said the event was consumed.
    consumed |= has_scroll_valuators;
  }

  if (!consumed) { *propagated = std::move(ev); }
  return true;
}

bool handle_event(conky::display_output_x11 *surface, Display *display,
                  xi_pointer_release ev, conky::x11::event *propagated) {
  bool cursor_over_conky = test_event_cursor_over_conky(ev);
  bool lua_consumed = false;

  if (!cursor_over_conky) { return true; }

#ifdef BUILD_MOUSE_EVENTS
  handle_press_release_events(ev, &lua_consumed);
#endif /* BUILD_MOUSE_EVENTS */

  if (!lua_consumed) { *propagated = std::move(ev); }
  return true;
}

bool handle_event(conky::display_output_x11 *surface, Display *display,
                  XReparentEvent ev, conky::x11::event *propagated) {
  if (own_window.get(*state)) { set_transparent_background(&window); }
  // Invalidate cached top parent -- window tree changed.
  window_top_parent = None;
  return true;
}

bool handle_event(conky::display_output_x11 *surface, Display *display,
                  XConfigureEvent ev, conky::x11::event *propagated) {
  if (ev.type != ConfigureNotify) return false;

  if (own_window.get(*state)) {
    auto configure_size = vec2i(ev.width, ev.height);
    /* if window size isn't what's expected, set fixed size */
    if (configure_size != window.geometry.size()) {
      if (window.geometry.size().surface() != 0) { fixed_size = 1; }

      /* clear old stuff before screwing up
       * size and pos */
      surface->clear_text(1);

      {
        XWindowAttributes attrs;
        if (XGetWindowAttributes(display, window.window, &attrs) != 0) {
          window.geometry.set_size(attrs.width, attrs.height);
        }
      }

      auto border_total = vec2i::uniform(get_border_total() * 2);
      text_size = window.geometry.size() - border_total;

      // don't apply dpi scaling to max pixel size
      int mw = dpi_scale(maximum_width.get(*state));
      if (mw > 0) { text_size.set_x(std::min(mw, text_size.x())); }
    }

    // Keep window.geometry.pos() in sync with the actual screen position.
    // ev.xconfigure.x/y can't be used directly because for reparented windows
    // they're relative to the WM's decoration frame, not the root.
    {
      int root_x, root_y;
      Window child_return;
      XTranslateCoordinates(display, window.window, window.root, 0, 0, &root_x,
                            &root_y, &child_return);
      window.geometry.set_pos(root_x, root_y);
    }
  }

  return true;
}
#endif /* OWN_WINDOW */

bool handle_event(conky::display_output_x11 *surface, Display *display,
                  XPropertyEvent ev, conky::x11::event *propagated) {
  if (ev.state == PropertyNewValue) {
    get_x11_desktop_info(ev.display, ev.atom);
  }

  if (ev.atom == 0) return false;

  if (ev.atom == XA_RESOURCE_MANAGER) {
    update_x11_resource_db();
    update_x11_workarea();
    screen_dpi = -1;
    update_dpi();
    return true;
  }

  if (window.opacity == 0xff) {
    Atom _XROOTPMAP_ID = XInternAtom(display, "_XROOTPMAP_ID", True);
    Atom _XROOTMAP_ID = XInternAtom(display, "_XROOTMAP_ID", True);
    if (ev.atom == _XROOTPMAP_ID || ev.atom == _XROOTMAP_ID) {
      if (forced_redraw.get(*state)) {
        draw_stuff();
        next_update_time = get_time();
        need_to_update = 1;
      }
      return true;
    }
  }

  return false;
}

bool handle_event(conky::display_output_x11 *surface, Display *display,
                  XExposeEvent ev, conky::x11::event *propagated) {
  XRectangle r{
      .x = static_cast<short>(ev.x),
      .y = static_cast<short>(ev.y),
      .width = static_cast<unsigned short>(ev.width),
      .height = static_cast<unsigned short>(ev.height),
  };
  XUnionRectWithRegion(&r, window.repaint_region, window.repaint_region);
  XSync(display, False);
  return true;
}

#ifdef BUILD_XDAMAGE
bool handle_event(conky::display_output_x11 *surface, Display *display,
                  XDamageNotifyEvent ev, conky::x11::event *propagated) {
  XFixesSetRegion(display, window.damage_scratch, &ev.area, 1);
  XFixesUnionRegion(display, window.damage_region, window.damage_region,
                    window.damage_scratch);
  return true;
}
#endif /* BUILD_XDAMAGE */

/// Handles all events conky can receive.
///
/// @return true if event should move input focus to conky
inline bool process_event(conky::display_output_x11 *surface, Display *display,
                          conky::x11::event &ev,
                          conky::x11::event *propagated) {
#define HANDLE_EV(type)                                                       \
  if (auto it = ev.into_inner<type>()) {                                      \
    return handle_event(surface, display, std::move(it.value()), propagated); \
  }
  // order of handlers here does not matter because they're all mutually
  // exclusive.

  HANDLE_EV(XPropertyEvent);
  HANDLE_EV(XExposeEvent);

#ifdef OWN_WINDOW
  HANDLE_EV(xi_pointer_move);
  HANDLE_EV(xi_pointer_press);
  HANDLE_EV(xi_pointer_release);
#ifdef BUILD_MOUSE_EVENTS
  HANDLE_EV(xi_pointer_enter);
  HANDLE_EV(xi_pointer_leave);
#endif /* BUILD_MOUSE_EVENTS */
  HANDLE_EV(XReparentEvent);
  HANDLE_EV(XConfigureEvent);
#endif /* OWN_WINDOW */

#ifdef BUILD_XDAMAGE
  HANDLE_EV(XDamageNotifyEvent);
#endif /* BUILD_XDAMAGE */

#undef HANDLE_EV

  // event not handled
  return false;
}

/// A series of checks which check whether a window should be opaque to events,
/// even if all other conditions (e.g. hints from Lua) promote propagation.
///
/// These checks don't rely on actual event data, only on how the window is
/// shown to the user.
///
/// Example: a normal decorated window shouldn't propagate events.
static bool is_window_opaque_to_events() {
  switch (own_window_type.get(*state)) {
    case window_type::NORMAL:
    case window_type::UTILITY:
      // decorated normal windows always consume events
      if (!TEST_HINT(own_window_hints.get(*state), window_hints::UNDECORATED)) {
        return true;
      }
      break;
    // when window_type::DESKTOP, we still propagate to root managed by e.g.
    // Openbox
    default:
      break;
  }
  return false;
}

void process_surface_events(conky::display_output_x11 *surface,
                            Display *display) {
  int pending = XPending(display);
  if (pending == 0) return;

  LOG_TRACE("processing {} X11 events", pending);

  /* handle X events */
  while (XPending(display) != 0) {
    auto ev = conky::x11::event::read(display);

    // Holds a propagated event in case a handler decides it would be correct
    // to propagate it.
    conky::x11::event propagated;
    bool handled = process_event(surface, display, ev, &propagated);

    if (propagated.is_some() && !is_window_opaque_to_events()) {
      LOG_TRACE("propagating event: {}", propagated.variant_index());
      propagate_x11_event(propagated);
    }
  }

  LOG_TRACE("done processing {} events", pending);
}

void display_output_x11::sigterm_cleanup() {
  XDestroyRegion(window.repaint_region);
  window.repaint_region = nullptr;
#ifdef BUILD_XDAMAGE
  if (window.window_damage) {
    XDamageDestroy(display, window.window_damage);
    XFixesDestroyRegion(display, window.damage_region);
    XFixesDestroyRegion(display, window.damage_scratch);
  }
#endif /* BUILD_XDAMAGE */
}

void display_output_x11::cleanup() {
  if (window.window != None) {
    int border_total = get_border_total();

    XClearArea(display, window.window, text_start.x() - border_total,
               text_start.y() - border_total, text_size.x() + 2 * border_total,
               text_size.y() + 2 * border_total, 0);
  }
  destroy_window();
  free_fonts(utf8_mode.get(*state));
  if (window.repaint_region != nullptr) {
    XDestroyRegion(window.repaint_region);
    window.repaint_region = nullptr;
  }
}

void display_output_x11::set_foreground_color(Colour c) {
  current_color = c;
  current_color.alpha = window.opacity;
  XSetForeground(
      display, window.gc,
      current_color.to_x11_color(display, screen, window.opacity < 0xff));
}

int display_output_x11::calc_text_width(const char *s) {
  std::size_t slen = strlen(s);
#ifdef BUILD_XFT
  if (use_xft.get(*state)) {
    XGlyphInfo gi;

    if (utf8_mode.get(*state)) {
      XftTextExtentsUtf8(display, x_fonts[selected_font].xftfont,
                         reinterpret_cast<const FcChar8 *>(s), slen, &gi);
    } else {
      XftTextExtents8(display, x_fonts[selected_font].xftfont,
                      reinterpret_cast<const FcChar8 *>(s), slen, &gi);
    }
    return gi.xOff;
  }
#endif /* BUILD_XFT */

  return XTextWidth(x_fonts[selected_font].font, s, slen);
}

void display_output_x11::draw_string_at(int x, int y, const char *s, int w) {
#ifdef BUILD_XFT
  if (use_xft.get(*state)) {
    XColor c{};
    XftColor c2{};

    c.pixel =
        current_color.to_x11_color(display, screen, window.opacity < 0xff);
    // query color on custom colormap
    XQueryColor(display, window.colourmap, &c);

    c2.pixel = c.pixel;
    c2.color.red = c.red;
    c2.color.green = c.green;
    c2.color.blue = c.blue;
    c2.color.alpha = x_fonts[selected_font].font_alpha;
    if (utf8_mode.get(*state)) {
      XftDrawStringUtf8(window.xftdraw, &c2, x_fonts[selected_font].xftfont, x,
                        y, reinterpret_cast<const XftChar8 *>(s), w);
    } else {
      XftDrawString8(window.xftdraw, &c2, x_fonts[selected_font].xftfont, x, y,
                     reinterpret_cast<const XftChar8 *>(s), w);
    }
  } else
#endif
  {
    if (utf8_mode.get(*state)) {
      Xutf8DrawString(display, window.drawable, x_fonts[selected_font].fontset,
                      window.gc, x, y, s, w);
    } else {
      XDrawString(display, window.drawable, window.gc, x, y, s, w);
    }
  }
}

void display_output_x11::set_line_style(int w, bool solid) {
  XSetLineAttributes(display, window.gc, w, solid ? LineSolid : LineOnOffDash,
                     CapButt, JoinMiter);
}

void display_output_x11::set_dashes(char *s) {
  XSetDashes(display, window.gc, 0, s, 2);
}

void display_output_x11::draw_line(int x1, int y1, int x2, int y2) {
  XDrawLine(display, window.drawable, window.gc, x1, y1, x2, y2);
}

void display_output_x11::draw_rect(int x, int y, int w, int h) {
  XDrawRectangle(display, window.drawable, window.gc, x, y, w, h);
}

void display_output_x11::fill_rect(int x, int y, int w, int h) {
  XFillRectangle(display, window.drawable, window.gc, x, y, w, h);
}

void display_output_x11::draw_arc(int x, int y, int w, int h, int a1, int a2) {
  XDrawArc(display, window.drawable, window.gc, x, y, w, h, a1, a2);
}

void display_output_x11::move_win(int x, int y) {
#ifdef OWN_WINDOW
  window.geometry.set_pos(x, y);
  XMoveWindow(display, window.window, x, y);
#endif /* OWN_WINDOW */
}

const float PIXELS_PER_INCH = 96.0;
float display_output_x11::get_dpi_scale() {
  if (screen_dpi > 0) {
    return static_cast<float>(screen_dpi) / PIXELS_PER_INCH;
  }
  return 1.0;
}

void display_output_x11::end_draw_stuff() { swap_x11_buffers(); }

void display_output_x11::clear_text(int exposures) {
  if (use_double_buffer.get(*state)) {
    /* The swap action is XdbeBackground, which clears */
    return;
  }
#ifndef BUILD_XDBE
  else
#endif
      if ((display != nullptr) &&
          (window.window != 0u)) {  // make sure these are !null
    /* there is some extra space for borders and outlines */
    int border_total = get_border_total();

    XClearArea(display, window.window, text_start.x() - border_total,
               text_start.y() - border_total, text_size.x() + 2 * border_total,
               text_size.y() + 2 * border_total, exposures != 0 ? True : 0);
  }
}

#ifdef BUILD_XFT

int display_output_x11::font_height(unsigned int f) {
  assert(f < x_fonts.size());
  if (use_xft.get(*state)) {
    return x_fonts[f].xftfont->ascent + x_fonts[f].xftfont->descent;
  } else {
    return x_fonts[f].font->max_bounds.ascent +
           x_fonts[f].font->max_bounds.descent;
  }
}

int display_output_x11::font_ascent(unsigned int f) {
  assert(f < x_fonts.size());
  if (use_xft.get(*state)) {
    return x_fonts[f].xftfont->ascent;
  } else {
    return x_fonts[f].font->max_bounds.ascent;
  }
}

int display_output_x11::font_descent(unsigned int f) {
  assert(f < x_fonts.size());
  if (use_xft.get(*state)) {
    return x_fonts[f].xftfont->descent;
  } else {
    return x_fonts[f].font->max_bounds.descent;
  }
}

#else

int display_output_x11::font_height(unsigned int f) {
  assert(f < x_fonts.size());
  return x_fonts[f].font->max_bounds.ascent +
         x_fonts[f].font->max_bounds.descent;
}

int display_output_x11::font_ascent(unsigned int f) {
  assert(f < x_fonts.size());
  return x_fonts[f].font->max_bounds.ascent;
}

int display_output_x11::font_descent(unsigned int f) {
  assert(f < x_fonts.size());
  return x_fonts[f].font->max_bounds.descent;
}

#endif

void display_output_x11::setup_fonts(void) {
#ifdef BUILD_XFT
  if (use_xft.get(*state)) {
    if (window.xftdraw != nullptr) {
      XftDrawDestroy(window.xftdraw);
      window.xftdraw = nullptr;
    }
    window.xftdraw = XftDrawCreate(display, window.drawable, window.visual,
                                   window.colourmap);
  }
#endif /* BUILD_XFT */
}

void display_output_x11::set_font(unsigned int f) {
  if (f >= x_fonts.size()) {
    LOG_WARNING("font index {} out of range ({} loaded)", f, x_fonts.size());
    return;
  }
#ifdef BUILD_XFT
  if (use_xft.get(*state)) { return; }
#endif /* BUILD_XFT */
  if (x_fonts.size() > f && x_fonts[f].font != nullptr &&
      window.gc != nullptr) {
    XSetFont(display, window.gc, x_fonts[f].font->fid);
  }
}

void display_output_x11::free_fonts(bool utf8) {
  for (auto &font : x_fonts) {
#ifdef BUILD_XFT
    if (use_xft.get(*state)) {
      /* Close each font if it has been initialized */
      if (font.xftfont) { XftFontClose(display, font.xftfont); }
    } else
#endif /* BUILD_XFT */
    {
      if (font.font != nullptr) { XFreeFont(display, font.font); }
      if (utf8 && (font.fontset != nullptr)) {
        XFreeFontSet(display, font.fontset);
      }
    }
  }
  x_fonts.clear();
#ifdef BUILD_XFT
  if (window.xftdraw != nullptr) {
    XftDrawDestroy(window.xftdraw);
    window.xftdraw = nullptr;
  }
#endif /* BUILD_XFT */
}
void display_output_x11::load_fonts(bool utf8) {
  x_fonts.resize(fonts.size());
  for (unsigned int i = 0; i < fonts.size(); i++) {
    auto &font = fonts[i];
    auto &xfont = x_fonts[i];
#ifdef BUILD_XFT
    /* load Xft font */
    if (use_xft.get(*state)) {
      if (xfont.xftfont == nullptr) {
        xfont.xftfont = XftFontOpenName(display, screen, font.name.c_str());
      }

      if (xfont.xftfont != nullptr) { continue; }

      LOG_WARNING("can't load Xft font '{}', trying fallback", font.name);
      if ((xfont.xftfont = XftFontOpenName(display, screen, "courier-12")) !=
          nullptr) {
        continue;
      }

      SYSTEM_ERR("can't load Xft font '{}'", "courier-12");

      continue;
    }
#endif
    if (utf8 && xfont.fontset == nullptr) {
      char **missing;
      int missingnum;
      char *missingdrawn;
      xfont.fontset = XCreateFontSet(display, font.name.c_str(), &missing,
                                     &missingnum, &missingdrawn);
      XFreeStringList(missing);
      if (xfont.fontset == nullptr) {
        LOG_WARNING("can't load fontset '{}', trying fallback", font.name);
        xfont.fontset = XCreateFontSet(display, "fixed", &missing, &missingnum,
                                       &missingdrawn);
        if (xfont.fontset == nullptr) {
          SYSTEM_ERR("can't load font '{}'", "fixed");
        }
      }
    }
    /* load normal font */
    if ((xfont.font == nullptr) &&
        (xfont.font = XLoadQueryFont(display, font.name.c_str())) == nullptr) {
      LOG_WARNING("can't load font '{}', trying fallback", font.name);
      if ((xfont.font = XLoadQueryFont(display, "fixed")) == nullptr) {
        SYSTEM_ERR("can't load font '{}'", "fixed");
      }
    }
  }
#ifdef BUILD_XFT
  if (!x_fonts.empty()) {
    x_fonts[0].font_alpha = static_cast<int>(text_alpha.get(*state) * 0xffff);
  }
#endif /* BUILD_XFT */
}

void display_output_x11::update_surface() {
#ifdef BUILD_LUA_CAIRO_XLIB
  current_surface.reset(cairo_xlib_surface_create(
                            display, window.drawable, window.visual,
                            window.geometry.width(), window.geometry.height()),
                        cairo_surface_destroy);
#endif /* BUILD_LUA_CAIRO_XLIB */
}

std::weak_ptr<conky::draw_surface> display_output_x11::drawing_surface() {
#ifdef BUILD_LUA_CAIRO_XLIB
  if (!current_surface && display && window.drawable) { update_surface(); }
  return current_surface;
#else
  return {};
#endif /* BUILD_LUA_CAIRO_XLIB */
}

}  // namespace conky
