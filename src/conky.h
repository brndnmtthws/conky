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

#ifndef _conky_h_
#define _conky_h_

#include <config.h>	/* defines */
#include "common.h"	/* at least for struct dns_data */
#include <sys/utsname.h> /* struct uname_s */
#include <arpa/inet.h>
#include <memory>
#include "luamm.hh"

#if defined(HAS_MCHECK_H)
#include <mcheck.h>
#endif /* HAS_MCHECK_H */

#undef EQUAL
#undef FALSE
#undef TRUE
#define EQUAL 0	//returnvalue of strcmp-variants when strings are equal
#define FALSE 0
#define TRUE 1

#define DEFAULT_BAR_WIDTH_NO_X 10

#if !defined(__GNUC__)
#  define __attribute__(x) /* nothing */
#endif

#ifndef HAVE_STRNDUP
// use our own strndup() if it's not available
char *strndup(const char *s, size_t n);
#endif /* HAVE_STRNDUP */

/* headers of optional features
 * include them here, so we don't need to run the check
 * in every code file optionally using the feature
 */

/* forward define to make gcc happy */
struct text_object;

#ifdef BUILD_BMPX
#include "bmpx.h"
#endif /* BUILD_BMPX */

#ifdef BUILD_EVE
#include "eve.h"
#endif /* BUILD_EVE */

#ifdef BUILD_HDDTEMP
#include "hddtemp.h"
#endif /* BUILD_HDDTEMP */

#ifdef BUILD_MOC
#include "moc.h"
#endif /* BUILD_MOC */

#ifdef BUILD_MPD
#include "mpd.h"
#endif /* BUILD_MPD */

/* C++ headers */
#ifdef BUILD_CURL
#include "ccurl_thread.h"
#endif /* BUILD_CURL */

#ifdef BUILD_AUDACIOUS
#include "audacious.h"
#endif /* BUILD_AUDACIOUS */

#ifdef BUILD_RSS
#include "rss.h"
#endif /* BUILD_RSS */

#ifdef BUILD_WEATHER_XOAP
#ifndef BUILD_WEATHER_METAR
#error "BUILD_WEATHER_METAR needs to be defined if XOAP is defined"
#endif /* BUILD_WEATHER_METAR */
#endif /* BUILD_WEATHER_XOAP */

#ifdef BUILD_WEATHER_METAR
#include "weather.h"
#endif /* BUILD_WEATHER_METAR */

#ifdef BUILD_PORT_MONITORS
#include "tcp-portmon.h"
#endif

#ifdef BUILD_XMMS2
#include "xmms2.h"
#endif /* BUILD_XMMS2 */

#ifdef BUILD_APCUPSD
#include "apcupsd.h"
#endif /* BUILD_APCUPSD */

/* sony support */
#include "sony.h"

/* A size for temporary, static buffers to use when
 * one doesn't know what to choose. Defaults to 256.  */
extern unsigned int text_buffer_size;

struct usr_info {
	char *names;
	char *times;
	char *ctime;
	char *terms;
	int number;
};

#ifdef BUILD_X11
struct monitor_info {
	int number;
	int current;
};

struct desktop_info {
        int current;
        int number;
		std::string all_names;
		std::string name;
};

struct x11_info {
	struct monitor_info monitor;
	struct desktop_info desktop;
};

#endif /* BUILD_X11 */

/* defined in conky.c */
extern long default_fg_color, default_bg_color, default_out_color;
extern long color0, color1, color2, color3, color4, color5, color6, color7,
	   color8, color9;

struct conftree {
	char* string;
	struct conftree* horz_next;
	struct conftree* vert_next;
	struct conftree* back;
};

char load_config_file(const char *);

char *get_global_text(void);
extern long global_text_lines;

//adds newstring to to the tree unless you can already see it when travelling back.
//if it's possible to attach it then it returns a pointer to the leaf, else it returns NULL
struct conftree* conftree_add(struct conftree* previous, const char* newstring);

extern struct conftree *currentconffile;

#define MAX_TEMPLATES 10
char **get_templates(void);

/* get_battery_stuff() item selector
 * needed by conky.c, linux.c and freebsd.c */
enum {
	BATTERY_STATUS,
	BATTERY_TIME
};

struct information {
	unsigned int mask;

	struct utsname uname_s;

	char freq[10];

	double uptime;

	/* memory information in kilobytes */
	unsigned long long mem, memwithbuffers, memeasyfree, memfree, memmax;
    unsigned long long swap, swapfree, swapmax;
	unsigned long long bufmem, buffers, cached;

	unsigned short procs;
	unsigned short run_procs;
	unsigned short threads;
	unsigned short run_threads;

	float *cpu_usage;
	/* struct cpu_stat cpu_summed; what the hell is this? */
	int cpu_count;
	int cpu_avg_samples;

	int net_avg_samples;

	int diskio_avg_samples;

	float loadavg[3];

#ifdef BUILD_XMMS2
	struct xmms2_s xmms2;
#endif /* BUILD_XMMS2 */
#ifdef BUILD_AUDACIOUS
	audacious_s audacious;
#endif
#ifdef BUILD_BMPX
	struct bmpx_s bmpx;
#endif /* BUILD_BMPX */
	struct usr_info users;
	struct process *cpu[10];
	struct process *memu[10];
	struct process *time[10];
#ifdef BUILD_IOSTATS
	struct process *io[10];
#endif /* BUILD_IOSTATS */
	struct process *first_process;
	unsigned long looped;
	double music_player_interval;

#ifdef BUILD_X11
	struct x11_info x11;
#endif /* BUILD_X11 */

	short kflags;	/* kernel settings, see enum KFLAG */
};

/* needed by linux.c and top.c -> outsource somewhere */
enum {
	/* set to true if kernel uses "long" format for /proc/stats */
	KFLAG_IS_LONGSTAT = 0x01,
	/* set to true if kernel shows # of threads for the proc value
	 * in sysinfo() call */
	KFLAG_PROC_IS_THREADS = 0x02
	/* bits 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 available for future use */
	/* KFLAG_NEXT_ONE = 0x04 */
};
#define KFLAG_SETON(a) info.kflags |= a
#define KFLAG_SETOFF(a) info.kflags &= (~a)
#define KFLAG_FLIP(a) info.kflags ^= a
#define KFLAG_ISSET(a) info.kflags & a

/* defined in conky.c, needed by top.c */
extern int top_cpu, top_mem, top_time;
#ifdef BUILD_IOSTATS
extern int top_io;
#endif /* BUILD_IOSTATS */
extern int top_running;

/* struct that has all info to be shared between
 * instances of the same text object */
extern struct information info;

/* defined in conky.c */
extern double current_update_time, last_update_time, update_interval;

/* if_up strictness selector
 * needed by conky.c and linux.c (and potentially others) */
enum _ifup_strictness {
	IFUP_UP,
	IFUP_LINK,
	IFUP_ADDR
};
extern int ifup_strictness;

int get_stippled_borders(void);

void set_current_text_color(long colour);
long get_current_text_color(void);

void set_updatereset(int);
int get_updatereset(void);
int get_total_updates(void);

/* defined in conky.c */
int spaced_print(char *, int, const char *, int, ...)
	__attribute__((format(printf, 3, 5)));
extern int inotify_fd;

/* defined in conky.c
 * evaluates 'text' and places the result in 'p' of max length 'p_max_size'
 */
void evaluate(const char *text, char *p, int p_max_size);

void set_update_interval(double interval);

void parse_conky_vars(struct text_object *, const char *, char *, int);

void extract_object_args_to_sub(struct text_object *, const char *);

void generate_text_internal(char *, int, struct text_object);

int percent_print(char *, int, unsigned);
void human_readable(long long, char *, int);

/* maximum size of config TEXT buffer, i.e. below TEXT line. */
extern unsigned int max_user_text;

/* path to config file */
extern std::string current_config;

#define TO_STDOUT 2
#define TO_STDERR 4
#define OVERWRITE_FILE 8
#define APPEND_FILE 16
#ifdef BUILD_NCURSES
#define TO_NCURSES 32
#endif /* BUILD_NCURSES */
enum x_initialiser_state {
	NO = 0,
	YES = 1,
	NEVER = 2
};
extern int output_methods;
extern enum x_initialiser_state x_initialised;

#define DEFAULT_TEXT_BUFFER_SIZE_S "##DEFAULT_TEXT_BUFFER_SIZE"

#define NOBATTERY 0

/* to get rid of 'unused variable' warnings */
#define UNUSED(a)  (void)a
#define UNUSED_ATTR __attribute__ ((unused))

template <class T>
void free_and_zero(T *&ptr) {
	if(ptr) {
		free(ptr);
		ptr = NULL;
	}
}

extern std::auto_ptr<lua::state> state;

#endif /* _conky_h_ */
