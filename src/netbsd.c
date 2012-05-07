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

#include "netbsd.h"
#include "net_stat.h"
#include "top.h"
#include <sys/types.h>
#include <sys/statvfs.h>
#include <ifaddrs.h>

u_int32_t sensvalue;
char errbuf[_POSIX2_LINE_MAX];
static short cpu_setup = 0;

inline void proc_find_top(struct process **cpu, struct process **mem);

void
prepare_update(void)
{
}

int
update_uptime(void)
{
	int 			mib[2] = { CTL_KERN, KERN_BOOTTIME };
	struct timeval	boottime;
	time_t			now;
	size_t			size;

	size = sizeof(boottime);

	if (sysctl(mib, 2, &boottime, &size, NULL, 0) < 0) {
		warn("sysctl kern.boottime failed");
		info.uptime = 0;
	} else {
		time(&now);
		info.uptime = now - boottime.tv_sec;
	}

	return 0;
}

/* checks is mp is a mounted mountpoint */
int
check_mount(char *mp)
{
	int				nbmount, i;
	struct statvfs	*mntbuf;

	nbmount = getmntinfo(&mntbuf, MNT_NOWAIT);

	for (i = 0; i < nbmount; i++) {
		if (strcmp(mntbuf[i].f_mntonname, mp) == 0) {
			return 1;
		}
	}

	return 0;
}

/* mostly from vmstat.c */
int
update_meminfo(void)
{
	int						mib[] = { CTL_VM, VM_UVMEXP2 };
	struct uvmexp_sysctl	uvmexp;
	size_t					ssize;

	ssize = sizeof(uvmexp);
	memset(&uvmexp, 0, ssize);

	info.mem = info.memmax = info.swap = info.swapfree = info.swapmax = 0;
	info.buffers = info.cached = info.memfree = info.memeasyfree = 0;
	info.bufmem = 0;

	if (sysctl(mib, 2, &uvmexp, &ssize, NULL, 0) < 0) {
		warn("sysctl vm.uvmexp2 failed");
		return 0;
	}

	info.memmax = uvmexp.npages * uvmexp.pagesize / 1024;
	info.memfree = uvmexp.inactive * uvmexp.pagesize / 1024;

	info.swapmax = uvmexp.swpages * uvmexp.pagesize / 1024;
	info.swapfree = (uvmexp.swpages - uvmexp.swpginuse) * \
		uvmexp.pagesize / 1024;

	info.buffers = uvmexp.filepages * uvmexp.pagesize / 1024;
	info.cached = uvmexp.execpages * uvmexp.pagesize / 1024;

	info.mem = info.memmax - info.memfree;
	info.memeasyfree = info.memfree;
	info.bufmem = info.cached + info.buffers;
	info.swap = info.swapmax - info.swapfree;

	return 0;
}

int
update_net_stats(void)
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
				ns->trans += (long long) 4294967295U - ns->last_read_trans + t;
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

int
update_total_processes(void)
{
	int		mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL};
	size_t	size;

	if (sysctl(mib, 3, NULL, &size, NULL, 0) < 0) {
		warn("sysctl KERN_PROC_ALL failed");
		return 0;
	}

	info.procs = (size / sizeof (struct kinfo_proc));

	return 0;
}

int
update_running_processes()
{
	int					n_processes, i, cnt = 0;
	struct kinfo_proc2	*p;

	info.run_procs = 0;

	p = kvm_getproc2(kd, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc2),
		&n_processes);

	for (i = 0; i < n_processes; i++)
		if (p[i].p_stat == LSRUN ||
			p[i].p_stat == LSIDL || 
			p[i].p_stat == LSONPROC)
			cnt++;

	info.run_procs = cnt;

	return 0;
}

struct cpu_load_struct {
	unsigned long load[5];
};

struct cpu_load_struct fresh = {
	{0, 0, 0, 0, 0}
};

long cpu_used, oldtotal, oldused;

int update_cpu_usage(void)
{
	long used, total;
	static u_int64_t cp_time[CPUSTATES];
	size_t len = sizeof(cp_time);

	if ((cpu_setup == 0) || (!info.cpu_usage)) {
		get_cpu_count();
		cpu_setup = 1;
	}

	info.cpu_usage[0] = 0;

	if (sysctlbyname("kern.cp_time", &cp_time, &len, NULL, 0) < 0) {
		warn("cannot get kern.cp_time");
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

	return 0;
}

int update_load_average(void)
{
	double v[3];

	getloadavg(v, 3);

	info.loadavg[0] = (float) v[0];
	info.loadavg[1] = (float) v[1];
	info.loadavg[2] = (float) v[2];

	return 0;
}

double get_acpi_temperature(int fd)
{
	return -1;
}

void get_battery_stuff(char *buf, unsigned int n, const char *bat, int item)
{
}

int open_acpi_temperature(const char *name)
{
	return -1;
}

void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size, const char *adapter)
{
	(void) adapter; // only linux uses this

	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	/* not implemented */
	memset(p_client_buffer, 0, client_buffer_size);
}

/* char *get_acpi_fan() */
void get_acpi_fan(char *p_client_buffer, size_t client_buffer_size)
{
	if (!p_client_buffer || client_buffer_size <= 0) {
		return;
	}

	/* not implemented */
	memset(p_client_buffer, 0, client_buffer_size);
}

int get_entropy_avail(unsigned int *val)
{
	return 1;
}

int get_entropy_poolsize(unsigned int *val)
{
	return 1;
}

/* void */
char get_freq(char *p_client_buffer, size_t client_buffer_size,
                const char *p_format, int divisor, unsigned int cpu)
{
        int freq = cpu;

        if (!p_client_buffer || client_buffer_size <= 0 || !p_format
                        || divisor <= 0) {
                return 0;
        }

        size_t size = sizeof(freq);

	if (sysctlbyname("machdep.est.frequency.current",
			NULL, &size, NULL, 0) == 0) {
		sysctlbyname("machdep.est.frequency.current", &freq, &size, NULL, 0);
                snprintf(p_client_buffer, client_buffer_size, p_format,
                        (float) freq / divisor);
        } else {
                snprintf(p_client_buffer, client_buffer_size, p_format, 0.0f);
        }

        return 1;
}

void get_cpu_count()
{
		int cpu_count = 1; /* default to 1 cpu */

		info.cpu_count = cpu_count;

		info.cpu_usage = malloc(info.cpu_count * sizeof(float));

		if (info.cpu_usage == NULL)
			warn("malloc");
}

void update_diskio()
{
	return; /* XXX: implement? hifi: not sure how */
}

int update_top()
{
	proc_find_top(info.cpu, info.memu);

	return 0;
}

int comparecpu(const void *a, const void *b)
{
	if (((struct process *) a)->amount > ((struct process *) b)->amount) {
		return -1;
	}
	if (((struct process *) a)->amount < ((struct process *) b)->amount) {
		return 1;
	}

	return 0;
}

int comparemem(const void *a, const void *b)
{
    if (((struct process *) a)->rss > ((struct process *) b)->rss) {
		return -1;
	}

	if (((struct process *) a)->rss < ((struct process *) b)->rss) {
		return 1;
	}

	return 0;
}

inline void proc_find_top(struct process **cpu, struct process **mem)
{
        struct kinfo_proc2 *p;
        int n_processes;
        int i, j = 0;
        struct process *processes;
        int mib[2];

        u_int total_pages;
        int64_t usermem;
        int pagesize = getpagesize();

        /* we get total pages count again to be sure it is up to date */
        mib[0] = CTL_HW;
        mib[1] = HW_USERMEM64;
        size_t size = sizeof(usermem);

        if (sysctl(mib, 2, &usermem, &size, NULL, 0) == -1) {
                err(EXIT_FAILURE, "error reading usermem");
        }

        /* translate bytes into page count */
        total_pages = usermem / pagesize;

        int max_size = sizeof(struct kinfo_proc2);

        p = kvm_getproc2(kd, KERN_PROC_ALL, 0, max_size, &n_processes);
        processes = malloc(n_processes * sizeof(struct process));

        for (i = 0; i < n_processes; i++) {
			if (!((p[i].p_flag & P_SYSTEM)) && p[i].p_comm != NULL) {
				processes[j].pid = p[i].p_pid;
				processes[j].name = strndup(p[i].p_comm, text_buffer_size);
				processes[j].amount = 100.0 * p[i].p_pctcpu / FSCALE;
				processes[j].rss = p[i].p_vm_rssize * pagesize;
				processes[j].vsize = p[i].p_vm_vsize;
				j++;
			}
        }

        qsort(processes, j - 1, sizeof(struct process), comparemem);
        for (i = 0; i < 10; i++) {
			struct process *tmp, *ttmp;
				
			tmp = malloc(sizeof(struct process));
			memcpy(tmp, &processes[i], sizeof(struct process));
			tmp->name = strndup(processes[i].name, text_buffer_size);

			ttmp = mem[i];
			mem[i] = tmp;
			if (ttmp != NULL) {
				free(ttmp->name);
				free(ttmp);
			}
        }

        qsort(processes, j - 1, sizeof(struct process), comparecpu);
        for (i = 0; i < 10; i++) {
			struct process *tmp, *ttmp;

			tmp = malloc(sizeof(struct process));
			memcpy(tmp, &processes[i], sizeof(struct process));
			tmp->name = strndup(processes[i].name, text_buffer_size);
			
			ttmp = cpu[i];
			cpu[i] = tmp;
			if (ttmp != NULL) {
				free(ttmp->name);
				free(ttmp);
			}
        }

        for (i = 0; i < j; i++) {
			free(processes[i].name);
        }
        free(processes);
}
