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

#include <X11/extensions/XI2.h>
#include <config.h>

#ifdef BUILD_X11
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
#include "fonts.h"
#ifdef BUILD_IMLIB2
#include "conky-imlib2.h"
#endif /* BUILD_IMLIB2 */
#ifdef BUILD_MOUSE_EVENTS
#include "mouse-events.h"
#ifdef BUILD_XINPUT
#include <X11/extensions/XInput2.h>
#endif /* BUILD_XINPUT */
#endif /* BUILD_MOUSE_EVENTS */
#endif /* BUILD_X11 */

#include <cstdint>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "colours.h"
#include "conky.h"
#include "display-x11.hh"
#include "gui.h"
#include "llua.h"

/* TODO: cleanup global namespace */
#ifdef BUILD_X11

#include "logging.h"
#include "x11.h"

// TODO: cleanup externs (move to conky.h ?)
#ifdef OWN_WINDOW
extern int fixed_size, fixed_pos;
#endif                                   /* OWN_WINDOW */
extern int text_start_x, text_start_y;   /* text start position in window */
extern int text_offset_x, text_offset_y; /* offset for start position */
extern int text_width,
    text_height; /* initially 1 so no zero-sized window is created */
extern double current_update_time, next_update_time, last_update_time;
void update_text();
extern int need_to_update;
int get_border_total();
extern conky::range_config_setting<int> maximum_width;
extern Colour current_color;
#ifdef BUILD_XFT
static int xft_dpi = -1;
#endif /* BUILD_XFT */

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

static void X11_create_window() {
  if (!window.window) { return; }
  setup_fonts();
  load_fonts(utf8_mode.get(*state));
#ifdef BUILD_XFT
  if (use_xft.get(*state)) {
    auto dpi = XGetDefault(display, "Xft", "dpi");
    if (dpi) { xft_dpi = strtol(dpi, nullptr, 10); }
  }
#endif                /* BUILD_XFT */
  update_text_area(); /* to position text/window on screen */

#ifdef OWN_WINDOW
  if (own_window.get(*state)) {
    if (fixed_pos == 0) {
      XMoveWindow(display, window.window, window.x, window.y);
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

#endif /* BUILD_X11 */

namespace conky {
namespace {

#ifdef BUILD_X11
conky::display_output_x11 x11_output;
#else
conky::disabled_display_output x11_output_disabled("x11", "BUILD_X11");
#endif

}  // namespace
extern void init_x11_output() {}

namespace priv {}  // namespace priv

#ifdef BUILD_X11

display_output_x11::display_output_x11() : display_output_base("x11") {
  is_graphical = true;
  priority = 2;
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

  if (need_to_update != 0) {
#ifdef OWN_WINDOW
    int wx = window.x, wy = window.y;
#endif

    need_to_update = 0;
    selected_font = 0;
    update_text_area();

#ifdef OWN_WINDOW
    if (own_window.get(*state)) {
      int changed = 0;
      int border_total = get_border_total();

      /* resize window if it isn't right size */
      if ((fixed_size == 0) &&
          (text_width + 2 * border_total != window.width ||
           text_height + 2 * border_total != window.height)) {
        window.width = text_width + 2 * border_total;
        window.height = text_height + 2 * border_total;
        draw_stuff(); /* redraw everything in our newly sized window */
        XResizeWindow(display, window.window, window.width,
                      window.height); /* resize window */
        set_transparent_background(window.window);
#ifdef BUILD_XDBE
        /* swap buffers */
        xdbe_swap_buffers();
#else
        if (use_xpmdb.get(*state)) {
          XFreePixmap(display, window.back_buffer);
          window.back_buffer =
              XCreatePixmap(display, window.window, window.width, window.height,
                            DefaultDepth(display, screen));

          if (window.back_buffer != None) {
            window.drawable = window.back_buffer;
          } else {
            // this is probably reallllly bad
            NORM_ERR("Failed to allocate back buffer");
          }
          XSetForeground(display, window.gc, 0);
          XFillRectangle(display, window.drawable, window.gc, 0, 0,
                         window.width, window.height);
        }
#endif

        changed++;
        /* update lua window globals */
        llua_update_window_table(text_start_x, text_start_y, text_width,
                                 text_height);
      }

      /* move window if it isn't in right position */
      if ((fixed_pos == 0) && (window.x != wx || window.y != wy)) {
        XMoveWindow(display, window.window, window.x, window.y);
        changed++;
      }

      /* update struts */
      if ((changed != 0) && own_window_type.get(*state) == TYPE_PANEL) {
        int sidenum = -1;

        DBGP("%s", _(PACKAGE_NAME ": defining struts\n"));
        fflush(stderr);

        switch (text_alignment.get(*state)) {
          case TOP_LEFT:
          case TOP_RIGHT:
          case TOP_MIDDLE: {
            sidenum = 2;
            break;
          }
          case BOTTOM_LEFT:
          case BOTTOM_RIGHT:
          case BOTTOM_MIDDLE: {
            sidenum = 3;
            break;
          }
          case MIDDLE_LEFT: {
            sidenum = 0;
            break;
          }
          case MIDDLE_RIGHT: {
            sidenum = 1;
            break;
          }

          case NONE:
          case MIDDLE_MIDDLE: /* XXX What about these? */;
        }

        set_struts(sidenum);
      }
    }
#endif

    clear_text(1);

#if defined(BUILD_XDBE)
    if (use_xdbe.get(*state)) {
#else
    if (use_xpmdb.get(*state)) {
#endif
      XRectangle r;
      int border_total = get_border_total();

      r.x = text_start_x - border_total;
      r.y = text_start_y - border_total;
      r.width = text_width + 2 * border_total;
      r.height = text_height + 2 * border_total;
      XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
    }
  }

  DBGP2("Processing %d X11 events...", XPending(display));
  /* handle X events */
  while (XPending(display) != 0) {
    XEvent ev;
    /* indicates whether processed event was consumed */
    bool consumed = false;

    XNextEvent(display, &ev);

#if defined(OWN_WINDOW) && defined(BUILD_MOUSE_EVENTS) && defined(BUILD_XINPUT)
    // no need to check whether these events have been consumed because
    // they're global and shouldn't be propagated
    if (ev.type == GenericEvent && ev.xcookie.extension == window.xi_opcode) {
      if (!XGetEventData(display, &ev.xcookie)) {
        NORM_ERR("unable to get XInput event data");
        continue;
      }

      auto *data = reinterpret_cast<XIDeviceEvent *>(ev.xcookie.data);

      // the only way to differentiate between a scroll and move event is
      // though valuators - move has first 2 set, other axis movements have
      // other.
      bool is_cursor_move =
          data->valuators.mask_len >= 1 &&
          (data->valuators.mask[0] & 3) == data->valuators.mask[0];
      for (std::size_t i = 1; i < data->valuators.mask_len; i++) {
        if (data->valuators.mask[i] != 0) {
          is_cursor_move = false;
          break;
        }
      }

      if (data->evtype == XI_Motion && is_cursor_move) {
        Window query_result =
            query_x11_window_at_pos(display, data->root_x, data->root_y);
        // query_result is not window.window in some cases.
        query_result = query_x11_last_descendant(display, query_result);

        static bool cursor_inside = false;

        // - over conky window
        // - conky has now window, over desktop and within conky region
        bool cursor_over_conky = query_result == window.window &&
                                 (window.window != 0u ||
                                  (data->root_x >= window.x &&
                                   data->root_x < (window.x + window.width) &&
                                   data->root_y >= window.y &&
                                   data->root_y < (window.y + window.height)));
        if (cursor_over_conky) {
          if (!cursor_inside) {
            llua_mouse_hook(mouse_crossing_event(
                mouse_event_t::AREA_ENTER, data->root_x - window.x,
                data->root_y - window.x, data->root_x, data->root_y));
          }
          cursor_inside = true;
        } else if (cursor_inside) {
          llua_mouse_hook(mouse_crossing_event(
              mouse_event_t::AREA_LEAVE, data->root_x - window.x,
              data->root_y - window.x, data->root_x, data->root_y));
          cursor_inside = false;
        }
      }
      XFreeEventData(display, &ev.xcookie);
      continue;
    }
#endif /* BUILD_MOUSE_EVENTS && BUILD_XINPUT */

    // Any of the remaining events apply to conky window
    if (ev.xany.window != window.window && ev.type != PropertyNotify) continue;
    switch (ev.type) {
      case Expose: {
        XRectangle r;
        r.x = ev.xexpose.x;
        r.y = ev.xexpose.y;
        r.width = ev.xexpose.width;
        r.height = ev.xexpose.height;
        XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
        XSync(display, False);

        continue;
      }

      case PropertyNotify: {
        if (ev.xproperty.state == PropertyNewValue) {
          get_x11_desktop_info(ev.xproperty.display, ev.xproperty.atom);
        }
#ifdef USE_ARGB
        if (!have_argb_visual) {
#endif
          if (ev.xproperty.atom == ATOM(_XROOTPMAP_ID) ||
              ev.xproperty.atom == ATOM(_XROOTMAP_ID)) {
            if (forced_redraw.get(*state)) {
              draw_stuff();
              next_update_time = get_time();
              need_to_update = 1;
            }
          }
#ifdef USE_ARGB
        }
#endif
        continue;
      }

#ifdef OWN_WINDOW
      case ReparentNotify:
        /* make background transparent */
        if (own_window.get(*state)) {
          set_transparent_background(window.window);
        }
        continue;

      case ConfigureNotify:
        if (own_window.get(*state)) {
          /* if window size isn't what expected, set fixed size */
          if (ev.xconfigure.width != window.width ||
              ev.xconfigure.height != window.height) {
            if (window.width != 0 && window.height != 0) { fixed_size = 1; }

            /* clear old stuff before screwing up
             * size and pos */
            clear_text(1);

            {
              XWindowAttributes attrs;
              if (XGetWindowAttributes(display, window.window, &attrs) != 0) {
                window.width = attrs.width;
                window.height = attrs.height;
              }
            }

            int border_total = get_border_total();

            text_width = window.width - 2 * border_total;
            text_height = window.height - 2 * border_total;
            int mw = this->dpi_scale(maximum_width.get(*state));
            if (text_width > mw && mw > 0) { text_width = mw; }
          }

          /* if position isn't what expected, set fixed pos
           * total_updates avoids setting fixed_pos when window
           * is set to weird locations when started */
          /* // this is broken
          if (total_updates >= 2 && !fixed_pos
              && (window.x != ev.xconfigure.x
              || window.y != ev.xconfigure.y)
              && (ev.xconfigure.x != 0
              || ev.xconfigure.y != 0)) {
            fixed_pos = 1;
          } */
        }
        continue;

      case ButtonPress:
#ifdef BUILD_MOUSE_EVENTS
      {
        modifier_state_t mods = x11_modifier_state(ev.xbutton.state);
        if (4 <= ev.xbutton.button && ev.xbutton.button <= 7) {
          scroll_direction_t direction =
              x11_scroll_direction(ev.xbutton.button);
          consumed = llua_mouse_hook(
              mouse_scroll_event(ev.xbutton.x, ev.xbutton.y, ev.xbutton.x_root,
                                 ev.xbutton.y_root, direction, mods));
        } else {
          mouse_button_t button = x11_mouse_button_code(ev.xbutton.button);
          consumed = llua_mouse_hook(mouse_button_event(
              mouse_event_t::MOUSE_PRESS, ev.xbutton.x, ev.xbutton.y,
              ev.xbutton.x_root, ev.xbutton.y_root, button, mods));
        }
      }
#endif /* BUILD_MOUSE_EVENTS */
        if (own_window.get(*state)) {
          /* if an ordinary window with decorations */
          if ((own_window_type.get(*state) == TYPE_NORMAL &&
               !TEST_HINT(own_window_hints.get(*state), HINT_UNDECORATED)) ||
              own_window_type.get(*state) == TYPE_DESKTOP) {
            /* allow conky to hold input focus. */
            break;
          }
        }
        break;

      case ButtonRelease:
#ifdef BUILD_MOUSE_EVENTS
        /* don't report scroll release events */
        if (4 > ev.xbutton.button || ev.xbutton.button > 7) {
          modifier_state_t mods = x11_modifier_state(ev.xbutton.state);
          mouse_button_t button = x11_mouse_button_code(ev.xbutton.button);
          consumed = llua_mouse_hook(mouse_button_event(
              mouse_event_t::MOUSE_RELEASE, ev.xbutton.x, ev.xbutton.y,
              ev.xbutton.x_root, ev.xbutton.y_root, button, mods));
        }
#endif /* BUILD_MOUSE_EVENTS */
        if (own_window.get(*state)) {
          /* if an ordinary window with decorations */
          if ((own_window_type.get(*state) == TYPE_NORMAL) &&
              !TEST_HINT(own_window_hints.get(*state), HINT_UNDECORATED)) {
            /* allow conky to hold input focus. */
            break;
          }
        }
        break;
#ifdef BUILD_MOUSE_EVENTS
      /*
      windows below are notified for the following events as well;
      can't forward the event without filtering XQueryTree output.
      */
      case MotionNotify: {
        modifier_state_t mods = x11_modifier_state(ev.xmotion.state);
        consumed = llua_mouse_hook(mouse_move_event(ev.xmotion.x, ev.xmotion.y,
                                                    ev.xmotion.x_root,
                                                    ev.xmotion.y_root, mods));
      } break;
      case LeaveNotify:
      case EnterNotify:
        if (window.xi_opcode == 0) {
          bool not_over_conky =
              ev.xcrossing.x_root <= window.x ||
              ev.xcrossing.y_root <= window.y ||
              ev.xcrossing.x_root >= window.x + window.width ||
              ev.xcrossing.y_root >= window.y + window.height;

          if ((not_over_conky && ev.xcrossing.type == LeaveNotify) ||
              (!not_over_conky && ev.xcrossing.type == EnterNotify)) {
            llua_mouse_hook(mouse_crossing_event(
                ev.xcrossing.type == EnterNotify ? mouse_event_t::AREA_ENTER
                                                 : mouse_event_t::AREA_LEAVE,
                ev.xcrossing.x, ev.xcrossing.y, ev.xcrossing.x_root,
                ev.xcrossing.y_root));
          }
        }
        // can't propagate these events in a way that makes sense for desktop
        continue;
#endif /* BUILD_MOUSE_EVENTS */
#endif /* OWN_WINDOW */
      default:
#ifdef BUILD_XDAMAGE
        if (ev.type == x11_stuff.event_base + XDamageNotify) {
          auto *dev = reinterpret_cast<XDamageNotifyEvent *>(&ev);

          XFixesSetRegion(display, x11_stuff.part, &dev->area, 1);
          XFixesUnionRegion(display, x11_stuff.region2, x11_stuff.region2,
                            x11_stuff.part);
          continue;  // TODO: Propagate damage
        }
#endif /* BUILD_XDAMAGE */
        break;
    }

    if (!consumed) {
      propagate_x11_event(ev);
    } else {
      InputEvent *i_ev = xev_as_input_event(ev);
      if (i_ev != nullptr) {
        XSetInputFocus(display, window.window, RevertToParent,
                       i_ev->common.time);
      }
    }
  }
  DBGP2("Done with events!");

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
      XRectangle r;
      int border_total = get_border_total();

      r.x = text_start_x - border_total;
      r.y = text_start_y - border_total;
      r.width = text_width + 2 * border_total;
      r.height = text_height + 2 * border_total;
      XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
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

    XClearArea(display, window.window, text_start_x - border_total,
               text_start_y - border_total, text_width + 2 * border_total,
               text_height + 2 * border_total, 0);
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
                 current_color.to_x11_color(display, screen));
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

    c.pixel = current_color.to_x11_color(display, screen);
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
  window.x = x;
  window.y = y;
  XMoveWindow(display, window.window, x, y);
}

int display_output_x11::dpi_scale(int value) {
#if defined(BUILD_XFT)
  if (use_xft.get(*state) && xft_dpi > 0) {
    return (value * xft_dpi + (value > 0 ? 48 : -48)) / 96;
  } else {
    return value;
  }
#else  /* defined(BUILD_XFT) */
  return value;
#endif /* defined(BUILD_XFT) */
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

    XClearArea(display, window.window, text_start_x - border_total,
               text_start_y - border_total, text_width + 2 * border_total,
               text_height + 2 * border_total, exposures != 0 ? True : 0);
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

#endif /* BUILD_X11 */

}  // namespace conky
