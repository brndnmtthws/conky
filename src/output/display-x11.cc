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

#include "display-x11.hh"

#include <X11/X.h>
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
#if defined(BUILD_MOUSE_EVENTS) || defined(BUILD_XINPUT)
#include "../mouse-events.h"
#endif /* BUILD_MOUSE_EVENTS || BUILD_XINPUT */
#ifdef BUILD_XINPUT
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#undef COUNT
#endif /* BUILD_XINPUT */
#include <X11/Xresource.h>

#include <cstdint>
#include <iostream>
#include <map>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../conky.h"
#include "../content/colours.hh"
#include "../geometry.h"
#include "../logging.h"
#include "../lua/llua.h"
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

#ifdef BUILD_XFT
namespace {
class xftalpha_setting : public conky::simple_config_setting<float> {
  using Base = conky::simple_config_setting<float>;

 protected:
  void lua_setter(lua::state &l, bool init) override {
    lua::stack_sentry s(l, -2);

    Base::lua_setter(l, init);

    if (init && out_to_x.get(*state)) {
      x_fonts.resize(std::max(1, static_cast<int>(fonts.size())));
      x_fonts[0].font_alpha = do_convert(l, -1).first * 0xffff;
    }

    ++s;
  }

 public:
  xftalpha_setting() : Base("xftalpha", 1.0, false) {}
};

xftalpha_setting xftalpha;
}  // namespace
#endif /* BUILD_XFT */

static void X11_create_window();

struct _x11_stuff_s {
  Region region;
#ifdef BUILD_XDAMAGE
  Damage damage;
  XserverRegion region2, part;
  int event_base, error_base;
#endif
} x11_stuff;

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

    set_transparent_background(window.window);
  }
#endif

  create_gc();

  draw_stuff();

  x11_stuff.region = XCreateRegion();
#ifdef BUILD_XDAMAGE
  if (XDamageQueryExtension(display, &x11_stuff.event_base,
                            &x11_stuff.error_base) == 0) {
    NORM_ERR("Xdamage extension unavailable");
    x11_stuff.damage = 0;
  } else {
    x11_stuff.damage =
        XDamageCreate(display, window.window, XDamageReportNonEmpty);
    x11_stuff.region2 = XFixesCreateRegionFromWindow(display, window.window, 0);
    x11_stuff.part = XFixesCreateRegionFromWindow(display, window.window, 0);
  }
#endif /* BUILD_XDAMAGE */

  selected_font = 0;
  update_text_area(); /* to get initial size of the window */
}

namespace conky {
namespace {
conky::display_output_x11 x11_output;
}  // namespace

template <>
void register_output<output_t::X11>(display_outputs_t &outputs) {
  outputs.push_back(&x11_output);
}

display_output_x11::display_output_x11() : display_output_base("x11") {
  is_graphical = true;
}

bool display_output_x11::detect() {
  if (out_to_x.get(*state)) {
    DBGP2("Display output '%s' enabled in config.", name.c_str());
    return true;
  }
  return false;
}

bool display_output_x11::initialize() {
  X11_create_window();
  return true;
}

bool display_output_x11::shutdown() {
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
    struct timeval tv {};
    int s;
    // t = next_update_time - get_time();

    t = std::min(std::max(t, 0.0), active_update_interval());

    tv.tv_sec = static_cast<long>(t);
    tv.tv_usec = static_cast<long>(t * 1000000) % 1000000;
    FD_ZERO(&fdsr);
    FD_SET(ConnectionNumber(display), &fdsr);

    s = select(ConnectionNumber(display) + 1, &fdsr, nullptr, nullptr, &tv);
    if (s == -1) {
      if (errno != EINTR) { NORM_ERR("can't select(): %s", strerror(errno)); }
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
        set_transparent_background(window.window);
#ifdef BUILD_XDBE
        /* swap buffers */
        xdbe_swap_buffers();
#else
        if (use_xpmdb.get(*state)) {
          XFreePixmap(display, window.back_buffer);
          window.back_buffer = XCreatePixmap(
              display, window.window, window.geometry.width(),
              window.geometry.height(), DefaultDepth(display, screen));

          if (window.back_buffer != None) {
            window.drawable = window.back_buffer;
          } else {
            // this is probably reallllly bad
            NORM_ERR("Failed to allocate back buffer");
          }
          XSetForeground(display, window.gc, 0);
          XFillRectangle(display, window.drawable, window.gc, 0, 0,
                         window.geometry.width(), window.geometry.height());
        }
#endif

        changed++;
        /* update lua window globals */
        llua_update_window_table(rect<int>(text_start, text_size));
      }

      /* move window if it isn't in right position */
      if ((fixed_pos == 0) && old_pos != window.geometry.pos()) {
        XMoveWindow(display, window.window, window.geometry.x(),
                    window.geometry.y());
        changed++;
      }

      /* update struts */
      if ((changed != 0) && own_window_type.get(*state) == window_type::PANEL) {
        set_struts(text_alignment.get(*state));
      }
    }
#endif

    clear_text(1);

#if defined(BUILD_XDBE)
    if (use_xdbe.get(*state)) {
#else
    if (use_xpmdb.get(*state)) {
#endif
      XRectangle rect = conky::rect<int>(text_start - border_total,
                                         text_size + border_total * 2)
                            .to_xrectangle();
      XUnionRectWithRegion(&rect, x11_stuff.region, x11_stuff.region);
    }
  }

  process_surface_events(this, display);

#ifdef BUILD_XDAMAGE
  if (x11_stuff.damage) {
    XDamageSubtract(display, x11_stuff.damage, x11_stuff.region2, None);
    XFixesSetRegion(display, x11_stuff.region2, nullptr, 0);
  }
#endif /* BUILD_XDAMAGE */

  /* XDBE doesn't seem to provide a way to clear the back buffer
   * without interfering with the front buffer, other than passing
   * XdbeBackground to XdbeSwapBuffers. That means that if we're
   * using XDBE, we need to redraw the text even if it wasn't part of
   * the exposed area. OTOH, if we're not going to call draw_stuff at
   * all, then no swap happens and we can safely do nothing. */

  if (XEmptyRegion(x11_stuff.region) == 0) {
#if defined(BUILD_XDBE)
    if (use_xdbe.get(*state)) {
#else
    if (use_xpmdb.get(*state)) {
#endif
      XRectangle rect = conky::rect<int>(text_start - border_total,
                                         text_size + border_total * 2)
                            .to_xrectangle();
      XUnionRectWithRegion(&rect, x11_stuff.region, x11_stuff.region);
    }
    XSetRegion(display, window.gc, x11_stuff.region);
#ifdef BUILD_XFT
    if (use_xft.get(*state)) {
      XftDrawSetClip(window.xftdraw, x11_stuff.region);
    }
#endif
    draw_stuff();
    XDestroyRegion(x11_stuff.region);
    x11_stuff.region = XCreateRegion();
  }

  // handled
  return true;
}

enum class x_event_handler {
  XINPUT_MOTION,
  MOUSE_INPUT,
  PROPERTY_NOTIFY,
  EXPOSE,
  REPARENT,
  CONFIGURE,
  BORDER_CROSSING,
  DAMAGE,
};

template <x_event_handler handler>
bool handle_event(conky::display_output_x11 *surface, Display *display,
                  XEvent &ev, bool *consumed, void **cookie) {
  return false;
}

#ifdef OWN_WINDOW
template <>
bool handle_event<x_event_handler::MOUSE_INPUT>(
    conky::display_output_x11 *surface, Display *display, XEvent &ev,
    bool *consumed, void **cookie) {
#ifdef BUILD_XINPUT
  if (ev.type == ButtonPress || ev.type == ButtonRelease ||
      ev.type == MotionNotify) {
    // destroy basic X11 events; and manufacture them later when trying to
    // propagate XInput ones - this is required because there's no (simple) way
    // of making sure the lua hook controls both when it only handles XInput
    // ones.
    *consumed = true;
    return true;
  }

  if (ev.type != GenericEvent || ev.xgeneric.extension != window.xi_opcode)
    return false;

  if (!XGetEventData(display, &ev.xcookie)) {
    // already consumed
    return true;
  }
  xi_event_type event_type = ev.xcookie.evtype;

  if (event_type == XI_HierarchyChanged) {
    auto device_change = reinterpret_cast<XIHierarchyEvent *>(ev.xcookie.data);
    handle_xi_device_change(device_change);
    XFreeEventData(display, &ev.xcookie);
    return true;
  }

  auto *data = xi_event_data::read_cookie(display, ev.xcookie.data);
  XFreeEventData(display, &ev.xcookie);
  if (data == nullptr) {
    // we ate the cookie, Xi event not handled
    return true;
  }
  *cookie = data;

  Window event_window = query_x11_window_at_pos(display, data->pos_absolute, data->device->master);

  bool same_window = query_x11_top_parent(display, event_window) ==
                     query_x11_top_parent(display, window.window);
  bool cursor_over_conky =
      same_window && window.geometry.contains(data->pos_absolute);

  // XInput reports events twice on some hardware (even by 'xinput --test-xi2')
  auto hash = std::make_tuple(data->serial, data->evtype, data->event);
  typedef std::map<decltype(hash), Time> MouseEventDebounceMap;
  static MouseEventDebounceMap debounce{};

  Time now = data->time;
  bool already_handled = debounce.count(hash) > 0;
  debounce[hash] = now;

  // clear stale entries
  for (auto iter = debounce.begin(); iter != debounce.end();) {
    if (data->time - iter->second > 1000) {
      iter = debounce.erase(iter);
    } else {
      ++iter;
    }
  }

  if (already_handled) {
    *consumed = true;
    return true;
  }

#ifdef BUILD_MOUSE_EVENTS
  // query_result is not window.window in some cases.
  modifier_state_t mods = x11_modifier_state(data->mods.effective);

  if (data->evtype == XI_Motion) {
    // TODO: Make valuator_index names configurable?

    bool is_move = data->test_valuator(valuator_t::MOVE_X) ||
                   data->test_valuator(valuator_t::MOVE_Y);
    bool is_scroll = data->test_valuator(valuator_t::SCROLL_X) ||
                     data->test_valuator(valuator_t::SCROLL_Y);

    if (is_move) {
      static bool cursor_inside = false;

      // generate crossing events
      if (cursor_over_conky) {
        if (!cursor_inside) {
          *consumed = llua_mouse_hook(mouse_crossing_event(
              mouse_event_t::AREA_ENTER,
              data->pos_absolute - window.geometry.pos(), data->pos_absolute));
        }
        cursor_inside = true;
      } else if (cursor_inside) {
        *consumed = llua_mouse_hook(mouse_crossing_event(
            mouse_event_t::AREA_LEAVE,
            data->pos_absolute - window.geometry.pos(), data->pos_absolute));
        cursor_inside = false;
      }

      // generate movement events
      if (cursor_over_conky) {
        *consumed = llua_mouse_hook(
            mouse_move_event(data->pos, data->pos_absolute, mods));
      }
    }
    if (is_scroll && cursor_over_conky) {
      scroll_direction_t scroll_direction;
      auto vertical = data->valuator_relative_value(valuator_t::SCROLL_Y);
      double vertical_value = vertical.value_or(0.0);

      if (vertical_value != 0.0) {
        scroll_direction = vertical_value < 0.0 ? scroll_direction_t::UP
                                                : scroll_direction_t::DOWN;
      } else {
        auto horizontal = data->valuator_relative_value(valuator_t::SCROLL_X);
        double horizontal_value = horizontal.value_or(0.0);
        if (horizontal_value != 0.0) {
          scroll_direction = horizontal_value < 0.0 ? scroll_direction_t::LEFT
                                                    : scroll_direction_t::RIGHT;
        }
      }

      if (scroll_direction != scroll_direction_t::UNKNOWN) {
        *consumed = llua_mouse_hook(mouse_scroll_event(
            data->pos, data->pos_absolute, scroll_direction, mods));
      }
    }
  } else if (cursor_over_conky && (data->evtype == XI_ButtonPress ||
                                   data->evtype == XI_ButtonRelease)) {
    if (data->detail >= 4 && data->detail <= 7) {
      // Handled via motion event valuators, ignoring "backward compatibility"
      // ones.
      return true;
    }

    mouse_event_t type = mouse_event_t::PRESS;
    if (data->evtype == XI_ButtonRelease) { type = mouse_event_t::RELEASE; }

    mouse_button_t button = x11_mouse_button_code(data->detail);
    *consumed = llua_mouse_hook(
        mouse_button_event(type, data->pos, data->pos_absolute, button, mods));
  }
#endif /* BUILD_MOUSE_EVENTS */

#else /* BUILD_XINPUT */
  if (ev.type != ButtonPress && ev.type != ButtonRelease &&
      ev.type != MotionNotify)
    return false;
  if (ev.xany.window != window.window) return true;  // Skip other windows

#ifdef BUILD_MOUSE_EVENTS
  switch (ev.type) {
    case ButtonPress: {
      modifier_state_t mods = x11_modifier_state(ev.xbutton.state);
      if (ev.xbutton.button >= 4 &&
          ev.xbutton.button <= 7) {  // scroll "buttons"
        scroll_direction_t direction = x11_scroll_direction(ev.xbutton.button);
        *consumed = llua_mouse_hook(mouse_scroll_event(
            vec2i(ev.xbutton.x, ev.xbutton.y),
            vec2i(ev.xbutton.x_root, ev.xbutton.y_root), direction, mods));
      } else {
        mouse_button_t button = x11_mouse_button_code(ev.xbutton.button);
        *consumed = llua_mouse_hook(mouse_button_event(
            mouse_event_t::PRESS, vec2i(ev.xbutton.x, ev.xbutton.y),
            vec2i(ev.xbutton.x_root, ev.xbutton.y_root), button, mods));
      }
      break;
    }
    case ButtonRelease: {
      /* don't report scroll release events */
      if (ev.xbutton.button >= 4 && ev.xbutton.button <= 7) return true;

      modifier_state_t mods = x11_modifier_state(ev.xbutton.state);
      mouse_button_t button = x11_mouse_button_code(ev.xbutton.button);
      *consumed = llua_mouse_hook(mouse_button_event(
          mouse_event_t::RELEASE, vec2i(ev.xbutton.x, ev.xbutton.y),
          vec2i(ev.xbutton.x_root, ev.xbutton.y_root), button, mods));
      break;
    }
    case MotionNotify: {
      modifier_state_t mods = x11_modifier_state(ev.xmotion.state);
      *consumed = llua_mouse_hook(
          mouse_move_event(vec2i(ev.xmotion.x, ev.xmotion.y),
                           vec2i(ev.xmotion.x_root, ev.xmotion.y_root), mods));
      break;
    }
  }
#endif /* BUILD_MOUSE_EVENTS */
#endif /* BUILD_XINPUT */
#ifndef BUILD_MOUSE_EVENTS
  // always propagate mouse input if not handling mouse events
  *consumed = false;
#endif /* BUILD_MOUSE_EVENTS */

  if (!own_window.get(*state)) return true;
  switch (own_window_type.get(*state)) {
    case window_type::NORMAL:
    case window_type::UTILITY:
      // decorated normal windows always consume events
      if (!TEST_HINT(own_window_hints.get(*state), window_hints::UNDECORATED)) {
        *consumed = true;
      }
      break;
    case window_type::DESKTOP:
      // assume conky is always on bottom; nothing to propagate events to
      *consumed = true;
    default:
      break;
  }

  return true;
}

template <>
bool handle_event<x_event_handler::REPARENT>(conky::display_output_x11 *surface,
                                             Display *display, XEvent &ev,
                                             bool *consumed, void **cookie) {
  if (ev.type != ReparentNotify) return false;

  if (own_window.get(*state)) { set_transparent_background(window.window); }
  return true;
}

template <>
bool handle_event<x_event_handler::CONFIGURE>(
    conky::display_output_x11 *surface, Display *display, XEvent &ev,
    bool *consumed, void **cookie) {
  if (ev.type != ConfigureNotify) return false;

  if (own_window.get(*state)) {
    auto configure_size = vec2i(ev.xconfigure.width, ev.xconfigure.height);
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

    /* if position isn't what expected, set fixed pos
     * total_updates avoids setting fixed_pos when window
     * is set to weird locations when started */
    /* // this is broken
    if (total_updates >= 2 && !fixed_pos
        && (window.geometry.x != ev.xconfigure.x
        || window.geometry.y != ev.xconfigure.y)
        && (ev.xconfigure.x != 0
        || ev.xconfigure.y != 0)) {
      fixed_pos = 1;
    } */
  }

  return true;
}

#ifdef BUILD_MOUSE_EVENTS
template <>
bool handle_event<x_event_handler::BORDER_CROSSING>(
    conky::display_output_x11 *surface, Display *display, XEvent &ev,
    bool *consumed, void **cookie) {
  if (ev.type != EnterNotify && ev.type != LeaveNotify) return false;
  if (window.xi_opcode != 0) return true;  // handled by mouse_input already

  auto crossing_pos = vec2i(ev.xcrossing.x_root, ev.xcrossing.y_root);
  bool over_conky = window.geometry.contains(crossing_pos);

  if ((!over_conky && ev.xcrossing.type == LeaveNotify) ||
      (over_conky && ev.xcrossing.type == EnterNotify)) {
    llua_mouse_hook(mouse_crossing_event(
        ev.xcrossing.type == EnterNotify ? mouse_event_t::AREA_ENTER
                                         : mouse_event_t::AREA_LEAVE,
        vec2i(ev.xcrossing.x, ev.xcrossing.y),
        vec2i(ev.xcrossing.x_root, ev.xcrossing.y_root)));
  }
  return true;
}
#endif /* BUILD_MOUSE_EVENTS */
#endif /* OWN_WINDOW */

template <>
bool handle_event<x_event_handler::PROPERTY_NOTIFY>(
    conky::display_output_x11 *surface, Display *display, XEvent &ev,
    bool *consumed, void **cookie) {
  if (ev.type != PropertyNotify) return false;

  if (ev.xproperty.state == PropertyNewValue) {
    get_x11_desktop_info(ev.xproperty.display, ev.xproperty.atom);
  }

  if (ev.xproperty.atom == 0) return false;

  if (ev.xproperty.atom == XA_RESOURCE_MANAGER) {
    update_x11_resource_db();
    update_x11_workarea();
    screen_dpi = -1;
    update_dpi();
    return true;
  }

  if (!have_argb_visual) {
    Atom _XROOTPMAP_ID = XInternAtom(display, "_XROOTPMAP_ID", True);
    Atom _XROOTMAP_ID = XInternAtom(display, "_XROOTMAP_ID", True);
    if (ev.xproperty.atom == _XROOTPMAP_ID ||
        ev.xproperty.atom == _XROOTMAP_ID) {
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

template <>
bool handle_event<x_event_handler::EXPOSE>(conky::display_output_x11 *surface,
                                           Display *display, XEvent &ev,
                                           bool *consumed, void **cookie) {
  if (ev.type != Expose) return false;

  XRectangle r{
      .x = static_cast<short>(ev.xexpose.x),
      .y = static_cast<short>(ev.xexpose.y),
      .width = static_cast<unsigned short>(ev.xexpose.width),
      .height = static_cast<unsigned short>(ev.xexpose.height),
  };
  XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
  XSync(display, False);
  return true;
}

#ifdef BUILD_XDAMAGE
template <>
bool handle_event<x_event_handler::DAMAGE>(conky::display_output_x11 *surface,
                                           Display *display, XEvent &ev,
                                           bool *consumed, void **cookie) {
  if (ev.type != x11_stuff.event_base + XDamageNotify) return false;

  auto *dev = reinterpret_cast<XDamageNotifyEvent *>(&ev);

  XFixesSetRegion(display, x11_stuff.part, &dev->area, 1);
  XFixesUnionRegion(display, x11_stuff.region2, x11_stuff.region2,
                    x11_stuff.part);
  return true;
}
#endif /* BUILD_XDAMAGE */

/// Handles all events conky can receive.
///
/// @return true if event should move input focus to conky
bool process_event(conky::display_output_x11 *surface, Display *display,
                   XEvent ev, bool *consumed, void **cookie) {
#define HANDLE_EV(event)                                                   \
  if (handle_event<x_event_handler::event>(surface, display, ev, consumed, \
                                           cookie)) {                      \
    return true;                                                           \
  }

  HANDLE_EV(XINPUT_MOTION)
  HANDLE_EV(MOUSE_INPUT)
  HANDLE_EV(PROPERTY_NOTIFY)

  // only accept remaining events if they're sent to Conky.
  if (ev.xany.window != window.window) return false;

  HANDLE_EV(EXPOSE)
  HANDLE_EV(REPARENT)
  HANDLE_EV(CONFIGURE)
  HANDLE_EV(BORDER_CROSSING)
  HANDLE_EV(DAMAGE)

#undef HANDLE_EV

  // event not handled
  return false;
}

void process_surface_events(conky::display_output_x11 *surface,
                            Display *display) {
  int pending = XPending(display);
  if (pending == 0) return;

  DBGP2("Processing %d X11 events...", pending);

  /* handle X events */
  while (XPending(display) != 0) {
    XEvent ev;
    XNextEvent(display, &ev);

    /*
    indicates whether processed event was consumed; true by default so we
    don't propagate handled events unless they explicitly state they haven't
    been consumed.
    */
    bool consumed = true;
    void *cookie = nullptr;
    bool handled = process_event(surface, display, ev, &consumed, &cookie);

    if (!consumed) { propagate_x11_event(ev, cookie); }

    if (cookie != nullptr) { free(cookie); }
  }

  DBGP2("Done processing %d events.", pending);
}

void display_output_x11::sigterm_cleanup() {
  XDestroyRegion(x11_stuff.region);
  x11_stuff.region = nullptr;
#ifdef BUILD_XDAMAGE
  if (x11_stuff.damage) {
    XDamageDestroy(display, x11_stuff.damage);
    XFixesDestroyRegion(display, x11_stuff.region2);
    XFixesDestroyRegion(display, x11_stuff.part);
  }
#endif /* BUILD_XDAMAGE */
}

void display_output_x11::cleanup() {
  if (window_created == 1) {
    int border_total = get_border_total();

    XClearArea(display, window.window, text_start.x() - border_total,
               text_start.y() - border_total, text_size.x() + 2 * border_total,
               text_size.y() + 2 * border_total, 0);
  }
  destroy_window();
  free_fonts(utf8_mode.get(*state));
  if (x11_stuff.region != nullptr) {
    XDestroyRegion(x11_stuff.region);
    x11_stuff.region = nullptr;
  }
}

void display_output_x11::set_foreground_color(Colour c) {
  current_color = c;
#ifdef BUILD_ARGB
  if (have_argb_visual) {
    current_color.alpha = own_window_argb_value.get(*state);
  }
#endif /* BUILD_ARGB */
  XSetForeground(display, window.gc,
                 current_color.to_x11_color(display, screen, have_argb_visual));
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

    c.pixel = current_color.to_x11_color(display, screen, have_argb_visual);
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

void display_output_x11::end_draw_stuff() {
#if defined(BUILD_XDBE)
  xdbe_swap_buffers();
#else
  xpmdb_swap_buffers();
#endif
}

void display_output_x11::clear_text(int exposures) {
#ifdef BUILD_XDBE
  if (use_xdbe.get(*state)) {
    /* The swap action is XdbeBackground, which clears */
    return;
  }
#else
  if (use_xpmdb.get(*state)) {
    return;
  } else
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
    DBGP("%d >= x_fonts.size()", f);
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

      NORM_ERR("can't load Xft font '%s'", font.name.c_str());
      if ((xfont.xftfont = XftFontOpenName(display, screen, "courier-12")) !=
          nullptr) {
        continue;
      }

      CRIT_ERR("can't load Xft font '%s'", "courier-12");

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
        NORM_ERR("can't load font '%s'", font.name.c_str());
        xfont.fontset = XCreateFontSet(display, "fixed", &missing, &missingnum,
                                       &missingdrawn);
        if (xfont.fontset == nullptr) {
          CRIT_ERR("can't load font '%s'", "fixed");
        }
      }
    }
    /* load normal font */
    if ((xfont.font == nullptr) &&
        (xfont.font = XLoadQueryFont(display, font.name.c_str())) == nullptr) {
      NORM_ERR("can't load font '%s'", font.name.c_str());
      if ((xfont.font = XLoadQueryFont(display, "fixed")) == nullptr) {
        CRIT_ERR("can't load font '%s'", "fixed");
      }
    }
  }
}

}  // namespace conky
