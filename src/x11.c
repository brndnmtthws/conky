/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */

#include "conky.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef XFT
#include <X11/Xft/Xft.h>
#endif

#ifdef XDBE
int use_xdbe;
#endif

#ifdef XFT
int use_xft = 0;
#endif

/* some basic X11 stuff */
Display *display;
int display_width;
int display_height;
int screen;

/* workarea from _NET_WORKAREA, this is where window / text is aligned */
int workarea[4];

/* Window stuff */
struct conky_window window;

/* local prototypes */
static void update_workarea();
static Window find_window_to_draw();
static Window find_subwindow(Window win, int w, int h);

/* X11 initializer */
void init_X11()
{
	if ((display = XOpenDisplay(0)) == NULL)
		CRIT_ERR("can't open display: %s", XDisplayName(0));

	screen = DefaultScreen(display);
	display_width = DisplayWidth(display, screen);
	display_height = DisplayHeight(display, screen);

	update_workarea();
}

static void update_workarea()
{
	Window root = RootWindow(display, screen);
	unsigned long nitems, bytes;
	unsigned char *buf = NULL;
	Atom type;
	int format;

	/* default work area is display */
	workarea[0] = 0;
	workarea[1] = 0;
	workarea[2] = display_width;
	workarea[3] = display_height;

	/* get current desktop */
	if (XGetWindowProperty(display, root, ATOM(_NET_CURRENT_DESKTOP),
			       0, 1, False, XA_CARDINAL, &type, &format,
			       &nitems, &bytes, &buf) == Success
	    && type == XA_CARDINAL && nitems > 0) {

		//Currently unused 
		/*  long desktop = * (long *) buf; */

		XFree(buf);
		buf = 0;

	}

	if (buf) {
		XFree(buf);
		buf = 0;
	}
}

static Window find_window_to_draw()
{
	Atom type;
	int format, i;
	unsigned long nitems, bytes;
	unsigned int n;
	Window root = RootWindow(display, screen);
	Window win = root;
	Window troot, parent, *children;
	unsigned char *buf = NULL;

	/* some window managers set __SWM_VROOT to some child of root window */

	XQueryTree(display, root, &troot, &parent, &children, &n);
	for (i = 0; i < (int) n; i++) {
		if (XGetWindowProperty
		    (display, children[i], ATOM(__SWM_VROOT), 0, 1, False,
		     XA_WINDOW, &type, &format, &nitems, &bytes,
		     &buf) == Success && type == XA_WINDOW) {
			win = *(Window *) buf;
			XFree(buf);
			XFree(children);
			fprintf(stderr,
				"Conky: drawing to window from __SWM_VROOT property\n");
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

	if (win != root)
		fprintf(stderr,
			"Conky: drawing to subwindow of root window (%lx)\n",
			win);
	else
		fprintf(stderr, "Conky: drawing to root window\n");

	return win;
}

/* sets background to ParentRelative for the Window and all parents */
void set_transparent_background(Window win)
{
	Window parent = win;
	unsigned int i;
	for (i = 0; i < 16 && parent != RootWindow(display, screen); i++) {
		Window r, *children;
		unsigned int n;

		XSetWindowBackgroundPixmap(display, parent,
					   ParentRelative);

		XQueryTree(display, parent, &r, &parent, &children, &n);
		XFree(children);
	}
	XClearWindow(display, win);
}

#if defined OWN_WINDOW
void init_window(int own_window, int w, int h, int l, int fixed_pos)
#else
void init_window(int own_window, int w, int h, int l)
#endif
{
	/* There seems to be some problems with setting transparent background (on
	 * fluxbox this time). It doesn't happen always and I don't know why it
	 * happens but I bet the bug is somewhere here. */
#ifdef OWN_WINDOW
	if (own_window) {
		/* looks like root pixmap isn't needed for anything */
		{
			XSetWindowAttributes attrs;
			XClassHint class_hints;

			/* just test color
			attrs.background_pixel = get_x11_color("green");
			*/

			window.window = XCreateWindow(display, RootWindow(display, screen), window.x, window.y, w, h, 0, CopyFromParent,	/* depth */
						      CopyFromParent,	/* class */
						      CopyFromParent,	/* visual */
						      CWBackPixel, &attrs);

			class_hints.res_class = "conky";
			class_hints.res_name = "conky";
			XSetClassHint(display, window.window,
				      &class_hints);

			set_transparent_background(window.window);

			XStoreName(display, window.window, "conky");

			XClearWindow(display, window.window);

			if (!fixed_pos)
				XMoveWindow(display, window.window, window.x,
					    window.y);
		}

		{
			/* turn off decorations */
			Atom a =
			    XInternAtom(display, "_MOTIF_WM_HINTS", True);
			if (a != None) {
				long prop[5] = { 2, 0, 0, 0, 0 };
				XChangeProperty(display, window.window, a,
						a, 32, PropModeReplace,
						(unsigned char *) prop, 5);
			}

			/* set window sticky (to all desktops) */
			a = XInternAtom(display, "_NET_WM_DESKTOP", True);
			if (a != None) {
				long prop = 0xFFFFFFFF;
				XChangeProperty(display, window.window, a,
						XA_CARDINAL, 32,
						PropModeReplace,
						(unsigned char *) &prop,
						1);
			}
			if(l) {
			/* make sure the layer is on the bottom */
         a = XInternAtom(display, "_WIN_LAYER", True);
         if (a != None) {
            long prop = 0;
            XChangeProperty(display, window.window, a,
            XA_CARDINAL, 32,
            PropModeReplace,
            (unsigned char *) &prop, 1);
         }
			}
		}

		XMapWindow(display, window.window);
	} else
#endif
		/* root / desktop window */
	{
		XWindowAttributes attrs;

		if (!window.window)
			window.window = find_window_to_draw();

		if (XGetWindowAttributes(display, window.window, &attrs)) {
			window.width = attrs.width;
			window.height = attrs.height;
		}
	}

	/* Drawable is same as window. This may be changed by double buffering. */
	window.drawable = window.window;

#ifdef XDBE
	if (use_xdbe) {
		int major, minor;
		if (!XdbeQueryExtension(display, &major, &minor)) {
			use_xdbe = 0;
		} else {
			window.back_buffer =
			    XdbeAllocateBackBufferName(display,
						       window.window,
						       XdbeBackground);
			if (window.back_buffer != None) {
				window.drawable = window.back_buffer;
				fprintf(stderr,
					"Conky: drawing to double buffer\n");
			} else
				use_xdbe = 0;
		}
		if (!use_xdbe)
			ERR("failed to set up double buffer");
	}
	if (!use_xdbe)
		fprintf(stderr, "Conky: drawing to single buffer\n");
#endif

	XFlush(display);

	/* set_transparent_background() must be done after double buffer stuff? */
#ifdef OWN_WINDOW
	if (own_window) {
		set_transparent_background(window.window);
		XClearWindow(display, window.window);
	}
#endif

	XSelectInput(display, window.window, ExposureMask
#ifdef OWN_WINDOW
		     | (own_window
			? (StructureNotifyMask | PropertyChangeMask) : 0)
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

			if (XGetWindowAttributes
			    (display, children[j], &attrs)) {
				/* Window must be mapped and same size as display or work space */
				if (attrs.map_state != 0 &&
				    ((attrs.width == display_width
				      && attrs.height == display_height)
				     || (attrs.width == w
					 && attrs.height == h))) {
					win = children[j];
					break;
				}
			}
		}

		XFree(children);
		if (j == n)
			break;
	}

	return win;
}

long get_x11_color(const char *name)
{
	XColor color;
	color.pixel = 0;
	if (!XParseColor
	    (display, DefaultColormap(display, screen), name, &color)) {
		ERR("can't parse X color '%s'", name);
		return 0xFF00FF;
	}
	if (!XAllocColor
	    (display, DefaultColormap(display, screen), &color))
		ERR("can't allocate X color '%s'", name);

	return (long) color.pixel;
}

void create_gc()
{
	XGCValues values;
	values.graphics_exposures = 0;
	values.function = GXcopy;
	window.gc = XCreateGC(display, window.drawable,
			      GCFunction | GCGraphicsExposures, &values);
}
