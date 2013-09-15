/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
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
#include "text_object.h"
#include "conky.h"
#include "common.h"
#include "timed_thread.h"
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
#endif
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
#include <curses.h>
#endif
#ifdef XOAP
#include <libxml/parser.h>
#endif /* XOAP */
#ifdef HAVE_CURL
#include <curl/curl.h>
#endif

/* local headers */
#include "core.h"
#include "algebra.h"
#include "build.h"
#include "colours.h"
#include "combine.h"
#include "diskio.h"
#include "entropy.h"
#include "exec.h"
#include "i8k.h"
#include "proc.h"
#include "user.h"
#ifdef X11
#include "fonts.h"
#endif
#include "fs.h"
#ifdef HAVE_ICONV
#include "iconv_tools.h"
#endif
#include "logging.h"
#include "mixer.h"
#include "mail.h"
#include "mboxscan.h"
#include "net_stat.h"
#ifdef NVIDIA
#include "nvidia.h"
#endif
#include "read_tcp.h"
#include "scroll.h"
#include "specials.h"
#include "temphelper.h"
#include "template.h"
#include "tailhead.h"
#include "timeinfo.h"
#include "top.h"

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "freebsd.h"
#elif defined(__OpenBSD__)
#include "openbsd.h"
#elif defined(__NetBSD__)
#include "netbsd.h"
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

/* disable inotify auto reload feature if desired */
int disable_auto_reload = 0;

/* two strings for internal use */
static char *tmpstring1, *tmpstring2;

/* variables holding various config settings */
int short_units;
int format_human_readable;
int cpu_separate;
enum {
	NO_SPACER = 0,
	LEFT_SPACER,
	RIGHT_SPACER
} use_spacer;
int top_cpu, top_mem, top_time;
#ifdef IOSTATS
int top_io;
#endif
int top_running;
int output_methods;
static int extra_newline;
enum x_initialiser_state x_initialised = NO;
static volatile int g_signal_pending;
/* Update interval */
double update_interval;
double update_interval_old;
double update_interval_bat;
void *global_cpu = NULL;
unsigned int max_text_width = 0;

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
# ifdef USE_ARGB
		   "  * ARGB visual\n"
# endif /* USE_ARGB */
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
#ifdef APCUPSD
		   "  * apcupsd\n"
#endif /* APCUPSD */
#ifdef IOSTATS
		   "  * iostats\n"
#endif /* IOSTATS */
#ifdef NCURSES
		   "  * ncurses\n"
#endif /* NCURSES */
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

static const char *suffixes[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "" };


#ifdef X11

static void X11_create_window(void);
static void X11_initialisation(void);

struct _x11_stuff_s {
	Region region;
#ifdef HAVE_XDAMAGE
	Damage damage;
	XserverRegion region2, part;
	int event_base, error_base;
#endif
} x11_stuff;

/* text size */

static int text_start_x, text_start_y;	/* text start position in window */
static int text_width, text_height;

/* alignments */
enum alignment {
	ALIGNMENT_ERROR,
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

/* display to connect to */
static char *disp = NULL;

#endif /* X11 */

/* struct that has all info to be shared between
 * instances of the same text object */
struct information info;

/* path to config file */
char *current_config;

/* set to 1 if you want all text to be in uppercase */
static unsigned int stuff_in_uppercase;

/* Run how many times? */
static unsigned long total_run_times;

/* fork? */
static int fork_to_background;

/* set to 0 after the first time conky is run, so we don't fork again after the
 * first forking */
static int first_pass = 1;

static int cpu_avg_samples, net_avg_samples, diskio_avg_samples;

/* filenames for output */
char *overwrite_file = NULL; FILE *overwrite_fpointer = NULL;
char *append_file = NULL; FILE *append_fpointer = NULL;

#ifdef X11

static int show_graph_scale;
static int show_graph_range;

/* Position on the screen */
static int text_alignment;
static int gap_x, gap_y;

/* border */
static int draw_borders;
static int draw_graph_borders;
static int stippled_borders;

int get_stippled_borders(void)
{
	return stippled_borders;
}

static int draw_shades, draw_outline;

long default_fg_color, default_bg_color, default_out_color;

/* create own window or draw stuff to root? */
static int set_transparent = 0;

#ifdef OWN_WINDOW
static int own_window = 0;
static int background_colour = 0;

/* fixed size/pos is set if wm/user changes them */
static int fixed_size = 0, fixed_pos = 0;
#endif

static int minimum_width, minimum_height;
static int maximum_width;

#endif /* X11 */

#ifdef __OpenBSD__
static int sensor_device;
#endif

long color0, color1, color2, color3, color4, color5, color6, color7, color8,
	 color9;

/* maximum size of config TEXT buffer, i.e. below TEXT line. */
unsigned int max_user_text;

/* maximum size of individual text buffers, ie $exec buffer size */
unsigned int text_buffer_size = DEFAULT_TEXT_BUFFER_SIZE;

/* UTF-8 */
int utf8_mode = 0;

/* no buffers in used memory? */
int no_buffers;

/* pad percentages to decimals? */
static int pad_percents = 0;

static char *global_text = 0;

char *get_global_text(void)
{
	return global_text;
}

long global_text_lines;

static int total_updates;
static int updatereset;

void set_updatereset(int i)
{
	updatereset = i;
}

int get_updatereset(void)
{
	return updatereset;
}

int check_contains(char *f, char *s)
{
	int ret = 0;
	FILE *where = open_file(f, 0);

	if (where) {
		char buf1[256];

		while (fgets(buf1, 256, where)) {
			if (strstr(buf1, s)) {
				ret = 1;
				break;
			}
		}
		fclose(where);
	} else {
		NORM_ERR("Could not open the file '%s'", f);
	}
	return ret;
}

int calc_text_width(const char *s)
{
	size_t slen = strlen(s);

#ifdef X11
	if ((output_methods & TO_X) == 0) {
#endif /* X11 */
		return slen;
#ifdef X11
	}
#ifdef XFT
	if (use_xft) {
		XGlyphInfo gi;

		if (utf8_mode) {
			XftTextExtentsUtf8(display, fonts[selected_font].xftfont,
				(const FcChar8 *) s, slen, &gi);
		} else {
			XftTextExtents8(display, fonts[selected_font].xftfont,
				(const FcChar8 *) s, slen, &gi);
		}
		return gi.xOff;
	} else
#endif
	{
		return XTextWidth(fonts[selected_font].font, s, slen);
	}
#endif /* X11 */
}

/* formatted text to render on screen, generated in generate_text(),
 * drawn in draw_stuff() */

static char *text_buffer;

/* quite boring functions */

static inline void for_each_line(char *b, int f(char *, int))
{
	char *ps, *pe;
	int special_index = 0; /* specials index */

	if(! b) return;
	for (ps = b, pe = b; *pe; pe++) {
		if (*pe == '\n') {
			*pe = '\0';
			special_index = f(ps, special_index);
			*pe = '\n';
			ps = pe + 1;
		}
	}

	if (ps < pe) {
		f(ps, special_index);
	}
}

static void convert_escapes(char *buf)
{
	char *p = buf, *s = buf;

	while (*s) {
		if (*s == '\\') {
			s++;
			if (*s == 'n') {
				*p++ = '\n';
			} else if (*s == '\\') {
				*p++ = '\\';
			}
			s++;
		} else {
			*p++ = *s++;
		}
	}
	*p = '\0';
}

/* Prints anything normally printed with snprintf according to the current value
 * of use_spacer.  Actually slightly more flexible than snprintf, as you can
 * safely specify the destination buffer as one of your inputs.  */
int spaced_print(char *buf, int size, const char *format, int width, ...)
{
	int len = 0;
	va_list argp;
	char *tempbuf;

	if (size < 1) {
		return 0;
	}
	tempbuf = malloc(size * sizeof(char));

	// Passes the varargs along to vsnprintf
	va_start(argp, width);
	vsnprintf(tempbuf, size, format, argp);
	va_end(argp);

	switch (use_spacer) {
		case NO_SPACER:
			len = snprintf(buf, size, "%s", tempbuf);
			break;
		case LEFT_SPACER:
			len = snprintf(buf, size, "%*s", width, tempbuf);
			break;
		case RIGHT_SPACER:
			len = snprintf(buf, size, "%-*s", width, tempbuf);
			break;
	}
	free(tempbuf);
	return len;
}

/* print percentage values
 *
 * - i.e., unsigned values between 0 and 100
 * - respect the value of pad_percents */
int percent_print(char *buf, int size, unsigned value)
{
	return spaced_print(buf, size, "%u", pad_percents, value);
}

/* converts from bytes to human readable format (K, M, G, T)
 *
 * The algorithm always divides by 1024, as unit-conversion of byte
 * counts suggests. But for output length determination we need to
 * compare with 1000 here, as we print in decimal form. */
void human_readable(long long num, char *buf, int size)
{
	const char **suffix = suffixes;
	float fnum;
	int precision;
	int width;
	const char *format;

	/* Possibly just output as usual, for example for stdout usage */
	if (!format_human_readable) {
		spaced_print(buf, size, "%lld", 6, num);
		return;
	}
	if (short_units) {
		width = 5;
		format = "%.*f%.1s";
	} else {
		width = 7;
		format = "%.*f%-3s";
	}

	if (llabs(num) < 1000LL) {
		spaced_print(buf, size, format, width, 0, (float)num, *suffix);
		return;
	}

	while (llabs(num / 1024) >= 1000LL && **(suffix + 2)) {
		num /= 1024;
		suffix++;
	}

	suffix++;
	fnum = num / 1024.0;

	/* fnum should now be < 1000, so looks like 'AAA.BBBBB'
	 *
	 * The goal is to always have a significance of 3, by
	 * adjusting the decimal part of the number. Sample output:
	 *  123MiB
	 * 23.4GiB
	 * 5.12B
	 * so the point of alignment resides between number and unit. The
	 * upside of this is that there is minimal padding necessary, though
	 * there should be a way to make alignment take place at the decimal
	 * dot (then with fixed width decimal part).
	 *
	 * Note the repdigits below: when given a precision value, printf()
	 * rounds the float to it, not just cuts off the remaining digits. So
	 * e.g. 99.95 with a precision of 1 gets 100.0, which again should be
	 * printed with a precision of 0. Yay. */

	precision = 0;		/* print 100-999 without decimal part */
	if (fnum < 99.95)
		precision = 1;	/* print 10-99 with one decimal place */
	if (fnum < 9.995)
		precision = 2;	/* print 0-9 with two decimal places */

	spaced_print(buf, size, format, width, precision, fnum, *suffix);
}

/* global object list root element */
static struct text_object global_root_object;

static long current_text_color;

void set_current_text_color(long colour)
{
	current_text_color = colour;
}

long get_current_text_color(void)
{
	return current_text_color;
}

//adds newstring to to the tree unless you can already see it when travelling back.
//if it's possible to attach it then it returns a pointer to the leaf, else it returns NULL
struct conftree* conftree_add(struct conftree* previous, const char* newstring) {
	struct conftree* node;
	struct conftree* node2;

	for(node = previous; node != NULL; node = node->back) {
		if(strcmp(node->string, newstring) == 0) {
			return NULL;
		}
	}
	node = malloc(sizeof(struct conftree));
	if (previous != NULL) {
		if(previous->vert_next == NULL) {
			previous->vert_next = node;
		} else {
			for(node2 = previous->vert_next; node2->horz_next != NULL; node2 = node2->horz_next ) { }
			node2->horz_next = node;
		}
	}
	node->string = strdup(newstring);
	node->horz_next = NULL;
	node->vert_next = NULL;
	node->back = previous;
	return node;
}

void conftree_empty(struct conftree* tree) {
	if(tree) {
		conftree_empty(tree->horz_next);
		conftree_empty(tree->vert_next);
		free(tree->string);
		free(tree);
	}
}

struct conftree *currentconffile;

static void extract_variable_text(const char *p)
{
	free_text_objects(&global_root_object, 0);
	if (tmpstring1) {
		free(tmpstring1);
		tmpstring1 = 0;
	}
	if (tmpstring2) {
		free(tmpstring2);
		tmpstring2 = 0;
	}
	if (text_buffer) {
		free(text_buffer);
		text_buffer = 0;
	}

	extract_variable_text_internal(&global_root_object, p);
}

void parse_conky_vars(struct text_object *root, const char *txt,
		char *p, int p_max_size, struct information *cur)
{
	extract_variable_text_internal(root, txt);
	generate_text_internal(p, p_max_size, *root, cur);
}

void generate_text_internal(char *p, int p_max_size,
		struct text_object root, struct information *cur)
{
	struct text_object *obj;
#ifdef X11
	int need_to_load_fonts = 0;
#endif /* X11 */

	/* for the OBJ_top* handler */

#ifdef HAVE_ICONV
	char buff_in[p_max_size];
	buff_in[0] = 0;
#endif /* HAVE_ICONV */

	if(! p) return;

	p[0] = 0;
	obj = root.next;
	while (obj && p_max_size > 0) {

/* IFBLOCK jumping algorithm
 *
 * This is easier as it looks like:
 * - each IF checks it's condition
 *   - on FALSE: call DO_JUMP
 *   - on TRUE: don't care
 * - each ELSE calls DO_JUMP unconditionally
 * - each ENDIF is silently being ignored
 *
 * Why this works:
 * DO_JUMP overwrites the "obj" variable of the loop and sets it to the target
 * (i.e. the corresponding ELSE or ENDIF). After that, processing for the given
 * object can continue, free()ing stuff e.g., then the for-loop does the rest: as
 * regularly, "obj" is being updated to point to obj->next, so object parsing
 * continues right after the corresponding ELSE or ENDIF. This means that if we
 * find an ELSE, it's corresponding IF must not have jumped, so we need to jump
 * always. If we encounter an ENDIF, it's corresponding IF or ELSE has not
 * jumped, and there is nothing to do.
 */
#define DO_JUMP { \
	DBGP2("jumping"); \
	if (obj->ifblock_next) \
		obj = obj->ifblock_next; \
}

#define OBJ(a) break; case OBJ_##a:

		switch (obj->type) {
			default:
				NORM_ERR("not implemented obj type %d", obj->type);
			OBJ(read_tcp) {
				print_read_tcp(obj, p, p_max_size);
			}
#if !defined(__OpenBSD__) && !defined(__NetBSD__)
			OBJ(acpitemp) {
				temp_print(p, p_max_size, get_acpi_temperature(obj->data.i), TEMP_CELSIUS);
			}
#endif /* !__OpenBSD__ */
			OBJ(freq) {
				static int ok = 1;
				if (ok) {
					ok = get_freq(p, p_max_size, "%.0f", 1,
							obj->data.i);
				}
			}
			OBJ(freq_g) {
				static int ok = 1;
				if (ok) {
#if !defined(__OpenBSD__) && !defined(__NetBSD__)
					ok = get_freq(p, p_max_size, "%'.2f", 1000,
							obj->data.i);
#else
					/* OpenBSD has no such flag (SUSv2) */
					ok = get_freq(p, p_max_size, "%.2f", 1000,
							obj->data.i);
#endif /* __OpenBSD */
				}
			}
#if defined(__linux__)
			OBJ(voltage_mv) {
				print_voltage_mv(obj, p, p_max_size);
			}
			OBJ(voltage_v) {
				print_voltage_v(obj, p, p_max_size);
			}

#ifdef HAVE_IWLIB
			OBJ(wireless_essid) {
				print_wireless_essid(obj, p, p_max_size);
			}
			OBJ(wireless_mode) {
				print_wireless_mode(obj, p, p_max_size);
			}
			OBJ(wireless_bitrate) {
				print_wireless_bitrate(obj, p, p_max_size);
			}
			OBJ(wireless_ap) {
				print_wireless_ap(obj, p, p_max_size);
			}
			OBJ(wireless_link_qual) {
				print_wireless_link_qual(obj, p, p_max_size);
			}
			OBJ(wireless_link_qual_max) {
				print_wireless_link_qual_max(obj, p, p_max_size);
			}
			OBJ(wireless_link_qual_perc) {
				print_wireless_link_qual_perc(obj, p, p_max_size);
			}
			OBJ(wireless_link_bar) {
				print_wireless_link_bar(obj, p, p_max_size);
			}
#endif /* HAVE_IWLIB */

#endif /* __linux__ */

#if !defined(__OpenBSD__) && !defined(__NetBSD__)
			OBJ(acpifan) {
				get_acpi_fan(p, p_max_size);
			}
			OBJ(acpiacadapter) {
				get_acpi_ac_adapter(p, p_max_size, (const char *)obj->data.opaque);
			}
			OBJ(battery) {
				get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_STATUS);
			}
			OBJ(battery_time) {
				get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_TIME);
			}
			OBJ(battery_percent) {
				percent_print(p, p_max_size, get_battery_perct(obj->data.s));
			}
			OBJ(battery_bar) {
				new_bar(obj, p, p_max_size, get_battery_perct_bar(obj->data.s));
			}
			OBJ(battery_short) {
				get_battery_short_status(p, p_max_size, obj->data.s);
			}
#endif /* __OpenBSD__ */

			OBJ(buffers) {
				human_readable(cur->buffers * 1024, p, 255);
			}
			OBJ(cached) {
				human_readable(cur->cached * 1024, p, 255);
			}
			OBJ(cmdline_to_pid) {
				print_cmdline_to_pid(obj, p, p_max_size);
			}
			OBJ(cpu) {
				if (cur->cpu_usage) {
					if (obj->data.i > info.cpu_count) {
						static int warned = 0;
						if(!warned) {
							NORM_ERR("obj->data.i %i info.cpu_count %i",
									obj->data.i, info.cpu_count);
							NORM_ERR("attempting to use more CPUs than you have!");
							warned = 1;
						}
					} else  {
						percent_print(p, p_max_size,
				              round_to_int(cur->cpu_usage[obj->data.i] * 100.0));
					}
				}
			}
			OBJ(cpugauge)
				new_gauge(obj, p, p_max_size, round_to_int(cur->cpu_usage[obj->data.i] * 255.0));
			OBJ(cpubar) {
				new_bar(obj, p, p_max_size, round_to_int(cur->cpu_usage[obj->data.i] * 255.0));
			}
#ifdef X11
			OBJ(cpugraph) {
				new_graph(obj, p, p_max_size, round_to_int(cur->cpu_usage[obj->data.i] * 100));
			}
			OBJ(loadgraph) {
				print_loadgraph(obj, p, p_max_size);
			}
#endif /* X11 */
			OBJ(color) {
				new_fg(p, obj->data.l);
			}
#ifdef X11
			OBJ(color0) {
				new_fg(p, color0);
			}
			OBJ(color1) {
				new_fg(p, color1);
			}
			OBJ(color2) {
				new_fg(p, color2);
			}
			OBJ(color3) {
				new_fg(p, color3);
			}
			OBJ(color4) {
				new_fg(p, color4);
			}
			OBJ(color5) {
				new_fg(p, color5);
			}
			OBJ(color6) {
				new_fg(p, color6);
			}
			OBJ(color7) {
				new_fg(p, color7);
			}
			OBJ(color8) {
				new_fg(p, color8);
			}
			OBJ(color9) {
				new_fg(p, color9);
			}
#endif /* X11 */
#if defined(__linux__)
			OBJ(disk_protect) {
				snprintf(p, p_max_size, "%s",
						get_disk_protect_queue(obj->data.s));
			}
			OBJ(i8k_version) {
				print_i8k_version(obj, p, p_max_size);
			}
			OBJ(i8k_bios) {
				print_i8k_bios(obj, p, p_max_size);
			}
			OBJ(i8k_serial) {
				print_i8k_serial(obj, p, p_max_size);
			}
			OBJ(i8k_cpu_temp) {
				print_i8k_cpu_temp(obj, p, p_max_size);
			}
			OBJ(i8k_left_fan_status) {
				print_i8k_left_fan_status(obj, p, p_max_size);
			}
			OBJ(i8k_right_fan_status) {
				print_i8k_right_fan_status(obj, p, p_max_size);
			}
			OBJ(i8k_left_fan_rpm) {
				print_i8k_left_fan_rpm(obj, p, p_max_size);
			}
			OBJ(i8k_right_fan_rpm) {
				print_i8k_right_fan_rpm(obj, p, p_max_size);
			}
			OBJ(i8k_ac_status) {
				print_i8k_ac_status(obj, p, p_max_size);
			}
			OBJ(i8k_buttons_status) {
				print_i8k_buttons_status(obj, p, p_max_size);
			}
#if defined(IBM)
			OBJ(ibm_fan) {
				get_ibm_acpi_fan(obj, p, p_max_size);
			}
			OBJ(ibm_temps) {
				print_ibm_temps(obj, p, p_max_size);
			}
			OBJ(ibm_volume) {
				get_ibm_acpi_volume(obj, p, p_max_size);
			}
			OBJ(ibm_brightness) {
				get_ibm_acpi_brightness(obj, p, p_max_size);
			}
#endif /* IBM */
			/* information from sony_laptop kernel module
			 * /sys/devices/platform/sony-laptop */
			OBJ(sony_fanspeed) {
				get_sony_fanspeed(p, p_max_size);
			}
			OBJ(if_gw) {
				if (!gateway_exists()) {
					DO_JUMP;
				}
			}
			OBJ(gw_iface) {
				print_gateway_iface(p, p_max_size);
			}
			OBJ(gw_ip) {
				print_gateway_ip(p, p_max_size);
			}
			OBJ(laptop_mode) {
				snprintf(p, p_max_size, "%d", get_laptop_mode());
			}
			OBJ(pb_battery) {
				get_powerbook_batt_info(p, p_max_size, obj->data.i);
			}
#endif /* __linux__ */
#if (defined(__FreeBSD__) || defined(__linux__))
			OBJ(if_up) {
				if (!interface_up(obj)) {
					DO_JUMP;
				}
			}
#endif
#ifdef __OpenBSD__
			OBJ(obsd_sensors_temp) {
				print_obsd_sensors_temp(obj, p, p_max_size);
			}
			OBJ(obsd_sensors_fan) {
				print_obsd_sensors_fan(obj, p, p_max_size);
			}
			OBJ(obsd_sensors_volt) {
				print_obsd_sensors_volt(obj, p, p_max_size);
			}
			OBJ(obsd_vendor) {
				get_obsd_vendor(p, p_max_size);
			}
			OBJ(obsd_product) {
				get_obsd_product(p, p_max_size);
			}
#endif /* __OpenBSD__ */
#ifdef X11
			OBJ(font) {
				new_font(p, obj->data.s);
				need_to_load_fonts = 1;
			}
#endif /* X11 */
			OBJ(diskio) {
				print_diskio(obj, p, p_max_size);
			}
			OBJ(diskio_write) {
				print_diskio_write(obj, p, p_max_size);
			}
			OBJ(diskio_read) {
				print_diskio_read(obj, p, p_max_size);
			}
#ifdef X11
			OBJ(diskiograph) {
				print_diskiograph(obj, p, p_max_size);
			}
			OBJ(diskiograph_read) {
				print_diskiograph_read(obj, p, p_max_size);
			}
			OBJ(diskiograph_write) {
				print_diskiograph_write(obj, p, p_max_size);
			}
#endif /* X11 */
			OBJ(downspeed) {
				print_downspeed(obj, p, p_max_size);
			}
			OBJ(downspeedf) {
				print_downspeedf(obj, p, p_max_size);
			}
#ifdef X11
			OBJ(downspeedgraph) {
				print_downspeedgraph(obj, p, p_max_size);
			}
#endif /* X11 */
			OBJ(else) {
				/* Since we see you, you're if has not jumped.
				 * Do Ninja jump here: without leaving traces.
				 * This is to prevent us from stale jumped flags.
				 */
				obj = obj->ifblock_next;
				continue;
			}
			OBJ(endif) {
				/* harmless object, just ignore */
			}
			OBJ(addr) {
				print_addr(obj, p, p_max_size);
			}
#if defined(__linux__)
			OBJ(addrs) {
				print_addrs(obj, p, p_max_size);
			}
#endif /* __linux__ */
#if defined(IMLIB2) && defined(X11)
			OBJ(image) {
				/* doesn't actually draw anything, just queues it omp.  the
				 * image will get drawn after the X event loop */
				cimlib_add_image(obj->data.s);
			}
#endif /* IMLIB2 */
			OBJ(eval) {
				evaluate(obj->data.s, p, p_max_size);
			}
			OBJ(exec) {
				print_exec(obj, p, p_max_size);
			}
			OBJ(execp) {
				print_execp(obj, p, p_max_size);
			}
			OBJ(execgauge) {
				print_execgauge(obj, p, p_max_size);
			}
			OBJ(execbar) {
				print_execbar(obj, p, p_max_size);
			}
#ifdef X11
			OBJ(execgraph) {
				print_execgraph(obj, p, p_max_size);
			}
#endif /* X11 */
			OBJ(execibar) {
				print_execibar(obj, p, p_max_size);
			}
#ifdef X11
			OBJ(execigraph) {
				print_execigraph(obj, p, p_max_size);
			}
#endif /* X11 */
			OBJ(execigauge) {
				print_execigauge(obj, p, p_max_size);
			}
			OBJ(execi) {
				print_execi(obj, p, p_max_size);
			}
			OBJ(execpi) {
				print_execpi(obj, p, p_max_size);
			}
			OBJ(texeci) {
				print_texeci(obj, p, p_max_size);
			}
			OBJ(imap_unseen) {
				print_imap_unseen(obj, p, p_max_size);
			}
			OBJ(imap_messages) {
				print_imap_messages(obj, p, p_max_size);
			}
			OBJ(pop3_unseen) {
				print_pop3_unseen(obj, p, p_max_size);
			}
			OBJ(pop3_used) {
				print_pop3_used(obj, p, p_max_size);
			}
			OBJ(fs_bar) {
				print_fs_bar(obj, 0, p, p_max_size);
			}
			OBJ(fs_free) {
				print_fs_free(obj, p, p_max_size);
			}
			OBJ(fs_free_perc) {
				print_fs_perc(obj, 1, p, p_max_size);
			}
			OBJ(fs_size) {
				print_fs_size(obj, p, p_max_size);
			}
			OBJ(fs_type) {
				print_fs_type(obj, p, p_max_size);
			}
			OBJ(fs_used) {
				print_fs_used(obj, p, p_max_size);
			}
			OBJ(fs_bar_free) {
				print_fs_bar(obj, 1, p, p_max_size);
			}
			OBJ(fs_used_perc) {
				print_fs_perc(obj, 0, p, p_max_size);
			}
			OBJ(loadavg) {
				print_loadavg(obj, p, p_max_size);
			}
			OBJ(goto) {
				new_goto(p, obj->data.i);
			}
			OBJ(tab) {
				new_tab(obj, p);
			}
#ifdef X11
			OBJ(hr) {
				new_hr(p, obj->data.i);
			}
#endif
			OBJ(nameserver) {
				print_nameserver(obj, p, p_max_size);
			}
#ifdef EVE
			OBJ(eve) {
				print_eve(obj, p, p_max_size);
			}
#endif
#ifdef HAVE_CURL
			OBJ(curl) {
				curl_print(obj, p, p_max_size);
			}
#endif
#ifdef RSS
			OBJ(rss) {
				rss_print_info(obj, p, p_max_size);
			}
#endif
#ifdef WEATHER
			OBJ(weather) {
				print_weather(obj, p, p_max_size);
			}
#endif
#ifdef XOAP
			OBJ(weather_forecast) {
				print_weather_forecast(obj, p, p_max_size);
			}
#endif
#ifdef HAVE_LUA
			OBJ(lua) {
				print_lua(obj, p, p_max_size);
			}
			OBJ(lua_parse) {
				print_lua_parse(obj, p, p_max_size);
			}
			OBJ(lua_bar) {
				print_lua_bar(obj, p, p_max_size);
			}
#ifdef X11
			OBJ(lua_graph) {
				print_lua_graph(obj, p, p_max_size);
			}
#endif /* X11 */
			OBJ(lua_gauge) {
				print_lua_gauge(obj, p, p_max_size);
			}
#endif /* HAVE_LUA */
#ifdef HDDTEMP
			OBJ(hddtemp) {
				short val;
				char unit;

				if (get_hddtemp_info(obj->data.s, &val, &unit)) {
					snprintf(p, p_max_size, "N/A");
				} else {
					temp_print(p, p_max_size, (double)val,
							(unit == 'C' ? TEMP_CELSIUS : TEMP_FAHRENHEIT));
				}
			}
#endif
			OBJ(offset) {
				new_offset(p, obj->data.i);
			}
			OBJ(voffset) {
				new_voffset(p, obj->data.i);
			}
#ifdef __linux__
			OBJ(i2c) {
				print_sysfs_sensor(obj, p, p_max_size);
			}
			OBJ(platform) {
				print_sysfs_sensor(obj, p, p_max_size);
			}
			OBJ(hwmon) {
				print_sysfs_sensor(obj, p, p_max_size);
			}
#endif /* __linux__ */
			OBJ(alignr) {
				new_alignr(p, obj->data.i);
			}
			OBJ(alignc) {
				new_alignc(p, obj->data.i);
			}
			OBJ(if_empty) {
				char buf[max_user_text];
				struct information *tmp_info =
					malloc(sizeof(struct information));
				memcpy(tmp_info, cur, sizeof(struct information));
				generate_text_internal(buf, max_user_text,
				                       *obj->sub, tmp_info);

				if (strlen(buf) != 0) {
					DO_JUMP;
				}
				free(tmp_info);
			}
			OBJ(if_match) {
				char expression[max_user_text];
				int val;
				struct information *tmp_info;

				tmp_info = malloc(sizeof(struct information));
				memcpy(tmp_info, cur, sizeof(struct information));
				generate_text_internal(expression, max_user_text,
				                       *obj->sub, tmp_info);
				DBGP("parsed arg into '%s'", expression);

				val = compare(expression);
				if (val == -2) {
					NORM_ERR("compare failed for expression '%s'",
							expression);
				} else if (!val) {
					DO_JUMP;
				}
				free(tmp_info);
			}
			OBJ(if_existing) {
				char *spc;

				spc = strchr(obj->data.s, ' ');
				if (!spc && access(obj->data.s, F_OK)) {
					DO_JUMP;
				} else if (spc) {
					*spc = '\0';
					if (!check_contains(obj->data.s, spc + 1))
						DO_JUMP;
					*spc = ' ';
				}
			}
			OBJ(if_mounted) {
				if ((obj->data.s)
						&& (!check_mount(obj->data.s))) {
					DO_JUMP;
				}
			}
			OBJ(if_running) {
#ifdef __linux__
				if (!get_process_by_name(obj->data.s)) {
#else
				if ((obj->data.s) && system(obj->data.s)) {
#endif
					DO_JUMP;
				}
			}
#if defined(__linux__)
			OBJ(ioscheduler) {
				snprintf(p, p_max_size, "%s", get_ioscheduler(obj->data.s));
			}
#endif
			OBJ(kernel) {
				snprintf(p, p_max_size, "%s", cur->uname_s.release);
			}
			OBJ(machine) {
				snprintf(p, p_max_size, "%s", cur->uname_s.machine);
			}

			/* memory stuff */
			OBJ(mem) {
				human_readable(cur->mem * 1024, p, 255);
			}
			OBJ(memeasyfree) {
				human_readable(cur->memeasyfree * 1024, p, 255);
			}
			OBJ(memfree) {
				human_readable(cur->memfree * 1024, p, 255);
			}
			OBJ(memmax) {
				human_readable(cur->memmax * 1024, p, 255);
			}
			OBJ(memperc) {
				if (cur->memmax)
					percent_print(p, p_max_size, cur->mem * 100 / cur->memmax);
			}
			OBJ(memgauge){
				new_gauge(obj, p, p_max_size, cur->memmax ? (cur->mem * 255) / (cur->memmax) : 0);
			}
			OBJ(membar) {
				new_bar(obj, p, p_max_size, cur->memmax ? (cur->mem * 255) / (cur->memmax) : 0);
			}
#ifdef X11
			OBJ(memgraph) {
				new_graph(obj, p, p_max_size, cur->memmax ? (cur->mem * 100.0) / (cur->memmax) : 0.0);
			}
#endif /* X11 */
			/* mixer stuff */
			OBJ(mixer) {
				print_mixer(obj, p, p_max_size);
			}
			OBJ(mixerl) {
				print_mixerl(obj, p, p_max_size);
			}
			OBJ(mixerr) {
				print_mixerr(obj, p, p_max_size);
			}
			OBJ(mixerbar) {
				print_mixer_bar(obj, p, p_max_size);
			}
			OBJ(mixerlbar) {
				print_mixerl_bar(obj, p, p_max_size);
			}
			OBJ(mixerrbar) {
				print_mixerr_bar(obj, p, p_max_size);
			}
			OBJ(if_mixer_mute) {
				if (!check_mixer_muted(obj)) {
					DO_JUMP;
				}
			}
#ifdef X11
#define NOT_IN_X "Not running in X"
			OBJ(monitor) {
				if(x_initialised != YES) {
					strncpy(p, NOT_IN_X, p_max_size);
				}else{
					snprintf(p, p_max_size, "%d", cur->x11.monitor.current);
				}
			}
			OBJ(monitor_number) {
				if(x_initialised != YES) {
					strncpy(p, NOT_IN_X, p_max_size);
				}else{
					snprintf(p, p_max_size, "%d", cur->x11.monitor.number);
				}
			}
			OBJ(desktop) {
				if(x_initialised != YES) {
					strncpy(p, NOT_IN_X, p_max_size);
				}else{
					snprintf(p, p_max_size, "%d", cur->x11.desktop.current);
				}
			}
			OBJ(desktop_number) {
				if(x_initialised != YES) {
					strncpy(p, NOT_IN_X, p_max_size);
				}else{
					snprintf(p, p_max_size, "%d", cur->x11.desktop.number);
				}
			}
			OBJ(desktop_name) {
				if(x_initialised != YES) {
					strncpy(p, NOT_IN_X, p_max_size);
				}else if(cur->x11.desktop.name != NULL) {
					strncpy(p, cur->x11.desktop.name, p_max_size);
				}
			}
#endif /* X11 */

			OBJ(format_time) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_format_time(obj, p, p_max_size);
			}
			/* mail stuff */
			OBJ(mails) {
				print_mails(obj, p, p_max_size);
			}
			OBJ(new_mails) {
				print_new_mails(obj, p, p_max_size);
			}
			OBJ(seen_mails) {
				print_seen_mails(obj, p, p_max_size);
			}
			OBJ(unseen_mails) {
				print_unseen_mails(obj, p, p_max_size);
			}
			OBJ(flagged_mails) {
				print_flagged_mails(obj, p, p_max_size);
			}
			OBJ(unflagged_mails) {
				print_unflagged_mails(obj, p, p_max_size);
			}
			OBJ(forwarded_mails) {
				print_forwarded_mails(obj, p, p_max_size);
			}
			OBJ(unforwarded_mails) {
				print_unforwarded_mails(obj, p, p_max_size);
			}
			OBJ(replied_mails) {
				print_replied_mails(obj, p, p_max_size);
			}
			OBJ(unreplied_mails) {
				print_unreplied_mails(obj, p, p_max_size);
			}
			OBJ(draft_mails) {
				print_draft_mails(obj, p, p_max_size);
			}
			OBJ(trashed_mails) {
				print_trashed_mails(obj, p, p_max_size);
			}
			OBJ(mboxscan) {
				print_mboxscan(obj, p, p_max_size);
			}
			OBJ(nodename) {
				snprintf(p, p_max_size, "%s", cur->uname_s.nodename);
			}
			OBJ(nodename_short) {
				char *pos;
				pos = strstr(cur->uname_s.nodename, ".");
				if(pos != NULL) {
					snprintf(p, MIN(pos-cur->uname_s.nodename+1, p_max_size), "%s", cur->uname_s.nodename);
				} else {
					snprintf(p, p_max_size, "%s", cur->uname_s.nodename);
				}
			}
			OBJ(outlinecolor) {
				new_outline(p, obj->data.l);
			}
			OBJ(pid_chroot) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_chroot(obj, p, p_max_size);
			}
			OBJ(pid_cmdline) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_cmdline(obj, p, p_max_size);
			}
			OBJ(pid_cwd) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_cwd(obj, p, p_max_size);
			}
			OBJ(pid_environ) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_environ(obj, p, p_max_size);
			}
			OBJ(pid_environ_list) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_environ_list(obj, p, p_max_size);
			}
			OBJ(pid_exe) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_exe(obj, p, p_max_size);
			}
			OBJ(pid_nice) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_nice(obj, p, p_max_size);
			}
			OBJ(pid_openfiles) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_openfiles(obj, p, p_max_size);
			}
			OBJ(pid_parent) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_parent(obj, p, p_max_size);
			}
			OBJ(pid_priority) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_priority(obj, p, p_max_size);
			}
			OBJ(pid_state) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_state(obj, p, p_max_size);
			}
			OBJ(pid_state_short) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_state_short(obj, p, p_max_size);
			}
			OBJ(pid_stderr) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_stderr(obj, p, p_max_size);
			}
			OBJ(pid_stdin) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_stdin(obj, p, p_max_size);
			}
			OBJ(pid_stdout) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_stdout(obj, p, p_max_size);
			}
			OBJ(pid_threads) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_threads(obj, p, p_max_size);
			}
			OBJ(pid_thread_list) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_thread_list(obj, p, p_max_size);
			}
			OBJ(pid_time_kernelmode) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_time_kernelmode(obj, p, p_max_size);
			}
			OBJ(pid_time_usermode) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_time_usermode(obj, p, p_max_size);
			}
			OBJ(pid_time) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_time(obj, p, p_max_size);
			}
			OBJ(pid_uid) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_uid(obj, p, p_max_size);
			}
			OBJ(pid_euid) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_euid(obj, p, p_max_size);
			}
			OBJ(pid_suid) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_suid(obj, p, p_max_size);
			}
			OBJ(pid_fsuid) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_fsuid(obj, p, p_max_size);
			}
			OBJ(pid_gid) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_gid(obj, p, p_max_size);
			}
			OBJ(pid_egid) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_egid(obj, p, p_max_size);
			}
			OBJ(pid_sgid) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_sgid(obj, p, p_max_size);
			}
			OBJ(pid_fsgid) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_fsgid(obj, p, p_max_size);
			}
			OBJ(pid_read) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_read(obj, p, p_max_size);
			}
			OBJ(pid_vmpeak) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_vmpeak(obj, p, p_max_size);
			}
			OBJ(pid_vmsize) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_vmsize(obj, p, p_max_size);
			}
			OBJ(pid_vmlck) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_vmlck(obj, p, p_max_size);
			}
			OBJ(pid_vmhwm) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_vmhwm(obj, p, p_max_size);
			}
			OBJ(pid_vmrss) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_vmrss(obj, p, p_max_size);
			}
			OBJ(pid_vmdata) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_vmdata(obj, p, p_max_size);
			}
			OBJ(pid_vmstk) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_vmstk(obj, p, p_max_size);
			}
			OBJ(pid_vmexe) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_vmexe(obj, p, p_max_size);
			}
			OBJ(pid_vmlib) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_vmlib(obj, p, p_max_size);
			}
			OBJ(pid_vmpte) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_vmpte(obj, p, p_max_size);
			}
			OBJ(pid_write) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_pid_write(obj, p, p_max_size);
			}
			OBJ(processes) {
				spaced_print(p, p_max_size, "%hu", 4, cur->procs);
			}
			OBJ(running_processes) {
				spaced_print(p, p_max_size, "%hu", 4, cur->run_procs);
			}
			OBJ(running_threads) {
				spaced_print(p, p_max_size, "%hu", 4, cur->run_threads);
			}
			OBJ(threads) {
				spaced_print(p, p_max_size, "%hu", 4, cur->threads);
			}
			OBJ(text) {
				snprintf(p, p_max_size, "%s", obj->data.s);
			}
#ifdef X11
			OBJ(shadecolor) {
				new_bg(p, obj->data.l);
			}
			OBJ(stippled_hr) {
				new_stippled_hr(obj, p);
			}
#endif /* X11 */
			OBJ(swap) {
				human_readable(cur->swap * 1024, p, 255);
			}
			OBJ(swapfree) {
				human_readable(cur->swapfree * 1024, p, 255);
			}
			OBJ(swapmax) {
				human_readable(cur->swapmax * 1024, p, 255);
			}
			OBJ(swapperc) {
				if (cur->swapmax == 0) {
					strncpy(p, "No swap", p_max_size);
				} else {
					percent_print(p, p_max_size, cur->swap * 100 / cur->swapmax);
				}
			}
			OBJ(swapbar) {
				new_bar(obj, p, p_max_size, cur->swapmax ? (cur->swap * 255) / (cur->swapmax) : 0);
			}
			OBJ(sysname) {
				snprintf(p, p_max_size, "%s", cur->uname_s.sysname);
			}
			OBJ(time) {
				print_time(obj, p, p_max_size);
			}
			OBJ(utime) {
				print_utime(obj, p, p_max_size);
			}
			OBJ(tztime) {
				print_tztime(obj, p, p_max_size);
			}
			OBJ(totaldown) {
				print_totaldown(obj, p, p_max_size);
			}
			OBJ(totalup) {
				print_totalup(obj, p, p_max_size);
			}
			OBJ(gid_name) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_gid_name(obj, p, p_max_size);
			}
			OBJ(uid_name) {
				char buf[max_user_text];

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				obj->data.s = buf;
				print_uid_name(obj, p, p_max_size);
			}
			OBJ(updates) {
				snprintf(p, p_max_size, "%d", total_updates);
			}
			OBJ(if_updatenr) {
				if(total_updates % updatereset != obj->data.i - 1) {
					DO_JUMP;
				}
			}
			OBJ(upspeed) {
				print_upspeed(obj, p, p_max_size);
			}
			OBJ(upspeedf) {
				print_upspeedf(obj, p, p_max_size);
			}
#ifdef X11
			OBJ(upspeedgraph) {
				print_upspeedgraph(obj, p, p_max_size);
			}
#endif /* X11 */
			OBJ(uptime_short) {
				format_seconds_short(p, p_max_size, (int) cur->uptime);
			}
			OBJ(uptime) {
				format_seconds(p, p_max_size, (int) cur->uptime);
			}
#ifdef __linux__
			OBJ(user_names) {
				snprintf(p, p_max_size, "%s", cur->users.names);
			}
			OBJ(user_terms) {
				snprintf(p, p_max_size, "%s", cur->users.terms);
			}
			OBJ(user_times) {
				snprintf(p, p_max_size, "%s", cur->users.times);
			}
			OBJ(user_time) {
				update_user_time(obj->data.s);
				snprintf(p, p_max_size, "%s", cur->users.ctime);
			}
			OBJ(user_number) {
				snprintf(p, p_max_size, "%d", cur->users.number);
			}
#endif /* __linux__ */
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
		|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
			OBJ(apm_adapter) {
				char *msg;

				msg = get_apm_adapter();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
			OBJ(apm_battery_life) {
				char *msg;

				msg = get_apm_battery_life();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
			OBJ(apm_battery_time) {
				char *msg;

				msg = get_apm_battery_time();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
#endif /* __FreeBSD__ __OpenBSD__ */

#ifdef MPD
			OBJ(mpd_title)
				print_mpd_title(obj, p, p_max_size);
			OBJ(mpd_artist)
				print_mpd_artist(obj, p, p_max_size);
			OBJ(mpd_albumartist)
				print_mpd_albumartist(obj, p, p_max_size);
			OBJ(mpd_album)
				print_mpd_album(obj, p, p_max_size);
			OBJ(mpd_date)
				print_mpd_date(obj, p, p_max_size);
			OBJ(mpd_random)
				print_mpd_random(obj, p, p_max_size);
			OBJ(mpd_repeat)
				print_mpd_repeat(obj, p, p_max_size);
			OBJ(mpd_track)
				print_mpd_track(obj, p, p_max_size);
			OBJ(mpd_name)
				print_mpd_name(obj, p, p_max_size);
			OBJ(mpd_file)
				print_mpd_file(obj, p, p_max_size);
			OBJ(mpd_vol)
				print_mpd_vol(obj, p, p_max_size);
			OBJ(mpd_bitrate)
				print_mpd_bitrate(obj, p, p_max_size);
			OBJ(mpd_status)
				print_mpd_status(obj, p, p_max_size);
			OBJ(mpd_elapsed) {
				print_mpd_elapsed(obj, p, p_max_size);
			}
			OBJ(mpd_length) {
				print_mpd_length(obj, p, p_max_size);
			}
			OBJ(mpd_percent) {
				print_mpd_percent(obj, p, p_max_size);
			}
			OBJ(mpd_bar) {
				print_mpd_bar(obj, p, p_max_size);
			}
			OBJ(mpd_smart) {
				print_mpd_smart(obj, p, p_max_size);
			}
			OBJ(if_mpd_playing) {
				if (!mpd_get_info()->is_playing) {
					DO_JUMP;
				}
			}
#endif

#ifdef MOC
#define MOC_PRINT(t, a) \
	snprintf(p, p_max_size, "%s", (moc.t ? moc.t : a))
			OBJ(moc_state) {
				MOC_PRINT(state, "??");
			}
			OBJ(moc_file) {
				MOC_PRINT(file, "no file");
			}
			OBJ(moc_title) {
				MOC_PRINT(title, "no title");
			}
			OBJ(moc_artist) {
				MOC_PRINT(artist, "no artist");
			}
			OBJ(moc_song) {
				MOC_PRINT(song, "no song");
			}
			OBJ(moc_album) {
				MOC_PRINT(album, "no album");
			}
			OBJ(moc_totaltime) {
				MOC_PRINT(totaltime, "0:00");
			}
			OBJ(moc_timeleft) {
				MOC_PRINT(timeleft, "0:00");
			}
			OBJ(moc_curtime) {
				MOC_PRINT(curtime, "0:00");
			}
			OBJ(moc_bitrate) {
				MOC_PRINT(bitrate, "0Kbps");
			}
			OBJ(moc_rate) {
				MOC_PRINT(rate, "0KHz");
			}
#undef MOC_PRINT
#endif /* MOC */
#ifdef XMMS2
			OBJ(xmms2_artist) {
				snprintf(p, p_max_size, "%s", cur->xmms2.artist);
			}
			OBJ(xmms2_album) {
				snprintf(p, p_max_size, "%s", cur->xmms2.album);
			}
			OBJ(xmms2_title) {
				snprintf(p, p_max_size, "%s", cur->xmms2.title);
			}
			OBJ(xmms2_genre) {
				snprintf(p, p_max_size, "%s", cur->xmms2.genre);
			}
			OBJ(xmms2_comment) {
				snprintf(p, p_max_size, "%s", cur->xmms2.comment);
			}
			OBJ(xmms2_url) {
				snprintf(p, p_max_size, "%s", cur->xmms2.url);
			}
			OBJ(xmms2_status) {
				snprintf(p, p_max_size, "%s", cur->xmms2.status);
			}
			OBJ(xmms2_date) {
				snprintf(p, p_max_size, "%s", cur->xmms2.date);
			}
			OBJ(xmms2_tracknr) {
				if (cur->xmms2.tracknr != -1) {
					snprintf(p, p_max_size, "%i", cur->xmms2.tracknr);
				}
			}
			OBJ(xmms2_bitrate) {
				snprintf(p, p_max_size, "%i", cur->xmms2.bitrate);
			}
			OBJ(xmms2_id) {
				snprintf(p, p_max_size, "%u", cur->xmms2.id);
			}
			OBJ(xmms2_size) {
				snprintf(p, p_max_size, "%2.1f", cur->xmms2.size);
			}
			OBJ(xmms2_elapsed) {
				snprintf(p, p_max_size, "%02d:%02d", cur->xmms2.elapsed / 60000,
					(cur->xmms2.elapsed / 1000) % 60);
			}
			OBJ(xmms2_duration) {
				snprintf(p, p_max_size, "%02d:%02d",
					cur->xmms2.duration / 60000,
					(cur->xmms2.duration / 1000) % 60);
			}
			OBJ(xmms2_percent) {
				snprintf(p, p_max_size, "%2.0f", cur->xmms2.progress * 100);
			}
#ifdef X11
			OBJ(xmms2_bar) {
				new_bar(obj, p, p_max_size, (int) (cur->xmms2.progress * 255.0f));
			}
#endif /* X11 */
			OBJ(xmms2_playlist) {
				snprintf(p, p_max_size, "%s", cur->xmms2.playlist);
			}
			OBJ(xmms2_timesplayed) {
				snprintf(p, p_max_size, "%i", cur->xmms2.timesplayed);
			}
			OBJ(xmms2_smart) {
				if (strlen(cur->xmms2.title) < 2
						&& strlen(cur->xmms2.title) < 2) {
					snprintf(p, p_max_size, "%s", cur->xmms2.url);
				} else {
					snprintf(p, p_max_size, "%s - %s", cur->xmms2.artist,
						cur->xmms2.title);
				}
			}
			OBJ(if_xmms2_connected) {
				if (cur->xmms2.conn_state != 1) {
					DO_JUMP;
				}
			}
#endif /* XMMS */
#ifdef AUDACIOUS
			OBJ(audacious_status) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_STATUS]);
			}
			OBJ(audacious_title) {
				snprintf(p, cur->audacious.max_title_len > 0
					? cur->audacious.max_title_len : p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_TITLE]);
			}
			OBJ(audacious_length) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_LENGTH]);
			}
			OBJ(audacious_length_seconds) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_LENGTH_SECONDS]);
			}
			OBJ(audacious_position) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_POSITION]);
			}
			OBJ(audacious_position_seconds) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_POSITION_SECONDS]);
			}
			OBJ(audacious_bitrate) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_BITRATE]);
			}
			OBJ(audacious_frequency) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_FREQUENCY]);
			}
			OBJ(audacious_channels) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_CHANNELS]);
			}
			OBJ(audacious_filename) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_FILENAME]);
			}
			OBJ(audacious_playlist_length) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_PLAYLIST_LENGTH]);
			}
			OBJ(audacious_playlist_position) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_PLAYLIST_POSITION]);
			}
			OBJ(audacious_main_volume) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_MAIN_VOLUME]);
			}
#ifdef X11
			OBJ(audacious_bar) {
				double progress;

				progress =
					atof(cur->audacious.items[AUDACIOUS_POSITION_SECONDS]) /
					atof(cur->audacious.items[AUDACIOUS_LENGTH_SECONDS]);
				new_bar(obj, p, p_max_size, (int) (progress * 255.0f));
			}
#endif /* X11 */
#endif /* AUDACIOUS */

#ifdef BMPX
			OBJ(bmpx_title) {
				snprintf(p, p_max_size, "%s", cur->bmpx.title);
			}
			OBJ(bmpx_artist) {
				snprintf(p, p_max_size, "%s", cur->bmpx.artist);
			}
			OBJ(bmpx_album) {
				snprintf(p, p_max_size, "%s", cur->bmpx.album);
			}
			OBJ(bmpx_uri) {
				snprintf(p, p_max_size, "%s", cur->bmpx.uri);
			}
			OBJ(bmpx_track) {
				snprintf(p, p_max_size, "%i", cur->bmpx.track);
			}
			OBJ(bmpx_bitrate) {
				snprintf(p, p_max_size, "%i", cur->bmpx.bitrate);
			}
#endif /* BMPX */
			/* we have four different types of top (top, top_mem,
			 * top_time and top_io). To avoid having almost-same code four
			 * times, we have this special handler. */
			break;
			case OBJ_top:
			case OBJ_top_mem:
			case OBJ_top_time:
#ifdef IOSTATS
			case OBJ_top_io:
#endif
				print_top(obj, p, p_max_size);
			OBJ(tail) {
				print_tailhead("tail", obj, p, p_max_size);
			}
			OBJ(head) {
				print_tailhead("head", obj, p, p_max_size);
			}
			OBJ(lines) {
				print_lines(obj, p, p_max_size);
			}

			OBJ(words) {
				print_words(obj, p, p_max_size);
			}
#ifdef TCP_PORT_MONITOR
			OBJ(tcp_portmon) {
				tcp_portmon_action(obj, p, p_max_size);
			}
#endif /* TCP_PORT_MONITOR */

#ifdef HAVE_ICONV
			OBJ(iconv_start) {
				do_iconv_start(obj);
			}
			OBJ(iconv_stop) {
				do_iconv_stop();
			}
#endif /* HAVE_ICONV */

			OBJ(entropy_avail) {
				print_entropy_avail(obj, p, p_max_size);
			}
			OBJ(entropy_perc) {
				print_entropy_perc(obj, p, p_max_size);
			}
			OBJ(entropy_poolsize) {
				print_entropy_poolsize(obj, p, p_max_size);
			}
			OBJ(entropy_bar) {
				print_entropy_bar(obj, p, p_max_size);
			}
#ifdef IBM
			OBJ(smapi) {
				print_smapi(obj, p, p_max_size);
			}
			OBJ(if_smapi_bat_installed) {
				int idx;
				if(obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
					if(!smapi_bat_installed(idx)) {
						DO_JUMP;
					}
				} else
					NORM_ERR("argument to if_smapi_bat_installed must be an integer");
			}
			OBJ(smapi_bat_perc) {
				print_smapi_bat_perc(obj, p, p_max_size);
			}
			OBJ(smapi_bat_temp) {
				print_smapi_bat_temp(obj, p, p_max_size);
			}
			OBJ(smapi_bat_power) {
				print_smapi_bat_power(obj, p, p_max_size);
			}
			OBJ(smapi_bat_bar) {
				print_smapi_bat_bar(obj, p, p_max_size);
			}
#endif /* IBM */
			OBJ(include) {
				if(obj->sub) {
					char buf[max_user_text];

					generate_text_internal(buf, max_user_text, *obj->sub, cur);
					snprintf(p, p_max_size, "%s", buf);
				} else {
					p[0] = 0;
				}
			}
			OBJ(blink) {
				//blinking like this can look a bit ugly if the chars in the font don't have the same width
				char buf[max_user_text];
				unsigned int j;

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				snprintf(p, p_max_size, "%s", buf);
				if(total_updates % 2) {
					for(j=0; p[j] != 0; j++) {
						p[j] = ' ';
					}
				}
			}
			OBJ(to_bytes) {
				char buf[max_user_text];
				long long bytes;
				char unit[16];	// 16 because we can also have long names (like mega-bytes)

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				if(sscanf(buf, "%lli%s", &bytes, unit) == 2 && strlen(unit) < 16){
					if(strncasecmp("b", unit, 1) == 0) snprintf(buf, max_user_text, "%lli", bytes);
					else if(strncasecmp("k", unit, 1) == 0) snprintf(buf, max_user_text, "%lli", bytes * 1024);
					else if(strncasecmp("m", unit, 1) == 0) snprintf(buf, max_user_text, "%lli", bytes * 1024 * 1024);
					else if(strncasecmp("g", unit, 1) == 0) snprintf(buf, max_user_text, "%lli", bytes * 1024 * 1024 * 1024);
					else if(strncasecmp("t", unit, 1) == 0) snprintf(buf, max_user_text, "%lli", bytes * 1024 * 1024 * 1024 * 1024);
				}
				snprintf(p, p_max_size, "%s", buf);
			}
			OBJ(scroll) {
				print_scroll(obj, p, p_max_size, cur);
			}
			OBJ(combine) {
				print_combine(obj, p, cur);
			}
#ifdef NVIDIA
			OBJ(nvidia) {
				print_nvidia_value(obj, p, p_max_size);
			}
#endif /* NVIDIA */
#ifdef APCUPSD
			OBJ(apcupsd) {
				/* This is just a meta-object to set host:port */
			}
			OBJ(apcupsd_name) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_NAME]);
			}
			OBJ(apcupsd_model) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_MODEL]);
			}
			OBJ(apcupsd_upsmode) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_UPSMODE]);
			}
			OBJ(apcupsd_cable) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_CABLE]);
			}
			OBJ(apcupsd_status) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_STATUS]);
			}
			OBJ(apcupsd_linev) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_LINEV]);
			}
			OBJ(apcupsd_load) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_LOAD]);
			}
			OBJ(apcupsd_loadbar) {
				double progress;
				progress = atof(cur->apcupsd.items[APCUPSD_LOAD]) / 100.0 * 255.0;
				new_bar(obj, p, p_max_size, (int) progress);
			}
#ifdef X11
			OBJ(apcupsd_loadgraph) {
				double progress;
				progress =	atof(cur->apcupsd.items[APCUPSD_LOAD]);
				new_graph(obj, p, p_max_size, (int)progress);
			}
#endif /* X11 */
			OBJ(apcupsd_loadgauge) {
				double progress;
				progress =	atof(cur->apcupsd.items[APCUPSD_LOAD]) / 100.0 * 255.0;
				new_gauge(obj, p, p_max_size, (int)progress);
			}
			OBJ(apcupsd_charge) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_CHARGE]);
			}
			OBJ(apcupsd_timeleft) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_TIMELEFT]);
			}
			OBJ(apcupsd_temp) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_TEMP]);
			}
			OBJ(apcupsd_lastxfer) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_LASTXFER]);
			}
#endif /* APCUPSD */
#ifdef JACK
			OBJ(if_jack_active) {
				if (!(cur->jack.state & JACK_IS_ACTIVE)) {
					DO_JUMP;
				}
			}
			OBJ(jack_cpu_load) {
				snprintf(p, p_max_size, "%2.2f",
						cur->jack.cpu_load);
			}
			OBJ(jack_buffer_size) {
				snprintf(p, p_max_size, "%u",
						cur->jack.buffer_size);
			}
			OBJ(jack_sample_rate) {
				snprintf(p, p_max_size, "%u",
						cur->jack.sample_rate);
			}
			OBJ(jack_xruns) {
				snprintf(p, p_max_size, "%d",
						cur->jack.xruns);
			}
			OBJ(if_jack_rolling) {
				if (!(cur->jack.state & JACK_IS_ROLLING)) {
					DO_JUMP;
				}
			}
			OBJ(jack_frame) {
				snprintf(p, p_max_size, "%d",
						cur->jack.frame);
			}
			OBJ(jack_hour) {
				snprintf(p, p_max_size, "%2d",
						cur->jack.hour);
			}
			OBJ(jack_min) {
				snprintf(p, p_max_size, "%02d",
						cur->jack.min);
			}
			OBJ(jack_sec) {
				snprintf(p, p_max_size, "%02d",
						cur->jack.sec);
			}
			OBJ(if_jack_bbt) {
				if(!(cur->jack.state & JACK_IS_BBT)) {
					DO_JUMP;
				}
			}
			OBJ(jack_beat_type) {
				snprintf(p, p_max_size, "%.0f",
						cur->jack.beat_type);
			}
			OBJ(jack_beats_per_bar) {
				snprintf(p, p_max_size, "%.0f",
						cur->jack.beats_per_bar);
			}
			OBJ(jack_bpm) {
				snprintf(p, p_max_size, "%.0lf",
						cur->jack.bpm);
			}
			OBJ(jack_bar) {
				snprintf(p, p_max_size, "%03d",
						cur->jack.bar);
			}
			OBJ(jack_beat) {
				snprintf(p, p_max_size, "%02d",
						cur->jack.beat);
			}
			OBJ(jack_tick) {
				snprintf(p, p_max_size, "%04d",
						cur->jack.tick);
			}
#endif /* JACK */
			break;
		}
#undef DO_JUMP


		{
			size_t a = strlen(p);

#ifdef HAVE_ICONV
			iconv_convert(&a, buff_in, p, p_max_size);
#endif /* HAVE_ICONV */
			p += a;
			p_max_size -= a;
			(*p) = 0;
		}
		obj = obj->next;
	}
#ifdef X11
	/* load any new fonts we may have had */
	if (need_to_load_fonts) {
		load_fonts();
	}
#endif /* X11 */
}

void evaluate(const char *text, char *p, int p_max_size)
{
	struct information *tmp_info;
	struct text_object subroot;

	tmp_info = malloc(sizeof(struct information));
	memcpy(tmp_info, &info, sizeof(struct information));
	parse_conky_vars(&subroot, text, p, p_max_size, tmp_info);
	DBGP2("evaluated '%s' to '%s'", text, p);

	free_text_objects(&subroot, 1);
	free(tmp_info);
}

double current_update_time, next_update_time, last_update_time;

static void generate_text(void)
{
	struct information *cur = &info;
	char *p;
	unsigned int i, j, k;

	special_count = 0;

	/* update info */

	current_update_time = get_time();

	update_stuff();

	/* add things to the buffer */

	/* generate text */

	p = text_buffer;

	generate_text_internal(p, max_user_text, global_root_object, cur);
	if(max_text_width > 0) {
		for(i = 0, j = 0; p[i] != 0; i++) {
			if(p[i] == '\n') j = 0;
			else if(j == max_text_width) {
				k = i + strlen(p + i) + 1;
				if(k < text_buffer_size) {
					while(k != i) {
						p[k] = p[k-1];
						k--;
					}
					p[k] = '\n';
					j = 0;
				} else NORM_ERR("The end of the text_buffer is reached, increase \"text_buffer_size\"");
			} else j++;
		}
	}

	if (stuff_in_uppercase) {
		char *tmp_p;

		tmp_p = text_buffer;
		while (*tmp_p) {
			*tmp_p = toupper(*tmp_p);
			tmp_p++;
		}
	}

	next_update_time += update_interval;
	if (next_update_time < get_time()) {
		next_update_time = get_time() + update_interval;
	} else if (next_update_time > get_time() + update_interval) {
		next_update_time = get_time() + update_interval;
	}
	last_update_time = current_update_time;
	total_updates++;
}

void set_update_interval(double interval)
{
	update_interval = interval;
	update_interval_old = interval;
}

int get_string_width(const char *s)
{
	return *s ? calc_text_width(s) : 0;
}

#ifdef X11
static int get_string_width_special(char *s, int special_index)
{
	char *p, *final;
	int idx = 1;
	int width = 0;
	long i;

	if (!s)
		return 0;

	if ((output_methods & TO_X) == 0)
		return strlen(s);

	p = strndup(s, text_buffer_size);
	final = p;

	while (*p) {
		if (*p == SPECIAL_CHAR) {
			/* shift everything over by 1 so that the special char
			 * doesn't mess up the size calculation */
			for (i = 0; i < (long)strlen(p); i++) {
				*(p + i) = *(p + i + 1);
			}
			if (specials[special_index + idx].type == GRAPH
					|| specials[special_index + idx].type == GAUGE
					|| specials[special_index + idx].type == BAR) {
				width += specials[special_index + idx].width;
			}
			idx++;
		} else {
			p++;
		}
	}
	if (strlen(final) > 1) {
		width += calc_text_width(final);
	}
	free(final);
	return width;
}

static int text_size_updater(char *s, int special_index);

int last_font_height;
static void update_text_area(void)
{
	int x = 0, y = 0;

	if ((output_methods & TO_X) == 0)
		return;
	/* update text size if it isn't fixed */
#ifdef OWN_WINDOW
	if (!fixed_size)
#endif
	{
		text_width = minimum_width;
		text_height = 0;
		last_font_height = font_height();
		for_each_line(text_buffer, text_size_updater);
		text_width += 1;
		if (text_height < minimum_height) {
			text_height = minimum_height;
		}
		if (text_width > maximum_width && maximum_width > 0) {
			text_width = maximum_width;
		}
	}

	/* get text position on workarea */
	switch (text_alignment) {
		case TOP_LEFT: case TOP_RIGHT: case TOP_MIDDLE:
			y = gap_y;
			break;

		case BOTTOM_LEFT: case BOTTOM_RIGHT: case BOTTOM_MIDDLE: default:
			y = workarea[3] - text_height - gap_y;
			break;

		case MIDDLE_LEFT: case MIDDLE_RIGHT: case MIDDLE_MIDDLE:
			y = workarea[3] / 2 - text_height / 2 - gap_y;
			break;
	}
	switch (text_alignment) {
		case TOP_LEFT: case BOTTOM_LEFT: case MIDDLE_LEFT: default:
			x = gap_x;
			break;

		case TOP_RIGHT: case BOTTOM_RIGHT: case MIDDLE_RIGHT:
			x = workarea[2] - text_width - gap_x;
			break;

		case TOP_MIDDLE: case BOTTOM_MIDDLE: case MIDDLE_MIDDLE:
			x = workarea[2] / 2 - text_width / 2 - gap_x;
			break;
	}
#ifdef OWN_WINDOW
	if (text_alignment == NONE) {	// Let the WM manage the window
			x = window.x;
			y = window.y;

			fixed_pos = 1;
			fixed_size = 1;
	}
#endif /* OWN_WINDOW */
#ifdef OWN_WINDOW

	if (own_window && !fixed_pos) {
		x += workarea[0];
		y += workarea[1];
		text_start_x = window.border_inner_margin + window.border_outer_margin + window.border_width;
		text_start_y = window.border_inner_margin + window.border_outer_margin + window.border_width;
		window.x = x - window.border_inner_margin - window.border_outer_margin - window.border_width;
		window.y = y - window.border_inner_margin - window.border_outer_margin - window.border_width;
	} else
#endif
	{
		/* If window size doesn't match to workarea's size,
		 * then window probably includes panels (gnome).
		 * Blah, doesn't work on KDE. */
		if (workarea[2] != window.width || workarea[3] != window.height) {
			y += workarea[1];
			x += workarea[0];
		}

		text_start_x = x;
		text_start_y = y;
	}
#ifdef HAVE_LUA
	/* update lua window globals */
	llua_update_window_table(text_start_x, text_start_y, text_width, text_height);
#endif /* HAVE_LUA */
}

/* drawing stuff */

static int cur_x, cur_y;	/* current x and y for drawing */
#endif
//draw_mode also without X11 because we only need to print to stdout with FG
static int draw_mode;		/* FG, BG or OUTLINE */
#ifdef X11
static long current_color;

static int text_size_updater(char *s, int special_index)
{
	int w = 0;
	char *p;

	if ((output_methods & TO_X) == 0)
		return 0;
	/* get string widths and skip specials */
	p = s;
	while (*p) {
		if (*p == SPECIAL_CHAR) {
			*p = '\0';
			w += get_string_width(s);
			*p = SPECIAL_CHAR;

			if (specials[special_index].type == BAR
					|| specials[special_index].type == GAUGE
					|| specials[special_index].type == GRAPH) {
				w += specials[special_index].width;
				if (specials[special_index].height > last_font_height) {
					last_font_height = specials[special_index].height;
					last_font_height += font_height();
				}
			} else if (specials[special_index].type == OFFSET) {
				if (specials[special_index].arg > 0) {
					w += specials[special_index].arg;
				}
			} else if (specials[special_index].type == VOFFSET) {
				last_font_height += specials[special_index].arg;
			} else if (specials[special_index].type == GOTO) {
				if (specials[special_index].arg > cur_x) {
					w = (int) specials[special_index].arg;
				}
			} else if (specials[special_index].type == TAB) {
				int start = specials[special_index].arg;
				int step = specials[special_index].width;

				if (!step || step < 0) {
					step = 10;
				}
				w += step - (cur_x - text_start_x - start) % step;
			} else if (specials[special_index].type == FONT) {
				selected_font = specials[special_index].font_added;
				if (font_height() > last_font_height) {
					last_font_height = font_height();
				}
			}

			special_index++;
			s = p + 1;
		}
		p++;
	}

	w += get_string_width(s);

	if (w > text_width) {
		text_width = w;
	}
	if (text_width > maximum_width && maximum_width) {
		text_width = maximum_width;
	}

	text_height += last_font_height;
	last_font_height = font_height();
	return special_index;
}
#endif /* X11 */

static inline void set_foreground_color(long c)
{
#ifdef X11
	if (output_methods & TO_X) {
#ifdef USE_ARGB
		if (have_argb_visual) {
			current_color = c | (own_window_argb_value << 24);
		} else {
#endif /* USE_ARGB */
			current_color = c;
#ifdef USE_ARGB
		}
#endif /* USE_ARGB */
		XSetForeground(display, window.gc, current_color);
	}
#endif /* X11 */
#ifdef NCURSES
	if (output_methods & TO_NCURSES) {
		attron(COLOR_PAIR(c));
	}
#endif /* NCURSES */
	UNUSED(c);
	return;
}

static void draw_string(const char *s)
{
	int i, i2, pos, width_of_s;
	int max = 0;
	int added;

	if (s[0] == '\0') {
		return;
	}

	width_of_s = get_string_width(s);
	if ((output_methods & TO_STDOUT) && draw_mode == FG) {
		printf("%s\n", s);
		if (extra_newline) fputc('\n', stdout);
		fflush(stdout);	/* output immediately, don't buffer */
	}
	if ((output_methods & TO_STDERR) && draw_mode == FG) {
		fprintf(stderr, "%s\n", s);
		fflush(stderr);	/* output immediately, don't buffer */
	}
	if ((output_methods & OVERWRITE_FILE) && draw_mode == FG && overwrite_fpointer) {
		fprintf(overwrite_fpointer, "%s\n", s);
	}
	if ((output_methods & APPEND_FILE) && draw_mode == FG && append_fpointer) {
		fprintf(append_fpointer, "%s\n", s);
	}
#ifdef NCURSES
	if ((output_methods & TO_NCURSES) && draw_mode == FG) {
		printw("%s", s);
	}
#endif
	memset(tmpstring1, 0, text_buffer_size);
	memset(tmpstring2, 0, text_buffer_size);
	strncpy(tmpstring1, s, text_buffer_size - 1);
	pos = 0;
	added = 0;

#ifdef X11
	if (output_methods & TO_X) {
		max = ((text_width - width_of_s) / get_string_width(" "));
	}
#endif /* X11 */
	/* This code looks for tabs in the text and coverts them to spaces.
	 * The trick is getting the correct number of spaces, and not going
	 * over the window's size without forcing the window larger. */
	for (i = 0; i < (int) text_buffer_size; i++) {
		if (tmpstring1[i] == '\t') {
			i2 = 0;
			for (i2 = 0; i2 < (8 - (1 + pos) % 8) && added <= max; i2++) {
				/* guard against overrun */
				tmpstring2[MIN(pos + i2, (int)text_buffer_size - 1)] = ' ';
				added++;
			}
			pos += i2;
		} else {
			/* guard against overrun */
			tmpstring2[MIN(pos, (int) text_buffer_size - 1)] = tmpstring1[i];
			pos++;
		}
	}
#ifdef X11
	if (output_methods & TO_X) {
		if (text_width == maximum_width) {
			/* this means the text is probably pushing the limit,
			 * so we'll chop it */
			while (cur_x + get_string_width(tmpstring2) - text_start_x
					> maximum_width && strlen(tmpstring2) > 0) {
				tmpstring2[strlen(tmpstring2) - 1] = '\0';
			}
		}
	}
#endif /* X11 */
	s = tmpstring2;
#ifdef X11
	if (output_methods & TO_X) {
#ifdef XFT
		if (use_xft) {
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
			if (utf8_mode) {
				XftDrawStringUtf8(window.xftdraw, &c2, fonts[selected_font].xftfont,
					cur_x, cur_y, (const XftChar8 *) s, strlen(s));
			} else {
				XftDrawString8(window.xftdraw, &c2, fonts[selected_font].xftfont,
					cur_x, cur_y, (const XftChar8 *) s, strlen(s));
			}
		} else
#endif
		{
			XDrawString(display, window.drawable, window.gc, cur_x, cur_y, s,
				strlen(s));
		}
		cur_x += width_of_s;
	}
#endif /* X11 */
	memcpy(tmpstring1, s, text_buffer_size);
}

int draw_each_line_inner(char *s, int special_index, int last_special_applied)
{
#ifdef X11
	int font_h;
	int cur_y_add = 0;
#endif /* X11 */
	char *p = s;
	int orig_special_index = special_index;

#ifdef X11
	if (output_methods & TO_X) {
		font_h = font_height();
		cur_y += font_ascent();
	}
	cur_x = text_start_x;
#endif /* X11 */

	while (*p) {
		if (*p == SPECIAL_CHAR || last_special_applied > -1) {
#ifdef X11
			int w = 0;
#endif /* X11 */

			/* draw string before special, unless we're dealing multiline
			 * specials */
			if (last_special_applied > -1) {
				special_index = last_special_applied;
			} else {
				*p = '\0';
				draw_string(s);
				*p = SPECIAL_CHAR;
				s = p + 1;
			}
			/* draw special */
			switch (specials[special_index].type) {
#ifdef X11
				case HORIZONTAL_LINE:
				{
					int h = specials[special_index].height;
					int mid = font_ascent() / 2;

					w = text_start_x + text_width - cur_x;

					XSetLineAttributes(display, window.gc, h, LineSolid,
						CapButt, JoinMiter);
					XDrawLine(display, window.drawable, window.gc, cur_x,
						cur_y - mid / 2, cur_x + w, cur_y - mid / 2);
					break;
				}

				case STIPPLED_HR:
				{
					int h = specials[special_index].height;
					int tmp_s = specials[special_index].arg;
					int mid = font_ascent() / 2;
					char ss[2] = { tmp_s, tmp_s };

					w = text_start_x + text_width - cur_x - 1;
					XSetLineAttributes(display, window.gc, h, LineOnOffDash,
						CapButt, JoinMiter);
					XSetDashes(display, window.gc, 0, ss, 2);
					XDrawLine(display, window.drawable, window.gc, cur_x,
						cur_y - mid / 2, cur_x + w, cur_y - mid / 2);
					break;
				}

				case BAR:
				{
					int h, bar_usage, by;
					if (cur_x - text_start_x > maximum_width
							&& maximum_width > 0) {
						break;
					}
					h = specials[special_index].height;
					bar_usage = specials[special_index].arg;
					by = cur_y - (font_ascent() / 2) - 1;

					if (h < font_h) {
						by -= h / 2 - 1;
					}
					w = specials[special_index].width;
					if (w == 0) {
						w = text_start_x + text_width - cur_x - 1;
					}
					if (w < 0) {
						w = 0;
					}

					XSetLineAttributes(display, window.gc, 1, LineSolid,
						CapButt, JoinMiter);

					XDrawRectangle(display, window.drawable, window.gc, cur_x,
						by, w, h);
					XFillRectangle(display, window.drawable, window.gc, cur_x,
						by, w * bar_usage / 255, h);
					if (h > cur_y_add
							&& h > font_h) {
						cur_y_add = h;
					}
					break;
				}

				case GAUGE: /* new GAUGE  */
				{
					int h, by = 0;
					unsigned long last_colour = current_color;
#ifdef MATH
					float angle, px, py;
					int usage;
#endif /* MATH */

					if (cur_x - text_start_x > maximum_width
							&& maximum_width > 0) {
						break;
					}

					h = specials[special_index].height;
					by = cur_y - (font_ascent() / 2) - 1;

					if (h < font_h) {
						by -= h / 2 - 1;
					}
					w = specials[special_index].width;
					if (w == 0) {
						w = text_start_x + text_width - cur_x - 1;
					}
					if (w < 0) {
						w = 0;
					}

					XSetLineAttributes(display, window.gc, 1, LineSolid,
							CapButt, JoinMiter);

					XDrawArc(display, window.drawable, window.gc,
							cur_x, by, w, h * 2, 0, 180*64);

#ifdef MATH
					usage = specials[special_index].arg;
					angle = (M_PI)*(float)(usage)/255.;
					px = (float)(cur_x+(w/2.))-(float)(w/2.)*cos(angle);
					py = (float)(by+(h))-(float)(h)*sin(angle);

					XDrawLine(display, window.drawable, window.gc,
							cur_x + (w/2.), by+(h), (int)(px), (int)(py));
#endif /* MATH */

					if (h > cur_y_add
							&& h > font_h) {
						cur_y_add = h;
					}

					set_foreground_color(last_colour);

					break;

				}

				case GRAPH:
				{
					int h, by, i = 0, j = 0;
					int colour_idx = 0;
					unsigned long last_colour = current_color;
					unsigned long *tmpcolour = 0;
					if (cur_x - text_start_x > maximum_width
							&& maximum_width > 0) {
						break;
					}
					h = specials[special_index].height;
					by = cur_y - (font_ascent() / 2) - 1;

					if (h < font_h) {
						by -= h / 2 - 1;
					}
					w = specials[special_index].width;
					if (w == 0) {
						w = text_start_x + text_width - cur_x - 1;
						specials[special_index].graph_width = MAX(w - 1, 0);
						if (specials[special_index].graph_width != specials[special_index].graph_allocated) {
							w = specials[special_index].graph_allocated + 1;
						}
					}
					if (w < 0) {
						w = 0;
					}
					if (draw_graph_borders) {
						XSetLineAttributes(display, window.gc, 1, LineSolid,
							CapButt, JoinMiter);
						XDrawRectangle(display, window.drawable, window.gc,
							cur_x, by, w, h);
					}
					XSetLineAttributes(display, window.gc, 1, LineSolid,
						CapButt, JoinMiter);

					/* in case we don't have a graph yet */
					if (specials[special_index].graph) {

						if (specials[special_index].last_colour != 0
								|| specials[special_index].first_colour != 0) {
							tmpcolour = do_gradient(w - 1,
									specials[special_index].last_colour,
									specials[special_index].first_colour);
						}
						colour_idx = 0;

						int og, g = -1; // dmr's here..

						for (i = w - 2; i > -1; i--) {
							if (specials[special_index].last_colour != 0
									|| specials[special_index].first_colour != 0) {
								if (specials[special_index].tempgrad) {
#ifdef DEBUG_lol
									assert(
											(int)((float)(w - 2) - specials[special_index].graph[j] *
												(w - 2) / (float)specials[special_index].graph_scale)
											< w - 1
										  );
									assert(
											(int)((float)(w - 2) - specials[special_index].graph[j] *
												(w - 2) / (float)specials[special_index].graph_scale)
											> -1
										  );
									if (specials[special_index].graph[j] == specials[special_index].graph_scale) {
										assert(
												(int)((float)(w - 2) - specials[special_index].graph[j] *
													(w - 2) / (float)specials[special_index].graph_scale)
												== 0
											  );
									}
#endif /* DEBUG_lol */
									set_foreground_color(tmpcolour[
											(int)((float)(w - 2) -
												specials[special_index].graph[j]
												* (w - 2) /
												(float)specials[special_index].graph_scale)
											]);
								} else {
									set_foreground_color(tmpcolour[colour_idx++]);
								}
							}

							og = g;
							g = round_to_int((double)by + h - specials[special_index].graph[j] *
											 (h - 1) / specials[special_index].graph_scale);

							if (!~og) {
								og = g;
							}

							if (specials[special_index].dotgraph) {
								if (og == g) {
									XDrawPoint(display, window.drawable,
											   window.gc, cur_x + i + 1, g);
								} else {
									XDrawLine(display, window.drawable, window.gc,
											  cur_x + i + 1,
											  og,
											  cur_x + i + 1,
											  g);
								}
							} else {
								/* this is mugfugly, but it works */
								XDrawLine(display, window.drawable, window.gc,
										  cur_x + i + 1,
										  by + h,
										  cur_x + i + 1,
										  g);
							}

							++j;
						}
						if (tmpcolour) free(tmpcolour);
					}
					if (h > cur_y_add
							&& h > font_h) {
						cur_y_add = h;
					}
					/* if (draw_mode == BG) {
						set_foreground_color(default_bg_color);
					} else if (draw_mode == OUTLINE) {
						set_foreground_color(default_out_color);
					} else {
						set_foreground_color(default_fg_color);
					} */
					if (show_graph_range) {
						int tmp_x = cur_x;
						int tmp_y = cur_y;
						unsigned short int seconds = update_interval * w;
						char *tmp_day_str;
						char *tmp_hour_str;
						char *tmp_min_str;
						char *tmp_sec_str;
						char *tmp_str;
						unsigned short int timeunits;
						if (seconds != 0) {
							timeunits = seconds / 86400; seconds %= 86400;
							if (timeunits > 0) {
								if (asprintf(&tmp_day_str, "%dd", timeunits) < 0) {
									tmp_day_str = 0;
								}
							} else {
								tmp_day_str = strdup("");
							}
							timeunits = seconds / 3600; seconds %= 3600;
							if (timeunits > 0) {
								if (asprintf(&tmp_hour_str, "%dh", timeunits) < 0) {
									tmp_day_str = 0;
								}
							} else {
								tmp_hour_str = strdup("");
							}
							timeunits = seconds / 60; seconds %= 60;
							if (timeunits > 0) {
								if (asprintf(&tmp_min_str, "%dm", timeunits) < 0) {
									tmp_min_str = 0;
								}
							} else {
								tmp_min_str = strdup("");
							}
							if (seconds > 0) {
								if (asprintf(&tmp_sec_str, "%ds", seconds) < 0) {
									tmp_sec_str = 0;
								}
							} else {
								tmp_sec_str = strdup("");
							}
							if (asprintf(&tmp_str, "%s%s%s%s", tmp_day_str,
										tmp_hour_str, tmp_min_str, tmp_sec_str) < 0) {
								tmp_str = 0;
							}
#define FREE(a) if ((a)) free((a));
							FREE(tmp_day_str); FREE(tmp_hour_str); FREE(tmp_min_str); FREE(tmp_sec_str);
						} else {
							tmp_str = strdup("Range not possible"); /* should never happen, but better safe then sorry */
						}
						cur_x += (w / 2) - (font_ascent() * (strlen(tmp_str) / 2));
						cur_y += font_h / 2;
						draw_string(tmp_str);
						FREE(tmp_str);
#undef FREE
						cur_x = tmp_x;
						cur_y = tmp_y;
					}
#ifdef MATH
					if (show_graph_scale && (specials[special_index].show_scale == 1)) {
						int tmp_x = cur_x;
						int tmp_y = cur_y;
						char *tmp_str;
						cur_x += font_ascent() / 2;
						cur_y += font_h / 2;
						tmp_str = (char *) calloc(20, sizeof(char));
						sprintf(tmp_str, "%.1f", specials[special_index].graph_scale);
						draw_string(tmp_str);
						free(tmp_str);
						cur_x = tmp_x;
						cur_y = tmp_y;
					}
#endif
					set_foreground_color(last_colour);
					break;
				}

				case FONT:
				{
					int old = font_ascent();

					cur_y -= font_ascent();
					selected_font = specials[special_index].font_added;
					set_font();
					if (cur_y + font_ascent() < cur_y + old) {
						cur_y += old;
					} else {
						cur_y += font_ascent();
					}
					font_h = font_height();
					break;
				}
#endif /* X11 */
				case FG:
					if (draw_mode == FG) {
						set_foreground_color(specials[special_index].arg);
					}
					break;

#ifdef X11
				case BG:
					if (draw_mode == BG) {
						set_foreground_color(specials[special_index].arg);
					}
					break;

				case OUTLINE:
					if (draw_mode == OUTLINE) {
						set_foreground_color(specials[special_index].arg);
					}
					break;

				case OFFSET:
					w += specials[special_index].arg;
					break;

				case VOFFSET:
					cur_y += specials[special_index].arg;
					break;

				case GOTO:
					if (specials[special_index].arg >= 0) {
						cur_x = (int) specials[special_index].arg;
#ifdef X11
						//make sure shades are 1 pixel to the right of the text
						if(draw_mode == BG) cur_x++;
#endif
					}
					break;

				case TAB:
				{
					int start = specials[special_index].arg;
					int step = specials[special_index].width;

					if (!step || step < 0) {
						step = 10;
					}
					w = step - (cur_x - text_start_x - start) % step;
					break;
				}

				case ALIGNR:
				{
					/* TODO: add back in "+ window.border_inner_margin" to the end of
					 * this line? */
					int pos_x = text_start_x + text_width -
						get_string_width_special(s, special_index);

					/* printf("pos_x %i text_start_x %i text_width %i cur_x %i "
						"get_string_width(p) %i gap_x %i "
						"specials[special_index].arg %i window.border_inner_margin %i "
						"window.border_width %i\n", pos_x, text_start_x, text_width,
						cur_x, get_string_width_special(s), gap_x,
						specials[special_index].arg, window.border_inner_margin,
						window.border_width); */
					if (pos_x > specials[special_index].arg && pos_x > cur_x) {
						cur_x = pos_x - specials[special_index].arg;
					}
					break;
				}

				case ALIGNC:
				{
					int pos_x = (text_width) / 2 - get_string_width_special(s,
							special_index) / 2 - (cur_x -
								text_start_x);
					/* int pos_x = text_start_x + text_width / 2 -
						get_string_width_special(s) / 2; */

					/* printf("pos_x %i text_start_x %i text_width %i cur_x %i "
						"get_string_width(p) %i gap_x %i "
						"specials[special_index].arg %i\n", pos_x, text_start_x,
						text_width, cur_x, get_string_width(s), gap_x,
						specials[special_index].arg); */
					if (pos_x > specials[special_index].arg) {
						w = pos_x - specials[special_index].arg;
					}
					break;
				}
#endif /* X11 */
			}

#ifdef X11
			cur_x += w;
#endif /* X11 */

			if (special_index != last_special_applied) {
				special_index++;
			} else {
				special_index = orig_special_index;
				last_special_applied = -1;
			}
		}
		p++;
	}

#ifdef X11
	cur_y += cur_y_add;
#endif /* X11 */
	draw_string(s);
#ifdef NCURSES
	if (output_methods & TO_NCURSES) {
		printw("\n");
	}
#endif /* NCURSES */
#ifdef X11
	if (output_methods & TO_X)
		cur_y += font_descent();
#endif /* X11 */
	return special_index;
}

static int draw_line(char *s, int special_index)
{
#ifdef X11
	if (output_methods & TO_X) {
		return draw_each_line_inner(s, special_index, -1);
	}
#endif /* X11 */
#ifdef NCURSES
	if (output_methods & TO_NCURSES) {
		return draw_each_line_inner(s, special_index, -1);
	}
#endif /* NCURSES */
	draw_string(s);
	UNUSED(special_index);
	return 0;
}

static void draw_text(void)
{
#ifdef X11
	if (output_methods & TO_X) {
		cur_y = text_start_y;

		/* draw borders */
		if (draw_borders && window.border_width > 0) {
			if (stippled_borders) {
				char ss[2] = { stippled_borders, stippled_borders };
				XSetLineAttributes(display, window.gc, window.border_width, LineOnOffDash,
					CapButt, JoinMiter);
				XSetDashes(display, window.gc, 0, ss, 2);
			} else {
				XSetLineAttributes(display, window.gc, window.border_width, LineSolid,
					CapButt, JoinMiter);
			}

			XDrawRectangle(display, window.drawable, window.gc,
				text_start_x - window.border_inner_margin - window.border_width,
				text_start_y - window.border_inner_margin - window.border_width,
				text_width + window.border_inner_margin * 2 + window.border_width * 2,
				text_height + window.border_inner_margin * 2 + window.border_width * 2);
		}

		/* draw text */
	}
	setup_fonts();
#endif /* X11 */
#ifdef NCURSES
	init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
	attron(COLOR_PAIR(COLOR_WHITE));
#endif /* NCURSES */
	for_each_line(text_buffer, draw_line);
}

static void draw_stuff(void)
{
#ifdef IMLIB2
	cimlib_render(text_start_x, text_start_y, window.width, window.height);
#endif /* IMLIB2 */
	if (overwrite_file) {
		overwrite_fpointer = fopen(overwrite_file, "w");
		if(!overwrite_fpointer)
			NORM_ERR("Can't overwrite '%s' anymore", overwrite_file);
	}
	if (append_file) {
		append_fpointer = fopen(append_file, "a");
		if(!append_fpointer)
			NORM_ERR("Can't append '%s' anymore", append_file);
	}
#ifdef X11
#ifdef HAVE_LUA
	llua_draw_pre_hook();
#endif /* HAVE_LUA */
	if (output_methods & TO_X) {
		selected_font = 0;
		if (draw_shades && !draw_outline) {
			text_start_x++;
			text_start_y++;
			set_foreground_color(default_bg_color);
			draw_mode = BG;
			draw_text();
			text_start_x--;
			text_start_y--;
		}

		if (draw_outline) {
			int i, j;
			selected_font = 0;

			for (i = -1; i < 2; i++) {
				for (j = -1; j < 2; j++) {
					if (i == 0 && j == 0) {
						continue;
					}
					text_start_x += i;
					text_start_y += j;
					set_foreground_color(default_out_color);
					draw_mode = OUTLINE;
					draw_text();
					text_start_x -= i;
					text_start_y -= j;
				}
			}
		}

		set_foreground_color(default_fg_color);
	}
#endif /* X11 */
	draw_mode = FG;
	draw_text();
#if defined(X11) && defined(HAVE_LUA)
	llua_draw_post_hook();
#endif /* X11 & HAVE_LUA */
#if defined(X11) && defined(HAVE_XDBE)
	if (output_methods & TO_X) {
		xdbe_swap_buffers();
	}
#endif /* X11 && HAVE_XDBE */
	if(overwrite_fpointer) {
		fclose(overwrite_fpointer);
		overwrite_fpointer = 0;
	}
	if(append_fpointer) {
		fclose(append_fpointer);
		append_fpointer = 0;
	}
}

#ifdef X11
static void clear_text(int exposures)
{
#ifdef HAVE_XDBE
	if (use_xdbe) {
		/* The swap action is XdbeBackground, which clears */
		return;
	} else
#endif
	if (display && window.window) { // make sure these are !null
		/* there is some extra space for borders and outlines */
		XClearArea(display, window.window, text_start_x - window.border_inner_margin - window.border_outer_margin - window.border_width,
			text_start_y - window.border_inner_margin - window.border_outer_margin - window.border_width,
			text_width + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2,
			text_height + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2, exposures ? True : 0);
	}
}
#endif /* X11 */

static int need_to_update;

/* update_text() generates new text and clears old text area */
static void update_text(void)
{
#ifdef IMLIB2
	cimlib_cleanup();
#endif /* IMLIB2 */
	generate_text();
#ifdef X11
	if (output_methods & TO_X)
		clear_text(1);
#endif /* X11 */
	need_to_update = 1;
#ifdef HAVE_LUA
	llua_update_info(&info, update_interval);
#endif /* HAVE_LUA */
}

#ifdef HAVE_SYS_INOTIFY_H
int inotify_fd = -1;
#endif

static void main_loop(void)
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

	last_update_time = 0.0;
	next_update_time = get_time();
	info.looped = 0;
	while (terminate == 0 && (total_run_times == 0 || info.looped < total_run_times)) {
#ifdef __NetBSD__
		update_interval = update_interval_old;
#else
		if(update_interval_bat != NOBATTERY && update_interval_bat != update_interval_old) {
			char buf[max_user_text];

			get_battery_short_status(buf, max_user_text, "BAT0");
			if(buf[0] == 'D') {
				update_interval = update_interval_bat;
			} else {
				update_interval = update_interval_old;
			}
		}
#endif
		info.looped++;

#ifdef SIGNAL_BLOCKING
		/* block signals.  we will inspect for pending signals later */
		if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
			CRIT_ERR(NULL, NULL, "unable to sigprocmask()");
		}
#endif

#ifdef X11
		if (output_methods & TO_X) {
			XFlush(display);

			/* wait for X event or timeout */

			if (!XPending(display)) {
				fd_set fdsr;
				struct timeval tv;
				int s;
				t = next_update_time - get_time();

				if (t < 0) {
					t = 0;
				} else if (t > update_interval) {
					t = update_interval;
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
						update_text();
					}
				}
			}

			if (need_to_update) {
#ifdef OWN_WINDOW
				int wx = window.x, wy = window.y;
#endif

				need_to_update = 0;
				selected_font = 0;
				update_text_area();
#ifdef OWN_WINDOW
				if (own_window) {
					int changed = 0;

					/* resize window if it isn't right size */
					if (!fixed_size
							&& (text_width + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2 != window.width
								|| text_height + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2 != window.height)) {
						window.width = text_width + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2;
						window.height = text_height + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2;
						draw_stuff(); /* redraw everything in our newly sized window */
						XResizeWindow(display, window.window, window.width,
								window.height); /* resize window */
						set_transparent_background(window.window, own_window_argb_value);
#ifdef HAVE_XDBE
						/* swap buffers */
						xdbe_swap_buffers();
#endif

						changed++;
#ifdef HAVE_LUA
						/* update lua window globals */
						llua_update_window_table(text_start_x, text_start_y, text_width, text_height);
#endif /* HAVE_LUA */
					}

					/* move window if it isn't in right position */
					if (!fixed_pos && (window.x != wx || window.y != wy)) {
						XMoveWindow(display, window.window, window.x, window.y);
						changed++;
					}

					/* update struts */
					if (changed && window.type == TYPE_PANEL) {
						int sidenum = -1;

						fprintf(stderr, PACKAGE_NAME": defining struts\n");
						fflush(stderr);

						switch (text_alignment) {
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

				clear_text(1);

#ifdef HAVE_XDBE
				if (use_xdbe) {
					XRectangle r;

					r.x = text_start_x - window.border_inner_margin - window.border_outer_margin - window.border_width;
					r.y = text_start_y - window.border_inner_margin - window.border_outer_margin - window.border_width;
					r.width = text_width + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2;
					r.height = text_height + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2;
					XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
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
						XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
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
						/* make background transparent */
						if (own_window) {
							set_transparent_background(window.window, own_window_argb_value);
						}
						break;

					case ConfigureNotify:
						if (own_window) {
							/* if window size isn't what expected, set fixed size */
							if (ev.xconfigure.width != window.width
									|| ev.xconfigure.height != window.height) {
								if (window.width != 0 && window.height != 0) {
									fixed_size = 1;
								}

								/* clear old stuff before screwing up
								 * size and pos */
								clear_text(1);

								{
									XWindowAttributes attrs;
									if (XGetWindowAttributes(display,
											window.window, &attrs)) {
										window.width = attrs.width;
										window.height = attrs.height;
									}
								}

								text_width = window.width - window.border_inner_margin * 2 - window.border_outer_margin * 2 - window.border_width * 2;
								text_height = window.height - window.border_inner_margin * 2 - window.border_outer_margin * 2 - window.border_width * 2;
								if (text_width > maximum_width
										&& maximum_width > 0) {
									text_width = maximum_width;
								}
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
						if (own_window) {
							/* if an ordinary window with decorations */
							if ((window.type == TYPE_NORMAL &&
										(!TEST_HINT(window.hints,
													HINT_UNDECORATED))) ||
									window.type == TYPE_DESKTOP) {
								/* allow conky to hold input focus. */
								break;
							} else {
								/* forward the click to the desktop window */
								XUngrabPointer(display, ev.xbutton.time);
								ev.xbutton.window = window.desktop;
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
						if (own_window) {
							/* if an ordinary window with decorations */
							if ((window.type == TYPE_NORMAL)
									&& (!TEST_HINT(window.hints,
									HINT_UNDECORATED))) {
								/* allow conky to hold input focus. */
								break;
							} else {
								/* forward the release to the desktop window */
								ev.xbutton.window = window.desktop;
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
						if (ev.type == x11_stuff.event_base + XDamageNotify) {
							XDamageNotifyEvent *dev = (XDamageNotifyEvent *) &ev;

							XFixesSetRegion(display, x11_stuff.part, &dev->area, 1);
							XFixesUnionRegion(display, x11_stuff.region2, x11_stuff.region2, x11_stuff.part);
						}
#endif /* HAVE_XDAMAGE */
						break;
				}
			}

#ifdef HAVE_XDAMAGE
			XDamageSubtract(display, x11_stuff.damage, x11_stuff.region2, None);
			XFixesSetRegion(display, x11_stuff.region2, 0, 0);
#endif /* HAVE_XDAMAGE */

			/* XDBE doesn't seem to provide a way to clear the back buffer
			 * without interfering with the front buffer, other than passing
			 * XdbeBackground to XdbeSwapBuffers. That means that if we're
			 * using XDBE, we need to redraw the text even if it wasn't part of
			 * the exposed area. OTOH, if we're not going to call draw_stuff at
			 * all, then no swap happens and we can safely do nothing. */

			if (!XEmptyRegion(x11_stuff.region)) {
#ifdef HAVE_XDBE
				if (use_xdbe) {
					XRectangle r;

					r.x = text_start_x - window.border_inner_margin - window.border_outer_margin - window.border_width;
					r.y = text_start_y - window.border_inner_margin - window.border_outer_margin - window.border_width;
					r.width = text_width + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2;
					r.height = text_height + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2;
					XUnionRectWithRegion(&r, x11_stuff.region, x11_stuff.region);
				}
#endif
				XSetRegion(display, window.gc, x11_stuff.region);
#ifdef XFT
				if (use_xft) {
					XftDrawSetClip(window.xftdraw, x11_stuff.region);
				}
#endif
				draw_stuff();
				XDestroyRegion(x11_stuff.region);
				x11_stuff.region = XCreateRegion();
			}
		} else {
#endif /* X11 */
			t = (next_update_time - get_time()) * 1000000;
			if(t > 0) usleep((useconds_t)t);
			update_text();
			draw_stuff();
#ifdef NCURSES
			if(output_methods & TO_NCURSES) {
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
				if (output_methods & TO_X) {
					XDestroyRegion(x11_stuff.region);
					x11_stuff.region = NULL;
#ifdef HAVE_XDAMAGE
					XDamageDestroy(display, x11_stuff.damage);
					XFixesDestroyRegion(display, x11_stuff.region2);
					XFixesDestroyRegion(display, x11_stuff.part);
#endif /* HAVE_XDAMAGE */
					if (disp) {
						free(disp);
					}
				}
#endif /* X11 */
				if(overwrite_file) {
					free(overwrite_file);
					overwrite_file = 0;
				}
				if(append_file) {
					free(append_file);
					append_file = 0;
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
		if (!disable_auto_reload && inotify_fd != -1 && inotify_config_wd == -1 && current_config != 0) {
			inotify_config_wd = inotify_add_watch(inotify_fd,
					current_config,
					IN_MODIFY);
		}
		if (!disable_auto_reload && inotify_fd != -1 && inotify_config_wd != -1 && current_config != 0) {
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
						/* current_config should be reloaded */
						NORM_ERR("'%s' modified, reloading...", current_config);
						reload_config();
						if (ev->mask & IN_IGNORED) {
							/* for some reason we get IN_IGNORED here
							 * sometimes, so we need to re-add the watch */
							inotify_config_wd = inotify_add_watch(inotify_fd,
									current_config,
									IN_MODIFY);
						}
						break;
					}
#ifdef HAVE_LUA
					else {
						llua_inotify_query(ev->wd, ev->mask);
					}
#endif /* HAVE_LUA */
					idx += INOTIFY_EVENT_SIZE + ev->len;
				}
			}
		} else if (disable_auto_reload && inotify_fd != -1) {
			inotify_rm_watch(inotify_fd, inotify_config_wd);
			close(inotify_fd);
			inotify_fd = inotify_config_wd = -1;
		}
#endif /* HAVE_SYS_INOTIFY_H */

#ifdef HAVE_LUA
		llua_update_info(&info, update_interval);
#endif /* HAVE_LUA */
		g_signal_pending = 0;
	}
	clean_up(current_mail_spool, NULL);

#ifdef HAVE_SYS_INOTIFY_H
	if (inotify_fd != -1) {
		inotify_rm_watch(inotify_fd, inotify_config_wd);
		close(inotify_fd);
		inotify_fd = inotify_config_wd = -1;
	}
#endif /* HAVE_SYS_INOTIFY_H */
}

#ifdef X11
static void load_config_file_x11(const char *);
#endif /* X11 */
void initialisation(int argc, char** argv);

	/* reload the config file */
static void reload_config(void)
{
	char *current_config_copy = strdup(current_config);
	clean_up(NULL, NULL);
	sleep(1); /* slight pause */
	current_config = current_config_copy;
	initialisation(argc_copy, argv_copy);
}

#ifdef X11
void clean_up_x11(void)
{
	if(window_created == 1) {
		XClearArea(display, window.window, text_start_x - window.border_inner_margin - window.border_outer_margin - window.border_width,
			text_start_y - window.border_inner_margin - window.border_outer_margin - window.border_width,
			text_width + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2,
			text_height + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2, 0);
	}
	destroy_window();
	free_fonts();
	fonts = NULL;
	if(x11_stuff.region) {
		XDestroyRegion(x11_stuff.region);
		x11_stuff.region = NULL;
	}
	if(display) {
		XCloseDisplay(display);
		display = NULL;
	}
	if(info.x11.desktop.all_names) {
		free(info.x11.desktop.all_names);
		info.x11.desktop.all_names = NULL;
	}
	if (info.x11.desktop.name) {
		free(info.x11.desktop.name);
		info.x11.desktop.name = NULL;
	}
	x_initialised = NO;
}
#endif

void clean_up_without_threads(void *memtofree1, void* memtofree2) {
	int i;

#ifdef NCURSES
	if(output_methods & TO_NCURSES) {
		endwin();
	}
#endif
	conftree_empty(currentconffile);
	currentconffile = NULL;
	if(memtofree1) {
		free(memtofree1);
	}
	if(memtofree2) {
		free(memtofree2);
	}
	timed_thread_destroy_registered_threads();

	if (info.cpu_usage) {
		free(info.cpu_usage);
		info.cpu_usage = NULL;
	}
#ifdef X11
	if (x_initialised == YES) {
		clean_up_x11();
	}else{
		free(fonts);	//in set_default_configurations a font is set but not loaded
		font_count = -1;
	}

#ifdef NVIDIA
	set_nvidia_display(NULL);
#endif
#endif /* X11 */

	free_templates();

	free_text_objects(&global_root_object, 0);
	if (tmpstring1) {
		free(tmpstring1);
		tmpstring1 = 0;
	}
	if (tmpstring2) {
		free(tmpstring2);
		tmpstring2 = 0;
	}
	if (text_buffer) {
		free(text_buffer);
		text_buffer = 0;
	}

	if (global_text) {
		free(global_text);
		global_text = 0;
	}

	free(current_config);
	current_config = 0;

#ifdef TCP_PORT_MONITOR
	tcp_portmon_clear();
#endif
#ifdef HAVE_CURL
	ccurl_free_info();
#endif
#ifdef RSS
	rss_free_info();
#endif
#ifdef WEATHER
	weather_free_info();
#endif
#ifdef HAVE_LUA
	llua_shutdown_hook();
	llua_close();
#endif /* HAVE_LUA */
#ifdef IMLIB2
	if (output_methods & TO_X)
		cimlib_deinit();
#endif /* IMLIB2 */
#ifdef XOAP
	xmlCleanupParser();
#endif /* XOAP */

	if (specials) {
		for (i = 0; i < special_count; i++) {
			if (specials[i].type == GRAPH) {
				free(specials[i].graph);
			}
		}
		free(specials);
		specials = NULL;
	}

	clear_net_stats();
	clear_diskio_stats();
	if(global_cpu != NULL) {
		free(global_cpu);
		global_cpu = NULL;
	}
}

void clean_up(void *memtofree1, void* memtofree2)
{
	free_update_callbacks();
	clean_up_without_threads(memtofree1, memtofree2);
}

static int string_to_bool(const char *s)
{
	if (!s) {
		// Assumes an option without a true/false means true
		return 1;
	} else if (strcasecmp(s, "yes") == EQUAL) {
		return 1;
	} else if (strcasecmp(s, "true") == EQUAL) {
		return 1;
	} else if (strcasecmp(s, "1") == EQUAL) {
		return 1;
	}
	return 0;
}

#ifdef X11
static enum alignment string_to_alignment(const char *s)
{
	if (strcasecmp(s, "top_left") == EQUAL) {
		return TOP_LEFT;
	} else if (strcasecmp(s, "top_right") == EQUAL) {
		return TOP_RIGHT;
	} else if (strcasecmp(s, "top_middle") == EQUAL) {
		return TOP_MIDDLE;
	} else if (strcasecmp(s, "bottom_left") == EQUAL) {
		return BOTTOM_LEFT;
	} else if (strcasecmp(s, "bottom_right") == EQUAL) {
		return BOTTOM_RIGHT;
	} else if (strcasecmp(s, "bottom_middle") == EQUAL) {
		return BOTTOM_MIDDLE;
	} else if (strcasecmp(s, "middle_left") == EQUAL) {
		return MIDDLE_LEFT;
	} else if (strcasecmp(s, "middle_right") == EQUAL) {
		return MIDDLE_RIGHT;
	} else if (strcasecmp(s, "middle_middle") == EQUAL) {
		return MIDDLE_MIDDLE;
	} else if (strcasecmp(s, "tl") == EQUAL) {
		return TOP_LEFT;
	} else if (strcasecmp(s, "tr") == EQUAL) {
		return TOP_RIGHT;
	} else if (strcasecmp(s, "tm") == EQUAL) {
		return TOP_MIDDLE;
	} else if (strcasecmp(s, "bl") == EQUAL) {
		return BOTTOM_LEFT;
	} else if (strcasecmp(s, "br") == EQUAL) {
		return BOTTOM_RIGHT;
	} else if (strcasecmp(s, "bm") == EQUAL) {
		return BOTTOM_MIDDLE;
	} else if (strcasecmp(s, "ml") == EQUAL) {
		return MIDDLE_LEFT;
	} else if (strcasecmp(s, "mr") == EQUAL) {
		return MIDDLE_RIGHT;
	} else if (strcasecmp(s, "mm") == EQUAL) {
		return MIDDLE_MIDDLE;
	} else if (strcasecmp(s, "none") == EQUAL) {
		return NONE;
	}
	return ALIGNMENT_ERROR;
}
#endif /* X11 */

#ifdef X11
static void set_default_configurations_for_x(void)
{
	default_fg_color = WhitePixel(display, screen);
	default_bg_color = BlackPixel(display, screen);
	default_out_color = BlackPixel(display, screen);
	color0 = default_fg_color;
	color1 = default_fg_color;
	color2 = default_fg_color;
	color3 = default_fg_color;
	color4 = default_fg_color;
	color5 = default_fg_color;
	color6 = default_fg_color;
	color7 = default_fg_color;
	color8 = default_fg_color;
	color9 = default_fg_color;
	current_text_color = default_fg_color;
}
#endif /* X11 */

static void set_default_configurations(void)
{
#ifdef MPD
	char *mpd_env_host;
	char *mpd_env_port;
#endif
	update_uname();
	fork_to_background = 0;
	total_run_times = 0;
	info.cpu_avg_samples = 2;
	info.net_avg_samples = 2;
	info.diskio_avg_samples = 2;
	info.memmax = 0;
	top_cpu = 0;
	cpu_separate = 0;
	short_units = 0;
	format_human_readable = 1;
	top_mem = 0;
	top_time = 0;
#ifdef IOSTATS
	top_io = 0;
#endif
	top_running = 0;
#ifdef MPD
	mpd_env_host = getenv("MPD_HOST");
	mpd_env_port = getenv("MPD_PORT");

	if (!mpd_env_host || !strlen(mpd_env_host)) {
		mpd_set_host("localhost");
	} else {
		/* MPD_HOST environment variable is set */
		char *mpd_hostpart = strchr(mpd_env_host, '@');
		if (!mpd_hostpart) {
			mpd_set_host(mpd_env_host);
		} else {
			/* MPD_HOST contains a password */
			char mpd_password[mpd_hostpart - mpd_env_host + 1];
			snprintf(mpd_password, mpd_hostpart - mpd_env_host + 1, "%s", mpd_env_host);

			if (!strlen(mpd_hostpart + 1)) {
				mpd_set_host("localhost");
			} else {
				mpd_set_host(mpd_hostpart + 1);
			}

			mpd_set_password(mpd_password, 1);
		}
	}


	if (!mpd_env_port || mpd_set_port(mpd_env_port)) {
		/* failed to set port from environment variable */
		mpd_set_port("6600");
	}
#endif
#ifdef XMMS2
	info.xmms2.artist = NULL;
	info.xmms2.album = NULL;
	info.xmms2.title = NULL;
	info.xmms2.genre = NULL;
	info.xmms2.comment = NULL;
	info.xmms2.url = NULL;
	info.xmms2.status = NULL;
	info.xmms2.playlist = NULL;
#endif
	use_spacer = NO_SPACER;
#ifdef X11
	output_methods = TO_X;
#else
	output_methods = TO_STDOUT;
#endif
#ifdef X11
#ifdef BUILD_XFT
	use_xft = 0;
#endif
	show_graph_scale = 0;
	show_graph_range = 0;
	draw_shades = 1;
	draw_borders = 0;
	draw_graph_borders = 1;
	draw_outline = 0;
	set_first_font("6x10");
	gap_x = 5;
	gap_y = 60;
	minimum_width = 5;
	minimum_height = 5;
	maximum_width = 0;
#ifdef OWN_WINDOW
	own_window = 0;
	window.type = TYPE_NORMAL;
	window.hints = 0;
	strcpy(window.class_name, PACKAGE_NAME);
	sprintf(window.title, PACKAGE_NAME" (%s)", info.uname_s.nodename);
#ifdef USE_ARGB
	use_argb_visual = 0;
	own_window_argb_value = 255;
#endif
#endif
	stippled_borders = 0;
	window.border_inner_margin = 3;
	window.border_outer_margin = 1;
	window.border_width = 1;
	text_alignment = BOTTOM_LEFT;
	info.x11.monitor.number = 1;
	info.x11.monitor.current = 0;
	info.x11.desktop.current = 1;
	info.x11.desktop.number = 1;
	info.x11.desktop.nitems = 0;
	info.x11.desktop.all_names = NULL;
	info.x11.desktop.name = NULL;
#endif /* X11 */

	free_templates();

	free(current_mail_spool);
	{
		char buf[256];

		variable_substitute(MAIL_FILE, buf, 256);
		if (buf[0] != '\0') {
			current_mail_spool = strndup(buf, text_buffer_size);
		}
	}

	no_buffers = 1;
	set_update_interval(3);
	update_interval_bat = NOBATTERY;
	info.music_player_interval = 1.0;
	stuff_in_uppercase = 0;
	info.users.number = 1;

	set_times_in_seconds(0);

#ifdef TCP_PORT_MONITOR
	/* set default connection limit */
	tcp_portmon_set_max_connections(0);
#endif
}

/* returns 1 if you can overwrite or create the file at 'path' */
static _Bool overwrite_works(const char *path)
{
	FILE *filepointer;

	if (!(filepointer = fopen(path, "w")))
		return 0;
	fclose(filepointer);
	return 1;
}

/* returns 1 if you can append or create the file at 'path' */
static _Bool append_works(const char *path)
{
	FILE *filepointer;

	if (!(filepointer = fopen(path, "a")))
		return 0;
	fclose(filepointer);
	return 1;
}

#ifdef X11
#ifdef DEBUG
/* WARNING, this type not in Xlib spec */
int x11_error_handler(Display *d, XErrorEvent *err)
	__attribute__((noreturn));
int x11_error_handler(Display *d, XErrorEvent *err)
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

int x11_ioerror_handler(Display *d)
	__attribute__((noreturn));
int x11_ioerror_handler(Display *d)
{
	NORM_ERR("X Error: Display %lx\n",
			(long unsigned)d
			);
	exit(1);
}
#endif /* DEBUG */

static void X11_initialisation(void)
{
	if (x_initialised == YES) return;
	output_methods |= TO_X;
	init_X11(disp);
	set_default_configurations_for_x();
	x_initialised = YES;
#ifdef DEBUG
	_Xdebug = 1;
	/* WARNING, this type not in Xlib spec */
	XSetErrorHandler(&x11_error_handler);
	XSetIOErrorHandler(&x11_ioerror_handler);
#endif /* DEBUG */
}

static char **xargv = 0;
static int xargc = 0;

static void X11_create_window(void)
{
	if (output_methods & TO_X) {
#ifdef OWN_WINDOW
		init_window(own_window, text_width + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2,
				text_height + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2, set_transparent, background_colour,
				xargv, xargc);
#else /* OWN_WINDOW */
		init_window(0, text_width + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2,
				text_height + window.border_inner_margin * 2 + window.border_outer_margin * 2 + window.border_width * 2, set_transparent, 0,
				xargv, xargc);
#endif /* OWN_WINDOW */

		setup_fonts();
		load_fonts();
		update_text_area();	/* to position text/window on screen */

#ifdef OWN_WINDOW
		if (own_window && !fixed_pos) {
			XMoveWindow(display, window.window, window.x, window.y);
		}
		if (own_window) {
			set_transparent_background(window.window, own_window_argb_value);
		}
#endif

		create_gc();

		draw_stuff();

		x11_stuff.region = XCreateRegion();
#ifdef HAVE_XDAMAGE
		if (!XDamageQueryExtension(display, &x11_stuff.event_base, &x11_stuff.error_base)) {
			NORM_ERR("Xdamage extension unavailable");
		}
		x11_stuff.damage = XDamageCreate(display, window.window, XDamageReportNonEmpty);
		x11_stuff.region2 = XFixesCreateRegionFromWindow(display, window.window, 0);
		x11_stuff.part = XFixesCreateRegionFromWindow(display, window.window, 0);
#endif /* HAVE_XDAMAGE */

		selected_font = 0;
		update_text_area();	/* to get initial size of the window */
	}
#ifdef HAVE_LUA
	/* setup lua window globals */
	llua_setup_window_table(text_start_x, text_start_y, text_width, text_height);
#endif /* HAVE_LUA */
}
#endif /* X11 */

#define CONF_ERR NORM_ERR("%s: %d: config file error", f, line)
#define CONF_ERR2(a) NORM_ERR("%s: %d: config file error: %s", f, line, a)
#define CONF2(a) if (strcasecmp(name, a) == 0)
#define CONF(a) else CONF2(a)
#define CONF3(a, b) else if (strcasecmp(name, a) == 0 \
		|| strcasecmp(name, b) == 0)
#define CONF_CONTINUE 1
#define CONF_BREAK 2
#define CONF_BUFF_SIZE 512

static FILE *open_config_file(const char *f)
{
#ifdef CONFIG_OUTPUT
	if (!strcmp(f, "==builtin==")) {
		return conf_cookie_open();
	} else
#endif /* CONFIG_OUTPUT */
		return fopen(f, "r");
}

static int do_config_step(int *line, FILE *fp, char *buf, char **name, char **value)
{
	char *p, *p2;
	(*line)++;
	if (fgets(buf, CONF_BUFF_SIZE, fp) == NULL) {
		return CONF_BREAK;
	}
	remove_comments(buf);

	p = buf;

	/* skip spaces */
	while (*p && isspace((int) *p)) {
		p++;
	}
	if (*p == '\0') {
		return CONF_CONTINUE;	/* empty line */
	}

	*name = p;

	/* skip name */
	p2 = p;
	while (*p2 && !isspace((int) *p2)) {
		p2++;
	}
	if (*p2 != '\0') {
		*p2 = '\0';	/* break at name's end */
		p2++;
	}

	/* get value */
	if (*p2) {
		p = p2;
		while (*p && isspace((int) *p)) {
			p++;
		}

		*value = p;

		p2 = *value + strlen(*value);
		while (isspace((int) *(p2 - 1))) {
			*--p2 = '\0';
		}
	} else {
		*value = 0;
	}
	return 0;
}

#ifdef X11
void setalignment(int* ltext_alignment, unsigned int windowtype, const char* value, const char *f, int line, char setbyconffile) {
#ifdef OWN_WINDOW
	if (windowtype == TYPE_DOCK) {
		NORM_ERR("alignment is disabled when own_window_type is dock");
	} else
#endif /*OWN_WINDOW */
	if (value) {
		int a = string_to_alignment(value);

		if (a <= 0) {
			if (setbyconffile) {
				CONF_ERR;
			} else NORM_ERR("'%s' is not a alignment setting", value);
		} else {
			*ltext_alignment = a;
		}
	} else if (setbyconffile) {
		CONF_ERR;
	}
}
#endif /* X11 */

char load_config_file(const char *f)
{
	int line = 0;
	FILE *fp;

	fp = open_config_file(f);
	if (!fp) {
		return FALSE;
	}
	DBGP("reading contents from config file '%s'", f);

	while (!feof(fp)) {
		char buff[CONF_BUFF_SIZE], *name, *value;
		int ret = do_config_step(&line, fp, buff, &name, &value);
		if (ret == CONF_BREAK) {
			break;
		} else if (ret == CONF_CONTINUE) {
			continue;
		}

#ifdef X11
		CONF2("out_to_x") {
			if (string_to_bool(value)) {
				output_methods &= TO_X;
			} else {
				clean_up_x11();
				output_methods &= ~TO_X;
				x_initialised = NEVER;
			}
		}
		CONF("display") {
			if (!value || x_initialised == YES) {
				CONF_ERR;
			} else {
				if (disp)
					free(disp);
				disp = strdup(value);
			}
		}
#ifdef NVIDIA
		CONF("nvidia_display") {
			if(value)
				set_nvidia_display(value);
		}
#endif
		CONF("alignment") {
			setalignment(&text_alignment, window.type, value, f, line, 1);
		}
		CONF("background") {
			fork_to_background = string_to_bool(value);
		}
#else
		CONF2("background") {
			fork_to_background = string_to_bool(value);
		}
#endif /* X11 */
#ifdef X11
		CONF("show_graph_scale") {
			show_graph_scale = string_to_bool(value);
		}
		CONF("show_graph_range") {
			show_graph_range = string_to_bool(value);
		}
		CONF("border_inner_margin") {
			if (value) {
				window.border_inner_margin = strtol(value, 0, 0);
				if (window.border_inner_margin < 0) window.border_inner_margin = 0;
			} else {
				CONF_ERR;
			}
		}
		CONF("border_outer_margin") {
			if (value) {
				window.border_outer_margin = strtol(value, 0, 0);
				if (window.border_outer_margin < 0) window.border_outer_margin = 0;
			} else {
				CONF_ERR;
			}
		}
		CONF("border_width") {
			if (value) {
				window.border_width = strtol(value, 0, 0);
				if (window.border_width < 1) window.border_width = 1;
			} else {
				CONF_ERR;
			}
		}
#endif /* X11 */
#define TEMPLATE_CONF(n) \
		CONF("template"#n) { \
			if (set_template(n, value)) \
				CONF_ERR; \
		}
		TEMPLATE_CONF(0)
		TEMPLATE_CONF(1)
		TEMPLATE_CONF(2)
		TEMPLATE_CONF(3)
		TEMPLATE_CONF(4)
		TEMPLATE_CONF(5)
		TEMPLATE_CONF(6)
		TEMPLATE_CONF(7)
		TEMPLATE_CONF(8)
		TEMPLATE_CONF(9)
		CONF("imap") {
			if (value) {
				parse_global_imap_mail_args(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("pop3") {
			if (value) {
				parse_global_pop3_mail_args(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("default_bar_size") {
			char err = 0;
			if (value) {
				if (sscanf(value, "%d %d", &default_bar_width, &default_bar_height) != 2) {
					err = 1;
				}
			} else {
				err = 1;
			}
			if (err) {
				CONF_ERR2("default_bar_size takes 2 integer arguments (ie. 'default_bar_size 0 6')")
			}
		}
#ifdef X11
		CONF("default_graph_size") {
			char err = 0;
			if (value) {
				if (sscanf(value, "%d %d", &default_graph_width, &default_graph_height) != 2) {
					err = 1;
				}
			} else {
				err = 1;
			}
			if (err) {
				CONF_ERR2("default_graph_size takes 2 integer arguments (ie. 'default_graph_size 0 6')")
			}
		}
		CONF("default_gauge_size") {
			char err = 0;
			if (value) {
				if (sscanf(value, "%d %d", &default_gauge_width, &default_gauge_height) != 2) {
					err = 1;
				}
			} else {
				err = 1;
			}
			if (err) {
				CONF_ERR2("default_gauge_size takes 2 integer arguments (ie. 'default_gauge_size 0 6')")
			}
		}
#endif
#ifdef MPD
		CONF("mpd_host") {
			if (value) {
				mpd_set_host(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("mpd_port") {
			if (value && mpd_set_port(value)) {
				CONF_ERR;
			}
		}
		CONF("mpd_password") {
			if (value) {
				mpd_set_password(value, 0);
			} else {
				CONF_ERR;
			}
		}
#endif
		CONF("music_player_interval") {
			if (value) {
				info.music_player_interval = strtod(value, 0);
			} else {
				CONF_ERR;
			}
		}
#ifdef __OpenBSD__
		CONF("sensor_device") {
			if (value) {
				sensor_device = strtol(value, 0, 0);
			} else {
				CONF_ERR;
			}
		}
#endif
		CONF("cpu_avg_samples") {
			if (value) {
				cpu_avg_samples = strtol(value, 0, 0);
				if (cpu_avg_samples < 1 || cpu_avg_samples > 14) {
					CONF_ERR;
				} else {
					info.cpu_avg_samples = cpu_avg_samples;
				}
			} else {
				CONF_ERR;
			}
		}
		CONF("net_avg_samples") {
			if (value) {
				net_avg_samples = strtol(value, 0, 0);
				if (net_avg_samples < 1 || net_avg_samples > 14) {
					CONF_ERR;
				} else {
					info.net_avg_samples = net_avg_samples;
				}
			} else {
				CONF_ERR;
			}
		}
		CONF("diskio_avg_samples") {
			if (value) {
				diskio_avg_samples = strtol(value, 0, 0);
				if (diskio_avg_samples < 1 || diskio_avg_samples > 14) {
					CONF_ERR;
				} else {
					info.diskio_avg_samples = diskio_avg_samples;
				}
			} else {
				CONF_ERR;
			}
		}

#ifdef HAVE_XDBE
		CONF("double_buffer") {
			use_xdbe = string_to_bool(value);
		}
#endif
#ifdef X11
		CONF("override_utf8_locale") {
			utf8_mode = string_to_bool(value);
		}
		CONF("draw_borders") {
			draw_borders = string_to_bool(value);
		}
		CONF("draw_graph_borders") {
			draw_graph_borders = string_to_bool(value);
		}
		CONF("draw_shades") {
			draw_shades = string_to_bool(value);
		}
		CONF("draw_outline") {
			draw_outline = string_to_bool(value);
		}
#endif /* X11 */
		CONF("times_in_seconds") {
			set_times_in_seconds(string_to_bool(value));
		}
		CONF("max_text_width") {
			max_text_width = atoi(value);
		}
		CONF("out_to_console") {
			if(string_to_bool(value)) {
				output_methods |= TO_STDOUT;
			} else {
				output_methods &= ~TO_STDOUT;
			}
		}
		CONF("extra_newline") {
			extra_newline = string_to_bool(value);
		}
		CONF("disable_auto_reload") {
			disable_auto_reload = string_to_bool(value);
		}
		CONF("out_to_stderr") {
			if(string_to_bool(value))
				output_methods |= TO_STDERR;
		}
#ifdef NCURSES
		CONF("out_to_ncurses") {
			if(string_to_bool(value)) {
				initscr();
				start_color();
				output_methods |= TO_NCURSES;
			}
		}
#endif
		CONF("overwrite_file") {
			if(overwrite_file) {
				free(overwrite_file);
				overwrite_file = 0;
			}
			if(overwrite_works(value)) {
				overwrite_file = strdup(value);
				output_methods |= OVERWRITE_FILE;
			} else
				NORM_ERR("overwrite_file won't be able to create/overwrite '%s'", value);
		}
		CONF("append_file") {
			if(append_file) {
				free(append_file);
				append_file = 0;
			}
			if(append_works(value)) {
				append_file = strdup(value);
				output_methods |= APPEND_FILE;
			} else
				NORM_ERR("append_file won't be able to create/append '%s'", value);
		}
		CONF("use_spacer") {
			if (value) {
				if (strcasecmp(value, "left") == EQUAL) {
					use_spacer = LEFT_SPACER;
				} else if (strcasecmp(value, "right") == EQUAL) {
					use_spacer = RIGHT_SPACER;
				} else if (strcasecmp(value, "none") == EQUAL) {
					use_spacer = NO_SPACER;
				} else {
					use_spacer = string_to_bool(value);
					NORM_ERR("use_spacer should have an argument of left, right, or"
						" none.  '%s' seems to be some form of '%s', so"
						" defaulting to %s.", value,
						use_spacer ? "true" : "false",
						use_spacer ? "right" : "none");
					if (use_spacer) {
						use_spacer = RIGHT_SPACER;
					} else {
						use_spacer = NO_SPACER;
					}
				}
			} else {
				NORM_ERR("use_spacer should have an argument. Defaulting to right.");
				use_spacer = RIGHT_SPACER;
			}
		}
#ifdef X11
#ifdef XFT
		CONF("use_xft") {
			use_xft = string_to_bool(value);
		}
		CONF("font") {
			if (value) {
				set_first_font(value);
			}
		}
		CONF("xftalpha") {
			if (value && font_count >= 0) {
				fonts[0].font_alpha = atof(value) * 65535.0;
			}
		}
		CONF("xftfont") {
			if (use_xft) {
#else
		CONF("use_xft") {
			if (string_to_bool(value)) {
				NORM_ERR("Xft not enabled at compile time");
			}
		}
		CONF("xftfont") {
			/* xftfont silently ignored when no Xft */
		}
		CONF("xftalpha") {
			/* xftalpha is silently ignored when no Xft */
		}
		CONF("font") {
#endif
			if (value) {
				set_first_font(value);
			}
#ifdef XFT
			}
#endif
		}
		CONF("gap_x") {
			if (value) {
				gap_x = atoi(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("gap_y") {
			if (value) {
				gap_y = atoi(value);
			} else {
				CONF_ERR;
			}
		}
#endif /* X11 */
		CONF("mail_spool") {
			if (value) {
				char buffer[256];

				variable_substitute(value, buffer, 256);

				if (buffer[0] != '\0') {
					if (current_mail_spool) {
						free(current_mail_spool);
					}
					current_mail_spool = strndup(buffer, text_buffer_size);
				}
			} else {
				CONF_ERR;
			}
		}
#ifdef X11
		CONF("minimum_size") {
			if (value) {
				if (sscanf(value, "%d %d", &minimum_width, &minimum_height)
						!= 2) {
					if (sscanf(value, "%d", &minimum_width) != 1) {
						CONF_ERR;
					}
				}
			} else {
				CONF_ERR;
			}
		}
		CONF("maximum_width") {
			if (value) {
				if (sscanf(value, "%d", &maximum_width) != 1) {
					CONF_ERR;
				}
			} else {
				CONF_ERR;
			}
		}
#endif /* X11 */
		CONF("no_buffers") {
			no_buffers = string_to_bool(value);
		}
		CONF("top_name_width") {
			if (set_top_name_width(value))
				CONF_ERR;
		}
		CONF("top_cpu_separate") {
			cpu_separate = string_to_bool(value);
		}
		CONF("short_units") {
			short_units = string_to_bool(value);
		}
		CONF("format_human_readable") {
			format_human_readable = string_to_bool(value);
		}
#ifdef HDDTEMP
		CONF("hddtemp_host") {
			set_hddtemp_host(value);
		}
		CONF("hddtemp_port") {
			set_hddtemp_port(value);
		}
#endif /* HDDTEMP */
		CONF("pad_percents") {
			pad_percents = atoi(value);
		}
#ifdef X11
#ifdef OWN_WINDOW
		CONF("own_window") {
			if (value) {
				own_window = string_to_bool(value);
			}
		}
		CONF("own_window_class") {
			if (value) {
				memset(window.class_name, 0, sizeof(window.class_name));
				strncpy(window.class_name, value,
						sizeof(window.class_name) - 1);
			}
		}
		CONF("own_window_title") {
			if (value) {
				memset(window.title, 0, sizeof(window.title));
				strncpy(window.title, value, sizeof(window.title) - 1);
			}
		}
		CONF("own_window_transparent") {
			if (value) {
				set_transparent = string_to_bool(value);
			}
		}
		CONF("own_window_hints") {
			if (value) {
				char *p_hint, *p_save;
				char delim[] = ", ";

				/* tokenize the value into individual hints */
				if ((p_hint = strtok_r(value, delim, &p_save)) != NULL) {
					do {
						/* fprintf(stderr, "hint [%s] parsed\n", p_hint); */
						if (strncmp(p_hint, "undecorate", 10) == EQUAL) {
							SET_HINT(window.hints, HINT_UNDECORATED);
						} else if (strncmp(p_hint, "below", 5) == EQUAL) {
							SET_HINT(window.hints, HINT_BELOW);
						} else if (strncmp(p_hint, "above", 5) == EQUAL) {
							SET_HINT(window.hints, HINT_ABOVE);
						} else if (strncmp(p_hint, "sticky", 6) == EQUAL) {
							SET_HINT(window.hints, HINT_STICKY);
						} else if (strncmp(p_hint, "skip_taskbar", 12) == EQUAL) {
							SET_HINT(window.hints, HINT_SKIP_TASKBAR);
						} else if (strncmp(p_hint, "skip_pager", 10) == EQUAL) {
							SET_HINT(window.hints, HINT_SKIP_PAGER);
						} else {
							CONF_ERR;
						}

						p_hint = strtok_r(NULL, delim, &p_save);
					} while (p_hint != NULL);
				}
			} else {
				CONF_ERR;
			}
		}
		CONF("own_window_type") {
			if (value) {
				if (strncmp(value, "normal", 6) == EQUAL) {
					window.type = TYPE_NORMAL;
				} else if (strncmp(value, "desktop", 7) == EQUAL) {
					window.type = TYPE_DESKTOP;
				} else if (strncmp(value, "dock", 4) == EQUAL) {
					window.type = TYPE_DOCK;
					text_alignment = TOP_LEFT;
				} else if (strncmp(value, "panel", 5) == EQUAL) {
					window.type = TYPE_PANEL;
				} else if (strncmp(value, "override", 8) == EQUAL) {
					window.type = TYPE_OVERRIDE;
				} else {
					CONF_ERR;
				}
			} else {
				CONF_ERR;
			}
		}
#ifdef USE_ARGB
		CONF("own_window_argb_visual") {
			use_argb_visual = string_to_bool(value);
		}
		CONF("own_window_argb_value") {
			own_window_argb_value = strtol(value, 0, 0);
			if (own_window_argb_value > 255 || own_window_argb_value < 0) {
				CONF_ERR2("own_window_argb_value must be <= 255 and >= 0");
			}
		}
#endif /* USE_ARGB */
#endif
		CONF("stippled_borders") {
			if (value) {
				stippled_borders = strtol(value, 0, 0);
			} else {
				stippled_borders = 4;
			}
		}
#ifdef IMLIB2
		CONF("imlib_cache_size") {
			if (value) {
				cimlib_set_cache_size(atoi(value));
			}
		}
		CONF("imlib_cache_flush_interval") {
			if (value) {
				cimlib_set_cache_flush_interval(atoi(value));
			}
		}
#endif /* IMLIB2 */
#endif /* X11 */
		CONF("update_interval_on_battery") {
			if (value) {
				update_interval_bat = strtod(value, 0);
			} else {
				CONF_ERR;
			}
		}
		CONF("update_interval") {
			if (value) {
				set_update_interval(strtod(value, 0));
			} else {
				CONF_ERR;
			}
			if (info.music_player_interval == 0) {
				// default to update_interval
				info.music_player_interval = update_interval;
			}
		}
		CONF("total_run_times") {
			if (value) {
				total_run_times = strtod(value, 0);
			} else {
				CONF_ERR;
			}
		}
		CONF("uppercase") {
			stuff_in_uppercase = string_to_bool(value);
		}
		CONF("max_specials") {
			if (value) {
				max_specials = atoi(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("max_user_text") {
			if (value) {
				max_user_text = atoi(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("text_buffer_size") {
			if (value) {
				text_buffer_size = atoi(value);
				if (text_buffer_size < DEFAULT_TEXT_BUFFER_SIZE) {
					NORM_ERR("text_buffer_size must be >=%i bytes", DEFAULT_TEXT_BUFFER_SIZE);
					text_buffer_size = DEFAULT_TEXT_BUFFER_SIZE;
				}
			} else {
				CONF_ERR;
			}
		}
		CONF("text") {
#ifdef X11
			if (output_methods & TO_X) {
				X11_initialisation();
			}
#endif

			if (global_text) {
				free(global_text);
				global_text = 0;
			}

			global_text = (char *) malloc(1);
			global_text[0] = '\0';

			while (!feof(fp)) {
				unsigned int l = strlen(global_text);
				unsigned int bl;
				char buf[CONF_BUFF_SIZE];

				if (fgets(buf, CONF_BUFF_SIZE, fp) == NULL) {
					break;
				}

				/* Remove \\-\n. */
				bl = strlen(buf);
				if (bl >= 2 && buf[bl-2] == '\\' && buf[bl-1] == '\n') {
					buf[bl-2] = '\0';
					bl -= 2;
					if (bl == 0) {
						continue;
					}
				}

				/* Check for continuation of \\-\n. */
				if (l > 0 && buf[0] == '\n' && global_text[l-1] == '\\') {
					global_text[l-1] = '\0';
					continue;
				}

				global_text = (char *) realloc(global_text, l + bl + 1);
				strcat(global_text, buf);

				if (strlen(global_text) > max_user_text) {
					break;
				}
			}
			global_text_lines = line + 1;
			break;
		}
#ifdef TCP_PORT_MONITOR
		CONF("max_port_monitor_connections") {
			int max;
			if (!value || (sscanf(value, "%d", &max) != 1)) {
				/* an error. use default, warn and continue. */
				tcp_portmon_set_max_connections(0);
				CONF_ERR;
			} else if (tcp_portmon_set_max_connections(max)) {
				/* max is < 0, default has been set*/
				CONF_ERR;
			}
		}
#endif
		CONF("if_up_strictness") {
			if (!value) {
				NORM_ERR("incorrect if_up_strictness value, defaulting to 'up'");
				ifup_strictness = IFUP_UP;
			} else if (strcasecmp(value, "up") == EQUAL) {
				ifup_strictness = IFUP_UP;
			} else if (strcasecmp(value, "link") == EQUAL) {
				ifup_strictness = IFUP_LINK;
			} else if (strcasecmp(value, "address") == EQUAL) {
				ifup_strictness = IFUP_ADDR;
			} else {
				NORM_ERR("incorrect if_up_strictness value, defaulting to 'up'");
				ifup_strictness = IFUP_UP;
			}
		}

		CONF("temperature_unit") {
			if (!value) {
				NORM_ERR("config option 'temperature_unit' needs an argument, either 'celsius' or 'fahrenheit'");
			} else if (set_temp_output_unit(value)) {
				NORM_ERR("temperature_unit: incorrect argument");
			}
		}

#ifdef HAVE_LUA
		CONF("lua_load") {
			if (value) {
				char *ptr = strtok(value, " ");
				while (ptr) {
					llua_load(ptr);
					ptr = strtok(NULL, " ");
				}
			} else {
				CONF_ERR;
			}
		}
#ifdef X11
		CONF("lua_draw_hook_pre") {
			if (value) {
				llua_set_draw_pre_hook(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("lua_draw_hook_post") {
			if (value) {
				llua_set_draw_post_hook(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("lua_startup_hook") {
			if (value) {
				llua_set_startup_hook(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("lua_shutdown_hook") {
			if (value) {
				llua_set_shutdown_hook(value);
			} else {
				CONF_ERR;
			}
		}
#endif /* X11 */
#endif /* HAVE_LUA */

		CONF("color0"){}
		CONF("color1"){}
		CONF("color2"){}
		CONF("color3"){}
		CONF("color4"){}
		CONF("color5"){}
		CONF("color6"){}
		CONF("color7"){}
		CONF("color8"){}
		CONF("color9"){}
		CONF("default_color"){}
		CONF3("default_shade_color", "default_shadecolor"){}
		CONF3("default_outline_color", "default_outlinecolor") {}
		CONF("own_window_colour") {}

		else {
			NORM_ERR("%s: %d: no such configuration: '%s'", f, line, name);
		}
	}

	fclose(fp);

	if (info.music_player_interval == 0) {
		// default to update_interval
		info.music_player_interval = update_interval;
	}
	if (!global_text) { // didn't supply any text
		CRIT_ERR(NULL, NULL, "missing text block in configuration; exiting");
	}
	if (!output_methods) {
		CRIT_ERR(0, 0, "no output_methods have been selected; exiting");
	}
#if defined(NCURSES)
#if defined(X11)
	if ((output_methods & TO_X) && (output_methods & TO_NCURSES)) {
		NORM_ERR("out_to_x and out_to_ncurses are incompatible, turning out_to_ncurses off");
		output_methods &= ~TO_NCURSES;
		endwin();
	}
#endif /* X11 */
	if ((output_methods & (TO_STDOUT | TO_STDERR)) && (output_methods & TO_NCURSES)) {
		NORM_ERR("out_to_ncurses conflicts with out_to_console and out_to_stderr, disabling the later ones");
		output_methods &= ~(TO_STDOUT | TO_STDERR);
	}
#endif /* NCURSES */
	return TRUE;
}

#ifdef X11
static void load_config_file_x11(const char *f)
{
	int line = 0;
	FILE *fp;

	fp = open_config_file(f);
	if (!fp) {
		return;
	}
	DBGP("reading contents from config file '%s'", f);

	while (!feof(fp)) {
		char buff[CONF_BUFF_SIZE], *name, *value;
		int ret = do_config_step(&line, fp, buff, &name, &value);
		if (ret == CONF_BREAK) {
			break;
		} else if (ret == CONF_CONTINUE) {
			continue;
		}

		CONF2("color0") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					color0 = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF("color1") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					color1 = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF("color2") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					color2 = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF("color3") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					color3 = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF("color4") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					color4 = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF("color5") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					color5 = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF("color6") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					color6 = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF("color7") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					color7 = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF("color8") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					color8 = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF("color9") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					color9 = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF("default_color") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					default_fg_color = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF3("default_shade_color", "default_shadecolor") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					default_bg_color = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
		CONF3("default_outline_color", "default_outlinecolor") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					default_out_color = get_x11_color(value);
				} else {
					CONF_ERR;
				}
			}
		}
#ifdef OWN_WINDOW
		CONF("own_window_colour") {
			X11_initialisation();
			if (x_initialised == YES) {
				if (value) {
					background_colour = get_x11_color(value);
				} else {
					NORM_ERR("Invalid colour for own_window_colour (try omitting the "
						"'#' for hex colours");
				}
			}
		}
#endif
		CONF("text") {
			/* initialize X11 if nothing X11-related is mentioned before TEXT (and if X11 is the default outputmethod) */
			if(output_methods & TO_X) {
				X11_initialisation();
			}
		}
#undef CONF
#undef CONF2
#undef CONF3
#undef CONF_ERR
#undef CONF_ERR2
#undef CONF_BREAK
#undef CONF_CONTINUE
#undef CONF_BUFF_SIZE
	}

	fclose(fp);

}
#endif /* X11 */

static void print_help(const char *prog_name) {
	printf("Usage: %s [OPTION]...\n"
			PACKAGE_NAME" is a system monitor that renders text on desktop or to own transparent\n"
			"window. Command line options will override configurations defined in config\n"
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
			"   -o, --own-window          create own window to draw\n"
#endif
#ifdef HAVE_XDBE
			"   -b, --double-buffer       double buffer (prevents flickering)\n"
#endif
			"   -w, --window-id=WIN_ID    window id to draw\n"
			"   -x X                      x position\n"
			"   -y Y                      y position\n"
#endif /* X11 */
			"   -t, --text=TEXT           text to render, remember single quotes, like -t '$uptime'\n"
			"   -u, --interval=SECS       update interval\n"
			"   -i COUNT                  number of times to update "PACKAGE_NAME" (and quit)\n"
			"   -p, --pause=SECS          pause for SECS seconds at startup before doing anything\n",
			prog_name
	);
}

/* : means that character before that takes an argument */
static const char *getopt_string = "vVqdDt:u:i:hc:p:"
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
	{ "quiet", 0, NULL, 'q' },
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
	{ "own-window", 0, NULL, 'o' },
#endif
#ifdef HAVE_XDBE
	{ "double-buffer", 0, NULL, 'b' },
#endif
	{ "window-id", 1, NULL, 'w' },
#endif /* X11 */
	{ "text", 1, NULL, 't' },
	{ "interval", 1, NULL, 'u' },
	{ "pause", 1, NULL, 'p' },
	{ 0, 0, 0, 0 }
};

void set_current_config(void);
void set_current_config(void)
{
	/* check if specified config file is valid */
	if (current_config) {
		struct stat sb;
		if (stat(current_config, &sb) ||
				(!S_ISREG(sb.st_mode) && !S_ISLNK(sb.st_mode))) {
			NORM_ERR("invalid configuration file '%s'\n", current_config);
			free(current_config);
			current_config = 0;
		}
	}

	/* load current_config, CONFIG_FILE or SYSTEM_CONFIG_FILE */

	if (!current_config) {
		/* load default config file */
		char buf[DEFAULT_TEXT_BUFFER_SIZE];
		FILE *fp;

		/* Try to use personal config file first */
		to_real_path(buf, CONFIG_FILE);
		if (buf[0] && (fp = fopen(buf, "r"))) {
			current_config = strndup(buf, max_user_text);
			fclose(fp);
		}

		/* Try to use system config file if personal config not readable */
		if (!current_config && (fp = fopen(SYSTEM_CONFIG_FILE, "r"))) {
			current_config = strndup(SYSTEM_CONFIG_FILE, max_user_text);
			fclose(fp);
		}

		/* No readable config found */
		if (!current_config) {
#define NOCFGFILEFOUND "no readable personal or system-wide config file found"
#ifdef BUILD_BUILTIN_CONFIG
			current_config = strdup("==builtin==");
			NORM_ERR(NOCFGFILEFOUND
					", using builtin default");
#else
			CRIT_ERR(NULL, NULL, NOCFGFILEFOUND);
#endif /* ! CONF_OUTPUT */
		}
	}
}

void initialisation(int argc, char **argv) {
	struct sigaction act, oact;

	set_default_configurations();
	set_current_config();
	load_config_file(current_config);
	currentconffile = conftree_add(currentconffile, current_config);

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
	pthread_mutex_init(&kvm_proc_mutex, NULL);
#endif
#if defined(__NetBSD__)
	if ((kd = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, NULL)) == NULL)
		CRIT_ERR(NULL, NULL, "cannot read kvm");
#endif

	while (1) {
		int c = getopt_long(argc, argv, getopt_string, longopts, NULL);
		int startup_pause;

		if (c == -1) {
			break;
		}

		switch (c) {
			case 'd':
				fork_to_background = 1;
				break;
			case 'D':
				global_debug_level++;
				break;
#ifdef X11
			case 'f':
				set_first_font(optarg);
				break;
			case 'a':
				setalignment(&text_alignment, window.type, optarg, NULL, 0, 0);
				break;

#ifdef OWN_WINDOW
			case 'o':
				own_window = 1;
				break;
#endif
#ifdef HAVE_XDBE
			case 'b':
				use_xdbe = 1;
				break;
#endif
#endif /* X11 */
			case 't':
				if (global_text) {
					free(global_text);
					global_text = 0;
				}
				global_text = strndup(optarg, max_user_text);
				convert_escapes(global_text);
				break;

			case 'u':
				update_interval = strtod(optarg, 0);
				update_interval_old = update_interval;
				if (info.music_player_interval == 0) {
					// default to update_interval
					info.music_player_interval = update_interval;
				}
				break;

			case 'i':
				total_run_times = strtod(optarg, 0);
				break;
#ifdef X11
			case 'x':
				gap_x = atoi(optarg);
				break;

			case 'y':
				gap_y = atoi(optarg);
				break;
#endif /* X11 */
			case 'p':
				if (first_pass) {
					startup_pause = atoi(optarg);
					sleep(startup_pause);
				}
				break;

			case '?':
				exit(EXIT_FAILURE);
		}
	}

#ifdef X11
	/* load font */
	if (output_methods & TO_X) {
		load_config_file_x11(current_config);
	}
#endif /* X11 */

	/* generate text and get initial size */
	extract_variable_text(global_text);
	if (global_text) {
		free(global_text);
		global_text = 0;
	}
	global_text = NULL;
	/* fork */
	if (fork_to_background && first_pass) {
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

	start_update_threading();

	text_buffer = malloc(max_user_text);
	memset(text_buffer, 0, max_user_text);
	tmpstring1 = malloc(text_buffer_size);
	memset(tmpstring1, 0, text_buffer_size);
	tmpstring2 = malloc(text_buffer_size);
	memset(tmpstring2, 0, text_buffer_size);

#ifdef X11
	xargc = argc;
	xargv = argv;
	X11_create_window();
#endif /* X11 */
#ifdef HAVE_LUA
	llua_setup_info(&info, update_interval);
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
#ifdef X11
	char *s, *temp;
	unsigned int x;
#endif

	argc_copy = argc;
	argv_copy = argv;
	g_signal_pending = 0;
	max_user_text = MAX_USER_TEXT_DEFAULT;
	current_config = 0;
	memset(&info, 0, sizeof(info));
	free_templates();
	clear_net_stats();

#ifdef TCP_PORT_MONITOR
	/* set default connection limit */
	tcp_portmon_set_max_connections(0);
#endif

#ifdef HAVE_CURL
	if(curl_global_init(CURL_GLOBAL_ALL))
		NORM_ERR("curl_global_init() failed, you may not be able to use curl variables");
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
			utf8_mode = 1;
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
				if (current_config) {
					free(current_config);
				}
				current_config = strndup(optarg, max_user_text);
				break;
			case 'q':
				if (!freopen("/dev/null", "w", stderr)) {
					NORM_ERR("unable to redirect stderr to /dev/null");
				}
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
				window.window = strtol(optarg, 0, 0);
				break;
			case 'X':
				if (disp)
					free(disp);
				disp = strdup(optarg);
				break;
#endif /* X11 */

			case '?':
				exit(EXIT_FAILURE);
		}
	}

	set_current_config();

#ifdef XOAP
	/* Load xoap keys, if existing */
	load_xoap_keys();
#endif /* XOAP */

#ifdef HAVE_SYS_INOTIFY_H
	inotify_fd = inotify_init();
	if(inotify_fd != -1) {
		int fl;

		fl = fcntl(inotify_fd, F_GETFL);
		fcntl(inotify_fd, F_SETFL, fl | O_NONBLOCK);
	}
#endif /* HAVE_SYS_INOTIFY_H */

	initialisation(argc, argv);

	first_pass = 0; /* don't ever call fork() again */

	main_loop();

#ifdef HAVE_CURL
	curl_global_cleanup();
#endif

#ifdef JACK
	jack_close();
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__NetBSD__)
	kvm_close(kd);
#ifndef __NetBSD__
	pthread_mutex_destroy(&kvm_proc_mutex);
#endif
#endif

	return 0;

}

void alarm_handler(void) {
	if(childpid > 0) {
		kill(childpid, SIGTERM);
	}
}

static void signal_handler(int sig)
{
	/* signal handler is light as a feather, as it should be.
	 * we will poll g_signal_pending with each loop of conky
	 * and do any signal processing there, NOT here (except
	 * SIGALRM because this is caused when conky is hanging) */
	if(sig == SIGALRM) {
		alarm_handler();
	} else {
		g_signal_pending = sig;
	}
}
