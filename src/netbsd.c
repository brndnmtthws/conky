/* NetBSD port */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <err.h>
#include <limits.h>
#include <paths.h>

#include <kvm.h>
#include <nlist.h>

#include <sys/time.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/user.h>
#include <sys/socket.h>
#include <sys/swap.h>
#include <sys/sched.h>
#include <sys/envsys.h>

#include <net/if.h>

#include <uvm/uvm_extern.h>

#include <machine/param.h>

#include "conky.h"


static kvm_t *kd = NULL;
int kd_init = 0, nkd_init = 0;
u_int32_t sensvalue;
char errbuf[_POSIX2_LINE_MAX];

static int init_kvm(void)
{
	if (kd_init)
		return 0;

	kd = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, errbuf);
	if (kd == NULL) {
		(void) warnx("cannot kvm_openfiles: %s", errbuf);
		return -1;
	}
	kd_init = 1;
	return 0;
}

static int swapmode(int *retavail, int *retfree)
{
	int n;
	struct swapent *sep;

	*retavail = 0;
	*retfree = 0;

	n = swapctl(SWAP_NSWAP, 0, 0);

	if (n < 1) {
		(void) warn("could not get swap information");
		return 0;
	}

	sep = (struct swapent *) malloc(n * (sizeof(*sep)));

	if (sep == NULL) {
		(void) warn("memory allocation failed");
		return 0;
	}

	if (swapctl(SWAP_STATS, (void *) sep, n) < n) {
		(void) warn("could not get swap stats");
		return 0;
	}
	for (; n > 0; n--) {
		*retavail += (int) dbtob(sep[n - 1].se_nblks);
		*retfree +=
		    (int) dbtob(sep[n - 1].se_nblks - sep[n - 1].se_inuse);
	}
	*retavail = (int) (*retavail / 1024);
	*retfree = (int) (*retfree / 1024);

	return 1;
}


void prepare_update()
{
}

void update_uptime()
{
	int mib[2] = { CTL_KERN, KERN_BOOTTIME };
	struct timeval boottime;
	time_t now;
	int size = sizeof(boottime);

	if ((sysctl(mib, 2, &boottime, &size, NULL, 0) != -1)
	    && (boottime.tv_sec != 0)) {
		(void) time(&now);
		info.uptime = now - boottime.tv_sec;
	} else {
		(void) warn("could not get uptime");
		info.uptime = 0;
	}
}


void update_meminfo()
{
	int mib[] = { CTL_VM, VM_UVMEXP2 };
	int total_pages, inactive_pages, free_pages;
	int swap_avail, swap_free;
	const int pagesize = getpagesize();
	struct uvmexp_sysctl uvmexp;
	size_t size = sizeof(uvmexp);

	info.memmax = info.mem = 0;
	info.swapmax = info.swap = 0;


	if (sysctl(mib, 2, &uvmexp, &size, NULL, 0) < 0) {
		warn("could not get memory info");
		return;
	}

	total_pages = uvmexp.npages;
	free_pages = uvmexp.free;
	inactive_pages = uvmexp.inactive;

	info.memmax = (total_pages * pagesize) >> 10;
	info.mem =
	    ((total_pages - free_pages - inactive_pages) * pagesize) >> 10;

	if (swapmode(&swap_avail, &swap_free) >= 0) {
		info.swapmax = swap_avail;
		info.swap = (swap_avail - swap_free);
	}
}

void update_net_stats()
{
	int i;
	double delta;
	struct ifnet ifnet;
	struct ifnet_head ifhead;	/* interfaces are in a tail queue */
	u_long ifnetaddr;
	static struct nlist namelist[] = {
		{"_ifnet"},
		{NULL},
	};
	static kvm_t *nkd;

	if (!nkd_init) {
		nkd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, errbuf);
		if (nkd == NULL) {
			(void) warnx("cannot kvm_openfiles: %s", errbuf);
			(void)
			    warnx
			    ("maybe you need to setgid kmem this program?");
			return;
		} else if (kvm_nlist(nkd, namelist) != 0) {
			(void) warn("cannot kvm_nlist");
			return;
		} else
			nkd_init = 1;
	}

	if (kvm_read(nkd, (u_long) namelist[0].n_value, (void *) &ifhead,
		     sizeof(ifhead)) < 0) {
		(void) warn("cannot kvm_read");
		return;
	}

	/* get delta */
	delta = current_update_time - last_update_time;
	if (delta <= 0.0001)
		return;

	for (i = 0, ifnetaddr = (u_long) ifhead.tqh_first;
	     ifnet.if_list.tqe_next && i < 16;
	     ifnetaddr = (u_long) ifnet.if_list.tqe_next, i++) {

		struct net_stat *ns;
		long long last_recv, last_trans;

		(void) kvm_read(nkd, (u_long) ifnetaddr, (void *) &ifnet,
				sizeof(ifnet));
		ns = get_net_stat(ifnet.if_xname);
		ns->up = 1;
		last_recv = ns->recv;
		last_trans = ns->trans;

		if (ifnet.if_ibytes < ns->last_read_recv)
			ns->recv +=
			    ((long long) 4294967295U -
			     ns->last_read_recv) + ifnet.if_ibytes;
		else
			ns->recv += (ifnet.if_ibytes - ns->last_read_recv);

		ns->last_read_recv = ifnet.if_ibytes;

		if (ifnet.if_obytes < ns->last_read_trans)
			ns->trans +=
			    ((long long) 4294967295U -
			     ns->last_read_trans) + ifnet.if_obytes;
		else
			ns->trans +=
			    (ifnet.if_obytes - ns->last_read_trans);

		ns->last_read_trans = ifnet.if_obytes;

		ns->recv += (ifnet.if_ibytes - ns->last_read_recv);
		ns->last_read_recv = ifnet.if_ibytes;
		ns->trans += (ifnet.if_obytes - ns->last_read_trans);
		ns->last_read_trans = ifnet.if_obytes;

		ns->recv_speed = (ns->recv - last_recv) / delta;
		ns->trans_speed = (ns->trans - last_trans) / delta;
	}
}

void update_total_processes()
{
	/* It's easier to use kvm here than sysctl */

	int n_processes;

	info.procs = 0;

	if (init_kvm() < 0)
		return;
	else
		kvm_getproc2(kd, KERN_PROC_ALL, 0,
			     sizeof(struct kinfo_proc2), &n_processes);

	info.procs = n_processes;
}

void update_running_processes()
{
	struct kinfo_proc2 *p;
	int n_processes;
	int i, cnt = 0;

	info.run_procs = 0;

	if (init_kvm() < 0)
		return;
	else {
		p = kvm_getproc2(kd, KERN_PROC_ALL, 0,
				 sizeof(struct kinfo_proc2), &n_processes);
		for (i = 0; i < n_processes; i++)
			if (p[i].p_stat == LSRUN || p[i].p_stat == LSIDL ||
			    p[i].p_stat == LSONPROC)
				cnt++;
	}

	info.run_procs = cnt;
}

struct cpu_load_struct {
	unsigned long load[5];
};

struct cpu_load_struct fresh = {
	{0, 0, 0, 0, 0}
};

long cpu_used, oldtotal, oldused;

void update_cpu_usage()
{
	long used, total;
	static u_int64_t cp_time[CPUSTATES];
	size_t len = sizeof(cp_time);

	info.cpu_usage = 0;

	if (sysctlbyname("kern.cp_time", &cp_time, &len, NULL, 0) < 0)
		(void) warn("cannot get kern.cp_time");


	fresh.load[0] = cp_time[CP_USER];
	fresh.load[1] = cp_time[CP_NICE];
	fresh.load[2] = cp_time[CP_SYS];
	fresh.load[3] = cp_time[CP_IDLE];
	fresh.load[4] = cp_time[CP_IDLE];

	used = fresh.load[0] + fresh.load[1] + fresh.load[2];
	total =
	    fresh.load[0] + fresh.load[1] + fresh.load[2] + fresh.load[3];

	if ((total - oldtotal) != 0)
		info.cpu_usage =
		    ((double) (used - oldused)) / (double) (total -
							    oldtotal);
	else
		info.cpu_usage = 0;

	oldused = used;
	oldtotal = total;

}

double get_i2c_info(int *fd, int div, char *devtype)
{
	return -1;
}

void update_load_average()
{
	double v[3];
	getloadavg(v, 3);

	info.loadavg[0] = (float) v[0];
	info.loadavg[1] = (float) v[1];
	info.loadavg[2] = (float) v[2];
}

double get_acpi_temperature(int fd)
{
	return -1;
}

void get_battery_stuff(char *buf, unsigned int n, const char *bat)
{
}

int
open_i2c_sensor(const char *dev, const char *type, int n, int *div,
		char *devtype)
{
	return -1;
}

int open_acpi_temperature(const char *name)
{
	return -1;
}

char *get_acpi_ac_adapter(void)
{
	return "N/A";
}

char *get_acpi_fan()
{
	return "N/A";
}
