/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* Small example demonstrating emulating knockout-groups as in PDF-1.4
 * using cairo_set_operator().
 *
 * Owen Taylor,

 * v0.1  30 November  2002
 * v0.2   1 December  2002 - typo fixes from Keith Packard
 * v0.3  17 April     2003 - Tracking changes in Xr, (Removal of Xr{Push,Pop}Group)
 * v0.4  29 September 2003 - Use cairo_rectangle rather than private rect_path
 *			     Use cairo_arc for oval_path
 * Keeping log of changes in ChangeLog/CVS now. (2003-11-19) Carl Worth
 */
#include "conky.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <math.h>
#include <stdio.h>

/* Fill the given area with checks in the standard style
 * for showing compositing effects.
 */
static void
		fill_checks (cairo_t *cr,
			     int x,     int y,
			     int width, int height)
{
	cairo_surface_t *check;
	cairo_pattern_t *check_pattern;
    
	cairo_save (cr);

#define CHECK_SIZE 32

    check = cairo_surface_create_similar (cairo_current_target_surface (cr),
					  CAIRO_FORMAT_RGB24,
					  2 * CHECK_SIZE, 2 * CHECK_SIZE);
    cairo_surface_set_repeat (check, 1);

    /* Draw the check */
    {
	    cairo_save (cr);

	    cairo_set_target_surface (cr, check);

	    cairo_set_operator (cr, CAIRO_OPERATOR_SRC);

	    cairo_set_rgb_color (cr, 0.4, 0.4, 0.4);

	    cairo_rectangle (cr, 0, 0, 2 * CHECK_SIZE, 2 * CHECK_SIZE);
	    cairo_fill (cr);

	    cairo_set_rgb_color (cr, 0.7, 0.7, 0.7);

	    cairo_rectangle (cr, x, y, CHECK_SIZE, CHECK_SIZE);
	    cairo_fill (cr);
	    cairo_rectangle (cr, x + CHECK_SIZE, y + CHECK_SIZE, CHECK_SIZE, CHECK_SIZE);
	    cairo_fill (cr);

	    cairo_restore (cr);
    }

    /* Fill the whole surface with the check */

    check_pattern = cairo_pattern_create_for_surface (check);
    cairo_set_pattern (cr, check_pattern);
    cairo_rectangle (cr, 0, 0, width, height);
    cairo_fill (cr);

    cairo_pattern_destroy (check_pattern);
    cairo_surface_destroy (check);

    cairo_restore (cr);
}

static void draw_pee (cairo_t *cr, double xc, double yc)
{
	cairo_set_rgb_color (cr, 0, 0, 0);
	cairo_show_text (cr, "Conky");
}

static void
		draw (cairo_t *cr,
		      int      width,
		      int      height)
{
	cairo_surface_t *overlay;

	/* Fill the background */
	double xc = width / 2.;
	double yc = height / 2.;

	overlay = cairo_surface_create_similar (cairo_current_target_surface (cr),
			CAIRO_FORMAT_ARGB32,
			width, height);
	if (overlay == NULL)
		return;

	fill_checks (cr, 0, 0, width, height);

	cairo_save (cr);
	cairo_set_target_surface (cr, overlay);

	cairo_set_alpha (cr, 0.5);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	draw_pee (cr, xc, yc);

	cairo_restore (cr);

	cairo_show_surface (cr, overlay, width, height);

	cairo_surface_destroy (overlay);
}

int
		do_it (void)
{
	Display *dpy;
	int screen;
	Window w;
	Pixmap pixmap;
	char *title = "cairo: Knockout Groups";
	unsigned int quit_keycode;
	int needs_redraw;
	GC gc;
	XWMHints *wmhints;
	XSizeHints *normalhints;
	XClassHint *classhint;
  
	int width = 400;
	int height = 400;
  
	dpy = XOpenDisplay (NULL);
	screen = DefaultScreen (dpy);

	w = XCreateSimpleWindow (dpy, RootWindow (dpy, screen),
				 0, 0, width, height, 0,
				 BlackPixel (dpy, screen), WhitePixel (dpy, screen));

	normalhints = XAllocSizeHints ();
	normalhints->flags = 0;
	normalhints->x = 0;
	normalhints->y = 0;
	normalhints->width = width;
	normalhints->height = height;

	classhint = XAllocClassHint ();
	classhint->res_name = "cairo-knockout";
	classhint->res_class = "Cairo-knockout";
    
	wmhints = XAllocWMHints ();
	wmhints->flags = InputHint;
	wmhints->input = True;
    
	XmbSetWMProperties (dpy, w, title, "cairo-knockout", 0, 0, 
			    normalhints, wmhints, classhint);
	XFree (wmhints);
	XFree (classhint);
	XFree (normalhints);

	pixmap = XCreatePixmap (dpy, w, width, height, DefaultDepth (dpy, screen));
	gc = XCreateGC (dpy, pixmap, 0, NULL);

	quit_keycode = XKeysymToKeycode(dpy, XStringToKeysym("Q"));

	XSelectInput (dpy, w, ExposureMask | StructureNotifyMask | ButtonPressMask | KeyPressMask);
	XMapWindow (dpy, w);
  
	needs_redraw = 1;

	while (1) {
		XEvent xev;

      /* Only do the redraw if there are no events pending.  This
		* avoids us getting behind doing several redraws for several
		* consecutive resize events for example.
      */
		if (!XPending (dpy) && needs_redraw) {
			cairo_t *cr = cairo_create ();

			cairo_set_target_drawable (cr, dpy, pixmap);

			draw (cr, width, height);

			cairo_destroy (cr);

			XCopyArea (dpy, pixmap, w, gc,
				   0, 0,
				   width, height,
				   0, 0);

			needs_redraw = 0;
		}
      
		XNextEvent (dpy, &xev);

		switch (xev.xany.type) {
			case ButtonPress:
				/* A click on the canvas ends the program */
				goto DONE;
			case KeyPress:
				if (xev.xkey.keycode == quit_keycode)
					goto DONE;
				break;
			case ConfigureNotify:
				/* Note new size and create new pixmap. */
				width = xev.xconfigure.width;
				height = xev.xconfigure.height;
				XFreePixmap (dpy, pixmap);
				pixmap = XCreatePixmap (dpy, w, width, height, DefaultDepth (dpy, screen));
				needs_redraw = 1;
				break;
			case Expose:
				XCopyArea (dpy, pixmap, w, gc,
					   xev.xexpose.x, xev.xexpose.y,
					   xev.xexpose.width, xev.xexpose.height,
					   xev.xexpose.x, xev.xexpose.y);
				break;
		}
	}
  DONE:

		  XFreeGC (dpy, gc);
  XCloseDisplay (dpy);

  return 0;
}
