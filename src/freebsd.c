/** freebsd.c
 * Contains FreeBSD specific stuff
 *
 * $Id$
 */

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kvm.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <sys/dkstat.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_mib.h>
#include <sys/socket.h>
#include <ifaddrs.h>

#include "conky.h"

#define GETSYSCTL(name, var) getsysctl(name, &(var), sizeof(var))
#define KELVTOC(x)      ((x - 2732) / 10.0)

#if defined(i386) || defined(__i386__)
static unsigned int get_timer();
static unsigned int get_cpu_speed(void);
static inline unsigned long long int rdtsc(void);

/* cpu frequency detection code based on mplayer's one */

static unsigned int get_timer()
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);

	return (tv.tv_sec * 1000000 + tv.tv_usec);
}

static inline unsigned long long int rdtsc(void)
{
	unsigned long long int retval;
	__asm __volatile("rdtsc":"=A"(retval)::"memory");
	return retval;
}

static unsigned int get_cpu_speed(void)
{
	unsigned long long int tscstart, tscstop;
	unsigned int start, stop;

	tscstart = rdtsc();
	start = get_timer();
	usleep(50000);
	stop = get_timer();
	tscstop = rdtsc();

	return ((tscstop - tscstart) / ((stop - start) / 1000.0));
}
#endif /* i386 */

static int getsysctl(char *name, void *ptr, size_t len)
{
	size_t nlen = len;
	if (sysctlbyname(name, ptr, &nlen, NULL, 0) == -1) {
		return -1;
	}

	if (nlen != len) {
		return -1;
	}

	return 0;
}

static kvm_t *kd = NULL;
struct ifmibdata *data = NULL;
size_t len = 0;

static int swapmode(int *retavail, int *retfree)
{
	int n;
	int pagesize = getpagesize();
	struct kvm_swap swapary[1];
	static int kd_init = 1;

	if (kd_init) {
		kd_init = 0;
		if ((kd = kvm_open("/dev/null", "/dev/null", "/dev/null",
				   O_RDONLY, "kvm_open")) == NULL) {
			(void) fprintf(stderr, "Cannot read kvm.");
			return -1;
		}
	}

	if (kd == NULL) {
		return -1;
	}

	*retavail = 0;
	*retfree = 0;

#define CONVERT(v)      ((quad_t)(v) * pagesize / 1024)

	n = kvm_getswapinfo(kd, swapary, 1, 0);
	if (n < 0 || swapary[0].ksw_total == 0)
		return (0);

	*retavail = CONVERT(swapary[0].ksw_total);
	*retfree = CONVERT(swapary[0].ksw_total - swapary[0].ksw_used);

	n = (int) ((double) swapary[0].ksw_used * 100.0 /
		   (double) swapary[0].ksw_total);

	return n;
}

void prepare_update()
{
}

void update_uptime()
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
		(void) fprintf(stderr, "Could not get uptime\n");
		info.uptime = 0;
	}
}

void update_meminfo()
{
	int total_pages, inactive_pages, free_pages;
	int swap_avail, swap_free;

	int pagesize = getpagesize();

	if (GETSYSCTL("vm.stats.vm.v_page_count", total_pages))
		(void) fprintf(stderr,
			       "Cannot read sysctl \"vm.stats.vm.v_page_count\"");

	if (GETSYSCTL("vm.stats.vm.v_free_count", free_pages))
		(void) fprintf(stderr,
			       "Cannot read sysctl \"vm.stats.vm.v_free_count\"");

	if (GETSYSCTL("vm.stats.vm.v_inactive_count", inactive_pages))
		(void) fprintf(stderr,
			       "Cannot read sysctl \"vm.stats.vm.v_inactive_count\"");

	info.memmax = (total_pages * pagesize) >> 10;
	info.mem =
	    ((total_pages - free_pages - inactive_pages) * pagesize) >> 10;


	if ((swapmode(&swap_avail, &swap_free)) >= 0) {
		info.swapmax = swap_avail;
		info.swap = (swap_avail - swap_free);
	} else {
		info.swapmax = 0;
		info.swap = 0;
	}
}

void update_net_stats()
{
	struct net_stat *ns;
	double delta;
	long long r, t, last_recv, last_trans;
	struct ifaddrs *ifap, *ifa;
	struct if_data *ifd;


	/* get delta */
	delta = current_update_time - last_update_time;
	if (delta <= 0.0001)
		return;

	if (getifaddrs(&ifap) < 0)
		return;

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		ns = get_net_stat((const char *) ifa->ifa_name);

		if (ifa->ifa_flags & IFF_UP) {
			last_recv = ns->recv;
			last_trans = ns->trans;

			if (ifa->ifa_addr->sa_family != AF_LINK)
				continue;

			ifd = (struct if_data *) ifa->ifa_data;
			r = ifd->ifi_ibytes;
			t = ifd->ifi_obytes;

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


			/* calculate speeds */
			ns->recv_speed = (ns->recv - last_recv) / delta;
			ns->trans_speed = (ns->trans - last_trans) / delta;
		}
	}

	freeifaddrs(ifap);
}

void update_total_processes()
{
	int n_processes;
	static int kd_init = 1;

	if (kd_init) {
		kd_init = 0;
		if ((kd = kvm_open("/dev/null", "/dev/null", "/dev/null",
				   O_RDONLY, "kvm_open")) == NULL) {
			(void) fprintf(stderr, "Cannot read kvm.");
			return;
		}
	}


	if (kd != NULL)
		kvm_getprocs(kd, KERN_PROC_ALL, 0, &n_processes);
	else
		return;

	info.procs = n_processes;
}

void update_running_processes()
{
	static int kd_init = 1;
	struct kinfo_proc *p;
	int n_processes;
	int i, cnt = 0;

	if (kd_init) {
		kd_init = 0;
		if ((kd =
		     kvm_open("/dev/null", "/dev/null", "/dev/null",
			      O_RDONLY, "kvm_open")) == NULL) {
			(void) fprintf(stderr, "Cannot read kvm.");
		}
	}

	if (kd != NULL) {
		p = kvm_getprocs(kd, KERN_PROC_ALL, 0, &n_processes);
		for (i = 0; i < n_processes; i++) {
#if __FreeBSD__ < 5
			if (p[i].kp_proc.p_stat == SRUN)
#else
			if (p[i].ki_stat == SRUN)
#endif
				cnt++;
		}
	} else
		return;

	info.run_procs = cnt;
}

struct cpu_load_struct {
	unsigned long load[5];
};

struct cpu_load_struct fresh = { {0, 0, 0, 0, 0} };
long cpu_used, oldtotal, oldused;

void update_cpu_usage()
{
	long used, total;
	long cp_time[CPUSTATES];
	size_t len = sizeof(cp_time);

	if (sysctlbyname("kern.cp_time", &cp_time, &len, NULL, 0) < 0) {
		(void) fprintf(stderr, "Cannot get kern.cp_time");
	}

	fresh.load[0] = cp_time[CP_USER];
	fresh.load[1] = cp_time[CP_NICE];
	fresh.load[2] = cp_time[CP_SYS];
	fresh.load[3] = cp_time[CP_IDLE];
	fresh.load[4] = cp_time[CP_IDLE];

	used = fresh.load[0] + fresh.load[1] + fresh.load[2];
	total =
	    fresh.load[0] + fresh.load[1] + fresh.load[2] + fresh.load[3];

	if ((total - oldtotal) != 0) {
		info.cpu_usage =
		    ((double) (used - oldused)) / (double) (total -
							    oldtotal);
	} else {
		info.cpu_usage = 0;
	}

	oldused = used;
	oldtotal = total;
}

double get_i2c_info(int *fd, int arg, char *devtype, char *type)
{
	return 0;
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
	int temp;

	if (GETSYSCTL("hw.acpi.thermal.tz0.temperature", temp)) {
		(void) fprintf(stderr,
			       "Cannot read sysctl \"hw.acpi.thermal.tz0.temperature\"\n");
		return 0.0;
	}

	return KELVTOC(temp);
}

void get_battery_stuff(char *buf, unsigned int n, const char *bat)
{
	int battime;

	if (GETSYSCTL("hw.acpi.battery.time", battime))
		(void) fprintf(stderr,
			       "Cannot read sysctl \"hw.acpi.battery.time\"\n");

	if (battime != -1)
		snprintf(buf, n, "Discharging, remaining %d:%2.2d",
			 battime / 60, battime % 60);
	else
		snprintf(buf, n, "Battery is charging");
}

int
open_i2c_sensor(const char *dev, const char *type, int n, int *div,
		char *devtype)
{
	return 0;
}

int open_acpi_temperature(const char *name)
{
	return 0;
}

char *get_acpi_ac_adapter(void)
{
	int state;
	char *acstate = (char *) malloc(100);

	if (GETSYSCTL("hw.acpi.acline", state)) {
		(void) fprintf(stderr,
			       "Cannot read sysctl \"hw.acpi.acline\"\n");
		return "n\\a";
	}


	if (state)
		strcpy(acstate, "Running on AC Power");
	else
		strcpy(acstate, "Running on battery");

	return acstate;
}

char *get_acpi_fan()
{
	return "";
}

char *get_adt746x_cpu()
{
	return "";
}

char *get_adt746x_fan()
{
	return "";
}

float get_freq_dynamic()
{
    /* TODO: implement */
    return get_freq();
}

float get_freq()
{
	/* First, try to obtain CPU frequency via dev.cpu.0.freq sysctl
	 * (cpufreq(4)). If failed, do i386 magic. */
	int freq;
	
	if (GETSYSCTL("dev.cpu.0.freq", freq) == 0)
		return (float)freq;
	else {
#if defined(i386) || defined(__i386__)
		int i;

		i = 0;
		if ((i = get_cpu_speed()) > 0) {
			return (float)(i / 1000);
		} else
			return 0;
	}
#else
		return 0;
	}
#endif /* i386 */
}

void update_top()
{
	/* XXX */
}

void update_wifi_stats()
{
	/* XXX */
}

void update_diskio()
{
	/* XXX */
}

