/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

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

#define ATOM(a) XInternAtom(display, #a, False)

#ifdef OWN_WINDOW
enum _window_type {
	TYPE_NORMAL = 0,
	TYPE_DOCK,
	TYPE_PANEL,
	TYPE_DESKTOP,
	TYPE_OVERRIDE
};

enum _window_hints {
	HINT_UNDECORATED = 0,
	HINT_BELOW,
	HINT_ABOVE,
	HINT_STICKY,
	HINT_SKIP_TASKBAR,
	HINT_SKIP_PAGER
};

#define SET_HINT(mask, hint)	(mask |= (1 << hint))
#define TEST_HINT(mask, hint)	(mask & (1 << hint))
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
	char class_name[256];
	char title[256];
	int x;
	int y;
	unsigned int type;
	unsigned long hints;
#endif
};

#ifdef BUILD_XDBE
extern int use_xdbe;
#endif

#ifdef BUILD_XFT
extern int use_xft;
#endif

#if defined(BUILD_ARGB) && defined(OWN_WINDOW)
/* 1 if config var set to 1, otherwise 0 */
extern int use_argb_visual;
/* 1 if use_argb_visual=1 and argb visual was found, otherwise 0 */
extern int have_argb_visual;
/* range of 0-255 for alpha */
extern int own_window_argb_value;
#endif

extern Display *display;
extern int display_width;
extern int display_height;
extern int screen;

extern int workarea[4];

extern struct conky_window window;
extern char window_created;

void init_X11(const char*);
void init_window(int use_own_window, int width, int height, int set_trans,
	int back_colour, char **argv, int argc);
void destroy_window(void);
void create_gc(void);
void set_transparent_background(Window win, int alpha);
void get_x11_desktop_info(Display *display, Atom atom);
void set_struts(int);

void print_monitor(struct text_object *, char *, int);
void print_monitor_number(struct text_object *, char *, int);
void print_desktop(struct text_object *, char *, int);
void print_desktop_number(struct text_object *, char *, int);
void print_desktop_name(struct text_object *, char *, int);
void free_desktop_info(void);

#ifdef BUILD_XDBE
void xdbe_swap_buffers(void);
#endif /* BUILD_XDBE */

#endif /*X11_H_*/
#endif /* BUILD_X11 */
