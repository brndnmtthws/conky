/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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
 * $Id$
 *
 */

#ifndef _conky_h_
#define _conky_h_

#if defined(HAS_MCHECK_H)
#include <mcheck.h>
#endif /* HAS_MCHECK_H */

#undef EQUAL
#undef FALSE
#undef TRUE
#define EQUAL 0	//returnvalue of strcmp-variants when strings are equal
#define FALSE 0
#define TRUE 1

#include "config.h"
#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <langinfo.h>
#include <wchar.h>
#include <sys/param.h>

#if !defined(__GNUC__)
#  define __attribute__(x) /* nothing */
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "freebsd.h"
#endif /* __FreeBSD__ || __FreeBSD_kernel__ */

#if defined(__OpenBSD__)
#include "openbsd.h"
#endif /* __OpenBSD__ */

#ifndef HAVE_STRNDUP
// use our own strndup() if it's not available
char *strndup(const char *s, size_t n);
#endif /* HAVE_STRNDUP */

#ifdef AUDACIOUS
#include "audacious.h"
#endif

#ifdef XMMS2
#include <xmmsclient/xmmsclient.h>
#endif

#ifdef RSS
#include "rss.h"
#endif

#ifdef EVE
#include "eve.h"
#endif

#ifdef SMAPI
#include "smapi.h"
#endif

#ifdef NVIDIA
#include "nvidia.h"
#endif

#include "mboxscan.h"
#include "timed_thread.h"
#include "top.h"

#define DEFAULT_TEXT_BUFFER_SIZE 256
extern unsigned int text_buffer_size;

/* maximum number of special things, e.g. fonts, offsets, aligns, etc. */
#define MAX_SPECIALS_DEFAULT 512

/* maximum size of config TEXT buffer, i.e. below TEXT line. */
#define MAX_USER_TEXT_DEFAULT 16384

#include <sys/socket.h>

#define ERR(...) { \
	fprintf(stderr, "Conky: "); \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, "\n"); \
}

/* critical error */
#define CRIT_ERR(...) \
	{ ERR(__VA_ARGS__); exit(EXIT_FAILURE); }

struct net_stat {
	const char *dev;
	int up;
	long long last_read_recv, last_read_trans;
	long long recv, trans;
	double recv_speed, trans_speed;
	struct sockaddr addr;
	char* addrs;
	double net_rec[15], net_trans[15];
	// wireless extensions
	char essid[32];
	char bitrate[16];
	char mode[16];
	int link_qual;
	int link_qual_max;
	char ap[18];
};

struct dns_data {
	int nscount;
	char **ns_list;
};

struct fs_stat {
	char path[DEFAULT_TEXT_BUFFER_SIZE];
	char type[DEFAULT_TEXT_BUFFER_SIZE];
	long long size;
	long long avail;
	long long free;
	char set;
};

#include "diskio.h"

struct mail_s {			// for imap and pop3
	unsigned long unseen;
	unsigned long messages;
	unsigned long used;
	unsigned long quota;
	unsigned long port;
	float interval;
	double last_update;
	char host[128];
	char user[128];
	char pass[128];
	char command[1024];
	char folder[128];
	timed_thread *p_timed_thread;
	char secure;
};

/* struct cpu_stat {
	unsigned int user, nice, system, idle, iowait, irq, softirq;
	int cpu_avg_samples;
}; */

#ifdef MPD
#include "mpd.h"
#endif

#ifdef XMMS2
#include "xmms2.h"
#endif

#ifdef AUDACIOUS
#include "audacious.h"
#endif

#ifdef BMPX
#include "bmpx.h"
#endif

void update_entropy(void);
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

struct x11_info {
	struct monitor_info monitor;
};
#endif

#ifdef TCP_PORT_MONITOR
#include "libtcp-portmon.h"
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
#ifdef RSS
	INFO_RSS = 24,
#endif
#ifdef SMAPI
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
	INFO_DNS = 30

};

/* get_battery_stuff() item selector */
enum {
	BATTERY_STATUS,
	BATTERY_TIME
};

/* if_up strictness selector */
enum {
	IFUP_UP,
	IFUP_LINK,
	IFUP_ADDR
} ifup_strictness;

/* Update interval */
double update_interval;

volatile int g_signal_pending;

struct information {
	unsigned int mask;

	struct utsname uname_s;

	char freq[10];

	double uptime;

	/* memory information in kilobytes */
	unsigned long long mem, memeasyfree, memfree, memmax, swap, swapmax;
	unsigned long long bufmem, buffers, cached;

	unsigned short procs;
	unsigned short run_procs;

	float *cpu_usage;
	/* struct cpu_stat cpu_summed; what the hell is this? */
	unsigned int cpu_count;
	unsigned int cpu_avg_samples;

	unsigned int net_avg_samples;

	float loadavg[3];

	struct mail_s *mail;
	int mail_running;
#ifdef MPD
	struct mpd_s mpd;
#endif
#ifdef XMMS2
	struct xmms2_s xmms2;
	int xmms2_conn_state;
	xmms_socket_t xmms2_fd;
	fd_set xmms2_fdset;
	xmmsc_connection_t *xmms2_conn;
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
	struct process *first_process;
	unsigned long looped;
#ifdef TCP_PORT_MONITOR
	tcp_port_monitor_collection_t *p_tcp_port_monitor_collection;
#endif
	struct entropy_s entropy;
	double music_player_interval;

#ifdef X11
	struct x11_info x11;
#endif

	short kflags;	/* kernel settings, see enum KFLAG */

	unsigned int diskio_value;
	unsigned int diskio_read_value;
	unsigned int diskio_write_value;
};

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

#define TO_X 1
#define TO_STDOUT 2
int output_methods;

int top_cpu;
int top_mem;

int use_spacer;

enum spacer_opts { NO_SPACER = 0, LEFT_SPACER, RIGHT_SPACER };

char *tmpstring1;
char *tmpstring2;

#ifdef X11
#include "x11.h"
#endif /* X11 */

int cpu_separate;
int short_units;

/* struct that has all info */
struct information info;

void signal_handler(int);
void reload_config(void);
void clean_up(void);

void update_uname(void);
double get_time(void);
FILE *open_file(const char *file, int *reported);
void variable_substitute(const char *s, char *dest, unsigned int n);
void format_seconds(char *buf, unsigned int n, long t);
void format_seconds_short(char *buf, unsigned int n, long t);
struct net_stat *get_net_stat(const char *dev);
void clear_net_stats(void);
void free_dns_data(void);
void update_dns_data(void);
void update_users(void);

#ifdef X11
void update_x11info(void);
#endif

void update_stuff(void);

int round_to_int(float f);

extern unsigned long long need_mask;

extern double current_update_time, last_update_time;

extern int no_buffers;

#if defined(__linux__)
#include "linux.h"
#endif

#include "fs.h"
#include "mixer.h"
#include "mail.h"

#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
		|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
int apm_getinfo(int fd, apm_info_t aip);
char *get_apm_adapter(void);
char *get_apm_battery_life(void);
char *get_apm_battery_time(void);
#endif

#ifdef HDDTEMP
#include "hddtemp.h"
#endif /* HDDTEMP */

/* in nvidia.c */
#ifdef NVIDIA

int get_nvidia_value(QUERY_ID qid, Display *dpy, int highorlow);

#endif /* NVIDIA */

#endif
