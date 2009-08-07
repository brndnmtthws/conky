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

#ifndef _CONKY_STRUCTS_H_
#define _CONKY_STRUCTS_H_

#include "config.h"	/* defines */

#include <sys/utsname.h> /* struct uname_s */
#include <stdio.h> /* FILE */

#ifdef X11
#include "x11.h"
#endif /* X11 */

#ifdef APCUPSD
#include "apcupsd.h"
#endif

#define MAX_TEMPLATES 10

struct entropy_s {
	unsigned int entropy_avail;
	unsigned int poolsize;
};

struct usr_info {
	char *names;
	char *times;
	char *terms;
	int number;
};

struct gateway_info {
	char *iface;
	char *ip;
	int count;
};

#ifdef X11
struct monitor_info {
	int number;
	int current;
};

struct desktop_info {
	int current;
	int number;
	unsigned int nitems;
	char *all_names;
	char *name;
};

struct x11_info {
	struct monitor_info monitor;
	struct desktop_info desktop;
};

#endif /* X11 */

struct dns_data {
	int nscount;
	char **ns_list;
};

struct conftree {
	char* string;
	struct conftree* horz_next;
	struct conftree* vert_next;
	struct conftree* back;
};

struct information {
	unsigned int mask;

	struct utsname uname_s;

	char freq[10];

	double uptime;

	/* memory information in kilobytes */
	unsigned long long mem, memeasyfree, memfree, memmax, swap, swapfree, swapmax;
	unsigned long long bufmem, buffers, cached;

	unsigned short procs;
	unsigned short run_procs;

	float *cpu_usage;
	/* struct cpu_stat cpu_summed; what the hell is this? */
	unsigned int cpu_count;
	int cpu_avg_samples;

	int net_avg_samples;

	int diskio_avg_samples;

	float loadavg[3];

	struct mail_s *mail;
	int mail_running;
#ifdef XMMS2
	struct xmms2_s xmms2;
#endif
#ifdef AUDACIOUS
	AUDACIOUS_S audacious;
#endif
#ifdef BMPX
	struct bmpx_s bmpx;
#endif
	struct usr_info users;
	struct gateway_info gw_info;
	struct dns_data nameserver_info;
	struct process *cpu[10];
	struct process *memu[10];
	struct process *time[10];
#ifdef IOSTATS
	struct process *io[10];
#endif
	struct process *first_process;
	unsigned long looped;
	struct entropy_s entropy;
	double music_player_interval;

#ifdef X11
	struct x11_info x11;
#endif

#ifdef APCUPSD
	APCUPSD_S apcupsd;
#endif

	short kflags;	/* kernel settings, see enum KFLAG */
};

enum x_initialiser_state {
	NO = 0,
	YES = 1,
	NEVER = 2
};

#ifdef X11
/* for fonts, used in fonts.c, core.c, etc */
struct font_list {

	char name[DEFAULT_TEXT_BUFFER_SIZE];
	int num;
	XFontStruct *font;

#ifdef XFT
	XftFont *xftfont;
	int font_alpha;
#endif
};
#endif /* X11 */

typedef struct _conky_context_s {
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
#endif /* IOSTATS */
	unsigned int top_name_width;
	int output_methods;
	int extra_newline;
	enum x_initialiser_state x_initialised;
	/* Update interval */
	double update_interval;
	double update_interval_old;
	double update_interval_bat;

	double current_update_time, next_update_time, last_update_time;

	/* struct that has all info to be shared between
	 * instances of the same text object */
	struct information info;

	/* path to config file */
	char *current_config;

	/* set to 1 if you want all text to be in uppercase */
	unsigned int stuff_in_uppercase;

	/* Run how many times? */
	unsigned long total_run_times;

	/* fork? */
	int fork_to_background;

	int cpu_avg_samples, net_avg_samples, diskio_avg_samples;

	/* filenames for output */
	char *overwrite_file; FILE *overwrite_fpointer;
	char *append_file; FILE *append_fpointer;

#ifdef X11
	/* display to connect to */
	char *disp;

	int show_graph_scale;
	int show_graph_range;

	/* Position on the screen */
	int text_alignment;
	int gap_x, gap_y;

	/* border */
	int draw_borders;
	int draw_graph_borders;
	int stippled_borders;

	int draw_shades, draw_outline;

	long default_fg_color, default_bg_color, default_out_color;

	/* create own window or draw stuff to root? */
	int set_transparent;

#ifdef OWN_WINDOW
	int own_window;
	int background_colour;

	/* fixed size/pos is set if wm/user changes them */
	int fixed_size, fixed_pos;
#endif

	int minimum_width, minimum_height;
	int maximum_width;

	int selected_font;
	int last_font_height;

	/* text size */
	int text_start_x, text_start_y;	/* text start position in window */
	int text_width, text_height;

	conky_window window;

#endif /* X11 */

#ifdef __OpenBSD__
	int sensor_device;
#endif

	long color0, color1, color2, color3, color4, color5, color6, color7, color8,
		 color9;

	char *template[MAX_TEMPLATES];

	/* maximum size of config TEXT buffer, i.e. below TEXT line. */
	unsigned int max_user_text;

	/* maximum size of individual text buffers, ie $exec buffer size */
	unsigned int text_buffer_size;

	/* UTF-8 */
	int utf8_mode;

	/* no buffers in used memory? */
	int no_buffers;

	/* pad percentages to decimals? */
	int pad_percents0;

	char *global_text;

	long global_text_lines;

	int total_updates;
	int updatereset;

	int need_to_update;

	/* formatted text to render on screen, generated in generate_text(), drawn
	 * in draw_stuff() */
	char *text_buffer;

	char **xargv;
	int xargc;

	/* used in colours.c */
	short colour_depth;
	long redmask, greenmask, bluemask;


	struct font_list *fonts;
	int font_count;

	/* used in common.c */
	double last_meminfo_update;
	double last_fs_update;

	unsigned long long need_mask;

	/* two strings for internal use */
	char *tmpstring1, *tmpstring2;
} conky_context;

#endif /* _CONKY_STRUCTS_H_ */
