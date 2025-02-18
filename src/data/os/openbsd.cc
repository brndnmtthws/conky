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
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
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
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/proc.h>
#include <sys/sensors.h>
#include <sys/sched.h>
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

#include "../../conky.h"
#include "../hardware/diskio.h"
#include "../../logging.h"
#include "../network/net_stat.h"
#include "openbsd.h"
#include "../../content/temphelper.h"
#include "../top.h"

#define MAXSHOWDEVS 16

#define LOG1024 10
#define pagetok(size) ((size) << pageshift)

inline void proc_find_top(struct process **cpu, struct process **mem);

struct ifmibdata *data = nullptr;
size_t len = 0;

static int init_sensors = 0;

int check_mount(struct text_object *obj) {
  /* stub */
  (void)obj;
  return 0;
}

int update_uptime() {
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

  return 0;
}

int update_meminfo() {
  bsdcommon::update_meminfo(info);
  return 1;
}

int update_net_stats() {
  struct net_stat *ns;
  double delta;
  long long r, t, last_recv, last_trans;
  struct ifaddrs *ifap, *ifa;
  struct if_data *ifd;

  /* get delta */
  delta = current_update_time - last_update_time;
  if (delta <= 0.0001) { return 0; }

  if (getifaddrs(&ifap) < 0) { return 0; }

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

  return 0;
}

int update_total_processes() {
  bsdcommon::get_processes(&info.procs);
  return 1;
}

int update_running_processes() {
  bsdcommon::get_number_of_running_processes(&info.run_procs);
  return 1;
}

void get_cpu_count() {
  bsdcommon::get_cpu_count(&info.cpu_usage, &info.cpu_count);
}

int update_cpu_usage() {
  bsdcommon::update_cpu_usage(&info.cpu_usage, &info.cpu_count);
  return 1;
}

void free_cpu(struct text_object *) { /* no-op */
}

int update_load_average() {
  double v[3];

  getloadavg(v, 3);

  info.loadavg[0] = (float)v[0];
  info.loadavg[1] = (float)v[1];
  info.loadavg[2] = (float)v[2];

  return 0;
}

#define MAXSENSORDEVICES 128
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
int update_obsd_sensors() {
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
    return 0;
    // continue;
  }
  for (int t = 0; t < SENSOR_MAX_TYPES; t++) {
    type = (enum sensor_type) t;
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

  return 0;
}

void parse_obsd_sensor(struct text_object *obj, const char *arg) {
  if (!isdigit((unsigned char)arg[0]) || atoi(&arg[0]) < 0 ||
      atoi(&arg[0]) > OBSD_MAX_SENSORS - 1) {
    obj->data.l = 0;
    NORM_ERR("Invalid sensor number!");
  } else
    obj->data.l = atoi(&arg[0]);
}

void print_obsd_sensors_temp(struct text_object *obj, char *p,
                             unsigned int p_max_size) {
  obsd_sensors.device = sensor_device.get(*state);
  update_obsd_sensors();
  temp_print(p, p_max_size, obsd_sensors.temp[obsd_sensors.device][obj->data.l],
             TEMP_CELSIUS, 1);
}

void print_obsd_sensors_fan(struct text_object *obj, char *p,
                            unsigned int p_max_size) {
  obsd_sensors.device = sensor_device.get(*state);
  update_obsd_sensors();
  snprintf(p, p_max_size, "%d",
           obsd_sensors.fan[obsd_sensors.device][obj->data.l]);
}

void print_obsd_sensors_volt(struct text_object *obj, char *p,
                             unsigned int p_max_size) {
  obsd_sensors.device = sensor_device.get(*state);
  update_obsd_sensors();
  snprintf(p, p_max_size, "%.2f",
           obsd_sensors.volt[obsd_sensors.device][obj->data.l]);
}

/* chipset vendor */
void get_obsd_vendor(struct text_object *obj, char *buf,
                     unsigned int client_buffer_size) {
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
                      unsigned int client_buffer_size) {
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

int update_diskio() { return 0; /* XXX: implement? hifi: not sure how */ }

void get_top_info(void) {
  bsdcommon::update_top_info();
}

void get_battery_short_status(char *buffer, unsigned int n, const char *bat) {
  /* Not implemented */
  (void)bat;
  if (buffer && n > 0) memset(buffer, 0, n);
}

/* empty stubs so conky links */
void prepare_update() {}

int get_entropy_avail(unsigned int *val) { return 1; }

int get_entropy_poolsize(unsigned int *val) { return 1; }
