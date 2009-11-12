/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
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

#include <sys/ioctl.h>
#include <sys/dkstat.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/user.h>

#include <net/if.h>
#include <net/if_mib.h>
#include <net/if_media.h>
#include <net/if_var.h>

#include <devstat.h>
#include <ifaddrs.h>
#include <limits.h>
#include <unistd.h>

#include <dev/wi/if_wavelan_ieee.h>
#include <dev/acpica/acpiio.h>

#include "conky.h"
#include "freebsd.h"
#include "logging.h"
#include "net_stat.h"
#include "top.h"
#include "diskio.h"

#define	GETSYSCTL(name, var)	getsysctl(name, &(var), sizeof(var))
#define	KELVTOC(x)				((x - 2732) / 10.0)
#define	MAXSHOWDEVS				16

#if 0
#define	FREEBSD_DEBUG
#endif

__attribute__((gnu_inline)) inline void
proc_find_top(struct process **cpu, struct process **mem);

static short cpu_setup = 0;

static int getsysctl(const char *name, void *ptr, size_t len)
{
	size_t nlen = len;

	if (sysctlbyname(name, ptr, &nlen, NULL, 0) == -1) {
		return -1;
	}

	if (nlen != len && errno == ENOMEM) {
		return -1;
	}

	return 0;
}

struct ifmibdata *data = NULL;
size_t len = 0;

static int swapmode(unsigned long *retavail, unsigned long *retfree)
{
	int n;
	unsigned long pagesize = getpagesize();
	struct kvm_swap swapary[1];

	*retavail = 0;
	*retfree = 0;

#define	CONVERT(v)	((quad_t)(v) * (pagesize / 1024))

	n = kvm_getswapinfo(kd, swapary, 1, 0);
	if (n < 0 || swapary[0].ksw_total == 0) {
		return 0;
	}

	*retavail = CONVERT(swapary[0].ksw_total);
	*retfree = CONVERT(swapary[0].ksw_total - swapary[0].ksw_used);

	n = (int) ((double) swapary[0].ksw_used * 100.0 /
		(double) swapary[0].ksw_total);

	return n;
}

void prepare_update(void)
{
}

void update_uptime(void)
{
	int mib[2] = { CTL_KERN, KERN_BOOTTIME };
	struct timeval boottime;
	time_t now;
	size_t size = sizeof(boottime);

	if ((sysctl(mib, 2, &boottime, &size, NULL, 0) != -1)
			&& (boottime.tv_sec != 0)) {
		time(&now);
		info.uptime = now - boottime.tv_sec;
	} else {
		fprintf(stderr, "Could not get uptime\n");
		info.uptime = 0;
	}
}

int check_mount(char *s)
{
	struct statfs *mntbuf;
	int i, mntsize;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = mntsize - 1; i >= 0; i--) {
		if (strcmp(mntbuf[i].f_mntonname, s) == 0) {
			return 1;
		}
	}

	return 0;
}

void update_meminfo(void)
{
	u_int total_pages, inactive_pages, free_pages;
	unsigned long swap_avail, swap_free;

	int pagesize = getpagesize();

	if (GETSYSCTL("vm.stats.vm.v_page_count", total_pages)) {
		fprintf(stderr, "Cannot read sysctl \"vm.stats.vm.v_page_count\"\n");
	}

	if (GETSYSCTL("vm.stats.vm.v_free_count", free_pages)) {
		fprintf(stderr, "Cannot read sysctl \"vm.stats.vm.v_free_count\"\n");
	}

	if (GETSYSCTL("vm.stats.vm.v_inactive_count", inactive_pages)) {
		fprintf(stderr, "Cannot read sysctl \"vm.stats.vm.v_inactive_count\"\n");
	}

	info.memmax = total_pages * (pagesize >> 10);
	info.mem = (total_pages - free_pages - inactive_pages) * (pagesize >> 10);
	info.memeasyfree = info.memfree = info.memmax - info.mem;

	if ((swapmode(&swap_avail, &swap_free)) >= 0) {
		info.swapmax = swap_avail;
		info.swap = (swap_avail - swap_free);
		info.swapfree = swap_free;
	} else {
		info.swapmax = 0;
		info.swap = 0;
		info.swapfree = 0;
	}
}

void update_net_stats(void)
{
	struct net_stat *ns;
	double delta;
	long long r, t, last_recv, last_trans;
	struct ifaddrs *ifap, *ifa;
	struct if_data *ifd;

	/* get delta */
	delta = current_update_time - last_update_time;
	if (delta <= 0.0001) {
		return;
	}

	if (getifaddrs(&ifap) < 0) {
		return;
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		ns = get_net_stat((const char *) ifa->ifa_name, NULL, NULL);

		if (ifa->ifa_flags & IFF_UP) {
			struct ifaddrs *iftmp;

			ns->up = 1;
			last_recv = ns->recv;
			last_trans = ns->trans;

			if (ifa->ifa_addr->sa_family != AF_LINK) {
				continue;
			}

			for (iftmp = ifa->ifa_next;
					iftmp != NULL && strcmp(ifa->ifa_name, iftmp->ifa_name) == 0;
					iftmp = iftmp->ifa_next) {
				if (iftmp->ifa_addr->sa_family == AF_INET) {
					memcpy(&(ns->addr), iftmp->ifa_addr,
						iftmp->ifa_addr->sa_len);
				}
			}

			ifd = (struct if_data *) ifa->ifa_data;
			r = ifd->ifi_ibytes;
			t = ifd->ifi_obytes;

			if (r < ns->last_read_recv) {
				ns->recv += ((long long) 4294967295U - ns->last_read_recv) + r;
			} else {
				ns->recv += (r - ns->last_read_recv);
			}

			ns->last_read_recv = r;

			if (t < ns->last_read_trans) {
				ns->trans += ((long long) 4294967295U -
					ns->last_read_trans) + t;
			} else {
				ns->trans += (t - ns->last_read_trans);
			}

			ns->last_read_trans = t;

			/* calculate speeds */
			ns->recv_speed = (ns->recv - last_recv) / delta;
			ns->trans_speed = (ns->trans - last_trans) / delta;
		} else {
			ns->up = 0;
		}
	}

	freeifaddrs(ifap);
}

void update_total_processes(void)
{
	int n_processes;

	kvm_getprocs(kd, KERN_PROC_ALL, 0, &n_processes);

	info.procs = n_processes;
}

void update_running_processes(void)
{
	struct kinfo_proc *p;
	int n_processes;
	int i, cnt = 0;

	p = kvm_getprocs(kd, KERN_PROC_ALL, 0, &n_processes);
	for (i = 0; i < n_processes; i++) {
#if (__FreeBSD__ < 5) && (__FreeBSD_kernel__ < 5)
		if (p[i].kp_proc.p_stat == SRUN) {
#else
		if (p[i].ki_stat == SRUN) {
#endif
			cnt++;
		}
	}

	info.run_procs = cnt;
}

struct cpu_load_struct {
	unsigned long load[5];
};

struct cpu_load_struct fresh = { {0, 0, 0, 0, 0} };
long cpu_used, oldtotal, oldused;

void get_cpu_count(void)
{
	/* int cpu_count = 0; */

	/* XXX: FreeBSD doesn't allow to get per CPU load stats on SMP machines.
	 * It's possible to get a CPU count, but as we fulfill only
	 * info.cpu_usage[0], it's better to report there's only one CPU.
	 * It should fix some bugs (e.g. cpugraph) */
#if 0
	if (GETSYSCTL("hw.ncpu", cpu_count) == 0) {
		info.cpu_count = cpu_count;
	}
#endif
	info.cpu_count = 1;

	info.cpu_usage = malloc(info.cpu_count * sizeof(float));
	if (info.cpu_usage == NULL) {
		CRIT_ERR(NULL, NULL, "malloc");
	}
}

/* XXX: SMP support */
void update_cpu_usage(void)
{
	long used, total;
	long cp_time[CPUSTATES];
	size_t cp_len = sizeof(cp_time);

	/* add check for !info.cpu_usage since that mem is freed on a SIGUSR1 */
	if ((cpu_setup == 0) || (!info.cpu_usage)) {
		get_cpu_count();
		cpu_setup = 1;
	}

	if (sysctlbyname("kern.cp_time", &cp_time, &cp_len, NULL, 0) < 0) {
		fprintf(stderr, "Cannot get kern.cp_time");
	}

	fresh.load[0] = cp_time[CP_USER];
	fresh.load[1] = cp_time[CP_NICE];
	fresh.load[2] = cp_time[CP_SYS];
	fresh.load[3] = cp_time[CP_IDLE];
	fresh.load[4] = cp_time[CP_IDLE];

	used = fresh.load[0] + fresh.load[1] + fresh.load[2];
	total = fresh.load[0] + fresh.load[1] + fresh.load[2] + fresh.load[3];

	if ((total - oldtotal) != 0) {
		info.cpu_usage[0] = ((double) (used - oldused)) /
			(double) (total - oldtotal);
	} else {
		info.cpu_usage[0] = 0;
	}

	oldused = used;
	oldtotal = total;
}

void update_load_average(void)
{
	double v[3];

	getloadavg(v, 3);

	info.loadavg[0] = (double) v[0];
	info.loadavg[1] = (double) v[1];
	info.loadavg[2] = (double) v[2];
}

double get_acpi_temperature(int fd)
{
	int temp;
	(void)fd;

	if (GETSYSCTL("hw.acpi.thermal.tz0.temperature", temp)) {
		fprintf(stderr,
			"Cannot read sysctl \"hw.acpi.thermal.tz0.temperature\"\n");
		return 0.0;
	}

	return KELVTOC(temp);
}

static void get_battery_stats(int *battime, int *batcapacity, int *batstate, int *ac) {
	if (battime && GETSYSCTL("hw.acpi.battery.time", *battime)) {
		fprintf(stderr, "Cannot read sysctl \"hw.acpi.battery.time\"\n");
	}
	if (batcapacity && GETSYSCTL("hw.acpi.battery.life", *batcapacity)) {
		fprintf(stderr, "Cannot read sysctl \"hw.acpi.battery.life\"\n");
	}
	if (batstate && GETSYSCTL("hw.acpi.battery.state", *batstate)) {
		fprintf(stderr, "Cannot read sysctl \"hw.acpi.battery.state\"\n");
	}
	if (ac && GETSYSCTL("hw.acpi.acline", *ac)) {
		fprintf(stderr, "Cannot read sysctl \"hw.acpi.acline\"\n");
	}
}

void get_battery_stuff(char *buf, unsigned int n, const char *bat, int item)
{
	int battime, batcapacity, batstate, ac;
	(void)bat;

	get_battery_stats(&battime, &batcapacity, &batstate, &ac);

	if (batstate != 1 && batstate != 2 && batstate != 0 && batstate != 7)
		fprintf(stderr, "Unknown battery state %d!\n", batstate);
	else if (batstate != 1 && ac == 0)
		fprintf(stderr, "Battery charging while not on AC!\n");
	else if (batstate == 1 && ac == 1)
		fprintf(stderr, "Battery discharing while on AC!\n");

	switch (item) {
		case BATTERY_TIME:
			if (batstate == 1 && battime != -1)
				snprintf(buf, n, "%d:%2.2d", battime / 60, battime % 60);
			break;
		case BATTERY_STATUS:
			if (batstate == 1) // Discharging
				snprintf(buf, n, "remaining %d%%", batcapacity);
			else
				snprintf(buf, n, batstate == 2 ? "charging (%d%%)" :
						(batstate == 7 ? "absent/on AC" : "charged (%d%%)"),
						batcapacity);
			break;
		default:
			fprintf(stderr, "Unknown requested battery stat %d\n", item);
	}
}

static int check_bat(const char *bat)
{
	int batnum, numbatts;
	char *endptr;
	if (GETSYSCTL("hw.acpi.battery.units", numbatts)) {
		fprintf(stderr, "Cannot read sysctl \"hw.acpi.battery.units\"\n");
		return -1;
	}
	if (numbatts <= 0) {
		fprintf(stderr, "No battery unit detected\n");
		return -1;
	}
	if (!bat || (batnum = strtol(bat, &endptr, 10)) < 0 ||
			bat == endptr || batnum > numbatts) {
		fprintf(stderr, "Wrong battery unit %s requested\n", bat ? bat : "");
		return -1;
	}
	return batnum;
}

int get_battery_perct(const char *bat)
{
	union acpi_battery_ioctl_arg battio;
	int batnum, acpifd;
	int designcap, lastfulcap, batperct;

	if ((battio.unit = batnum = check_bat(bat)) < 0)
		return 0;
	if ((acpifd = open("/dev/acpi", O_RDONLY)) < 0) {
		fprintf(stderr, "Can't open ACPI device\n");
		return 0;
	}
	if (ioctl(acpifd, ACPIIO_BATT_GET_BIF, &battio) == -1) {
		fprintf(stderr, "Unable to get info for battery unit %d\n", batnum);
		return 0;
	}
	close(acpifd);
	designcap = battio.bif.dcap;
	lastfulcap = battio.bif.lfcap;
	batperct = (designcap > 0 && lastfulcap > 0) ?
		(int) (((float) lastfulcap / designcap) * 100) : 0;
	return batperct > 100 ? 100 : batperct;
}

int get_battery_perct_bar(const char *bar)
{
	int batperct = get_battery_perct(bar);
	return (int)(batperct * 2.56 - 1);
}

int open_acpi_temperature(const char *name)
{
	(void)name;
	/* Not applicable for FreeBSD. */
	return 0;
}

void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size)
{
	int state;

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	if (GETSYSCTL("hw.acpi.acline", state)) {
		fprintf(stderr, "Cannot read sysctl \"hw.acpi.acline\"\n");
		return;
	}

	if (state) {
		strncpy(p_client_buffer, "Running on AC Power", client_buffer_size);
	} else {
		strncpy(p_client_buffer, "Running on battery", client_buffer_size);
	}
}

void get_acpi_fan(char *p_client_buffer, size_t client_buffer_size)
{
	/* not implemented */
	if (p_client_buffer && client_buffer_size > 0) {
		memset(p_client_buffer, 0, client_buffer_size);
	}
}

/* rdtsc() and get_freq_dynamic() copied from linux.c */

#if  defined(__i386) || defined(__x86_64)
__attribute__((gnu_inline)) inline unsigned long long int rdtsc(void)
{
	unsigned long long int x;

	__asm__ volatile(".byte 0x0f, 0x31":"=A" (x));
	return x;
}
#endif

/* return system frequency in MHz (use divisor=1) or GHz (use divisor=1000) */
void get_freq_dynamic(char *p_client_buffer, size_t client_buffer_size,
		const char *p_format, int divisor)
{
#if  defined(__i386) || defined(__x86_64)
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

	snprintf(p_client_buffer, client_buffer_size, p_format,
		(float) ((cycles[1] - cycles[0]) / microseconds) / divisor);
#else
	get_freq(p_client_buffer, client_buffer_size, p_format, divisor, 1);
#endif
}

/* void */
char get_freq(char *p_client_buffer, size_t client_buffer_size, const char *p_format,
		int divisor, unsigned int cpu)
{
	int freq;
	char *freq_sysctl;

	freq_sysctl = (char *) calloc(16, sizeof(char));
	if (freq_sysctl == NULL) {
		exit(-1);
	}

	snprintf(freq_sysctl, 16, "dev.cpu.%d.freq", (cpu - 1));

	if (!p_client_buffer || client_buffer_size <= 0 || !p_format
			|| divisor <= 0) {
		return 0;
	}

	if (GETSYSCTL(freq_sysctl, freq) == 0) {
		snprintf(p_client_buffer, client_buffer_size, p_format,
			(float) freq / divisor);
	} else {
		snprintf(p_client_buffer, client_buffer_size, p_format, 0.0f);
	}

	free(freq_sysctl);
	return 1;
}

void update_top(void)
{
	proc_find_top(info.cpu, info.memu);
}

#if 0
void update_wifi_stats(void)
{
	struct ifreq ifr;		/* interface stats */
	struct wi_req wireq;
	struct net_stat *ns;
	struct ifaddrs *ifap, *ifa;
	struct ifmediareq ifmr;
	int s;

	/* Get iface table */
	if (getifaddrs(&ifap) < 0) {
		return;
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		ns = get_net_stat((const char *) ifa->ifa_name, NULL, NULL);

		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		/* Get media type */
		bzero(&ifmr, sizeof(ifmr));
		strlcpy(ifmr.ifm_name, ifa->ifa_name, IFNAMSIZ);
		if (ioctl(s, SIOCGIFMEDIA, (caddr_t) &ifmr) < 0) {
			close(s);
			return;
		}

		/* We can monitor only wireless interfaces
		 * which are not in hostap mode */
		if ((ifmr.ifm_active & IFM_IEEE80211)
				&& !(ifmr.ifm_active & IFM_IEEE80211_HOSTAP)) {
			/* Get wi status */
			bzero(&ifr, sizeof(ifr));
			strlcpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ);
			wireq.wi_type = WI_RID_COMMS_QUALITY;
			wireq.wi_len = WI_MAX_DATALEN;
			ifr.ifr_data = (void *) &wireq;

			if (ioctl(s, SIOCGWAVELAN, (caddr_t) &ifr) < 0) {
				perror("ioctl (getting wi status)");
				exit(1);
			}

			/* wi_val[0] = quality
			 * wi_val[1] = signal
			 * wi_val[2] = noise */
			ns->linkstatus = (int) wireq.wi_val[1];
		}
cleanup:
		close(s);
	}
}
#endif

void update_diskio(void)
{
	int devs_count, num_selected, num_selections, dn;
	struct device_selection *dev_select = NULL;
	long select_generation;
	static struct statinfo statinfo_cur;
	char device_name[text_buffer_size];
	struct diskio_stat *cur;
	unsigned int reads, writes;
	unsigned int total_reads = 0, total_writes = 0;


	memset(&statinfo_cur, 0, sizeof(statinfo_cur));
	statinfo_cur.dinfo = (struct devinfo *)calloc(1, sizeof(struct devinfo));
	stats.current = stats.current_read = stats.current_write = 0;

	if (devstat_getdevs(NULL, &statinfo_cur) < 0) {
		free(statinfo_cur.dinfo);
		return;
	}

	devs_count = statinfo_cur.dinfo->numdevs;
	if (devstat_selectdevs(&dev_select, &num_selected, &num_selections,
			&select_generation, statinfo_cur.dinfo->generation,
			statinfo_cur.dinfo->devices, devs_count, NULL, 0, NULL, 0,
			DS_SELECT_ONLY, MAXSHOWDEVS, 1) >= 0) {
		for (dn = 0; dn < devs_count; dn++) {
			int di;
			struct devstat *dev;

			di = dev_select[dn].position;
			dev = &statinfo_cur.dinfo->devices[di];
			snprintf(device_name, text_buffer_size, "%s%d",
					dev_select[dn].device_name, dev_select[dn].unit_number);

			total_reads += (reads = dev->bytes[DEVSTAT_READ] / 512);
			total_writes += (writes = dev->bytes[DEVSTAT_WRITE] / 512);
			for (cur = stats.next; cur; cur = cur->next) {
				if (cur->dev && !strcmp(device_name, cur->dev)) {
					update_diskio_values(cur, reads, writes);
					break;
				}
			}
		}
		update_diskio_values(&stats, total_reads, total_writes);

		free(dev_select);
	}

	free(statinfo_cur.dinfo);
}

/* While topless is obviously better, top is also not bad. */

int comparecpu(const void *a, const void *b)
{
	if (((const struct process *)a)->amount > ((const struct process *)b)->amount) {
		return -1;
	} else if (((const struct process *)a)->amount < ((const struct process *)b)->amount) {
		return 1;
	} else {
		return 0;
	}
}

int comparemem(const void *a, const void *b)
{
	if (((const struct process *)a)->rss > ((const struct process *)b)->rss) {
		return -1;
	} else if (((const struct process *)a)->rss < ((const struct process *)b)->rss) {
		return 1;
	} else {
		return 0;
	}
}

__attribute__((gnu_inline)) inline void
proc_find_top(struct process **cpu, struct process **mem)
{
	struct kinfo_proc *p;
	int n_processes;
	int i, j = 0;
	struct process *processes;

	int total_pages;

	/* we get total pages count again to be sure it is up to date */
	if (GETSYSCTL("vm.stats.vm.v_page_count", total_pages) != 0) {
		CRIT_ERR(NULL, NULL, "Cannot read sysctl \"vm.stats.vm.v_page_count\"");
	}

	p = kvm_getprocs(kd, KERN_PROC_PROC, 0, &n_processes);
	processes = malloc(n_processes * sizeof(struct process));

	for (i = 0; i < n_processes; i++) {
		if (!((p[i].ki_flag & P_SYSTEM)) && p[i].ki_comm != NULL) {
			processes[j].pid = p[i].ki_pid;
			processes[j].name = strndup(p[i].ki_comm, text_buffer_size);
			processes[j].amount = 100.0 * p[i].ki_pctcpu / FSCALE;
			processes[j].vsize = p[i].ki_size;
			processes[j].rss = (p[i].ki_rssize * getpagesize());
			j++;
		}
	}

	qsort(processes, j - 1, sizeof(struct process), comparemem);
	for (i = 0; i < 10 && i < n_processes; i++) {
		struct process *tmp, *ttmp;

		tmp = malloc(sizeof(struct process));
		tmp->pid = processes[i].pid;
		tmp->amount = processes[i].amount;
		tmp->name = strndup(processes[i].name, text_buffer_size);
		tmp->rss = processes[i].rss;
		tmp->vsize = processes[i].vsize;

		ttmp = mem[i];
		mem[i] = tmp;
		if (ttmp != NULL) {
			free(ttmp->name);
			free(ttmp);
		}
	}

	qsort(processes, j - 1, sizeof(struct process), comparecpu);
	for (i = 0; i < 10 && i < n_processes; i++) {
		struct process *tmp, *ttmp;

		tmp = malloc(sizeof(struct process));
		tmp->pid = processes[i].pid;
		tmp->amount = processes[i].amount;
		tmp->name = strndup(processes[i].name, text_buffer_size);
		tmp->rss = processes[i].rss;
		tmp->vsize = processes[i].vsize;

		ttmp = cpu[i];
		cpu[i] = tmp;
		if (ttmp != NULL) {
			free(ttmp->name);
			free(ttmp);
		}
	}

#if defined(FREEBSD_DEBUG)
	printf("=====\nmem\n");
	for (i = 0; i < 10; i++) {
		printf("%d: %s(%d) %ld %ld\n", i, mem[i]->name,
				mem[i]->pid, mem[i]->vsize, mem[i]->rss);
	}
#endif

	for (i = 0; i < j; i++) {
		free(processes[i].name);
	}
	free(processes);
}

#if	defined(i386) || defined(__i386__)
#define	APMDEV		"/dev/apm"
#define	APM_UNKNOWN	255

int apm_getinfo(int fd, apm_info_t aip)
{
	if (ioctl(fd, APMIO_GETINFO, aip) == -1) {
		return -1;
	}

	return 0;
}

char *get_apm_adapter(void)
{
	int fd;
	struct apm_info a_info;
	char *out;

	out = (char *) calloc(16, sizeof(char));

	fd = open(APMDEV, O_RDONLY);
	if (fd < 0) {
		strncpy(out, "ERR", 16);
		return out;
	}

	if (apm_getinfo(fd, &a_info) != 0) {
		close(fd);
		strncpy(out, "ERR", 16);
		return out;
	}
	close(fd);

	switch (a_info.ai_acline) {
		case 0:
			strncpy(out, "off-line", 16);
			return out;
			break;
		case 1:
			if (a_info.ai_batt_stat == 3) {
				strncpy(out, "charging", 16);
				return out;
			} else {
				strncpy(out, "on-line", 16);
				return out;
			}
			break;
		default:
			strncpy(out, "unknown", 16);
			return out;
			break;
	}
}

char *get_apm_battery_life(void)
{
	int fd;
	u_int batt_life;
	struct apm_info a_info;
	char *out;

	out = (char *) calloc(16, sizeof(char));

	fd = open(APMDEV, O_RDONLY);
	if (fd < 0) {
		strncpy(out, "ERR", 16);
		return out;
	}

	if (apm_getinfo(fd, &a_info) != 0) {
		close(fd);
		strncpy(out, "ERR", 16);
		return out;
	}
	close(fd);

	batt_life = a_info.ai_batt_life;
	if (batt_life == APM_UNKNOWN) {
		strncpy(out, "unknown", 16);
	} else if (batt_life <= 100) {
		snprintf(out, 16, "%d%%", batt_life);
		return out;
	} else {
		strncpy(out, "ERR", 16);
	}

	return out;
}

char *get_apm_battery_time(void)
{
	int fd;
	int batt_time;
	int h, m, s;
	struct apm_info a_info;
	char *out;

	out = (char *) calloc(16, sizeof(char));

	fd = open(APMDEV, O_RDONLY);
	if (fd < 0) {
		strncpy(out, "ERR", 16);
		return out;
	}

	if (apm_getinfo(fd, &a_info) != 0) {
		close(fd);
		strncpy(out, "ERR", 16);
		return out;
	}
	close(fd);

	batt_time = a_info.ai_batt_time;

	if (batt_time == -1) {
		strncpy(out, "unknown", 16);
	} else {
		h = batt_time;
		s = h % 60;
		h /= 60;
		m = h % 60;
		h /= 60;
		snprintf(out, 16, "%2d:%02d:%02d", h, m, s);
	}

	return out;
}

#endif

void get_battery_short_status(char *buffer, unsigned int n, const char *bat)
{
	get_battery_stuff(buffer, n, bat, BATTERY_STATUS);
	if (0 == strncmp("charging", buffer, 8)) {
		buffer[0] = 'C';
		memmove(buffer + 1, buffer + 8, n - 8);
	} else if (0 == strncmp("discharging", buffer, 11)) {
		buffer[0] = 'D';
		memmove(buffer + 1, buffer + 11, n - 11);
	} else if (0 == strncmp("absent/on AC", buffer, 12)) {
		buffer[0] = 'A';
		memmove(buffer + 1, buffer + 12, n - 12);
	}
}

int get_entropy_avail(unsigned int *val)
{
	/* Not applicable for FreeBSD as it uses the yarrow prng. */
	(void)val;
	return 1;
}

int get_entropy_poolsize(unsigned int *val)
{
	/* Not applicable for FreeBSD as it uses the yarrow prng. */
	(void)val;
	return 1;
}

/* empty stub so conky links */
void free_all_processes(void)
{
}
