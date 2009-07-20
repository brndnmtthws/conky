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
 */

#ifndef _conky_h_
#define _conky_h_

#include "config.h"	/* defines */
#include "common.h"	/* at least for struct dns_data */
#include <sys/utsname.h> /* struct uname_s */

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

#ifdef AUDACIOUS
#include "audacious.h"
#endif

#ifdef BMPX
#include "bmpx.h"
#endif

#ifdef EVE
#include "eve.h"
#endif

#ifdef HDDTEMP
#include "hddtemp.h"
#endif /* HDDTEMP */

#ifdef MOC
#include "moc.h"
#endif

#ifdef MPD
#include "mpd.h"
#endif

#ifdef NVIDIA
#include "nvidia.h"
#endif

#ifdef HAVE_CURL
#include "ccurl_thread.h"
#endif /* HAVE_CURL */

#ifdef RSS
#include "rss.h"
#endif /* RSS */

#ifdef XOAP
#ifndef WEATHER
#error "WEATHER needs to be defined if XOAP is defined"
#endif /* WEATHER */
#endif /* XOAP */

#ifdef WEATHER
#include "weather.h"
#endif /* WEATHER */

#ifdef HAVE_LUA
#include "llua.h"
#endif /* HAVE_LUA */

#ifdef TCP_PORT_MONITOR
#include "tcp-portmon.h"
#endif

#ifdef XMMS2
#include "xmms2.h"
#endif

#ifdef IBM
#include "ibm.h"
#include "smapi.h"
#endif

#ifdef APCUPSD
#include "apcupsd.h"
#endif

/* sony support */
#include "sony.h"

/* A size for temporary, static buffers to use when
 * one doesn't know what to choose. Defaults to 256.  */
extern unsigned int text_buffer_size;

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
#endif

enum {
	INFO_CPU = 0,
	INFO_MAIL = 1,
	INFO_MEM = 2,
	INFO_NET = 3,
	INFO_PROCS = 4,
	INFO_RUN_PROCS = 5,
	INFO_UPTIME = 6,
	INFO_BUFFERS = 7,
	INFO_FS = 8,
	INFO_SYSFS = 9,
	INFO_MIXER = 10,
	INFO_LOADAVG = 11,
	INFO_UNAME = 12,
	INFO_FREQ = 13,
#ifdef MPD
	INFO_MPD = 14,
#endif
	INFO_TOP = 15,
	INFO_WIFI = 16,
	INFO_DISKIO = 17,
	INFO_I8K = 18,
#ifdef TCP_PORT_MONITOR
	INFO_TCP_PORT_MONITOR = 19,
#endif
#ifdef AUDACIOUS
	INFO_AUDACIOUS = 20,
#endif
#ifdef BMPX
	INFO_BMPX = 21,
#endif
#ifdef XMMS2
	INFO_XMMS2 = 22,
#endif
	INFO_ENTROPY = 23,
#ifdef IBM
	INFO_SMAPI = 25,
#endif
	INFO_USERS = 26,
	INFO_GW = 27,
#ifdef NVIDIA
	INFO_NVIDIA = 28,
#endif
#ifdef X11
	INFO_X11 = 29,
#endif
	INFO_DNS = 30,
#ifdef MOC
	INFO_MOC = 31,
#endif
#ifdef APCUPSD
 	INFO_APCUPSD = 32,
#endif
#ifdef WEATHER
	INFO_WEATHER = 33,
#endif
};

/* get_battery_stuff() item selector
 * needed by conky.c, linux.c and freebsd.c */
enum {
	BATTERY_STATUS,
	BATTERY_TIME
};

/* if_up strictness selector
 * needed by conky.c and linux.c (and potentially others) */
enum {
	IFUP_UP,
	IFUP_LINK,
	IFUP_ADDR
} ifup_strictness;

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
#ifdef IOSTATS
extern int top_io;
#endif

/* defined in conky.c, needed by top.c */
extern int cpu_separate;

/* struct that has all info to be shared between
 * instances of the same text object */
extern struct information info;

/* defined in users.c */
void update_users(void);

/* defined in conky.c */
extern double current_update_time, last_update_time, update_interval;

/* defined in conky.c */
int spaced_print(char *, int, const char *, int, ...)
	__attribute__((format(printf, 3, 5)));
extern int inotify_fd;

/* defined in conky.c
 * evaluates 'text' and places the result in 'buffer'
 */
void evaluate(char *text, char *buffer);

/* maximum size of config TEXT buffer, i.e. below TEXT line. */
extern unsigned int max_user_text;

/* path to config file */
extern char *current_config;

#ifdef X11
#define TO_X 1
#endif
#define TO_STDOUT 2
#define TO_STDERR 4
#define OVERWRITE_FILE 8
#define APPEND_FILE 16
enum x_initialiser_state {
	NO = 0,
	YES = 1,
	NEVER = 2
};
extern int output_methods;
extern enum x_initialiser_state x_initialised;

#define DEFAULT_TEXT_BUFFER_SIZE_S "##DEFAULT_TEXT_BUFFER_SIZE"

#endif /* _conky_h_ */
