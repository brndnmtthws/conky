/*
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al. (see AUTHORS)
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
 *  $Id$
 */

#ifndef _conky_h_
#define _conky_h_

#if defined(HAS_MCHECK_H)
#include <mcheck.h>
#endif /* HAS_MCHECK_H */
#include "config.h"
#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <langinfo.h>
#include <wchar.h>
#include <sys/param.h>
#if defined(__FreeBSD__)
#include <sys/mount.h>
#include <sys/ucred.h>
#include <fcntl.h>
#include <kvm.h>
#endif /* __FreeBSD__ */

#if defined(__FreeBSD__) && (defined(i386) || defined(__i386__))
#include <machine/apm_bios.h>
#endif /* __FreeBSD__ */

#if defined(__OpenBSD__)
#include <sys/sysctl.h>
#include <sys/sensors.h>
#include <machine/apmvar.h>
#endif /* __OpenBSD__ */

#ifdef AUDACIOUS
#include "audacious.h"
#endif

#ifdef XMMS2
#include <xmmsclient/xmmsclient.h>
#endif

#ifdef RSS
#include "prss.h"
#endif

#include "mboxscan.h"
#include "timed_thread.h"

#define TOP_CPU 1
#define TOP_NAME 2
#define TOP_PID 3
#define TOP_MEM 4

#define TEXT_BUFFER_SIZE 1280
#define P_MAX_SIZE ((TEXT_BUFFER_SIZE * 4) - 2) 
extern unsigned int text_buffer_size;

/* maximum number of special things, e.g. fonts, offsets, aligns, etc. */
#define MAX_SPECIALS_DEFAULT 512

/* maximum size of config TEXT buffer, i.e. below TEXT line. */
#define MAX_USER_TEXT_DEFAULT 16384

#include <sys/socket.h>

#define ERR(s, varargs...) \
fprintf(stderr, "Conky: " s "\n", ##varargs)

/* critical error */
#define CRIT_ERR(s, varargs...) \
{ fprintf(stderr, "Conky: " s "\n", ##varargs);  exit(EXIT_FAILURE); }

struct i8k_struct {
	char *version;
	char *bios;
	char *serial;
	char *cpu_temp;
	char *left_fan_status;
	char *right_fan_status;
	char *left_fan_rpm;
	char *right_fan_rpm;
	char *ac_status;
	char *buttons_status;
};

struct i8k_struct i8k;

struct net_stat {
	const char *dev;
	int up;
	long long last_read_recv, last_read_trans;
	long long recv, trans;
	double recv_speed, trans_speed;
	struct sockaddr addr;
	double net_rec[15], net_trans[15];
	// wireless extensions
	char essid[32];
	char bitrate[16];
	char mode[16];
	int link_qual;
	int link_qual_max;
	char ap[18];
};

unsigned int diskio_value;
unsigned int diskio_read_value;
unsigned int diskio_write_value;

struct fs_stat {
	char *path;
	long long size;
	long long avail;
	long long free;
};

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
} mail;

/*struct cpu_stat {
	unsigned int user, nice, system, idle, iowait, irq, softirq;
	int cpu_avg_samples;
};*/

#ifdef MPD
struct mpd_s {
	char *title;
	char *artist;
	char *album;
	char *status;
	char *random;
	char *repeat;
	char *track;
	char *name;
	char *file;
	int volume;
	unsigned int port;
	char host[128];
	char password[128];
	float progress;
	int bitrate;
	int length;
	int elapsed;
	int max_title_len;		/* e.g. ${mpd_title 50} */
};

#endif

#ifdef XMMS2
struct xmms2_s {
    char* artist;
    char* album;
    char* title;
    char* genre;
    char* comment;
    char* decoder;
    char* transport;
    char* url;
    char* date;
    int tracknr;
    int bitrate;
    unsigned int id;
    int duration;
    int elapsed;
    float size;

    float progress;
    char* status;
};
#endif

#ifdef AUDACIOUS
struct audacious_s {
	audacious_t items;              /* e.g. items[AUDACIOUS_STATUS] */
	int max_title_len;		/* e.g. ${audacious_title 50} */
	timed_thread *p_timed_thread;
};
#endif

#ifdef BMPX
void update_bmpx();
struct bmpx_s {
	char *title;
	char *artist;
	char *album;
	char *uri;
	int bitrate;
	int track;
};
#endif

void update_entropy();
struct entropy_s {
	unsigned int entropy_avail;
	unsigned int poolsize;
};

#ifdef TCP_PORT_MONITOR
#include "libtcp-portmon.h"
#define MAX_PORT_MONITOR_CONNECTIONS_DEFAULT 256
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
};


/* get_battery_stuff() item selector */
enum {
	BATTERY_STATUS,
	BATTERY_TIME
};

#ifdef MPD
#include "libmpdclient.h"
#endif

/* Update interval */
double update_interval;

volatile int g_signal_pending;

struct information {
	unsigned int mask;

	struct utsname uname_s;

	char freq[10];

	double uptime;

	/* memory information in kilobytes */
	unsigned long long mem, memmax, swap, swapmax;
	unsigned long long bufmem, buffers, cached;

	unsigned short procs;
	unsigned short run_procs;

	float *cpu_usage;
	/*	struct cpu_stat cpu_summed; what the hell is this? */
	unsigned int cpu_count;
	unsigned int cpu_avg_samples;

	unsigned int net_avg_samples;

	float loadavg[3];

	struct mail_s* mail;
	int mail_running;
#ifdef MPD
	struct mpd_s mpd;
	mpd_Connection *conn;
#endif
#ifdef XMMS2
	struct xmms2_s xmms2;
	int xmms2_conn_state;
	xmms_socket_t xmms2_fd; 
	fd_set xmms2_fdset;
	xmmsc_connection_t *xmms2_conn;
#endif
#ifdef AUDACIOUS
	struct audacious_s audacious;
#endif
#ifdef BMPX
	struct bmpx_s bmpx;
#endif
	struct process *cpu[10];
	struct process *memu[10];
	struct process *first_process;
	unsigned long looped;
#ifdef TCP_PORT_MONITOR
  tcp_port_monitor_collection_t * p_tcp_port_monitor_collection;
#endif
	struct entropy_s entropy;
  double music_player_interval;

	short kflags;  /* kernel settings, see enum KFLAG */
};

enum {
	KFLAG_IS_LONGSTAT = 0x01,         /* set to true if kernel uses "long" format for /proc/stats */
	KFLAG_PROC_IS_THREADS=0x02       /* set to true if kernel shows # of threads for the proc value in sysinfo() call */
/* 	KFLAG_NEXT_ONE=0x04                 bits 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 available for future use */
     };	

#define KFLAG_SETON(a) info.kflags |= a 
#define KFLAG_SETOFF(a) info.kflags &= (~a)
#define KFLAG_FLIP(a) info.kflags ^= a
#define KFLAG_ISSET(a) info.kflags & a


int out_to_console;

int top_cpu;
int top_mem;

int use_spacer;

char tmpstring1[TEXT_BUFFER_SIZE];
char tmpstring2[TEXT_BUFFER_SIZE];

#ifdef X11
/* in x11.c */

#include <X11/Xlib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#ifdef XFT
#include <X11/Xft/Xft.h>
#endif

#ifdef HAVE_XDBE
#include <X11/extensions/Xdbe.h>
#endif

#define ATOM(a) XInternAtom(display, #a, False)

#ifdef OWN_WINDOW
enum _window_type {
        TYPE_NORMAL = 0,
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
#define SET_HINT(mask,hint)	(mask |= (1<<hint))
#define TEST_HINT(mask,hint)	(mask & (1<<hint))
#endif
struct conky_window {
	Window root,window,desktop;
	Drawable drawable;
	GC gc;
#ifdef HAVE_XDBE
	XdbeBackBuffer back_buffer;
#endif
#ifdef XFT
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

#ifdef HAVE_XDBE
extern int use_xdbe;
#endif


#ifdef XFT
extern int use_xft;
#endif

extern Display *display;
extern int display_width;
extern int display_height;
extern int screen;

extern int workarea[4];

extern struct conky_window window;

void init_X11();
void init_window(int use_own_window, int width, int height, int set_trans, int back_colour, 
                 char **argv, int argc);
void create_gc();
void set_transparent_background(Window win);
long get_x11_color(const char *);

#endif /* X11 */

/* in common.c */

/* struct that has all info */
struct information info;

void signal_handler(int);
void reload_config(void);
void clean_up(void);

void update_uname();
double get_time(void);
FILE *open_file(const char *file, int *reported);
void variable_substitute(const char *s, char *dest, unsigned int n);
void format_seconds(char *buf, unsigned int n, long t);
void format_seconds_short(char *buf, unsigned int n, long t);
struct net_stat *get_net_stat(const char *dev);
void clear_net_stats(void);

void update_stuff();

int round_to_int(float f);

#define SET_NEED(a) need_mask |= 1 << (a)
extern unsigned long long need_mask;

extern double current_update_time, last_update_time;

extern int no_buffers;

/* system dependant (in linux.c) */

int check_mount(char *s);
void update_diskio(void);
void prepare_update(void);
void update_uptime(void);
void update_meminfo(void);
void update_net_stats(void);
void update_cpu_usage(void);
void update_total_processes(void);
void update_running_processes(void);
void update_i8k(void);
char get_freq( char *, size_t, char *, int, unsigned int ); 
void get_freq_dynamic( char *, size_t, char *, int ); 
char get_voltage(char *, size_t, char *, int, unsigned int ); /* ptarjan */
void update_load_average();

int open_sysfs_sensor(const char *dir, const char *dev, const char *type, int n, int *div, char *devtype);
#define open_i2c_sensor(dev,type,n,div,devtype) \
    open_sysfs_sensor("/sys/bus/i2c/devices/",dev,type,n,div,devtype)

#define open_platform_sensor(dev,type,n,div,devtype) \
    open_sysfs_sensor("/sys/bus/platform/devices/",dev,type,n,div,devtype)

#define open_hwmon_sensor(dev,type,n,div,devtype) \
   open_sysfs_sensor("/sys/class/hwmon/",dev,type,n,div,devtype); \

double get_sysfs_info(int *fd, int arg, char *devtype, char *type);

void get_adt746x_cpu( char *, size_t ); 
void get_adt746x_fan( char *, size_t ); 
unsigned int get_diskio(void);

int open_acpi_temperature(const char *name);
double get_acpi_temperature(int fd);
void get_acpi_ac_adapter( char *, size_t ); 
void get_acpi_fan( char *, size_t ); 
void get_battery_stuff(char *buf, unsigned int n, const char *bat, int item);
int get_battery_perct(const char *bat);
int get_battery_perct_bar(const char *bat);
void get_ibm_acpi_fan(char *buf, size_t client_buffer_size);
void get_ibm_acpi_temps(void);
void get_ibm_acpi_volume(char *buf, size_t client_buffer_size);
void get_ibm_acpi_brightness(char *buf, size_t client_buffer_size);
void get_cpu_count();

struct ibm_acpi_struct {
    unsigned int temps[8];
};

struct ibm_acpi_struct ibm_acpi;

#if defined(__OpenBSD__)
void update_obsd_sensors(void);
void get_obsd_vendor(char *buf, size_t client_buffer_size);
void get_obsd_product(char *buf, size_t client_buffer_size);

#define OBSD_MAX_SENSORS 256
struct obsd_sensors_struct {
	int device;
	float temp[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
	unsigned int fan[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
	float volt[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
};
struct obsd_sensors_struct obsd_sensors;
#endif /* __OpenBSD__ */


enum { PB_BATT_STATUS, PB_BATT_PERCENT, PB_BATT_TIME};
void get_powerbook_batt_info(char*, size_t, int);

struct process {
	struct process *next;
	struct process *previous;

	pid_t pid;
	char *name;
	float amount;
	unsigned long user_time;
	unsigned long total;
	unsigned long kernel_time;
	unsigned long previous_user_time;
	unsigned long previous_kernel_time;
	unsigned int vsize;
	unsigned int rss;
	unsigned int time_stamp;
	unsigned int counted;
	unsigned int changed;
	float totalmem;
};

struct local_mail_s {
	char *box;
	int mail_count;
	int new_mail_count;
	float interval;
	time_t last_mtime;
	double last_update;
};

void update_top();
void free_all_processes();
struct process *get_first_process();

/* fs-stuff is possibly system dependant (in fs.c) */

void update_fs_stats(void);
struct fs_stat *prepare_fs_stat(const char *path);
void clear_fs_stats(void);

/* in mixer.c */

int mixer_init(const char *);
int mixer_get_avg(int);
int mixer_get_left(int);
int mixer_get_right(int);

/* in mail.c */

extern char *current_mail_spool;

void update_mail_count();

/* in freebsd.c */
#if defined(__FreeBSD__)
kvm_t *kd;
#endif

#if (defined(__FreeBSD__) || defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
#ifdef __OpenBSD__
typedef struct apm_power_info *apm_info_t;
#endif
int apm_getinfo(int fd, apm_info_t aip);
char *get_apm_adapter(void);
char *get_apm_battery_life(void);
char *get_apm_battery_time(void);
#endif

/* in mpd.c */
#ifdef MPD
extern void init_mpd_stats(struct information *current_info);
void *update_mpd(void);
extern timed_thread *mpd_timed_thread;
#endif /* MPD */

/* in xmms2.c */
#ifdef XMMS2
void update_xmms2();
#endif

/* in hddtemp.c */
#ifdef HDDTEMP
int scan_hddtemp(const char *arg, char **dev, char **addr, int *port);
char *get_hddtemp_info(char *dev, char *addr, int port, char *unit);
#endif /* HDDTEMP */

/* in rss.c */
#ifdef RSS
PRSS* get_rss_info(char *uri, int delay);
void init_rss_info();
void free_rss_info();
#endif /* RSS */

/* in linux.c */

#endif


