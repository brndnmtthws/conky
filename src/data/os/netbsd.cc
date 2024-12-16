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
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
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

#include "netbsd.h"
#include "net_stat.h"
#include "bsdcommon.h"

#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <time.h>
#include <unistd.h>

#include <sys/envsys.h>
#include <sys/param.h>
#include <sys/sched.h>
#include <sys/socket.h>
#include <sys/swap.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <uvm/uvm_param.h>
#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_types.h>

static int nkd_init = 0;
static u_int32_t sensvalue;
static char errbuf[_POSIX2_LINE_MAX];

void prepare_update() {}

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
    NORM_ERR("could not get uptime");
    info.uptime = 0;
  }

  return 1;
}

int check_mount(struct text_object *obj) {
  /* stub */
  (void)obj;
  return 0;
}

int update_meminfo() {
  bsdcommon::update_meminfo(info);
  return 1;
}

int update_net_stats() {
  int i;
  double delta;
  struct ifnet ifnet;
  struct ifnet_head ifhead; /* interfaces are in a tail queue */
  u_long ifnetaddr;
  static struct nlist namelist[] = {{"_ifnet"}, {nullptr}};
  static kvm_t *nkd;

  if (!nkd_init) {
    nkd = kvm_openfiles(nullptr, NULL, NULL, O_RDONLY, errbuf);
    if (nkd == nullptr) {
      NORM_ERR("cannot kvm_openfiles: %s", errbuf);
      NORM_ERR("maybe you need to setgid kmem this program?");
      return 1;
    } else if (kvm_nlist(nkd, namelist) != 0) {
      NORM_ERR("cannot kvm_nlist");
      return 1;
    } else {
      nkd_init = 1;
    }
  }

  if (kvm_read(nkd, (u_long)namelist[0].n_value, (void *)&ifhead,
               sizeof(ifhead)) < 0) {
    NORM_ERR("cannot kvm_read");
    return 1;
  }

  /* get delta */
  delta = current_update_time - last_update_time;
  if (delta <= 0.0001) { return 1; }

  // TODO(gmb)
  /*
  for (i = 0, ifnetaddr = (u_long)ifhead.tqh_first;
       ifnet.if_list.tqe_next && i < 16;
       ifnetaddr = (u_long)ifnet.if_list.tqe_next, i++) {
    struct net_stat *ns;
    long long last_recv, last_trans;

    kvm_read(nkd, (u_long)ifnetaddr, (void *)&ifnet, sizeof(ifnet));
    ns = get_net_stat(ifnet.if_xname, nullptr, NULL);
    ns->up = 1;
    last_recv = ns->recv;
    last_trans = ns->trans;

    if (ifnet.if_ibytes < ns->last_read_recv) {
      ns->recv +=
          ((long long)4294967295U - ns->last_read_recv) + ifnet.if_ibytes;
    } else {
      ns->recv += (ifnet.if_ibytes - ns->last_read_recv);
    }

    ns->last_read_recv = ifnet.if_ibytes;

    if (ifnet.if_obytes < ns->last_read_trans) {
      ns->trans +=
          ((long long)4294967295U - ns->last_read_trans) + ifnet.if_obytes;
    } else {
      ns->trans += (ifnet.if_obytes - ns->last_read_trans);
    }

    ns->last_read_trans = ifnet.if_obytes;

    ns->recv += (ifnet.if_ibytes - ns->last_read_recv);
    ns->last_read_recv = ifnet.if_ibytes;
    ns->trans += (ifnet.if_obytes - ns->last_read_trans);
    ns->last_read_trans = ifnet.if_obytes;

    ns->recv_speed = (ns->recv - last_recv) / delta;
    ns->trans_speed = (ns->trans - last_trans) / delta;
  }*/

  return 1;
}

int update_total_processes() {
  bsdcommon::get_processes(&info.procs);
  return 1;
}

int update_running_processes() {
  bsdcommon::get_number_of_running_processes(&info.run_procs);
  return 1;
}

void get_cpu_count(void) {
  bsdcommon::get_cpu_count(&info.cpu_usage, &info.cpu_count);
}

int update_cpu_usage() {
  bsdcommon::update_cpu_usage(&info.cpu_usage, &info.cpu_count);
  return 1;
}

void get_top_info(void) {
  bsdcommon::update_top_info();
}

void free_cpu(struct text_object *) { /* no-op */
}

int update_load_average() {
  double v[3];

  getloadavg(v, 3);

  info.loadavg[0] = (float)v[0];
  info.loadavg[1] = (float)v[1];
  info.loadavg[2] = (float)v[2];

  return 1;
}

double get_acpi_temperature(int fd) { return -1; }

void get_battery_stuff(char *buf, unsigned int n, const char *bat, int item) {}

int open_acpi_temperature(const char *name) { return -1; }

void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size,
                         const char *adapter) {
  (void)adapter;  // only linux uses this

  if (!p_client_buffer || client_buffer_size <= 0) { return; }

  /* not implemented */
  memset(p_client_buffer, 0, client_buffer_size);
}

/* char *get_acpi_fan() */
void get_acpi_fan(char *p_client_buffer, size_t client_buffer_size) {
  if (!p_client_buffer || client_buffer_size <= 0) { return; }

  /* not implemented */
  memset(p_client_buffer, 0, client_buffer_size);
}

int get_entropy_avail(unsigned int *val) { return 1; }
int get_entropy_poolsize(unsigned int *val) { return 1; }

char get_freq(char *p_client_buffer, size_t client_buffer_size,
              const char *p_format, int divisor, unsigned int cpu) {
  //TODO(gmb)
  return 1;
}

int get_battery_perct(const char *) {
  // TODO(gmb)
  return 0;
}

void get_battery_power_draw(char *buffer, unsigned int n, const char *bat) {
  // TODO(gmb)
}

void get_battery_short_status(char *buffer, unsigned int n, const char *bat) {
  // TODO(gmb)
}

double get_battery_perct_bar(struct text_object *obj) {
  int batperct = get_battery_perct(obj->data.s);
  return batperct;
}

int update_diskio(void) {
  // TODO(gmb)
  return 1;
}

bool is_conky_already_running() {
  return bsdcommon::is_conky_already_running();
}
