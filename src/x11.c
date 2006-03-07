/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */


#include "conky.h"

#ifdef X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef XFT
#include <X11/Xft/Xft.h>
#endif

#include <stdio.h>

#ifdef XDBE
int use_xdbe;
#endif

#ifdef XFT
int use_xft = 0;
#endif

#define WINDOW_NAME_FMT "%s - conky" 

/* some basic X11 stuff */
Display *display;
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
static void update_workarea();
static Window find_desktop_window();
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

static Window find_desktop_window()
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
				"Conky: desktop window (%lx) found from __SWM_VROOT property\n", win);
			fflush(stderr);
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
			"Conky: desktop window (%lx) is subwindow of root window (%lx)\n",win,root);
	else
		fprintf(stderr, "Conky: desktop window (%lx) is root window\n",win);

	fflush(stderr);
	return win;
}

/* sets background to ParentRelative for the Window and all parents */
inline void set_transparent_background(Window win)
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
	//XClearWindow(display, win); not sure why this was here
}

void init_window(int own_window, int w, int h, int set_trans, int back_colour, char * nodename, 
		 char **argv, int argc)
{
	/* There seems to be some problems with setting transparent background (on
	 * fluxbox this time). It doesn't happen always and I don't know why it
	 * happens but I bet the bug is somewhere here. */
	set_transparent = set_trans;
	background_colour = back_colour;

	nodename = (char *)nodename;

#ifdef OWN_WINDOW
	if (own_window) {
		{
			/* Allow WM control of conky again.  Shielding conky from the WM 
			 * via override redirect creates more problems than it's worth and
			 * makes it impossible to use tools like devilspie to manage the
			 * conky windows beyond the parametsrs we offer.  Also, button 
			 * press events are now explicitly forwarded to the root window. */
			XSetWindowAttributes attrs = {
				ParentRelative,0L,0,0L,0,0,Always,0L,0L,False,
				StructureNotifyMask|ExposureMask|ButtonPressMask,
				0L,False,0,0 };

			XClassHint classHint;
			XWMHints wmHint;
			Atom xa;
			char window_title[256];

			window.root = find_desktop_window();

			window.window = XCreateWindow(display, window.root, 
					      	      window.x, window.y, w, h, 0, 
						      CopyFromParent,
						      InputOutput,
						      CopyFromParent,
						      CWBackPixel|CWOverrideRedirect,
						      &attrs);

			fprintf(stderr, "Conky: drawing to created window (%lx)\n", window.window);
			fflush(stderr);

			classHint.res_name = window.wm_class_name;
			classHint.res_class = classHint.res_name;

			wmHint.flags = InputHint | StateHint;
			wmHint.input = False;
			wmHint.initial_state = NormalState;

			sprintf(window_title,WINDOW_NAME_FMT,nodename);

			XmbSetWMProperties (display, window.window, window_title, NULL, 
					    argv, argc,
					    NULL, &wmHint, &classHint);

			/* Sets an empty WM_PROTOCOLS property */
			XSetWMProtocols(display,window.window,NULL,0);


			/* Set NORMAL window type for _NET_WM_WINDOW_TYPE. */
			xa = ATOM(_NET_WM_WINDOW_TYPE);
			if (xa != None) {
				Atom prop = ATOM(_NET_WM_WINDOW_TYPE_NORMAL);
				XChangeProperty(display, window.window, xa,
						XA_ATOM, 32,
						PropModeReplace,
						(unsigned char *) &prop,
						1);
			}

			/* Set desired hints */
			
			/* Window decorations */
			if (TEST_HINT(window.hints,HINT_UNDECORATED)) {
			    fprintf(stderr, "Conky: hint - undecorated\n"); fflush(stderr);

			    xa = ATOM(_MOTIF_WM_HINTS);
			    if (xa != None) {
				long prop[5] = { 2, 0, 0, 0, 0 };
				XChangeProperty(display, window.window, xa,
						xa, 32, PropModeAppend,
						(unsigned char *) prop, 5);
			    }
			}

			/* Below other windows */
			if (TEST_HINT(window.hints,HINT_BELOW)) {
			    fprintf(stderr, "Conky: hint - below\n"); fflush(stderr);

         		    xa = ATOM(_WIN_LAYER);
         		    if (xa != None) {
            			long prop = 0;
            			XChangeProperty(display, window.window, xa,
            					XA_CARDINAL, 32,
            					PropModeAppend,
            					(unsigned char *) &prop, 1);
			    }
			
			    xa = ATOM(_NET_WM_STATE);
			    if (xa != None) {
				Atom xa_prop = ATOM(_NET_WM_STATE_BELOW);
				XChangeProperty(display, window.window, xa,
					XA_ATOM, 32,
					PropModeAppend,
					(unsigned char *) &xa_prop,
					1);
			    }
			}

			/* Above other windows */
			if (TEST_HINT(window.hints,HINT_ABOVE)) {
                            fprintf(stderr, "Conky: hint - above\n"); fflush(stderr);

                            xa = ATOM(_WIN_LAYER);
                            if (xa != None) {
                                long prop = 6;
                                XChangeProperty(display, window.window, xa,
                                                XA_CARDINAL, 32,
                                                PropModeAppend,
                                                (unsigned char *) &prop, 1);
                            }

                            xa = ATOM(_NET_WM_STATE);
                            if (xa != None) {
                                Atom xa_prop = ATOM(_NET_WM_STATE_ABOVE);
                                XChangeProperty(display, window.window, xa,
                                        XA_ATOM, 32,
                                        PropModeAppend,
                                        (unsigned char *) &xa_prop,
                                        1);
                            }
                        }

			/* Sticky */
			if (TEST_HINT(window.hints,HINT_STICKY)) {
                            fprintf(stderr, "Conky: hint - sticky\n"); fflush(stderr);

                            xa = ATOM(_NET_WM_STATE);
                            if (xa != None) {
                                Atom xa_prop = ATOM(_NET_WM_STATE_STICKY);
                                XChangeProperty(display, window.window, xa,
                                        XA_ATOM, 32,
                                        PropModeAppend,
                                        (unsigned char *) &xa_prop,
                                        1);
                            }
                        }

			/* Skip taskbar */
                        if (TEST_HINT(window.hints,HINT_SKIP_TASKBAR)) {
                            fprintf(stderr, "Conky: hint - skip_taskbar\n"); fflush(stderr);

                            xa = ATOM(_NET_WM_STATE);
                            if (xa != None) {
                                Atom xa_prop = ATOM(_NET_WM_STATE_SKIP_TASKBAR);
                                XChangeProperty(display, window.window, xa,
                                        XA_ATOM, 32,
                                        PropModeAppend,
                                        (unsigned char *) &xa_prop,
                                        1);
                            }
                        }

			/* Skip pager */
                        if (TEST_HINT(window.hints,HINT_SKIP_PAGER)) {
                            fprintf(stderr, "Conky: hint - skip_pager\n"); fflush(stderr);

                            xa = ATOM(_NET_WM_STATE);
                            if (xa != None) {
                                Atom xa_prop = ATOM(_NET_WM_STATE_SKIP_PAGER);
                                XChangeProperty(display, window.window, xa,
                                        XA_ATOM, 32,
                                        PropModeAppend,
                                        (unsigned char *) &xa_prop,
                                        1);
                            }
                        }

			XMapWindow(display, window.window);

		}
	} else
#endif
		/* root / desktop window */
	{
		XWindowAttributes attrs;

		if (!window.window)
			window.window = find_desktop_window();

		if (XGetWindowAttributes(display, window.window, &attrs)) {
			window.width = attrs.width;
			window.height = attrs.height;
		}

		fprintf(stderr, "Conky: drawing to desktop window\n");
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

	/*set_transparent_background(window.window); must be done after double buffer stuff? */
#ifdef OWN_WINDOW
	/*if (own_window) {
	set_transparent_background(window.window);
		XClearWindow(display, window.window);
}*/
#endif

	XSelectInput(display, window.window, ExposureMask
#ifdef OWN_WINDOW
		     | (own_window
			? (StructureNotifyMask | PropertyChangeMask | ButtonPressMask) : 0)
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
		/* lets check if it's a hex colour with the # missing in front
		 * if yes, then do something about it
		 */
		char newname[64];
		newname[0] = '#';
		strncpy(&newname[1], name, 62);
		/* now lets try again */
		if (!XParseColor(display, DefaultColormap(display, screen), &newname[0], &color)) {
			ERR("can't parse X color '%s'", name);
			return 0xFF00FF;
		}
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

#endif /* X11 */
