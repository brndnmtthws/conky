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

#include "x11.h"

#include <X11/X.h>
#include <sys/types.h>
#include "common.h"
#include "config.h"
#include "conky.h"
#include "gui.h"
#include "logging.h"

#ifdef BUILD_XINPUT
#include "mouse-events.h"

#include <vector>
#endif

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <string>

// #ifndef OWN_WINDOW
// #include <iostream>
// #endif

extern "C" {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#pragma GCC diagnostic ignored "-Wregister"
#include <X11/XKBlib.h>
#pragma GCC diagnostic pop
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>

#ifdef BUILD_IMLIB2
#include "conky-imlib2.h"
#endif /* BUILD_IMLIB2 */
#ifdef BUILD_XFT
#include <X11/Xft/Xft.h>
#endif
#ifdef BUILD_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#ifdef BUILD_XSHAPE
#include <X11/extensions/shape.h>
#endif /* BUILD_XSHAPE */
#ifdef BUILD_XFIXES
#include <X11/extensions/Xfixes.h>
#endif /* BUILD_XFIXES */
#ifdef BUILD_XINPUT
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#endif /* BUILD_XINPUT */
#ifdef HAVE_XCB_ERRORS
#include <xcb/xcb.h>
#include <xcb/xcb_errors.h>
#endif
}

/* some basic X11 stuff */
Display *display = nullptr;

#ifdef HAVE_XCB_ERRORS
xcb_connection_t *xcb_connection;
xcb_errors_context_t *xcb_errors_ctx;
#endif

/* Window stuff */
struct conky_x11_window window;

bool have_argb_visual = false;

/* local prototypes */
static void update_workarea();
static Window find_desktop_window(Window *p_root, Window *p_desktop);
static Window find_subwindow(Window win, int w, int h);

/* WARNING, this type not in Xlib spec */
static int x11_error_handler(Display *d, XErrorEvent *err) {
  char *error_name = nullptr;
  bool name_allocated = false;

  char *code_description = nullptr;
  bool code_allocated = false;

#ifdef HAVE_XCB_ERRORS
  if (xcb_errors_ctx != nullptr) {
    const char *extension;
    const char *base_name = xcb_errors_get_name_for_error(
        xcb_errors_ctx, err->error_code, &extension);
    if (extension != nullptr) {
      const std::size_t size = strlen(base_name) + strlen(extension) + 4;
      error_name = new char[size];
      snprintf(error_name, size, "%s (%s)", base_name, extension);
      name_allocated = true;
    } else {
      error_name = const_cast<char *>(base_name);
    }

    const char *major =
        xcb_errors_get_name_for_major_code(xcb_errors_ctx, err->request_code);
    const char *minor = xcb_errors_get_name_for_minor_code(
        xcb_errors_ctx, err->request_code, err->minor_code);
    if (minor != nullptr) {
      const std::size_t size = strlen(major) + strlen(minor) + 4;
      code_description = new char[size];
      snprintf(code_description, size, "%s - %s", major, minor);
      code_allocated = true;
    } else {
      code_description = const_cast<char *>(major);
    }
  }
#endif

  if (error_name == nullptr) {
    if (err->error_code > 0 && err->error_code < 17) {
      static std::array<std::string, 17> NAMES = {
          "request", "value",         "window",    "pixmap",    "atom",
          "cursor",  "font",          "match",     "drawable",  "access",
          "alloc",   "colormap",      "G context", "ID choice", "name",
          "length",  "implementation"};
      error_name = const_cast<char *>(NAMES[err->error_code].c_str());
    } else {
      static char code_name_buffer[5];
      error_name = reinterpret_cast<char *>(&code_name_buffer);
      snprintf(error_name, 4, "%d", err->error_code);
    }
  }
  if (code_description == nullptr) {
    const std::size_t size = 37;
    code_description = new char[size];
    snprintf(code_description, size, "error code: [major: %i, minor: %i]",
             err->request_code, err->minor_code);
    code_allocated = true;
  }

  DBGP(
      "X %s Error:\n"
      "Display: %lx, XID: %li, Serial: %lu\n"
      "%s",
      error_name, reinterpret_cast<uint64_t>(err->display),
      static_cast<int64_t>(err->resourceid), err->serial, code_description);

  if (name_allocated) delete[] error_name;
  if (code_allocated) delete[] code_description;

  return 0;
}

__attribute__((noreturn)) static int x11_ioerror_handler(Display *d) {
  CRIT_ERR("X IO Error: Display %lx\n", reinterpret_cast<uint64_t>(d));
}

/// @brief Function to get virtual root windows of screen.
///
/// Some WMs (swm, tvtwm, amiwm, enlightenment, etc.) use virtual roots to
/// manage workspaces. These are direct descendants of root and WMs reparent all
/// children to them.
///
/// @param screen screen to get the (current) virtual root of @return the
/// virtual root window of the screen
static Window VRootWindowOfScreen(Screen *screen) {
  Window root = screen->root;
  Display *dpy = screen->display;

  Window rootReturn, parentReturn, *children;
  unsigned int numChildren;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytesafter;

  /* go look for a virtual root */
  Atom __SWM_VROOT = ATOM(__SWM_VROOT);
  if (XQueryTree(dpy, root, &rootReturn, &parentReturn, &children,
                 &numChildren)) {
    for (int i = 0; i < numChildren; i++) {
      Window *newRoot = None;

      if (XGetWindowProperty(
              dpy, children[i], __SWM_VROOT, 0, 1, False, XA_WINDOW,
              &actual_type, &actual_format, &nitems, &bytesafter,
              reinterpret_cast<unsigned char **>(&newRoot)) == Success &&
          newRoot != None) {
        root = *newRoot;
        break;
      }
    }
    if (children) XFree((char *)children);
  }

  return root;
}
inline Window VRootWindow(Display *display, int screen) {
  return VRootWindowOfScreen(ScreenOfDisplay(display, screen));
}
inline Window DefaultVRootWindow(Display *display) {
  return VRootWindowOfScreen(DefaultScreenOfDisplay(display));
}

/* X11 initializer */
void init_x11() {
  DBGP("enter init_x11()");
  if (display == nullptr) {
    const std::string &dispstr = display_name.get(*state);
    // passing nullptr to XOpenDisplay should open the default display
    const char *disp = static_cast<unsigned int>(!dispstr.empty()) != 0u
                           ? dispstr.c_str()
                           : nullptr;
    if ((display = XOpenDisplay(disp)) == nullptr) {
      std::string err =
          std::string("can't open display: ") + XDisplayName(disp);
#ifdef BUILD_WAYLAND
      NORM_ERR(err.c_str());
      return;
#else  /* BUILD_WAYLAND */
      throw std::runtime_error(err);
#endif /* BUILD_WAYLAND */
    }
  }

  info.x11.monitor.number = 1;
  info.x11.monitor.current = 0;
  info.x11.desktop.current = 1;
  info.x11.desktop.number = 1;
  info.x11.desktop.all_names.clear();
  info.x11.desktop.name.clear();

  screen = DefaultScreen(display);
  display_width = DisplayWidth(display, screen);
  display_height = DisplayHeight(display, screen);

  get_x11_desktop_info(display, 0);

  update_workarea();

#ifdef HAVE_XCB_ERRORS
  auto connection = xcb_connect(NULL, NULL);
  if (!xcb_connection_has_error(connection)) {
    if (xcb_errors_context_new(connection, &xcb_errors_ctx) != Success) {
      xcb_errors_ctx = nullptr;
    }
  }
#endif /* HAVE_XCB_ERRORS */

  /* WARNING, this type not in Xlib spec */
  XSetErrorHandler(&x11_error_handler);
  XSetIOErrorHandler(&x11_ioerror_handler);

  DBGP("leave init_x11()");
}

void deinit_x11() {
  if (display) {
    DBGP("deinit_x11()");
    XCloseDisplay(display);
    display = nullptr;
  }
}

static void update_workarea() {
  /* default work area is display */
  workarea[0] = 0;
  workarea[1] = 0;
  workarea[2] = display_width;
  workarea[3] = display_height;

#ifdef BUILD_XINERAMA
  /* if xinerama is being used, adjust workarea to the head's area */
  int useless1, useless2;
  if (XineramaQueryExtension(display, &useless1, &useless2) == 0) {
    return; /* doesn't even have xinerama */
  }

  if (XineramaIsActive(display) == 0) {
    return; /* has xinerama but isn't using it */
  }

  int heads = 0;
  XineramaScreenInfo *si = XineramaQueryScreens(display, &heads);
  if (si == nullptr) {
    NORM_ERR(
        "warning: XineramaQueryScreen returned nullptr, ignoring head "
        "settings");
    return; /* queryscreens failed? */
  }

  int i = head_index.get(*state);
  if (i < 0 || i >= heads) {
    NORM_ERR("warning: invalid head index, ignoring head settings");
    return;
  }

  XineramaScreenInfo *ps = &si[i];
  workarea[0] = ps->x_org;
  workarea[1] = ps->y_org;
  workarea[2] = workarea[0] + ps->width;
  workarea[3] = workarea[1] + ps->height;
  XFree(si);

  DBGP("Fixed xinerama area to: %d %d %d %d", workarea[0], workarea[1],
       workarea[2], workarea[3]);
#endif
}

/* Find root window and desktop window.
 * Return desktop window on success,
 * and set root and desktop byref return values.
 * Return 0 on failure. */
static Window find_desktop_window(Window root) {
  Window desktop = root;

  /* get subwindows from root */
  desktop = find_subwindow(root, -1, -1);
  update_workarea();
  desktop = find_subwindow(desktop, workarea[2], workarea[3]);

  if (desktop != root) {
    NORM_ERR("desktop window (0x%lx) is subwindow of root window (0x%lx)",
             desktop, root);
  } else {
    NORM_ERR("desktop window (0x%lx) is root window", desktop);
  }
  return desktop;
}

#ifdef OWN_WINDOW
#ifdef BUILD_ARGB
namespace {
/* helper function for set_transparent_background() */
void do_set_background(Window win, uint8_t alpha) {
  Colour colour = background_colour.get(*state);
  colour.alpha = alpha;
  unsigned long xcolor =
      colour.to_x11_color(display, screen, have_argb_visual, true);
  XSetWindowBackground(display, win, xcolor);
}
}  // namespace
#endif /* BUILD_ARGB */

/* if no argb visual is configured sets background to ParentRelative for the
   Window and all parents, else real transparency is used */
void set_transparent_background(Window win) {
#ifdef BUILD_ARGB
  if (have_argb_visual) {
    // real transparency
    do_set_background(win, set_transparent.get(*state)
                               ? 0
                               : own_window_argb_value.get(*state));
    return;
  }
#endif /* BUILD_ARGB */

  // pseudo transparency
  if (set_transparent.get(*state)) {
    Window parent = win;
    unsigned int i;

    for (i = 0; i < 50 && parent != RootWindow(display, screen); i++) {
      Window r, *children;
      unsigned int n;

      XSetWindowBackgroundPixmap(display, parent, ParentRelative);

      XQueryTree(display, parent, &r, &parent, &children, &n);
      XFree(children);
    }
    return;
  }

#ifdef BUILD_ARGB
  do_set_background(win, 0);
#endif /* BUILD_ARGB */
}
#endif /* OWN_WINDOW */

#ifdef BUILD_ARGB
static int get_argb_visual(Visual **visual, int *depth) {
  /* code from gtk project, gdk_screen_get_rgba_visual */
  XVisualInfo visual_template;
  XVisualInfo *visual_list;
  int nxvisuals = 0, i;

  visual_template.screen = screen;
  visual_list =
      XGetVisualInfo(display, VisualScreenMask, &visual_template, &nxvisuals);
  for (i = 0; i < nxvisuals; i++) {
    if (visual_list[i].depth == 32 && (visual_list[i].red_mask == 0xff0000 &&
                                       visual_list[i].green_mask == 0x00ff00 &&
                                       visual_list[i].blue_mask == 0x0000ff)) {
      *visual = visual_list[i].visual;
      *depth = visual_list[i].depth;
      DBGP("Found ARGB Visual");
      XFree(visual_list);
      return 1;
    }
  }

  // no argb visual available
  DBGP("No ARGB Visual found");
  XFree(visual_list);

  return 0;
}
#endif /* BUILD_ARGB */

void destroy_window() {
#ifdef BUILD_XFT
  if (window.xftdraw != nullptr) { XftDrawDestroy(window.xftdraw); }
#endif /* BUILD_XFT */
  if (window.gc != nullptr) { XFreeGC(display, window.gc); }
  memset(&window, 0, sizeof(struct conky_x11_window));
}

void x11_init_window(lua::state &l, bool own) {
  DBGP("enter x11_init_window()");
  // own is unused if OWN_WINDOW is not defined
  (void)own;

  window.root = VRootWindow(display, screen);
  if (window.root == None) {
    DBGP2("no desktop window found");
    return;
  }
  window.desktop = find_desktop_window(window.root);

  window.visual = DefaultVisual(display, screen);
  window.colourmap = DefaultColormap(display, screen);

#ifdef OWN_WINDOW
  if (own) {
    int depth = 0, flags = CWOverrideRedirect | CWBackingStore;
    Visual *visual = nullptr;

    depth = CopyFromParent;
    visual = CopyFromParent;
#ifdef BUILD_ARGB
    if (use_argb_visual.get(l) && (get_argb_visual(&visual, &depth) != 0)) {
      have_argb_visual = true;
      window.visual = visual;
      window.colourmap = XCreateColormap(display, DefaultRootWindow(display),
                                         window.visual, AllocNone);
    }
#endif /* BUILD_ARGB */

    int b = border_inner_margin.get(l) + border_width.get(l) +
            border_outer_margin.get(l);

    /* Sanity check to avoid making an invalid 0x0 window */
    if (b == 0) { b = 1; }

    XClassHint classHint;

    // class_name must be a named local variable, so that c_str() remains
    // valid until we call XmbSetWMProperties() or XSetClassHint. We use
    // const_cast because, for whatever reason, res_name is not declared as
    // const char *. XmbSetWMProperties hopefully doesn't modify the value
    // (hell, even their own example app assigns a literal string constant to
    // the field)
    const std::string &class_name = own_window_class.get(l);

    classHint.res_name = const_cast<char *>(class_name.c_str());
    classHint.res_class = classHint.res_name;

    if (own_window_type.get(l) == window_type::OVERRIDE) {
      /* An override_redirect True window.
       * No WM hints or button processing needed. */
      XSetWindowAttributes attrs = {ParentRelative,
                                    0L,
                                    0,
                                    0L,
                                    0,
                                    0,
                                    Always,
                                    0L,
                                    0L,
                                    False,
                                    StructureNotifyMask | ExposureMask,
                                    0L,
                                    True,
                                    0,
                                    0};
      flags |= CWBackPixel;
#ifdef BUILD_ARGB
      if (have_argb_visual) {
        attrs.colormap = window.colourmap;
        flags &= ~CWBackPixel;
        flags |= CWBorderPixel | CWColormap;
      }
#endif /* BUILD_ARGB */

      /* Parent is desktop window (which might be a child of root) */
      window.window =
          XCreateWindow(display, window.desktop, window.x, window.y, b, b, 0,
                        depth, InputOutput, visual, flags, &attrs);

      XLowerWindow(display, window.window);
      XSetClassHint(display, window.window, &classHint);

      NORM_ERR("window type - override");
    } else { /* own_window_type.get(l) != TYPE_OVERRIDE */

      /* A window managed by the window manager.
       * Process hints and buttons. */
      XSetWindowAttributes attrs = {
          ParentRelative,
          0L,
          0,
          0L,
          0,
          0,
          Always,
          0L,
          0L,
          False,
          StructureNotifyMask | ExposureMask | ButtonPressMask |
              ButtonReleaseMask,
          0L,
          own_window_type.get(l) == window_type::UTILITY ? True : False,
          0,
          0};

      XWMHints wmHint;
      Atom xa;

      flags |= CWBackPixel;
#ifdef BUILD_ARGB
      if (have_argb_visual) {
        attrs.colormap = window.colourmap;
        flags &= ~CWBackPixel;
        flags |= CWBorderPixel | CWColormap;
      }
#endif /* BUILD_ARGB */

      if (own_window_type.get(l) == window_type::DOCK) {
        window.x = window.y = 0;
      }
      /* Parent is root window so WM can take control */
      window.window =
          XCreateWindow(display, window.root, window.x, window.y, b, b, 0,
                        depth, InputOutput, visual, flags, &attrs);

      uint16_t hints = own_window_hints.get(l);

      wmHint.flags = InputHint | StateHint;
      /* allow decorated windows to be given input focus by WM */
      wmHint.input = TEST_HINT(hints, window_hints::UNDECORATED) ? False : True;
#ifdef BUILD_XSHAPE
#ifdef BUILD_XFIXES
      if (own_window_type.get(l) == window_type::UTILITY) {
        XRectangle rect;
        XserverRegion region = XFixesCreateRegion(display, &rect, 1);
        XFixesSetWindowShapeRegion(display, window.window, ShapeInput, 0, 0,
                                   region);
        XFixesDestroyRegion(display, region);
      }
#endif /* BUILD_XFIXES */
      if (!wmHint.input) {
        /* allow only decorated windows to be given mouse input */
        int major_version;
        int minor_version;
        if (XShapeQueryVersion(display, &major_version, &minor_version) == 0) {
          NORM_ERR("Input shapes are not supported");
        } else {
          if (own_window.get(*state) &&
              (own_window_type.get(*state) != window_type::NORMAL ||
               ((TEST_HINT(own_window_hints.get(*state),
                           window_hints::UNDECORATED)) != 0))) {
            XShapeCombineRectangles(display, window.window, ShapeInput, 0, 0,
                                    nullptr, 0, ShapeSet, Unsorted);
          }
        }
      }
#endif /* BUILD_XSHAPE */
      if (own_window_type.get(l) == window_type::DOCK ||
          own_window_type.get(l) == window_type::PANEL) {
        wmHint.initial_state = WithdrawnState;
      } else {
        wmHint.initial_state = NormalState;
      }

      XmbSetWMProperties(display, window.window, nullptr, nullptr, argv_copy,
                         argc_copy, nullptr, &wmHint, &classHint);
      XStoreName(display, window.window, own_window_title.get(l).c_str());

      /* Sets an empty WM_PROTOCOLS property */
      XSetWMProtocols(display, window.window, nullptr, 0);

      /* Set window type */
      if ((xa = ATOM(_NET_WM_WINDOW_TYPE)) != None) {
        Atom prop;

        switch (own_window_type.get(l)) {
          case window_type::DESKTOP:
            prop = ATOM(_NET_WM_WINDOW_TYPE_DESKTOP);
            NORM_ERR("window type - desktop");
            break;
          case window_type::DOCK:
            prop = ATOM(_NET_WM_WINDOW_TYPE_DOCK);
            NORM_ERR("window type - dock");
            break;
          case window_type::PANEL:
            prop = ATOM(_NET_WM_WINDOW_TYPE_DOCK);
            NORM_ERR("window type - panel");
            break;
          case window_type::UTILITY:
            prop = ATOM(_NET_WM_WINDOW_TYPE_UTILITY);
            NORM_ERR("window type - utility");
            break;
          case window_type::NORMAL:
          default:
            prop = ATOM(_NET_WM_WINDOW_TYPE_NORMAL);
            NORM_ERR("window type - normal");
            break;
        }
        XChangeProperty(display, window.window, xa, XA_ATOM, 32,
                        PropModeReplace,
                        reinterpret_cast<unsigned char *>(&prop), 1);
      }

      /* Set desired hints */

      /* Window decorations */
      if (TEST_HINT(hints, window_hints::UNDECORATED)) {
        DBGP("hint - undecorated");
        xa = ATOM(_MOTIF_WM_HINTS);
        if (xa != None) {
          long prop[5] = {2, 0, 0, 0, 0};
          XChangeProperty(display, window.window, xa, xa, 32, PropModeReplace,
                          reinterpret_cast<unsigned char *>(prop), 5);
        }
      }

      /* Below other windows */
      if (TEST_HINT(hints, window_hints::BELOW)) {
        DBGP("hint - below");
        xa = ATOM(_WIN_LAYER);
        if (xa != None) {
          long prop = 0;

          XChangeProperty(display, window.window, xa, XA_CARDINAL, 32,
                          PropModeAppend,
                          reinterpret_cast<unsigned char *>(&prop), 1);
        }

        xa = ATOM(_NET_WM_STATE);
        if (xa != None) {
          Atom xa_prop = ATOM(_NET_WM_STATE_BELOW);

          XChangeProperty(display, window.window, xa, XA_ATOM, 32,
                          PropModeAppend,
                          reinterpret_cast<unsigned char *>(&xa_prop), 1);
        }
      }

      /* Above other windows */
      if (TEST_HINT(hints, window_hints::ABOVE)) {
        DBGP("hint - above");
        xa = ATOM(_WIN_LAYER);
        if (xa != None) {
          long prop = 6;

          XChangeProperty(display, window.window, xa, XA_CARDINAL, 32,
                          PropModeAppend,
                          reinterpret_cast<unsigned char *>(&prop), 1);
        }

        xa = ATOM(_NET_WM_STATE);
        if (xa != None) {
          Atom xa_prop = ATOM(_NET_WM_STATE_ABOVE);

          XChangeProperty(display, window.window, xa, XA_ATOM, 32,
                          PropModeAppend,
                          reinterpret_cast<unsigned char *>(&xa_prop), 1);
        }
      }

      /* Sticky */
      if (TEST_HINT(hints, window_hints::STICKY)) {
        DBGP("hint - sticky");
        xa = ATOM(_NET_WM_DESKTOP);
        if (xa != None) {
          CARD32 xa_prop = 0xFFFFFFFF;

          XChangeProperty(display, window.window, xa, XA_CARDINAL, 32,
                          PropModeAppend,
                          reinterpret_cast<unsigned char *>(&xa_prop), 1);
        }

        xa = ATOM(_NET_WM_STATE);
        if (xa != None) {
          Atom xa_prop = ATOM(_NET_WM_STATE_STICKY);

          XChangeProperty(display, window.window, xa, XA_ATOM, 32,
                          PropModeAppend,
                          reinterpret_cast<unsigned char *>(&xa_prop), 1);
        }
      }

      /* Skip taskbar */
      if (TEST_HINT(hints, window_hints::SKIP_TASKBAR)) {
        DBGP("hint - skip taskbar");
        xa = ATOM(_NET_WM_STATE);
        if (xa != None) {
          Atom xa_prop = ATOM(_NET_WM_STATE_SKIP_TASKBAR);

          XChangeProperty(display, window.window, xa, XA_ATOM, 32,
                          PropModeAppend,
                          reinterpret_cast<unsigned char *>(&xa_prop), 1);
        }
      }

      /* Skip pager */
      if (TEST_HINT(hints, window_hints::SKIP_PAGER)) {
        DBGP("hint - skip pager");
        xa = ATOM(_NET_WM_STATE);
        if (xa != None) {
          Atom xa_prop = ATOM(_NET_WM_STATE_SKIP_PAGER);

          XChangeProperty(display, window.window, xa, XA_ATOM, 32,
                          PropModeAppend,
                          reinterpret_cast<unsigned char *>(&xa_prop), 1);
        }
      }
    }

    NORM_ERR("drawing to created window (0x%lx)", window.window);
    XMapWindow(display, window.window);
  } else
#endif /* OWN_WINDOW */
  {
    XWindowAttributes attrs;

    if (window.window == None) { window.window = window.desktop; }

    if (XGetWindowAttributes(display, window.window, &attrs) != 0) {
      window.width = attrs.width;
      window.height = attrs.height;
    }

    NORM_ERR("drawing to desktop window");
  }

  /* Drawable is same as window. This may be changed by double buffering. */
  window.drawable = window.window;

  XFlush(display);

  int64_t input_mask = ExposureMask | PropertyChangeMask;
#ifdef OWN_WINDOW
  if (own_window.get(l)) {
    input_mask |= StructureNotifyMask;
#if !defined(BUILD_XINPUT)
    input_mask |= ButtonPressMask | ButtonReleaseMask;
#endif
  }
#if defined(BUILD_MOUSE_EVENTS) || defined(BUILD_XINPUT)
  bool xinput_ok = false;
#ifdef BUILD_XINPUT
  // not a loop; substitutes goto with break - if checks fail
  do {
    int _ignored;  // segfault if NULL
    if (!XQueryExtension(display, "XInputExtension", &window.xi_opcode,
                         &_ignored, &_ignored)) {
      // events will still ~work but let the user know why they're buggy
      NORM_ERR("XInput extension is not supported by X11!");
      break;
    }

    int major = 2, minor = 0;
    int retval = XIQueryVersion(display, &major, &minor);
    if (retval != Success) {
      NORM_ERR("Error: XInput 2.0 is not supported!");
      break;
    }

    const std::size_t mask_size = (XI_LASTEVENT + 7) / 8;
    unsigned char mask_bytes[mask_size] = {0}; /* must be zeroed! */
    XISetMask(mask_bytes, XI_HierarchyChanged);
#ifdef BUILD_MOUSE_EVENTS
    XISetMask(mask_bytes, XI_Motion);
#endif /* BUILD_MOUSE_EVENTS */
    // Capture click events for "override" window type
    if (!own) {
      XISetMask(mask_bytes, XI_ButtonPress);
      XISetMask(mask_bytes, XI_ButtonRelease);
    }

    XIEventMask ev_masks[1];
    ev_masks[0].deviceid = XIAllDevices;
    ev_masks[0].mask_len = sizeof(mask_bytes);
    ev_masks[0].mask = mask_bytes;
    XISelectEvents(display, window.root, ev_masks, 1);

    if (own) {
#ifdef BUILD_MOUSE_EVENTS
      XIClearMask(mask_bytes, XI_Motion);
#endif /* BUILD_MOUSE_EVENTS */
      XISetMask(mask_bytes, XI_ButtonPress);
      XISetMask(mask_bytes, XI_ButtonRelease);

      ev_masks[0].deviceid = XIAllDevices;
      ev_masks[0].mask_len = sizeof(mask_bytes);
      ev_masks[0].mask = mask_bytes;
      XISelectEvents(display, window.window, ev_masks, 1);
    }

    // setup cache
    int num_devices;
    XDeviceInfo *info = XListInputDevices(display, &num_devices);
    for (int i = 0; i < num_devices; i++) {
      if (info[i].use == IsXPointer || info[i].use == IsXExtensionPointer) {
        conky::device_info::from_xi_id(info[i].id, display);
      }
    }
    XFreeDeviceList(info);

    xinput_ok = true;
  } while (false);
#endif /* BUILD_XINPUT */
  // Fallback to basic X11 enter/leave events if xinput fails to init.
  // It's not recommended to add event masks to special windows in X; causes a
  // crash (thus own_window_type != TYPE_DESKTOP)
#ifdef BUILD_MOUSE_EVENTS
  if (!xinput_ok && own && own_window_type.get(l) != window_type::DESKTOP) {
    input_mask |= PointerMotionMask | EnterWindowMask | LeaveWindowMask;
  }
#endif /* BUILD_MOUSE_EVENTS */
#endif /* BUILD_MOUSE_EVENTS || BUILD_XINPUT */
#endif /* OWN_WINDOW */
  window.event_mask = input_mask;
  XSelectInput(display, window.window, input_mask);

  window_created = 1;
  DBGP("leave x11_init_window()");
}

static Window find_subwindow(Window win, int w, int h) {
  unsigned int i, j;
  Window troot, parent, *children;
  unsigned int n;

  /* search subwindows with same size as display or work area */

  for (i = 0; i < 10; i++) {
    XQueryTree(display, win, &troot, &parent, &children, &n);

    for (j = 0; j < n; j++) {
      XWindowAttributes attrs;

      if (XGetWindowAttributes(display, children[j], &attrs) != 0) {
        /* Window must be mapped and same size as display or
         * work space */
        if (attrs.map_state != 0 &&
            ((attrs.width == display_width && attrs.height == display_height) ||
             (attrs.width == w && attrs.height == h))) {
          win = children[j];
          break;
        }
      }
    }

    XFree(children);
    if (j == n) { break; }
  }

  return win;
}

void create_gc() {
  XGCValues values;

  values.graphics_exposures = 0;
  values.function = GXcopy;
  window.gc = XCreateGC(display, window.drawable,
                        GCFunction | GCGraphicsExposures, &values);
}

// Get current desktop number
static inline void get_x11_desktop_current(Display *current_display,
                                           Window root, Atom atom) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *prop = nullptr;
  struct information *current_info = &info;

  if (atom == None) { return; }

  if ((XGetWindowProperty(current_display, root, atom, 0, 1L, False,
                          XA_CARDINAL, &actual_type, &actual_format, &nitems,
                          &bytes_after, &prop) == Success) &&
      (actual_type == XA_CARDINAL) && (nitems == 1L) && (actual_format == 32)) {
    current_info->x11.desktop.current = prop[0] + 1;
  }
  if (prop != nullptr) { XFree(prop); }
}

// Get total number of available desktops
static inline void get_x11_desktop_number(Display *current_display, Window root,
                                          Atom atom) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *prop = nullptr;
  struct information *current_info = &info;

  if (atom == None) { return; }

  if ((XGetWindowProperty(current_display, root, atom, 0, 1L, False,
                          XA_CARDINAL, &actual_type, &actual_format, &nitems,
                          &bytes_after, &prop) == Success) &&
      (actual_type == XA_CARDINAL) && (nitems == 1L) && (actual_format == 32)) {
    current_info->x11.desktop.number = prop[0];
  }
  if (prop != nullptr) { XFree(prop); }
}

// Get all desktop names
static inline void get_x11_desktop_names(Display *current_display, Window root,
                                         Atom atom) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *prop = nullptr;
  struct information *current_info = &info;

  if (atom == None) { return; }

  if ((XGetWindowProperty(current_display, root, atom, 0, (~0L), False,
                          ATOM(UTF8_STRING), &actual_type, &actual_format,
                          &nitems, &bytes_after, &prop) == Success) &&
      (actual_type == ATOM(UTF8_STRING)) && (nitems > 0L) &&
      (actual_format == 8)) {
    current_info->x11.desktop.all_names.assign(
        reinterpret_cast<const char *>(prop), nitems);
  }
  if (prop != nullptr) { XFree(prop); }
}

// Get current desktop name
static inline void get_x11_desktop_current_name(const std::string &names) {
  struct information *current_info = &info;
  unsigned int i = 0, j = 0;
  int k = 0;

  while (i < names.size()) {
    if (names[i++] == '\0') {
      if (++k == current_info->x11.desktop.current) {
        current_info->x11.desktop.name.assign(names.c_str() + j);
        break;
      }
      j = i;
    }
  }
}

void get_x11_desktop_info(Display *current_display, Atom atom) {
  Window root;
  static Atom atom_current, atom_number, atom_names;
  struct information *current_info = &info;
  XWindowAttributes window_attributes;

  root = RootWindow(current_display, current_info->x11.monitor.current);

  /* Check if we initialise else retrieve changed property */
  if (atom == 0) {
    atom_current = XInternAtom(current_display, "_NET_CURRENT_DESKTOP", True);
    atom_number = XInternAtom(current_display, "_NET_NUMBER_OF_DESKTOPS", True);
    atom_names = XInternAtom(current_display, "_NET_DESKTOP_NAMES", True);
    get_x11_desktop_current(current_display, root, atom_current);
    get_x11_desktop_number(current_display, root, atom_number);
    get_x11_desktop_names(current_display, root, atom_names);
    get_x11_desktop_current_name(current_info->x11.desktop.all_names);

    /* Set the PropertyChangeMask on the root window, if not set */
    XGetWindowAttributes(display, root, &window_attributes);
    if ((window_attributes.your_event_mask & PropertyChangeMask) == 0) {
      XSetWindowAttributes attributes;
      attributes.event_mask =
          window_attributes.your_event_mask | PropertyChangeMask;
      XChangeWindowAttributes(display, root, CWEventMask, &attributes);
      XGetWindowAttributes(display, root, &window_attributes);
    }
  } else {
    if (atom == atom_current) {
      get_x11_desktop_current(current_display, root, atom_current);
      get_x11_desktop_current_name(current_info->x11.desktop.all_names);
    } else if (atom == atom_number) {
      get_x11_desktop_number(current_display, root, atom_number);
    } else if (atom == atom_names) {
      get_x11_desktop_names(current_display, root, atom_names);
      get_x11_desktop_current_name(current_info->x11.desktop.all_names);
    }
  }
}

static const char NOT_IN_X[] = "Not running in X";

void print_monitor(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;

  if (!out_to_x.get(*state)) {
    strncpy(p, NOT_IN_X, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", XDefaultScreen(display));
}

void print_monitor_number(struct text_object *obj, char *p,
                          unsigned int p_max_size) {
  (void)obj;

  if (!out_to_x.get(*state)) {
    strncpy(p, NOT_IN_X, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", XScreenCount(display));
}

void print_desktop(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;

  if (!out_to_x.get(*state)) {
    strncpy(p, NOT_IN_X, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", info.x11.desktop.current);
}

void print_desktop_number(struct text_object *obj, char *p,
                          unsigned int p_max_size) {
  (void)obj;

  if (!out_to_x.get(*state)) {
    strncpy(p, NOT_IN_X, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", info.x11.desktop.number);
}

void print_desktop_name(struct text_object *obj, char *p,
                        unsigned int p_max_size) {
  (void)obj;

  if (!out_to_x.get(*state)) {
    strncpy(p, NOT_IN_X, p_max_size);
  } else {
    strncpy(p, info.x11.desktop.name.c_str(), p_max_size);
  }
}

#ifdef OWN_WINDOW
enum class x11_strut : size_t {
  LEFT,
  RIGHT,
  TOP,
  BOTTOM,
  LEFT_START_Y,
  LEFT_END_Y,
  RIGHT_START_Y,
  RIGHT_END_Y,
  TOP_START_X,
  TOP_END_X,
  BOTTOM_START_X,
  BOTTOM_END_X,
  COUNT
};
constexpr size_t operator*(x11_strut index) {
  return static_cast<size_t>(index);
}

/* reserve window manager space */
void set_struts(alignment align) {
  // Middle and none align don't have least significant bit set.
  // Ensures either vertical or horizontal axis are start/end
  if ((*align & 0b0101) == 0) return;

  Atom strut = ATOM(_NET_WM_STRUT);
  if (strut != None) {
    /* reserve space at left, right, top, bottom */
    uint32_t sizes[*x11_strut::COUNT] = {0};
    int i;

    switch (vertical_alignment(align)) {
      case axis_align::START:
        sizes[*x11_strut::TOP] =
            std::min(window.y + window.height, display_height);
        sizes[*x11_strut::TOP_START_X] = window.x;
        sizes[*x11_strut::TOP_END_X] =
            std::min(window.x + window.width, display_width);
      case axis_align::END:
        sizes[*x11_strut::BOTTOM] =
            window.y < display_height ? display_height - window.y : 0;
        sizes[*x11_strut::BOTTOM_START_X] = window.x;
        sizes[*x11_strut::BOTTOM_END_X] =
            std::min(window.x + window.width, display_width);
      case axis_align::MIDDLE:
        // can't reserve space in middle of the screen
      default:
        break;
    }
    // adding `vertical_alignment(align) & 0x1` makes the switch hit MIDDLE if
    // vertical alignment is set to left or right
    switch (static_cast<axis_align>(
        *horizontal_alignment(align) + *vertical_alignment(align) & 0x1)) {
      case axis_align::START:
        sizes[*x11_strut::LEFT] =
            std::min(window.x + window.width, display_width);
        sizes[*x11_strut::LEFT_START_Y] = window.y;
        sizes[*x11_strut::LEFT_END_Y] =
            std::min(window.y + window.height, display_height);
      case axis_align::END:
        sizes[*x11_strut::RIGHT] =
            window.x < display_width ? display_width - window.x : 0;
        sizes[*x11_strut::RIGHT_START_Y] = window.y;
        sizes[*x11_strut::RIGHT_END_Y] =
            std::min(window.y + window.height, display_height);
      case axis_align::MIDDLE:
        // can't reserve space in middle of the screen
      default:
        break;
    }

    XChangeProperty(display, window.window, strut, XA_CARDINAL, 32,
                    PropModeReplace, reinterpret_cast<unsigned char *>(&sizes),
                    4);

    strut = ATOM(_NET_WM_STRUT_PARTIAL);
    if (strut != None) {
      XChangeProperty(display, window.window, strut, XA_CARDINAL, 32,
                      PropModeReplace,
                      reinterpret_cast<unsigned char *>(&sizes), 12);
    }
  }
}
#endif /* OWN_WINDOW */

#ifdef BUILD_XDBE
void xdbe_swap_buffers() {
  if (use_xdbe.get(*state)) {
    XdbeSwapInfo swap;

    swap.swap_window = window.window;
    swap.swap_action = XdbeBackground;
    XdbeSwapBuffers(display, &swap, 1);
  }
}
#else
void xpmdb_swap_buffers(void) {
  if (use_xpmdb.get(*state)) {
    XCopyArea(display, window.back_buffer, window.window, window.gc, 0, 0,
              window.width, window.height, 0, 0);
    XSetForeground(display, window.gc, 0);
    XFillRectangle(display, window.drawable, window.gc, 0, 0, window.width,
                   window.height);
    XFlush(display);
  }
}
#endif /* BUILD_XDBE */

void print_kdb_led(const int keybit, char *p, unsigned int p_max_size) {
  XKeyboardState x;
  XGetKeyboardControl(display, &x);
  snprintf(p, p_max_size, "%s", (x.led_mask & keybit ? "On" : "Off"));
}
void print_key_caps_lock(struct text_object *obj, char *p,
                         unsigned int p_max_size) {
  (void)obj;
  print_kdb_led(1, p, p_max_size);
}

void print_key_num_lock(struct text_object *obj, char *p,
                        unsigned int p_max_size) {
  (void)obj;
  print_kdb_led(2, p, p_max_size);
}

void print_key_scroll_lock(struct text_object *obj, char *p,
                           unsigned int p_max_size) {
  (void)obj;
  print_kdb_led(4, p, p_max_size);
}

void print_keyboard_layout(struct text_object *obj, char *p,
                           unsigned int p_max_size) {
  (void)obj;

  char *group = NULL;
  XkbStateRec state;
  XkbDescPtr desc;

  XkbGetState(display, XkbUseCoreKbd, &state);
  desc = XkbGetKeyboard(display, XkbAllComponentsMask, XkbUseCoreKbd);
  group = XGetAtomName(display, desc->names->groups[state.group]);

  snprintf(p, p_max_size, "%s", (group != NULL ? group : "unknown"));
  XFree(group);
  XkbFreeKeyboard(desc, XkbGBN_AllComponentsMask, True);
}

void print_mouse_speed(struct text_object *obj, char *p,
                       unsigned int p_max_size) {
  (void)obj;
  int acc_num = 0;
  int acc_denom = 0;
  int threshold = 0;

  XGetPointerControl(display, &acc_num, &acc_denom, &threshold);
  snprintf(p, p_max_size, "%d%%", (110 - threshold));
}

/// @brief Returns a mask for the event_type
/// @param event_type Xlib event type
/// @return Xlib event mask
int ev_to_mask(int event_type, int button) {
  switch (event_type) {
    case KeyPress:
      return KeyPressMask;
    case KeyRelease:
      return KeyReleaseMask;
    case ButtonPress:
      return ButtonPressMask;
    case ButtonRelease:
      switch (button) {
        case 1:
          return ButtonReleaseMask | Button1MotionMask;
        case 2:
          return ButtonReleaseMask | Button2MotionMask;
        case 3:
          return ButtonReleaseMask | Button3MotionMask;
        case 4:
          return ButtonReleaseMask | Button4MotionMask;
        case 5:
          return ButtonReleaseMask | Button5MotionMask;
        default:
          return ButtonReleaseMask;
      }
    case EnterNotify:
      return EnterWindowMask;
    case LeaveNotify:
      return LeaveWindowMask;
    case MotionNotify:
      return PointerMotionMask;
    default:
      return NoEventMask;
  }
}

#ifdef BUILD_XINPUT
void propagate_xinput_event(const conky::xi_event_data *ev) {
  if (ev->evtype != XI_Motion && ev->evtype != XI_ButtonPress &&
      ev->evtype != XI_ButtonRelease) {
    return;
  }

  Window target = window.root;
  Window child = None;
  int target_x = ev->event_x;
  int target_y = ev->event_y;
  {
    std::vector<Window> below = query_x11_windows_at_pos(
        display, ev->root_x, ev->root_y,
        [](XWindowAttributes &a) { return a.map_state == IsViewable; });
    auto it = std::remove_if(below.begin(), below.end(),
                             [](Window w) { return w == window.window; });
    below.erase(it, below.end());
    if (!below.empty()) {
      target = below.back();

      // Update event x and y coordinates to be target window relative
      XTranslateCoordinates(display, window.desktop, ev->event, ev->root_x,
                            ev->root_y, &target_x, &target_y, &child);
    }
  }

  auto events = ev->generate_events(target, child, target_x, target_y);

  XUngrabPointer(display, CurrentTime);
  for (auto it : events) {
    auto ev = std::get<1>(it);
    XSendEvent(display, target, True, std::get<0>(it), ev);
    free(ev);
  }

  XFlush(display);
}
#endif

void propagate_x11_event(XEvent &ev, const void *cookie) {
  bool focus = ev.type == ButtonPress;

  // cookie must be allocated before propagation, and freed after
#ifdef BUILD_XINPUT
  if (ev.type == GenericEvent && ev.xgeneric.extension == window.xi_opcode) {
    if (cookie == nullptr) { return; }
    return propagate_xinput_event(
        reinterpret_cast<const conky::xi_event_data *>(cookie));
  }
#endif

  if (!(ev.type == KeyPress || ev.type == KeyRelease ||
        ev.type == ButtonPress || ev.type == ButtonRelease ||
        ev.type == MotionNotify || ev.type == EnterNotify ||
        ev.type == LeaveNotify)) {
    // Not a known input event; blindly propagating them causes loops and all
    // sorts of other evil.
    return;
  }
  // Note that using ev.xbutton is the same as using any of the above events.
  // It's only important we don't access fields that are not common to all of
  // them.

  ev.xbutton.window = window.desktop;
  ev.xbutton.x = ev.xbutton.x_root;
  ev.xbutton.y = ev.xbutton.y_root;
  ev.xbutton.time = CurrentTime;

  /* forward the event to the window below conky (e.g. caja) or desktop */
  {
    std::vector<Window> below = query_x11_windows_at_pos(
        display, ev.xbutton.x_root, ev.xbutton.y_root,
        [](XWindowAttributes &a) { return a.map_state == IsViewable; });
    auto it = std::remove_if(below.begin(), below.end(),
                             [](Window w) { return w == window.window; });
    below.erase(it, below.end());
    if (!below.empty()) {
      ev.xbutton.window = below.back();

      Window _ignore;
      // Update event x and y coordinates to be target window relative
      XTranslateCoordinates(display, window.root, ev.xbutton.window,
                            ev.xbutton.x_root, ev.xbutton.y_root, &ev.xbutton.x,
                            &ev.xbutton.y, &_ignore);
    }
    // drop below vector
  }

  int mask =
      ev_to_mask(ev.type, ev.type == ButtonRelease ? ev.xbutton.button : 0);
  XUngrabPointer(display, CurrentTime);
  XSendEvent(display, ev.xbutton.window, True, mask, &ev);
  if (focus) {
    XSetInputFocus(display, ev.xbutton.window, RevertToParent, CurrentTime);
  }
}

Window query_x11_top_parent(Display *display, Window child) {
  Window root = DefaultVRootWindow(display);

  if (child == None || child == root) return child;

  Window ret_root, parent, *children;
  std::uint32_t child_count;

  Window current = child;
  int i;
  do {
    if (XQueryTree(display, current, &ret_root, &parent, &children,
                   &child_count) == 0) {
      break;
    }
    if (child_count != 0) XFree(children);
    if (parent == root) break;
    current = parent;
  } while (true);

  return current;
}

std::vector<Window> x11_atom_window_list(Display *display, Window window,
                                         Atom atom) {
  Atom actual_type;
  int actual_format;
  unsigned long nitems;
  unsigned long bytes_after;
  unsigned char *data = nullptr;

  if (XGetWindowProperty(display, window, atom, 0, (~0L), False, XA_WINDOW,
                         &actual_type, &actual_format, &nitems, &bytes_after,
                         &data) == Success) {
    if (actual_format == XA_WINDOW && nitems > 0) {
      Window *wdata = reinterpret_cast<Window *>(data);
      std::vector<Window> result(wdata, wdata + nitems);
      XFree(data);
      return result;
    }
  }

  return std::vector<Window>{};
}

std::vector<Window> query_x11_windows(Display *display) {
  Window root = DefaultRootWindow(display);

  Atom clients_atom = ATOM(_NET_CLIENT_LIST_STACKING);
  std::vector<Window> result =
      x11_atom_window_list(display, root, clients_atom);
  if (result.empty()) { return result; }

  clients_atom = ATOM(_NET_CLIENT_LIST);
  result = x11_atom_window_list(display, root, clients_atom);
  if (result.empty()) { return result; }

  // slowest method

  std::vector<Window> queue = {DefaultVRootWindow(display)};

  Window _ignored, *children;
  std::uint32_t count;

  const auto has_wm_hints = [&](Window window) {
    auto hints = XGetWMHints(display, window);
    bool result = hints != NULL;
    if (result) XFree(hints);
    return result;
  };

  while (!queue.empty()) {
    Window current = queue.back();
    queue.pop_back();
    if (XQueryTree(display, current, &_ignored, &_ignored, &children, &count)) {
      for (size_t i = 0; i < count; i++) queue.push_back(children[i]);
      if (has_wm_hints(current)) result.push_back(current);
      if (count > 0) XFree(children);
    }
  }

  return result;
}

Window query_x11_window_at_pos(Display *display, int x, int y) {
  Window root = DefaultVRootWindow(display);

  // these values are ignored but NULL can't be passed to XQueryPointer.
  Window root_return;
  int root_x_return, root_y_return, win_x_return, win_y_return;
  unsigned int mask_return;

  Window last = None;
  XQueryPointer(display, window.root, &root_return, &last, &root_x_return,
                &root_y_return, &win_x_return, &win_y_return, &mask_return);

  if (last == 0) return root;
  return last;
}

std::vector<Window> query_x11_windows_at_pos(
    Display *display, int x, int y,
    std::function<bool(XWindowAttributes &)> predicate) {
  std::vector<Window> result;

  Window root = DefaultVRootWindow(display);
  XWindowAttributes attr;

  for (Window current : query_x11_windows(display)) {
    int pos_x, pos_y;
    Window _ignore;
    // Doesn't account for decorations. There's no sane way to do that.
    XTranslateCoordinates(display, current, root, 0, 0, &pos_x, &pos_y,
                          &_ignore);
    XGetWindowAttributes(display, current, &attr);

    if (pos_x <= x && pos_y <= y && pos_x + attr.width >= x &&
        pos_y + attr.height >= y && predicate(attr)) {
      result.push_back(current);
    }
  }

  return result;
}
