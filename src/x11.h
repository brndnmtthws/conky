/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2010 Brenden Matthews, Philip Kovacs, et. al.
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

#ifdef BUILD_X11
#ifndef X11_H_
#define X11_H_

#include <X11/Xlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#ifdef BUILD_XFT
#include <X11/Xft/Xft.h>
#endif

#ifdef BUILD_XDBE
#include <X11/extensions/Xdbe.h>
#endif

#include "setting.hh"

#define ATOM(a) XInternAtom(display, #a, False)

#ifdef OWN_WINDOW
enum window_type {
	TYPE_NORMAL = 0,
	TYPE_DOCK,
	TYPE_PANEL,
	TYPE_DESKTOP,
	TYPE_OVERRIDE
};

enum window_hints {
	HINT_UNDECORATED = 0,
	HINT_BELOW,
	HINT_ABOVE,
	HINT_STICKY,
	HINT_SKIP_TASKBAR,
	HINT_SKIP_PAGER
};

#define SET_HINT(mask, hint)	(mask |= (1 << (hint)))
#define TEST_HINT(mask, hint)	(mask & (1 << (hint)))
#endif

struct conky_window {
	Window root, window, desktop;
	Drawable drawable;
	Visual *visual;
	Colormap colourmap;
	GC gc;
	long border_inner_margin, border_outer_margin, border_width;

#ifdef BUILD_XDBE
	XdbeBackBuffer back_buffer;
#endif
#ifdef BUILD_XFT
	XftDraw *xftdraw;
#endif

	int width;
	int height;
#ifdef OWN_WINDOW
	int x;
	int y;
#endif
};

#ifdef BUILD_XDBE
extern int use_xdbe;
#endif

#ifdef BUILD_XFT
extern int use_xft;
#endif

#if defined(BUILD_ARGB) && defined(OWN_WINDOW)
/* true if use_argb_visual=true and argb visual was found*/
extern bool have_argb_visual;
#endif

extern Display *display;
extern int display_width;
extern int display_height;
extern int screen;

extern int workarea[4];

extern struct conky_window window;
extern char window_created;

void init_window(int width, int height, char **argv, int argc);
void destroy_window(void);
void create_gc(void);
void set_transparent_background(Window win);
void get_x11_desktop_info(Display *display, Atom atom);
void set_struts(int);

void print_monitor(struct text_object *, char *, int);
void print_monitor_number(struct text_object *, char *, int);
void print_desktop(struct text_object *, char *, int);
void print_desktop_number(struct text_object *, char *, int);
void print_desktop_name(struct text_object *, char *, int);

#ifdef BUILD_XDBE
void xdbe_swap_buffers(void);
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

extern conky::simple_config_setting<alignment>   text_alignment;

namespace priv {
	class out_to_x_setting: public conky::simple_config_setting<bool> {
		typedef conky::simple_config_setting<bool> Base;
	
	protected:
		virtual void lua_setter(lua::state &l, bool init);
		virtual void cleanup(lua::state &l);

	public:
		out_to_x_setting()
			: Base("out_to_x", false, false)
		{}
	};
}
extern priv::out_to_x_setting                    out_to_x;
extern conky::simple_config_setting<std::string> display_name;

#ifdef OWN_WINDOW
extern conky::simple_config_setting<bool>        own_window;
extern conky::simple_config_setting<bool>        set_transparent;
extern conky::simple_config_setting<std::string> own_window_class;
extern conky::simple_config_setting<std::string> own_window_title;
extern conky::simple_config_setting<window_type> own_window_type;

struct window_hints_traits {
	static const lua::Type type = lua::TSTRING;
	static std::pair<uint16_t, bool> convert(lua::state &l, int index, const std::string &name);
};
extern conky::simple_config_setting<uint16_t, window_hints_traits> own_window_hints;

// this setting is not checked for validity when set, we leave that to the caller
// the reason for that is that we need to have X initialised in order to call XParseColor()
extern conky::simple_config_setting<std::string> background_colour;
#ifdef BUILD_ARGB
extern conky::simple_config_setting<bool>        use_argb_visual;

/* range of 0-255 for alpha */
extern conky::range_config_setting<int>          own_window_argb_value;
#endif
#endif /*OWN_WINDOW*/

#endif /*X11_H_*/
#endif /* BUILD_X11 */
