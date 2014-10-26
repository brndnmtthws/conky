/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
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
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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
#include "conky.h"
#include "logging.h"
#include "common.h"

#include "x11.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#ifdef BUILD_IMLIB2
#include "imlib2.h"
#endif /* BUILD_IMLIB2 */
#ifndef OWN_WINDOW
#include <iostream>
#endif
#ifdef BUILD_XFT
#include <X11/Xft/Xft.h>
#endif
#ifdef BUILD_XSHAPE
#include <X11/extensions/shape.h>
#include <X11/extensions/shapeconst.h>
#endif

#ifdef BUILD_ARGB
bool have_argb_visual;
#endif /* BUILD_ARGB */

/* some basic X11 stuff */
Display *display = NULL;
int display_width;
int display_height;
int screen;

/* workarea from _NET_WORKAREA, this is where window / text is aligned */
int workarea[4];

/* Window stuff */
struct conky_window window;
char window_created = 0;

/* local prototypes */
static void update_workarea(void);
static Window find_desktop_window(Window *p_root, Window *p_desktop);
static Window find_subwindow(Window win, int w, int h);
static void init_X11();
static void deinit_X11();
static void init_window(lua::state &l, bool own);

/********************* <SETTINGS> ************************/
namespace priv {
	void out_to_x_setting::lua_setter(lua::state &l, bool init)
	{
		lua::stack_sentry s(l, -2);

		Base::lua_setter(l, init);

		if(init && do_convert(l, -1).first)
			init_X11();

		++s;
	}

	void out_to_x_setting::cleanup(lua::state &l)
	{
		lua::stack_sentry s(l, -1);

		if(do_convert(l, -1).first)
			deinit_X11();

		l.pop();
	}

	void own_window_setting::lua_setter(lua::state &l, bool init)
	{
		lua::stack_sentry s(l, -2);

		Base::lua_setter(l, init);

		if(init) {
			if(do_convert(l, -1).first) {
#ifndef OWN_WINDOW
				std::cerr << "Support for the own_window setting has been "
							 "disabled during compilation\n";
				l.pop();
				l.pushboolean(false);
#endif
			}

			if(out_to_x.get(l))
				init_window(l, do_convert(l, -1).first);
			else {
				// own_window makes no sense when not drawing to X
				l.pop();
				l.pushboolean(false);
			} 
		}

		++s;
	}

#ifdef BUILD_XDBE
	bool use_xdbe_setting::set_up(lua::state &l)
	{
		// double_buffer makes no sense when not drawing to X
		if(not out_to_x.get(l))
			return false;

		int major, minor;

		if (not XdbeQueryExtension(display, &major, &minor)) {
			NORM_ERR("No compatible double buffer extension found");
			return false;
		}
		
		window.back_buffer = XdbeAllocateBackBufferName(display,
				window.window, XdbeBackground);
		if (window.back_buffer != None) {
			window.drawable = window.back_buffer;
		} else {
			NORM_ERR("Failed to allocate back buffer");
			return false;
		}

		XFlush(display);
		return true;
	}

	void use_xdbe_setting::lua_setter(lua::state &l, bool init)
	{
		lua::stack_sentry s(l, -2);

		Base::lua_setter(l, init);

		if(init && do_convert(l, -1).first) {
			if(not set_up(l)) {
				l.pop();
				l.pushboolean(false);
			}

			fprintf(stderr, PACKAGE_NAME": drawing to %s buffer\n",
					do_convert(l, -1).first?"double":"single");
		}

		++s;
	}



#else
	bool use_xpmdb_setting::set_up(lua::state &l)
	{
		// double_buffer makes no sense when not drawing to X
		if(not out_to_x.get(l))
			return false;

		window.back_buffer = XCreatePixmap(display,
				window.window, window.width+1, window.height+1, DefaultDepth(display, screen));
		if (window.back_buffer != None) {
			window.drawable = window.back_buffer;
		} else {
			NORM_ERR("Failed to allocate back buffer");
			return false;
		}
		

		XFlush(display);
		return true;
	}

	void use_xpmdb_setting::lua_setter(lua::state &l, bool init)
	{
		lua::stack_sentry s(l, -2);

		Base::lua_setter(l, init);

		if(init && do_convert(l, -1).first) {
			if(not set_up(l)) {
				l.pop();
				l.pushboolean(false);
			}

			fprintf(stderr, PACKAGE_NAME": drawing to %s buffer\n",
					do_convert(l, -1).first?"double":"single");
		}

		++s;
	}
#endif

	void colour_setting::lua_setter(lua::state &l, bool init)
	{
		lua::stack_sentry s(l, -2);

		if(not out_to_x.get(l)) {
			// ignore if we're not using X
			l.replace(-2);
		} else
			Base::lua_setter(l, init);

		++s;
	}
}

template<>
conky::lua_traits<alignment>::Map conky::lua_traits<alignment>::map = {
	{ "top_left",      TOP_LEFT },
	{ "top_right",     TOP_RIGHT },
	{ "top_middle",    TOP_MIDDLE },
	{ "bottom_left",   BOTTOM_LEFT },
	{ "bottom_right",  BOTTOM_RIGHT },
	{ "bottom_middle", BOTTOM_MIDDLE },
	{ "middle_left",   MIDDLE_LEFT },
	{ "middle_middle", MIDDLE_MIDDLE },
	{ "middle_right",  MIDDLE_RIGHT },
	{ "none",          NONE }
};

#ifdef OWN_WINDOW
template<>
conky::lua_traits<window_type>::Map conky::lua_traits<window_type>::map = {
	{ "normal",   TYPE_NORMAL },
	{ "dock",     TYPE_DOCK },
	{ "panel",    TYPE_PANEL },
	{ "desktop",  TYPE_DESKTOP },
	{ "override", TYPE_OVERRIDE }
};

template<>
conky::lua_traits<window_hints>::Map conky::lua_traits<window_hints>::map = {
	{ "undecorated",  HINT_UNDECORATED },
	{ "below",        HINT_BELOW },
	{ "above",        HINT_ABOVE },
	{ "sticky",       HINT_STICKY },
	{ "skip_taskbar", HINT_SKIP_TASKBAR },
	{ "skip_pager",   HINT_SKIP_PAGER }
};

std::pair<uint16_t, bool>
window_hints_traits::convert(lua::state &l, int index, const std::string &name)
{
	typedef conky::lua_traits<window_hints> Traits;

	lua::stack_sentry s(l);
	l.checkstack(1);

	std::string hints = l.tostring(index);
	// add a sentinel to simplify the following loop
	hints += ',';
	size_t pos = 0;
	size_t newpos;
	uint16_t ret = 0;
	while((newpos = hints.find_first_of(", ", pos)) != std::string::npos) {
		if(newpos > pos) {
			l.pushstring(hints.substr(pos, newpos-pos));
			auto t = conky::lua_traits<window_hints>::convert(l, -1, name);
			if(not t.second)
				return {0, false};
			SET_HINT(ret, t.first);
			l.pop();
		}
		pos = newpos+1;
	}
	return {ret, true};
}
#endif

namespace {
	// used to set the default value for own_window_title
	std::string gethostnamecxx()
	{ update_uname(); return info.uname_s.nodename; }
}

/*
 * The order of these settings cannot be completely arbitrary. Some of them depend on others, and
 * the setters are called in the order in which they are defined. The order should be:
 * display_name -> out_to_x -> everything colour related
 *                          -> border_*, own_window_*, etc -> own_window -> double_buffer ->  imlib_cache_size
 */

conky::simple_config_setting<alignment>   text_alignment("alignment", BOTTOM_LEFT, false);
conky::simple_config_setting<std::string> display_name("display", std::string(), false);
priv::out_to_x_setting                    out_to_x;

priv::colour_setting					  color[10] = {
	{ "color0", 0xffffff },
	{ "color1", 0xffffff },
	{ "color2", 0xffffff },
	{ "color3", 0xffffff },
	{ "color4", 0xffffff },
	{ "color5", 0xffffff },
	{ "color6", 0xffffff },
	{ "color7", 0xffffff },
	{ "color8", 0xffffff },
	{ "color9", 0xffffff }
};
priv::colour_setting					  default_color("default_color", 0xffffff);
priv::colour_setting					  default_shade_color("default_shade_color", 0x000000);
priv::colour_setting					  default_outline_color("default_outline_color", 0x000000);

conky::range_config_setting<int>          border_inner_margin("border_inner_margin", 0,
													std::numeric_limits<int>::max(), 3, true);
conky::range_config_setting<int>          border_outer_margin("border_outer_margin", 0,
													std::numeric_limits<int>::max(), 1, true);
conky::range_config_setting<int>          border_width("border_width", 0,
													std::numeric_limits<int>::max(), 1, true);
#ifdef BUILD_XFT
conky::simple_config_setting<bool>        use_xft("use_xft", false, false);
#endif

#ifdef OWN_WINDOW
conky::simple_config_setting<bool>        set_transparent("own_window_transparent", false, false);
conky::simple_config_setting<std::string> own_window_class("own_window_class",
															PACKAGE_NAME, false);

conky::simple_config_setting<std::string> own_window_title("own_window_title",
										PACKAGE_NAME " (" + gethostnamecxx()+")", false);

conky::simple_config_setting<window_type> own_window_type("own_window_type", TYPE_NORMAL, false);
conky::simple_config_setting<uint16_t, window_hints_traits>
									      own_window_hints("own_window_hints", 0, false);

priv::colour_setting                      background_colour("own_window_colour", 0);

#ifdef BUILD_ARGB
conky::simple_config_setting<bool>        use_argb_visual("own_window_argb_visual", false, false);
conky::range_config_setting<int>          own_window_argb_value("own_window_argb_value",
																0, 255, 255, false);
#endif
#endif /*OWN_WINDOW*/
priv::own_window_setting				  own_window;

#ifdef BUILD_XDBE
priv::use_xdbe_setting			 		  use_xdbe;
#else
priv::use_xpmdb_setting			 		  use_xpmdb;
#endif

#ifdef BUILD_IMLIB2
/*
 * the only reason this is not in imlib2.cc is so that we can be sure it's setter executes after
 * use_xdbe
 */
imlib_cache_size_setting imlib_cache_size;
#endif
/******************** </SETTINGS> ************************/

#ifdef DEBUG
/* WARNING, this type not in Xlib spec */
static int __attribute__((noreturn)) x11_error_handler(Display *d, XErrorEvent *err)
{
	NORM_ERR("X Error: type %i Display %lx XID %li serial %lu error_code %i request_code %i minor_code %i other Display: %lx\n",
			err->type,
			(long unsigned)err->display,
			(long)err->resourceid,
			err->serial,
			err->error_code,
			err->request_code,
			err->minor_code,
			(long unsigned)d
			);
	abort();
}

static int __attribute__((noreturn)) x11_ioerror_handler(Display *d)
{
	NORM_ERR("X Error: Display %lx\n",
			(long unsigned)d
			);
	exit(1);
}
#endif /* DEBUG */

/* X11 initializer */
static void init_X11()
{
	if (!display) {
		const std::string &dispstr = display_name.get(*state).c_str();
		// passing NULL to XOpenDisplay should open the default display
		const char *disp = dispstr.size() ? dispstr.c_str() : NULL;
		if ((display = XOpenDisplay(disp)) == NULL) {
			throw std::runtime_error(std::string("can't open display: ") + XDisplayName(disp));
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

#ifdef DEBUG
		_Xdebug = 1;
		/* WARNING, this type not in Xlib spec */
		XSetErrorHandler(&x11_error_handler);
		XSetIOErrorHandler(&x11_ioerror_handler);
#endif /* DEBUG */
}

static void deinit_X11()
{
	XCloseDisplay(display);
	display = NULL;
}

static void update_workarea(void)
{
	/* default work area is display */
	workarea[0] = 0;
	workarea[1] = 0;
	workarea[2] = display_width;
	workarea[3] = display_height;
}

/* Find root window and desktop window.
 * Return desktop window on success,
 * and set root and desktop byref return values.
 * Return 0 on failure. */
static Window find_desktop_window(Window *p_root, Window *p_desktop)
{
	Atom type;
	int format, i;
	unsigned long nitems, bytes;
	unsigned int n;
	Window root = RootWindow(display, screen);
	Window win = root;
	Window troot, parent, *children;
	unsigned char *buf = NULL;

	if (!p_root || !p_desktop) {
		return 0;
	}

	/* some window managers set __SWM_VROOT to some child of root window */

	XQueryTree(display, root, &troot, &parent, &children, &n);
	for (i = 0; i < (int) n; i++) {
		if (XGetWindowProperty(display, children[i], ATOM(__SWM_VROOT), 0, 1,
					False, XA_WINDOW, &type, &format, &nitems, &bytes, &buf)
				== Success && type == XA_WINDOW) {
			win = *(Window *) buf;
			XFree(buf);
			XFree(children);
			fprintf(stderr,
					PACKAGE_NAME": desktop window (%lx) found from __SWM_VROOT property\n",
					win);
			fflush(stderr);
			*p_root = win;
			*p_desktop = win;
			return win;
		}

		if (buf) {
			XFree(buf);
			buf = 0;
		}
	}
	XFree(children);

	/* get subwindows from root */
	win = find_subwindow(root, -1, -1);

	update_workarea();

	win = find_subwindow(win, workarea[2], workarea[3]);

	if (buf) {
		XFree(buf);
		buf = 0;
	}

	if (win != root) {
		fprintf(stderr,
				PACKAGE_NAME": desktop window (%lx) is subwindow of root window (%lx)\n",
				win, root);
	} else {
		fprintf(stderr, PACKAGE_NAME": desktop window (%lx) is root window\n", win);
	}

	fflush(stderr);

	*p_root = root;
	*p_desktop = win;

	return win;
}

#ifdef OWN_WINDOW
namespace {
	/* helper function for set_transparent_background() */
	void do_set_background(Window win, int argb)
	{
		unsigned long colour = background_colour.get(*state) | (argb<<24);
		XSetWindowBackground(display, win, colour);
	}
}

/* if no argb visual is configured sets background to ParentRelative for the Window and all parents,
   else real transparency is used */
void set_transparent_background(Window win)
{
#ifdef BUILD_ARGB
	if (have_argb_visual) {
		// real transparency
		do_set_background(win, set_transparent.get(*state) ? 0 : own_window_argb_value.get(*state));
	} else {
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
	} else
		do_set_background(win, 0);
#ifdef BUILD_ARGB
	}
#endif /* BUILD_ARGB */
}
#endif

#ifdef BUILD_ARGB
static int get_argb_visual(Visual** visual, int *depth) {
	/* code from gtk project, gdk_screen_get_rgba_visual */
	XVisualInfo visual_template;
	XVisualInfo *visual_list;
	int nxvisuals = 0, i;
	
	visual_template.screen = screen;
	visual_list = XGetVisualInfo (display, VisualScreenMask,
				&visual_template, &nxvisuals);
	for (i = 0; i < nxvisuals; i++) {
		if (visual_list[i].depth == 32 &&
			 (visual_list[i].red_mask   == 0xff0000 &&
			  visual_list[i].green_mask == 0x00ff00 &&
			  visual_list[i].blue_mask  == 0x0000ff)) {
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

void destroy_window(void)
{
#ifdef BUILD_XFT
	if(window.xftdraw) {
		XftDrawDestroy(window.xftdraw);
	}
#endif /* BUILD_XFT */
	if(window.gc) {
		XFreeGC(display, window.gc);
	}
	memset(&window, 0, sizeof(struct conky_window));
}

static void init_window(lua::state &l __attribute__((unused)), bool own)
{
	// own is unused if OWN_WINDOW is not defined
	(void) own;

	window_created = 1;

#ifdef OWN_WINDOW
	if (own) {
		int depth = 0, flags = CWOverrideRedirect | CWBackingStore;
		Visual *visual = NULL;
		
		if (!find_desktop_window(&window.root, &window.desktop)) {
			return;
		}
		
#ifdef BUILD_ARGB
		if (use_argb_visual.get(l) && get_argb_visual(&visual, &depth)) {
			have_argb_visual = true;
			window.visual = visual;
			window.colourmap = XCreateColormap(display,
				DefaultRootWindow(display), window.visual, AllocNone);
		} else {
#endif /* BUILD_ARGB */
			window.visual = DefaultVisual(display, screen);
			window.colourmap = DefaultColormap(display, screen);
			depth = CopyFromParent;
			visual = CopyFromParent;
#ifdef BUILD_ARGB
		}
#endif /* BUILD_ARGB */

		int b = border_inner_margin.get(l) + border_width.get(l)
			+ border_outer_margin.get(l);

		if (own_window_type.get(l) == TYPE_OVERRIDE) {

			/* An override_redirect True window.
			 * No WM hints or button processing needed. */
			XSetWindowAttributes attrs = { ParentRelative, 0L, 0, 0L, 0, 0,
				Always, 0L, 0L, False, StructureNotifyMask | ExposureMask, 0L,
				True, 0, 0 };
#ifdef BUILD_ARGB
			if (have_argb_visual) {
				attrs.colormap = window.colourmap;
				flags |= CWBorderPixel | CWColormap;
			} else {
#endif /* BUILD_ARGB */
				flags |= CWBackPixel;
#ifdef BUILD_ARGB
			}
#endif /* BUILD_ARGB */

			/* Parent is desktop window (which might be a child of root) */
			window.window = XCreateWindow(display, window.desktop, window.x,
					window.y, b, b, 0, depth, InputOutput, visual,
					flags, &attrs);

			XLowerWindow(display, window.window);

			fprintf(stderr, PACKAGE_NAME": window type - override\n");
			fflush(stderr);
		} else { /* own_window_type.get(l) != TYPE_OVERRIDE */

			/* A window managed by the window manager.
			 * Process hints and buttons. */
			XSetWindowAttributes attrs = { ParentRelative, 0L, 0, 0L, 0, 0,
				Always, 0L, 0L, False, StructureNotifyMask | ExposureMask |
					ButtonPressMask | ButtonReleaseMask, 0L, False, 0, 0 };

			XClassHint classHint;
			XWMHints wmHint;
			Atom xa;
			
#ifdef BUILD_ARGB
			if (have_argb_visual) {
				attrs.colormap = window.colourmap;
				flags |= CWBorderPixel | CWColormap;
			} else {
#endif /* BUILD_ARGB */
				flags |= CWBackPixel;
#ifdef BUILD_ARGB
			}
#endif /* BUILD_ARGB */

			if (own_window_type.get(l) == TYPE_DOCK) {
				window.x = window.y = 0;
			}
			/* Parent is root window so WM can take control */
			window.window = XCreateWindow(display, window.root, window.x,
					window.y, b, b, 0, depth, InputOutput, visual,
					flags, &attrs);

			// class_name must be a named local variable, so that c_str() remains valid until we
			// call XmbSetWMProperties(). We use const_cast because, for whatever reason,
			// res_name is not declared as const char *. XmbSetWMProperties hopefully doesn't
			// modify the value (hell, even their own example app assigns a literal string
			// constant to the field)
			const std::string &class_name = own_window_class.get(l);

			classHint.res_name = const_cast<char *>(class_name.c_str());
			classHint.res_class = classHint.res_name;

			uint16_t hints = own_window_hints.get(l);

			wmHint.flags = InputHint | StateHint;
			/* allow decorated windows to be given input focus by WM */
			wmHint.input =
				TEST_HINT(hints, HINT_UNDECORATED) ? False : True;
#ifdef BUILD_XSHAPE
			if (!wmHint.input) {
				int event_base, error_base;
				if (XShapeQueryExtension(display, &event_base, &error_base)) {
					int major_version = 0, minor_version = 0;
					XShapeQueryVersion(display, &major_version, &minor_version);
					if ((major_version > 1) || ((major_version == 1) && (minor_version >=1))) {
						Region empty_region = XCreateRegion();
						XShapeCombineRegion(display, window.window, ShapeInput, 0, 0, empty_region, ShapeSet);
						XDestroyRegion(empty_region);
					} else {
						NORM_ERR("Input shapes are not supported");
					}
				} else {
					NORM_ERR("No shape extension found");
				}
			}
#endif
			if (own_window_type.get(l) == TYPE_DOCK || own_window_type.get(l) == TYPE_PANEL) {
				wmHint.initial_state = WithdrawnState;
			} else {
				wmHint.initial_state = NormalState;
			}

			XmbSetWMProperties(display, window.window, NULL, NULL, argv_copy,
					argc_copy, NULL, &wmHint, &classHint);
			XStoreName(display, window.window, own_window_title.get(l).c_str() );

			/* Sets an empty WM_PROTOCOLS property */
			XSetWMProtocols(display, window.window, NULL, 0);

			/* Set window type */
			if ((xa = ATOM(_NET_WM_WINDOW_TYPE)) != None) {
				Atom prop;

				switch (own_window_type.get(l)) {
					case TYPE_DESKTOP:
						prop = ATOM(_NET_WM_WINDOW_TYPE_DESKTOP);
						fprintf(stderr, PACKAGE_NAME": window type - desktop\n");
						fflush(stderr);
						break;
					case TYPE_DOCK:
						prop = ATOM(_NET_WM_WINDOW_TYPE_DOCK);
						fprintf(stderr, PACKAGE_NAME": window type - dock\n");
						fflush(stderr);
						break;
					case TYPE_PANEL:
						prop = ATOM(_NET_WM_WINDOW_TYPE_DOCK);
						fprintf(stderr, PACKAGE_NAME": window type - panel\n");
						fflush(stderr);
						break;
					case TYPE_NORMAL:
					default:
						prop = ATOM(_NET_WM_WINDOW_TYPE_NORMAL);
						fprintf(stderr, PACKAGE_NAME": window type - normal\n");
						fflush(stderr);
						break;
				}
				XChangeProperty(display, window.window, xa, XA_ATOM, 32,
						PropModeReplace, (unsigned char *) &prop, 1);
			}

			/* Set desired hints */

			/* Window decorations */
			if (TEST_HINT(hints, HINT_UNDECORATED)) {
				/* fprintf(stderr, PACKAGE_NAME": hint - undecorated\n");
				   fflush(stderr); */

				xa = ATOM(_MOTIF_WM_HINTS);
				if (xa != None) {
					long prop[5] = { 2, 0, 0, 0, 0 };
					XChangeProperty(display, window.window, xa, xa, 32,
							PropModeReplace, (unsigned char *) prop, 5);
				}
			}

			/* Below other windows */
			if (TEST_HINT(hints, HINT_BELOW)) {
				/* fprintf(stderr, PACKAGE_NAME": hint - below\n");
				   fflush(stderr); */

				xa = ATOM(_WIN_LAYER);
				if (xa != None) {
					long prop = 0;

					XChangeProperty(display, window.window, xa, XA_CARDINAL, 32,
							PropModeAppend, (unsigned char *) &prop, 1);
				}

				xa = ATOM(_NET_WM_STATE);
				if (xa != None) {
					Atom xa_prop = ATOM(_NET_WM_STATE_BELOW);

					XChangeProperty(display, window.window, xa, XA_ATOM, 32,
							PropModeAppend, (unsigned char *) &xa_prop, 1);
				}
			}

			/* Above other windows */
			if (TEST_HINT(hints, HINT_ABOVE)) {
				/* fprintf(stderr, PACKAGE_NAME": hint - above\n");
				   fflush(stderr); */

				xa = ATOM(_WIN_LAYER);
				if (xa != None) {
					long prop = 6;

					XChangeProperty(display, window.window, xa, XA_CARDINAL, 32,
							PropModeAppend, (unsigned char *) &prop, 1);
				}

				xa = ATOM(_NET_WM_STATE);
				if (xa != None) {
					Atom xa_prop = ATOM(_NET_WM_STATE_ABOVE);

					XChangeProperty(display, window.window, xa, XA_ATOM, 32,
							PropModeAppend, (unsigned char *) &xa_prop, 1);
				}
			}

			/* Sticky */
			if (TEST_HINT(hints, HINT_STICKY)) {
				/* fprintf(stderr, PACKAGE_NAME": hint - sticky\n");
				   fflush(stderr); */

				xa = ATOM(_NET_WM_DESKTOP);
				if (xa != None) {
					CARD32 xa_prop = 0xFFFFFFFF;

					XChangeProperty(display, window.window, xa, XA_CARDINAL, 32,
							PropModeAppend, (unsigned char *) &xa_prop, 1);
				}

				xa = ATOM(_NET_WM_STATE);
				if (xa != None) {
					Atom xa_prop = ATOM(_NET_WM_STATE_STICKY);

					XChangeProperty(display, window.window, xa, XA_ATOM, 32,
							PropModeAppend, (unsigned char *) &xa_prop, 1);
				}
			}

			/* Skip taskbar */
			if (TEST_HINT(hints, HINT_SKIP_TASKBAR)) {
				/* fprintf(stderr, PACKAGE_NAME": hint - skip_taskbar\n");
				   fflush(stderr); */

				xa = ATOM(_NET_WM_STATE);
				if (xa != None) {
					Atom xa_prop = ATOM(_NET_WM_STATE_SKIP_TASKBAR);

					XChangeProperty(display, window.window, xa, XA_ATOM, 32,
							PropModeAppend, (unsigned char *) &xa_prop, 1);
				}
			}

			/* Skip pager */
			if (TEST_HINT(hints, HINT_SKIP_PAGER)) {
				/* fprintf(stderr, PACKAGE_NAME": hint - skip_pager\n");
				   fflush(stderr); */

				xa = ATOM(_NET_WM_STATE);
				if (xa != None) {
					Atom xa_prop = ATOM(_NET_WM_STATE_SKIP_PAGER);

					XChangeProperty(display, window.window, xa, XA_ATOM, 32,
							PropModeAppend, (unsigned char *) &xa_prop, 1);
				}
			}
		}

		fprintf(stderr, PACKAGE_NAME": drawing to created window (0x%lx)\n",
				window.window);
		fflush(stderr);

		XMapWindow(display, window.window);

	} else
#endif /* OWN_WINDOW */
	{
		XWindowAttributes attrs;

		if (!window.window) {
			window.window = find_desktop_window(&window.root, &window.desktop);
		}
		window.visual = DefaultVisual(display, screen);
		window.colourmap = DefaultColormap(display, screen);

		if (XGetWindowAttributes(display, window.window, &attrs)) {
			window.width = attrs.width;
			window.height = attrs.height;
		}

		fprintf(stderr, PACKAGE_NAME": drawing to desktop window\n");
	}

	/* Drawable is same as window. This may be changed by double buffering. */
	window.drawable = window.window;

	XFlush(display);

	XSelectInput(display, window.window, ExposureMask | PropertyChangeMask
#ifdef OWN_WINDOW
			| (own_window.get(l) ? (StructureNotifyMask |
					ButtonPressMask | ButtonReleaseMask) : 0)
#endif
			);
}

static Window find_subwindow(Window win, int w, int h)
{
	unsigned int i, j;
	Window troot, parent, *children;
	unsigned int n;

	/* search subwindows with same size as display or work area */

	for (i = 0; i < 10; i++) {
		XQueryTree(display, win, &troot, &parent, &children, &n);

		for (j = 0; j < n; j++) {
			XWindowAttributes attrs;

			if (XGetWindowAttributes(display, children[j], &attrs)) {
				/* Window must be mapped and same size as display or
				 * work space */
				if (attrs.map_state != 0 && ((attrs.width == display_width
								&& attrs.height == display_height)
							|| (attrs.width == w && attrs.height == h))) {
					win = children[j];
					break;
				}
			}
		}

		XFree(children);
		if (j == n) {
			break;
		}
	}

	return win;
}

void create_gc(void)
{
	XGCValues values;

	values.graphics_exposures = 0;
	values.function = GXcopy;
	window.gc = XCreateGC(display, window.drawable,
			GCFunction | GCGraphicsExposures, &values);
}

//Get current desktop number
static inline void get_x11_desktop_current(Display *current_display, Window root, Atom atom)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char *prop = NULL;
	struct information *current_info = &info;

	if (atom == None) return;

	if ( (XGetWindowProperty( current_display, root, atom,
					0, 1L, False, XA_CARDINAL,
					&actual_type, &actual_format, &nitems,
					&bytes_after, &prop ) == Success ) &&
			(actual_type == XA_CARDINAL) &&
			(nitems == 1L) && (actual_format == 32) ) {
		current_info->x11.desktop.current = prop[0]+1;
	}
	if(prop) {
		XFree(prop);
	}
}

//Get total number of available desktops
static inline void get_x11_desktop_number(Display *current_display, Window root, Atom atom)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char *prop = NULL;
	struct information *current_info = &info;

	if (atom == None) return;

	if ( (XGetWindowProperty( current_display, root, atom,
					0, 1L, False, XA_CARDINAL,
					&actual_type, &actual_format, &nitems,
					&bytes_after, &prop ) == Success ) &&
			(actual_type == XA_CARDINAL) &&
			(nitems == 1L) && (actual_format == 32) ) {
		current_info->x11.desktop.number = prop[0];
	}
	if(prop) {
		XFree(prop);
	}
}

//Get all desktop names
static inline void get_x11_desktop_names(Display *current_display, Window root, Atom atom)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char *prop = NULL;
	struct information *current_info = &info;

	if (atom == None) return;

	if ( (XGetWindowProperty( current_display, root, atom,
					0, (~0L), False, ATOM(UTF8_STRING),
					&actual_type, &actual_format, &nitems,
					&bytes_after, &prop ) == Success ) &&
			(actual_type == ATOM(UTF8_STRING)) &&
			(nitems > 0L) && (actual_format == 8) ) {

		current_info->x11.desktop.all_names.assign(reinterpret_cast<const char *>(prop), nitems);
	}
	if(prop) {
		XFree(prop);
	}
}

//Get current desktop name
static inline void get_x11_desktop_current_name(const std::string &names)
{
	struct information *current_info = &info;
	unsigned int i = 0, j = 0;
	int k = 0;

	while ( i < names.size() ) {
		if ( names[i++] == '\0' ) {
			if ( ++k == current_info->x11.desktop.current ) {
				current_info->x11.desktop.name.assign(names.c_str()+j);
				break;
			}
			j = i;
		}
	}
}

void get_x11_desktop_info(Display *current_display, Atom atom)
{
	Window root;
	static Atom atom_current, atom_number, atom_names;
	struct information *current_info = &info;
	XWindowAttributes window_attributes;

	root = RootWindow(current_display, current_info->x11.monitor.current);

	/* Check if we initialise else retrieve changed property */
	if (atom == 0) {
		atom_current = XInternAtom(current_display, "_NET_CURRENT_DESKTOP", True);
		atom_number  = XInternAtom(current_display, "_NET_NUMBER_OF_DESKTOPS", True);
		atom_names   = XInternAtom(current_display, "_NET_DESKTOP_NAMES", True);
		get_x11_desktop_current(current_display, root, atom_current);
		get_x11_desktop_number(current_display, root, atom_number);
		get_x11_desktop_names(current_display, root, atom_names);
		get_x11_desktop_current_name(current_info->x11.desktop.all_names);

		/* Set the PropertyChangeMask on the root window, if not set */
		XGetWindowAttributes(display, root, &window_attributes);
		if (!(window_attributes.your_event_mask & PropertyChangeMask)) {
			XSetWindowAttributes attributes;
			attributes.event_mask = window_attributes.your_event_mask | PropertyChangeMask;
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

void print_monitor(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;

	if(not out_to_x.get(*state)) {
		strncpy(p, NOT_IN_X, p_max_size);
		return;
	}
	snprintf(p, p_max_size, "%d", XDefaultScreen(display));
}

void print_monitor_number(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;

	if(not out_to_x.get(*state)) {
		strncpy(p, NOT_IN_X, p_max_size);
		return;
	}
	snprintf(p, p_max_size, "%d", XScreenCount(display));
}

void print_desktop(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;

	if(not out_to_x.get(*state)) {
		strncpy(p, NOT_IN_X, p_max_size);
		return;
	}
	snprintf(p, p_max_size, "%d", info.x11.desktop.current);
}

void print_desktop_number(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;

	if(not out_to_x.get(*state)) {
		strncpy(p, NOT_IN_X, p_max_size);
		return;
	}
	snprintf(p, p_max_size, "%d", info.x11.desktop.number);
}

void print_desktop_name(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;

	if(not out_to_x.get(*state)) {
		strncpy(p, NOT_IN_X, p_max_size);
	} else {
		strncpy(p, info.x11.desktop.name.c_str(), p_max_size);
	}
}

#ifdef OWN_WINDOW
/* reserve window manager space */
void set_struts(int sidenum)
{
	Atom strut;
	if ((strut = ATOM(_NET_WM_STRUT)) != None) {
		/* reserve space at left, right, top, bottom */
		signed long sizes[12] = {0};
		int i;

		/* define strut depth */
		switch (sidenum) {
			case 0:
				/* left side */
				sizes[0] = window.x + window.width;
				break;
			case 1:
				/* right side */
				sizes[1] = display_width - window.x;
				break;
			case 2:
				/* top side */
				sizes[2] = window.y + window.height;
				break;
			case 3:
				/* bottom side */
				sizes[3] = display_height - window.y;
				break;
		}

		/* define partial strut length */
		if (sidenum <= 1) {
			sizes[4 + (sidenum*2)] = window.y;
			sizes[5 + (sidenum*2)] = window.y + window.height;
		} else if (sidenum <= 3) {
			sizes[4 + (sidenum*2)] = window.x;
			sizes[5 + (sidenum*2)] = window.x + window.width;
		}

		/* check constraints */
		for (i = 0; i < 12; i++) {
			if (sizes[i] < 0) {
				sizes[i] = 0;
			} else {
				if (i <= 1 || i >= 8) {
					if (sizes[i] > display_width) {
						sizes[i] = display_width;
					}
				} else {
					if (sizes[i] > display_height) {
						sizes[i] = display_height;
					}
				}
			}
		}

		XChangeProperty(display, window.window, strut, XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *) &sizes, 4);

		if ((strut = ATOM(_NET_WM_STRUT_PARTIAL)) != None) {
			XChangeProperty(display, window.window, strut, XA_CARDINAL, 32,
					PropModeReplace, (unsigned char *) &sizes, 12);
		}
	}
}
#endif /* OWN_WINDOW */

#ifdef BUILD_XDBE
void xdbe_swap_buffers(void)
{
	if (use_xdbe.get(*state)) {
		XdbeSwapInfo swap;

		swap.swap_window = window.window;
		swap.swap_action = XdbeBackground;
		XdbeSwapBuffers(display, &swap, 1);
	}
}
#else
void xpmdb_swap_buffers(void)
{
	if (use_xpmdb.get(*state)) {
		XCopyArea(display, window.back_buffer, window.window, window.gc, 0, 0, window.width, window.height, 0, 0);
		XSetForeground(display, window.gc, 0);
		XFillRectangle(display, window.drawable, window.gc, 0, 0, window.width, window.height);
		XFlush(display);
	}
}
#endif /* BUILD_XDBE */
