/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */

#ifndef _conky_h_
#define _conky_h_

#include <pthread.h>
#if defined(HAS_MCHECK_H)
#include <mcheck.h>
#endif /* HAS_MCHECK_H */
#include "config.h"
#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>
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

#ifdef X11
#if defined(HAVE_CAIRO_H) && defined(HAVE_CAIRO_XLIB_H) && defined(WANT_CAIRO)
#define CAIRO
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cairo.h>
#include <cairo-xlib.h>
#endif
#endif /* X11 */

#if defined(__FreeBSD__) && (defined(i386) || defined(__i386__))
#include <machine/apm_bios.h>
#endif /* __FreeBSD__ */

#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
#include "xmms.h"
#endif

#define TOP_CPU 1
#define TOP_NAME 2
#define TOP_PID 3
#define TOP_MEM 4

#define TEXT_BUFFER_SIZE 1280
#define P_MAX_SIZE ((TEXT_BUFFER_SIZE * 4) - 2) 

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
	int linkstatus;
	double net_rec[15], net_trans[15];
};

unsigned int diskio_value;

struct fs_stat {
	char *path;
	long long size;
	long long avail;
	long long free;
};

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
};
#endif

#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
struct xmms_s {
	unsigned int project_mask;
	unsigned int current_project;
	xmms_t items;                   /* e.g. items[XMMS_STATUS] yields char[] */
	int runnable;                   /* used to signal worker thread to stop */
	pthread_t thread;               /* worker thread for xmms updating */
	pthread_attr_t thread_attr;     /* thread attributes */
	pthread_mutex_t item_mutex;     /* mutex for item array */
	pthread_mutex_t runnable_mutex; /* mutex for runnable flag */
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

#ifdef TCP_PORT_MONITOR
#include "libtcp-portmon.h"
#define MIN_PORT_MONITORS_DEFAULT 16
#define MIN_PORT_MONITOR_CONNECTIONS_DEFAULT 256
#endif

enum {
	INFO_CPU = 0,
	INFO_MAIL = 1,
	INFO_MEM = 2,
	INFO_NET = 3,
#ifdef SETI
	INFO_SETI = 4,
#endif
	INFO_PROCS = 5,
	INFO_RUN_PROCS = 6,
	INFO_UPTIME = 7,
	INFO_BUFFERS = 8,
	INFO_FS = 9,
	INFO_I2C = 10,
	INFO_MIXER = 11,
	INFO_LOADAVG = 12,
	INFO_UNAME = 13,
	INFO_FREQ = 14,
#ifdef MPD
	INFO_MPD = 15,
#endif
	INFO_TOP = 16,
#ifdef MLDONKEY
	INFO_MLDONKEY = 18,
#endif
	INFO_WIFI = 19,
	INFO_DISKIO = 20,
	INFO_I8K = 21,
#ifdef TCP_PORT_MONITOR
        INFO_TCP_PORT_MONITOR = 22,
#endif
#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
	INFO_XMMS = 23,
#endif
#ifdef BMPX
	INFO_BMPX = 24,
#endif
};


#ifdef MPD
#include "libmpdclient.h"
#endif

volatile int g_signal_pending;

struct information {
	unsigned int mask;

	struct utsname uname_s;

	char freq[10];

	double uptime;

	/* memory information in kilobytes */
	unsigned long mem, memmax, swap, swapmax;
	unsigned long bufmem, buffers, cached;

	unsigned short procs;
	unsigned short run_procs;

	float *cpu_usage;
	/*	struct cpu_stat cpu_summed; what the hell is this? */
	unsigned int cpu_count;
	unsigned int cpu_avg_samples;

	unsigned int net_avg_samples;

	float loadavg[3];

	int new_mail_count, mail_count;
#ifdef SETI
	float seti_prog;
	float seti_credit;
#endif
#ifdef MPD
	struct mpd_s mpd;
	mpd_Connection *conn;
#endif
#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
	struct xmms_s xmms;
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

#if defined(HAVE_XDBE) && defined(DOUBLE_BUFFER)
#define XDBE
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
#ifdef XDBE
	XdbeBackBuffer back_buffer;
#endif
#ifdef XFT
	XftDraw *xftdraw;
#endif

	int width;
	int height;
#ifdef OWN_WINDOW
	char wm_class_name[256];
	int x;
	int y;
	unsigned int type;
	unsigned long hints;
#endif
};

#ifdef XDBE
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
void init_window(int use_own_window, int width, int height, int set_trans, int back_colour, char * nodename,
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

void update_stuff();

int round_to_int(float f);

#define SET_NEED(a) need_mask |= 1 << (a)
extern unsigned long long need_mask;

extern double current_update_time, last_update_time;

extern int no_buffers;

/* system dependant (in linux.c) */

void update_diskio(void);
void prepare_update(void);
void update_uptime(void);
void update_meminfo(void);
void update_net_stats(void);
void update_wifi_stats(void);
void update_cpu_usage(void);
void update_total_processes(void);
void update_running_processes(void);
void update_i8k(void);
void get_freq( char *, size_t, char *, int ); /* pk */
void get_freq_dynamic( char *, size_t, char *, int ); /* pk */
void update_load_average();
int open_i2c_sensor(const char *dev, const char *type, int n, int *div,
		    char *devtype);
double get_i2c_info(int *fd, int arg, char *devtype, char *type);

void get_adt746x_cpu( char *, size_t ); /* pk */
void get_adt746x_fan( char *, size_t ); /* pk */
unsigned int get_diskio(void);

int open_acpi_temperature(const char *name);
double get_acpi_temperature(int fd);
void get_acpi_ac_adapter( char *, size_t ); /* pk */
void get_acpi_fan( char *, size_t ); /* pk */
void get_battery_stuff(char *buf, unsigned int n, const char *bat);

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

/* in seti.c */

#ifdef SETI
extern char *seti_dir;

void update_seti();
#endif

/* in freebsd.c */
#if defined(__FreeBSD__)
kvm_t *kd;
#endif

#if defined(__FreeBSD__) && (defined(i386) || defined(__i386__))
int apm_getinfo(int fd, apm_info_t aip);
char *get_apm_adapter(void);
char *get_apm_battery_life(void);
char *get_apm_battery_time(void);
#endif
/* in mpd.c */

#ifdef MPD
void update_mpd();
#endif

#ifdef MLDONKEY
/* in mldonkey.c */
typedef long long int64;
/* The info necessary to connect to mldonkey. login and password can be NULL. */
typedef struct mldonkey_config {
	char *mldonkey_hostname;
	int mldonkey_port;
	char *mldonkey_login;
	char *mldonkey_password;
} mldonkey_config;

/* The MLDonkey status returned */
typedef struct mldonkey_info {
	int64 upload_counter;
	int64 download_counter;
	int nshared_files;
	int64 shared_counter;
	int tcp_upload_rate;
	int tcp_download_rate;
	int udp_upload_rate;
	int udp_download_rate;
	int ndownloaded_files;
	int ndownloading_files;
	int nconnected_networks;
	int connected_networks[1];
} mldonkey_info;

extern mldonkey_info mlinfo;
extern mldonkey_config mlconfig;

int get_mldonkey_status(mldonkey_config * config, mldonkey_info * info);
#endif

/* in linux.c */

/* nothing to see here */

/* in cairo.c */

extern int do_it(void);

#endif
