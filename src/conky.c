/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
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
#include "text_object.h"
#include "conky.h"
#include "common.h"
#include "core.h"
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <sys/time.h>
#include <sys/param.h>
#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>
#endif /* HAVE_SYS_INOTIFY_H */
#ifdef X11
#include "x11.h"
#include <X11/Xutil.h>
#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif /* HAVE_XDAMAGE */
#ifdef IMLIB2
#include "imlib2.h"
#endif /* IMLIB2 */
#endif /* X11 */
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <getopt.h>
#ifdef NCURSES
#include <ncurses.h>
#endif
#ifdef XOAP
#include <libxml/parser.h>
#endif /* XOAP */

/* local headers */
#include "obj_create.h"
#include "obj_display.h"
#include "obj_destroy.h"
#include "algebra.h"
#include "build.h"
#include "colours.h"
#include "diskio.h"
#ifdef X11
#include "fonts.h"
#endif
#include "fs.h"
#include "logging.h"
#include "mixer.h"
#include "mail.h"
#include "mboxscan.h"
#include "specials.h"
#include "temphelper.h"
#include "tailhead.h"
#include "top.h"

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "freebsd.h"
#elif defined(__OpenBSD__)
#include "openbsd.h"
#endif

#if defined(__FreeBSD_kernel__)
#include <bsd/bsd.h>
#endif

/* FIXME: apm_getinfo is unused here. maybe it's meant for common.c */
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
		|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
int apm_getinfo(int fd, apm_info_t aip);
char *get_apm_adapter(void);
char *get_apm_battery_life(void);
char *get_apm_battery_time(void);
#endif

#ifdef CONFIG_OUTPUT
#include "defconfig.h"
#include "conf_cookie.h"
#endif

#ifndef S_ISSOCK
#define S_ISSOCK(x)   ((x & S_IFMT) == S_IFSOCK)
#endif

#define MAIL_FILE "$MAIL"
#define MAX_IF_BLOCK_DEPTH 5

//#define SIGNAL_BLOCKING
#undef SIGNAL_BLOCKING

/* debugging level, used by logging.h */
int global_debug_level = 0;

static volatile int g_signal_pending;

int argc_copy;
char** argv_copy;

/* prototypes for internally used functions */
static void signal_handler(int);
static void print_version(void) __attribute__((noreturn));
static void reload_config(void);

static void print_version(void)
{
	printf(PACKAGE_NAME" "VERSION" compiled "BUILD_DATE" for "BUILD_ARCH"\n");

	printf("\nCompiled in features:\n\n"
		   "System config file: "SYSTEM_CONFIG_FILE"\n"
		   "Package library path: "PACKAGE_LIBDIR"\n\n"
#ifdef X11
		   " X11:\n"
# ifdef HAVE_XDAMAGE
		   "  * Xdamage extension\n"
# endif /* HAVE_XDAMAGE */
# ifdef HAVE_XDBE
		   "  * XDBE (double buffer extension)\n"
# endif /* HAVE_XDBE */
# ifdef XFT
		   "  * Xft\n"
# endif /* XFT */
#endif /* X11 */
		   "\n Music detection:\n"
#ifdef AUDACIOUS
		   "  * Audacious\n"
#endif /* AUDACIOUS */
#ifdef BMPX
		   "  * BMPx\n"
#endif /* BMPX */
#ifdef MPD
		   "  * MPD\n"
#endif /* MPD */
#ifdef MOC
		   "  * MOC\n"
#endif /* MOC */
#ifdef XMMS2
		   "  * XMMS2\n"
#endif /* XMMS2 */
		   "\n General:\n"
#ifdef HAVE_OPENMP
		   "  * OpenMP\n"
#endif /* HAVE_OPENMP */
#ifdef MATH
		   "  * math\n"
#endif /* Math */
#ifdef HDDTEMP
		   "  * hddtemp\n"
#endif /* HDDTEMP */
#ifdef TCP_PORT_MONITOR
		   "  * portmon\n"
#endif /* TCP_PORT_MONITOR */
#ifdef HAVE_CURL
		   "  * Curl\n"
#endif /* HAVE_CURL */
#ifdef RSS
		   "  * RSS\n"
#endif /* RSS */
#ifdef WEATHER
		   "  * Weather (METAR)\n"
#ifdef XOAP
		   "  * Weather (XOAP)\n"
#endif /* XOAP */
#endif /* WEATHER */
#ifdef HAVE_IWLIB
		   "  * wireless\n"
#endif /* HAVE_IWLIB */
#ifdef IBM
		   "  * support for IBM/Lenovo notebooks\n"
#endif /* IBM */
#ifdef NVIDIA
		   "  * nvidia\n"
#endif /* NVIDIA */
#ifdef EVE
		   "  * eve-online\n"
#endif /* EVE */
#ifdef CONFIG_OUTPUT
		   "  * config-output\n"
#endif /* CONFIG_OUTPUT */
#ifdef IMLIB2
		   "  * Imlib2\n"
#endif /* IMLIB2 */
#ifdef MIXER_IS_ALSA
		   "  * ALSA mixer support\n"
#endif /* MIXER_IS_ALSA */
#ifdef APCUPSD
		   "  * apcupsd\n"
#endif /* APCUPSD */
#ifdef IOSTATS
		   "  * iostats\n"
#endif /* IOSTATS */
#ifdef HAVE_LUA
		   "  * Lua\n"
		   "\n  Lua bindings:\n"
#ifdef HAVE_LUA_CAIRO
		   "   * Cairo\n"
#endif /* HAVE_LUA_CAIRO */
#ifdef HAVE_LUA_IMLIB2
		   "   * Imlib2\n"
#endif /* IMLIB2 */
#endif /* HAVE_LUA */
	);

	exit(EXIT_SUCCESS);
}

#ifdef HAVE_SYS_INOTIFY_H
int inotify_fd;
#endif

static void main_loop(conky_context *ctx)
{
	int terminate = 0;
#ifdef SIGNAL_BLOCKING
	sigset_t newmask, oldmask;
#endif
	double t;
#ifdef HAVE_SYS_INOTIFY_H
	int inotify_config_wd = -1;
#define INOTIFY_EVENT_SIZE  (sizeof(struct inotify_event))
#define INOTIFY_BUF_LEN     (20 * (INOTIFY_EVENT_SIZE + 16))
	char inotify_buff[INOTIFY_BUF_LEN];
#endif /* HAVE_SYS_INOTIFY_H */


#ifdef SIGNAL_BLOCKING
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGINT);
	sigaddset(&newmask, SIGTERM);
	sigaddset(&newmask, SIGUSR1);
#endif

	ctx->next_update_time = get_time();
	while (terminate == 0 && (ctx->total_run_times == 0 || ctx->info.looped < ctx->total_run_times)) {
		if (ctx->update_interval_bat != NOBATTERY && ctx->update_interval_bat != ctx->update_interval_old) {
			char buf[ctx->max_user_text];

			get_battery_short_status(buf, ctx->max_user_text, "BAT0");
			if(buf[0] == 'D') {
				ctx->update_interval = ctx->update_interval_bat;
			} else {
				ctx->update_interval = ctx->update_interval_old;
			}
		}
		ctx->info.looped++;

#ifdef SIGNAL_BLOCKING
		/* block signals.  we will inspect for pending signals later */
		if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
			CRIT_ERR(NULL, NULL, "unable to sigprocmask()");
		}
#endif

#ifdef X11
		if (ctx->output_methods & TO_X) {
			XFlush(display);

			/* wait for X event or timeout */

			if (!XPending(display)) {
				fd_set fdsr;
				struct timeval tv;
				int s;
				t = ctx->next_update_time - get_time();

				if (t < 0) {
					t = 0;
				} else if (t > ctx->update_interval) {
					t = ctx->update_interval;
				}

				tv.tv_sec = (long) t;
				tv.tv_usec = (long) (t * 1000000) % 1000000;
				FD_ZERO(&fdsr);
				FD_SET(ConnectionNumber(display), &fdsr);

				s = select(ConnectionNumber(display) + 1, &fdsr, 0, 0, &tv);
				if (s == -1) {
					if (errno != EINTR) {
						NORM_ERR("can't select(): %s", strerror(errno));
					}
				} else {
					/* timeout */
					if (s == 0) {
						update_text(ctx);
					}
				}
			}

			if (ctx->need_to_update) {
#ifdef OWN_WINDOW
				int wx = ctx->window.x, wy = ctx->window.y;
#endif

				ctx->need_to_update = 0;
				ctx->selected_font = 0;
				update_text_area(ctx);
#ifdef OWN_WINDOW
				if (ctx->own_window) {
					int changed = 0;

					/* resize ctx->window if it isn't right size */
					if (!ctx->fixed_size
							&& (ctx->text_width + ctx->window.border_inner_margin * 2 + ctx->window.border_outer_margin * 2 + ctx->window.border_width * 2 != ctx->window.width
								|| ctx->text_height + ctx->window.border_inner_margin * 2 + ctx->window.border_outer_margin * 2 + ctx->window.border_width * 2 != ctx->window.height)) {
						ctx->window.width = ctx->text_width + ctx->window.border_inner_margin * 2 + ctx->window.border_outer_margin * 2 + ctx->window.border_width * 2;
						ctx->window.height = ctx->text_height + ctx->window.border_inner_margin * 2 + ctx->window.border_outer_margin * 2 + ctx->window.border_width * 2;
						draw_stuff(ctx); /* redraw everything in our newly sized ctx->window */
						XResizeWindow(display, ctx->window.window, ctx->window.width,
								ctx->window.height); /* resize ctx->window */
						set_transparent_background(ctx->window.window);
#ifdef HAVE_XDBE
						/* swap buffers */
						xdbe_swap_buffers();
#endif

						changed++;
#ifdef HAVE_LUA
						/* update lua ctx->window globals */
						llua_update_window_table(ctx->text_start_x, ctx->text_start_y, ctx->text_width, ctx->text_height);
#endif /* HAVE_LUA */
					}

					/* move ctx->window if it isn't in right position */
					if (!ctx->fixed_pos && (ctx->window.x != wx || ctx->window.y != wy)) {
						XMoveWindow(display, ctx->window.window, ctx->window.x, ctx->window.y);
						changed++;
					}

					/* update struts */
					if (changed && ctx->window.type == TYPE_PANEL) {
						int sidenum = -1;

						fprintf(stderr, PACKAGE_NAME": defining struts\n");
						fflush(stderr);

						switch (ctx->text_alignment) {
							case TOP_LEFT:
							case TOP_RIGHT:
							case TOP_MIDDLE:
								{
									sidenum = 2;
									break;
								}
							case BOTTOM_LEFT:
							case BOTTOM_RIGHT:
							case BOTTOM_MIDDLE:
								{
									sidenum = 3;
									break;
								}
							case MIDDLE_LEFT:
								{
									sidenum = 0;
									break;
								}
							case MIDDLE_RIGHT:
								{
									sidenum = 1;
									break;
								}
						}

						set_struts(sidenum);
					}
				}
#endif

				clear_text(ctx, 1);

#ifdef HAVE_XDBE
				if (use_xdbe) {
					XRectangle r;

					r.x = ctx->text_start_x - ctx->window.border_inner_margin - ctx->window.border_outer_margin - ctx->window.border_width;
					r.y = ctx->text_start_y - ctx->window.border_inner_margin - ctx->window.border_outer_margin - ctx->window.border_width;
					r.width = ctx->text_width + ctx->window.border_inner_margin * 2 + ctx->window.border_outer_margin * 2 + ctx->window.border_width * 2;
					r.height = ctx->text_height + ctx->window.border_inner_margin * 2 + ctx->window.border_outer_margin * 2 + ctx->window.border_width * 2;
					XUnionRectWithRegion(&r, ctx->window.region, ctx->window.region);
				}
#endif
			}

			/* handle X events */
			while (XPending(display)) {
				XEvent ev;

				XNextEvent(display, &ev);
				switch (ev.type) {
					case Expose:
					{
						XRectangle r;
						r.x = ev.xexpose.x;
						r.y = ev.xexpose.y;
						r.width = ev.xexpose.width;
						r.height = ev.xexpose.height;
						XUnionRectWithRegion(&r, ctx->window.region, ctx->window.region);
						break;
					}

					case PropertyNotify:
					{
					        if ( ev.xproperty.state == PropertyNewValue ) {
						        get_x11_desktop_info( ev.xproperty.display, ev.xproperty.atom );
						}
						break;
					}

#ifdef OWN_WINDOW
					case ReparentNotify:
						/* set background to ParentRelative for all parents */
						if (ctx->own_window) {
							set_transparent_background(ctx->window.window);
						}
						break;

					case ConfigureNotify:
						if (ctx->own_window) {
							/* if ctx->window size isn't what expected, set fixed size */
							if (ev.xconfigure.width != ctx->window.width
									|| ev.xconfigure.height != ctx->window.height) {
								if (ctx->window.width != 0 && ctx->window.height != 0) {
									ctx->fixed_size = 1;
								}

								/* clear old stuff before screwing up
								 * size and pos */
								clear_text(ctx, 1);

								{
									XWindowAttributes attrs;
									if (XGetWindowAttributes(display,
											ctx->window.window, &attrs)) {
										ctx->window.width = attrs.width;
										ctx->window.height = attrs.height;
									}
								}

								ctx->text_width = ctx->window.width - ctx->window.border_inner_margin * 2 - ctx->window.border_outer_margin * 2 - ctx->window.border_width * 2;
								ctx->text_height = ctx->window.height - ctx->window.border_inner_margin * 2 - ctx->window.border_outer_margin * 2 - ctx->window.border_width * 2;
								if (ctx->text_width > ctx->maximum_width
										&& ctx->maximum_width > 0) {
									ctx->text_width = ctx->maximum_width;
								}
							}

							/* if position isn't what expected, set fixed pos
							 * total_updates avoids setting ctx->fixed_pos when ctx->window
							 * is set to weird locations when started */
							/* // this is broken
							if (total_updates >= 2 && !ctx->fixed_pos
									&& (ctx->window.x != ev.xconfigure.x
									|| ctx->window.y != ev.xconfigure.y)
									&& (ev.xconfigure.x != 0
									|| ev.xconfigure.y != 0)) {
								ctx->fixed_pos = 1;
							} */
						}
						break;

					case ButtonPress:
						if (ctx->own_window) {
							/* if an ordinary ctx->window with decorations */
							if ((ctx->window.type == TYPE_NORMAL &&
										(!TEST_HINT(ctx->window.hints,
													HINT_UNDECORATED))) ||
									ctx->window.type == TYPE_DESKTOP) {
								/* allow conky to hold input focus. */
								break;
							} else {
								/* forward the click to the desktop ctx->window */
								XUngrabPointer(display, ev.xbutton.time);
								ev.xbutton.window = ctx->window.desktop;
								ev.xbutton.x = ev.xbutton.x_root;
								ev.xbutton.y = ev.xbutton.y_root;
								XSendEvent(display, ev.xbutton.window, False,
									ButtonPressMask, &ev);
								XSetInputFocus(display, ev.xbutton.window,
									RevertToParent, ev.xbutton.time);
							}
						}
						break;

					case ButtonRelease:
						if (ctx->own_window) {
							/* if an ordinary ctx->window with decorations */
							if ((ctx->window.type == TYPE_NORMAL)
									&& (!TEST_HINT(ctx->window.hints,
									HINT_UNDECORATED))) {
								/* allow conky to hold input focus. */
								break;
							} else {
								/* forward the release to the desktop ctx->window */
								ev.xbutton.window = ctx->window.desktop;
								ev.xbutton.x = ev.xbutton.x_root;
								ev.xbutton.y = ev.xbutton.y_root;
								XSendEvent(display, ev.xbutton.window, False,
									ButtonReleaseMask, &ev);
							}
						}
						break;

#endif

					default:
#ifdef HAVE_XDAMAGE
						if (ev.type == ctx->window.event_base + XDamageNotify) {
							XDamageNotifyEvent *dev = (XDamageNotifyEvent *) &ev;

							XFixesSetRegion(display, ctx->window.part, &dev->area, 1);
							XFixesUnionRegion(display, ctx->window.region2, ctx->window.region2, ctx->window.part);
						}
#endif /* HAVE_XDAMAGE */
						break;
				}
			}

#ifdef HAVE_XDAMAGE
			XDamageSubtract(display, ctx->window.damage, ctx->window.region2, None);
			XFixesSetRegion(display, ctx->window.region2, 0, 0);
#endif /* HAVE_XDAMAGE */

			/* XDBE doesn't seem to provide a way to clear the back buffer
			 * without interfering with the front buffer, other than passing
			 * XdbeBackground to XdbeSwapBuffers. That means that if we're
			 * using XDBE, we need to redraw the text even if it wasn't part of
			 * the exposed area. OTOH, if we're not going to call draw_stuff at
			 * all, then no swap happens and we can safely do nothing. */

			if (!XEmptyRegion(ctx->window.region)) {
#ifdef HAVE_XDBE
				if (use_xdbe) {
					XRectangle r;

					r.x = ctx->text_start_x - ctx->window.border_inner_margin - ctx->window.border_outer_margin - ctx->window.border_width;
					r.y = ctx->text_start_y - ctx->window.border_inner_margin - ctx->window.border_outer_margin - ctx->window.border_width;
					r.width = ctx->text_width + ctx->window.border_inner_margin * 2 + ctx->window.border_outer_margin * 2 + ctx->window.border_width * 2;
					r.height = ctx->text_height + ctx->window.border_inner_margin * 2 + ctx->window.border_outer_margin * 2 + ctx->window.border_width * 2;
					XUnionRectWithRegion(&r, ctx->window.region, ctx->window.region);
				}
#endif
				XSetRegion(display, ctx->window.gc, ctx->window.region);
#ifdef XFT
				if (use_xft) {
					XftDrawSetClip(ctx->window.xftdraw, ctx->window.region);
				}
#endif
				draw_stuff(ctx);
				XDestroyRegion(ctx->window.region);
				ctx->window.region = XCreateRegion();
			}
		} else {
#endif /* X11 */
			t = (ctx->next_update_time - get_time()) * 1000000;
			if(t > 0) usleep((useconds_t)t);
			update_text(ctx);
			draw_stuff(ctx);
#ifdef NCURSES
			if(ctx->output_methods & TO_NCURSES) {
				refresh();
				clear();
			}
#endif
#ifdef X11
		}
#endif /* X11 */

#ifdef SIGNAL_BLOCKING
		/* unblock signals of interest and let handler fly */
		if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
			CRIT_ERR(NULL, NULL, "unable to sigprocmask()");
		}
#endif

		switch (g_signal_pending) {
			case SIGHUP:
			case SIGUSR1:
				NORM_ERR("received SIGHUP or SIGUSR1. reloading the config file.");
				reload_config();
				break;
			case SIGINT:
			case SIGTERM:
				NORM_ERR("received SIGINT or SIGTERM to terminate. bye!");
				terminate = 1;
#ifdef X11
				if (ctx->output_methods & TO_X) {
					XDestroyRegion(ctx->window.region);
					ctx->window.region = NULL;
#ifdef HAVE_XDAMAGE
					XDamageDestroy(display, ctx->window.damage);
					XFixesDestroyRegion(display, ctx->window.region2);
					XFixesDestroyRegion(display, ctx->window.part);
#endif /* HAVE_XDAMAGE */
					if (ctx->disp) {
						free(ctx->disp);
					}
				}
#endif /* X11 */
				if(ctx->overwrite_file) {
					free(ctx->overwrite_file);
					ctx->overwrite_file = 0;
				}
				if(ctx->append_file) {
					free(ctx->append_file);
					ctx->append_file = 0;
				}
				break;
			default:
				/* Reaching here means someone set a signal
				 * (SIGXXXX, signal_handler), but didn't write any code
				 * to deal with it.
				 * If you don't want to handle a signal, don't set a handler on
				 * it in the first place. */
				if (g_signal_pending) {
					NORM_ERR("ignoring signal (%d)", g_signal_pending);
				}
				break;
		}
#ifdef HAVE_SYS_INOTIFY_H
		if (inotify_fd != -1 && inotify_config_wd == -1 && ctx->current_config != 0) {
			inotify_config_wd = inotify_add_watch(inotify_fd,
					ctx->current_config,
					IN_MODIFY);
		}
		if (inotify_fd != -1 && inotify_config_wd != -1 && ctx->current_config != 0) {
			int len = 0, idx = 0;
			fd_set descriptors;
			struct timeval time_to_wait;

			FD_ZERO(&descriptors);
			FD_SET(inotify_fd, &descriptors);

			time_to_wait.tv_sec = time_to_wait.tv_usec = 0;

			select(inotify_fd + 1, &descriptors, NULL, NULL, &time_to_wait);
			if (FD_ISSET(inotify_fd, &descriptors)) {
				/* process inotify events */
				len = read(inotify_fd, inotify_buff, INOTIFY_BUF_LEN);
				while (len > 0 && idx < len) {
					struct inotify_event *ev = (struct inotify_event *) &inotify_buff[idx];
					if (ev->wd == inotify_config_wd && (ev->mask & IN_MODIFY || ev->mask & IN_IGNORED)) {
						/* ctx->current_config should be reloaded */
						NORM_ERR("'%s' modified, reloading...", ctx->current_config);
						reload_config();
						if (ev->mask & IN_IGNORED) {
							/* for some reason we get IN_IGNORED here
							 * sometimes, so we need to re-add the watch */
							inotify_config_wd = inotify_add_watch(inotify_fd,
									ctx->current_config,
									IN_MODIFY);
						}
					}
#ifdef HAVE_LUA
					else {
						llua_inotify_query(ev->wd, ev->mask);
					}
#endif /* HAVE_LUA */
					idx += INOTIFY_EVENT_SIZE + ev->len;
				}
			}
		}
#endif /* HAVE_SYS_INOTIFY_H */

#ifdef HAVE_LUA
	llua_update_info(ctx, ctx->update_interval);
#endif /* HAVE_LUA */
		g_signal_pending = 0;
	}
	clean_up(NULL, NULL);

#ifdef HAVE_SYS_INOTIFY_H
	if (inotify_fd != -1) {
		inotify_rm_watch(inotify_fd, inotify_config_wd);
		close(inotify_fd);
		inotify_fd = inotify_config_wd = 0;
	}
#endif /* HAVE_SYS_INOTIFY_H */
}

static void print_help(const char *prog_name) {
	printf("Usage: %s [OPTION]...\n"
			PACKAGE_NAME" is a system monitor that renders text on desktop or to own transparent\n"
			"ctx->window. Command line options will override configurations defined in config\n"
			"file.\n"
			"   -v, --version             version\n"
			"   -q, --quiet               quiet mode\n"
			"   -D, --debug               increase debugging output, ie. -DD for more debugging\n"
			"   -c, --config=FILE         config file to load\n"
#ifdef CONFIG_OUTPUT
			"   -C, --print-config        print the builtin default config to stdout\n"
			"                             e.g. 'conky -C > ~/.conkyrc' will create a new default config\n"
#endif
			"   -d, --daemonize           daemonize, fork to background\n"
			"   -h, --help                help\n"
#ifdef X11
			"   -a, --alignment=ALIGNMENT text alignment on screen, {top,bottom,middle}_{left,right,middle}\n"
			"   -f, --font=FONT           font to use\n"
			"   -X, --display=DISPLAY     X11 display to use\n"
#ifdef OWN_WINDOW
			"   -o, --own-ctx->window          create own ctx->window to draw\n"
#endif
#ifdef HAVE_XDBE
			"   -b, --double-buffer       double buffer (prevents flickering)\n"
#endif
			"   -w, --ctx->window-id=WIN_ID    ctx->window id to draw\n"
			"   -x X                      x position\n"
			"   -y Y                      y position\n"
#endif /* X11 */
			"   -t, --text=TEXT           text to render, remember single quotes, like -t '$uptime'\n"
			"   -u, --interval=SECS       update interval\n"
			"   -i COUNT                  number of times to update "PACKAGE_NAME" (and quit)\n",
			prog_name
	);
}

/* : means that character before that takes an argument */
static const char *getopt_string = "vVqdDt:u:i:hc:"
#ifdef X11
	"x:y:w:a:f:X:"
#ifdef OWN_WINDOW
	"o"
#endif
#ifdef HAVE_XDBE
	"b"
#endif
#endif /* X11 */
#ifdef CONFIG_OUTPUT
	"C"
#endif
	;

static const struct option longopts[] = {
	{ "help", 0, NULL, 'h' },
	{ "version", 0, NULL, 'V' },
	{ "debug", 0, NULL, 'D' },
	{ "config", 1, NULL, 'c' },
#ifdef CONFIG_OUTPUT
	{ "print-config", 0, NULL, 'C' },
#endif
	{ "daemonize", 0, NULL, 'd' },
#ifdef X11
	{ "alignment", 1, NULL, 'a' },
	{ "font", 1, NULL, 'f' },
	{ "display", 1, NULL, 'X' },
#ifdef OWN_WINDOW
	{ "own-ctx->window", 0, NULL, 'o' },
#endif
#ifdef HAVE_XDBE
	{ "double-buffer", 0, NULL, 'b' },
#endif
	{ "ctx->window-id", 1, NULL, 'w' },
#endif /* X11 */
	{ "text", 1, NULL, 't' },
	{ "interval", 0, NULL, 'u' },
	{ 0, 0, 0, 0 }
};

void initialisation(conky_context *ctx, int argc, char **argv)
{
	struct sigaction act, oact;

	set_default_configurations(ctx);
	load_config_file(ctx, ctx->current_config);
	currentconffile = conftree_add(currentconffile, ctx->current_config);

	/* init specials array */
	if ((specials = calloc(sizeof(struct special_t), max_specials)) == 0) {
		NORM_ERR("failed to create specials array");
	}

#ifdef MAIL_FILE
	if (current_mail_spool == NULL) {
		char buf[256];

		variable_substitute(MAIL_FILE, buf, 256);

		if (buf[0] != '\0') {
			current_mail_spool = strndup(buf, text_buffer_size);
		}
	}
#endif

	/* handle other command line arguments */

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) \
		|| defined(__NetBSD__)
	optind = optreset = 1;
#else
	optind = 0;
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
	if ((kd = kvm_open("/dev/null", "/dev/null", "/dev/null", O_RDONLY,
			"kvm_open")) == NULL) {
		CRIT_ERR(NULL, NULL, "cannot read kvm");
	}
#endif

	while (1) {
		int c = getopt_long(argc, argv, getopt_string, longopts, NULL);

		if (c == -1) {
			break;
		}

		switch (c) {
			case 'd':
				ctx->fork_to_background = 1;
				break;
			case 'D':
				global_debug_level++;
				break;
#ifdef X11
			case 'f':
				set_first_font(optarg);
				break;
			case 'a':
				ctx->text_alignment = string_to_alignment(optarg);
				break;

#ifdef OWN_WINDOW
			case 'o':
				ctx->own_window = 1;
				break;
#endif
#ifdef HAVE_XDBE
			case 'b':
				use_xdbe = 1;
				break;
#endif
#endif /* X11 */
			case 't':
				if (ctx->global_text) {
					free(ctx->global_text);
					ctx->global_text = 0;
				}
				ctx->global_text = strndup(optarg, ctx->max_user_text);
				convert_escapes(ctx->global_text);
				break;

			case 'u':
				ctx->update_interval = strtod(optarg, 0);
				ctx->update_interval_old = ctx->update_interval;
				if (ctx->info.music_player_interval == 0) {
					// default to ctx->update_interval
					ctx->info.music_player_interval = ctx->update_interval;
				}
				break;

			case 'i':
				ctx->total_run_times = strtod(optarg, 0);
				break;
#ifdef X11
			case 'x':
				ctx->gap_x = atoi(optarg);
				break;

			case 'y':
				ctx->gap_y = atoi(optarg);
				break;
#endif /* X11 */

			case '?':
				exit(EXIT_FAILURE);
		}
	}

#ifdef X11
	/* load font */
	if (ctx->output_methods & TO_X) {
		load_config_file_x11(ctx, ctx->current_config);
	}
#endif /* X11 */

	/* generate text and get initial size */
	extract_variable_text(ctx, ctx->global_text);
	if (ctx->global_text) {
		free(ctx->global_text);
		ctx->global_text = 0;
	}
	ctx->global_text = NULL;
	/* fork */
	if (ctx->fork_to_background) {
		int pid = fork();

		switch (pid) {
			case -1:
				NORM_ERR(PACKAGE_NAME": couldn't fork() to background: %s",
					strerror(errno));
				break;

			case 0:
				/* child process */
				usleep(25000);
				fprintf(stderr, "\n");
				fflush(stderr);
				break;

			default:
				/* parent process */
				fprintf(stderr, PACKAGE_NAME": forked to background, pid is %d\n",
					pid);
				fflush(stderr);
				exit(EXIT_SUCCESS);
		}
	}

	ctx->text_buffer = malloc(ctx->max_user_text);
	memset(ctx->text_buffer, 0, ctx->max_user_text);
	ctx->tmpstring1 = malloc(text_buffer_size);
	memset(ctx->tmpstring1, 0, text_buffer_size);
	ctx->tmpstring2 = malloc(text_buffer_size);
	memset(ctx->tmpstring2, 0, text_buffer_size);

#ifdef X11
	ctx->xargc = argc;
	ctx->xargv = argv;
	X11_create_window(ctx);
#endif /* X11 */
#ifdef HAVE_LUA
	llua_setup_info(ctx, ctx->update_interval);
#endif /* HAVE_LUA */
#ifdef XOAP
	xmlInitParser();
#endif /* XOAP */

	/* Set signal handlers */
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
#ifdef SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif

	if (		sigaction(SIGINT,  &act, &oact) < 0
			||	sigaction(SIGALRM, &act, &oact) < 0
			||	sigaction(SIGUSR1, &act, &oact) < 0
			||	sigaction(SIGHUP,  &act, &oact) < 0
			||	sigaction(SIGTERM, &act, &oact) < 0) {
		NORM_ERR("error setting signal handler: %s", strerror(errno));
	}

#ifdef HAVE_LUA
	llua_startup_hook();
#endif /* HAVE_LUA */
}

int main(int argc, char **argv)
{
	/* Conky's main context struct */
	conky_context mctx;
	conky_context *ctx = &mctx;

#ifdef X11
	char *s, *temp;
	unsigned int x;
#endif

	argc_copy = argc;
	argv_copy = argv;
	g_signal_pending = 0;
	memset(ctx, 0, sizeof(conky_context));
	ctx->max_user_text = MAX_USER_TEXT_DEFAULT;
	clear_net_stats();

#ifdef TCP_PORT_MONITOR
	/* set default connection limit */
	tcp_portmon_set_max_connections(0);
#endif

	/* handle command line parameters that don't change configs */
#ifdef X11
	if (((s = getenv("LC_ALL")) && *s) || ((s = getenv("LC_CTYPE")) && *s)
			|| ((s = getenv("LANG")) && *s)) {
		temp = (char *) malloc((strlen(s) + 1) * sizeof(char));
		if (temp == NULL) {
			NORM_ERR("malloc failed");
		}
		for (x = 0; x < strlen(s); x++) {
			temp[x] = tolower(s[x]);
		}
		temp[x] = 0;
		if (strstr(temp, "utf-8") || strstr(temp, "utf8")) {
			ctx->utf8_mode = 1;
		}

		free(temp);
	}
	if (!setlocale(LC_CTYPE, "")) {
		NORM_ERR("Can't set the specified locale!\nCheck LANG, LC_CTYPE, LC_ALL.");
	}
#endif /* X11 */
	while (1) {
		int c = getopt_long(argc, argv, getopt_string, longopts, NULL);

		if (c == -1) {
			break;
		}

		switch (c) {
			case 'v':
			case 'V':
				print_version();
			case 'c':
				if (ctx->current_config) {
					free(ctx->current_config);
				}
				ctx->current_config = strndup(optarg, ctx->max_user_text);
				break;
			case 'q':
				freopen("/dev/null", "w", stderr);
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
#ifdef CONFIG_OUTPUT
			case 'C':
				print_defconfig();
				return 0;
#endif
#ifdef X11
			case 'w':
				ctx->window.window = strtol(optarg, 0, 0);
				break;
			case 'X':
				if (ctx->disp)
					free(ctx->disp);
				ctx->disp = strdup(optarg);
				break;
#endif /* X11 */

			case '?':
				exit(EXIT_FAILURE);
		}
	}

	/* check if specified config file is valid */
	if (ctx->current_config) {
		struct stat sb;
		if (stat(ctx->current_config, &sb) ||
				(!S_ISREG(sb.st_mode) && !S_ISLNK(sb.st_mode))) {
			NORM_ERR("invalid configuration file '%s'\n", ctx->current_config);
			free(ctx->current_config);
			ctx->current_config = 0;
		}
	}

	/* load ctx->current_config, CONFIG_FILE or SYSTEM_CONFIG_FILE */

	if (!ctx->current_config) {
		/* load default config file */
		char buf[DEFAULT_TEXT_BUFFER_SIZE];
		FILE *fp;

		/* Try to use personal config file first */
		to_real_path(buf, CONFIG_FILE);
		if (buf[0] && (fp = fopen(buf, "r"))) {
			ctx->current_config = strndup(buf, ctx->max_user_text);
			fclose(fp);
		}

		/* Try to use system config file if personal config not readable */
		if (!ctx->current_config && (fp = fopen(SYSTEM_CONFIG_FILE, "r"))) {
			ctx->current_config = strndup(SYSTEM_CONFIG_FILE, ctx->max_user_text);
			fclose(fp);
		}

		/* No readable config found */
		if (!ctx->current_config) {
#ifdef CONFIG_OUTPUT
			ctx->current_config = strdup("==builtin==");
			NORM_ERR("no readable personal or system-wide config file found,"
					" using builtin default");
#else
			CRIT_ERR(NULL, NULL, "no readable personal or system-wide config file found");
#endif /* ! CONF_OUTPUT */
		}
	}

#ifdef XOAP
	/* Load xoap keys, if existing */
	load_xoap_keys();
#endif /* XOAP */

#ifdef HAVE_SYS_INOTIFY_H
	inotify_fd = inotify_init();
#endif /* HAVE_SYS_INOTIFY_H */

	initialisation(&mctx, argc, argv);

	main_loop(&mctx);

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
	kvm_close(kd);
#endif

	return 0;

}

static void signal_handler(int sig)
{
	/* signal handler is light as a feather, as it should be.  we will poll
	 * g_signal_pending with each loop of conky and do any signal processing
	 * there, NOT here */
		g_signal_pending = sig;
}
