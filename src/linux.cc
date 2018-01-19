/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2007 Toni Spets
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

#include "conky.h"
#include "logging.h"
#include "common.h"
#include "linux.h"
#include "net_stat.h"
#include "diskio.h"
#include "temphelper.h"
#include "proc.h"
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#ifndef HAVE_CLOCK_GETTIME
#include <sys/time.h>
#endif
#include <fcntl.h>
#include <unistd.h>
// #include <assert.h>
#include <time.h>
#include <unordered_map>
#include "setting.hh"
#include "top.h"

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <arpa/inet.h>
#ifdef _NET_IF_H
#define _LINUX_IF_H
#endif
#include <linux/route.h>
#include <math.h>
#include <pthread.h>

/* The following ifdefs were adapted from gkrellm */
#include <linux/major.h>

#if !defined(MD_MAJOR)
#define MD_MAJOR 9
#endif

#if !defined(LVM_BLK_MAJOR)
#define LVM_BLK_MAJOR 58
#endif

#if !defined(NBD_MAJOR)
#define NBD_MAJOR 43
#endif

#if !defined(DM_MAJOR)
#define DM_MAJOR 253
#endif

#ifdef BUILD_WLAN
#include <iwlib.h>
#endif

struct sysfs {
	int fd;
	int arg;
	char devtype[256];
	char type[64];
	float factor, offset;
};

#define SHORTSTAT_TEMPL "%*s %llu %llu %llu"
#define LONGSTAT_TEMPL "%*s %llu %llu %llu "

static conky::simple_config_setting<bool> top_cpu_separate("top_cpu_separate", false, true);

/* This flag tells the linux routines to use the /proc system where possible,
 * even if other api's are available, e.g. sysinfo() or getloadavg().
 * the reason for this is to allow for /proc-based distributed monitoring.
 * using a flag in this manner creates less confusing code. */
static int prefer_proc = 0;

void prepare_update(void)
{
}

int update_uptime(void)
{
#ifdef HAVE_SYSINFO
	if (!prefer_proc) {
		struct sysinfo s_info;

		sysinfo(&s_info);
		info.uptime = (double) s_info.uptime;
	} else
#endif
	{
		static int rep = 0;
		FILE *fp;

		if (!(fp = open_file("/proc/uptime", &rep))) {
			info.uptime = 0.0;
			return 0;
		}
		if (fscanf(fp, "%lf", &info.uptime) <= 0)
			info.uptime = 0;
		fclose(fp);
	}
	return 0;
}

int check_mount(struct text_object *obj)
{
	int ret = 0;
	FILE *mtab;

	if (!obj->data.s)
		return 0;

	if ((mtab = fopen("/proc/mounts", "r"))) {
		char buf1[256], buf2[129];

		while (fgets(buf1, 256, mtab)) {
			sscanf(buf1, "%*s %128s", buf2);
			if (!strcmp(obj->data.s, buf2)) {
				ret = 1;
				break;
			}
		}
		fclose(mtab);
	} else {
		NORM_ERR("Could not open mtab");
	}
	return ret;
}

/* these things are also in sysinfo except Buffers:
 * (that's why I'm reading them from proc) */

int update_meminfo(void)
{
	FILE *meminfo_fp;
	static int rep = 0;

	/* unsigned int a; */
	char buf[256];

	unsigned long long shmem = 0, sreclaimable = 0;

	info.mem = info.memwithbuffers = info.memmax = info.memdirty = info.swap = info.swapfree = info.swapmax =
        info.bufmem = info.buffers = info.cached = info.memfree = info.memeasyfree = 0;

	if (!(meminfo_fp = open_file("/proc/meminfo", &rep))) {
		return 0;
	}

	while (!feof(meminfo_fp)) {
		if (fgets(buf, 255, meminfo_fp) == NULL) {
			break;
		}

		if (strncmp(buf, "MemTotal:", 9) == 0) {
			sscanf(buf, "%*s %llu", &info.memmax);
		} else if (strncmp(buf, "MemFree:", 8) == 0) {
			sscanf(buf, "%*s %llu", &info.memfree);
		} else if (strncmp(buf, "SwapTotal:", 10) == 0) {
			sscanf(buf, "%*s %llu", &info.swapmax);
		} else if (strncmp(buf, "SwapFree:", 9) == 0) {
			sscanf(buf, "%*s %llu", &info.swapfree);
		} else if (strncmp(buf, "Buffers:", 8) == 0) {
			sscanf(buf, "%*s %llu", &info.buffers);
		} else if (strncmp(buf, "Cached:", 7) == 0) {
			sscanf(buf, "%*s %llu", &info.cached);
		} else if (strncmp(buf, "Dirty:", 6) == 0) {
			sscanf(buf, "%*s %llu", &info.memdirty);
		} else if (strncmp(buf, "Shmem:", 6) == 0) {
			sscanf(buf, "%*s %llu", &shmem);
		} else if (strncmp(buf, "SReclaimable:", 13) == 0) {
			sscanf(buf, "%*s %llu", &sreclaimable);
		}
	}

	info.mem = info.memwithbuffers = info.memmax - info.memfree;
	info.memeasyfree = info.memfree;
	info.swap = info.swapmax - info.swapfree;

	/* Reclaimable memory: does not include shared memory, which is part of cached but unreclaimable.
	   Includes the reclaimable part of the Slab cache though.
	   Note: when shared memory is swapped out, shmem decreases and swapfree decreases - we want this.
	*/
	info.bufmem = (info.cached - shmem) + info.buffers + sreclaimable;

	/* Now (info.mem - info.bufmem) is the *really used* (aka unreclaimable) memory.
	   When this value reaches the size of the physical RAM, and swap is full or non-present, OOM happens.
	   Therefore this is the value users want to monitor, regarding their RAM.
	*/

	fclose(meminfo_fp);
	return 0;
}

void print_laptop_mode(struct text_object *obj, char *p, int p_max_size)
{
	FILE *fp;
	int val = -1;

	(void)obj;

	if ((fp = fopen("/proc/sys/vm/laptop_mode", "r")) != NULL) {
		if (fscanf(fp, "%d\n", &val) <= 0)
			val = 0;
		fclose(fp);
	}
	snprintf(p, p_max_size, "%d", val);
}

/* my system says:
 * # cat /sys/block/sda/queue/scheduler
 * noop [anticipatory] cfq
 */
void print_ioscheduler(struct text_object *obj, char *p, int p_max_size)
{
	FILE *fp;
	char buf[128];

	if (!obj->data.s)
		goto out_fail;

	snprintf(buf, 127, "/sys/block/%s/queue/scheduler", obj->data.s);
	if ((fp = fopen(buf, "r")) == NULL)
		goto out_fail;

	while (fscanf(fp, "%127s", buf) == 1) {
		if (buf[0] == '[') {
			buf[strlen(buf) - 1] = '\0';
			snprintf(p, p_max_size, "%s", buf + 1);
			fclose(fp);
			return;
		}
	}
	fclose(fp);
out_fail:
	snprintf(p, p_max_size, "n/a");
	return;
}

static struct {
	char *iface;
	char *ip;
	int count;
} gw_info;

#define SAVE_SET_STRING(x, y) \
	if (x && strcmp((char *)x, (char *)y)) { \
		free(x); \
		x = strndup("multiple", text_buffer_size.get(*state)); \
	} else if (!x) { \
		x = strndup(y, text_buffer_size.get(*state)); \
	}

void update_gateway_info_failure(const char *reason)
{
	if(reason != NULL) {
		perror(reason);
	}
	//2 pointers to 1 location causes a crash when we try to free them both
	gw_info.iface = strndup("failed", text_buffer_size.get(*state));
	gw_info.ip = strndup("failed", text_buffer_size.get(*state));
}


/* Iface Destination Gateway Flags RefCnt Use Metric Mask MTU Window IRTT */
#define RT_ENTRY_FORMAT "%63s %lx %lx %x %*d %*d %*d %lx %*d %*d %*d\n"

int update_gateway_info(void)
{
	FILE *fp;
	struct in_addr ina;
	char iface[64];
	unsigned long dest, gate, mask;
	unsigned int flags;

	free_and_zero(gw_info.iface);
	free_and_zero(gw_info.ip);
	gw_info.count = 0;

	if ((fp = fopen("/proc/net/route", "r")) == NULL) {
		update_gateway_info_failure("fopen()");
		return 0;
	}

	/* skip over the table header line, which is always present */
	if (fscanf(fp, "%*[^\n]\n") < 0) {
		fclose(fp);
		return 0;
	}

	while (!feof(fp)) {
		if(fscanf(fp, RT_ENTRY_FORMAT,
			  iface, &dest, &gate, &flags, &mask) != 5) {
			update_gateway_info_failure("fscanf()");
			break;
		}
		if (!(dest || mask) && ((flags & RTF_GATEWAY) || !gate) ) {
			gw_info.count++;
			SAVE_SET_STRING(gw_info.iface, iface)
			ina.s_addr = gate;
			SAVE_SET_STRING(gw_info.ip, inet_ntoa(ina))
		}
	}
	fclose(fp);
	return 0;
}

void free_gateway_info(struct text_object *obj)
{
	(void)obj;

	free_and_zero(gw_info.iface);
	free_and_zero(gw_info.ip);
	memset(&gw_info, 0, sizeof(gw_info));
}

int gateway_exists(struct text_object *obj)
{
	(void)obj;
	return !!gw_info.count;
}

void print_gateway_iface(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;

	snprintf(p, p_max_size, "%s", gw_info.iface);
}

void print_gateway_ip(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;

	snprintf(p, p_max_size, "%s", gw_info.ip);
}


/**
 * Parses information from /proc/net/dev and stores them in ???
 *
 * For the output format of /proc/net/dev @see http://linux.die.net/man/5/proc
 *
 * @return always returns 0. May change in the future, e.g. returning non zero
 * if some error happened
 **/
int update_net_stats(void)
{
	FILE *net_dev_fp;
	static int rep = 0;
	/* variably to notify the parts averaging the download speed, that this
	 * is the first call ever to this function. This variable can't be used
	 * to decide if this is the first time an interface was parsed as there
	 * are many interfaces, which can be activated and deactivated at arbitrary
	 * times */
	static char first = 1;

	// FIXME: arbitrary size chosen to keep code simple.
	int i, i2;
	unsigned int curtmp1, curtmp2;
	unsigned int k;
	struct ifconf conf;
	char buf[256];
	double delta;

#ifdef BUILD_WLAN
	// wireless info variables
	int skfd, has_bitrate = 0;
	struct wireless_info *winfo;
	struct iwreq wrq;
#endif

	/* get delta */
	delta = current_update_time - last_update_time;
	if (delta <= 0.0001) {
		return 0;
	}

	/* open file /proc/net/dev. If not something went wrong, clear all
	 * network statistics */
	if (!(net_dev_fp = open_file("/proc/net/dev", &rep))) {
		clear_net_stats();
		return 0;
	}
	/* ignore first two header lines in file /proc/net/dev. If somethings
	 * goes wrong, e.g. end of file reached, quit.
	 * (Why isn't clear_net_stats called for this case ??? */
	if (!fgets(buf, 255, net_dev_fp) ||  /* garbage */
	    !fgets(buf, 255, net_dev_fp)) {  /* garbage (field names) */
		fclose(net_dev_fp);
		return 0;
	}

	/* read each interface */
	for (i2 = 0; i2 < MAX_NET_INTERFACES; i2++) {
		struct net_stat *ns;
		char *s, *p;
		char temp_addr[18];
		long long r, t, last_recv, last_trans;

		/* quit only after all non-header lines from /proc/net/dev parsed */
		if (fgets(buf, 255, net_dev_fp) == NULL) {
			break;
		}
		p = buf;
		/* change char * p to first non-space character, which is the beginning
		 * of the interface name */
		while (*p != '\0' && isspace((int) *p)) {
			p++;
		}

		s = p;

		/* increment p until the end of the interface name has been reached */
		while (*p != '\0' && *p != ':') {
			p++;
		}
		if (*p == '\0') {
			continue;
		}
		/* replace ':' with '\0' in output of /proc/net/dev */
		*p = '\0';
		p++;

		/* get pointer to interface statistics with the interface name in s */
		ns = get_net_stat(s, NULL, NULL);
		ns->up = 1;
		memset(&(ns->addr.sa_data), 0, 14);

		memset(ns->addrs, 0, 17 * MAX_NET_INTERFACES + 1); /* Up to 17 chars per ip, max MAX_NET_INTERFACES interfaces. Nasty memory usage... */

		/* bytes packets errs drop fifo frame compressed multicast|bytes ... */
		sscanf(p, "%lld  %*d     %*d  %*d  %*d  %*d   %*d        %*d       %lld",
			&r, &t);

		/* if the interface is parsed the first time, then set recv and trans
		 * to currently received, meaning the change in network traffic is 0 */
		if (ns->last_read_recv == -1) {
			ns->recv = r;
			first = 1;
			ns->last_read_recv = r;
		}
		if (ns->last_read_trans == -1) {
			ns->trans = t;
			first = 1;
			ns->last_read_trans = t;
		}
		/* move current traffic statistic to last thereby obsoleting the
		 * current statistic */
		last_recv  = ns->recv;
		last_trans = ns->trans;

		/* If recv or trans is less than last time, an overflow happened.
		 * In that case set the last traffic to the current one, don't set
		 * it to 0, else a spike in the download and upload speed will occur! */
		if (r < ns->last_read_recv) {
			last_recv = r;
		} else {
			ns->recv += (r - ns->last_read_recv);
		}
		ns->last_read_recv = r;

		if (t < ns->last_read_trans) {
			last_trans = t;
		} else {
			ns->trans += (t - ns->last_read_trans);
		}
		ns->last_read_trans = t;

		/*** ip addr patch ***/
		i = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

		conf.ifc_buf = (char*)malloc(sizeof(struct ifreq) * MAX_NET_INTERFACES);
		conf.ifc_len = sizeof(struct ifreq) * MAX_NET_INTERFACES;
		memset(conf.ifc_buf, 0, conf.ifc_len);

		ioctl((long) i, SIOCGIFCONF, &conf);

		for (k = 0; k < conf.ifc_len / sizeof(struct ifreq); k++) {
			struct net_stat *ns2;

			if (!(((struct ifreq *) conf.ifc_buf) + k))
				break;

			ns2 = get_net_stat(
					((struct ifreq *) conf.ifc_buf)[k].ifr_ifrn.ifrn_name, NULL, NULL);
			ns2->addr = ((struct ifreq *) conf.ifc_buf)[k].ifr_ifru.ifru_addr;
			sprintf(temp_addr, "%u.%u.%u.%u, ",
					ns2->addr.sa_data[2] & 255,
					ns2->addr.sa_data[3] & 255,
					ns2->addr.sa_data[4] & 255,
					ns2->addr.sa_data[5] & 255);
			if(NULL == strstr(ns2->addrs, temp_addr))
				strncpy(ns2->addrs + strlen(ns2->addrs), temp_addr, 17);
		}

		close((long) i);

		free(conf.ifc_buf);
		/*** end ip addr patch ***/

		if (!first) {
			/* calculate instantenous speeds */
			ns->net_rec  [0] = (ns->recv  - last_recv ) / delta;
			ns->net_trans[0] = (ns->trans - last_trans) / delta;
		}

		curtmp1 = 0;
		curtmp2 = 0;
		/* get an average over the last speed samples */
		int samples = net_avg_samples.get(*state);
		/* is OpenMP actually useful here? How large is samples? > 1000 ? */
#ifdef HAVE_OPENMP
#pragma omp parallel for reduction(+:curtmp1, curtmp2) schedule(dynamic,10)
#endif /* HAVE_OPENMP */
		for (i = 0; i < samples; i++) {
			curtmp1 = curtmp1 + ns->net_rec[i];
			curtmp2 = curtmp2 + ns->net_trans[i];
		}
		ns->recv_speed = curtmp1 / (double) samples;
		ns->trans_speed = curtmp2 / (double) samples;
		if (samples > 1) {
#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic,10)
#endif /* HAVE_OPENMP */
			for (i = samples; i > 1; i--) {
				ns->net_rec[i - 1] = ns->net_rec[i - 2];
				ns->net_trans[i - 1] = ns->net_trans[i - 2];
			}
		}

#ifdef BUILD_WLAN
		/* update wireless info */
		winfo = (struct wireless_info *) malloc(sizeof(struct wireless_info));
		memset(winfo, 0, sizeof(struct wireless_info));

		skfd = iw_sockets_open();
		if (iw_get_basic_config(skfd, s, &(winfo->b)) > -1) {

			// set present winfo variables
			if (iw_get_range_info(skfd, s, &(winfo->range)) >= 0) {
				winfo->has_range = 1;
			}
			if (iw_get_stats(skfd, s, &(winfo->stats),
					&winfo->range, winfo->has_range) >= 0) {
				winfo->has_stats = 1;
			}
			if (iw_get_ext(skfd, s, SIOCGIWAP, &wrq) >= 0) {
				winfo->has_ap_addr = 1;
				memcpy(&(winfo->ap_addr), &(wrq.u.ap_addr), sizeof(sockaddr));
			}

			// get bitrate
			if (iw_get_ext(skfd, s, SIOCGIWRATE, &wrq) >= 0) {
				memcpy(&(winfo->bitrate), &(wrq.u.bitrate), sizeof(iwparam));
				iw_print_bitrate(ns->bitrate, 16, winfo->bitrate.value);
				has_bitrate = 1;
			}

			// get link quality
			if (winfo->has_range && winfo->has_stats
					&& ((winfo->stats.qual.level != 0)
					|| (winfo->stats.qual.updated & IW_QUAL_DBM))) {
				if (!(winfo->stats.qual.updated & IW_QUAL_QUAL_INVALID)) {
					ns->link_qual = winfo->stats.qual.qual;
					ns->link_qual_max = winfo->range.max_qual.qual;
				}
			}

			// get ap mac
			if (winfo->has_ap_addr) {
				iw_sawap_ntop(&winfo->ap_addr, ns->ap);
			}

			// get essid
			if (winfo->b.has_essid) {
				if (winfo->b.essid_on) {
					snprintf(ns->essid, 32, "%s", winfo->b.essid);
				} else {
					snprintf(ns->essid, 32, "off/any");
				}
			}
			// get channel and freq
			if (winfo->b.has_freq) {
				if(winfo->has_range == 1) {
					ns->channel = iw_freq_to_channel(winfo->b.freq, &(winfo->range));
					iw_print_freq_value(ns->freq, 16, winfo->b.freq);
				} else {
					ns->channel = 0;
					ns->freq[0] = 0;
				}
			}

			snprintf(ns->mode, 16, "%s", iw_operation_mode[winfo->b.mode]);
		}
		iw_sockets_close(skfd);
		free(winfo);
#endif
	}

#ifdef BUILD_IPV6
	FILE *file;
	char v6addr[33];
	char devname[21];
	unsigned int netmask, scope;
	struct net_stat *ns;
	struct v6addr *lastv6;
	//remove the old v6 addresses otherwise they are listed multiple times
	for (unsigned int i = 0; i < MAX_NET_INTERFACES; i++) {
		ns = &netstats[i];
		while(ns->v6addrs != NULL) {
			lastv6 = ns->v6addrs;
			ns->v6addrs = ns->v6addrs->next;
			free(lastv6);
		}
	}
	if ((file = fopen(PROCDIR"/net/if_inet6", "r")) != NULL) {
		while (fscanf(file, "%32s %*02x %02x %02x %*02x %20s\n", v6addr, &netmask, &scope, devname) != EOF) {
			ns = get_net_stat(devname, NULL, NULL);
			if(ns->v6addrs == NULL) {
				lastv6 = (struct v6addr *) malloc(sizeof(struct v6addr));
				ns->v6addrs = lastv6;
			} else {
				lastv6 = ns->v6addrs;
				while(lastv6->next) lastv6 = lastv6->next;
				lastv6->next = (struct v6addr *) malloc(sizeof(struct v6addr));
				lastv6 = lastv6->next;
			}
			for(int i=0; i<16; i++)
				sscanf(v6addr+2*i, "%2hhx", &(lastv6->addr.s6_addr[i]));
			lastv6->netmask = netmask;
			switch(scope) {
			case 0:	//global
				lastv6->scope = 'G';
				break;
			case 16:	//host-local
				lastv6->scope = 'H';
				break;
			case 32:	//link-local
				lastv6->scope = 'L';
				break;
			case 64:	//site-local
				lastv6->scope = 'S';
				break;
			case 128:	//compat
				lastv6->scope = 'C';
				break;
			default:
				lastv6->scope = '?';
			}
			lastv6->next = NULL;
		}
		fclose(file);
	}
#endif /* BUILD_IPV6 */

	first = 0;

	fclose(net_dev_fp);
	return 0;
}

int result;

int update_total_processes(void)
{
	DIR *dir;
	struct dirent *entry;
	int ignore1;
	char ignore2;

	info.procs = 0;
	if (!(dir = opendir("/proc"))) {
		return 0;
	}
	while ((entry = readdir(dir))) {
		if (!entry) {
			/* Problem reading list of processes */
			closedir(dir);
			info.procs = 0;
			return 0;
		}
		if (sscanf(entry->d_name, "%d%c", &ignore1, &ignore2) == 1) {
			info.procs++;
		}
	}
	closedir(dir);
	return 0;
}

int update_threads(void)
{
#ifdef HAVE_SYSINFO
	if (!prefer_proc) {
		struct sysinfo s_info;

		sysinfo(&s_info);
		info.threads = s_info.procs;
	} else
#endif
	{
		static int rep = 0;
		FILE *fp;

		if (!(fp = open_file("/proc/loadavg", &rep))) {
			info.threads = 0;
			return 0;
		}
		if (fscanf(fp, "%*f %*f %*f %*d/%hu", &info.threads) <= 0)
			info.threads = 0;
		fclose(fp);
	}
	return 0;
}

#define CPU_SAMPLE_COUNT 15
struct cpu_info {
	unsigned long long cpu_user;
	unsigned long long cpu_system;
	unsigned long long cpu_nice;
	unsigned long long cpu_idle;
	unsigned long long cpu_iowait;
	unsigned long long cpu_irq;
	unsigned long long cpu_softirq;
	unsigned long long cpu_steal;
	unsigned long long cpu_total;
	unsigned long long cpu_active_total;
	unsigned long long cpu_last_total;
	unsigned long long cpu_last_active_total;
	double cpu_val[CPU_SAMPLE_COUNT];
};
static short cpu_setup = 0;

/* Determine if this kernel gives us "extended" statistics information in
 * /proc/stat.
 * Kernels around 2.5 and earlier only reported user, system, nice, and
 * idle values in proc stat.
 * Kernels around 2.6 and greater report these PLUS iowait, irq, softirq,
 * and steal */
void determine_longstat(char *buf)
{
	unsigned long long iowait = 0;

	KFLAG_SETOFF(KFLAG_IS_LONGSTAT);
	/* scanf will either return -1 or 1 because there is only 1 assignment */
	if (sscanf(buf, "%*s %*d %*d %*d %*d %llu", &iowait) > 0) {
		KFLAG_SETON(KFLAG_IS_LONGSTAT);
	}
}

void determine_longstat_file(void) {
#define MAX_PROCSTAT_LINELEN 255
	FILE *stat_fp;
	static int rep = 0;
	char buf[MAX_PROCSTAT_LINELEN + 1];

	if ( not (stat_fp = open_file("/proc/stat", &rep))) return;
	while ( not feof(stat_fp)) {
		if (fgets(buf, MAX_PROCSTAT_LINELEN, stat_fp) == NULL) break;
		if (strncmp(buf, "cpu", 3) == 0) {
			determine_longstat(buf);
			break;
		}
	}
	fclose(stat_fp);
}

void get_cpu_count(void)
{
	FILE *stat_fp;
	static int rep = 0;
	char buf[256];
	char *str1, *str2, *token, *subtoken;
	char *saveptr1, *saveptr2;
	int subtoken1=-1;
	int subtoken2=-1;

	if (info.cpu_usage) {
		return;
	}

	if (!(stat_fp = open_file("/sys/devices/system/cpu/present", &rep))) {
		return;
	}

	info.cpu_count = 0;

	while (!feof(stat_fp)) {
		if (fgets(buf, 255, stat_fp) == NULL) {
			break;
		}

		// Do some parsing here to handle skipped cpu numbers.  For example,
		// for an AMD FX(tm)-6350 Six-Core Processor /sys/.../present reports
		// "0,3-7".  I assume that chip is really an 8-core die with two cores
		// disabled...  Presumably you could also get "0,3-4,6", and other
		// combos too...
		for (str1 = buf; ; str1 = NULL) {
			token = strtok_r(str1, ",", &saveptr1);
			if (token == NULL) break;
			++info.cpu_count;

			subtoken1=-1;
			subtoken2=-1;
			for (str2 = token; ; str2 = NULL) {
				subtoken = strtok_r(str2, "-", &saveptr2);
				if (subtoken == NULL) break;
				if(subtoken1 < 0)
					subtoken1=atoi(subtoken);
				else
					subtoken2=atoi(subtoken);
			}
			if(subtoken2 > 0)
				info.cpu_count += subtoken2 - subtoken1;
		}
	}
	info.cpu_usage = (float*)malloc((info.cpu_count + 1) * sizeof(float));

	fclose(stat_fp);
}

#define TMPL_LONGSTAT "%*s %llu %llu %llu %llu %llu %llu %llu %llu"
#define TMPL_SHORTSTAT "%*s %llu %llu %llu %llu"

int update_stat(void)
{
	FILE *stat_fp;
	static int rep = 0;
	static struct cpu_info *cpu = NULL;
	char buf[256];
	int i;
	unsigned int idx;
	double curtmp;
	const char *stat_template = NULL;
	unsigned int malloc_cpu_size = 0;
	extern void* global_cpu;

	static pthread_mutex_t last_stat_update_mutex = PTHREAD_MUTEX_INITIALIZER;
	static double last_stat_update = 0.0;
	float cur_total = 0.0;

	/* since we use wrappers for this function, the update machinery
	 * can't eliminate double invocations of this function. Check for
	 * them here, otherwise cpu_usage counters are freaking out. */
	pthread_mutex_lock(&last_stat_update_mutex);
	if (last_stat_update == current_update_time) {
		pthread_mutex_unlock(&last_stat_update_mutex);
		return 0;
	}
	last_stat_update = current_update_time;
	pthread_mutex_unlock(&last_stat_update_mutex);

	/* add check for !info.cpu_usage since that mem is freed on a SIGUSR1 */
	if (!cpu_setup || !info.cpu_usage) {
		get_cpu_count();
		cpu_setup = 1;
	}

	if (!stat_template) {
		stat_template =
			KFLAG_ISSET(KFLAG_IS_LONGSTAT) ? TMPL_LONGSTAT : TMPL_SHORTSTAT;
	}

	if (!global_cpu) {
		malloc_cpu_size = (info.cpu_count + 1) * sizeof(struct cpu_info);
		cpu = (struct cpu_info *)malloc(malloc_cpu_size);
		memset(cpu, 0, malloc_cpu_size);
		global_cpu = cpu;
	}

	if (!(stat_fp = open_file("/proc/stat", &rep))) {
		info.run_threads = 0;
		if (info.cpu_usage) {
			memset(info.cpu_usage, 0, info.cpu_count * sizeof(float));
		}
		return 0;
	}

	idx = 0;
	while (!feof(stat_fp)) {
		if (fgets(buf, 255, stat_fp) == NULL) {
			break;
		}

		if (strncmp(buf, "procs_running ", 14) == 0) {
			sscanf(buf, "%*s %hu", &info.run_threads);
		} else if (strncmp(buf, "cpu", 3) == 0) {
			double delta;
			if (isdigit(buf[3])) {
				idx++;  //just increment here since the CPU index can skip numbers
			} else {
				idx = 0;
			}
			if (idx > info.cpu_count) {
				continue;
			}
			sscanf(buf, stat_template, &(cpu[idx].cpu_user),
				&(cpu[idx].cpu_nice), &(cpu[idx].cpu_system),
				&(cpu[idx].cpu_idle), &(cpu[idx].cpu_iowait),
				&(cpu[idx].cpu_irq), &(cpu[idx].cpu_softirq),
				&(cpu[idx].cpu_steal));

			cpu[idx].cpu_total = cpu[idx].cpu_user + cpu[idx].cpu_nice +
				cpu[idx].cpu_system + cpu[idx].cpu_idle +
				cpu[idx].cpu_iowait + cpu[idx].cpu_irq +
				cpu[idx].cpu_softirq + cpu[idx].cpu_steal;

			cpu[idx].cpu_active_total = cpu[idx].cpu_total -
				(cpu[idx].cpu_idle + cpu[idx].cpu_iowait);

			delta = current_update_time - last_update_time;

			if (delta <= 0.001) {
				break;
			}

			cur_total = (float) (cpu[idx].cpu_total - cpu[idx].cpu_last_total);
			if (cur_total == 0.0) {
				cpu[idx].cpu_val[0] = 1.0;
			} else {
				cpu[idx].cpu_val[0] = (cpu[idx].cpu_active_total -
					cpu[idx].cpu_last_active_total) / cur_total;
			}
			curtmp = 0;

			int samples = cpu_avg_samples.get(*state);
#ifdef HAVE_OPENMP
#pragma omp parallel for reduction(+:curtmp) schedule(dynamic,10)
#endif /* HAVE_OPENMP */
			for (i = 0; i < samples; i++) {
				curtmp = curtmp + cpu[idx].cpu_val[i];
			}
			info.cpu_usage[idx] = curtmp / samples;

			cpu[idx].cpu_last_total = cpu[idx].cpu_total;
			cpu[idx].cpu_last_active_total = cpu[idx].cpu_active_total;
#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic,10)
#endif /* HAVE_OPENMP */
			for (i = samples - 1; i > 0; i--) {
				cpu[idx].cpu_val[i] = cpu[idx].cpu_val[i - 1];
			}
		}
	}
	fclose(stat_fp);
	return 0;
}

int update_running_processes(void)
{
	update_stat();
	return 0;
}

int update_cpu_usage(void)
{
	update_stat();
	return 0;
}

//fscanf() that reads floats with points even if you are using a locale where
//floats are with commas
int fscanf_no_i18n(FILE *stream, const char *format, ...) {
	int returncode;
	va_list ap;

#ifdef BUILD_I18N
	const char *oldlocale = setlocale(LC_NUMERIC, NULL);

	setlocale(LC_NUMERIC, "C");
#endif
	va_start(ap, format);
	returncode = vfscanf(stream, format, ap);
	va_end(ap);
#ifdef BUILD_I18N
	setlocale(LC_NUMERIC, oldlocale);
#endif
	return returncode;
}

int update_load_average(void)
{
#ifdef HAVE_GETLOADAVG
	if (!prefer_proc) {
		double v[3];

		getloadavg(v, 3);
		info.loadavg[0] = (float) v[0];
		info.loadavg[1] = (float) v[1];
		info.loadavg[2] = (float) v[2];
	} else
#endif
	{
		static int rep = 0;
		FILE *fp;

		if (!(fp = open_file("/proc/loadavg", &rep))) {
			info.loadavg[0] = info.loadavg[1] = info.loadavg[2] = 0.0;
			return 0;
		}
		if (fscanf_no_i18n(fp, "%f %f %f", &info.loadavg[0], &info.loadavg[1],
		           &info.loadavg[2]) < 0)
			info.loadavg[0] = info.loadavg[1] = info.loadavg[2] = 0.0;
		fclose(fp);
	}
	return 0;
}

/***********************************************************/
/***********************************************************/
/***********************************************************/

static int no_dots(const struct dirent *d)
{
	if (d->d_name[0] == '.') {
		return 0;
	}
	return 1;
}

static int get_first_file_in_a_directory(const char *dir, char *s, int *rep)
{
	struct dirent **namelist;
	int i, n;

	n = scandir(dir, &namelist, no_dots, alphasort);
	if (n < 0) {
		if (!rep || !*rep) {
			NORM_ERR("scandir for %s: %s", dir, strerror(errno));
			if (rep) {
				*rep = 1;
			}
		}
		return 0;
	} else {
		if (n == 0) {
			return 0;
		}

		strncpy(s, namelist[0]->d_name, 255);
		s[255] = '\0';

#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic,10)
#endif /* HAVE_OPENMP */
		for (i = 0; i < n; i++) {
			free(namelist[i]);
		}
		free(namelist);

		return 1;
	}
}

static int open_sysfs_sensor(const char *dir, const char *dev, const char *type, int n,
		int *divisor, char *devtype)
{
	char path[256];
	char buf[256];
	int fd;
	int divfd;

	memset(buf, 0, sizeof(buf));

	/* if device is NULL or *, get first */
	if (dev == NULL || strcmp(dev, "*") == 0) {
		static int rep = 0;

		if (!get_first_file_in_a_directory(dir, buf, &rep)) {
			return -1;
		}
		dev = buf;
	}

	if (strcmp(dir, "/sys/class/hwmon/") == 0) {
		if (*buf) {
			/* buf holds result from get_first_file_in_a_directory() above,
			 * e.g. "hwmon0" -- append "/device" */
			strcat(buf, "/device");
		} else {
			/* dev holds device number N as a string,
			 * e.g. "0", -- convert to "hwmon0/device" */
			sprintf(buf, "hwmon%s/device", dev);
			dev = buf;
		}
	}

	/* change vol to in, tempf to temp */
	if (strcmp(type, "vol") == 0) {
		type = "in";
	} else if (strcmp(type, "tempf") == 0) {
		type = "temp";
	}

	/* construct path */
	snprintf(path, 255, "%s%s/%s%d_input", dir, dev, type, n);

	/* first, attempt to open file in /device */
	fd = open(path, O_RDONLY);
	if (fd < 0) {

		/* if it fails, strip the /device from dev and attempt again */
		buf[strlen(buf) - 7] = 0;
		snprintf(path, 255, "%s%s/%s%d_input", dir, dev, type, n);
		fd = open(path, O_RDONLY);
		if (fd < 0) {
			NORM_ERR("can't open '%s': %s\nplease check your device or remove this "
					 "var from " PACKAGE_NAME, path, strerror(errno));
		}
	}

	strncpy(devtype, path, 255);

	if (strcmp(type, "in") == 0 || strcmp(type, "temp") == 0
			|| strcmp(type, "tempf") == 0) {
		*divisor = 1;
	} else {
		*divisor = 0;
	}
	/* fan does not use *_div as a read divisor */
	if (strcmp("fan", type) == 0) {
		return fd;
	}

	/* test if *_div file exist, open it and use it as divisor */
	if (strcmp(type, "tempf") == 0) {
		snprintf(path, 255, "%s%s/%s%d_div", dir, "one", "two", n);
	} else {
		snprintf(path, 255, "%s%s/%s%d_div", dir, dev, type, n);
	}

	divfd = open(path, O_RDONLY);
	if (divfd > 0) {
		/* read integer */
		char divbuf[64];
		int divn;

		divn = read(divfd, divbuf, 63);
		/* should read until n == 0 but I doubt that kernel will give these
		 * in multiple pieces. :) */
		if (divn < 0) {
			NORM_ERR("open_sysfs_sensor(): can't read from sysfs");
		} else {
			divbuf[divn] = '\0';
			*divisor = atoi(divbuf);
		}
		close(divfd);
	}

	return fd;
}

static double get_sysfs_info(int *fd, int divisor, char *devtype, char *type)
{
	int val = 0;

	if (*fd <= 0) {
		return 0;
	}

	lseek(*fd, 0, SEEK_SET);

	/* read integer */
	{
		char buf[64];
		int n;
		n = read(*fd, buf, 63);
		/* should read until n == 0 but I doubt that kernel will give these
		 * in multiple pieces. :) */
		if (n < 0) {
			NORM_ERR("get_sysfs_info(): read from %s failed\n", devtype);
		} else {
			buf[n] = '\0';
			val = atoi(buf);
		}
	}

	close(*fd);
	/* open file */
	*fd = open(devtype, O_RDONLY);
	if (*fd < 0) {
		NORM_ERR("can't open '%s': %s", devtype, strerror(errno));
	}

	/* My dirty hack for computing CPU value
	 * Filedil, from forums.gentoo.org */
	/* if (strstr(devtype, "temp1_input") != NULL) {
		return -15.096 + 1.4893 * (val / 1000.0);
	} */

	/* divide voltage and temperature by 1000 */
	/* or if any other divisor is given, use that */
	if (strcmp(type, "tempf") == 0) {
		if (divisor > 1) {
			return ((val / divisor + 40) * 9.0 / 5) - 40;
		} else if (divisor) {
			return ((val / 1000.0 + 40) * 9.0 / 5) - 40;
		} else {
			return ((val + 40) * 9.0 / 5) - 40;
		}
	} else {
		if (divisor > 1) {
			return val / divisor;
		} else if (divisor) {
			return val / 1000.0;
		} else {
			return val;
		}
	}
}

#define HWMON_RESET() {\
		buf1[0] = 0; \
		factor = 1.0; \
		offset = 0.0; }

static void parse_sysfs_sensor(struct text_object *obj, const char *arg, const char *path, const char *type)
{
	char buf1[64], buf2[64];
	float factor, offset;
	int n, found = 0;
	struct sysfs *sf;

	if (sscanf(arg, "%63s %d %f %f", buf2, &n, &factor, &offset) == 4) found = 1; else HWMON_RESET();
	if (!found && sscanf(arg, "%63s %63s %d %f %f", buf1, buf2, &n, &factor, &offset) == 5) found = 1; else if (!found) HWMON_RESET();
	if (!found && sscanf(arg, "%63s %63s %d", buf1, buf2, &n) == 3) found = 1; else if (!found) HWMON_RESET();
	if (!found && sscanf(arg, "%63s %d", buf2, &n) == 2) found = 1; else if (!found) HWMON_RESET();

	if (!found) {
		obj_be_plain_text(obj, "fail");
		return;
	}
	DBGP("parsed %s args: '%s' '%s' %d %f %f\n", type, buf1, buf2, n, factor, offset);
	sf = (struct sysfs*)malloc(sizeof(struct sysfs));
	memset(sf, 0, sizeof(struct sysfs));
	sf->fd = open_sysfs_sensor(path, (*buf1) ? buf1 : 0, buf2, n,
			&sf->arg, sf->devtype);
	strncpy(sf->type, buf2, 63);
	sf->factor = factor;
	sf->offset = offset;
	obj->data.opaque = sf;
}

#define PARSER_GENERATOR(name, path)                                \
void parse_##name##_sensor(struct text_object *obj, const char *arg) \
{                                                                   \
	parse_sysfs_sensor(obj, arg, path, #name);           \
}

PARSER_GENERATOR(i2c, "/sys/bus/i2c/devices/")
PARSER_GENERATOR(hwmon, "/sys/class/hwmon/")
PARSER_GENERATOR(platform, "/sys/bus/platform/devices/")

void print_sysfs_sensor(struct text_object *obj, char *p, int p_max_size)
{
	double r;
	struct sysfs *sf = (struct sysfs *)obj->data.opaque;

	if (!sf || sf->fd < 0)
		return;

	r = get_sysfs_info(&sf->fd, sf->arg,
			sf->devtype, sf->type);

	r = r * sf->factor + sf->offset;

	if (!strncmp(sf->type, "temp", 4)) {
		temp_print(p, p_max_size, r, TEMP_CELSIUS);
	} else if (r >= 100.0 || r == 0) {
		snprintf(p, p_max_size, "%d", (int) r);
	} else {
		snprintf(p, p_max_size, "%.1f", r);
	}
}

void free_sysfs_sensor(struct text_object *obj)
{
	struct sysfs *sf = (struct sysfs *)obj->data.opaque;

	if (!sf)
		return;

	if(sf->fd >= 0)
		close(sf->fd);
	free_and_zero(obj->data.opaque);
}

#define CPUFREQ_PREFIX "/sys/devices/system/cpu"
#define CPUFREQ_POSTFIX "cpufreq/scaling_cur_freq"

/* return system frequency in MHz (use divisor=1) or GHz (use divisor=1000) */
char get_freq(char *p_client_buffer, size_t client_buffer_size,
		const char *p_format, int divisor, unsigned int cpu)
{
	FILE *f;
	static int rep = 0;
	char frequency[32];
	char s[256];
	double freq = 0;

	if (!p_client_buffer || client_buffer_size <= 0 || !p_format
			|| divisor <= 0) {
		return 0;
	}

	if (!prefer_proc) {
		char current_freq_file[128];

		snprintf(current_freq_file, 127, "%s/cpu%d/%s", CPUFREQ_PREFIX, cpu - 1,
			CPUFREQ_POSTFIX);
		f = fopen(current_freq_file, "r");
		if (f) {
			/* if there's a cpufreq /sys node, read the current frequency from
			 * this node and divide by 1000 to get Mhz. */
			if (fgets(s, sizeof(s), f)) {
				s[strlen(s) - 1] = '\0';
				freq = strtod(s, NULL);
			}
			fclose(f);
			snprintf(p_client_buffer, client_buffer_size, p_format,
				(freq / 1000) / divisor);
			return 1;
		}
	}

	// open the CPU information file
	f = open_file("/proc/cpuinfo", &rep);
	if (!f) {
		perror(PACKAGE_NAME": Failed to access '/proc/cpuinfo' at get_freq()");
		return 0;
	}

	// read the file
	while (fgets(s, sizeof(s), f) != NULL) {

#if defined(__i386) || defined(__x86_64)
		// and search for the cpu mhz
		if (strncmp(s, "cpu MHz", 7) == 0 && cpu == 0) {
#else
#if defined(__alpha)
		// different on alpha
		if (strncmp(s, "cycle frequency [Hz]", 20) == 0 && cpu == 0) {
#else
		// this is different on ppc for some reason
		if (strncmp(s, "clock", 5) == 0 && cpu == 0) {
#endif // defined(__alpha)
#endif // defined(__i386) || defined(__x86_64)

			// copy just the number
			strcpy(frequency, strchr(s, ':') + 2);
#if defined(__alpha)
			// strip " est.\n"
			frequency[strlen(frequency) - 6] = '\0';
			// kernel reports in Hz
			freq = strtod(frequency, NULL) / 1000000;
#else
			// strip \n
			frequency[strlen(frequency) - 1] = '\0';
			freq = strtod(frequency, NULL);
#endif
			break;
		}
		if (strncmp(s, "processor", 9) == 0) {
			cpu--;
			continue;
		}
	}

	fclose(f);
	snprintf(p_client_buffer, client_buffer_size, p_format,
		(float) freq / divisor);
	return 1;
}

#define CPUFREQ_VOLTAGE "cpufreq/scaling_voltages"

/* /sys/devices/system/cpu/cpu0/cpufreq/scaling_voltages looks something
 * like this:
# frequency voltage
1800000 1340
1600000 1292
1400000 1100
1200000 988
1000000 1116
800000 1004
600000 988
 * Peter Tarjan (ptarjan@citromail.hu) */

/* return cpu voltage in mV (use divisor=1) or V (use divisor=1000) */
static char get_voltage(char *p_client_buffer, size_t client_buffer_size,
		const char *p_format, int divisor, unsigned int cpu)
{
	FILE *f;
	char s[256];
	int freq = 0;
	int voltage = 0;
	char current_freq_file[128];
	int freq_comp = 0;

	/* build the voltage file name */
	cpu--;
	snprintf(current_freq_file, 127, "%s/cpu%d/%s", CPUFREQ_PREFIX, cpu,
		CPUFREQ_POSTFIX);

	if (!p_client_buffer || client_buffer_size <= 0 || !p_format
			|| divisor <= 0) {
		return 0;
	}

	/* read the current cpu frequency from the /sys node */
	f = fopen(current_freq_file, "r");
	if (f) {
		if (fgets(s, sizeof(s), f)) {
			s[strlen(s) - 1] = '\0';
			freq = strtod(s, NULL);
		}
		fclose(f);
	} else {
		fprintf(stderr, PACKAGE_NAME": Failed to access '%s' at ", current_freq_file);
		perror("get_voltage()");
		if (f) {
			fclose(f);
		}
		return 0;
	}

	snprintf(current_freq_file, 127, "%s/cpu%d/%s", CPUFREQ_PREFIX, cpu,
		CPUFREQ_VOLTAGE);

	/* use the current cpu frequency to find the corresponding voltage */
	f = fopen(current_freq_file, "r");

	if (f) {
		while (!feof(f)) {
			char line[256];

			if (fgets(line, 255, f) == NULL) {
				break;
			}
			sscanf(line, "%d %d", &freq_comp, &voltage);
			if (freq_comp == freq) {
				break;
			}
		}
		fclose(f);
	} else {
		fprintf(stderr, PACKAGE_NAME": Failed to access '%s' at ", current_freq_file);
		perror("get_voltage()");
		if (f) {
			fclose(f);
		}
		return 0;
	}
	snprintf(p_client_buffer, client_buffer_size, p_format,
		(float) voltage / divisor);
	return 1;
}

void print_voltage_mv(struct text_object *obj, char *p, int p_max_size)
{
	static int ok = 1;
	if (ok) {
		ok = get_voltage(p, p_max_size, "%.0f", 1, obj->data.i);
	}
}

void print_voltage_v(struct text_object *obj, char *p, int p_max_size)
{
	static int ok = 1;
	if (ok) {
		ok = get_voltage(p, p_max_size, "%'.3f", 1000, obj->data.i);
	}
}

#define ACPI_FAN_DIR "/proc/acpi/fan/"

void get_acpi_fan(char *p_client_buffer, size_t client_buffer_size)
{
	static int rep = 0;
	char buf[256];
	char buf2[256];
	FILE *fp;

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	/* yeah, slow... :/ */
	if (!get_first_file_in_a_directory(ACPI_FAN_DIR, buf, &rep)) {
		snprintf(p_client_buffer, client_buffer_size, "no fans?");
		return;
	}

	snprintf(buf2, sizeof(buf2), "%s%s/state", ACPI_FAN_DIR, buf);

	fp = open_file(buf2, &rep);
	if (!fp) {
		snprintf(p_client_buffer, client_buffer_size,
			"can't open fan's state file");
		return;
	}
	memset(buf, 0, sizeof(buf));
	if (fscanf(fp, "%*s %99s", buf) <= 0)
		perror("fscanf()");
	fclose(fp);

	snprintf(p_client_buffer, client_buffer_size, "%s", buf);
}

#define SYSFS_AC_ADAPTER_DIR "/sys/class/power_supply"
#define ACPI_AC_ADAPTER_DIR "/proc/acpi/ac_adapter/"
/* Linux 2.6.25 onwards ac adapter info is in
   /sys/class/power_supply/AC/
   On my system I get the following.
     /sys/class/power_supply/AC/uevent:
     PHYSDEVPATH=/devices/LNXSYSTM:00/device:00/PNP0A08:00/device:01/PNP0C09:00/ACPI0003:00
     PHYSDEVBUS=acpi
     PHYSDEVDRIVER=ac
     POWER_SUPPLY_NAME=AC
     POWER_SUPPLY_TYPE=Mains
     POWER_SUPPLY_ONLINE=1

   Update: it seems the folder name is hardware-dependent. We add an aditional adapter
   argument, specifying the folder name.

   Update: on some systems it's /sys/class/power_supply/ADP1 instead of /sys/class/power_supply/AC
*/

void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size, const char *adapter)
{
	static int rep = 0;

	char buf[256];
	char buf2[256];
	struct stat sb;
	FILE *fp;

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	if(adapter)
		snprintf(buf2, sizeof(buf2), "%s/%s/uevent", SYSFS_AC_ADAPTER_DIR, adapter);
	else{
		snprintf(buf2, sizeof(buf2), "%s/AC/uevent", SYSFS_AC_ADAPTER_DIR);
		if(stat(buf2, &sb) == -1) snprintf(buf2, sizeof(buf2), "%s/ADP1/uevent", SYSFS_AC_ADAPTER_DIR);
	}
	if(stat(buf2, &sb) == 0) fp = open_file(buf2, &rep); else fp = 0;
	if (fp) {
		/* sysfs processing */
		while (!feof(fp)) {
			if (fgets(buf, sizeof(buf), fp) == NULL)
				break;

			if (strncmp(buf, "POWER_SUPPLY_ONLINE=", 20) == 0) {
				int online = 0;
				sscanf(buf, "POWER_SUPPLY_ONLINE=%d", &online);
				snprintf(p_client_buffer, client_buffer_size,
					 "%s-line", (online ? "on" : "off"));
				break;
			}
		}
		fclose(fp);
	} else {
		/* yeah, slow... :/ */
		if (!get_first_file_in_a_directory(ACPI_AC_ADAPTER_DIR, buf, &rep)) {
			snprintf(p_client_buffer, client_buffer_size, "no ac_adapters?");
			return;
		}

		snprintf(buf2, sizeof(buf2), "%s%s/state", ACPI_AC_ADAPTER_DIR, buf);

		fp = open_file(buf2, &rep);
		if (!fp) {
			snprintf(p_client_buffer, client_buffer_size,
				 "No ac adapter found.... where is it?");
			return;
		}
		memset(buf, 0, sizeof(buf));
		if (fscanf(fp, "%*s %99s", buf) <= 0)
			perror("fscanf()");
		fclose(fp);

		snprintf(p_client_buffer, client_buffer_size, "%s", buf);
	}
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

#define ACPI_THERMAL_ZONE_DEFAULT "thermal_zone0"
#define ACPI_THERMAL_FORMAT "/sys/class/thermal/%s/temp"

int open_acpi_temperature(const char *name)
{
	char path[256];
	int fd;

	if (name == NULL || strcmp(name, "*") == 0) {
		snprintf(path, 255, ACPI_THERMAL_FORMAT, ACPI_THERMAL_ZONE_DEFAULT);
	} else {
		snprintf(path, 255, ACPI_THERMAL_FORMAT, name);
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		NORM_ERR("can't open '%s': %s", path, strerror(errno));
	}

	return fd;
}

static double last_acpi_temp;
static double last_acpi_temp_time;

//the maximum length of the string inside a ACPI_THERMAL_FORMAT file including the ending 0
#define MAXTHERMZONELEN 6

double get_acpi_temperature(int fd)
{
	if (fd <= 0) {
		return 0;
	}

	/* don't update acpi temperature too often */
	if (current_update_time - last_acpi_temp_time < 11.32) {
		return last_acpi_temp;
	}
	last_acpi_temp_time = current_update_time;

	/* seek to beginning */
	lseek(fd, 0, SEEK_SET);

	/* read */
	{
		char buf[MAXTHERMZONELEN];
		int n;

		n = read(fd, buf, MAXTHERMZONELEN-1);
		if (n < 0) {
			NORM_ERR("can't read fd %d: %s", fd, strerror(errno));
		} else {
			buf[n] = '\0';
			sscanf(buf, "%lf", &last_acpi_temp);
			last_acpi_temp /= 1000;
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
2213<@jupet�kellari��> jupet@lagi-unstable:~$ cat /proc/apm
2213<@jupet�kellari��> 1.16 1.2 0x03 0x01 0xff 0x10 -1% -1 ?
2213<@jupet�kellari��> (-1 ollee ei akkua kiinni, koska akku on p�yd�ll�)
2214<@jupet�kellari��> jupet@lagi-unstable:~$ cat /proc/apm
2214<@jupet�kellari��> 1.16 1.2 0x03 0x01 0x03 0x09 98% -1 ?

2238<@jupet�kellari��> 1.16 1.2 0x03 0x00 0x00 0x01 100% -1 ? ilman verkkovirtaa
2239<@jupet�kellari��> 1.16 1.2 0x03 0x01 0x00 0x01 99% -1 ? verkkovirralla

2240<@jupet�kellari��> 1.16 1.2 0x03 0x01 0x03 0x09 100% -1 ? verkkovirralla ja monitori p��ll�
2241<@jupet�kellari��> 1.16 1.2 0x03 0x00 0x00 0x01 99% -1 ? monitori p��ll� mutta ilman verkkovirtaa
*/

/* Kapil Hari Paranjape <kapil@imsc.res.in>
  Linux 2.6.24 onwards battery info is in
  /sys/class/power_supply/BAT0/
  On my system I get the following.
	/sys/class/power_supply/BAT0/uevent:
	PHYSDEVPATH=/devices/LNXSYSTM:00/device:00/PNP0A03:00/device:01/PNP0C09:00/PNP0C0A:00
	PHYSDEVBUS=acpi
	PHYSDEVDRIVER=battery
	POWER_SUPPLY_NAME=BAT0
	POWER_SUPPLY_TYPE=Battery
	POWER_SUPPLY_STATUS=Discharging
	POWER_SUPPLY_PRESENT=1
	POWER_SUPPLY_TECHNOLOGY=Li-ion
	POWER_SUPPLY_VOLTAGE_MIN_DESIGN=10800000
	POWER_SUPPLY_VOLTAGE_NOW=10780000
	POWER_SUPPLY_CURRENT_NOW=13970000
	POWER_SUPPLY_ENERGY_FULL_DESIGN=47510000
	POWER_SUPPLY_ENERGY_FULL=27370000
	POWER_SUPPLY_ENERGY_NOW=11810000
	POWER_SUPPLY_MODEL_NAME=IBM-92P1060
	POWER_SUPPLY_MANUFACTURER=Panasonic
  On some systems POWER_SUPPLY_ENERGY_* is replaced by POWER_SUPPLY_CHARGE_*
*/

/* Tiago Marques Vale <tiagomarquesvale@gmail.com>
  Regarding the comment above, since kernel 2.6.36.1 I have
  POWER_SUPPLY_POWER_NOW instead of POWER_SUPPLY_CURRENT_NOW
  See http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=532000
*/

#define SYSFS_BATTERY_BASE_PATH "/sys/class/power_supply"
#define ACPI_BATTERY_BASE_PATH "/proc/acpi/battery"
#define APM_PATH "/proc/apm"
#define MAX_BATTERY_COUNT 4

static FILE *sysfs_bat_fp[MAX_BATTERY_COUNT] = { NULL, NULL, NULL, NULL };
static FILE *acpi_bat_fp[MAX_BATTERY_COUNT] = { NULL, NULL, NULL, NULL };
static FILE *apm_bat_fp[MAX_BATTERY_COUNT] = { NULL, NULL, NULL, NULL };

static int batteries_initialized = 0;
static char batteries[MAX_BATTERY_COUNT][32];

static int acpi_last_full[MAX_BATTERY_COUNT];
static int acpi_design_capacity[MAX_BATTERY_COUNT];

/* e.g. "charging 75%" */
static char last_battery_str[MAX_BATTERY_COUNT][64];
/* e.g. "3h 15m" */
static char last_battery_time_str[MAX_BATTERY_COUNT][64];

static double last_battery_time[MAX_BATTERY_COUNT];

static int last_battery_perct[MAX_BATTERY_COUNT];
static double last_battery_perct_time[MAX_BATTERY_COUNT];

void init_batteries(void)
{
	int idx;

	if (batteries_initialized) {
		return;
	}
#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic,10)
#endif /* HAVE_OPENMP */
	for (idx = 0; idx < MAX_BATTERY_COUNT; idx++) {
		batteries[idx][0] = '\0';
	}
	batteries_initialized = 1;
}

int get_battery_idx(const char *bat)
{
	int idx;

	for (idx = 0; idx < MAX_BATTERY_COUNT; idx++) {
		if (!strlen(batteries[idx]) || !strcmp(batteries[idx], bat)) {
			break;
		}
	}

	/* if not found, enter a new entry */
	if (!strlen(batteries[idx])) {
		snprintf(batteries[idx], 31, "%s", bat);
	}

	return idx;
}

void set_return_value(char *buffer, unsigned int n, int item, int idx);

void get_battery_stuff(char *buffer, unsigned int n, const char *bat, int item)
{
	static int idx, rep = 0, rep1 = 0, rep2 = 0;
	char acpi_path[128];
	char sysfs_path[128];

	snprintf(acpi_path, 127, ACPI_BATTERY_BASE_PATH "/%s/state", bat);
	snprintf(sysfs_path, 127, SYSFS_BATTERY_BASE_PATH "/%s/uevent", bat);

	init_batteries();

	idx = get_battery_idx(bat);

	/* don't update battery too often */
	if (current_update_time - last_battery_time[idx] < 29.5) {
		set_return_value(buffer, n, item, idx);
		return;
	}

	last_battery_time[idx] = current_update_time;

	memset(last_battery_str[idx], 0, sizeof(last_battery_str[idx]));
	memset(last_battery_time_str[idx], 0, sizeof(last_battery_time_str[idx]));

	/* first try SYSFS if that fails try ACPI */

	if (sysfs_bat_fp[idx] == NULL && acpi_bat_fp[idx] == NULL && apm_bat_fp[idx] == NULL) {
		sysfs_bat_fp[idx] = open_file(sysfs_path, &rep);
	}

	if (sysfs_bat_fp[idx] == NULL && acpi_bat_fp[idx] == NULL && apm_bat_fp[idx] == NULL) {
		acpi_bat_fp[idx] = open_file(acpi_path, &rep1);
	}

	if (sysfs_bat_fp[idx] != NULL) {
		/* SYSFS */
		int present_rate = -1;
		int remaining_capacity = -1;
		char charging_state[64];
		char present[4];

		strcpy(charging_state, "unknown");

		while (!feof(sysfs_bat_fp[idx])) {
			char buf[256];
			if (fgets(buf, 256, sysfs_bat_fp[idx]) == NULL)
				break;

			/* let's just hope units are ok */
			if (strncmp (buf, "POWER_SUPPLY_PRESENT=1", 22) == 0)
				strcpy(present, "yes");
			else if (strncmp (buf, "POWER_SUPPLY_PRESENT=0", 22) == 0)
				strcpy(present, "no");
			else if (strncmp (buf, "POWER_SUPPLY_STATUS=", 20) == 0)
				sscanf(buf, "POWER_SUPPLY_STATUS=%63s", charging_state);
			/* present_rate is not the same as the current flowing now but it
			 * is the same value which was used in the past. so we continue the
			 * tradition! */
			else if (strncmp(buf, "POWER_SUPPLY_CURRENT_NOW=", 25) == 0)
				sscanf(buf, "POWER_SUPPLY_CURRENT_NOW=%d", &present_rate);
			else if (strncmp(buf, "POWER_SUPPLY_POWER_NOW=", 23) == 0)
				sscanf(buf, "POWER_SUPPLY_POWER_NOW=%d", &present_rate);
			else if (strncmp(buf, "POWER_SUPPLY_ENERGY_NOW=", 24) == 0)
				sscanf(buf, "POWER_SUPPLY_ENERGY_NOW=%d", &remaining_capacity);
			else if (strncmp(buf, "POWER_SUPPLY_ENERGY_FULL=", 25) == 0)
				sscanf(buf, "POWER_SUPPLY_ENERGY_FULL=%d", &acpi_last_full[idx]);
			else if (strncmp(buf, "POWER_SUPPLY_CHARGE_NOW=", 24) == 0)
				sscanf(buf, "POWER_SUPPLY_CHARGE_NOW=%d", &remaining_capacity);
			else if (strncmp(buf, "POWER_SUPPLY_CHARGE_FULL=", 25) == 0)
				sscanf(buf, "POWER_SUPPLY_CHARGE_FULL=%d", &acpi_last_full[idx]);
		}

		fclose(sysfs_bat_fp[idx]);
		sysfs_bat_fp[idx] = NULL;

		/* Hellf[i]re notes that remaining capacity can exceed acpi_last_full */
		if (remaining_capacity > acpi_last_full[idx])
			acpi_last_full[idx] = remaining_capacity;  /* normalize to 100% */

		/* not present */
		if (strcmp(present, "No") == 0) {
			strncpy(last_battery_str[idx], "not present", 64);
		}
		/* charging */
		else if (strcmp(charging_state, "Charging") == 0) {
			if (acpi_last_full[idx] != 0 && present_rate > 0) {
				/* e.g. charging 75% */
				snprintf(last_battery_str[idx], sizeof(last_battery_str[idx])-1, "charging %i%%",
						(int) (((float) remaining_capacity / acpi_last_full[idx]) * 100 ));
				/* e.g. 2h 37m */
				format_seconds(last_battery_time_str[idx], sizeof(last_battery_time_str[idx])-1,
						(long) (((float)(acpi_last_full[idx] - remaining_capacity) / present_rate) * 3600));
			} else if (acpi_last_full[idx] != 0 && present_rate <= 0) {
				snprintf(last_battery_str[idx], sizeof(last_battery_str[idx])-1, "charging %d%%",
						(int) (((float)remaining_capacity / acpi_last_full[idx]) * 100));
				snprintf(last_battery_time_str[idx],
						sizeof(last_battery_time_str[idx]) - 1, "unknown");
			} else {
				strncpy(last_battery_str[idx], "charging", sizeof(last_battery_str[idx])-1);
				snprintf(last_battery_time_str[idx],
						sizeof(last_battery_time_str[idx]) - 1, "unknown");
			}
		}
		/* discharging */
		else if (strncmp(charging_state, "Discharging", 64) == 0) {
			if (present_rate > 0) {
				/* e.g. discharging 35% */
				snprintf(last_battery_str[idx], sizeof(last_battery_str[idx])-1, "discharging %i%%",
					(int) (((float) remaining_capacity / acpi_last_full[idx]) * 100 ));
				/* e.g. 1h 12m */
				format_seconds(last_battery_time_str[idx], sizeof(last_battery_time_str[idx])-1,
					      (long) (((float) remaining_capacity / present_rate) * 3600));
			} else if (present_rate == 0) { /* Thanks to Nexox for this one */
				snprintf(last_battery_str[idx], sizeof(last_battery_str[idx])-1, "full");
				snprintf(last_battery_time_str[idx],
					sizeof(last_battery_time_str[idx]) - 1, "unknown");
			} else {
				snprintf(last_battery_str[idx], sizeof(last_battery_str[idx])-1,
					"discharging %d%%",
					(int) (((float)remaining_capacity / acpi_last_full[idx]) * 100));
				snprintf(last_battery_time_str[idx],
					sizeof(last_battery_time_str[idx]) - 1, "unknown");
			}
		}
		/* charged */
		/* thanks to Lukas Zapletal <lzap@seznam.cz> */
		else if (strncmp(charging_state, "Charged", 64) == 0 || strncmp(charging_state, "Full", 64) == 0) {
				/* Below happens with the second battery on my X40,
				 * when the second one is empty and the first one
				 * being charged. */
				if (remaining_capacity == 0)
					strcpy(last_battery_str[idx], "empty");
				else
					strcpy(last_battery_str[idx], "charged");
		}
		/* unknown, probably full / AC */
		else {
			if (acpi_last_full[idx] != 0
			    && remaining_capacity != acpi_last_full[idx])
				snprintf(last_battery_str[idx], 64, "unknown %d%%",
					(int) (((float)remaining_capacity / acpi_last_full[idx]) * 100));
			else
				strncpy(last_battery_str[idx], "not present", 64);
		}
	} else if (acpi_bat_fp[idx] != NULL) {
		/* ACPI */
		int present_rate = -1;
		int remaining_capacity = -1;
		char charging_state[64];
		char present[5];

		/* read last full capacity if it's zero */
		if (acpi_last_full[idx] == 0) {
			static int rep3 = 0;
			char path[128];
			FILE *fp;

			snprintf(path, 127, ACPI_BATTERY_BASE_PATH "/%s/info", bat);
			fp = open_file(path, &rep3);
			if (fp != NULL) {
				while (!feof(fp)) {
					char b[256];

					if (fgets(b, 256, fp) == NULL) {
						break;
					}
					if (sscanf(b, "last full capacity: %d",
								&acpi_last_full[idx]) != 0) {
						break;
					}
				}

				fclose(fp);
			}
		}

		fseek(acpi_bat_fp[idx], 0, SEEK_SET);

		strcpy(charging_state, "unknown");

		while (!feof(acpi_bat_fp[idx])) {
			char buf[256];

			if (fgets(buf, 256, acpi_bat_fp[idx]) == NULL) {
				break;
			}

			/* let's just hope units are ok */
			if (strncmp(buf, "present:", 8) == 0) {
				sscanf(buf, "present: %4s", present);
			} else if (strncmp(buf, "charging state:", 15) == 0) {
				sscanf(buf, "charging state: %63s", charging_state);
			} else if (strncmp(buf, "present rate:", 13) == 0) {
				sscanf(buf, "present rate: %d", &present_rate);
			} else if (strncmp(buf, "remaining capacity:", 19) == 0) {
				sscanf(buf, "remaining capacity: %d", &remaining_capacity);
			}
		}
		/* Hellf[i]re notes that remaining capacity can exceed acpi_last_full */
		if (remaining_capacity > acpi_last_full[idx]) {
			/* normalize to 100% */
			acpi_last_full[idx] = remaining_capacity;
		}

		/* not present */
		if (strcmp(present, "no") == 0) {
			strncpy(last_battery_str[idx], "not present", 64);
			/* charging */
		} else if (strcmp(charging_state, "charging") == 0) {
			if (acpi_last_full[idx] != 0 && present_rate > 0) {
				/* e.g. charging 75% */
				snprintf(last_battery_str[idx],
						sizeof(last_battery_str[idx]) - 1, "charging %i%%",
						(int) ((remaining_capacity * 100) / acpi_last_full[idx]));
				/* e.g. 2h 37m */
				format_seconds(last_battery_time_str[idx],
						sizeof(last_battery_time_str[idx]) - 1,
						(long) (((acpi_last_full[idx] - remaining_capacity) *
								3600) / present_rate));
			} else if (acpi_last_full[idx] != 0 && present_rate <= 0) {
				snprintf(last_battery_str[idx],
						sizeof(last_battery_str[idx]) - 1, "charging %d%%",
						(int) ((remaining_capacity * 100) / acpi_last_full[idx]));
				snprintf(last_battery_time_str[idx],
						sizeof(last_battery_time_str[idx]) - 1, "unknown");
			} else {
				strncpy(last_battery_str[idx], "charging",
						sizeof(last_battery_str[idx]) - 1);
				snprintf(last_battery_time_str[idx],
						sizeof(last_battery_time_str[idx]) - 1, "unknown");
			}
			/* discharging */
		} else if (strncmp(charging_state, "discharging", 64) == 0) {
			if (present_rate > 0) {
				/* e.g. discharging 35% */
				snprintf(last_battery_str[idx],
						sizeof(last_battery_str[idx]) - 1, "discharging %i%%",
						(int) ((remaining_capacity * 100) / acpi_last_full[idx]));
				/* e.g. 1h 12m */
				format_seconds(last_battery_time_str[idx],
						sizeof(last_battery_time_str[idx]) - 1,
						(long) ((remaining_capacity * 3600) / present_rate));
			} else if (present_rate == 0) {	/* Thanks to Nexox for this one */
				snprintf(last_battery_str[idx],
						sizeof(last_battery_str[idx]) - 1, "charged");
				snprintf(last_battery_time_str[idx],
						sizeof(last_battery_time_str[idx]) - 1, "unknown");
			} else {
				snprintf(last_battery_str[idx],
						sizeof(last_battery_str[idx]) - 1, "discharging %d%%",
						(int) ((remaining_capacity * 100) / acpi_last_full[idx]));
				snprintf(last_battery_time_str[idx],
						sizeof(last_battery_time_str[idx]) - 1, "unknown");
			}
			/* charged */
		} else if (strncmp(charging_state, "charged", 64) == 0) {
			/* thanks to Lukas Zapletal <lzap@seznam.cz> */
			/* Below happens with the second battery on my X40,
			 * when the second one is empty and the first one being charged. */
			if (remaining_capacity == 0) {
				strcpy(last_battery_str[idx], "empty");
			} else {
				strcpy(last_battery_str[idx], "charged");
			}
			/* unknown, probably full / AC */
		} else {
			if (strncmp(charging_state, "Full", 64) == 0) {
				strncpy(last_battery_str[idx], "charged", 64);
			} else if (acpi_last_full[idx] != 0
					&& remaining_capacity != acpi_last_full[idx]) {
				snprintf(last_battery_str[idx], 64, "unknown %d%%",
						(int) ((remaining_capacity * 100) / acpi_last_full[idx]));
			} else {
				strncpy(last_battery_str[idx], "not present", 64);
			}
		}
		fclose(acpi_bat_fp[idx]);
		acpi_bat_fp[idx] = NULL;
	} else {
		/* APM */
		if (apm_bat_fp[idx] == NULL) {
			apm_bat_fp[idx] = open_file(APM_PATH, &rep2);
		}

		if (apm_bat_fp[idx] != NULL) {
			unsigned int ac, status, flag;
			int life;

			if (fscanf(apm_bat_fp[idx], "%*s %*s %*x %x   %x       %x     %d%%",
			           &ac, &status, &flag, &life) <= 0)
				goto read_bat_fp_end;

			if (life == -1) {
				/* could check now that there is ac */
				snprintf(last_battery_str[idx], 64, "not present");

			/* could check that status == 3 here? */
			} else if (ac && life != 100) {
				snprintf(last_battery_str[idx], 64, "charging %d%%", life);
			} else {
				snprintf(last_battery_str[idx], 64, "%d%%", life);
			}

read_bat_fp_end:
			/* it seemed to buffer it so file must be closed (or could use
			 * syscalls directly but I don't feel like coding it now) */
			fclose(apm_bat_fp[idx]);
			apm_bat_fp[idx] = NULL;
		}
	}
	set_return_value(buffer, n, item, idx);
}

void set_return_value(char *buffer, unsigned int n, int item, int idx)
{
	switch (item) {
		case BATTERY_STATUS:
			snprintf(buffer, n, "%s", last_battery_str[idx]);
			break;
		case BATTERY_TIME:
			snprintf(buffer, n, "%s", last_battery_time_str[idx]);
			break;
		default:
			break;
	}
}

void get_battery_short_status(char *buffer, unsigned int n, const char *bat)
{
	get_battery_stuff(buffer, n, bat, BATTERY_STATUS);
	if (0 == strncmp("charging", buffer, 8)) {
		buffer[0] = 'C';
		memmove(buffer + 1, buffer + 8, n - 8);
	} else if (0 == strncmp("discharging", buffer, 11)) {
		buffer[0] = 'D';
		memmove(buffer + 1, buffer + 11, n - 11);
	} else if (0 == strncmp("charged", buffer, 7)) {
		buffer[0] = 'F';
		memmove(buffer + 1, buffer + 7, n - 7);
	} else if (0 == strncmp("not present", buffer, 11)) {
		buffer[0] = 'N';
		memmove(buffer + 1, buffer + 11, n - 11);
	} else if (0 == strncmp("empty", buffer, 5)) {
		buffer[0] = 'E';
		memmove(buffer + 1, buffer + 5, n - 5);
	} else if (0 == strncmp("unknown", buffer, 7)) {
		buffer[0] = 'U';
		memmove(buffer + 1, buffer + 7, n - 7);
	}
	// Otherwise, don't shorten.
}

int _get_battery_perct(const char *bat)
{
	static int rep = 0;
	int idx;
	char acpi_path[128];
	char sysfs_path[128];
	int remaining_capacity = -1;

	snprintf(acpi_path, 127, ACPI_BATTERY_BASE_PATH "/%s/state", bat);
	snprintf(sysfs_path, 127, SYSFS_BATTERY_BASE_PATH "/%s/uevent", bat);

	idx = get_battery_idx(bat);

	/* don't update battery too often */
	if (current_update_time - last_battery_perct_time[idx] < 30) {
		return last_battery_perct[idx];
	}
	last_battery_perct_time[idx] = current_update_time;

	/* Only check for SYSFS or ACPI */

	if (sysfs_bat_fp[idx] == NULL && acpi_bat_fp[idx] == NULL && apm_bat_fp[idx] == NULL) {
		sysfs_bat_fp[idx] = open_file(sysfs_path, &rep);
		rep = 0;
	}

	if (sysfs_bat_fp[idx] == NULL && acpi_bat_fp[idx] == NULL && apm_bat_fp[idx] == NULL) {
		acpi_bat_fp[idx] = open_file(acpi_path, &rep);
	}

	if (sysfs_bat_fp[idx] != NULL) {
		/* SYSFS */
		while (!feof(sysfs_bat_fp[idx])) {
			char buf[256];
			if (fgets(buf, 256, sysfs_bat_fp[idx]) == NULL)
				break;

			if (strncmp(buf, "POWER_SUPPLY_CHARGE_NOW=", 24) == 0) {
				sscanf(buf, "POWER_SUPPLY_CHARGE_NOW=%d", &remaining_capacity);
			} else if (strncmp(buf, "POWER_SUPPLY_CHARGE_FULL=",25) == 0) {
				sscanf(buf, "POWER_SUPPLY_CHARGE_FULL=%d", &acpi_design_capacity[idx]);
			} else if (strncmp(buf, "POWER_SUPPLY_ENERGY_NOW=", 24) == 0) {
				sscanf(buf, "POWER_SUPPLY_ENERGY_NOW=%d", &remaining_capacity);
			} else if (strncmp(buf, "POWER_SUPPLY_ENERGY_FULL=",25) == 0) {
				sscanf(buf, "POWER_SUPPLY_ENERGY_FULL=%d", &acpi_design_capacity[idx]);
			}
		}

		fclose(sysfs_bat_fp[idx]);
		sysfs_bat_fp[idx] = NULL;

	} else if (acpi_bat_fp[idx] != NULL) {
		/* ACPI */
		/* read last full capacity if it's zero */
		if (acpi_design_capacity[idx] == 0) {
			static int rep2;
			char path[128];
			FILE *fp;

			snprintf(path, 127, ACPI_BATTERY_BASE_PATH "/%s/info", bat);
			fp = open_file(path, &rep2);
			if (fp != NULL) {
				while (!feof(fp)) {
					char b[256];

					if (fgets(b, 256, fp) == NULL) {
						break;
					}
					if (sscanf(b, "last full capacity: %d",
								&acpi_design_capacity[idx]) != 0) {
						break;
					}
				}
				fclose(fp);
			}
		}

		fseek(acpi_bat_fp[idx], 0, SEEK_SET);

		while (!feof(acpi_bat_fp[idx])) {
			char buf[256];

			if (fgets(buf, 256, acpi_bat_fp[idx]) == NULL) {
				break;
			}

			if (buf[0] == 'r') {
				sscanf(buf, "remaining capacity: %d", &remaining_capacity);
			}
		}
	}
	if (remaining_capacity < 0) {
		return 0;
	}
	/* compute the battery percentage */
	last_battery_perct[idx] =
		(int) (((float) remaining_capacity / acpi_design_capacity[idx]) * 100);
	if (last_battery_perct[idx] > 100) last_battery_perct[idx] = 100;
	return last_battery_perct[idx];
}

int get_battery_perct(const char *bat)
{
	int idx, n = 0, total_capacity = 0, remaining_capacity;;
#define BATTERY_LEN 8
	char battery[BATTERY_LEN];

	init_batteries();

	/* Check if user asked for the mean percentage of all batteries. */
	if (!strcmp(bat, "all")) {
		for (idx = 0; idx < MAX_BATTERY_COUNT; idx++) {
			snprintf(battery, BATTERY_LEN - 1, "BAT%d", idx);
#undef BATTERY_LEN
			remaining_capacity = _get_battery_perct(battery);
			if (remaining_capacity > 0) {
				total_capacity += remaining_capacity;
				n++;
			}
		}

		if (n == 0)
			return 0;
		else
			return total_capacity / n;
	} else {
		return _get_battery_perct(bat);
	}
}

double get_battery_perct_bar(struct text_object *obj)
{
	int idx;

	get_battery_perct(obj->data.s);
	idx = get_battery_idx(obj->data.s);
	return last_battery_perct[idx];
}

/* On Apple powerbook and ibook:
$ cat /proc/pmu/battery_0
flags      : 00000013
charge     : 3623
max_charge : 3720
current    : 388
voltage    : 16787
time rem.  : 900
$ cat /proc/pmu/info
PMU driver version     : 2
PMU firmware version   : 0c
AC Power               : 1
Battery count          : 1
*/

/* defines as in <linux/pmu.h> */
#define PMU_BATT_PRESENT		0x00000001
#define PMU_BATT_CHARGING		0x00000002

static FILE *pmu_battery_fp;
static FILE *pmu_info_fp;
static char pb_battery_info[3][32];
static double pb_battery_info_update;

#define PMU_PATH "/proc/pmu"
void get_powerbook_batt_info(struct text_object *obj, char *buffer, int n)
{
	static int rep = 0;
	const char *batt_path = PMU_PATH "/battery_0";
	const char *info_path = PMU_PATH "/info";
	unsigned int flags;
	int charge, max_charge, ac = -1;
	long timeval = -1;

	/* don't update battery too often */
	if (current_update_time - pb_battery_info_update < 29.5) {
		snprintf(buffer, n, "%s", pb_battery_info[obj->data.i]);
		return;
	}
	pb_battery_info_update = current_update_time;

	if (pmu_battery_fp == NULL) {
		pmu_battery_fp = open_file(batt_path, &rep);
		if (pmu_battery_fp == NULL) {
			return;
		}
	}

	if (pmu_battery_fp != NULL) {
		rewind(pmu_battery_fp);
		while (!feof(pmu_battery_fp)) {
			char buf[32];

			if (fgets(buf, sizeof(buf), pmu_battery_fp) == NULL) {
				break;
			}

			if (buf[0] == 'f') {
				sscanf(buf, "flags      : %8x", &flags);
			} else if (buf[0] == 'c' && buf[1] == 'h') {
				sscanf(buf, "charge     : %d", &charge);
			} else if (buf[0] == 'm') {
				sscanf(buf, "max_charge : %d", &max_charge);
			} else if (buf[0] == 't') {
				sscanf(buf, "time rem.  : %ld", &timeval);
			}
		}
	}
	if (pmu_info_fp == NULL) {
		pmu_info_fp = open_file(info_path, &rep);
		if (pmu_info_fp == NULL) {
			return;
		}
	}

	if (pmu_info_fp != NULL) {
		rewind(pmu_info_fp);
		while (!feof(pmu_info_fp)) {
			char buf[32];

			if (fgets(buf, sizeof(buf), pmu_info_fp) == NULL) {
				break;
			}
			if (buf[0] == 'A') {
				sscanf(buf, "AC Power               : %d", &ac);
			}
		}
	}
	/* update status string */
	if ((ac && !(flags & PMU_BATT_PRESENT))) {
		strncpy(pb_battery_info[PB_BATT_STATUS], "AC", sizeof(pb_battery_info[PB_BATT_STATUS]));
	} else if (ac && (flags & PMU_BATT_PRESENT)
			&& !(flags & PMU_BATT_CHARGING)) {
		strncpy(pb_battery_info[PB_BATT_STATUS], "charged", sizeof(pb_battery_info[PB_BATT_STATUS]));
	} else if ((flags & PMU_BATT_PRESENT) && (flags & PMU_BATT_CHARGING)) {
		strncpy(pb_battery_info[PB_BATT_STATUS], "charging", sizeof(pb_battery_info[PB_BATT_STATUS]));
	} else {
		strncpy(pb_battery_info[PB_BATT_STATUS], "discharging", sizeof(pb_battery_info[PB_BATT_STATUS]));
	}

	/* update percentage string */
	if (timeval == 0 && ac && (flags & PMU_BATT_PRESENT)
			&& !(flags & PMU_BATT_CHARGING)) {
		snprintf(pb_battery_info[PB_BATT_PERCENT],
			sizeof(pb_battery_info[PB_BATT_PERCENT]), "100%%");
	} else if (timeval == 0) {
		snprintf(pb_battery_info[PB_BATT_PERCENT],
			sizeof(pb_battery_info[PB_BATT_PERCENT]), "unknown");
	} else {
		snprintf(pb_battery_info[PB_BATT_PERCENT],
			sizeof(pb_battery_info[PB_BATT_PERCENT]), "%d%%",
			(charge * 100) / max_charge);
	}

	/* update time string */
	if (timeval == 0) {			/* fully charged or battery not present */
		snprintf(pb_battery_info[PB_BATT_TIME],
			sizeof(pb_battery_info[PB_BATT_TIME]), "unknown");
	} else if (timeval < 60 * 60) {	/* don't show secs */
		format_seconds_short(pb_battery_info[PB_BATT_TIME],
			sizeof(pb_battery_info[PB_BATT_TIME]), timeval);
	} else {
		format_seconds(pb_battery_info[PB_BATT_TIME],
			sizeof(pb_battery_info[PB_BATT_TIME]), timeval);
	}

	snprintf(buffer, n, "%s", pb_battery_info[obj->data.i]);
}

#define ENTROPY_AVAIL_PATH "/proc/sys/kernel/random/entropy_avail"

int get_entropy_avail(unsigned int *val)
{
	static int rep = 0;
	FILE *fp;

	if (!(fp = open_file(ENTROPY_AVAIL_PATH, &rep)))
		return 1;

	if (fscanf(fp, "%u", val) != 1)
		return 1;

	fclose(fp);
	return 0;
}

#define ENTROPY_POOLSIZE_PATH "/proc/sys/kernel/random/poolsize"

int get_entropy_poolsize(unsigned int *val)
{
	static int rep = 0;
	FILE *fp;

	if (!(fp = open_file(ENTROPY_POOLSIZE_PATH, &rep)))
		return 1;

	if (fscanf(fp, "%u", val) != 1)
		return 1;

	fclose(fp);
	return 0;
}

void print_disk_protect_queue(struct text_object *obj, char *p, int p_max_size)
{
	FILE *fp;
	char path[128];
	int state;

	snprintf(path, 127, "/sys/block/%s/device/unload_heads", obj->data.s);
	if (access(path, F_OK)) {
		snprintf(path, 127, "/sys/block/%s/queue/protect", obj->data.s);
	}
	if ((fp = fopen(path, "r")) == NULL) {
		snprintf(p, p_max_size, "n/a   ");
		return;
	}
	if (fscanf(fp, "%d\n", &state) != 1) {
		fclose(fp);
		snprintf(p, p_max_size, "failed");
		return;
	}
	fclose(fp);
	snprintf(p, p_max_size, (state > 0) ? "frozen" : "free  ");
}

std::unordered_map<std::string, bool> dev_list;

/* Same as sf #2942117 but memoized using a linked list */
int is_disk(char *dev)
{
	std::string orig(dev);
	std::string syspath("/sys/block/");
	char *slash;

	auto i = dev_list.find(orig);
	if(i != dev_list.end())
		return i->second;

	while ((slash = strchr(dev, '/')))
		*slash = '!';
	syspath += dev;

	return dev_list[orig] = !(access(syspath.c_str(), F_OK));
}

int update_diskio(void)
{
	FILE *fp;
	static int rep = 0;
	char buf[512], devbuf[64];
	unsigned int major, minor;
	int col_count = 0;
	struct diskio_stat *cur;
	unsigned int reads, writes;
	unsigned int total_reads = 0, total_writes = 0;

	stats.current = 0;
	stats.current_read = 0;
	stats.current_write = 0;

	if (!(fp = open_file("/proc/diskstats", &rep))) {
		return 0;
	}

	/* read reads and writes from all disks (minor = 0), including cd-roms
	 * and floppies, and sum them up */
	while (fgets(buf, 512, fp)) {
		col_count = sscanf(buf, "%u %u %s %*u %*u %u %*u %*u %*u %u", &major,
			&minor, devbuf, &reads, &writes);
		/* ignore subdevices (they have only 3 matching entries in their line)
		 * and virtual devices (LVM, network block devices, RAM disks, Loopback)
		 *
		 * XXX: ignore devices which are part of a SW RAID (MD_MAJOR) */
		if (col_count == 5 && major != LVM_BLK_MAJOR && major != NBD_MAJOR
				&& major != RAMDISK_MAJOR && major != LOOP_MAJOR
				&& major != DM_MAJOR) {
			/* check needed for kernel >= 2.6.31, see sf #2942117 */
			if (is_disk(devbuf)) {
				total_reads += reads;
				total_writes += writes;
			}
		} else {
			col_count = sscanf(buf, "%u %u %s %*u %u %*u %u",
				&major, &minor, devbuf, &reads, &writes);
			if (col_count != 5) {
				continue;
			}
		}
		cur = stats.next;
		while (cur && strcmp(devbuf, cur->dev))
			cur = cur->next;

		if (cur)
			update_diskio_values(cur, reads, writes);
	}
	update_diskio_values(&stats, total_reads, total_writes);
	fclose(fp);
	return 0;
}

void print_distribution(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	int i, bytes_read;
	char* buf;
	struct stat sb;

	if(stat("/etc/arch-release", &sb) == 0) {
		snprintf(p, p_max_size, "Arch Linux");
		return;
	}
	snprintf(p, p_max_size, "Unknown");
	buf = readfile("/proc/version", &bytes_read, 1);
	if(buf) {
		/* I am assuming the distribution name is the first string in /proc/version that:
		- is preceded by a '('
		- starts with a capital
		- is followed by a space and a number
		but i am not sure if this is always true... */
		for(i=1; i<bytes_read; i++) {
			if(buf[i-1] == '(' && buf[i] >= 'A' && buf[i] <= 'Z') break;
		}
		if(i < bytes_read) {
			snprintf(p, p_max_size, "%s", &buf[i]);
			for(i=1; p[i]; i++) {
				if(p[i-1] == ' ' && p[i] >= '0' && p[i] <= '9') {
					p[i-1] = 0;
					break;
				}
			}
		}
		free(buf);
	}
}

/******************************************
 * Calculate cpu total					  *
 ******************************************/
#define TMPL_SHORTPROC "%*s %llu %llu %llu %llu"
#define TMPL_LONGPROC "%*s %llu %llu %llu %llu %llu %llu %llu %llu"

static unsigned long long calc_cpu_total(void)
{
	static unsigned long long previous_total = 0;
	unsigned long long total = 0;
	unsigned long long t = 0;
	int rc;
	int ps;
	char line[BUFFER_LEN] = { 0 };
	unsigned long long cpu = 0;
	unsigned long long niceval = 0;
	unsigned long long systemval = 0;
	unsigned long long idle = 0;
	unsigned long long iowait = 0;
	unsigned long long irq = 0;
	unsigned long long softirq = 0;
	unsigned long long steal = 0;
	const char *template_ =
		KFLAG_ISSET(KFLAG_IS_LONGSTAT) ? TMPL_LONGPROC : TMPL_SHORTPROC;

	ps = open("/proc/stat", O_RDONLY);
	rc = read(ps, line, BUFFER_LEN - 1);
	close(ps);
	if (rc < 0) {
		return 0;
	}

	sscanf(line, template_, &cpu, &niceval, &systemval, &idle, &iowait, &irq,
			&softirq, &steal);
	total = cpu + niceval + systemval + idle + iowait + irq + softirq + steal;

	t = total - previous_total;
	previous_total = total;

	return t;
}

/******************************************
 * Calculate each processes cpu			  *
 ******************************************/

inline static void calc_cpu_each(unsigned long long total)
{
	float mul = 100.0;
	if(top_cpu_separate.get(*state))
		mul *= info.cpu_count;

	for(struct process *p = first_process; p; p = p->next)
		p->amount = mul * (p->user_time + p->kernel_time) / (float) total;
}

#ifdef BUILD_IOSTATS
static void calc_io_each(void)
{
	struct process *p;
	unsigned long long sum = 0;

	for (p = first_process; p; p = p->next)
		sum += p->read_bytes + p->write_bytes;

	if(sum == 0)
		sum = 1; /* to avoid having NANs if no I/O occured */
	for (p = first_process; p; p = p->next)
		p->io_perc = 100.0 * (p->read_bytes + p->write_bytes) / (float) sum;
}
#endif /* BUILD_IOSTATS */

/******************************************
 * Extract information from /proc		  *
 ******************************************/

#define PROCFS_TEMPLATE "/proc/%d/stat"
#define PROCFS_CMDLINE_TEMPLATE "/proc/%d/cmdline"

/* These are the guts that extract information out of /proc.
 * Anyone hoping to port wmtop should look here first. */
static void process_parse_stat(struct process *process)
{
	char line[BUFFER_LEN] = { 0 }, filename[BUFFER_LEN], procname[BUFFER_LEN];
	char cmdline[BUFFER_LEN] = { 0 }, cmdline_filename[BUFFER_LEN], cmdline_procname[BUFFER_LEN];
	char basename[BUFFER_LEN] = { 0 };
	char tmpstr[BUFFER_LEN] = { 0 };
	char state[4];
	int ps, cmdline_ps;
	unsigned long user_time = 0;
	unsigned long kernel_time = 0;
	int rc;
	char *r, *q;
	int endl;
	int nice_val;
	char *lparen, *rparen;
	struct stat process_stat;

	snprintf(filename, sizeof(filename), PROCFS_TEMPLATE, process->pid);
	snprintf(cmdline_filename, sizeof(cmdline_filename), PROCFS_CMDLINE_TEMPLATE, process->pid);

	ps = open(filename, O_RDONLY);
	if (ps < 0) {
		/* The process must have finished in the last few jiffies! */
		return;
	}

	if (fstat(ps, &process_stat) != 0)
		return;
	process->uid=process_stat.st_uid;

	/* Mark process as up-to-date. */
	process->time_stamp = g_time;

	rc = read(ps, line, BUFFER_LEN - 1);
	close(ps);
	if (rc < 0) {
		return;
	}

	/* Read /proc/<pid>/cmdline */
	cmdline_ps = open(cmdline_filename, O_RDONLY);
	if (cmdline_ps < 0) {
		/* The process must have finished in the last few jiffies! */
		return;
	}

	endl = read(cmdline_ps, cmdline, BUFFER_LEN - 1);
	close(cmdline_ps);
	if (endl < 0) {
		return;
	}

	/* Some processes have null-separated arguments (see proc(5)); let's fix it */
	int i = endl;
	while (i && cmdline[i-1] == 0) {
		/* Skip past any trailing null characters */
		--i;
	}
	while (i--) {
		/* Replace null character between arguments with a space */
		if (cmdline[i] == 0) {
			cmdline[i] = ' ';
		}
	}

	cmdline[endl] = 0;

	/* We want to transform for example "/usr/bin/python program.py" to "python program.py"
	 * 1. search for first space
	 * 2. search for last / before first space
	 * 3. copy string from its position
	 */
	char *space_ptr = strchr(cmdline, ' ');
	if (space_ptr == NULL) {
		strcpy(tmpstr, cmdline);
	} else {
		long int space_pos = space_ptr - cmdline;
		strncpy(tmpstr, cmdline, space_pos);
		tmpstr[space_pos] = 0;
	}

	char *slash_ptr = strrchr(tmpstr, '/');
	if (slash_ptr == NULL) {
		strncpy(cmdline_procname, cmdline, BUFFER_LEN);
	} else {
		long int slash_pos = slash_ptr - tmpstr;
		strncpy(cmdline_procname, cmdline + slash_pos + 1, BUFFER_LEN - slash_pos);
		cmdline_procname[BUFFER_LEN - slash_pos] = 0;
	}

	/* Extract cpu times from data in /proc filesystem */
	lparen = strchr(line, '(');
	rparen = strrchr(line, ')');
	if (!lparen || !rparen || rparen < lparen)
		return; // this should not happen

	rc = MIN((unsigned)(rparen - lparen - 1), sizeof(procname) - 1);
	strncpy(procname, lparen + 1, rc);
	procname[rc] = '\0';
	strncpy(basename, procname, strlen(procname) + 1);

	if (strlen(procname) < strlen(cmdline_procname))
		strncpy(procname, cmdline_procname, strlen(cmdline_procname) + 1);

	rc = sscanf(rparen + 1, "%3s %*s %*s %*s %*s %*s %*s %*s %*s %*s %*s %lu "
			"%lu %*s %*s %*s %d %*s %*s %*s %llu %llu", state, &process->user_time,
			&process->kernel_time, &nice_val, &process->vsize, &process->rss);
	if (rc < 6) {
		NORM_ERR("scaning data for %s failed, got only %d fields", procname, rc);
		return;
	}

	if (state[0] == 'R')
		++ info.run_procs;

	free_and_zero(process->name);
	free_and_zero(process->basename);
	process->name = strndup(procname, text_buffer_size.get(*::state));
	process->basename = strndup(basename, text_buffer_size.get(*::state));
	process->rss *= getpagesize();

	process->total_cpu_time = process->user_time + process->kernel_time;
	if (process->previous_user_time == ULONG_MAX) {
		process->previous_user_time = process->user_time;
	}
	if (process->previous_kernel_time == ULONG_MAX) {
		process->previous_kernel_time = process->kernel_time;
	}

	/* strangely, the values aren't monotonous */
	if (process->previous_user_time > process->user_time)
		process->previous_user_time = process->user_time;

	if (process->previous_kernel_time > process->kernel_time)
		process->previous_kernel_time = process->kernel_time;

	/* store the difference of the user_time */
	user_time = process->user_time - process->previous_user_time;
	kernel_time = process->kernel_time - process->previous_kernel_time;

	/* backup the process->user_time for next time around */
	process->previous_user_time = process->user_time;
	process->previous_kernel_time = process->kernel_time;

	/* store only the difference of the user_time here... */
	process->user_time = user_time;
	process->kernel_time = kernel_time;
}

#ifdef BUILD_IOSTATS
#define PROCFS_TEMPLATE_IO "/proc/%d/io"
static void process_parse_io(struct process *process)
{
	static const char *read_bytes_str="read_bytes:";
	static const char *write_bytes_str="write_bytes:";

	char line[BUFFER_LEN] = { 0 }, filename[BUFFER_LEN];
	int ps;
	int rc;
	char *pos, *endpos;
	unsigned long long read_bytes, write_bytes;

	snprintf(filename, sizeof(filename), PROCFS_TEMPLATE_IO, process->pid);

	ps = open(filename, O_RDONLY);
	if (ps < 0) {
		/* The process must have finished in the last few jiffies!
		 * Or, the kernel doesn't support I/O accounting.
		 */
		return;
	}

	rc = read(ps, line, BUFFER_LEN - 1);
	close(ps);
	if (rc < 0) {
		return;
	}

	pos = strstr(line, read_bytes_str);
	if (pos == NULL) {
		/* these should not happen (unless the format of the file changes) */
		return;
	}
	pos += strlen(read_bytes_str);
	process->read_bytes = strtoull(pos, &endpos, 10);
	if (endpos == pos) {
		return;
	}

	pos = strstr(line, write_bytes_str);
	if (pos == NULL) {
		return;
	}
	pos += strlen(write_bytes_str);
	process->write_bytes = strtoull(pos, &endpos, 10);
	if (endpos == pos) {
		return;
	}

	if (process->previous_read_bytes == ULLONG_MAX) {
		process->previous_read_bytes = process->read_bytes;
	}
	if (process->previous_write_bytes == ULLONG_MAX) {
		process->previous_write_bytes = process->write_bytes;
	}

	/* store the difference of the byte counts */
	read_bytes = process->read_bytes - process->previous_read_bytes;
	write_bytes = process->write_bytes - process->previous_write_bytes;

	/* backup the counts for next time around */
	process->previous_read_bytes = process->read_bytes;
	process->previous_write_bytes = process->write_bytes;

	/* store only the difference here... */
	process->read_bytes = read_bytes;
	process->write_bytes = write_bytes;
}
#endif /* BUILD_IOSTATS */

/******************************************
 * Get process structure for process pid  *
 ******************************************/

/* This function seems to hog all of the CPU time.
 * I can't figure out why - it doesn't do much. */
static void calculate_stats(struct process *process)
{
	/* compute each process cpu usage by reading /proc/<proc#>/stat */
	process_parse_stat(process);

#ifdef BUILD_IOSTATS
	process_parse_io(process);
#endif /* BUILD_IOSTATS */

	/*
	 * Check name against the exclusion list
	 */
	/* if (process->counted && exclusion_expression &&
	 * !regexec(exclusion_expression, process->name, 0, 0, 0))
	 * process->counted = 0; */
}

/******************************************
 * Update process table					  *
 ******************************************/

static void update_process_table(void)
{
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir("/proc"))) {
		return;
	}

	info.run_procs = 0;

	/* Get list of processes from /proc directory */
	while ((entry = readdir(dir))) {
		pid_t pid;

		if (!entry) {
			/* Problem reading list of processes */
			closedir(dir);
			return;
		}

		if (sscanf(entry->d_name, "%d", &pid) > 0) {
		/* compute each process cpu usage */
			calculate_stats(get_process(pid));
		}
	}

	closedir(dir);
}

void get_top_info(void)
{
	unsigned long long total = 0;

	total = calc_cpu_total();	/* calculate the total of the processor */
	update_process_table();		/* update the table with process list */
	calc_cpu_each(total);		/* and then the percentage for each task */
#ifdef BUILD_IOSTATS
	calc_io_each();			/* percentage of I/O for each task */
#endif /* BUILD_IOSTATS */
}
