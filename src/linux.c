/* linux.c
 * Contains linux specific code
 *
 *  $Id$
 */


#include "conky.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
// #include <assert.h>
#include <time.h>
#include "top.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <math.h>

static struct sysinfo s_info;

static int show_nice_processes;

void prepare_update()
{
}

static void update_sysinfo()
{
	sysinfo(&s_info);

	info.uptime = (double) s_info.uptime;

	/* there was some problem with these */
#if 0
//      info.loadavg[0] = s_info.loads[0] / 100000.0f;
	info.loadavg[1] = s_info.loads[1] / 100000.0f;
	info.loadavg[2] = s_info.loads[2] / 100000.0f;
	gkrelltop_process_find_top_three info.mask |= 1 << INFO_LOADAVG;
#endif

	info.procs = s_info.procs;

	/* these aren't nice, no cache and should check kernel version for mem_unit */
#if 0
	info.memmax = s_info.totalram;
	info.mem = s_info.totalram - s_info.freeram;
	info.swapmax = s_info.totalswap;
	info.swap = s_info.totalswap - s_info.swap;
	info.mask |= 1 << INFO_MEM;
#endif

	info.mask |= (1 << INFO_UPTIME) | (1 << INFO_PROCS);
}

void update_uptime()
{
	/* prefers sysinfo() for uptime, I don't really know which one is better
	 * (=faster?) */
#ifdef USE_PROC_UPTIME
	static int rep;
	FILE *fp = open_file("/proc/uptime", &rep);
	if (!fp)
		return 0;
	fscanf(fp, "%lf", &info.uptime);
	fclose(fp);

	info.mask |= (1 << INFO_UPTIME);
#else
	update_sysinfo();
#endif
}

/* these things are also in sysinfo except Buffers:, that's why I'm reading
* them from proc */

static FILE *meminfo_fp;

void update_meminfo()
{
	static int rep;
	/*  unsigned int a; */
	char buf[256];

	info.mem = info.memmax = info.swap = info.swapmax = info.bufmem =
	    info.buffers = info.cached = 0;

	if (meminfo_fp == NULL)
		meminfo_fp = open_file("/proc/meminfo", &rep);
	else
		fseek(meminfo_fp, 0, SEEK_SET);
	if (meminfo_fp == NULL)
		return;

	while (!feof(meminfo_fp)) {
		if (fgets(buf, 255, meminfo_fp) == NULL)
			break;

		if (strncmp(buf, "MemTotal:", 9) == 0) {
			sscanf(buf, "%*s %lu", &info.memmax);
		} else if (strncmp(buf, "MemFree:", 8) == 0) {
			sscanf(buf, "%*s %lu", &info.mem);
		} else if (strncmp(buf, "SwapTotal:", 10) == 0) {
			sscanf(buf, "%*s %lu", &info.swapmax);
		} else if (strncmp(buf, "SwapFree:", 9) == 0) {
			sscanf(buf, "%*s %lu", &info.swap);
		} else if (strncmp(buf, "Buffers:", 8) == 0) {
			sscanf(buf, "%*s %lu", &info.buffers);
		} else if (strncmp(buf, "Cached:", 7) == 0) {
			sscanf(buf, "%*s %lu", &info.cached);
		}
	}
	
	info.mem = info.memmax - info.mem;
	info.swap = info.swapmax - info.swap;

	info.bufmem = info.cached + info.buffers;

	/*if (no_buffers) {
		info.mem -= info.bufmem;
	}*/

	info.mask |= (1 << INFO_MEM) | (1 << INFO_BUFFERS);
}

static FILE *net_dev_fp;
static FILE *net_wireless_fp;

inline void update_net_stats()
{
	static int rep;
	// FIXME: arbitrary size chosen to keep code simple.
	int i, i2;
	unsigned int curtmp1, curtmp2;
	unsigned int k;
	struct ifconf conf;


	char buf[256];
	double delta;

	/* get delta */
	delta = current_update_time - last_update_time;
	if (delta <= 0.0001)
		return;

	/* open file and ignore first two lines */
	if (net_dev_fp == NULL)
		net_dev_fp = open_file("/proc/net/dev", &rep);
	else
		fseek(net_dev_fp, 0, SEEK_SET);
	if (!net_dev_fp)
		return;

	fgets(buf, 255, net_dev_fp);	/* garbage */
	fgets(buf, 255, net_dev_fp);	/* garbage (field names) */

	/* read each interface */
	for (i2 = 0; i2 < 16; i2++) {
		struct net_stat *ns;
		char *s, *p;
		long long r, t, last_recv, last_trans;

		if (fgets(buf, 255, net_dev_fp) == NULL)
			break;
		p = buf;
		while (isspace((int) *p))
			p++;

		s = p;

		while (*p && *p != ':')
			p++;
		if (*p == '\0')
			continue;
		*p = '\0';
		p++;

		ns = get_net_stat(s);
		ns->up = 1;
		last_recv = ns->recv;
		last_trans = ns->trans;

		sscanf(p,
		       /* bytes packets errs drop fifo frame compressed multicast|bytes ... */
		       "%Ld  %*d     %*d  %*d  %*d  %*d   %*d        %*d       %Ld",
		       &r, &t);

		/* if recv or trans is less than last time, an overflow happened */

		if (r < ns->last_read_recv)
			ns->recv +=
			    ((long long) 4294967295U -
			     ns->last_read_recv) + r;
		else
			ns->recv += (r - ns->last_read_recv);
		ns->last_read_recv = r;

		if (t < ns->last_read_trans)
			ns->trans +=
			    ((long long) 4294967295U -
			     ns->last_read_trans) + t;
		else
			ns->trans += (t - ns->last_read_trans);
		ns->last_read_trans = t;

		/*** ip addr patch ***/
		i = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

		conf.ifc_buf = malloc(sizeof(struct ifreq) * 16);

		conf.ifc_len = sizeof(struct ifreq) * 16;

		ioctl((long) i, SIOCGIFCONF, &conf);

		for (k = 0; k < conf.ifc_len / sizeof(struct ifreq); k++) {
			struct net_stat *ns;
			ns = get_net_stat(((struct ifreq *) conf.
					   ifc_buf)[k].ifr_ifrn.ifrn_name);
			ns->addr =
			    ((struct ifreq *) conf.ifc_buf)[k].ifr_ifru.
			    ifru_addr;
		}

		close((long) i);

		free(conf.ifc_buf);


		/*** end ip addr patch ***/


		/* calculate speeds */
		ns->net_rec[0] = (ns->recv - last_recv) / delta;
		ns->net_trans[0] = (ns->trans - last_trans) / delta;
		curtmp1 = 0;
		curtmp2 = 0;
		// get an average
		for (i = 0; (unsigned) i < info.net_avg_samples; i++) {
			curtmp1 += ns->net_rec[i];
			curtmp2 += ns->net_trans[i];
		}
		ns->recv_speed = curtmp1 / (double) info.net_avg_samples;
		ns->trans_speed = curtmp2 / (double) info.net_avg_samples;
		if (info.net_avg_samples > 1) {
			for (i = info.net_avg_samples; i > 1; i--) {
				ns->net_rec[i - 1] = ns->net_rec[i - 2];
				ns->net_trans[i - 1] =
				    ns->net_trans[i - 2];
			}
		}



	}

	/* fclose(net_dev_fp); net_dev_fp = NULL; */
}

inline void update_wifi_stats()
{
	/** wireless stats patch by Bobby Beckmann **/
	static int rep;
	int i;
	char buf[256];
	/*open file and ignore first two lines       sorry, this code sucks ass right now, i'll clean it up later */
	if (net_wireless_fp == NULL)
		net_wireless_fp = open_file("/proc/net/wireless", &rep);
	else
		fseek(net_wireless_fp, 0, SEEK_SET);
	if (net_wireless_fp == NULL)
		return;

	fgets(buf, 255, net_wireless_fp);	/* garbage */
	fgets(buf, 255, net_wireless_fp);	/* garbage (field names) */

	/* read each interface */
	for (i = 0; i < 16; i++) {
		struct net_stat *ns;
		char *s, *p;
		int l, m, n;

		if (fgets(buf, 255, net_wireless_fp) == NULL)
			break;
		p = buf;
		while (isspace((int) *p))
			p++;

		s = p;

		while (*p && *p != ':')
			p++;
		if (*p == '\0')
			continue;
		*p = '\0';
		p++;

		ns = get_net_stat(s);

		sscanf(p, "%*d   %d.  %d.  %d", &l, &m, &n);

		ns->linkstatus = (int) (log(MIN(MAX(l,1),92)) / log(92) * 100);

	}

	/*** end wireless patch ***/
}

int result;

void update_total_processes()
{
	update_sysinfo();
}

#define CPU_SAMPLE_COUNT 15
struct cpu_info {
	unsigned long cpu_user;
	unsigned long cpu_system;
	unsigned long cpu_nice;
	double last_cpu_sum;
	unsigned long clock_ticks;
	double cpu_val[CPU_SAMPLE_COUNT];
};
static short cpu_setup = 0;
static int rep;


static FILE *stat_fp;

void get_cpu_count()
{
	char buf[256];
	if (stat_fp == NULL)
		stat_fp = open_file("/proc/stat", &rep);
	else
		fseek(stat_fp, 0, SEEK_SET);
	if (stat_fp == NULL)
		return;

	info.cpu_count = 0;

	while (!feof(stat_fp)) {
		if (fgets(buf, 255, stat_fp) == NULL)
			break;

		if (strncmp(buf, "cpu", 3) == 0 && isdigit(buf[3])) {
			info.cpu_count++;
		}
	}
	info.cpu_usage = malloc((info.cpu_count + 1) * sizeof(float));
}


inline static void update_stat()
{
	static struct cpu_info *cpu = NULL;
	char buf[256];
	unsigned int i;
	unsigned int index;
	double curtmp;
	if (!cpu_setup) {
		get_cpu_count();
		cpu_setup = 1;
	}
	if (cpu == NULL) {
		cpu = malloc((info.cpu_count + 1) * sizeof(struct cpu_info));
		for (index = 0; index < info.cpu_count + 1; ++index) {
			cpu[index].clock_ticks = 0;
			cpu[index].last_cpu_sum = 0;
			for (i = 0; i < CPU_SAMPLE_COUNT; ++i) {
				cpu[index].cpu_val[i] = 0;
			}
		}
	}
	if (stat_fp == NULL) {
		stat_fp = open_file("/proc/stat", &rep);
	} else {
		fseek(stat_fp, 0, SEEK_SET);
	}
	if (stat_fp == NULL) {
		return;
	}
	index = 0;
	while (!feof(stat_fp)) {
		if (fgets(buf, 255, stat_fp) == NULL)
			break;

		if (strncmp(buf, "procs_running ", 14) == 0) {
			sscanf(buf, "%*s %d", &info.run_procs);
			info.mask |= (1 << INFO_RUN_PROCS);
		} else if (strncmp(buf, "cpu ", 4) == 0) {
			sscanf(buf, "%*s %lu %lu %lu", &(cpu[index].cpu_user), &(cpu[index].cpu_nice), &(cpu[index].cpu_system));
			index++;
			info.mask |= (1 << INFO_CPU);
		} else if (strncmp(buf, "cpu", 3) == 0 && isdigit(buf[3]) && index <= info.cpu_count) {
			sscanf(buf, "%*s %lu %lu %lu", &(cpu[index].cpu_user), &(cpu[index].cpu_nice), &(cpu[index].cpu_system));
			index++;
			info.mask |= (1 << INFO_CPU);
		}
	}
	for (index = 0; index < info.cpu_count + 1; index++) {
		double delta;
		delta = current_update_time - last_update_time;
		if (delta <= 0.001) {
			return;
		}

		if (cpu[index].clock_ticks == 0) {
			cpu[index].clock_ticks = sysconf(_SC_CLK_TCK);
		}
		curtmp = 0;
		cpu[index].cpu_val[0] =
				(cpu[index].cpu_user + cpu[index].cpu_nice + cpu[index].cpu_system -
				cpu[index].last_cpu_sum) / delta / (double) cpu[index].clock_ticks;
		for (i = 0; i < info.cpu_avg_samples; i++) {
			curtmp += cpu[index].cpu_val[i];
		}
		if (index == 0) {
			info.cpu_usage[index] = curtmp / info.cpu_avg_samples / info.cpu_count;
		} else {
			info.cpu_usage[index] = curtmp / info.cpu_avg_samples;
		}
		cpu[index].last_cpu_sum = cpu[index].cpu_user + cpu[index].cpu_nice + cpu[index].cpu_system;
		for (i = info.cpu_avg_samples; i > 1; i--)
			cpu[index].cpu_val[i - 1] = cpu[index].cpu_val[i - 2];

	}

// test code
// this is for getting proc shit
// pee pee
// poo
	//






}

void update_running_processes()
{
	update_stat();
}

void update_cpu_usage()
{
	update_stat();
}

void update_load_average()
{
#ifdef HAVE_GETLOADAVG
	double v[3];
	getloadavg(v, 3);
	info.loadavg[0] = (float) v[0];
	info.loadavg[1] = (float) v[1];
	info.loadavg[2] = (float) v[2];
#else
	static int rep;
	FILE *fp;

	fp = open_file("/proc/loadavg", &rep);
	if (!fp) {
		v[0] = v[1] = v[2] = 0.0;
		return;
	}

	fscanf(fp, "%f %f %f", &info.loadavg[0], &info.loadavg[1],
	       &info.loadavg[2]);

	fclose(fp);
#endif
}

#define PROC_I8K "/proc/i8k"
#define I8K_DELIM " "
static char *i8k_procbuf = NULL;
void update_i8k()
{
	FILE *fp;
	if (!i8k_procbuf) {
		i8k_procbuf = (char*)malloc(128*sizeof(char));
	}
	if ((fp = fopen(PROC_I8K,"r")) == NULL) {
		CRIT_ERR("/proc/i8k doesn't exist! use insmod to make sure the kernel driver is loaded...");
	}

	memset(&i8k_procbuf[0],0,128);
	if (fread(&i8k_procbuf[0],sizeof(char),128,fp) == 0) {
		ERR("something wrong with /proc/i8k...");
	}

	fclose(fp);

  i8k.version = strtok(&i8k_procbuf[0],I8K_DELIM);
	i8k.bios = strtok(NULL,I8K_DELIM);
	i8k.serial = strtok(NULL,I8K_DELIM);
	i8k.cpu_temp = strtok(NULL,I8K_DELIM);
	i8k.left_fan_status = strtok(NULL,I8K_DELIM);	
	i8k.right_fan_status = strtok(NULL,I8K_DELIM);	
	i8k.left_fan_rpm = strtok(NULL,I8K_DELIM);
	i8k.right_fan_rpm = strtok(NULL,I8K_DELIM);
	i8k.ac_status = strtok(NULL,I8K_DELIM);
	i8k.buttons_status = strtok(NULL,I8K_DELIM);
}


/***********************************************************/
/***********************************************************/
/***********************************************************/

static int no_dots(const struct dirent *d)
{
	if (d->d_name[0] == '.')
		return 0;
	return 1;
}

static int
get_first_file_in_a_directory(const char *dir, char *s, int *rep)
{
	struct dirent **namelist;
	int i, n;

	n = scandir(dir, &namelist, no_dots, alphasort);
	if (n < 0) {
		if (!rep || !*rep) {
			ERR("scandir for %s: %s", dir, strerror(errno));
			if (rep)
				*rep = 1;
		}
		return 0;
	} else {
		if (n == 0)
			return 0;

		strncpy(s, namelist[0]->d_name, 255);
		s[255] = '\0';

		for (i = 0; i < n; i++)
			free(namelist[i]);
		free(namelist);

		return 1;
	}
}

#define I2C_DIR "/sys/bus/i2c/devices/"

int
open_i2c_sensor(const char *dev, const char *type, int n, int *div,
		char *devtype)
{
	char path[256];
	char buf[256];
	int fd;
	int divfd;

	/* if i2c device is NULL or *, get first */
	if (dev == NULL || strcmp(dev, "*") == 0) {
		static int rep;
		if (!get_first_file_in_a_directory(I2C_DIR, buf, &rep))
			return -1;
		dev = buf;
	}

	/* change vol to in */
	if (strcmp(type, "vol") == 0)
		type = "in";

	if (strcmp(type, "tempf") == 0) {
		snprintf(path, 255, I2C_DIR "%s/%s%d_input", dev, "temp", n);
	} else {
		snprintf(path, 255, I2C_DIR "%s/%s%d_input", dev, type, n);
	}
	strncpy(devtype, path, 255);

	/* open file */
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		CRIT_ERR("can't open '%s': %s\nplease fix i2c or remove it from Conky", path, strerror(errno));
	}

	if (strcmp(type, "in") == 0 || strcmp(type, "temp") == 0
	    || strcmp(type, "tempf") == 0)
		*div = 1;
	else
		*div = 0;
	/* fan does not use *_div as a read divisor */
	if (strcmp("fan", type) == 0)
		return fd;

	/* test if *_div file exist, open it and use it as divisor */
	if (strcmp(type, "tempf") == 0) {
		snprintf(path, 255, I2C_DIR "%s/%s%d_div", "one", "two",
			 n);
	} else {
		snprintf(path, 255, I2C_DIR "%s/%s%d_div", dev, type, n);
	}

	divfd = open(path, O_RDONLY);
	if (divfd > 0) {
		/* read integer */
		char divbuf[64];
		unsigned int divn;
		divn = read(divfd, divbuf, 63);
		/* should read until n == 0 but I doubt that kernel will give these
		 * in multiple pieces. :) */
		divbuf[divn] = '\0';
		*div = atoi(divbuf);
	}

	close(divfd);

	return fd;
}

double get_i2c_info(int *fd, int div, char *devtype, char *type)
{
	int val = 0;

	if (*fd <= 0)
		return 0;

	lseek(*fd, 0, SEEK_SET);

	/* read integer */
	{
		char buf[64];
		unsigned int n;
		n = read(*fd, buf, 63);
		/* should read until n == 0 but I doubt that kernel will give these
		 * in multiple pieces. :) */
		buf[n] = '\0';
		val = atoi(buf);
	}

	close(*fd);
	/* open file */
	*fd = open(devtype, O_RDONLY);
	if (*fd < 0)
		ERR("can't open '%s': %s", devtype, strerror(errno));

	/* My dirty hack for computing CPU value 
	 * Filedil, from forums.gentoo.org
	 */
/*	if (strstr(devtype, "temp1_input") != NULL)
	return -15.096+1.4893*(val / 1000.0); */


	/* divide voltage and temperature by 1000 */
	/* or if any other divisor is given, use that */
	if (strcmp(type, "tempf") == 0) {
		if (div > 1)
			return ((val / div + 40) * 9.0 / 5) - 40;
		else if (div)
			return ((val / 1000.0 + 40) * 9.0 / 5) - 40;
		else
			return ((val + 40) * 9.0 / 5) - 40;
	} else {
		if (div > 1)
			return val / div;
		else if (div)
			return val / 1000.0;
		else
			return val;
	}
}

#define ADT746X_FAN "/sys/devices/temperatures/cpu_fan_speed"

static char *adt746x_fan_state;

char *get_adt746x_fan()
{
	static int rep;
	FILE *fp;

	if (adt746x_fan_state == NULL) {
		adt746x_fan_state = (char *) malloc(100);
		assert(adt746x_fan_state != NULL);
	}

	fp = open_file(ADT746X_FAN, &rep);
	if (!fp) {
		strcpy(adt746x_fan_state,
		       "No fan found! Hey, you don't have one?");
		return adt746x_fan_state;
	}
	fscanf(fp, "%s", adt746x_fan_state);
	fclose(fp);

	return adt746x_fan_state;
}

#define ADT746X_CPU "/sys/devices/temperatures/cpu_temperature"

static char *adt746x_cpu_state;

char *get_adt746x_cpu()
{
	static int rep;
	FILE *fp;

	if (adt746x_cpu_state == NULL) {
		adt746x_cpu_state = (char *) malloc(100);
		assert(adt746x_cpu_state != NULL);
	}

	fp = open_file(ADT746X_CPU, &rep);
	fscanf(fp, "%2s", adt746x_cpu_state);
	fclose(fp);

	return adt746x_cpu_state;
}

/* Thanks to "Walt Nelson" <wnelsonjr@comcast.net> */

/***********************************************************************/
/*
 *  This file is part of x86info.
 *  (C) 2001 Dave Jones.
 *
 *  Licensed under the terms of the GNU GPL License version 2.
 *
 * Estimate CPU MHz routine by Andrea Arcangeli <andrea@suse.de>
 * Small changes by David Sterba <sterd9am@ss1000.ms.mff.cuni.cz>
 *
 */
#if  defined(__i386) || defined(__x86_64)
__inline__ unsigned long long int rdtsc()
{
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31":"=A" (x));
	return x;
}
static char *buffer = NULL;
#endif

float get_freq_dynamic()
{
#if  defined(__i386) || defined(__x86_64)
	if (buffer == NULL)
		buffer = malloc(64);
	struct timezone tz;
	struct timeval tvstart, tvstop;
	unsigned long long cycles[2];	/* gotta be 64 bit */
	unsigned int microseconds;	/* total time taken */

	memset(&tz, 0, sizeof(tz));

	/* get this function in cached memory */
	gettimeofday(&tvstart, &tz);
	cycles[0] = rdtsc();
	gettimeofday(&tvstart, &tz);

	/* we don't trust that this is any specific length of time */
	usleep(100);
	cycles[1] = rdtsc();
	gettimeofday(&tvstop, &tz);
	microseconds = ((tvstop.tv_sec - tvstart.tv_sec) * 1000000) +
	    (tvstop.tv_usec - tvstart.tv_usec);

	return (cycles[1] - cycles[0]) / microseconds;
#else
	return get_freq();
#endif
}

#define CPUFREQ_CURRENT "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"

static char *frequency;
	
float get_freq()
{
	FILE *f;
	char s[1000];
	if (frequency == NULL) {
		frequency = (char *) malloc(100);
		assert(frequency != NULL);
	}
	f = fopen(CPUFREQ_CURRENT, "r");
	if (f) {
		/* if there's a cpufreq /sys node, read the current
		 * frequency there from this node; divice by 1000 to
		 * get MHz
		 */
		double freq = 0;
		if (fgets(s, 1000,f)) {
			s[strlen(s)-1] = '\0';
			freq = strtod(s, NULL);
		}
		fclose(f);
		return (freq/1000);
	}
	
	f = fopen("/proc/cpuinfo", "r");	//open the CPU information file
	if (!f)
	    return 0;
	while (fgets(s, 1000, f) != NULL){	//read the file
#if defined(__i386) || defined(__x86_64)
		if (strncmp(s, "cpu MHz", 5) == 0) {	//and search for the cpu mhz
#else
		if (strncmp(s, "clock", 5) == 0) {	// this is different on ppc for some reason
#endif
		strcpy(frequency, strchr(s, ':') + 2);	//copy just the number
		frequency[strlen(frequency) - 1] = '\0';	// strip \n
		break;
		}
	}
		fclose(f);
		return strtod(frequency, (char **)NULL);
}


#define ACPI_FAN_DIR "/proc/acpi/fan/"

void get_acpi_fan( char * p_client_buffer, size_t client_buffer_size )
{
	static int rep;
	char buf[256];
	char buf2[256];
	FILE *fp;

	if ( !p_client_buffer || client_buffer_size <= 0 )
		return;

	/* yeah, slow... :/ */
	if (!get_first_file_in_a_directory(ACPI_FAN_DIR, buf, &rep))
	{
		snprintf( p_client_buffer, client_buffer_size, "no fans?" );
		return;
	}

	snprintf(buf2, sizeof(buf2), "%s%s/state", ACPI_FAN_DIR, buf );

	fp = open_file(buf2, &rep);
	if (!fp) {
		snprintf( p_client_buffer, client_buffer_size, "can't open fan's state file" );
		return;
	}
	memset(buf,0,sizeof(buf));
	fscanf(fp, "%*s %99s", buf);
	fclose(fp);

	snprintf( p_client_buffer, client_buffer_size, "%s", buf );

	return;
}

#define ACPI_AC_ADAPTER_DIR "/proc/acpi/ac_adapter/"

void get_acpi_ac_adapter( char * p_client_buffer, size_t client_buffer_size )
{
	static int rep;
	char buf[256];
	char buf2[256];
	FILE *fp;

	if ( !p_client_buffer || client_buffer_size <= 0 )
		return;

	/* yeah, slow... :/ */
	if (!get_first_file_in_a_directory(ACPI_AC_ADAPTER_DIR, buf, &rep))
	{
		snprintf( p_client_buffer, client_buffer_size, "no ac_adapters?" );
		return;	
	}

	snprintf(buf2, sizeof(buf2), "%s%s/state", ACPI_AC_ADAPTER_DIR, buf );
	 

	fp = open_file(buf2, &rep);
	if (!fp) {
		snprintf( p_client_buffer, client_buffer_size, "No ac adapter found.... where is it?" );
		return;
	}
	memset(buf,0,sizeof(buf));
	fscanf(fp, "%*s %99s", buf );
	fclose(fp);

	snprintf( p_client_buffer, client_buffer_size, "%s", buf );

	return;
}

/*
/proc/acpi/thermal_zone/THRM/cooling_mode
cooling mode:            active
/proc/acpi/thermal_zone/THRM/polling_frequency
<polling disabled>
/proc/acpi/thermal_zone/THRM/state
state:                   ok
/proc/acpi/thermal_zone/THRM/temperature
temperature:             45 C
/proc/acpi/thermal_zone/THRM/trip_points
critical (S5):           73 C
passive:                 73 C: tc1=4 tc2=3 tsp=40 devices=0xcdf6e6c0
*/

#define ACPI_THERMAL_DIR "/proc/acpi/thermal_zone/"
#define ACPI_THERMAL_FORMAT "/proc/acpi/thermal_zone/%s/temperature"

int open_acpi_temperature(const char *name)
{
	char path[256];
	char buf[256];
	int fd;

	if (name == NULL || strcmp(name, "*") == 0) {
		static int rep;
		if (!get_first_file_in_a_directory
		    (ACPI_THERMAL_DIR, buf, &rep))
			return -1;
		name = buf;
	}

	snprintf(path, 255, ACPI_THERMAL_FORMAT, name);

	fd = open(path, O_RDONLY);
	if (fd < 0)
		ERR("can't open '%s': %s", path, strerror(errno));

	return fd;
}

static double last_acpi_temp;
static double last_acpi_temp_time;

double get_acpi_temperature(int fd)
{
	if (fd <= 0)
		return 0;

	/* don't update acpi temperature too often */
	if (current_update_time - last_acpi_temp_time < 11.32) {
		return last_acpi_temp;
	}
	last_acpi_temp_time = current_update_time;

	/* seek to beginning */
	lseek(fd, 0, SEEK_SET);

	/* read */
	{
		char buf[256];
		int n;
		n = read(fd, buf, 255);
		if (n < 0)
			ERR("can't read fd %d: %s", fd, strerror(errno));
		else {
			buf[n] = '\0';
			sscanf(buf, "temperature: %lf", &last_acpi_temp);
		}
	}

	return last_acpi_temp;
}

/*
hipo@lepakko hipo $ cat /proc/acpi/battery/BAT1/info 
present:                 yes
design capacity:         4400 mAh
last full capacity:      4064 mAh
battery technology:      rechargeable
design voltage:          14800 mV
design capacity warning: 300 mAh
design capacity low:     200 mAh
capacity granularity 1:  32 mAh
capacity granularity 2:  32 mAh
model number:            02KT
serial number:           16922
battery type:            LION
OEM info:                SANYO
*/

/*
hipo@lepakko conky $ cat /proc/acpi/battery/BAT1/state
present:                 yes
capacity state:          ok
charging state:          unknown
present rate:            0 mA
remaining capacity:      4064 mAh
present voltage:         16608 mV
*/

/*
2213<@jupet kellari ö> jupet@lagi-unstable:~$ cat /proc/apm 
2213<@jupet kellari ö> 1.16 1.2 0x03 0x01 0xff 0x10 -1% -1 ?
2213<@jupet kellari ö> (-1 ollee ei akkua kiinni, koska akku on pöydällä)
2214<@jupet kellari ö> jupet@lagi-unstable:~$ cat /proc/apm 
2214<@jupet kellari ö> 1.16 1.2 0x03 0x01 0x03 0x09 98% -1 ?

2238<@jupet kellari ö> 1.16 1.2 0x03 0x00 0x00 0x01 100% -1 ? ilman verkkovirtaa
2239<@jupet kellari ö> 1.16 1.2 0x03 0x01 0x00 0x01 99% -1 ? verkkovirralla

2240<@jupet kellari ö> 1.16 1.2 0x03 0x01 0x03 0x09 100% -1 ? verkkovirralla ja monitori päällä
2241<@jupet kellari ö> 1.16 1.2 0x03 0x00 0x00 0x01 99% -1 ? monitori päällä mutta ilman verkkovirtaa
*/

#define ACPI_BATTERY_BASE_PATH "/proc/acpi/battery"
#define APM_PATH "/proc/apm"

static FILE *acpi_bat_fp;
static FILE *apm_bat_fp;

static int acpi_last_full;

static char last_battery_str[64];

static double last_battery_time;

void get_battery_stuff(char *buf, unsigned int n, const char *bat)
{
	static int rep, rep2;
	char acpi_path[128];
	snprintf(acpi_path, 127, ACPI_BATTERY_BASE_PATH "/%s/state", bat);

	/* don't update battery too often */
	if (current_update_time - last_battery_time < 29.5) {
		snprintf(buf, n, "%s", last_battery_str);
		return;
	}
	last_battery_time = current_update_time;

	/* first try ACPI */

	if (acpi_bat_fp == NULL && apm_bat_fp == NULL)
		acpi_bat_fp = open_file(acpi_path, &rep);

	if (acpi_bat_fp != NULL) {
		int present_rate = -1;
		int remaining_capacity = -1;
		char charging_state[64];

		/* read last full capacity if it's zero */
		if (acpi_last_full == 0) {
			static int rep;
			char path[128];
			FILE *fp;
			snprintf(path, 127,
				 ACPI_BATTERY_BASE_PATH "/%s/info", bat);
			fp = open_file(path, &rep);
			if (fp != NULL) {
				while (!feof(fp)) {
					char b[256];
					if (fgets(b, 256, fp) == NULL)
						break;

					if (sscanf
					    (b, "last full capacity: %d",
					     &acpi_last_full) != 0)
						break;
				}

				fclose(fp);
			}
		}

		fseek(acpi_bat_fp, 0, SEEK_SET);

		strcpy(charging_state, "unknown");

		while (!feof(acpi_bat_fp)) {
			char buf[256];
			if (fgets(buf, 256, acpi_bat_fp) == NULL)
				break;

			/* let's just hope units are ok */
			if (buf[0] == 'c')
				sscanf(buf, "charging state: %63s",
				       charging_state);
			else if (buf[0] == 'p')
				sscanf(buf, "present rate: %d",
				       &present_rate);
			else if (buf[0] == 'r')
				sscanf(buf, "remaining capacity: %d",
				       &remaining_capacity);
		}

		/* charging */
		if (strcmp(charging_state, "charging") == 0) {
			if (acpi_last_full != 0 && present_rate > 0) {
				strcpy(last_battery_str, "charging ");
				format_seconds(last_battery_str + 9,
					       63 - 9,
					       (acpi_last_full -
						remaining_capacity) * 60 *
					       60 / present_rate);
			} else if (acpi_last_full != 0
				   && present_rate <= 0) {
				sprintf(last_battery_str, "charging %d%%",
					remaining_capacity * 100 /
					acpi_last_full);
			} else {
				strcpy(last_battery_str, "charging");
			}
		}
		/* discharging */
		else if (strcmp(charging_state, "discharging") == 0) {
			if (present_rate > 0)
				format_seconds(last_battery_str, 63,
					       (remaining_capacity * 60 *
						60) / present_rate);
			else
				sprintf(last_battery_str,
					"discharging %d%%",
					remaining_capacity * 100 /
					acpi_last_full);
		}
		/* charged */
		/* thanks to Lukas Zapletal <lzap@seznam.cz> */
		else if (strcmp(charging_state, "charged") == 0) {
			if (acpi_last_full != 0
			    && remaining_capacity != acpi_last_full)
				sprintf(last_battery_str, "charged %d%%",
					remaining_capacity * 100 /
					acpi_last_full);
			else
				strcpy(last_battery_str, "charged");
		}
		/* unknown, probably full / AC */
		else {
			if (acpi_last_full != 0
			    && remaining_capacity != acpi_last_full)
				sprintf(last_battery_str, "unknown %d%%",
					remaining_capacity * 100 /
					acpi_last_full);
			else
				strcpy(last_battery_str, "AC");
		}
	} else {
		/* APM */
		if (apm_bat_fp == NULL)
			apm_bat_fp = open_file(APM_PATH, &rep2);

		if (apm_bat_fp != NULL) {
			int ac, status, flag, life;

			fscanf(apm_bat_fp,
			       "%*s %*s %*x %x   %x       %x     %d%%",
			       &ac, &status, &flag, &life);

			if (life == -1) {
				/* could check now that there is ac */
				snprintf(last_battery_str, 64, "AC");
			} else if (ac && life != 100) {	/* could check that status==3 here? */
				snprintf(last_battery_str, 64,
					 "charging %d%%", life);
			} else {
				snprintf(last_battery_str, 64, "%d%%",
					 life);
			}

			/* it seemed to buffer it so file must be closed (or could use syscalls
			 * directly but I don't feel like coding it now) */
			fclose(apm_bat_fp);
			apm_bat_fp = NULL;
		}
	}

	snprintf(buf, n, "%s", last_battery_str);
}

void update_top()
{
	show_nice_processes = 1;
	process_find_top(info.cpu, info.memu);
	info.first_process = get_first_process();
}


/*
 *  The following ifdefs were adapted from gkrellm
 */
#include <linux/major.h>

#if ! defined (MD_MAJOR)
#define MD_MAJOR 9
#endif

#if !defined(LVM_BLK_MAJOR)
#define LVM_BLK_MAJOR 58
#endif

#if !defined(NBD_MAJOR)
#define NBD_MAJOR 43
#endif

void update_diskio()
{
	static unsigned int last = UINT_MAX;
	static FILE* fp;

	char buf[512];
	int major, minor;
	unsigned int current = 0;
	unsigned int reads, writes = 0;
	int col_count = 0;

	if (!fp) {
		fp = fopen("/proc/diskstats", "r");
	} else {
		fseek(fp, 0, SEEK_SET);
	}

	/* read reads and writes from all disks (minor = 0), including
	 * cd-roms and floppies, and summ them up
	 */
	current = 0;
	while (!feof(fp)) {
		fgets(buf, 512, fp);
		col_count = sscanf(buf, "%u %u %*s %*u %*u %u %*u %*u %*u %u",
				   &major, &minor, &reads, &writes);
		/* ignore subdevices (they have only 3 matching entries in their line)
		 * and virtual devices (LVM, network block devices, RAM disks, Loopback)
		 *
		 * XXX ignore devices which are part of a SW RAID (MD_MAJOR)
		 */
		if (col_count > 3 &&
		    major != LVM_BLK_MAJOR && major != NBD_MAJOR &&
		    major != RAMDISK_MAJOR && major != LOOP_MAJOR) {
			current += reads + writes;
		}
	}

	/* since the values in /proc/diststats are absolute, we have
	 * to substract our last reading. The numbers stand for
	 * "sectors read", and we therefore have to divide by two to
	 * get KB */
	int tot = ((double)(current-last)/2);
	if (last > current) {
		/* we hit this either if it's the very first time we
                 * run this, or when /proc/diskstats overflows; while
                 * 0 is not correct, it's at least not way off */
		tot = 0;
	}
	last = current;

	diskio_value = tot;
}

