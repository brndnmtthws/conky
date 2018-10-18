/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018 François Revol et al.
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

#include <config.h>

#ifdef BUILD_X11
#include <X11/Xutil.h>
#include "x11.h"
#ifdef BUILD_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif
#ifdef BUILD_IMLIB2
#include "imlib2.h"
#endif /* BUILD_IMLIB2 */
#ifdef BUILD_XSHAPE
#include <X11/extensions/shape.h>
#endif /* BUILD_XSHAPE */
#endif /* BUILD_X11 */

#include <iostream>
#include <sstream>
#include <unordered_map>

#include "conky.h"
#include "llua.h"
#include "display-x11.hh"
#include "x11.h"
#ifdef BUILD_X11
#include "fonts.h"
#endif

/* TODO: cleanup global namespace */
#ifdef BUILD_X11

//TODO: cleanup externs (move to conky.h ?)
#ifdef OWN_WINDOW
extern int fixed_size, fixed_pos;
#endif
extern int text_start_x, text_start_y;   /* text start position in window */
extern int text_offset_x, text_offset_y; /* offset for start position */
extern int text_width,
           text_height; /* initially 1 so no zero-sized window is created */
extern double current_update_time, next_update_time, last_update_time;
void update_text();
extern int need_to_update;
int get_border_total();
extern conky::range_config_setting<int> maximum_width;
extern long current_color;


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
  setup_fonts();
  load_fonts(utf8_mode.get(*state));
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
  }
  x11_stuff.damage =
      XDamageCreate(display, window.window, XDamageReportNonEmpty);
  x11_stuff.region2 = XFixesCreateRegionFromWindow(display, window.window, 0);
  x11_stuff.part = XFixesCreateRegionFromWindow(display, window.window, 0);
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

namespace priv {


}  // namespace priv

#ifdef BUILD_X11

display_output_x11::display_output_x11()
    : display_output_base("x11") {
  is_graphical = true;
  priority = 2;
}

bool display_output_x11::detect() {
  if (out_to_x.get(*state)) {
    std::cerr << "Display output '" << name << "' enabled in config." << std::endl;
    return true;
  }
  return false;
}

bool display_output_x11::initialize() {
  X11_create_window();

  //XXX: this was at the top of main_loop() but I don't see any reason
#ifdef BUILD_XSHAPE
  /* allow only decorated windows to be given mouse input */
  int major_version, minor_version;
  if (XShapeQueryVersion(display, &major_version, &minor_version) == 0) {
    NORM_ERR("Input shapes are not supported");
  } else {
    if (own_window.get(*state) &&
        (own_window_type.get(*state) != TYPE_NORMAL ||
         ((TEST_HINT(own_window_hints.get(*state), HINT_UNDECORATED)) !=
          0))) {
      XShapeCombineRectangles(display, window.window, ShapeInput, 0, 0,
                              nullptr, 0, ShapeSet, Unsorted);
    }
  }
#endif /* BUILD_XSHAPE */

  return true;
}

bool display_output_x11::shutdown() {
  return false;
}

bool display_output_x11::main_loop_wait(double t) {
  XFlush(display);

  /* wait for X event or timeout */

  if (XPending(display) == 0) {
    fd_set fdsr;
    struct timeval tv {};
    int s;
    //t = next_update_time - get_time();

    t = std::min(std::max(t, 0.0), active_update_interval());

    tv.tv_sec = static_cast<long>(t);
    tv.tv_usec = static_cast<long>(t * 1000000) % 1000000;
    FD_ZERO(&fdsr);
    FD_SET(ConnectionNumber(display), &fdsr);

    s = select(ConnectionNumber(display) + 1, &fdsr, nullptr, nullptr, &tv);
    if (s == -1) {
      if (errno != EINTR) {
        NORM_ERR("can't select(): %s", strerror(errno));
      }
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
              XCreatePixmap(display, window.window, window.width,
                            window.height, DefaultDepth(display, screen));

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

        fprintf(stderr, "%s", _(PACKAGE_NAME ": defining struts\n"));
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

  /* handle X events */
  while (XPending(display) != 0) {
    XEvent ev;

    XNextEvent(display, &ev);
    switch (ev.type) {
      case Expose: {
        XRectangle r;
        r.x = ev.xexpose.x;
        r.y = ev.xexpose.y;
        r.width = ev.xexpose.width;
        r.height = ev.xexpose.height;
        XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
        break;
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
            draw_stuff();
            next_update_time = get_time();
            need_to_update = 1;
          }
#ifdef USE_ARGB
        }
#endif
        break;
      }

#ifdef OWN_WINDOW
      case ReparentNotify:
        /* make background transparent */
        if (own_window.get(*state)) {
          set_transparent_background(window.window);
        }
        break;

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
              if (XGetWindowAttributes(display, window.window, &attrs) !=
                  0) {
                window.width = attrs.width;
                window.height = attrs.height;
              }
            }

            int border_total = get_border_total();

            text_width = window.width - 2 * border_total;
            text_height = window.height - 2 * border_total;
            int mw = maximum_width.get(*state);
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
        break;

      case ButtonPress:
        if (own_window.get(*state)) {
          /* if an ordinary window with decorations */
          if ((own_window_type.get(*state) == TYPE_NORMAL &&
               not TEST_HINT(own_window_hints.get(*state),
                             HINT_UNDECORATED)) ||
              own_window_type.get(*state) == TYPE_DESKTOP) {
            /* allow conky to hold input focus. */
            break;
          }
          /* forward the click to the desktop window */
          XUngrabPointer(display, ev.xbutton.time);
          ev.xbutton.window = window.desktop;
          ev.xbutton.x = ev.xbutton.x_root;
          ev.xbutton.y = ev.xbutton.y_root;
          XSendEvent(display, ev.xbutton.window, False, ButtonPressMask,
                     &ev);
          XSetInputFocus(display, ev.xbutton.window, RevertToParent,
                         ev.xbutton.time);
        }
        break;

      case ButtonRelease:
        if (own_window.get(*state)) {
          /* if an ordinary window with decorations */
          if ((own_window_type.get(*state) == TYPE_NORMAL) &&
              not TEST_HINT(own_window_hints.get(*state),
                            HINT_UNDECORATED)) {
            /* allow conky to hold input focus. */
            break;
          }
          /* forward the release to the desktop window */
          ev.xbutton.window = window.desktop;
          ev.xbutton.x = ev.xbutton.x_root;
          ev.xbutton.y = ev.xbutton.y_root;
          XSendEvent(display, ev.xbutton.window, False, ButtonReleaseMask,
                     &ev);
        }
        break;

#endif

      default:
#ifdef BUILD_XDAMAGE
        if (ev.type == x11_stuff.event_base + XDamageNotify) {
          auto *dev = reinterpret_cast<XDamageNotifyEvent *>(&ev);

          XFixesSetRegion(display, x11_stuff.part, &dev->area, 1);
          XFixesUnionRegion(display, x11_stuff.region2, x11_stuff.region2,
                            x11_stuff.part);
        }
#endif /* BUILD_XDAMAGE */
        break;
    }
  }

#ifdef BUILD_XDAMAGE
  XDamageSubtract(display, x11_stuff.damage, x11_stuff.region2, None);
  XFixesSetRegion(display, x11_stuff.region2, nullptr, 0);
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
  XDamageDestroy(display, x11_stuff.damage);
  XFixesDestroyRegion(display, x11_stuff.region2);
  XFixesDestroyRegion(display, x11_stuff.part);
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


void display_output_x11::set_foreground_color(long c) {
#ifdef BUILD_ARGB
  if (have_argb_visual) {
    current_color = c | (own_window_argb_value.get(*state) << 24);
  } else {
#endif /* BUILD_ARGB */
    current_color = c;
#ifdef BUILD_ARGB
  }
#endif /* BUILD_ARGB */
  XSetForeground(display, window.gc, current_color);
}


int display_output_x11::calc_text_width(const char *s) {
  size_t slen = strlen(s);
#ifdef BUILD_XFT
  if (use_xft.get(*state)) {
    XGlyphInfo gi;

    if (utf8_mode.get(*state)) {
      XftTextExtentsUtf8(display, fonts[selected_font].xftfont,
                         reinterpret_cast<const FcChar8 *>(s), slen, &gi);
    } else {
      XftTextExtents8(display, fonts[selected_font].xftfont,
                      reinterpret_cast<const FcChar8 *>(s), slen, &gi);
    }
    return gi.xOff;
  }
#endif /* BUILD_XFT */

  return XTextWidth(fonts[selected_font].font, s, slen);
}

void display_output_x11::draw_string_at(int x, int y, const char *s, int w) {
#ifdef BUILD_XFT
  if (use_xft.get(*state)) {
    XColor c;
    XftColor c2;

    c.pixel = current_color;
    // query color on custom colormap
    XQueryColor(display, window.colourmap, &c);

    c2.pixel = c.pixel;
    c2.color.red = c.red;
    c2.color.green = c.green;
    c2.color.blue = c.blue;
    c2.color.alpha = fonts[selected_font].font_alpha;
    if (utf8_mode.get(*state)) {
      XftDrawStringUtf8(window.xftdraw, &c2, fonts[selected_font].xftfont,
                        x, y,
                        reinterpret_cast<const XftChar8 *>(s), w);
    } else {
      XftDrawString8(window.xftdraw, &c2, fonts[selected_font].xftfont,
                     x, y,
                     reinterpret_cast<const XftChar8 *>(s), w);
    }
  } else
#endif
  {
    if (utf8_mode.get(*state)) {
      Xutf8DrawString(display, window.drawable, fonts[selected_font].fontset,
                      window.gc, x, y,
                      s, w);
    } else {
      XDrawString(display, window.drawable, window.gc, x,
                  y, s, w);
    }
  }
}

void display_output_x11::set_line_style(int w, bool solid) {
  XSetLineAttributes(display, window.gc, w,
                     solid ? LineSolid : LineOnOffDash, CapButt, JoinMiter);
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
  XMoveWindow(display, window.window, window.x, window.y);
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

void display_output_x11::load_fonts(bool utf8) {
  ::load_fonts(utf8);
}

#endif /* BUILD_X11 */

}  // namespace conky

