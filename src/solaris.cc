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
#include "common.h"
#include <kstat.h>
#include <unistd.h>
#include <limits.h>
#include <sys/loadavg.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/task.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <procfs.h>
#include <project.h>
#include <dirent.h>
#include <net/if.h>
#include <sys/sockio.h>
#include "top.h"

#include <assert.h>

#include "solaris.h"
#include "net_stat.h"

static kstat_ctl_t *kstat;
static int kstat_updated;
static int pageshift = INT_MAX;

static int pagetok(int pages)
{
	if (pageshift == INT_MAX) {
		int pagesize = sysconf(_SC_PAGESIZE);
		pageshift = 0;
		while ((pagesize >>= 1) > 0)
			pageshift++;
		pageshift -= 10; /* 2^10 = 1024 */
	}
	return (pageshift > 0 ? pages << pageshift : pages >> -pageshift);
}

static void update_kstat()
{
	if (kstat == NULL) {
		kstat = kstat_open();
		if (kstat == NULL) {
			NORM_ERR("can't open kstat: %s", strerror(errno));
		}
	}

	if (kstat_chain_update(kstat) == -1) {
		perror("kstat_chain_update");
		return;
	}
}

static kstat_named_t *get_kstat(const char *module, int inst, const char *name,
  const char *stat)
{
	kstat_t *ksp;

	update_kstat();

	ksp = kstat_lookup(kstat, (char *)module, inst, (char *)name);
	if (ksp == NULL) {
		perror("kstat_lookup");
		return NULL;
	}

	if (kstat_read(kstat, ksp, NULL) >= 0) {
		if (ksp->ks_type == KSTAT_TYPE_NAMED) {
				kstat_named_t *knp = (kstat_named_t *)kstat_data_lookup(ksp,
				  (char *)stat);
				return knp;
		} else {
			printf("ks: %p has type: %d\n", ksp, ksp->ks_type);
			assert(0);
		}
	}
	return NULL; 
}

void prepare_update()
{
	kstat_updated = 0;
}

int update_meminfo()
{
	kstat_named_t *knp;
	int nswap = swapctl(SC_GETNSWP, 0);
	struct swaptable *swt;
	struct swapent *swe;
	char path[PATH_MAX];
	unsigned long stp, sfp;
	
	update_kstat();

	/* RAM */
	knp = get_kstat("unix", -1, "system_pages", "freemem");
	if (knp != NULL)
		info.memfree = pagetok(knp->value.ui32);
	info.memmax = pagetok(sysconf(_SC_PHYS_PAGES));
	if (info.memmax > info.memfree)
		info.mem = info.memmax - info.memfree;
	else
		info.mem = info.memmax;

	/* swap */
	if (nswap < 1)
		return 0;
	/* swapctl(2) */
	swt = (struct swaptable *)malloc(nswap * sizeof (struct swapent) +
	  sizeof (int));
	if (swt == NULL)
		return 0;
	swt->swt_n = nswap;
	swe = &(swt->swt_ent[0]);
	/* We are not interested in ste_path */ 
	for (int i = 0; i < nswap; i++)
		swe[i].ste_path = path;
	nswap = swapctl(SC_LIST, swt);
	swe = &(swt->swt_ent[0]);
	stp = sfp = 0;
	for (int i = 0; i < nswap; i++) {
		if ((swe[i].ste_flags & ST_INDEL) || (swe[i].ste_flags & ST_DOINGDEL))
			continue;
		stp += swe->ste_pages;
		sfp += swe->ste_free;
	}
	free(swt);
	info.swapfree = pagetok(sfp);
	info.swapmax = pagetok(stp);
	info.swap = info.swapmax - info.swapfree;

	return 0;
}

int check_mount(struct text_object *obj)
{
	/* stub */
	(void)obj;
	return 0;
}

double get_battery_perct_bar(struct text_object *obj)
{
	return 100.0;
}

double get_acpi_temperature(int fd)
{
	return 0.0;
}

int update_total_processes(void)
{
	kstat_named_t *knp = get_kstat("unix", -1, "system_misc", "nproc");
	if (knp != NULL)
		info.procs = knp->value.ui32;
	return 0;
}

void get_battery_stuff(char *buf, unsigned int n, const char *bat, int item)
{
}

int update_running_processes(void)
{
}

int update_net_stats(void)
{
	struct ifconf ifc;
	int sockfd;
	char buf[1024];
	double d = current_update_time - last_update_time;

	if (d < 0.1)
		return 0;

	/* Find all active net interfaces. */
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return -1;
	}
	ifc.ifc_buf = buf;
	ifc.ifc_len = sizeof(buf);
	if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
		perror("ioctl(SIOCGIFCONF)");
		close(sockfd);
		return -1;
	}
	close(sockfd);

	for (int i = 0;  i < ifc.ifc_len / sizeof(struct ifreq); i++) {
		struct net_stat *ns;
		struct ifreq *ifr = &ifc.ifc_req[i];
		long long last_recv, last_trans;
		long long r, t;
		kstat_named_t *knp;

		ns = get_net_stat((const char *) ifr->ifr_name, NULL, NULL);
		ns->up = 1;
		memcpy(&(ns->addr), &ifr->ifr_addr, sizeof(ifr->ifr_addr));

		/* loopback interface does not have kstat data */
		if (ifr->ifr_flags & IFF_LOOPBACK || strcmp(ifr->ifr_name, "lo0") == 0)
			continue;
		last_recv = ns->recv;
		last_trans = ns->trans;

		/* get received bytes */
		knp = get_kstat("link", -1, ifr->ifr_name, "rbytes");
		if (knp == NULL) {
			printf("cannot read rbytes for %s\n", ifr->ifr_name);
			continue;
		}
		r = (long long)knp->value.ui32;
		if (r <= ns->last_read_recv) {
			ns->recv += ((long long) 4294967295U - ns->last_read_recv) + r;
		} else {
			ns->recv += (r - ns->last_read_recv);
		}
		ns->last_read_recv = r;

		/* get transceived bytes */
		knp = get_kstat("link", -1, ifr->ifr_name, "obytes");
		if (knp == NULL) {
			printf("cannot read obytes for %s\n", ifr->ifr_name);
			continue;
		}
		t = (long long)knp->value.ui32;
		if (t < ns->last_read_trans) {
			ns->trans += ((long long) 4294967295U - ns->last_read_trans) + t;
		} else {
			ns->trans += (t - ns->last_read_trans);
		}
		ns->last_read_trans = t;

		ns->recv_speed = (ns->recv - last_recv) / d;
		ns->trans_speed = (ns->trans - last_trans) / d;
	}
	return 0;
}

int update_cpu_usage(void)
{
	static int last_cpu_cnt = 0;
	static int *last_cpu_use = NULL;
	double d = current_update_time - last_update_time;
	int cpu;

	if (d < 0.1)
		return 0;

	update_kstat();

	info.cpu_count = sysconf(_SC_NPROCESSORS_ONLN);

	if (last_cpu_cnt != info.cpu_count || last_cpu_use == NULL) { 
		last_cpu_use = (int *)realloc(last_cpu_use,
		  info.cpu_count * sizeof (int));
		last_cpu_cnt = info.cpu_count;
		if (last_cpu_use == NULL)
			return 0;
	}

	info.cpu_usage = (float *)malloc(info.cpu_count * sizeof(float));

	for (cpu = 0; cpu < info.cpu_count; cpu++) {
		char stat_name[PATH_MAX];
		unsigned long cpu_user, cpu_nice, cpu_system, cpu_idle;
		unsigned long cpu_use;
		cpu_stat_t *cs;
		kstat_t *ksp;

		snprintf(stat_name, PATH_MAX, "cpu_stat%d", cpu);
		ksp = kstat_lookup(kstat, (char *)"cpu_stat", cpu, stat_name);
		if (ksp == NULL)
			continue;
		if (kstat_read(kstat, ksp, NULL) == -1)
			continue;
		cs = (cpu_stat_t *)ksp->ks_data;
	
		cpu_idle = cs->cpu_sysinfo.cpu[CPU_IDLE];
	    cpu_user = cs->cpu_sysinfo.cpu[CPU_USER];
	    cpu_nice = cs->cpu_sysinfo.cpu[CPU_WAIT]; 
	    cpu_system = cs->cpu_sysinfo.cpu[CPU_KERNEL];

		cpu_use = cpu_user + cpu_nice + cpu_system;

		info.cpu_usage[cpu] = (double)(cpu_use - last_cpu_use[cpu]) / d / 100.0;
		last_cpu_use[cpu] = cpu_use;
	}

	return 0;
}

void update_proc_entry(struct process *p)
{
	psinfo_t proc;
	int fd;
	double prc;
	char pfn[PATH_MAX];

	snprintf(pfn, PATH_MAX, "/proc/%d/psinfo", p->pid);
	if ((fd = open(pfn, O_RDONLY)) < 0) {
		printf("cannot read pid %d, err: %d\n", p->pid, errno);
		return;
	}
	if (pread(fd, &proc, sizeof(psinfo_t), 0) != sizeof(psinfo_t)) {
		printf("%s has wrong size\n", pfn);
		(void) close(fd);
		return;
	}
	(void) close(fd);
	free_and_zero(p->name);
	free_and_zero(p->basename);
	p->name = strndup(proc.pr_fname, text_buffer_size.get(*::state));
	p->basename = strndup(proc.pr_fname, text_buffer_size.get(*::state));
	p->uid = proc.pr_uid;
	/* p->amount = proc.pr_pctcpu; */
	prc = (double)proc.pr_pctcpu / (double)0x8000 * 100.0;
	p->amount = prc;
	p->rss = proc.pr_rssize * 1024;		/* to bytes */
	p->vsize = proc.pr_size * 1024;		/* to bytes */
	p->total_cpu_time = proc.pr_time.tv_sec * 100; /* to hundredths of secs */
	if (proc.pr_lwp.pr_sname == 'O' || proc.pr_lwp.pr_sname == 'R')
		info.run_procs++;

	p->time_stamp = g_time;
}

void get_top_info(void)
{
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir("/proc"))) {
		return;
	}
	info.run_procs = 0;

	while ((entry = readdir(dir))) {
		pid_t pid;

		if (entry == NULL) 
			break;
		if (sscanf(entry->d_name, "%d", &pid) != 1)
			continue;
		update_proc_entry(get_process(pid));
	}
	(void) closedir(dir);
}

int update_diskio(void)
{
	return 0;
}

void get_battery_short_status(char *buffer, unsigned int n, const char *bat)
{

}

void get_acpi_fan(char *p_client_buffer, size_t client_buffer_size)
{
}

void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size,
    const char *adapter)
{
	/* Not implemented */
	if (p_client_buffer && client_buffer_size > 0) {
		memset(p_client_buffer, 0, client_buffer_size);
	}
}

int get_battery_perct(const char *bat)
{
	(void)bat;
	return 1;
}

int get_entropy_poolsize(unsigned int *val)
{
	/* Not implemented */
	(void)val;
	return 1;
}

char get_freq(char *p_client_buffer, size_t client_buffer_size,
  const char *p_format, int divisor, unsigned int cpu)
{
	char stat_name[PATH_MAX];
	kstat_named_t *knp;

	snprintf(stat_name, PATH_MAX, "cpu_info%d", cpu - 1);
	knp = get_kstat("cpu_info", cpu - 1, stat_name, "current_clock_Hz");
	if (knp == NULL)
		return 0;
	snprintf(p_client_buffer, client_buffer_size, p_format,
		(float) knp->value.ui32 / divisor / 1000000.0);
	return 1;
}

int update_uptime(void)
{
	kstat_named_t *knp = get_kstat("unix", -1, "system_misc", "boot_time");
	if (knp != NULL)
		info.uptime = time(NULL) - knp->value.ui32;
	return 0;
}

int open_acpi_temperature(const char *name)
{
	printf("open_acpi_temperature: '%s'\n", name);
	return 0;
}

int get_entropy_avail(unsigned int *val)
{
	/* Not implemented */
	(void)val;
	return 1;
}

int update_load_average(void)
{
	double load[3];

	getloadavg(load, 3);
	info.loadavg[0] = (float) load[0];
	info.loadavg[1] = (float) load[1];
	info.loadavg[2] = (float) load[2];

	return 0;
}

void get_cpu_count(void)
{
	kstat_named_t *knp = get_kstat("unix", -1, "system_misc", "ncpus");
	if (knp != NULL)
		info.cpu_count = knp->value.ui32;
}
