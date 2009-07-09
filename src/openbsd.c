/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2007 Toni Spets
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
#include <sys/sensors.h>
#include <sys/malloc.h>
#include <sys/swap.h>
#include <kvm.h>

#include <net/if.h>
#include <net/if_media.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <limits.h>
#include <unistd.h>
#include <machine/apmvar.h>

#include <net80211/ieee80211.h>
#include <net80211/ieee80211_ioctl.h>

#include "conky.h"
#include "diskio.h"
#include "logging.h"
#include "openbsd.h"
#include "top.h"

#define	MAXSHOWDEVS		16

#define LOG1024			10
#define pagetok(size) ((size) << pageshift)

inline void proc_find_top(struct process **cpu, struct process **mem);

static short cpu_setup = 0;
static kvm_t *kd = 0;

struct ifmibdata *data = NULL;
size_t len = 0;

int init_kvm = 0;
int init_sensors = 0;

static int kvm_init()
{
	if (init_kvm) {
		return 1;
	}

	kd = kvm_open(NULL, NULL, NULL, KVM_NO_FILES, NULL);
	if (kd == NULL) {
		ERR("error opening kvm");
	} else {
		init_kvm = 1;
	}

	return 1;
}

/* note: swapmode taken from 'top' source */
/* swapmode is rewritten by Tobias Weingartner <weingart@openbsd.org>
 * to be based on the new swapctl(2) system call. */
static int swapmode(int *used, int *total)
{
	struct swapent *swdev;
	int nswap, rnswap, i;

	nswap = swapctl(SWAP_NSWAP, 0, 0);
	if (nswap == 0) {
		return 0;
	}

	swdev = malloc(nswap * sizeof(*swdev));
	if (swdev == NULL) {
		return 0;
	}

	rnswap = swapctl(SWAP_STATS, swdev, nswap);
	if (rnswap == -1) {
		return 0;
	}

	/* if rnswap != nswap, then what? */

	/* Total things up */
	*total = *used = 0;
	for (i = 0; i < nswap; i++) {
		if (swdev[i].se_flags & SWF_ENABLE) {
			*used += (swdev[i].se_inuse / (1024 / DEV_BSIZE));
			*total += (swdev[i].se_nblks / (1024 / DEV_BSIZE));
		}
	}
	free(swdev);
	return 1;
}

int check_mount(char *s)
{
	/* stub */
	return 0;
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
		ERR("Could not get uptime");
		info.uptime = 0;
	}
}

void update_meminfo()
{
	static int mib[2] = { CTL_VM, VM_METER };
	struct vmtotal vmtotal;
	size_t size;
	int pagesize, pageshift, swap_avail, swap_used;

	pagesize = getpagesize();
	pageshift = 0;
	while (pagesize > 1) {
		pageshift++;
		pagesize >>= 1;
	}

	/* we only need the amount of log(2)1024 for our conversion */
	pageshift -= LOG1024;

	/* get total -- systemwide main memory usage structure */
	size = sizeof(vmtotal);
	if (sysctl(mib, 2, &vmtotal, &size, NULL, 0) < 0) {
		warn("sysctl failed");
		bzero(&vmtotal, sizeof(vmtotal));
	}

	info.memmax = pagetok(vmtotal.t_rm) + pagetok(vmtotal.t_free);
	info.mem = pagetok(vmtotal.t_rm);
	info.memeasyfree = info.memfree = info.memmax - info.mem;

	if ((swapmode(&swap_used, &swap_avail)) >= 0) {
		info.swapmax = swap_avail;
		info.swap = swap_used;
		info.swapfree = swap_avail - swap_used;
	} else {
		info.swapmax = 0;
		info.swap = 0;
		info.swapfree = 0;
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
	if (delta <= 0.0001) {
		return;
	}

	if (getifaddrs(&ifap) < 0) {
		return;
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		ns = get_net_stat((const char *) ifa->ifa_name);

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
}

void update_total_processes()
{
	int n_processes;

	kvm_init();
	kvm_getprocs(kd, KERN_PROC_ALL, 0, &n_processes);

	info.procs = n_processes;
}

void update_running_processes()
{
	struct kinfo_proc2 *p;
	int n_processes;
	int i, cnt = 0;

	kvm_init();
	int max_size = sizeof(struct kinfo_proc2);

	p = kvm_getproc2(kd, KERN_PROC_ALL, 0, max_size, &n_processes);
	for (i = 0; i < n_processes; i++) {
		if (p[i].p_stat == SRUN) {
			cnt++;
		}
	}

	info.run_procs = cnt;
}

/* new SMP code can be enabled by commenting the following line */
#define OLDCPU

#ifdef OLDCPU
struct cpu_load_struct {
	unsigned long load[5];
};

struct cpu_load_struct fresh = { {0, 0, 0, 0, 0} };
long cpu_used, oldtotal, oldused;
#else
#include <assert.h>
int64_t *fresh = NULL;

/* XXX is 8 enough? - What's the constant for MAXCPU? */
/* allocate this with malloc would be better */
int64_t oldtotal[8], oldused[8];
#endif

void get_cpu_count()
{
	int cpu_count = 1;	/* default to 1 cpu */
#ifndef OLDCPU
	int mib[2] = { CTL_HW, HW_NCPU };
	size_t len = sizeof(cpu_count);

	if (sysctl(mib, 2, &cpu_count, &len, NULL, 0) != 0) {
		ERR("error getting cpu count, defaulting to 1");
	}
#endif
	info.cpu_count = cpu_count;

	info.cpu_usage = malloc(info.cpu_count * sizeof(float));
	if (info.cpu_usage == NULL) {
		CRIT_ERR("malloc");
	}

#ifndef OLDCPU
	assert(fresh == NULL);	/* XXX Is this leaking memory? */
	/* XXX Where shall I free this? */
	if (NULL == (fresh = calloc(cpu_count, sizeof(int64_t) * CPUSTATES))) {
		CRIT_ERR("calloc");
	}
#endif
}

void update_cpu_usage()
{
#ifdef OLDCPU
	int mib[2] = { CTL_KERN, KERN_CPTIME };
	long used, total;
	long cp_time[CPUSTATES];
	size_t len = sizeof(cp_time);
#else
	size_t size;
	unsigned int i;
#endif

	/* add check for !info.cpu_usage since that mem is freed on a SIGUSR1 */
	if ((cpu_setup == 0) || (!info.cpu_usage)) {
		get_cpu_count();
		cpu_setup = 1;
	}

#ifdef OLDCPU
	if (sysctl(mib, 2, &cp_time, &len, NULL, 0) < 0) {
		ERR("Cannot get kern.cp_time");
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
#else
	if (info.cpu_count > 1) {
		size = CPUSTATES * sizeof(int64_t);
		for (i = 0; i < info.cpu_count; i++) {
			int cp_time_mib[] = { CTL_KERN, KERN_CPTIME2, i };
			if (sysctl(cp_time_mib, 3, &(fresh[i * CPUSTATES]), &size, NULL, 0)
					< 0) {
				ERR("sysctl kern.cp_time2 failed");
			}
		}
	} else {
		int cp_time_mib[] = { CTL_KERN, KERN_CPTIME };
		long cp_time_tmp[CPUSTATES];

		size = sizeof(cp_time_tmp);
		if (sysctl(cp_time_mib, 2, cp_time_tmp, &size, NULL, 0) < 0) {
			ERR("sysctl kern.cp_time failed");
		}

		for (i = 0; i < CPUSTATES; i++) {
			fresh[i] = (int64_t) cp_time_tmp[i];
		}
	}

	/* XXX Do sg with this int64_t => long => double ? float hell. */
	for (i = 0; i < info.cpu_count; i++) {
		int64_t used, total;
		int at = i * CPUSTATES;

		used = fresh[at + CP_USER] + fresh[at + CP_NICE] + fresh[at + CP_SYS];
		total = used + fresh[at + CP_IDLE];

		if ((total - oldtotal[i]) != 0) {
			info.cpu_usage[i] = ((double) (used - oldused[i])) /
				(double) (total - oldtotal[i]);
		} else {
			info.cpu_usage[i] = 0;
		}

		oldused[i] = used;
		oldtotal[i] = total;
	}
#endif
}

void update_load_average()
{
	double v[3];

	getloadavg(v, 3);

	info.loadavg[0] = (float) v[0];
	info.loadavg[1] = (float) v[1];
	info.loadavg[2] = (float) v[2];
}

/* read sensors from sysctl */
void update_obsd_sensors()
{
	int sensor_cnt, dev, numt, mib[5] = { CTL_HW, HW_SENSORS, 0, 0, 0 };
	struct sensor sensor;
	struct sensordev sensordev;
	size_t slen, sdlen;
	enum sensor_type type;

	slen = sizeof(sensor);
	sdlen = sizeof(sensordev);

	sensor_cnt = 0;

	dev = obsd_sensors.device;	// FIXME: read more than one device

	/* for (dev = 0; dev < MAXSENSORDEVICES; dev++) { */
		mib[2] = dev;
		if (sysctl(mib, 3, &sensordev, &sdlen, NULL, 0) == -1) {
			if (errno != ENOENT) {
				warn("sysctl");
			}
			return;
			// continue;
		}
		for (type = 0; type < SENSOR_MAX_TYPES; type++) {
			mib[3] = type;
			for (numt = 0; numt < sensordev.maxnumt[type]; numt++) {
				mib[4] = numt;
				if (sysctl(mib, 5, &sensor, &slen, NULL, 0) == -1) {
					if (errno != ENOENT) {
						warn("sysctl");
					}
					continue;
				}
				if (sensor.flags & SENSOR_FINVALID) {
					continue;
				}

				switch (type) {
					case SENSOR_TEMP:
						obsd_sensors.temp[dev][sensor.numt] =
							(sensor.value - 273150000) / 1000000.0;
						break;
					case SENSOR_FANRPM:
						obsd_sensors.fan[dev][sensor.numt] = sensor.value;
						break;
					case SENSOR_VOLTS_DC:
						obsd_sensors.volt[dev][sensor.numt] =
							sensor.value / 1000000.0;
						break;
					default:
						break;
				}

				sensor_cnt++;
			}
		}
	/* } */

	init_sensors = 1;
}

/* chipset vendor */
void get_obsd_vendor(char *buf, size_t client_buffer_size)
{
	int mib[2];

	mib[0] = CTL_HW;
	mib[1] = HW_VENDOR;
	char vendor[64];
	size_t size = sizeof(vendor);

	if (sysctl(mib, 2, vendor, &size, NULL, 0) == -1) {
		ERR("error reading vendor");
		snprintf(buf, client_buffer_size, "unknown");
	} else {
		snprintf(buf, client_buffer_size, "%s", vendor);
	}
}

/* chipset name */
void get_obsd_product(char *buf, size_t client_buffer_size)
{
	int mib[2];

	mib[0] = CTL_HW;
	mib[1] = HW_PRODUCT;
	char product[64];
	size_t size = sizeof(product);

	if (sysctl(mib, 2, product, &size, NULL, 0) == -1) {
		ERR("error reading product");
		snprintf(buf, client_buffer_size, "unknown");
	} else {
		snprintf(buf, client_buffer_size, "%s", product);
	}
}

/* rdtsc() and get_freq_dynamic() copied from linux.c */

#if  defined(__i386) || defined(__x86_64)
__inline__ unsigned long long int rdtsc()
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
char get_freq(char *p_client_buffer, size_t client_buffer_size,
		const char *p_format, int divisor, unsigned int cpu)
{
	int freq = cpu;
	int mib[2] = { CTL_HW, HW_CPUSPEED };

	if (!p_client_buffer || client_buffer_size <= 0 || !p_format
			|| divisor <= 0) {
		return 0;
	}

	size_t size = sizeof(freq);

	if (sysctl(mib, 2, &freq, &size, NULL, 0) == 0) {
		snprintf(p_client_buffer, client_buffer_size, p_format,
			(float) freq / divisor);
	} else {
		snprintf(p_client_buffer, client_buffer_size, p_format, 0.0f);
	}

	return 1;
}

void update_top()
{
	kvm_init();
	proc_find_top(info.cpu, info.memu);
}

#if 0
/* deprecated, will rewrite this soon in update_net_stats() -hifi */
void update_wifi_stats()
{
	struct net_stat *ns;
	struct ifaddrs *ifap, *ifa;
	struct ifmediareq ifmr;
	struct ieee80211_nodereq nr;
	struct ieee80211_bssid bssid;
	int s, ibssid;

	/* Get iface table */
	if (getifaddrs(&ifap) < 0) {
		return;
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		ns = get_net_stat((const char *) ifa->ifa_name);

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

			memset(&bssid, 0, sizeof(bssid));
			strlcpy(bssid.i_name, ifa->ifa_name, sizeof(bssid.i_name));
			ibssid = ioctl(s, SIOCG80211BSSID, &bssid);

			bzero(&nr, sizeof(nr));
			bcopy(bssid.i_bssid, &nr.nr_macaddr, sizeof(nr.nr_macaddr));
			strlcpy(nr.nr_ifname, ifa->ifa_name, sizeof(nr.nr_ifname));

			if (ioctl(s, SIOCG80211NODE, &nr) == 0 && nr.nr_rssi) {
				ns->linkstatus = nr.nr_rssi;
			}
		}
cleanup:
		close(s);
	}
}
#endif

void clear_diskio_stats()
{
}

struct diskio_stat *prepare_diskio_stat(const char *s)
{
}

void update_diskio()
{
	return;	/* XXX: implement? hifi: not sure how */
}

/* While topless is obviously better, top is also not bad. */

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
	if (((struct process *) a)->totalmem > ((struct process *) b)->totalmem) {
		return -1;
	}

	if (((struct process *) a)->totalmem < ((struct process *) b)->totalmem) {
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
		ERR("error reading usermem");
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
			processes[j].totalmem = (float) (p[i].p_vm_rssize /
					(float) total_pages) * 100.0;
			j++;
		}
	}

	qsort(processes, j - 1, sizeof(struct process), comparemem);
	for (i = 0; i < 10; i++) {
		struct process *tmp, *ttmp;

		tmp = malloc(sizeof(struct process));
		tmp->pid = processes[i].pid;
		tmp->amount = processes[i].amount;
		tmp->totalmem = processes[i].totalmem;
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
		tmp->pid = processes[i].pid;
		tmp->amount = processes[i].amount;
		tmp->totalmem = processes[i].totalmem;
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

#if	defined(i386) || defined(__i386__)
#define	APMDEV		"/dev/apm"
#define	APM_UNKNOWN	255

int apm_getinfo(int fd, apm_info_t aip)
{
	if (ioctl(fd, APM_IOC_GETPOWER, aip) == -1) {
		return -1;
	}

	return 0;
}

char *get_apm_adapter()
{
	int fd;
	struct apm_power_info info;
	char *out;

	out = (char *) calloc(16, sizeof(char));

	fd = open(APMDEV, O_RDONLY);
	if (fd < 0) {
		strncpy(out, "ERR", 16);
		return out;
	}

	if (apm_getinfo(fd, &info) != 0) {
		close(fd);
		strncpy(out, "ERR", 16);
		return out;
	}
	close(fd);

	switch (info.ac_state) {
		case APM_AC_OFF:
			strncpy(out, "off-line", 16);
			return out;
			break;
		case APM_AC_ON:
			if (info.battery_state == APM_BATT_CHARGING) {
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

char *get_apm_battery_life()
{
	int fd;
	u_int batt_life;
	struct apm_power_info info;
	char *out;

	out = (char *) calloc(16, sizeof(char));

	fd = open(APMDEV, O_RDONLY);
	if (fd < 0) {
		strncpy(out, "ERR", 16);
		return out;
	}

	if (apm_getinfo(fd, &info) != 0) {
		close(fd);
		strncpy(out, "ERR", 16);
		return out;
	}
	close(fd);

	batt_life = info.battery_life;
	if (batt_life <= 100) {
		snprintf(out, 16, "%d%%", batt_life);
		return out;
	} else {
		strncpy(out, "ERR", 16);
	}

	return out;
}

char *get_apm_battery_time()
{
	int fd;
	int batt_time;
	int h, m;
	struct apm_power_info info;
	char *out;

	out = (char *) calloc(16, sizeof(char));

	fd = open(APMDEV, O_RDONLY);
	if (fd < 0) {
		strncpy(out, "ERR", 16);
		return out;
	}

	if (apm_getinfo(fd, &info) != 0) {
		close(fd);
		strncpy(out, "ERR", 16);
		return out;
	}
	close(fd);

	batt_time = info.minutes_left;

	if (batt_time == -1) {
		strncpy(out, "unknown", 16);
	} else {
		h = batt_time / 60;
		m = batt_time % 60;
		snprintf(out, 16, "%2d:%02d", h, m);
	}

	return out;
}

#endif

/* empty stubs so conky links */
void prepare_update()
{
}

void update_entropy(void)
{
}

void free_all_processes(void)
{
}
