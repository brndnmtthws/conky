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

#ifndef _conky_h_
#define _conky_h_

#define __STDC_FORMAT_MACROS

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

#ifdef BUILD_MYSQL
#include "mysql.h"
#endif /* BUILD_MYSQL */

#ifdef BUILD_WEATHER_XOAP
#ifndef BUILD_WEATHER_METAR
#error "BUILD_WEATHER_METAR needs to be defined if XOAP is defined"
#endif /* BUILD_WEATHER_METAR */
#endif /* BUILD_WEATHER_XOAP */

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
extern conky::range_config_setting<unsigned int> text_buffer_size;

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

struct conftree {
	char* string;
	struct conftree* horz_next;
	struct conftree* vert_next;
	struct conftree* back;
};

void load_config_file();

char *get_global_text(void);
extern long global_text_lines;

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
#if defined(__DragonFly__)
    char uname_v[256]; /* with git version */
#endif

	char freq[10];

	double uptime;

	/* memory information in kilobytes */
	unsigned long long mem, memwithbuffers, memeasyfree, memfree, memmax, memdirty;
    unsigned long long swap, swapfree, swapmax;
	unsigned long long bufmem, buffers, cached;

	unsigned short procs;
	unsigned short run_procs;
	unsigned short threads;
	unsigned short run_threads;

	float *cpu_usage;
	/* struct cpu_stat cpu_summed; what the hell is this? */
	int cpu_count;

	float loadavg[3];

#ifdef BUILD_XMMS2
	struct xmms2_s xmms2;
#endif /* BUILD_XMMS2 */
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

#ifdef BUILD_X11
	struct x11_info x11;
#endif /* BUILD_X11 */

	short kflags;	/* kernel settings, see enum KFLAG */
};

class music_player_interval_setting: public conky::simple_config_setting<double> {
	typedef conky::simple_config_setting<double> Base;

protected:
	virtual void lua_setter(lua::state &l, bool init);

public:
	music_player_interval_setting()
		: Base("music_player_interval", 0, true)
	{}
};
extern music_player_interval_setting music_player_interval;

extern conky::range_config_setting<int> cpu_avg_samples;
extern conky::range_config_setting<int> net_avg_samples;
extern conky::range_config_setting<int> diskio_avg_samples;

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
extern double current_update_time, last_update_time;

extern conky::range_config_setting<double> update_interval;
extern conky::range_config_setting<double> update_interval_on_battery;
double active_update_interval();

extern conky::range_config_setting<char>  stippled_borders;

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

void parse_conky_vars(struct text_object *, const char *, char *, int);

void extract_object_args_to_sub(struct text_object *, const char *);

void generate_text_internal(char *, int, struct text_object);

int percent_print(char *, int, unsigned);
void human_readable(long long, char *, int);

/* maximum size of config TEXT buffer, i.e. below TEXT line. */
extern conky::range_config_setting<unsigned int> max_user_text;

/* path to config file */
extern std::string current_config;

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

extern std::unique_ptr<lua::state> state;

extern int argc_copy;
extern char** argv_copy;

#endif /* _conky_h_ */
