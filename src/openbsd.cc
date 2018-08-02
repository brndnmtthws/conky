/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2007 Toni Spets
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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

#include <kvm.h>
#include <sys/dkstat.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/sensors.h>
#include <sys/socket.h>
#include <sys/swap.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/vmmeter.h>

#include <net/if.h>
#include <net/if_media.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <limits.h>
#include <machine/apmvar.h>
#include <unistd.h>

#include <net80211/ieee80211.h>
#include <net80211/ieee80211_ioctl.h>

#include "conky.h"
#include "diskio.h"
#include "logging.h"
#include "net_stat.h"
#include "openbsd.h"
#include "top.h"

#define MAXSHOWDEVS 16

#define LOG1024 10
#define pagetok(size) ((size) << pageshift)

inline void proc_find_top(struct process **cpu, struct process **mem);

static short cpu_setup = 0;
static kvm_t *kd = 0;

struct ifmibdata *data = nullptr;
size_t len = 0;

int init_kvm = 0;
int init_sensors = 0;

static int kvm_init() {
  if (init_kvm) { return 1; }

  kd = kvm_open(nullptr, NULL, NULL, KVM_NO_FILES, NULL);
  if (kd == nullptr) {
    NORM_ERR("error opening kvm");
  } else {
    init_kvm = 1;
  }

  return 1;
}

/* note: swapmode taken from 'top' source */
/* swapmode is rewritten by Tobias Weingartner <weingart@openbsd.org>
 * to be based on the new swapctl(2) system call. */
static int swapmode(int *used, int *total) {
  struct swapent *swdev;
  int nswap, rnswap, i;

  nswap = swapctl(SWAP_NSWAP, 0, 0);
  if (nswap == 0) { return 0; }

  swdev = malloc(nswap * sizeof(*swdev));
  if (swdev == nullptr) { return 0; }

  rnswap = swapctl(SWAP_STATS, swdev, nswap);
  if (rnswap == -1) {
    free(swdev);
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

int check_mount(struct text_object *obj) {
  /* stub */
  (void)obj;
  return 0;
}

void update_uptime() {
  int mib[2] = {CTL_KERN, KERN_BOOTTIME};
  struct timeval boottime;
  time_t now;
  size_t size = sizeof(boottime);

  if ((sysctl(mib, 2, &boottime, &size, nullptr, 0) != -1) &&
      (boottime.tv_sec != 0)) {
    time(&now);
    info.uptime = now - boottime.tv_sec;
  } else {
    NORM_ERR("Could not get uptime");
    info.uptime = 0;
  }
}

void update_meminfo() {
  static int mib[2] = {CTL_VM, VM_METER};
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
  if (sysctl(mib, 2, &vmtotal, &size, nullptr, 0) < 0) {
    warn("sysctl failed");
    bzero(&vmtotal, sizeof(vmtotal));
  }

  info.memmax = pagetok(vmtotal.t_rm) + pagetok(vmtotal.t_free);
  info.mem = info.memwithbuffers = pagetok(vmtotal.t_rm);
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

void update_net_stats() {
  struct net_stat *ns;
  double delta;
  long long r, t, last_recv, last_trans;
  struct ifaddrs *ifap, *ifa;
  struct if_data *ifd;

  /* get delta */
  delta = current_update_time - last_update_time;
  if (delta <= 0.0001) { return; }

  if (getifaddrs(&ifap) < 0) { return; }

  for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
    ns = get_net_stat((const char *)ifa->ifa_name, nullptr, NULL);

    if (ifa->ifa_flags & IFF_UP) {
      struct ifaddrs *iftmp;

      ns->up = 1;
      last_recv = ns->recv;
      last_trans = ns->trans;

      if (ifa->ifa_addr->sa_family != AF_LINK) { continue; }

      for (iftmp = ifa->ifa_next;
           iftmp != nullptr && strcmp(ifa->ifa_name, iftmp->ifa_name) == 0;
           iftmp = iftmp->ifa_next) {
        if (iftmp->ifa_addr->sa_family == AF_INET) {
          memcpy(&(ns->addr), iftmp->ifa_addr, iftmp->ifa_addr->sa_len);
        }
      }

      ifd = (struct if_data *)ifa->ifa_data;
      r = ifd->ifi_ibytes;
      t = ifd->ifi_obytes;

      if (r < ns->last_read_recv) {
        ns->recv += ((long long)4294967295U - ns->last_read_recv) + r;
      } else {
        ns->recv += (r - ns->last_read_recv);
      }

      ns->last_read_recv = r;

      if (t < ns->last_read_trans) {
        ns->trans += (long long)4294967295U - ns->last_read_trans + t;
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

int update_total_processes() {
  int n_processes;

  kvm_init();
  kvm_getprocs(kd, KERN_PROC_ALL, 0, &n_processes);

  info.procs = n_processes;
}

void update_running_processes() {
  struct kinfo_proc2 *p;
  int n_processes;
  int i, cnt = 0;

  kvm_init();
  int max_size = sizeof(struct kinfo_proc2);

  p = kvm_getproc2(kd, KERN_PROC_ALL, 0, max_size, &n_processes);
  for (i = 0; i < n_processes; i++) {
    if (p[i].p_stat == SRUN) { cnt++; }
  }

  info.run_procs = cnt;
}

/* new SMP code can be enabled by commenting the following line */
#define OLDCPU

#ifdef OLDCPU
struct cpu_load_struct {
  unsigned long load[5];
};

struct cpu_load_struct fresh = {{0, 0, 0, 0, 0}};
long cpu_used, oldtotal, oldused;
#else
#include <assert.h>
int64_t *fresh = nullptr;

/* XXX is 8 enough? - What's the constant for MAXCPU? */
/* allocate this with malloc would be better */
int64_t oldtotal[8], oldused[8];
#endif

void get_cpu_count() {
  int cpu_count = 1; /* default to 1 cpu */
#ifndef OLDCPU
  int mib[2] = {CTL_HW, HW_NCPU};
  size_t len = sizeof(cpu_count);

  if (sysctl(mib, 2, &cpu_count, &len, nullptr, 0) != 0) {
    NORM_ERR("error getting cpu count, defaulting to 1");
  }
#endif
  info.cpu_count = cpu_count;

  info.cpu_usage = malloc(info.cpu_count * sizeof(float));
  if (info.cpu_usage == nullptr) { CRIT_ERR(nullptr, NULL, "malloc"); }

#ifndef OLDCPU
  assert(fresh == nullptr); /* XXX Is this leaking memory? */
  /* XXX Where shall I free this? */
  if (nullptr == (fresh = calloc(cpu_count, sizeof(int64_t) * CPUSTATES))) {
    CRIT_ERR(nullptr, NULL, "calloc");
  }
#endif
}

void update_cpu_usage() {
#ifdef OLDCPU
  int mib[2] = {CTL_KERN, KERN_CPTIME};
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
  if (sysctl(mib, 2, &cp_time, &len, nullptr, 0) < 0) {
    NORM_ERR("Cannot get kern.cp_time");
  }

  fresh.load[0] = cp_time[CP_USER];
  fresh.load[1] = cp_time[CP_NICE];
  fresh.load[2] = cp_time[CP_SYS];
  fresh.load[3] = cp_time[CP_IDLE];
  fresh.load[4] = cp_time[CP_IDLE];

  used = fresh.load[0] + fresh.load[1] + fresh.load[2];
  total = fresh.load[0] + fresh.load[1] + fresh.load[2] + fresh.load[3];

  if ((total - oldtotal) != 0) {
    info.cpu_usage[0] = ((double)(used - oldused)) / (double)(total - oldtotal);
  } else {
    info.cpu_usage[0] = 0;
  }

  oldused = used;
  oldtotal = total;
#else
  if (info.cpu_count > 1) {
    size = CPUSTATES * sizeof(int64_t);
    for (i = 0; i < info.cpu_count; i++) {
      int cp_time_mib[] = {CTL_KERN, KERN_CPTIME2, i};
      if (sysctl(cp_time_mib, 3, &(fresh[i * CPUSTATES]), &size, nullptr, 0) <
          0) {
        NORM_ERR("sysctl kern.cp_time2 failed");
      }
    }
  } else {
    int cp_time_mib[] = {CTL_KERN, KERN_CPTIME};
    long cp_time_tmp[CPUSTATES];

    size = sizeof(cp_time_tmp);
    if (sysctl(cp_time_mib, 2, cp_time_tmp, &size, nullptr, 0) < 0) {
      NORM_ERR("sysctl kern.cp_time failed");
    }

    for (i = 0; i < CPUSTATES; i++) { fresh[i] = (int64_t)cp_time_tmp[i]; }
  }

  /* XXX Do sg with this int64_t => long => double ? float hell. */
  for (i = 0; i < info.cpu_count; i++) {
    int64_t used, total;
    int at = i * CPUSTATES;

    used = fresh[at + CP_USER] + fresh[at + CP_NICE] + fresh[at + CP_SYS];
    total = used + fresh[at + CP_IDLE];

    if ((total - oldtotal[i]) != 0) {
      info.cpu_usage[i] =
          ((double)(used - oldused[i])) / (double)(total - oldtotal[i]);
    } else {
      info.cpu_usage[i] = 0;
    }

    oldused[i] = used;
    oldtotal[i] = total;
  }
#endif
}

void update_load_average() {
  double v[3];

  getloadavg(v, 3);

  info.loadavg[0] = (float)v[0];
  info.loadavg[1] = (float)v[1];
  info.loadavg[2] = (float)v[2];
}

#define OBSD_MAX_SENSORS 256
static struct obsd_sensors_struct {
  int device;
  float temp[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
  unsigned int fan[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
  float volt[MAXSENSORDEVICES][OBSD_MAX_SENSORS];
} obsd_sensors;

static conky::simple_config_setting<int> sensor_device("sensor_device", 0,
                                                       false);

/* read sensors from sysctl */
void update_obsd_sensors() {
  int sensor_cnt, dev, numt, mib[5] = {CTL_HW, HW_SENSORS, 0, 0, 0};
  struct sensor sensor;
  struct sensordev sensordev;
  size_t slen, sdlen;
  enum sensor_type type;

  slen = sizeof(sensor);
  sdlen = sizeof(sensordev);

  sensor_cnt = 0;

  dev = obsd_sensors.device;  // FIXME: read more than one device

  /* for (dev = 0; dev < MAXSENSORDEVICES; dev++) { */
  mib[2] = dev;
  if (sysctl(mib, 3, &sensordev, &sdlen, nullptr, 0) == -1) {
    if (errno != ENOENT) { warn("sysctl"); }
    return;
    // continue;
  }
  for (type = 0; type < SENSOR_MAX_TYPES; type++) {
    mib[3] = type;
    for (numt = 0; numt < sensordev.maxnumt[type]; numt++) {
      mib[4] = numt;
      if (sysctl(mib, 5, &sensor, &slen, nullptr, 0) == -1) {
        if (errno != ENOENT) { warn("sysctl"); }
        continue;
      }
      if (sensor.flags & SENSOR_FINVALID) { continue; }

      switch (type) {
        case SENSOR_TEMP:
          obsd_sensors.temp[dev][sensor.numt] =
              (sensor.value - 273150000) / 1000000.0;
          break;
        case SENSOR_FANRPM:
          obsd_sensors.fan[dev][sensor.numt] = sensor.value;
          break;
        case SENSOR_VOLTS_DC:
          obsd_sensors.volt[dev][sensor.numt] = sensor.value / 1000000.0;
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

void parse_obsd_sensor(struct text_object *obj, const char *arg) {
  if (!isdigit((unsigned char)arg[0]) || atoi(&arg[0]) < 0 ||
      atoi(&arg[0]) > OBSD_MAX_SENSORS - 1) {
    obj->data.l = 0;
    NORM_ERR("Invalid sensor number!");
  } else
    obj->data.l = atoi(&arg[0]);
}

void print_obsd_sensors_temp(struct text_object *obj, char *p, int p_max_size) {
  obsd_sensors.device = sensor_device.get(*state);
  update_obsd_sensors();
  temp_print(p, p_max_size, obsd_sensors.temp[obsd_sensors.device][obj->data.l],
             TEMP_CELSIUS);
}

void print_obsd_sensors_fan(struct text_object *obj, char *p, int p_max_size) {
  obsd_sensors.device = sensor_device.get(*state);
  update_obsd_sensors();
  snprintf(p, p_max_size, "%d",
           obsd_sensors.fan[obsd_sensors.device][obj->data.l]);
}

void print_obsd_sensors_volt(struct text_object *obj, char *p, int p_max_size) {
  obsd_sensors.device = sensor_device.get(*state);
  update_obsd_sensors();
  snprintf(p, p_max_size, "%.2f",
           obsd_sensors.volt[obsd_sensors.device][obj->data.l]);
}

/* chipset vendor */
void get_obsd_vendor(struct text_object *obj, char *buf,
                     size_t client_buffer_size) {
  int mib[2];
  char vendor[64];
  size_t size = sizeof(vendor);

  (void)obj;

  mib[0] = CTL_HW;
  mib[1] = HW_VENDOR;

  if (sysctl(mib, 2, vendor, &size, nullptr, 0) == -1) {
    NORM_ERR("error reading vendor");
    snprintf(buf, client_buffer_size, "%s", "unknown");
  } else {
    snprintf(buf, client_buffer_size, "%s", vendor);
  }
}

/* chipset name */
void get_obsd_product(struct text_object *obj, char *buf,
                      size_t client_buffer_size) {
  int mib[2];
  char product[64];
  size_t size = sizeof(product);

  (void)obj;

  mib[0] = CTL_HW;
  mib[1] = HW_PRODUCT;

  if (sysctl(mib, 2, product, &size, nullptr, 0) == -1) {
    NORM_ERR("error reading product");
    snprintf(buf, client_buffer_size, "%s", "unknown");
  } else {
    snprintf(buf, client_buffer_size, "%s", product);
  }
}

/* void */
char get_freq(char *p_client_buffer, size_t client_buffer_size,
              const char *p_format, int divisor, unsigned int cpu) {
  int freq = cpu;
  int mib[2] = {CTL_HW, HW_CPUSPEED};

  if (!p_client_buffer || client_buffer_size <= 0 || !p_format ||
      divisor <= 0) {
    return 0;
  }

  size_t size = sizeof(freq);

  if (sysctl(mib, 2, &freq, &size, nullptr, 0) == 0) {
    snprintf(p_client_buffer, client_buffer_size, p_format,
             (float)freq / divisor);
  } else {
    snprintf(p_client_buffer, client_buffer_size, p_format, 0.0f);
  }

  return 1;
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

void clear_diskio_stats() {}

struct diskio_stat *prepare_diskio_stat(const char *s) {}

void update_diskio() { return; /* XXX: implement? hifi: not sure how */ }

/* While topless is obviously better, top is also not bad. */

void get_top_info(void) {
  struct kinfo_proc2 *p;
  struct process *proc;
  int n_processes;
  int i;

  kvm_init();

  p = kvm_getproc2(kd, KERN_PROC_ALL, 0, sizeof(struct kinfo_proc2),
                   &n_processes);

  for (i = 0; i < n_processes; i++) {
    if (!((p[i].p_flag & P_SYSTEM)) && p[i].p_comm != nullptr) {
      proc = find_process(p[i].p_pid);
      if (!proc) proc = new_process(p[i].p_pid);

      proc->time_stamp = g_time;
      proc->name = strndup(p[i].p_comm, text_buffer_size);
      proc->amount = 100.0 * p[i].p_pctcpu / FSCALE;
      /* TODO: vsize, rss, total_cpu_time */
    }
  }
}

/* empty stubs so conky links */
void prepare_update() {}

int get_entropy_avail(unsigned int *val) { return 1; }

int get_entropy_poolsize(unsigned int *val) { return 1; }
