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

#include <sys/ioctl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <string.h>
#include <ctype.h>
#include "conky.h"
#include "logging.h"
#include "net/if.h"
#include "net_stat.h"
#include "specials.h"
#include "text_object.h"
#if defined(__sun)
#include <sys/sockio.h>
#endif
#if defined(__HAIKU__)
#include <sys/sockio.h>
#define IFF_RUNNING IFF_LINK
#endif
#ifndef SOCK_CLOEXEC
#define SOCK_CLOEXEC O_CLOEXEC
#endif /* SOCK_CLOEXEC */

#if defined(__linux__)
#include "linux.h"
#else
static char e_iface[50] = "empty";
static char interfaces_arr[64][64] = {""};
#endif /* __linux__ */

/* network interface stuff */

enum if_up_strictness_ { IFUP_UP, IFUP_LINK, IFUP_ADDR };

template <>
conky::lua_traits<if_up_strictness_>::Map
    conky::lua_traits<if_up_strictness_>::map = {
        {"up", IFUP_UP}, {"link", IFUP_LINK}, {"address", IFUP_ADDR}};

static conky::simple_config_setting<if_up_strictness_> if_up_strictness(
    "if_up_strictness", IFUP_UP, true);
/**
 * global array of structs containing network statistics for each interface
 **/
struct net_stat netstats[MAX_NET_INTERFACES];
struct net_stat foo_netstats;

/**
 * Returns pointer to specified interface in netstats array.
 * If not found then add the specified interface to the array.
 * The added interface will have all its members initialized to 0,
 * because clear_net_stats() is called from main() in conky.cc!
 *
 * @param[in] dev  device / interface name. Silently ignores char * == nullptr
 **/
struct net_stat *get_net_stat(const char *dev, void * /*free_at_crash1*/,
                              void * /*free_at_crash2*/) {
  unsigned int i;

  if (dev == nullptr) { return nullptr; }

  /* find interface stat */
  for (i = 0; i < MAX_NET_INTERFACES; i++) {
    if ((netstats[i].dev != nullptr) && strcmp(netstats[i].dev, dev) == 0) {
      return &netstats[i];
    }
  }

  /* wasn't found? add it */
  for (i = 0; i < MAX_NET_INTERFACES; i++) {
    if (netstats[i].dev == nullptr) {
      netstats[i].dev = strndup(dev, text_buffer_size.get(*state));
      /* initialize last_read_recv and last_read_trans to -1 denoting
       * that they were never read before */
      netstats[i].last_read_recv = -1;
      netstats[i].last_read_trans = -1;
      return &netstats[i];
    }
  }

  clear_net_stats(&foo_netstats);
  foo_netstats.dev = strndup(dev, text_buffer_size.get(*state));
  /* initialize last_read_recv and last_read_trans to -1 denoting
   * that they were never read before */
  foo_netstats.last_read_recv = -1;
  foo_netstats.last_read_trans = -1;
  return &foo_netstats;
}

void parse_net_stat_arg(struct text_object *obj, const char *arg,
                        void *free_at_crash) {
  bool shownetmask = false;
  bool showscope = false;
  char nextarg[21];  // longest arg possible is a devname (max 20 chars)
  int i = 0;
  struct net_stat *netstat = nullptr;
  long int x = 0;
  unsigned int found = 0;
  char *arg_ptr = (char *)arg;
  char buf[64];
  char *buf_ptr = buf;

  if (arg == nullptr) { arg = DEFAULTNETDEV; }

  if (0 == (strcmp("$gw_iface", arg)) ||
      0 == (strcmp("${gw_iface}", arg))) {
    arg = e_iface;
  }

  if (0 == strncmp(arg, "${iface", 7)) {
    if (nullptr != arg_ptr) {
      for (; *arg_ptr; arg_ptr++) {
        if (isdigit((unsigned char)*arg_ptr)) {
          *buf_ptr++ = *arg_ptr;
          found = 1;
        }
      }
    }
    if (1U == found) {
      *buf_ptr = '\0';
      x = strtol(buf, (char **)NULL, 10);
      if (63L > x) {
        arg = interfaces_arr[x];
      }
    }
  }

  while (sscanf(arg + i, " %20s", nextarg) == 1) {
    if (strcmp(nextarg, "-n") == 0 || strcmp(nextarg, "--netmask") == 0) {
      shownetmask = true;
    } else if (strcmp(nextarg, "-s") == 0 || strcmp(nextarg, "--scope") == 0) {
      showscope = true;
    } else if (nextarg[0] == '-') {  // multiple flags in 1 arg
      for (int j = 1; nextarg[j] != 0; j++) {
        if (nextarg[j] == 'n') { shownetmask = true; }
        if (nextarg[j] == 's') { showscope = true; }
      }
    } else {
      netstat = get_net_stat(nextarg, obj, free_at_crash);
    }
    i += strlen(nextarg);  // skip this arg
    while (!((isspace((unsigned char)arg[i]) != 0) || arg[i] == 0)) {
      i++;  // and skip the spaces in front of it
    }
  }
  if (netstat == nullptr) {
    netstat = get_net_stat(DEFAULTNETDEV, obj, free_at_crash);
  }

#ifdef BUILD_IPV6
  netstat->v6show_nm = shownetmask;
  netstat->v6show_sc = showscope;
#endif /* BUILD_IPV6 */
  obj->data.opaque = netstat;
}

void parse_net_stat_bar_arg(struct text_object *obj, const char *arg,
                            void *free_at_crash) {
  if (arg != nullptr) {
    arg = scan_bar(obj, arg, 1);
    obj->data.opaque = get_net_stat(arg, obj, free_at_crash);
  } else {
    // default to DEFAULTNETDEV
    char *buf = strndup(DEFAULTNETDEV, text_buffer_size.get(*state));
    obj->data.opaque = get_net_stat(buf, obj, free_at_crash);
    free(buf);
  }
}

void print_downspeed(struct text_object *obj, char *p, unsigned int p_max_size) {
  auto *ns = static_cast<struct net_stat *>(obj->data.opaque);

  if (ns == nullptr) { return; }

  human_readable(ns->recv_speed, p, p_max_size);
}

void print_downspeedf(struct text_object *obj, char *p, unsigned int p_max_size) {
  auto *ns = static_cast<struct net_stat *>(obj->data.opaque);

  if (ns == nullptr) { return; }

  spaced_print(p, p_max_size, "%.1f", 8, ns->recv_speed / 1024.0);
}

void print_upspeed(struct text_object *obj, char *p, unsigned int p_max_size) {
  auto *ns = static_cast<struct net_stat *>(obj->data.opaque);

  if (ns == nullptr) { return; }

  human_readable(ns->trans_speed, p, p_max_size);
}

void print_upspeedf(struct text_object *obj, char *p, unsigned int p_max_size) {
  auto *ns = static_cast<struct net_stat *>(obj->data.opaque);

  if (ns == nullptr) { return; }

  spaced_print(p, p_max_size, "%.1f", 8, ns->trans_speed / 1024.0);
}

void print_totaldown(struct text_object *obj, char *p, unsigned int p_max_size) {
  auto *ns = static_cast<struct net_stat *>(obj->data.opaque);

  if (ns == nullptr) { return; }

  human_readable(ns->recv, p, p_max_size);
}

void print_totalup(struct text_object *obj, char *p, unsigned int p_max_size) {
  auto *ns = static_cast<struct net_stat *>(obj->data.opaque);

  if (ns == nullptr) { return; }

  human_readable(ns->trans, p, p_max_size);
}

void print_addr(struct text_object *obj, char *p, unsigned int p_max_size) {
  auto *ns = static_cast<struct net_stat *>(obj->data.opaque);

  if (ns == nullptr) { return; }

  if ((ns->addr.sa_data[2] & 255) == 0 && (ns->addr.sa_data[3] & 255) == 0 &&
      (ns->addr.sa_data[4] & 255) == 0 && (ns->addr.sa_data[5] & 255) == 0) {
    snprintf(p, p_max_size, "%s", "No Address");
  } else {
    snprintf(p, p_max_size, "%u.%u.%u.%u", ns->addr.sa_data[2] & 255,
             ns->addr.sa_data[3] & 255, ns->addr.sa_data[4] & 255,
             ns->addr.sa_data[5] & 255);
  }
}

#ifdef __linux__
void print_addrs(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) return;

  if (0 != ns->addrs[0] && strlen(ns->addrs) > 2) {
    ns->addrs[strlen(ns->addrs) - 2] = 0; /* remove ", " from end of string */
    strncpy(p, ns->addrs, p_max_size);
  } else {
    strncpy(p, "0.0.0.0", p_max_size);
  }
}

#ifdef BUILD_IPV6
void print_v6addrs(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;
  char tempaddress[INET6_ADDRSTRLEN];
  struct v6addr *current_v6 = ns->v6addrs;

  if (p_max_size == 0) return;
  if (!ns->v6addrs) {
    snprintf(p, p_max_size, "%s", "No Address");
    return;
  }
  *p = 0;
  while (current_v6) {
    inet_ntop(AF_INET6, &(current_v6->addr), tempaddress, INET6_ADDRSTRLEN);
    strncat(p, tempaddress, p_max_size);
    // netmask
    if (ns->v6show_nm) {
      char netmaskstr[5];  // max 5 chars (/128 + null-terminator)
      sprintf(netmaskstr, "/%u", current_v6->netmask);
      strncat(p, netmaskstr, p_max_size);
    }
    // scope
    if (ns->v6show_sc) {
      char scopestr[4];
      sprintf(scopestr, "(%c)", current_v6->scope);
      strncat(p, scopestr, p_max_size);
    }
    // next (or last) address
    current_v6 = current_v6->next;
    if (current_v6) strncat(p, ", ", p_max_size);
  }
}
#endif /* BUILD_IPV6 */

#endif /* __linux__ */

#ifdef BUILD_X11

/**
 * This function is called periodically to update the download and upload graphs
 *
 * - evaluates argument strings like 'eth0 50,120 #FFFFFF #FF0000 0 -l'
 * - sets the obj->data.opaque pointer to the interface specified
 *
 * @param[out] obj  struct which will hold evaluated arguments
 * @param[in]  arg  argument string to evaluate
 **/
void parse_net_stat_graph_arg(struct text_object *obj, const char *arg,
                              void *free_at_crash) {
  /* scan arguments and get interface name back */
  char *buf = nullptr;
  buf = scan_graph(obj, arg, 0);

  // default to DEFAULTNETDEV
  if (buf != nullptr) {
    obj->data.opaque = get_net_stat(buf, obj, free_at_crash);
    free(buf);
    return;
  }
  obj->data.opaque = get_net_stat(DEFAULTNETDEV, obj, free_at_crash);
}

/**
 * returns the download speed in kiB/s for the interface referenced by obj
 *
 * @param[in] obj struct containting a member data, which is a struct
 *                containing a void * to a net_stat struct
 **/
double downspeedgraphval(struct text_object *obj) {
  auto *ns = static_cast<struct net_stat *>(obj->data.opaque);

  return (ns != nullptr ? (ns->recv_speed / 1024.0) : 0);
}

double upspeedgraphval(struct text_object *obj) {
  auto *ns = static_cast<struct net_stat *>(obj->data.opaque);

  return (ns != nullptr ? (ns->trans_speed / 1024.0) : 0);
}
#endif /* BUILD_X11 */

#ifdef BUILD_WLAN
void print_wireless_essid(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) {
    for (unsigned int i = 0; *(netstats[i].dev) != 0; i++) {
      if (*(netstats[i].essid) != 0) {
        snprintf(p, p_max_size, "%s", netstats[i].essid);
        return;
      }
    }
    return;
  }

  snprintf(p, p_max_size, "%s", ns->essid);
}
void print_wireless_mode(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) return;

  snprintf(p, p_max_size, "%s", ns->mode);
}
void print_wireless_channel(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) return;

  if (ns->channel != 0) {
    snprintf(p, p_max_size, "%i", ns->channel);
  } else {
    snprintf(p, p_max_size, "%s", "/");
  }
}
void print_wireless_frequency(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) return;

  if (ns->freq[0] != 0) {
    snprintf(p, p_max_size, "%s", ns->freq);
  } else {
    snprintf(p, p_max_size, "/");
  }
}
void print_wireless_bitrate(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) return;

  snprintf(p, p_max_size, "%s", ns->bitrate);
}
void print_wireless_ap(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) return;

  snprintf(p, p_max_size, "%s", ns->ap);
}
void print_wireless_link_qual(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) return;

  spaced_print(p, p_max_size, "%d", 4, ns->link_qual);
}
void print_wireless_link_qual_max(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) return;

  spaced_print(p, p_max_size, "%d", 4, ns->link_qual_max);
}
void print_wireless_link_qual_perc(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) return;

  if (ns->link_qual_max > 0) {
    spaced_print(p, p_max_size, "%.0f", 5,
                 (double)ns->link_qual / ns->link_qual_max * 100);
  } else {
    spaced_print(p, p_max_size, "unk", 5);
  }
}
double wireless_link_barval(struct text_object *obj) {
  struct net_stat *ns = (struct net_stat *)obj->data.opaque;

  if (!ns) return 0;

  return (double)ns->link_qual / ns->link_qual_max;
}
#endif /* BUILD_WLAN */

/**
 * Clears the global array of net_stat structs which contains networks
 * statistics for every interface.
 **/
void clear_net_stats() {
#ifdef BUILD_IPV6
  struct v6addr *nextv6;
#endif /* BUILD_IPV6 */
  int i;
  for (i = 0; i < MAX_NET_INTERFACES; i++) {
    free_and_zero(netstats[i].dev);
#ifdef BUILD_IPV6
    while (netstats[i].v6addrs) {
      nextv6 = netstats[i].v6addrs;
      netstats[i].v6addrs = netstats[i].v6addrs->next;
      free_and_zero(nextv6);
    }
#endif /* BUILD_IPV6 */
  }
  memset(netstats, 0, sizeof(netstats));
}

void clear_net_stats(net_stat *in) {
#ifdef BUILD_IPV6
  struct v6addr *nextv6;
#endif /* BUILD_IPV6 */
  free_and_zero(in->dev);
#ifdef BUILD_IPV6
  while (in->v6addrs) {
    nextv6 = in->v6addrs;
    in->v6addrs = in->v6addrs->next;
    free_and_zero(nextv6);
  }
#endif /* BUILD_IPV6 */
}

void parse_if_up_arg(struct text_object *obj, const char *arg) {
  obj->data.opaque = strndup(arg, text_buffer_size.get(*state));
}

void free_if_up(struct text_object *obj) { free_and_zero(obj->data.opaque); }

/* We should check if this is ok with OpenBSD and NetBSD as well. */
int interface_up(struct text_object *obj) {
  int fd;
  struct ifreq ifr {};
  auto *dev = static_cast<char *>(obj->data.opaque);

  if (dev == nullptr) { return 0; }

#if defined(__APPLE__) && defined(__MACH__)
  if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
#else
  if ((fd = socket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0)) < 0) {
#endif
    CRIT_ERR(nullptr, nullptr, "could not create sockfd");
    return 0;
  }
  strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  if (ioctl(fd, SIOCGIFFLAGS, &ifr) != 0) {
    /* if device does not exist, treat like not up */
    if (errno != ENODEV && errno != ENXIO) { perror("SIOCGIFFLAGS"); }
    goto END_FALSE;
  }

  if ((ifr.ifr_flags & IFF_UP) == 0) { /* iface is not up */
    goto END_FALSE;
  }
  if (if_up_strictness.get(*state) == IFUP_UP) { goto END_TRUE; }

#ifdef IFF_RUNNING
  if ((ifr.ifr_flags & IFF_RUNNING) == 0) { goto END_FALSE; }
#endif
  if (if_up_strictness.get(*state) == IFUP_LINK) { goto END_TRUE; }

  if (ioctl(fd, SIOCGIFADDR, &ifr) != 0) {
    perror("SIOCGIFADDR");
    goto END_FALSE;
  }
  if ((reinterpret_cast<struct sockaddr_in *>(&(ifr.ifr_addr)))
          ->sin_addr.s_addr != 0u) {
    goto END_TRUE;
  }

END_FALSE:
  close(fd);
  return 0;
END_TRUE:
  close(fd);
  return 1;
}

struct _dns_data {
  _dns_data() = default;
  int nscount{0};
  char **ns_list{nullptr};
};

static _dns_data dns_data;

void free_dns_data(struct text_object *obj) {
  int i;

  (void)obj;

  for (i = 0; i < dns_data.nscount; i++) { free(dns_data.ns_list[i]); }
  free(dns_data.ns_list);
  memset(&dns_data, 0, sizeof(dns_data));
}

int update_dns_data() {
  FILE *fp;
  char line[256];
  // static double last_dns_update = 0.0;

  /* maybe updating too often causes higher load because of /etc lying on a real
  FS if (current_update_time - last_dns_update < 10.0) return 0;

  last_dns_update = current_update_time;
  */

  free_dns_data(nullptr);

  if ((fp = fopen("/etc/resolv.conf", "re")) == nullptr) { return 0; }
  while (feof(fp) == 0) {
    if (fgets(line, 255, fp) == nullptr) { break; }
    if (strncmp(line, "nameserver ", 11) == 0) {
      line[strlen(line) - 1] = '\0';  // remove trailing newline
      dns_data.nscount++;
      dns_data.ns_list = static_cast<char **>(
          realloc(dns_data.ns_list, dns_data.nscount * sizeof(char *)));
      dns_data.ns_list[dns_data.nscount - 1] =
          strndup(line + 11, text_buffer_size.get(*state));
    }
  }
  fclose(fp);
  return 0;
}

void parse_nameserver_arg(struct text_object *obj, const char *arg) {
  obj->data.l = arg != nullptr ? atoi(arg) : 0;
}

void print_nameserver(struct text_object *obj, char *p, unsigned int p_max_size) {
  if (dns_data.nscount > obj->data.l) {
    snprintf(p, p_max_size, "%s", dns_data.ns_list[obj->data.l]);
  }
}
