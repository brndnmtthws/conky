/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
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
 * vim: ts=4 sw=4 noet ai cindent syntax=c
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
#ifdef IMLIB2
#include "imlib2.h"
#endif /* IMLIB2 */

#ifdef XFT
#include <X11/Xft/Xft.h>
int use_xft = 0;
#endif

#ifdef HAVE_XDBE
int use_xdbe;
#endif

/* some basic X11 stuff */
Display *display = NULL;
int display_width;
int display_height;
int screen;
static int set_transparent;
static int background_colour;

/* workarea from _NET_WORKAREA, this is where window / text is aligned */
int workarea[4];

/* Window stuff */
struct conky_window window;

/* local prototypes */
static void update_workarea(void);
static Window find_desktop_window(Window *p_root, Window *p_desktop);
static Window find_subwindow(Window win, int w, int h);

/* X11 initializer */
void init_X11(const char *disp)
{
	if (!display) {
		if ((display = XOpenDisplay(disp)) == NULL) {
			CRIT_ERR(NULL, NULL, "can't open display: %s", XDisplayName(disp));
		}
	}

	screen = DefaultScreen(display);
	display_width = DisplayWidth(display, screen);
	display_height = DisplayHeight(display, screen);

	get_x11_desktop_info(display, 0);

	update_workarea();
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

/* sets background to ParentRelative for the Window and all parents */
void set_transparent_background(Window win)
{
	static int colour_set = -1;

	if (set_transparent) {
		Window parent = win;
		unsigned int i;

		for (i = 0; i < 50 && parent != RootWindow(display, screen); i++) {
			Window r, *children;
			unsigned int n;

			XSetWindowBackgroundPixmap(display, parent, ParentRelative);

			XQueryTree(display, parent, &r, &parent, &children, &n);
			XFree(children);
		}
	} else if (colour_set != background_colour) {
		XSetWindowBackground(display, win, background_colour);
		colour_set = background_colour;
	}
}

void destroy_window(void)
{
	if(window.xftdraw) {
		XftDrawDestroy(window.xftdraw);
	}
	if(window.gc) {
		XFreeGC(display, window.gc);
	}
	memset(&window, 0, sizeof(struct conky_window));
}

void init_window(int own_window, int w, int h, int set_trans, int back_colour,
		char **argv, int argc)
{
	/* There seems to be some problems with setting transparent background
	 * (on fluxbox this time). It doesn't happen always and I don't know why it
	 * happens but I bet the bug is somewhere here. */
	set_transparent = set_trans;
	background_colour = back_colour;

#ifdef OWN_WINDOW
	if (own_window) {
		if (!find_desktop_window(&window.root, &window.desktop)) {
			return;
		}

		if (window.type == TYPE_OVERRIDE) {

			/* An override_redirect True window.
			 * No WM hints or button processing needed. */
			XSetWindowAttributes attrs = { ParentRelative, 0L, 0, 0L, 0, 0,
				Always, 0L, 0L, False, StructureNotifyMask | ExposureMask, 0L,
				True, 0, 0 };

			/* Parent is desktop window (which might be a child of root) */
			window.window = XCreateWindow(display, window.desktop, window.x,
					window.y, w, h, 0, CopyFromParent, InputOutput, CopyFromParent,
					CWBackPixel | CWOverrideRedirect, &attrs);

			XLowerWindow(display, window.window);

			fprintf(stderr, PACKAGE_NAME": window type - override\n");
			fflush(stderr);
		} else { /* window.type != TYPE_OVERRIDE */

			/* A window managed by the window manager.
			 * Process hints and buttons. */
			XSetWindowAttributes attrs = { ParentRelative, 0L, 0, 0L, 0, 0,
				Always, 0L, 0L, False, StructureNotifyMask | ExposureMask |
					ButtonPressMask | ButtonReleaseMask, 0L, False, 0, 0 };

			XClassHint classHint;
			XWMHints wmHint;
			Atom xa;

			if (window.type == TYPE_DOCK) {
				window.x = window.y = 0;
			}
			/* Parent is root window so WM can take control */
			window.window = XCreateWindow(display, window.root, window.x,
					window.y, w, h, 0, CopyFromParent, InputOutput, CopyFromParent,
					CWBackPixel | CWOverrideRedirect, &attrs);

			classHint.res_name = window.class_name;
			classHint.res_class = classHint.res_name;

			wmHint.flags = InputHint | StateHint;
			/* allow decorated windows to be given input focus by WM */
			wmHint.input =
				TEST_HINT(window.hints, HINT_UNDECORATED) ? False : True;
			if (window.type == TYPE_DOCK || window.type == TYPE_PANEL) {
				wmHint.initial_state = WithdrawnState;
			} else {
				wmHint.initial_state = NormalState;
			}

			XmbSetWMProperties(display, window.window, window.title, NULL, argv,
					argc, NULL, &wmHint, &classHint);

			/* Sets an empty WM_PROTOCOLS property */
			XSetWMProtocols(display, window.window, NULL, 0);

			/* Set window type */
			if ((xa = ATOM(_NET_WM_WINDOW_TYPE)) != None) {
				Atom prop;

				switch (window.type) {
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
			if (TEST_HINT(window.hints, HINT_UNDECORATED)) {
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
			if (TEST_HINT(window.hints, HINT_BELOW)) {
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
			if (TEST_HINT(window.hints, HINT_ABOVE)) {
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
			if (TEST_HINT(window.hints, HINT_STICKY)) {
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
			if (TEST_HINT(window.hints, HINT_SKIP_TASKBAR)) {
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
			if (TEST_HINT(window.hints, HINT_SKIP_PAGER)) {
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

		if (XGetWindowAttributes(display, window.window, &attrs)) {
			window.width = attrs.width;
			window.height = attrs.height;
		}

		fprintf(stderr, PACKAGE_NAME": drawing to desktop window\n");
	}

	/* Drawable is same as window. This may be changed by double buffering. */
	window.drawable = window.window;

#ifdef HAVE_XDBE
	if (use_xdbe) {
		int major, minor;

		if (!XdbeQueryExtension(display, &major, &minor)) {
			use_xdbe = 0;
		} else {
			window.back_buffer = XdbeAllocateBackBufferName(display,
					window.window, XdbeBackground);
			if (window.back_buffer != None) {
				window.drawable = window.back_buffer;
				fprintf(stderr, PACKAGE_NAME": drawing to double buffer\n");
			} else {
				use_xdbe = 0;
			}
		}
		if (!use_xdbe) {
			ERR("failed to set up double buffer");
		}
	}
	if (!use_xdbe) {
		fprintf(stderr, PACKAGE_NAME": drawing to single buffer\n");
	}
#endif
	window.visual = DefaultVisual(display, DefaultScreen(display));
	window.colourmap = DefaultColormap(display, DefaultScreen(display));
#ifdef IMLIB2
	{
		cimlib_init(display, window.drawable, window.visual, window.colourmap);
	}
#endif /* IMLIB2 */
	XFlush(display);

	XSelectInput(display, window.window, ExposureMask | PropertyChangeMask
#ifdef OWN_WINDOW
			| (own_window ? (StructureNotifyMask |
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

long get_x11_color(const char *name)
{
	XColor color;

	color.pixel = 0;
	if (!XParseColor(display, DefaultColormap(display, screen), name, &color)) {
		/* lets check if it's a hex colour with the # missing in front
		 * if yes, then do something about it */
		char newname[DEFAULT_TEXT_BUFFER_SIZE];

		newname[0] = '#';
		strncpy(&newname[1], name, DEFAULT_TEXT_BUFFER_SIZE - 1);
		/* now lets try again */
		if (!XParseColor(display, DefaultColormap(display, screen), &newname[0],
					&color)) {
			ERR("can't parse X color '%s'", name);
			return 0xFF00FF;
		}
	}
	if (!XAllocColor(display, DefaultColormap(display, screen), &color)) {
		ERR("can't allocate X color '%s'", name);
	}

	return (long) color.pixel;
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

		if(current_info->x11.desktop.all_names) {
			free(current_info->x11.desktop.all_names);
			current_info->x11.desktop.all_names = NULL;
		}
		current_info->x11.desktop.all_names = malloc(nitems*sizeof(char));
		memcpy(current_info->x11.desktop.all_names, prop, nitems);
		current_info->x11.desktop.nitems = nitems;
	}
	if(prop) {
		XFree(prop);
	}
}

//Get current desktop name
static inline void get_x11_desktop_current_name(char *names)
{
	struct information *current_info = &info;
	unsigned int i = 0, j = 0;
	int k = 0;

	while ( i < current_info->x11.desktop.nitems ) {
		if ( names[i++] == '\0' ) {
			if ( ++k == current_info->x11.desktop.current ) {
				if (current_info->x11.desktop.name) {
					free(current_info->x11.desktop.name);
					current_info->x11.desktop.name = NULL;
				}
				current_info->x11.desktop.name = malloc((i-j)*sizeof(char));
				//desktop names can be empty but should always be not null
				strcpy( current_info->x11.desktop.name, (char *)&names[j] );
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

void update_x11info(void)
{
	struct information *current_info = &info;
	current_info->x11.monitor.number = XScreenCount(display);
	current_info->x11.monitor.current = XDefaultScreen(display);
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
