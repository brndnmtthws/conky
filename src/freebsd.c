/*
 * freebsd.c
 * Contains FreeBSD specific stuff
 *
 * $Id$
 */

#include <sys/dkstat.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/vmmeter.h>
#include <sys/user.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_mib.h>
#include <net/if_media.h>
#include <net/if_var.h>
#include <netinet/in.h>

#include <devstat.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dev/wi/if_wavelan_ieee.h>

#include "conky.h"

#define	GETSYSCTL(name, var)	getsysctl(name, &(var), sizeof (var))
#define	KELVTOC(x)		((x - 2732) / 10.0)
#define	MAXSHOWDEVS		16

#if 0
#define	FREEBSD_DEBUG
#endif

inline void proc_find_top(struct process **cpu, struct process **mem);

u_int64_t diskio_prev = 0;
static short cpu_setup = 0;
static short diskio_setup = 0;

static int getsysctl(char *name, void *ptr, size_t len)
{
	size_t nlen = len;
	if (sysctlbyname(name, ptr, &nlen, NULL, 0) == -1) {
		return (-1);
	}

	if (nlen != len) {
		return (-1);
	}

	return (0);
}

struct ifmibdata *data = NULL;
size_t len = 0;

static int swapmode(int *retavail, int *retfree)
{
	int n;
	int pagesize = getpagesize();
	struct kvm_swap swapary[1];

	*retavail = 0;
	*retfree = 0;

#define	CONVERT(v)	((quad_t)(v) * pagesize / 1024)

	n = kvm_getswapinfo(kd, swapary, 1, 0);
	if (n < 0 || swapary[0].ksw_total == 0)
		return (0);

	*retavail = CONVERT(swapary[0].ksw_total);
	*retfree = CONVERT(swapary[0].ksw_total - swapary[0].ksw_used);

	n = (int) ((double) swapary[0].ksw_used * 100.0 /
		(double) swapary[0].ksw_total);

	return (n);
}

void
prepare_update()
{
}

void
update_uptime()
{
	int mib[2] = { CTL_KERN, KERN_BOOTTIME };
	struct timeval boottime;
	time_t now;
	size_t size = sizeof (boottime);

	if ((sysctl(mib, 2, &boottime, &size, NULL, 0) != -1) &&
			(boottime.tv_sec != 0)) {
		time(&now);
		info.uptime = now - boottime.tv_sec;
	} else {
		fprintf(stderr, "Could not get uptime\n");
		info.uptime = 0;
	}
}

void
update_meminfo()
{
	int total_pages, inactive_pages, free_pages;
	int swap_avail, swap_free;

	int pagesize = getpagesize();

	if (GETSYSCTL("vm.stats.vm.v_page_count", total_pages))
		fprintf(stderr,
			"Cannot read sysctl \"vm.stats.vm.v_page_count\"");

	if (GETSYSCTL("vm.stats.vm.v_free_count", free_pages))
		fprintf(stderr,
			"Cannot read sysctl \"vm.stats.vm.v_free_count\"");

	if (GETSYSCTL("vm.stats.vm.v_inactive_count", inactive_pages))
		fprintf(stderr,
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

void
update_net_stats()
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
			struct ifaddrs *iftmp;

			ns->up = 1;
			ns->linkstatus = 1;
			last_recv = ns->recv;
			last_trans = ns->trans;

			if (ifa->ifa_addr->sa_family != AF_LINK)
				continue;

			for (iftmp = ifa->ifa_next; iftmp != NULL &&
				strcmp(ifa->ifa_name, iftmp->ifa_name) == 0;
				iftmp = iftmp->ifa_next)
				if (iftmp->ifa_addr->sa_family == AF_INET)
					memcpy(&(ns->addr), iftmp->ifa_addr,
						iftmp->ifa_addr->sa_len);

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
		} else {
			ns->up = 0;
			ns->linkstatus = 0;
		}
	}

	freeifaddrs(ifap);
}

void
update_total_processes()
{
	int n_processes;

	kvm_getprocs(kd, KERN_PROC_ALL, 0, &n_processes);

	info.procs = n_processes;
}

void
update_running_processes()
{
	struct kinfo_proc *p;
	int n_processes;
	int i, cnt = 0;

	p = kvm_getprocs(kd, KERN_PROC_ALL, 0, &n_processes);
	for (i = 0; i < n_processes; i++) {
#if __FreeBSD__ < 5
		if (p[i].kp_proc.p_stat == SRUN)
#else
		if (p[i].ki_stat == SRUN)
#endif
			cnt++;
	}

	info.run_procs = cnt;
}

struct cpu_load_struct {
	unsigned long load[5];
};

struct cpu_load_struct fresh = { {0, 0, 0, 0, 0} };
long cpu_used, oldtotal, oldused;

void
get_cpu_count()
{
	/* int cpu_count = 0; */

	/*
	 * XXX
	 * FreeBSD doesn't allow to get per CPU load stats
	 * on SMP machines. It's possible to get a CPU count,
	 * but as we fulfil only info.cpu_usage[0], it's better
	 * to report there's only one CPU. It should fix some bugs
	 * (e.g. cpugraph)
	 */
#if 0
	if (GETSYSCTL("hw.ncpu", cpu_count) == 0)
		info.cpu_count = cpu_count;
#endif
	info.cpu_count = 1;

	info.cpu_usage = malloc(info.cpu_count * sizeof (float));
	if (info.cpu_usage == NULL)
		CRIT_ERR("malloc");
}

/* XXX: SMP support */
void
update_cpu_usage()
{
	long used, total;
	long cp_time[CPUSTATES];
	size_t len = sizeof (cp_time);

	/* add check for !info.cpu_usage since that mem is freed on a SIGUSR1 */
	if ((cpu_setup == 0) || (!info.cpu_usage)) {
		get_cpu_count();
		cpu_setup = 1;
	}

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
		info.cpu_usage[0] = ((double) (used - oldused)) /
			(double) (total - oldtotal);
	} else {
		info.cpu_usage[0] = 0;
	}

	oldused = used;
	oldtotal = total;
}

double
get_i2c_info(int *fd, int arg, char *devtype, char *type)
{
	return (0);
}

void
update_load_average()
{
	double v[3];
	getloadavg(v, 3);

	info.loadavg[0] = (float) v[0];
	info.loadavg[1] = (float) v[1];
	info.loadavg[2] = (float) v[2];
}

double
get_acpi_temperature(int fd)
{
	int temp;

	if (GETSYSCTL("hw.acpi.thermal.tz0.temperature", temp)) {
		fprintf(stderr,
		"Cannot read sysctl \"hw.acpi.thermal.tz0.temperature\"\n");
		return (0.0);
	}

	return (KELVTOC(temp));
}

void
get_battery_stuff(char *buf, unsigned int n, const char *bat, int item)
{
	int battime, batcapacity, batstate, ac;
	char battery_status[64];
	char battery_time[64];

	if (GETSYSCTL("hw.acpi.battery.time", battime))
		(void) fprintf(stderr,
			"Cannot read sysctl \"hw.acpi.battery.time\"\n");
	if (GETSYSCTL("hw.acpi.battery.life", batcapacity))
		(void) fprintf(stderr,
					   "Cannot read sysctl \"hw.acpi.battery.life\"\n");

	if (GETSYSCTL("hw.acpi.battery.state", batstate))
		(void) fprintf(stderr,
					   "Cannot read sysctl \"hw.acpi.battery.state\"\n");

	if (GETSYSCTL("hw.acpi.acline", ac))
		(void) fprintf(stderr,
					   "Cannot read sysctl \"hw.acpi.acline\"\n");

	if (batstate == 1) {
		if (battime != -1) {
			snprintf (battery_status, sizeof(battery_status)-1,
				  "remaining %d%%", batcapacity);
			snprintf (battery_time, sizeof(battery_time)-1,
				  "%d:%2.2d", battime / 60, battime % 60);
			/*
			snprintf(buf, n, "remaining %d%% (%d:%2.2d)",
					batcapacity, battime / 60, battime % 60);
			*/
		}
		else
			/* no time estimate available yet */
			snprintf(battery_status, sizeof(battery_status)-1,
				 "remaining %d%%", batcapacity);
			/*
			snprintf(buf, n, "remaining %d%%",
					batcapacity);
			*/
		if (ac == 1)
			(void) fprintf(stderr, "Discharging while on AC!\n");
	} else {
		snprintf (battery_status, sizeof(battery_status)-1,
			  batstate == 2 ? "charging (%d%%)" : "charged (%d%%)", batcapacity);
		/*
		snprintf(buf, n, batstate == 2 ? "charging (%d%%)" : "charged (%d%%)", batcapacity);
		*/
		if (batstate != 2 && batstate != 0)
			(void) fprintf(stderr, "Unknown battery state %d!\n", batstate);
		if (ac == 0)
			(void) fprintf(stderr, "Charging while not on AC!\n");
	}

	switch (item) {
        case BATTERY_STATUS:
                {
                        snprintf(buf, n, "%s", battery_status);
                        break;
                }
        case BATTERY_TIME:
                {
                        snprintf(buf, n, "%s", battery_time);
                        break;
                }
        default:
                        break;
        }
        return;
}

int
open_i2c_sensor(const char *dev, const char *type, int n, int *div,
		char *devtype)
{
	return (0);
}

int
open_acpi_temperature(const char *name)
{
	return (0);
}

void
get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size)
{
	int state;

	if (!p_client_buffer || client_buffer_size <= 0)
		return;

	if (GETSYSCTL("hw.acpi.acline", state)) {
		fprintf(stderr,
			"Cannot read sysctl \"hw.acpi.acline\"\n");
		return;
	}


	if (state)
		strncpy(p_client_buffer, "Running on AC Power",
				client_buffer_size);
	else
		strncpy(p_client_buffer, "Running on battery",
				client_buffer_size);

}

void
get_acpi_fan(char *p_client_buffer, size_t client_buffer_size)
{
	if (!p_client_buffer || client_buffer_size <= 0)
		return;

	/* not implemented */
	memset(p_client_buffer, 0, client_buffer_size);
}

void
get_adt746x_cpu(char *p_client_buffer, size_t client_buffer_size)
{
	if (!p_client_buffer || client_buffer_size <= 0)
		return;

	/* not implemented */
	memset(p_client_buffer, 0, client_buffer_size);
}

void
get_adt746x_fan(char *p_client_buffer, size_t client_buffer_size)
{
	if (!p_client_buffer || client_buffer_size <= 0)
		return;

	/* not implemented */
	memset(p_client_buffer, 0, client_buffer_size);
}

/* rdtsc() and get_freq_dynamic() copied from linux.c */

#if  defined(__i386) || defined(__x86_64)
__inline__ unsigned long long int
rdtsc()
{
	unsigned long long int x;
	__asm__ volatile(".byte 0x0f, 0x31":"=A" (x));
	return (x);
}
#endif

/* return system frequency in MHz (use divisor=1) or GHz (use divisor=1000) */
void
get_freq_dynamic(char *p_client_buffer, size_t client_buffer_size,
		char *p_format, int divisor)
{
#if  defined(__i386) || defined(__x86_64)
	struct timezone tz;
	struct timeval tvstart, tvstop;
	unsigned long long cycles[2];	/* gotta be 64 bit */
	unsigned int microseconds;	/* total time taken */

	memset(&tz, 0, sizeof (tz));

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
		(float)((cycles[1] - cycles[0]) / microseconds) / divisor);
#else
	get_freq(p_client_buffer, client_buffer_size, p_format, divisor);
#endif
}

/*void*/
char
get_freq(char *p_client_buffer, size_t client_buffer_size,
		char *p_format, int divisor, unsigned int cpu)
{
	int freq;
	char *freq_sysctl;

	freq_sysctl = (char *)calloc(16, sizeof(char));
	if (freq_sysctl == NULL)
		exit(-1);

	snprintf(freq_sysctl, 16, "dev.cpu.%d.freq", (cpu - 1));
	
	if (!p_client_buffer || client_buffer_size <= 0 ||
			!p_format || divisor <= 0)
		return 0;

	if (GETSYSCTL(freq_sysctl, freq) == 0)
		snprintf(p_client_buffer, client_buffer_size,
				p_format, (float)freq/divisor);
	else
		snprintf(p_client_buffer, client_buffer_size, p_format, 0.0f);

	free(freq_sysctl);
	return 1;
}

void
update_top()
{
	proc_find_top(info.cpu, info.memu);
}

void
update_wifi_stats()
{
	struct ifreq ifr;		/* interface stats */
	struct wi_req wireq;
	struct net_stat * ns;
	struct ifaddrs *ifap, *ifa;
	struct ifmediareq ifmr;
	int s;

	/*
	 * Get iface table
	 */
	if (getifaddrs(&ifap) < 0)
		return;

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		ns = get_net_stat((const char *) ifa->ifa_name);

		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		/* Get media type */
		bzero(&ifmr, sizeof(ifmr));
		strlcpy(ifmr.ifm_name, ifa->ifa_name, IFNAMSIZ);
		if (ioctl(s, SIOCGIFMEDIA, (caddr_t) &ifmr) < 0)
			goto cleanup;
		
		/*
		 * We can monitor only wireless interfaces
		 * which not in hostap mode
		 */
		if ((ifmr.ifm_active & IFM_IEEE80211) &&
				!(ifmr.ifm_active & IFM_IEEE80211_HOSTAP)) {
			/* Get wi status */
			bzero(&ifr, sizeof(ifr));
			strlcpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ);
			wireq.wi_type	= WI_RID_COMMS_QUALITY;
			wireq.wi_len	= WI_MAX_DATALEN;
			ifr.ifr_data	= (void *) &wireq;

			if (ioctl(s, SIOCGWAVELAN, (caddr_t) &ifr) < 0) {
				perror("ioctl (getting wi status)");
				exit(1);
			}

			/*
			 * wi_val[0] = quality
			 * wi_val[1] = signal
			 * wi_val[2] = noise
			 */
			ns->linkstatus = (int) wireq.wi_val[1];
		}
cleanup:
		close(s);
	}
}
void
update_diskio()
{
	int devs_count,
	    num_selected,
	    num_selections;
	struct device_selection *dev_select = NULL;
	long select_generation;
	int dn;
	static struct statinfo  statinfo_cur;
	u_int64_t diskio_current = 0;

	bzero(&statinfo_cur, sizeof (statinfo_cur));
	statinfo_cur.dinfo = (struct devinfo *)malloc(sizeof (struct devinfo));
	bzero(statinfo_cur.dinfo, sizeof (struct devinfo));

	if (devstat_getdevs(NULL, &statinfo_cur) < 0)
		return;

	devs_count = statinfo_cur.dinfo->numdevs;
	if (devstat_selectdevs(&dev_select, &num_selected, &num_selections,
			&select_generation, statinfo_cur.dinfo->generation,
			statinfo_cur.dinfo->devices, devs_count, NULL, 0,
			NULL, 0, DS_SELECT_ONLY, MAXSHOWDEVS, 1) >= 0) {
		for (dn = 0; dn < devs_count; ++dn) {
			int di;
			struct devstat  *dev;

			di = dev_select[dn].position;
			dev = &statinfo_cur.dinfo->devices[di];

			diskio_current += dev->bytes[DEVSTAT_READ] +
				dev->bytes[DEVSTAT_WRITE];
		}

		free(dev_select);
	}

	/*
	 * Since we return (diskio_total_current - diskio_total_old), first
	 * frame will be way too high (it will be equal to
	 * diskio_total_current, i.e. all disk I/O since boot). That's why
	 * it is better to return 0 first time;
	 */
	if (diskio_setup == 0) {
		diskio_setup = 1;
		diskio_value = 0;
	} else
		diskio_value = (unsigned int)((diskio_current - diskio_prev)/
				1024);
	diskio_prev = diskio_current;

	free(statinfo_cur.dinfo);
}

/*
 * While topless is obviously better, top is also not bad.
 */

int
comparecpu(const void *a, const void *b)
{
	if (((struct process *)a)->amount > ((struct process *)b)->amount)
		return (-1);

	if (((struct process *)a)->amount < ((struct process *)b)->amount)
		return (1);

	return (0);
}

int
comparemem(const void *a, const void *b)
{
	if (((struct process *)a)->totalmem > ((struct process *)b)->totalmem)
		return (-1);

	if (((struct process *)a)->totalmem < ((struct process *)b)->totalmem)
		return (1);

	return (0);
}

inline void
proc_find_top(struct process **cpu, struct process **mem)
{
	struct kinfo_proc *p;
	int n_processes;
	int i, j = 0;
	struct process *processes;

	int total_pages;

	/* we get total pages count again to be sure it is up to date */
	if (GETSYSCTL("vm.stats.vm.v_page_count", total_pages) != 0)
		CRIT_ERR("Cannot read sysctl"
			"\"vm.stats.vm.v_page_count\"");

	p = kvm_getprocs(kd, KERN_PROC_PROC, 0, &n_processes);
	processes = malloc(n_processes * sizeof (struct process));

	for (i = 0; i < n_processes; i++) {
		if (!((p[i].ki_flag & P_SYSTEM)) &&
				p[i].ki_comm != NULL) {
			processes[j].pid = p[i].ki_pid;
			processes[j].name =  strdup(p[i].ki_comm);
			processes[j].amount = 100.0 *
				p[i].ki_pctcpu / FSCALE;
			processes[j].totalmem = (float)(p[i].ki_rssize /
					(float)total_pages) * 100.0;
			j++;
		}
	}

	qsort(processes, j - 1, sizeof (struct process), comparemem);
	for (i = 0; i < 10; i++) {
		struct process *tmp, *ttmp;

		tmp = malloc(sizeof (struct process));
		tmp->pid = processes[i].pid;
		tmp->amount = processes[i].amount;
		tmp->totalmem = processes[i].totalmem;
		tmp->name = strdup(processes[i].name);

		ttmp = mem[i];
		mem[i] = tmp;
		if (ttmp != NULL) {
			free(ttmp->name);
			free(ttmp);
		}
	}

	qsort(processes, j - 1, sizeof (struct process), comparecpu);
	for (i = 0; i < 10; i++) {
		struct process *tmp, *ttmp;

		tmp = malloc(sizeof (struct process));
		tmp->pid = processes[i].pid;
		tmp->amount = processes[i].amount;
		tmp->totalmem = processes[i].totalmem;
		tmp->name = strdup(processes[i].name);

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
		printf("%d: %s(%d) %.2f\n", i, mem[i]->name,
				mem[i]->pid, mem[i]->totalmem);
	}
#endif

	for (i = 0; i < j; free(processes[i++].name));
	free(processes);
}

#if	defined(i386) || defined(__i386__)
#define	APMDEV		"/dev/apm"
#define	APM_UNKNOWN	255

int
apm_getinfo(int fd, apm_info_t aip)
{
	if (ioctl(fd, APMIO_GETINFO, aip) == -1)
		return (-1);

	return (0);
}

char
*get_apm_adapter()
{
	int fd;
	struct apm_info info;

	fd = open(APMDEV, O_RDONLY);
	if (fd < 0)
		return ("ERR");

	if (apm_getinfo(fd, &info) != 0) {
		close(fd);
		return ("ERR");
	}
	close(fd);

	switch (info.ai_acline) {
		case 0:
			return ("off-line");
			break;
		case 1:
			if (info.ai_batt_stat == 3)
				return ("charging");
			else
				return ("on-line");
			break;
		default:
			return ("unknown");
			break;
	}
}

char
*get_apm_battery_life()
{
	int fd;
	u_int batt_life;
	struct apm_info info;
	char *out;

	out = (char *)calloc(16, sizeof (char));

	fd = open(APMDEV, O_RDONLY);
	if (fd < 0) {
		strncpy(out, "ERR", 16);
		return (out);
	}

	if (apm_getinfo(fd, &info) != 0) {
		close(fd);
		strncpy(out, "ERR", 16);
		return (out);
	}
	close(fd);

	batt_life = info.ai_batt_life;
	if (batt_life == APM_UNKNOWN)
		strncpy(out, "unknown", 16);
	else if (batt_life <= 100) {
		snprintf(out, 16, "%d%%", batt_life);
		return (out);
	} else
		strncpy(out, "ERR", 16);

	return (out);
}

char
*get_apm_battery_time()
{
	int fd;
	int batt_time;
	int h, m, s;
	struct apm_info info;
	char *out;

	out = (char *)calloc(16, sizeof (char));

	fd = open(APMDEV, O_RDONLY);
	if (fd < 0) {
		strncpy(out, "ERR", 16);
		return (out);
	}

	if (apm_getinfo(fd, &info) != 0) {
		close(fd);
		strncpy(out, "ERR", 16);
		return (out);
	}
	close(fd);

	batt_time = info.ai_batt_time;

	if (batt_time == -1)
		strncpy(out, "unknown", 16);
	else {
		h = batt_time;
		s = h % 60;
		h /= 60;
		m = h % 60;
		h /= 60;
		snprintf(out, 16, "%2d:%02d:%02d", h, m, s);
	}

	return (out);
}

#endif

void update_entropy (void)
{
     /* mirrorbox: can you do anything equivalent in freebsd? -drphibes. */
}

/* empty stub so conky links */
void
free_all_processes(void)
{
}

#ifdef HAVE_LIBDEXTER
/* return 0 on success, -1 on failure */
int dexter_client_init (void)
{
    /* init libdexter for freebsd-specific client-side activity */
    return 0;
}

/* return 0 on success, -1 on failure */
int dexter_client_exit (void)
{
    /* de-init libdexter for freebsd-specific client-side activity */
    return 0;
}
#endif

