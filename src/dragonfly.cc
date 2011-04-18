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
 * Copyright (c) 2011 Andrea Magliano <masterblaster@tiscali.it>
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
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/user.h>
#include <kinfo_pcpu.h>

#include <net/if.h>
#include <net/if_mib.h>
#include <net/if_media.h>
#include <net/if_var.h>

#include <devstat.h>
#include <ifaddrs.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>

#include <dev/acpica/acpiio.h>

#include "conky.h"
#include "dragonfly.h"
#include "logging.h"
#include "net_stat.h"
#include "top.h"
#include "diskio.h"

#define	GETSYSCTL(name, var)	getsysctl(name, &(var), sizeof(var))
#define	KELVTOC(x)				((x - 2732) / 10.0)
#define	MAXSHOWDEVS				16

static short cpu_setup = 0;

static int getsysctl(const char *name, void *ptr, size_t len)
{
	size_t nlen = len;

	if (sysctlbyname(name, ptr, &nlen, NULL, 0) == -1) {
		fprintf(stderr, "getsysctl(): %s failed '%s'\n",
				name, strerror(errno));
		return -1;
	}

	if (nlen != len && errno == ENOMEM) {
		fprintf(stderr, "getsysctl(): %s failed %zu != %zu\n", name, nlen, len);
		return -1;
	}

	return 0;
}

static int swapmode(unsigned long *retavail, unsigned long *retfree)
{
    int total, used; size_t len = sizeof(int);

    if (sysctlbyname("vm.swap_size", &total, &len, NULL, 0) == -1)
        perror("vm_swap_usage(): vm.swap_size");
    else if (sysctlbyname("vm.swap_anon_use", &used, &len, NULL, 0) == -1)
        perror("vm_swap_usage(): vm.swap_anon_use");
    else {
		int size = getpagesize();

#define	CONVERT(v)	((quad_t)(v) * (size / 1024))

		*retavail = CONVERT(total);
		*retfree = CONVERT(total - used);

		return (int) ((double) used * 100.0 / (double) total);
	}
	return 0;
}

void prepare_update(void)
{
}

int update_uptime(void)
{
	int mib[2] = { CTL_KERN, KERN_BOOTTIME };
	struct timeval boottime;
	time_t now;
	size_t size = sizeof(boottime);

	if ((sysctl(mib, 2, &boottime, &size, NULL, 0) != -1) && boottime.tv_sec) {
		time(&now);
		info.uptime = now - boottime.tv_sec;
	} else {
		fprintf(stderr, "Could not get uptime\n");
		info.uptime = 0;
	}

	return 0;
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

int update_meminfo(void)
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

	return 0;
}

int update_net_stats(void)
{
	struct net_stat *ns;
	double delta;
	long long r, t, last_recv, last_trans;
	struct ifaddrs *ifap, *ifa;
	struct if_data *ifd;

	/* get delta */
	delta = current_update_time - last_update_time;
	if (delta <= 0.0001) {
		return 0;
	}

	if (getifaddrs(&ifap) < 0) {
		return 0;
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
	return 0;
}

static int kern_proc_all_n()
{
	size_t len = 0;

    if (sysctlbyname("kern.proc.all_lwp", NULL, &len, NULL, 0) == -1) {
        perror("kern.proc.all_lwp");
        return -1;
	}

    if (len % sizeof(struct kinfo_proc)) {
        fprintf(stderr, "kern_proc(): "
                "len %% sizeof(struct kinfo_proc) != 0");
        return -1;
    }

	return len / sizeof(struct kinfo_proc);
}

static struct kinfo_proc *kern_proc_all(size_t proc_n)
{
    if (proc_n > 0) {
		size_t len = proc_n * sizeof(struct kinfo_proc);
        struct kinfo_proc *kp = (struct kinfo_proc *) malloc(len);

		if (kp) {
			if (sysctlbyname("kern.proc.all_lwp", kp, &len, NULL, 0) == -1)
				perror("kern_proc(): kern.proc.all_lwp");
			else return kp;
			free (kp);
		}
		else perror("malloc");
	}
	return NULL;
}

void get_cpu_count(void)
{
	int cpu_count = 0;

	if (GETSYSCTL("hw.ncpu", cpu_count) == 0) {
		info.cpu_count = cpu_count;
	} else {
		fprintf(stderr, "Cannot get hw.ncpu\n");
		info.cpu_count = 0;
	}

	info.cpu_usage = (float *) malloc((info.cpu_count + 1) * sizeof(float));
	if (info.cpu_usage == NULL) {
		CRIT_ERR(NULL, NULL, "malloc");
	}
}

struct cpu_info {
	long oldtotal;
	long oldused;
};

PCPU_STATISTICS_FUNC(cputime, struct kinfo_cputime, uint64_t);

static void stat_cpu(struct cpu_info *cpu,
					 struct kinfo_cputime *percpu, float *usage)
{
	long int used = (percpu->cp_user + percpu->cp_nice +
					 percpu->cp_sys + percpu->cp_intr),
		total = used + percpu->cp_idle;

	*usage = (total - cpu->oldtotal) && cpu->oldtotal ?
		((float) (used - cpu->oldused)) / (total - cpu->oldtotal) : 0;

	cpu->oldused = used;
	cpu->oldtotal = total;
}

int update_cpu_usage(void)
{
	static struct cpu_info *cpu = NULL;
	extern void* global_cpu;

	/* add check for !info.cpu_usage since that mem is freed on a SIGUSR1 */
	if ((cpu_setup == 0) || (!info.cpu_usage)) {
		get_cpu_count();
		cpu_setup = 1;
	}

	if (!global_cpu) {
		if (!cpu) cpu = (struct cpu_info *)
					  calloc(sizeof(struct cpu_info), info.cpu_count + 1);
		global_cpu = cpu;
	}

	{
		size_t percpu_n = info.cpu_count * sizeof(struct kinfo_cputime);
		struct kinfo_cputime *percpu = (struct kinfo_cputime *)
			malloc(info.cpu_count * sizeof(struct kinfo_cputime));

		if (percpu) {
			if (sysctlbyname("kern.cputime", percpu,
							 &percpu_n, NULL, 0) == -1 && errno != ENOMEM) {
				printf("update_cpu_usage(): with %d cpu(s) ", info.cpu_count);
				perror("kern.cputime");
			}
			else {
				struct kinfo_cputime total;
				cputime_pcpu_statistics(&percpu[0], &total, info.cpu_count);

				{
					int i;
					for (i = 0; i < info.cpu_count; i++)
						stat_cpu(&cpu[i+1], &percpu[i], &info.cpu_usage[i+1]);
				}
				stat_cpu(&cpu[0], &total, &info.cpu_usage[0]);
			}
			free(percpu);
		}
	}

	return 0;
}

int update_load_average(void)
{
	double v[3];

	getloadavg(v, 3);

	info.loadavg[0] = (double) v[0];
	info.loadavg[1] = (double) v[1];
	info.loadavg[2] = (double) v[2];

	return 0;
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
		(((float) lastfulcap / designcap) * 100) : 0;
	return batperct > 100 ? 100 : batperct;
}

double get_battery_perct_bar(struct text_object *obj)
{
	int batperct = get_battery_perct(obj->data.s);
	return batperct * 2.56 - 1;
}

int open_acpi_temperature(const char *name)
{
	(void)name;
	/* Not applicable for FreeBSD. */
	return 0;
}

void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size, const char *adapter)
{
	int state;

	(void) adapter; // only linux uses this

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

/* void */
char get_freq(char *p_client_buffer, size_t client_buffer_size, const char *p_format,
			  int divisor, unsigned int cpu)
{
	int64_t freq;

	if (p_client_buffer && client_buffer_size > 0 && p_format && divisor > 0) {
		if (GETSYSCTL("hw.tsc_frequency", freq) == 0) {
			snprintf(p_client_buffer, client_buffer_size, p_format,
					 (float) freq / (divisor * 1000000));
		} else {
			snprintf(p_client_buffer, client_buffer_size, p_format, 0.0f);
		}
		return 1;
	}
	return 0;
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

int update_diskio(void)
{
	int devs_count, num_selected, num_selections, dn;
	struct device_selection *dev_select = NULL;
	long select_generation;
	static struct statinfo statinfo_cur;
	char device_name[DEFAULT_TEXT_BUFFER_SIZE];
	struct diskio_stat *cur;
	unsigned int reads, writes;
	unsigned int total_reads = 0, total_writes = 0;


	memset(&statinfo_cur, 0, sizeof(statinfo_cur));
	statinfo_cur.dinfo = (struct devinfo *)calloc(1, sizeof(struct devinfo));
	stats.current = stats.current_read = stats.current_write = 0;

	if (getdevs(&statinfo_cur) < 0) {
		free(statinfo_cur.dinfo);
		return 0;
	}

	devs_count = statinfo_cur.dinfo->numdevs;
	if (selectdevs(&dev_select, &num_selected, &num_selections,
			&select_generation, statinfo_cur.dinfo->generation,
			statinfo_cur.dinfo->devices, devs_count, NULL, 0, NULL, 0,
			DS_SELECT_ONLY, MAXSHOWDEVS, 1) >= 0) {
		for (dn = 0; dn < devs_count; dn++) {
			int di;
			struct devstat *dev;

			di = dev_select[dn].position;
			dev = &statinfo_cur.dinfo->devices[di];
			snprintf(device_name, DEFAULT_TEXT_BUFFER_SIZE, "%s%d",
					dev_select[dn].device_name, dev_select[dn].unit_number);

			total_reads += (reads = dev->bytes_read / 512);
			total_writes += (writes = dev->bytes_written / 512);
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
	return 0;
}

static int proc_rusage(struct kinfo_proc *p)
{
    struct kinfo_lwp *lwp = &p->kp_lwp;
    struct rusage *cru = &p->kp_cru;

	return (lwp->kl_uticks +
			lwp->kl_sticks + lwp->kl_iticks) +
		(cru->ru_stime.tv_sec + cru->ru_utime.tv_sec) * 1000000;
}

static void proc_count(struct kinfo_proc *kp, size_t proc_n)
{
	size_t i, act = 0, run = 0;

	for (i = 0; i < proc_n; i++) {
		struct kinfo_proc *p = &kp[i];

		if (!(p->kp_flags & P_SYSTEM)) {
			struct kinfo_lwp *lwp = &p->kp_lwp;

			if (!lwp->kl_tid) act++;
			if (lwp->kl_stat == LSRUN) run++;
		}
	}

	info.procs = act;
	info.run_procs = run;
}

static void proc_fill(struct kinfo_proc *kp, size_t proc_n)
{
	size_t i, f = getpagesize();
	static long prev_ticks = 0; /* safe as long as in same thread */

	for (i = 0; i < proc_n; i++) {
		struct kinfo_proc *p = &kp[i];
		struct kinfo_lwp *lwp = &p->kp_lwp;

		if (!(p->kp_flags & P_SYSTEM) &&
			p->kp_comm && *p->kp_comm && /* just to be sure */
			!lwp->kl_tid) { /* 'main' lwp, the real process (observed) */

			struct process *my = get_process(p->kp_pid);
			long ticks = proc_rusage(p);

			my->time_stamp = g_time;

			free_and_zero(my->name);
			my->name = strdup(p->kp_comm);

			my->amount = 100.0 * lwp->kl_pctcpu / FSCALE;
			my->vsize = p->kp_vm_map_size;
			my->rss = p->kp_vm_rssize * f;

			my->total_cpu_time = ticks - prev_ticks;
			prev_ticks = ticks;

			// printf("\tmy[%p]: %s(%u) %d %d 0x%x 0x%x %f\n", p,
			//        my->name, my->pid, my->vsize, my->rss,
			// 	   p->kp_flags, lwp->kl_stat, my->amount);
		}
	}
}

void get_top_info(void)
{
	size_t proc_n = kern_proc_all_n();
	struct kinfo_proc *kp = kern_proc_all(proc_n);

    if (kp) {
		proc_count(kp, proc_n);
		proc_fill(kp, proc_n);
		free(kp);
	}
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
