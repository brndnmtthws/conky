/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2007 Toni Spets
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
 */

#include "conky.h"
#include "logging.h"
#include "common.h"
#include "linux.h"
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

#ifdef HAVE_IWLIB
#include <iwlib.h>
#endif

#define SHORTSTAT_TEMPL "%*s %llu %llu %llu"
#define LONGSTAT_TEMPL "%*s %llu %llu %llu "

static int show_nice_processes;

/* This flag tells the linux routines to use the /proc system where possible,
 * even if other api's are available, e.g. sysinfo() or getloadavg().
 * the reason for this is to allow for /proc-based distributed monitoring.
 * using a flag in this manner creates less confusing code. */
static int prefer_proc = 0;

void prepare_update(void)
{
}

void update_uptime(void)
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
			return;
		}
		fscanf(fp, "%lf", &info.uptime);
		fclose(fp);
	}
	info.mask |= (1 << INFO_UPTIME);
}

int check_mount(char *s)
{
	int ret = 0;
	FILE *mtab = fopen("/etc/mtab", "r");

	if (mtab) {
		char buf1[256], buf2[128];

		while (fgets(buf1, 256, mtab)) {
			sscanf(buf1, "%*s %128s", buf2);
			if (!strcmp(s, buf2)) {
				ret = 1;
				break;
			}
		}
		fclose(mtab);
	} else {
		ERR("Could not open mtab");
	}
	return ret;
}

/* these things are also in sysinfo except Buffers:
 * (that's why I'm reading them from proc) */

void update_meminfo(void)
{
	FILE *meminfo_fp;
	static int rep = 0;

	/* unsigned int a; */
	char buf[256];

	info.mem = info.memmax = info.swap = info.swapmax = info.bufmem =
		info.buffers = info.cached = info.memfree = info.memeasyfree = 0;

	if (!(meminfo_fp = open_file("/proc/meminfo", &rep))) {
		return;
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
			sscanf(buf, "%*s %llu", &info.swap);
		} else if (strncmp(buf, "Buffers:", 8) == 0) {
			sscanf(buf, "%*s %llu", &info.buffers);
		} else if (strncmp(buf, "Cached:", 7) == 0) {
			sscanf(buf, "%*s %llu", &info.cached);
		}
	}

	info.mem = info.memmax - info.memfree;
	info.memeasyfree = info.memfree;
	info.swap = info.swapmax - info.swap;

	info.bufmem = info.cached + info.buffers;

	info.mask |= (1 << INFO_MEM) | (1 << INFO_BUFFERS);

	fclose(meminfo_fp);
}

int get_laptop_mode(void)
{
	FILE *fp;
	int val = -1;

	if ((fp = fopen("/proc/sys/vm/laptop_mode", "r")) != NULL)
		fscanf(fp, "%d\n", &val);
	fclose(fp);
	return val;
}

/* my system says:
 * # cat /sys/block/sda/queue/scheduler
 * noop [anticipatory] cfq
 */
char *get_ioscheduler(char *disk)
{
	FILE *fp;
	char buf[128];

	if (!disk)
		return strndup("n/a", text_buffer_size);

	snprintf(buf, 127, "/sys/block/%s/queue/scheduler", disk);
	if ((fp = fopen(buf, "r")) == NULL) {
		return strndup("n/a", text_buffer_size);
	}
	while (!feof(fp)) {
		fscanf(fp, "%127s", buf);
		if (buf[0] == '[') {
			buf[strlen(buf) - 1] = '\0';
			fclose(fp);
			return strndup(buf + 1, text_buffer_size);
		}
	}
	fclose(fp);
	return strndup("n/a", text_buffer_size);
}

int interface_up(const char *dev)
{
	int fd;
	struct ifreq ifr;

	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		CRIT_ERR("could not create sockfd");
		return 0;
	}
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr)) {
		/* if device does not exist, treat like not up */
		if (errno != ENODEV)
			perror("SIOCGIFFLAGS");
		goto END_FALSE;
	}

	if (!(ifr.ifr_flags & IFF_UP)) /* iface is not up */
		goto END_FALSE;
	if (ifup_strictness == IFUP_UP)
		goto END_TRUE;

	if (!(ifr.ifr_flags & IFF_RUNNING))
		goto END_FALSE;
	if (ifup_strictness == IFUP_LINK)
		goto END_TRUE;

	if (ioctl(fd, SIOCGIFADDR, &ifr)) {
		perror("SIOCGIFADDR");
		goto END_FALSE;
	}
	if (((struct sockaddr_in *)&(ifr.ifr_ifru.ifru_addr))->sin_addr.s_addr)
		goto END_TRUE;

END_FALSE:
	close(fd);
	return 0;
END_TRUE:
	close(fd);
	return 1;
}

#define COND_FREE(x) if(x) free(x); x = 0
#define SAVE_SET_STRING(x, y) \
	if (x && strcmp((char *)x, (char *)y)) { \
		free(x); \
		x = strndup("multiple", text_buffer_size); \
	} else if (!x) { \
		x = strndup(y, text_buffer_size); \
	}

void update_gateway_info_failure(const char *reason)
{
	if(reason != NULL) {
		perror(reason);
	}
	//2 pointers to 1 location causes a crash when we try to free them both
	info.gw_info.iface = strndup("failed", text_buffer_size);
	info.gw_info.ip = strndup("failed", text_buffer_size);
}


/* Iface Destination Gateway Flags RefCnt Use Metric Mask MTU Window IRTT */
#define RT_ENTRY_FORMAT "%63s %lx %lx %x %*d %*d %*d %lx %*d %*d %*d\n"

void update_gateway_info(void)
{
	FILE *fp;
	struct in_addr ina;
	char iface[64];
	unsigned long dest, gate, mask;
	unsigned int flags;

	struct gateway_info *gw_info = &info.gw_info;

	COND_FREE(gw_info->iface);
	COND_FREE(gw_info->ip);
	gw_info->count = 0;

	if ((fp = fopen("/proc/net/route", "r")) == NULL) {
		update_gateway_info_failure("fopen()");
		return;
	}

	/* skip over the table header line, which is always present */
	fscanf(fp, "%*[^\n]\n");

	while (!feof(fp)) {
		if(fscanf(fp, RT_ENTRY_FORMAT,
			  iface, &dest, &gate, &flags, &mask) != 5) {
			update_gateway_info_failure("fscanf()");
			break;
		}
		if (!(dest || mask) && ((flags & RTF_GATEWAY) || !gate) ) {
			gw_info->count++;
			SAVE_SET_STRING(gw_info->iface, iface)
			ina.s_addr = gate;
			SAVE_SET_STRING(gw_info->ip, inet_ntoa(ina))
		}
	}
	fclose(fp);
	return;
}

void update_net_stats(void)
{
	FILE *net_dev_fp;
	static int rep = 0;

	// FIXME: arbitrary size chosen to keep code simple.
	int i, i2;
	unsigned int curtmp1, curtmp2;
	unsigned int k;
	struct ifconf conf;
	char buf[256];
	double delta;

#ifdef HAVE_IWLIB
	// wireless info variables
	int skfd, has_bitrate = 0;
	struct wireless_info *winfo;
	struct iwreq wrq;
#endif

	/* get delta */
	delta = current_update_time - last_update_time;
	if (delta <= 0.0001) {
		return;
	}

	/* open file and ignore first two lines */
	if (!(net_dev_fp = open_file("/proc/net/dev", &rep))) {
		clear_net_stats();
		return;
	}

	fgets(buf, 255, net_dev_fp);	/* garbage */
	fgets(buf, 255, net_dev_fp);	/* garbage (field names) */

	/* read each interface */
	for (i2 = 0; i2 < 16; i2++) {
		struct net_stat *ns;
		char *s, *p;
		char temp_addr[18];
		long long r, t, last_recv, last_trans;

		if (fgets(buf, 255, net_dev_fp) == NULL) {
			break;
		}
		p = buf;
		while (isspace((int) *p)) {
			p++;
		}

		s = p;

		while (*p && *p != ':') {
			p++;
		}
		if (*p == '\0') {
			continue;
		}
		*p = '\0';
		p++;

		ns = get_net_stat(s);
		ns->up = 1;
		memset(&(ns->addr.sa_data), 0, 14);

		if(NULL == ns->addrs)
			ns->addrs = (char*) malloc(17 * 16 + 1);
		if(NULL != ns->addrs)
			memset(ns->addrs, 0, 17 * 16 + 1); /* Up to 17 chars per ip, max 16 interfaces. Nasty memory usage... */

		last_recv = ns->recv;
		last_trans = ns->trans;

		/* bytes packets errs drop fifo frame compressed multicast|bytes ... */
		sscanf(p, "%lld  %*d     %*d  %*d  %*d  %*d   %*d        %*d       %lld",
			&r, &t);

		/* if recv or trans is less than last time, an overflow happened */
		if (r < ns->last_read_recv) {
			last_recv = 0;
		} else {
			ns->recv += (r - ns->last_read_recv);
		}
		ns->last_read_recv = r;

		if (t < ns->last_read_trans) {
			last_trans = 0;
		} else {
			ns->trans += (t - ns->last_read_trans);
		}
		ns->last_read_trans = t;

		/*** ip addr patch ***/
		i = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

		conf.ifc_buf = malloc(sizeof(struct ifreq) * 16);
		conf.ifc_len = sizeof(struct ifreq) * 16;
		memset(conf.ifc_buf, 0, conf.ifc_len);

		ioctl((long) i, SIOCGIFCONF, &conf);

		for (k = 0; k < conf.ifc_len / sizeof(struct ifreq); k++) {
			struct net_stat *ns2;

			if (!(((struct ifreq *) conf.ifc_buf) + k))
				break;

			ns2 = get_net_stat(
				((struct ifreq *) conf.ifc_buf)[k].ifr_ifrn.ifrn_name);
			ns2->addr = ((struct ifreq *) conf.ifc_buf)[k].ifr_ifru.ifru_addr;
			if(NULL != ns2->addrs) {
				sprintf(temp_addr, "%u.%u.%u.%u, ",
					ns2->addr.sa_data[2] & 255,
					ns2->addr.sa_data[3] & 255,
					ns2->addr.sa_data[4] & 255,
					ns2->addr.sa_data[5] & 255);
				if(NULL == strstr(ns2->addrs, temp_addr))
					strncpy(ns2->addrs + strlen(ns2->addrs), temp_addr, 17);
			}
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
		if (curtmp1 == 0) {
			curtmp1 = 1;
		}
		if (curtmp2 == 0) {
			curtmp2 = 1;
		}
		ns->recv_speed = curtmp1 / (double) info.net_avg_samples;
		ns->trans_speed = curtmp2 / (double) info.net_avg_samples;
		if (info.net_avg_samples > 1) {
			for (i = info.net_avg_samples; i > 1; i--) {
				ns->net_rec[i - 1] = ns->net_rec[i - 2];
				ns->net_trans[i - 1] = ns->net_trans[i - 2];
			}
		}

#ifdef HAVE_IWLIB
		/* update wireless info */
		winfo = malloc(sizeof(struct wireless_info));
		memset(winfo, 0, sizeof(struct wireless_info));

		skfd = iw_sockets_open();
		if (iw_get_basic_config(skfd, s, &(winfo->b)) > -1) {

			// set present winfo variables
			if (iw_get_stats(skfd, s, &(winfo->stats),
					&winfo->range, winfo->has_range) >= 0) {
				winfo->has_stats = 1;
			}
			if (iw_get_range_info(skfd, s, &(winfo->range)) >= 0) {
				winfo->has_range = 1;
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

			snprintf(ns->mode, 16, "%s", iw_operation_mode[winfo->b.mode]);
		}
		iw_sockets_close(skfd);
		free(winfo);
#endif
	}

	fclose(net_dev_fp);

	info.mask |= (1 << INFO_NET);
}

int result;

void update_total_processes(void)
{
#ifdef HAVE_SYSINFO
	if (!prefer_proc) {
		struct sysinfo s_info;

		sysinfo(&s_info);
		info.procs = s_info.procs;
	} else
#endif
	{
		static int rep = 0;
		FILE *fp;

		if (!(fp = open_file("/proc/loadavg", &rep))) {
			info.procs = 0;
			return;
		}
		fscanf(fp, "%*f %*f %*f %*d/%hu", &info.procs);
		fclose(fp);
	}
	info.mask |= (1 << INFO_PROCS);
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

void get_cpu_count(void)
{
	FILE *stat_fp;
	static int rep = 0;
	char buf[256];

	if (info.cpu_usage) {
		return;
	}

	if (!(stat_fp = open_file("/proc/stat", &rep))) {
		return;
	}

	info.cpu_count = 0;

	while (!feof(stat_fp)) {
		if (fgets(buf, 255, stat_fp) == NULL) {
			break;
		}

		if (strncmp(buf, "cpu", 3) == 0 && isdigit(buf[3])) {
			if (info.cpu_count == 0) {
				determine_longstat(buf);
			}
			info.cpu_count++;
		}
	}
	info.cpu_usage = malloc((info.cpu_count + 1) * sizeof(float));

	fclose(stat_fp);
}

#define TMPL_LONGSTAT "%*s %llu %llu %llu %llu %llu %llu %llu %llu"
#define TMPL_SHORTSTAT "%*s %llu %llu %llu %llu"

inline static void update_stat(void)
{
	FILE *stat_fp;
	static int rep = 0;
	static struct cpu_info *cpu = NULL;
	char buf[256];
	unsigned int i;
	unsigned int idx;
	double curtmp;
	const char *stat_template = NULL;
	unsigned int malloc_cpu_size = 0;

	/* add check for !info.cpu_usage since that mem is freed on a SIGUSR1 */
	if (!cpu_setup || !info.cpu_usage) {
		get_cpu_count();
		cpu_setup = 1;
	}

	if (!stat_template) {
		stat_template =
			KFLAG_ISSET(KFLAG_IS_LONGSTAT) ? TMPL_LONGSTAT : TMPL_SHORTSTAT;
	}

	if (!cpu) {
		malloc_cpu_size = (info.cpu_count + 1) * sizeof(struct cpu_info);
		cpu = malloc(malloc_cpu_size);
		memset(cpu, 0, malloc_cpu_size);
	}

	if (!(stat_fp = open_file("/proc/stat", &rep))) {
		info.run_procs = 0;
		if (info.cpu_usage) {
			memset(info.cpu_usage, 0, info.cpu_count * sizeof(float));
		}
		return;
	}

	idx = 0;
	while (!feof(stat_fp)) {
		if (fgets(buf, 255, stat_fp) == NULL) {
			break;
		}

		if (strncmp(buf, "procs_running ", 14) == 0) {
			sscanf(buf, "%*s %hu", &info.run_procs);
			info.mask |= (1 << INFO_RUN_PROCS);
		} else if (strncmp(buf, "cpu", 3) == 0) {
			double delta;
			if (isdigit(buf[3])) {
				idx = atoi(&buf[3]) + 1;
			} else {
				idx = 0;
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
			info.mask |= (1 << INFO_CPU);

			delta = current_update_time - last_update_time;

			if (delta <= 0.001) {
				break;
			}

			cpu[idx].cpu_val[0] = (cpu[idx].cpu_active_total -
				cpu[idx].cpu_last_active_total) /
				(float) (cpu[idx].cpu_total - cpu[idx].cpu_last_total);
			curtmp = 0;
			for (i = 0; i < info.cpu_avg_samples; i++) {
				curtmp += cpu[idx].cpu_val[i];
			}
			/* TESTING -- I've removed this, because I don't think it is right.
			 * You shouldn't divide by the cpu count here ...
			 * removing for testing */
			/* if (idx == 0) {
				info.cpu_usage[idx] = curtmp / info.cpu_avg_samples /
					info.cpu_count;
			} else {
				info.cpu_usage[idx] = curtmp / info.cpu_avg_samples;
			} */
			/* TESTING -- this line replaces the prev. "suspect" if/else */
			info.cpu_usage[idx] = curtmp / info.cpu_avg_samples;

			cpu[idx].cpu_last_total = cpu[idx].cpu_total;
			cpu[idx].cpu_last_active_total = cpu[idx].cpu_active_total;
			for (i = info.cpu_avg_samples - 1; i > 0; i--) {
				cpu[idx].cpu_val[i] = cpu[idx].cpu_val[i - 1];
			}
		}
	}
	fclose(stat_fp);
}

void update_running_processes(void)
{
	update_stat();
}

void update_cpu_usage(void)
{
	update_stat();
}

void update_load_average(void)
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
			return;
		}
		fscanf(fp, "%f %f %f", &info.loadavg[0], &info.loadavg[1],
			&info.loadavg[2]);
		fclose(fp);
	}
	info.mask |= (1 << INFO_LOADAVG);
}

#define PROC_I8K "/proc/i8k"
#define I8K_DELIM " "
static char *i8k_procbuf = NULL;
void update_i8k(void)
{
	FILE *fp;

	if (!i8k_procbuf) {
		i8k_procbuf = (char *) malloc(128 * sizeof(char));
	}
	if ((fp = fopen(PROC_I8K, "r")) == NULL) {
		CRIT_ERR("/proc/i8k doesn't exist! use insmod to make sure the kernel "
			"driver is loaded...");
	}

	memset(&i8k_procbuf[0], 0, 128);
	if (fread(&i8k_procbuf[0], sizeof(char), 128, fp) == 0) {
		ERR("something wrong with /proc/i8k...");
	}

	fclose(fp);

	i8k.version = strtok(&i8k_procbuf[0], I8K_DELIM);
	i8k.bios = strtok(NULL, I8K_DELIM);
	i8k.serial = strtok(NULL, I8K_DELIM);
	i8k.cpu_temp = strtok(NULL, I8K_DELIM);
	i8k.left_fan_status = strtok(NULL, I8K_DELIM);
	i8k.right_fan_status = strtok(NULL, I8K_DELIM);
	i8k.left_fan_rpm = strtok(NULL, I8K_DELIM);
	i8k.right_fan_rpm = strtok(NULL, I8K_DELIM);
	i8k.ac_status = strtok(NULL, I8K_DELIM);
	i8k.buttons_status = strtok(NULL, I8K_DELIM);
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
			ERR("scandir for %s: %s", dir, strerror(errno));
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

		for (i = 0; i < n; i++) {
			free(namelist[i]);
		}
		free(namelist);

		return 1;
	}
}

int open_sysfs_sensor(const char *dir, const char *dev, const char *type, int n,
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

	/* change vol to in */
	if (strcmp(type, "vol") == 0) {
		type = "in";
	}

	if (strcmp(type, "tempf") == 0) {
		snprintf(path, 255, "%s%s/%s%d_input", dir, dev, "temp", n);
	} else {
		snprintf(path, 255, "%s%s/%s%d_input", dir, dev, type, n);
	}
	strncpy(devtype, path, 255);

	/* open file */
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		CRIT_ERR("can't open '%s': %s\nplease check your device or remove this "
			"var from "PACKAGE_NAME, path, strerror(errno));
	}

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
			ERR("open_sysfs_sensor(): can't read from sysfs");
		} else {
			divbuf[divn] = '\0';
			*divisor = atoi(divbuf);
		}
	}

	close(divfd);

	return fd;
}

double get_sysfs_info(int *fd, int divisor, char *devtype, char *type)
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
			ERR("get_sysfs_info(): read from %s failed\n", devtype);
		} else {
			buf[n] = '\0';
			val = atoi(buf);
		}
	}

	close(*fd);
	/* open file */
	*fd = open(devtype, O_RDONLY);
	if (*fd < 0) {
		ERR("can't open '%s': %s", devtype, strerror(errno));
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

/* Prior to kernel version 2.6.12, the CPU fan speed was available in
 * ADT746X_FAN_OLD, whereas later kernel versions provide this information in
 * ADT746X_FAN. */
#define ADT746X_FAN "/sys/devices/temperatures/sensor1_fan_speed"
#define ADT746X_FAN_OLD "/sys/devices/temperatures/cpu_fan_speed"

void get_adt746x_fan(char *p_client_buffer, size_t client_buffer_size)
{
	static int rep = 0;
	char adt746x_fan_state[64];
	FILE *fp;

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	if ((fp = open_file(ADT746X_FAN, &rep)) == NULL
			&& (fp = open_file(ADT746X_FAN_OLD, &rep)) == NULL) {
		sprintf(adt746x_fan_state, "adt746x not found");
	} else {
		fgets(adt746x_fan_state, sizeof(adt746x_fan_state), fp);
		adt746x_fan_state[strlen(adt746x_fan_state) - 1] = 0;
		fclose(fp);
	}

	snprintf(p_client_buffer, client_buffer_size, "%s", adt746x_fan_state);
}

/* Prior to kernel version 2.6.12, the CPU temperature was found in
 * ADT746X_CPU_OLD, whereas later kernel versions provide this information in
 * ADT746X_CPU. */
#define ADT746X_CPU "/sys/devices/temperatures/sensor1_temperature"
#define ADT746X_CPU_OLD "/sys/devices/temperatures/cpu_temperature"

void get_adt746x_cpu(char *p_client_buffer, size_t client_buffer_size)
{
	static int rep = 0;
	char adt746x_cpu_state[64];
	FILE *fp;

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	if ((fp = open_file(ADT746X_CPU, &rep)) == NULL
			&& (fp = open_file(ADT746X_CPU_OLD, &rep)) == NULL) {
		sprintf(adt746x_cpu_state, "adt746x not found");
	} else {
		fscanf(fp, "%2s", adt746x_cpu_state);
		fclose(fp);
	}

	snprintf(p_client_buffer, client_buffer_size, "%s", adt746x_cpu_state);
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
char get_voltage(char *p_client_buffer, size_t client_buffer_size,
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
	fscanf(fp, "%*s %99s", buf);
	fclose(fp);

	snprintf(p_client_buffer, client_buffer_size, "%s", buf);
}

#define SYSFS_AC_ADAPTER_DIR "/sys/class/power_supply/AC"
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
*/

void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size)
{
	static int rep = 0;

	char buf[256];
	char buf2[256];
	FILE *fp;

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	snprintf(buf2, sizeof(buf2), "%s/uevent", SYSFS_AC_ADAPTER_DIR);
	fp = open_file(buf2, &rep);
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
		fscanf(fp, "%*s %99s", buf);
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

#define ACPI_THERMAL_DIR "/proc/acpi/thermal_zone/"
#define ACPI_THERMAL_FORMAT "/proc/acpi/thermal_zone/%s/temperature"

int open_acpi_temperature(const char *name)
{
	char path[256];
	char buf[256];
	int fd;

	if (name == NULL || strcmp(name, "*") == 0) {
		static int rep = 0;

		if (!get_first_file_in_a_directory(ACPI_THERMAL_DIR, buf, &rep)) {
			return -1;
		}
		name = buf;
	}

	snprintf(path, 255, ACPI_THERMAL_FORMAT, name);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		ERR("can't open '%s': %s", path, strerror(errno));
	}

	return fd;
}

static double last_acpi_temp;
static double last_acpi_temp_time;

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
		char buf[256];
		int n;

		n = read(fd, buf, 255);
		if (n < 0) {
			ERR("can't read fd %d: %s", fd, strerror(errno));
		} else {
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
 			/* present_rate is not the same as the
 			current flowing now but it is the same value
 			which was used in the past. so we continue
 			the tradition! */
 			else if (strncmp(buf, "POWER_SUPPLY_CURRENT_NOW=", 25) == 0)
 				sscanf(buf, "POWER_SUPPLY_CURRENT_NOW=%d", &present_rate);
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
 		else if (strncmp(charging_state, "Charged", 64) == 0) {
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
 				strncpy(last_battery_str[idx], "AC", 64);
 		}
 	} else if (acpi_bat_fp[idx] != NULL) {
 		/* ACPI */
		int present_rate = -1;
		int remaining_capacity = -1;
		char charging_state[64];
		char present[4];

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
					sizeof(last_battery_str[idx]) - 1, "full");
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
			if (acpi_last_full[idx] != 0
					&& remaining_capacity != acpi_last_full[idx]) {
				snprintf(last_battery_str[idx], 64, "unknown %d%%",
					(int) ((remaining_capacity * 100) / acpi_last_full[idx]));
			} else {
				strncpy(last_battery_str[idx], "AC", 64);
			}
		}
	} else {
		/* APM */
		if (apm_bat_fp[idx] == NULL) {
			apm_bat_fp[idx] = open_file(APM_PATH, &rep2);
		}

		if (apm_bat_fp[idx] != NULL) {
			unsigned int ac, status, flag;
			int life;

			fscanf(apm_bat_fp[idx], "%*s %*s %*x %x   %x       %x     %d%%",
				&ac, &status, &flag, &life);

			if (life == -1) {
				/* could check now that there is ac */
				snprintf(last_battery_str[idx], 64, "AC");

			/* could check that status == 3 here? */
			} else if (ac && life != 100) {
				snprintf(last_battery_str[idx], 64, "charging %d%%", life);
			} else {
				snprintf(last_battery_str[idx], 64, "%d%%", life);
			}

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

int get_battery_perct(const char *bat)
{
	static int rep = 0;
	int idx;
	char acpi_path[128];
	char sysfs_path[128];
	int remaining_capacity = -1;

	snprintf(acpi_path, 127, ACPI_BATTERY_BASE_PATH "/%s/state", bat);
	snprintf(sysfs_path, 127, SYSFS_BATTERY_BASE_PATH "/%s/uevent", bat);

	init_batteries();

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

int get_battery_perct_bar(const char *bar)
{
	int idx;

	get_battery_perct(bar);
	idx = get_battery_idx(bar);
	return (int) (last_battery_perct[idx] * 2.56 - 1);
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
void get_powerbook_batt_info(char *buffer, size_t n, int i)
{
	static int rep = 0;
	const char *batt_path = PMU_PATH "/battery_0";
	const char *info_path = PMU_PATH "/info";
	unsigned int flags;
	int charge, max_charge, ac = -1;
	long timeval = -1;

	/* don't update battery too often */
	if (current_update_time - pb_battery_info_update < 29.5) {
		snprintf(buffer, n, "%s", pb_battery_info[i]);
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

	snprintf(buffer, n, "%s", pb_battery_info[i]);
}

void update_top(void)
{
	show_nice_processes = 1;
	process_find_top(info.cpu, info.memu);
	info.first_process = get_first_process();
}

/* Here come the IBM ACPI-specific things. For reference, see
 * http://ibm-acpi.sourceforge.net/README
 * If IBM ACPI is installed, /proc/acpi/ibm contains the following files:
bay
beep
bluetooth
brightness
cmos
dock
driver
ecdump
fan
hotkey
led
light
thermal
video
volume
 * The content of these files is described in detail in the aforementioned
 * README - some of them also in the following functions accessing them.
 * Peter Tarjan (ptarjan@citromail.hu) */

#define IBM_ACPI_DIR "/proc/acpi/ibm"

/* get fan speed on IBM/Lenovo laptops running the ibm acpi.
 * /proc/acpi/ibm/fan looks like this (3 lines):
status:         disabled
speed:          2944
commands:       enable, disable
 * Peter Tarjan (ptarjan@citromail.hu) */

void get_ibm_acpi_fan(char *p_client_buffer, size_t client_buffer_size)
{
	FILE *fp;
	unsigned int speed = 0;
	char fan[128];

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	snprintf(fan, 127, "%s/fan", IBM_ACPI_DIR);

	fp = fopen(fan, "r");
	if (fp != NULL) {
		while (!feof(fp)) {
			char line[256];

			if (fgets(line, 255, fp) == NULL) {
				break;
			}
			if (sscanf(line, "speed: %u", &speed)) {
				break;
			}
		}
	} else {
		CRIT_ERR("can't open '%s': %s\nYou are not using the IBM ACPI. Remove "
			"ibm* from your "PACKAGE_NAME" config file.", fan, strerror(errno));
	}

	fclose(fp);
	snprintf(p_client_buffer, client_buffer_size, "%d", speed);
}

/* get the measured temperatures from the temperature sensors
 * on IBM/Lenovo laptops running the ibm acpi.
 * There are 8 values in /proc/acpi/ibm/thermal, and according to
 * http://ibm-acpi.sourceforge.net/README
 * these mean the following (at least on an IBM R51...)
 * 0:  CPU (also on the T series laptops)
 * 1:  Mini PCI Module (?)
 * 2:  HDD (?)
 * 3:  GPU (also on the T series laptops)
 * 4:  Battery (?)
 * 5:  N/A
 * 6:  Battery (?)
 * 7:  N/A
 * I'm not too sure about those with the question mark, but the values I'm
 * reading from *my* thermal file (on a T42p) look realistic for the
 * hdd and the battery.
 * #5 and #7 are always -128.
 * /proc/acpi/ibm/thermal looks like this (1 line):
temperatures:   41 43 31 46 33 -128 29 -128
 * Peter Tarjan (ptarjan@citromail.hu) */

static double last_ibm_acpi_temp_time;
void get_ibm_acpi_temps(void)
{

	FILE *fp;
	char thermal[128];

	/* don't update too often */
	if (current_update_time - last_ibm_acpi_temp_time < 10.00) {
		return;
	}
	last_ibm_acpi_temp_time = current_update_time;

	/* if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	} */

	snprintf(thermal, 127, "%s/thermal", IBM_ACPI_DIR);
	fp = fopen(thermal, "r");

	if (fp != NULL) {
		while (!feof(fp)) {
			char line[256];

			if (fgets(line, 255, fp) == NULL) {
				break;
			}
			if (sscanf(line, "temperatures: %d %d %d %d %d %d %d %d",
					&ibm_acpi.temps[0], &ibm_acpi.temps[1], &ibm_acpi.temps[2],
					&ibm_acpi.temps[3], &ibm_acpi.temps[4], &ibm_acpi.temps[5],
					&ibm_acpi.temps[6], &ibm_acpi.temps[7])) {
				break;
			}
		}
	} else {
		CRIT_ERR("can't open '%s': %s\nYou are not using the IBM ACPI. Remove "
			"ibm* from your "PACKAGE_NAME" config file.", thermal, strerror(errno));
	}

	fclose(fp);
}

/* get volume (0-14) on IBM/Lenovo laptops running the ibm acpi.
 * "Volume" here is none of the mixer volumes, but a "master of masters"
 * volume adjusted by the IBM volume keys.
 * /proc/acpi/ibm/fan looks like this (4 lines):
level:          4
mute:           off
commands:       up, down, mute
commands:       level <level> (<level> is 0-15)
 * Peter Tarjan (ptarjan@citromail.hu) */

void get_ibm_acpi_volume(char *p_client_buffer, size_t client_buffer_size)
{
	FILE *fp;
	char volume[128];
	unsigned int vol = -1;
	char mute[3] = "";

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	snprintf(volume, 127, "%s/volume", IBM_ACPI_DIR);

	fp = fopen(volume, "r");
	if (fp != NULL) {
		while (!feof(fp)) {
			char line[256];
			unsigned int read_vol = -1;

			if (fgets(line, 255, fp) == NULL) {
				break;
			}
			if (sscanf(line, "level: %u", &read_vol)) {
				vol = read_vol;
				continue;
			}
			if (sscanf(line, "mute: %s", mute)) {
				break;
			}
		}
	} else {
		CRIT_ERR("can't open '%s': %s\nYou are not using the IBM ACPI. Remove "
			"ibm* from your "PACKAGE_NAME" config file.", volume, strerror(errno));
	}

	fclose(fp);

	if (strcmp(mute, "on") == 0) {
		snprintf(p_client_buffer, client_buffer_size, "%s", "mute");
		return;
	} else {
		snprintf(p_client_buffer, client_buffer_size, "%d", vol);
		return;
	}
}

/* static FILE *fp = NULL; */

/* get LCD brightness on IBM/Lenovo laptops running the ibm acpi.
 * /proc/acpi/ibm/brightness looks like this (3 lines):
level:          7
commands:       up, down
commands:       level <level> (<level> is 0-7)
 * Peter Tarjan (ptarjan@citromail.hu) */

void get_ibm_acpi_brightness(char *p_client_buffer, size_t client_buffer_size)
{
	FILE *fp;
	unsigned int brightness = 0;
	char filename[128];

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	snprintf(filename, 127, "%s/brightness", IBM_ACPI_DIR);

	fp = fopen(filename, "r");
	if (fp != NULL) {
		while (!feof(fp)) {
			char line[256];

			if (fgets(line, 255, fp) == NULL) {
				break;
			}
			if (sscanf(line, "level: %u", &brightness)) {
				break;
			}
		}
	} else {
		CRIT_ERR("can't open '%s': %s\nYou are not using the IBM ACPI. Remove "
			"ibm* from your "PACKAGE_NAME" config file.", filename, strerror(errno));
	}

	fclose(fp);

	snprintf(p_client_buffer, client_buffer_size, "%d", brightness);
}

void update_entropy(void)
{
	static int rep = 0;
	const char *entropy_avail = "/proc/sys/kernel/random/entropy_avail";
	const char *entropy_poolsize = "/proc/sys/kernel/random/poolsize";
	FILE *fp1, *fp2;

	info.entropy.entropy_avail = 0;
	info.entropy.poolsize = 0;

	if ((fp1 = open_file(entropy_avail, &rep)) == NULL) {
		return;
	}

	if ((fp2 = open_file(entropy_poolsize, &rep)) == NULL) {
		fclose(fp1);
		return;
	}

	fscanf(fp1, "%u", &info.entropy.entropy_avail);
	fscanf(fp2, "%u", &info.entropy.poolsize);

	fclose(fp1);
	fclose(fp2);

	info.mask |= (1 << INFO_ENTROPY);
}

const char *get_disk_protect_queue(const char *disk)
{
	FILE *fp;
	char path[128];
	int state;

	snprintf(path, 127, "/sys/block/%s/device/unload_heads", disk);
	if (access(path, F_OK)) {
		snprintf(path, 127, "/sys/block/%s/queue/protect", disk);
	}
	if ((fp = fopen(path, "r")) == NULL)
		return "n/a   ";
	if (fscanf(fp, "%d\n", &state) != 1) {
		fclose(fp);
		return "failed";
	}
	fclose(fp);
	return (state > 0) ? "frozen" : "free  ";
}

