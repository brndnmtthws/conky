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
 * Copyright (c) 2005-2019 Brenden Matthews, Philip Kovacs, et. al.
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

/* local headers */
#include "core.h"
#include "algebra.h"
#include "bsdapm.h"
#include "build.h"
#include "colours.h"
#include "combine.h"
#include "diskio.h"
#include "entropy.h"
#include "exec.h"
#include "i8k.h"
#include "misc.h"
#include "text_object.h"
#ifdef BUILD_IMLIB2
#include "imlib2.h"
#endif /* BUILD_IMLIB2 */
#include "proc.h"
#ifdef BUILD_MYSQL
#include "mysql.h"
#endif /* BUILD_MYSQL */
#ifdef BUILD_ICAL
#include "ical.h"
#endif /* BUILD_ICAL */
#ifdef BUILD_IRC
#include "irc.h"
#endif /* BUILD_IRC */
#ifdef BUILD_X11
#include "fonts.h"
#endif /* BUILD_X11 */
#include "fs.h"
#ifdef BUILD_IBM
#include "ibm.h"
#include "smapi.h"
#endif /* BUILD_IBM */
#ifdef BUILD_ICONV
#include "iconv_tools.h"
#endif /* BUILD_ICONV */
#include "llua.h"
#include "logging.h"
#include "mail.h"
#include "mboxscan.h"
#include "mixer.h"
#include "nc.h"
#include "net_stat.h"
#ifdef BUILD_NVIDIA
#include "nvidia.h"
#endif /* BUILD_NVIDIA */
#include <inttypes.h>
#include "cpu.h"
#include "read_tcpip.h"
#include "scroll.h"
#include "specials.h"
#include "tailhead.h"
#include "temphelper.h"
#include "template.h"
#include "timeinfo.h"
#include "top.h"
#include "user.h"
#include "users.h"
#ifdef BUILD_CURL
#include "ccurl_thread.h"
#endif /* BUILD_CURL */
#ifdef BUILD_WEATHER_METAR
#include "weather.h"
#endif /* BUILD_WEATHER_METAR */
#ifdef BUILD_RSS
#include "rss.h"
#endif /* BUILD_RSS */
#ifdef BUILD_AUDACIOUS
#include "audacious.h"
#endif /* BUILD_AUDACIOUS */
#ifdef BUILD_CMUS
#include "cmus.h"
#endif /* BUILD_CMUS */
#ifdef BUILD_JOURNAL
#include "journal.h"
#endif /* BUILD_JOURNAL */
#ifdef BUILD_PULSEAUDIO
#include "pulseaudio.h"
#endif /* BUILD_PULSEAUDIO */

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "freebsd.h"
#elif defined(__DragonFly__)
#include "dragonfly.h"
#elif defined(__OpenBSD__)
#include "openbsd.h"
#elif defined(__APPLE__) && defined(__MACH__)
#include "darwin.h"
#endif

#define STRNDUP_ARG strndup(arg ? arg : "", text_buffer_size.get(*state))

#include <cctype>
#include <cstring>

/* strip a leading /dev/ if any, following symlinks first
 *
 * BEWARE: this function returns a pointer to static content
 *         which gets overwritten in consecutive calls. I.e.:
 *         this function is NOT reentrant.
 */
const char *dev_name(const char *path) {
  static char buf[PATH_MAX];

  if (path == nullptr) { return nullptr; }

#define DEV_NAME(x)                                                         \
  ((x) != nullptr && strlen(x) > 5 && strncmp(x, "/dev/", 5) == 0 ? (x) + 5 \
                                                                  : (x))
  if (realpath(path, buf) == nullptr) { return DEV_NAME(path); }
  return DEV_NAME(buf);
#undef DEV_NAME
}

static struct text_object *new_text_object_internal() {
  auto *obj = static_cast<text_object *>(malloc(sizeof(struct text_object)));
  memset(obj, 0, sizeof(struct text_object));
  return obj;
}

static struct text_object *create_plain_text(const char *s) {
  struct text_object *obj;

  if (s == nullptr || *s == '\0') { return nullptr; }

  obj = new_text_object_internal();

  obj_be_plain_text(obj, s);
  return obj;
}

#ifdef BUILD_CURL
void stock_parse_arg(struct text_object *obj, const char *arg) {
  char stock[8];
  char data[16];

  obj->data.s = nullptr;
  if (sscanf(arg, "%7s %15s", stock, data) != 2) {
    NORM_ERR("wrong number of arguments for $stock");
    return;
  }
  if (!strcasecmp("ask", data)) {
    strncpy(data, "a", 3);
  } else if (!strcasecmp("adv", data)) {
    strncpy(data, "a2", 3);
  } else if (!strcasecmp("asksize", data)) {
    strncpy(data, "a5", 3);
  } else if (!strcasecmp("bid", data)) {
    strncpy(data, "b", 3);
  } else if (!strcasecmp("askrt", data)) {
    strncpy(data, "b2", 3);
  } else if (!strcasecmp("bidrt", data)) {
    strncpy(data, "b3", 3);
  } else if (!strcasecmp("bookvalue", data)) {
    strncpy(data, "b4", 3);
  } else if (!strcasecmp("bidsize", data)) {
    strncpy(data, "b6", 3);
  } else if (!strcasecmp("change", data)) {
    strncpy(data, "c1", 3);
  } else if (!strcasecmp("commission", data)) {
    strncpy(data, "c3", 3);
  } else if (!strcasecmp("changert", data)) {
    strncpy(data, "c6", 3);
  } else if (!strcasecmp("ahcrt", data)) {
    strncpy(data, "c8", 3);
  } else if (!strcasecmp("ds", data)) {
    strncpy(data, "d", 3);
  } else if (!strcasecmp("ltd", data)) {
    strncpy(data, "d1", 3);
  } else if (!strcasecmp("tradedate", data)) {
    strncpy(data, "d2", 3);
  } else if (!strcasecmp("es", data)) {
    strncpy(data, "e", 3);
  } else if (!strcasecmp("ei", data)) {
    strncpy(data, "e1", 3);
  } else if (!strcasecmp("epsecy", data)) {
    strncpy(data, "e7", 3);
  } else if (!strcasecmp("epseny", data)) {
    strncpy(data, "e8", 3);
  } else if (!strcasecmp("epsenq", data)) {
    strncpy(data, "e9", 3);
  } else if (!strcasecmp("floatshares", data)) {
    strncpy(data, "f6", 3);
  } else if (!strcasecmp("dayslow", data)) {
    strncpy(data, "g", 3);
  } else if (!strcasecmp("dayshigh", data)) {
    strncpy(data, "h", 3);
  } else if (!strcasecmp("52weeklow", data)) {
    strncpy(data, "j", 3);
  } else if (!strcasecmp("52weekhigh", data)) {
    strncpy(data, "k", 3);
  } else if (!strcasecmp("hgp", data)) {
    strncpy(data, "g1", 3);
  } else if (!strcasecmp("ag", data)) {
    strncpy(data, "g3", 3);
  } else if (!strcasecmp("hg", data)) {
    strncpy(data, "g4", 3);
  } else if (!strcasecmp("hgprt", data)) {
    strncpy(data, "g5", 3);
  } else if (!strcasecmp("hgrt", data)) {
    strncpy(data, "g6", 3);
  } else if (!strcasecmp("moreinfo", data)) {
    strncpy(data, "i", 3);
  } else if (!strcasecmp("obrt", data)) {
    strncpy(data, "i5", 3);
  } else if (!strcasecmp("mc", data)) {
    strncpy(data, "j1", 3);
  } else if (!strcasecmp("mcrt", data)) {
    strncpy(data, "j3", 3);
  } else if (!strcasecmp("ebitda", data)) {
    strncpy(data, "j4", 3);
  } else if (!strcasecmp("c52wlow", data)) {
    strncpy(data, "j5", 3);
  } else if (!strcasecmp("pc52wlow", data)) {
    strncpy(data, "j6", 3);
  } else if (!strcasecmp("cprt", data)) {
    strncpy(data, "k2", 3);
  } else if (!strcasecmp("lts", data)) {
    strncpy(data, "k3", 3);
  } else if (!strcasecmp("c52whigh", data)) {
    strncpy(data, "k4", 3);
  } else if (!strcasecmp("pc52whigh", data)) {
    strncpy(data, "k5", 3);
  } else if (!strcasecmp("ltp", data)) {
    strncpy(data, "l1", 3);
  } else if (!strcasecmp("hl", data)) {
    strncpy(data, "l2", 3);
  } else if (!strcasecmp("ll", data)) {
    strncpy(data, "l3", 3);
  } else if (!strcasecmp("dr", data)) {
    strncpy(data, "m", 3);
  } else if (!strcasecmp("drrt", data)) {
    strncpy(data, "m2", 3);
  } else if (!strcasecmp("50ma", data)) {
    strncpy(data, "m3", 3);
  } else if (!strcasecmp("200ma", data)) {
    strncpy(data, "m4", 3);
  } else if (!strcasecmp("c200ma", data)) {
    strncpy(data, "m5", 3);
  } else if (!strcasecmp("pc200ma", data)) {
    strncpy(data, "m6", 3);
  } else if (!strcasecmp("c50ma", data)) {
    strncpy(data, "m7", 3);
  } else if (!strcasecmp("pc50ma", data)) {
    strncpy(data, "m8", 3);
  } else if (!strcasecmp("name", data)) {
    strncpy(data, "n", 3);
  } else if (!strcasecmp("notes", data)) {
    strncpy(data, "n4", 3);
  } else if (!strcasecmp("open", data)) {
    strncpy(data, "o", 3);
  } else if (!strcasecmp("pc", data)) {
    strncpy(data, "p", 3);
  } else if (!strcasecmp("pricepaid", data)) {
    strncpy(data, "p1", 3);
  } else if (!strcasecmp("cip", data)) {
    strncpy(data, "p2", 3);
  } else if (!strcasecmp("ps", data)) {
    strncpy(data, "p5", 3);
  } else if (!strcasecmp("pb", data)) {
    strncpy(data, "p6", 3);
  } else if (!strcasecmp("edv", data)) {
    strncpy(data, "q", 3);
  } else if (!strcasecmp("per", data)) {
    strncpy(data, "r", 3);
  } else if (!strcasecmp("dpd", data)) {
    strncpy(data, "r1", 3);
  } else if (!strcasecmp("perrt", data)) {
    strncpy(data, "r2", 3);
  } else if (!strcasecmp("pegr", data)) {
    strncpy(data, "r5", 3);
  } else if (!strcasecmp("pepsecy", data)) {
    strncpy(data, "r6", 3);
  } else if (!strcasecmp("pepseny", data)) {
    strncpy(data, "r7", 3);
  } else if (!strcasecmp("symbol", data)) {
    strncpy(data, "s", 3);
  } else if (!strcasecmp("sharesowned", data)) {
    strncpy(data, "s1", 3);
  } else if (!strcasecmp("shortratio", data)) {
    strncpy(data, "s7", 3);
  } else if (!strcasecmp("ltt", data)) {
    strncpy(data, "t1", 3);
  } else if (!strcasecmp("tradelinks", data)) {
    strncpy(data, "t6", 3);
  } else if (!strcasecmp("tt", data)) {
    strncpy(data, "t7", 3);
  } else if (!strcasecmp("1ytp", data)) {
    strncpy(data, "t8", 3);
  } else if (!strcasecmp("volume", data)) {
    strncpy(data, "v", 3);
  } else if (!strcasecmp("hv", data)) {
    strncpy(data, "v1", 3);
  } else if (!strcasecmp("hvrt", data)) {
    strncpy(data, "v7", 3);
  } else if (!strcasecmp("52weekrange", data)) {
    strncpy(data, "w", 3);
  } else if (!strcasecmp("dvc", data)) {
    strncpy(data, "w1", 3);
  } else if (!strcasecmp("dvcrt", data)) {
    strncpy(data, "w4", 3);
  } else if (!strcasecmp("se", data)) {
    strncpy(data, "x", 3);
  } else if (!strcasecmp("dy", data)) {
    strncpy(data, "y", 3);
  } else {
    NORM_ERR(
        "\"%s\" is not supported by $stock. Supported: 1ytp, 200ma, 50ma, "
        "52weeklow, 52weekhigh, 52weekrange, adv, ag, ahcrt, ask, askrt, "
        "asksize, bid, bidrt, bidsize, bookvalue, c200ma, c50ma, c52whigh, "
        "c52wlow, change, changert, cip, commission, cprt, dayshigh, dayslow, "
        "dpd, dr, drrt, ds, dvc, dvcrt, dy, ebitda, edv, ei, epsecy, epsenq, "
        "epseny, es, floatshares, hg, hgp, hgprt, hl, hv, hvrt, ll, ltd, ltp, "
        "lts, ltt, mc, mcrt, moreinfo, name, notes, obrt, open, pb, pc, "
        "pc200ma, pc50ma, pc52whigh, pc52wlow, pegr, pepsecy, pepseny, per, "
        "perrt, pricepaid, ps, se, sharesowned, shortratio, symbol, tradedate, "
        "tradelinks, tt, volume",
        data);
    return;
  }
#define MAX_FINYAH_URL_LENGTH 64
  obj->data.s = static_cast<char *>(malloc(MAX_FINYAH_URL_LENGTH));
  snprintf(obj->data.s, MAX_FINYAH_URL_LENGTH,
           "http://download.finance.yahoo.com/d/quotes.csv?s=%s&f=%s", stock,
           data);
}
#endif /* BUILD_CURL */

legacy_cb_handle *create_cb_handle(int (*fn)()) {
  if (fn != nullptr) {
    return new legacy_cb_handle(conky::register_cb<legacy_cb>(1, fn));
  }
  { return nullptr; }
}

/* construct_text_object() creates a new text_object */
struct text_object *construct_text_object(char *s, const char *arg, long line,
                                          void **ifblock_opaque,
                                          void *free_at_crash) {
  // struct text_object *obj = new_text_object();
  struct text_object *obj = new_text_object_internal();

  obj->line = line;

/* helper defines for internal use only */
#define __OBJ_HEAD(a, n) \
  if (!strcmp(s, #a)) {  \
    obj->cb_handle = create_cb_handle(n);
#define __OBJ_IF obj_be_ifblock_if(ifblock_opaque, obj)
#define __OBJ_ARG(...)                         \
  if (!arg) {                                  \
    free(s);                                   \
    CRIT_ERR(obj, free_at_crash, __VA_ARGS__); \
  }

/* defines to be used below */
#define OBJ(a, n) __OBJ_HEAD(a, n) {
#define OBJ_ARG(a, n, ...) __OBJ_HEAD(a, n) __OBJ_ARG(__VA_ARGS__) {
#define OBJ_IF(a, n)         \
  __OBJ_HEAD(a, n) __OBJ_IF; \
  {
#define OBJ_IF_ARG(a, n, ...)                       \
  __OBJ_HEAD(a, n) __OBJ_ARG(__VA_ARGS__) __OBJ_IF; \
  {
#define END \
  }         \
  }         \
  else

#ifdef BUILD_X11
  if (s[0] == '#') {
    obj->data.l = get_x11_color(s);
    obj->callbacks.print = &new_fg;
  } else
#endif /* BUILD_X11 */
#ifndef __OpenBSD__
    OBJ(acpitemp, nullptr)
  obj->data.i = open_acpi_temperature(arg);
  obj->callbacks.print = &print_acpitemp;
  obj->callbacks.free = &free_acpitemp;
  END OBJ(acpiacadapter, nullptr) if (arg != nullptr) {
#ifdef __linux__
    if (strpbrk(arg, "/.") != nullptr) {
      /*
       * a bit of paranoia. screen out funky paths
       * i hope no device will have a '.' in its name
       */
      NORM_ERR("acpiacadapter: arg must not contain '/' or '.'");
    } else
      obj->data.opaque = strdup(arg);
#else
    NORM_ERR("acpiacadapter: arg is only used on linux");
#endif
  }
  obj->callbacks.print = &print_acpiacadapter;
  obj->callbacks.free = &gen_free_opaque;
#endif /* !__OpenBSD__ */
  END OBJ(freq, nullptr) get_cpu_count();
  if ((arg == nullptr) || (isdigit(static_cast<unsigned char>(arg[0])) == 0) ||
      strlen(arg) >= 3 || atoi(&arg[0]) == 0 ||
      static_cast<unsigned int>(atoi(&arg[0])) > info.cpu_count) {
    obj->data.i = 1;
    /* NORM_ERR("freq: Invalid CPU number or you don't have that many CPUs! "
      "Displaying the clock for CPU 1."); */
  } else {
    obj->data.i = atoi(&arg[0]);
  }
  obj->callbacks.print = &print_freq;
  END OBJ(freq_g, nullptr) get_cpu_count();
  if ((arg == nullptr) || (isdigit(static_cast<unsigned char>(arg[0])) == 0) ||
      strlen(arg) >= 3 || atoi(&arg[0]) == 0 ||
      static_cast<unsigned int>(atoi(&arg[0])) > info.cpu_count) {
    obj->data.i = 1;
    /* NORM_ERR("freq_g: Invalid CPU number or you don't have that many "
      "CPUs! Displaying the clock for CPU 1."); */
  } else {
    obj->data.i = atoi(&arg[0]);
  }
  obj->callbacks.print = &print_freq_g;
  END OBJ_ARG(read_tcp, nullptr,
              "read_tcp: Needs \"(host) port\" as argument(s)")
      parse_read_tcpip_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_read_tcp;
  obj->callbacks.free = &free_read_tcpip;
  END OBJ_ARG(read_udp, nullptr,
              "read_udp: Needs \"(host) port\" as argument(s)")
      parse_read_tcpip_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_read_udp;
  obj->callbacks.free = &free_read_tcpip;
  END OBJ_ARG(tcp_ping, nullptr,
              "tcp_ping: Needs \"host (port)\" as argument(s)")
      parse_tcp_ping_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_tcp_ping;
  obj->callbacks.free = &free_tcp_ping;
#if defined(__linux__)
  END OBJ(voltage_mv, 0) get_cpu_count();
  if (!arg || !isdigit((unsigned char)arg[0]) || strlen(arg) >= 3 ||
      atoi(&arg[0]) == 0 || (unsigned int)atoi(&arg[0]) > info.cpu_count) {
    obj->data.i = 1;
    /* NORM_ERR("voltage_mv: Invalid CPU number or you don't have that many "
      "CPUs! Displaying voltage for CPU 1."); */
  } else {
    obj->data.i = atoi(&arg[0]);
  }
  obj->callbacks.print = &print_voltage_mv;
  END OBJ(voltage_v, 0) get_cpu_count();
  if (!arg || !isdigit((unsigned char)arg[0]) || strlen(arg) >= 3 ||
      atoi(&arg[0]) == 0 || (unsigned int)atoi(&arg[0]) > info.cpu_count) {
    obj->data.i = 1;
    /* NORM_ERR("voltage_v: Invalid CPU number or you don't have that many "
      "CPUs! Displaying voltage for CPU 1."); */
  } else {
    obj->data.i = atoi(&arg[0]);
  }
  obj->callbacks.print = &print_voltage_v;

#endif /* __linux__ */

#ifdef BUILD_WLAN
  END OBJ(wireless_essid, &update_net_stats) obj->data.opaque =
      get_net_stat(arg, obj, free_at_crash);
  parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_wireless_essid;
  END OBJ(wireless_channel, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_wireless_channel;
  END OBJ(wireless_freq, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_wireless_frequency;
  END OBJ(wireless_mode, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_wireless_mode;
  END OBJ(wireless_bitrate, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_wireless_bitrate;
  END OBJ(wireless_ap, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_wireless_ap;
  END OBJ(wireless_link_qual, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_wireless_link_qual;
  END OBJ(wireless_link_qual_max, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_wireless_link_qual_max;
  END OBJ(wireless_link_qual_perc, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_wireless_link_qual_perc;
  END OBJ(wireless_link_bar, &update_net_stats)
      parse_net_stat_bar_arg(obj, arg, free_at_crash);
  obj->callbacks.barval = &wireless_link_barval;
#endif /* BUILD_WLAN */

#ifndef __OpenBSD__
  END OBJ(acpifan, nullptr) obj->callbacks.print = &print_acpifan;
  END OBJ(battery, nullptr) char bat[64];

  if (arg != nullptr) {
    sscanf(arg, "%63s", bat);
  } else {
    strncpy(bat, "BAT0", 5);
  }
  obj->data.s = strndup(bat, text_buffer_size.get(*state));
  obj->callbacks.print = &print_battery;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(battery_short, nullptr) char bat[64];

  if (arg != nullptr) {
    sscanf(arg, "%63s", bat);
  } else {
    strncpy(bat, "BAT0", 5);
  }
  obj->data.s = strndup(bat, text_buffer_size.get(*state));
  obj->callbacks.print = &print_battery_short;
  obj->callbacks.free = &gen_free_opaque;

  END OBJ(battery_status, 0) obj->data.s =
      strndup(arg ? arg : "BAT0", text_buffer_size.get(*state));
  obj->callbacks.print = &print_battery_status;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(battery_time, nullptr) char bat[64];

  if (arg != nullptr) {
    sscanf(arg, "%63s", bat);
  } else {
    strncpy(bat, "BAT0", 5);
  }
  obj->data.s = strndup(bat, text_buffer_size.get(*state));
  obj->callbacks.print = &print_battery_time;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(battery_percent, nullptr) char bat[64];

  if (arg != nullptr) {
    sscanf(arg, "%63s", bat);
  } else {
    strncpy(bat, "BAT0", 5);
  }
  obj->data.s = strndup(bat, text_buffer_size.get(*state));
  obj->callbacks.percentage = &battery_percentage;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(battery_bar, nullptr) char bat[64];

  arg = scan_bar(obj, arg, 100);
  if ((arg != nullptr) && strlen(arg) > 0) {
    sscanf(arg, "%63s", bat);
  } else {
    strncpy(bat, "BAT0", 5);
  }
  obj->data.s = strndup(bat, text_buffer_size.get(*state));
  obj->callbacks.barval = &get_battery_perct_bar;
  obj->callbacks.free = &gen_free_opaque;
#endif /* !__OpenBSD__ */

#if defined(__linux__)
  END OBJ_ARG(disk_protect, 0, "disk_protect needs an argument") obj->data.s =
      strndup(dev_name(arg), text_buffer_size.get(*state));
  obj->callbacks.print = &print_disk_protect_queue;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(i8k_version, &update_i8k) obj->callbacks.print = &print_i8k_version;
  END OBJ(i8k_bios, &update_i8k) obj->callbacks.print = &print_i8k_bios;
  END OBJ(i8k_serial, &update_i8k) obj->callbacks.print = &print_i8k_serial;
  END OBJ(i8k_cpu_temp, &update_i8k) obj->callbacks.print = &print_i8k_cpu_temp;
  END OBJ(i8k_left_fan_status, &update_i8k) obj->callbacks.print =
      &print_i8k_left_fan_status;
  END OBJ(i8k_right_fan_status, &update_i8k) obj->callbacks.print =
      &print_i8k_right_fan_status;
  END OBJ(i8k_left_fan_rpm, &update_i8k) obj->callbacks.print =
      &print_i8k_left_fan_rpm;
  END OBJ(i8k_right_fan_rpm, &update_i8k) obj->callbacks.print =
      &print_i8k_right_fan_rpm;
  END OBJ(i8k_ac_status, &update_i8k) obj->callbacks.print =
      &print_i8k_ac_status;
  END OBJ(i8k_buttons_status, &update_i8k) obj->callbacks.print =
      &print_i8k_buttons_status;
#if defined(BUILD_IBM)
  END OBJ(ibm_fan, 0) obj->callbacks.print = &get_ibm_acpi_fan;
  END OBJ_ARG(ibm_temps, &get_ibm_acpi_temps, "ibm_temps: needs an argument")
      parse_ibm_temps_arg(obj, arg);
  obj->callbacks.print = &print_ibm_temps;
  END OBJ(ibm_volume, 0) obj->callbacks.print = &get_ibm_acpi_volume;
  END OBJ(ibm_brightness, 0) obj->callbacks.print = &get_ibm_acpi_brightness;
  END OBJ(ibm_thinklight, 0) obj->callbacks.print = &get_ibm_acpi_thinklight;
#endif
  /* information from sony_laptop kernel module
   * /sys/devices/platform/sony-laptop */
  END OBJ(sony_fanspeed, 0) obj->callbacks.print = &get_sony_fanspeed;
  END OBJ_ARG(ioscheduler, 0, "get_ioscheduler needs an argument (e.g. hda)")
      obj->data.s = strndup(dev_name(arg), text_buffer_size.get(*state));
  obj->callbacks.print = &print_ioscheduler;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(laptop_mode, 0) obj->callbacks.print = &print_laptop_mode;
  END OBJ_ARG(
      pb_battery, 0,
      "pb_battery: needs one argument: status, percent or time") if (strcmp(arg,
                                                                            "st"
                                                                            "at"
                                                                            "u"
                                                                            "s") ==
                                                                     EQUAL) {
    obj->data.i = PB_BATT_STATUS;
  }
  else if (strcmp(arg, "percent") == EQUAL) {
    obj->data.i = PB_BATT_PERCENT;
  }
  else if (strcmp(arg, "time") == EQUAL) {
    obj->data.i = PB_BATT_TIME;
  }
  else {
    NORM_ERR("pb_battery: illegal argument '%s', defaulting to status", arg);
    obj->data.i = PB_BATT_STATUS;
  }
  obj->callbacks.print = get_powerbook_batt_info;
#endif /* __linux__ */
#if (defined(__FreeBSD__) || defined(__linux__) || defined(__DragonFly__) || \
     (defined(__APPLE__) && defined(__MACH__)))
  END OBJ_IF_ARG(if_up, nullptr, "if_up needs an argument")
      parse_if_up_arg(obj, arg);
  obj->callbacks.iftest = &interface_up;
  obj->callbacks.free = &free_if_up;
#endif
#if defined(__OpenBSD__)
  END OBJ_ARG(obsd_sensors_temp, 0, "obsd_sensors_temp: needs an argument")
      parse_obsd_sensor(obj, arg);
  obj->callbacks.print = &print_obsd_sensors_temp;
  END OBJ_ARG(obsd_sensors_fan, 0,
              "obsd_sensors_fan: needs 2 arguments (device and sensor number)")
      parse_obsd_sensor(obj, arg);
  obj->callbacks.print = &print_obsd_sensors_fan;
  END OBJ_ARG(obsd_sensors_volt, 0,
              "obsd_sensors_volt: needs 2 arguments (device and sensor number)")
      parse_obsd_sensor(obj, arg);
  obj->callbacks.print = &print_obsd_sensors_volt;
  END OBJ(obsd_vendor, 0) obj->callbacks.print = &get_obsd_vendor;
  END OBJ(obsd_product, 0) obj->callbacks.print = &get_obsd_product;
#endif /* __OpenBSD__ */
  END OBJ(buffers, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_buffers;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(cached, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_cached;
  obj->callbacks.free = &gen_free_opaque;
#define SCAN_CPU(__arg, __var)                                          \
  {                                                                     \
    int __offset = 0;                                                   \
    if ((__arg) && sscanf(__arg, " cpu%d %n", &(__var), &__offset) > 0) \
      (__arg) += __offset;                                              \
    else                                                                \
      (__var) = 0;                                                      \
  }
  END OBJ(cpu, &update_cpu_usage) get_cpu_count();
  SCAN_CPU(arg, obj->data.i);
  obj->callbacks.percentage = &cpu_percentage;
  obj->callbacks.free = &free_cpu;
  DBGP2("Adding $cpu for CPU %d", obj->data.i);
#ifdef BUILD_X11
  END OBJ(cpugauge, &update_cpu_usage) get_cpu_count();
  SCAN_CPU(arg, obj->data.i);
  scan_gauge(obj, arg, 1);
  obj->callbacks.gaugeval = &cpu_barval;
  obj->callbacks.free = &free_cpu;
  DBGP2("Adding $cpugauge for CPU %d", obj->data.i);
#endif
  END OBJ(cpubar, &update_cpu_usage) get_cpu_count();
  SCAN_CPU(arg, obj->data.i);
  scan_bar(obj, arg, 1);
  obj->callbacks.barval = &cpu_barval;
  obj->callbacks.free = &free_cpu;
  DBGP2("Adding $cpubar for CPU %d", obj->data.i);
#ifdef BUILD_X11
  END OBJ(cpugraph, &update_cpu_usage) get_cpu_count();
  char *buf = nullptr;
  SCAN_CPU(arg, obj->data.i);
  buf = scan_graph(obj, arg, 1);
  DBGP2("Adding $cpugraph for CPU %d", obj->data.i);
  free_and_zero(buf);
  obj->callbacks.graphval = &cpu_barval;
  obj->callbacks.free = &free_cpu;
  END OBJ(loadgraph, &update_load_average) scan_loadgraph_arg(obj, arg);
  obj->callbacks.graphval = &loadgraphval;
#endif /* BUILD_X11 */
  END OBJ(diskio, &update_diskio) parse_diskio_arg(obj, arg);
  obj->callbacks.print = &print_diskio;
  END OBJ(diskio_read, &update_diskio) parse_diskio_arg(obj, arg);
  obj->callbacks.print = &print_diskio_read;
  END OBJ(diskio_write, &update_diskio) parse_diskio_arg(obj, arg);
  obj->callbacks.print = &print_diskio_write;
#ifdef BUILD_X11
  END OBJ(diskiograph, &update_diskio) parse_diskiograph_arg(obj, arg);
  obj->callbacks.graphval = &diskiographval;
  END OBJ(diskiograph_read, &update_diskio) parse_diskiograph_arg(obj, arg);
  obj->callbacks.graphval = &diskiographval_read;
  END OBJ(diskiograph_write, &update_diskio) parse_diskiograph_arg(obj, arg);
  obj->callbacks.graphval = &diskiographval_write;
#endif /* BUILD_X11 */
  END OBJ(color, nullptr)
#ifdef BUILD_X11
      if (out_to_x.get(*state)) {
    obj->data.l =
        arg != nullptr ? get_x11_color(arg) : default_color.get(*state);
    set_current_text_color(obj->data.l);
  }
#endif /* BUILD_X11 */
#ifdef BUILD_NCURSES
  if (out_to_ncurses.get(*state)) {
    obj->data.l = COLOR_WHITE;
    if (arg != nullptr) {
      if (strcasecmp(arg, "red") == 0) {
        obj->data.l = COLOR_RED;
      } else if (strcasecmp(arg, "green") == 0) {
        obj->data.l = COLOR_GREEN;
      } else if (strcasecmp(arg, "yellow") == 0) {
        obj->data.l = COLOR_YELLOW;
      } else if (strcasecmp(arg, "blue") == 0) {
        obj->data.l = COLOR_BLUE;
      } else if (strcasecmp(arg, "magenta") == 0) {
        obj->data.l = COLOR_MAGENTA;
      } else if (strcasecmp(arg, "cyan") == 0) {
        obj->data.l = COLOR_CYAN;
      } else if (strcasecmp(arg, "black") == 0) {
        obj->data.l = COLOR_BLACK;
      }
    }
    set_current_text_color(obj->data.l);
    init_pair(obj->data.l, obj->data.l, COLOR_BLACK);
  }
#endif /* BUILD_NCURSES */
  obj->callbacks.print = &new_fg;
#ifdef BUILD_X11
  END OBJ(color0, nullptr) obj->data.l = color[0].get(*state);
  set_current_text_color(obj->data.l);
  obj->callbacks.print = &new_fg;
  END OBJ(color1, nullptr) obj->data.l = color[1].get(*state);
  set_current_text_color(obj->data.l);
  obj->callbacks.print = &new_fg;
  END OBJ(color2, nullptr) obj->data.l = color[2].get(*state);
  set_current_text_color(obj->data.l);
  obj->callbacks.print = &new_fg;
  END OBJ(color3, nullptr) obj->data.l = color[3].get(*state);
  set_current_text_color(obj->data.l);
  obj->callbacks.print = &new_fg;
  END OBJ(color4, nullptr) obj->data.l = color[4].get(*state);
  set_current_text_color(obj->data.l);
  obj->callbacks.print = &new_fg;
  END OBJ(color5, nullptr) obj->data.l = color[5].get(*state);
  set_current_text_color(obj->data.l);
  obj->callbacks.print = &new_fg;
  END OBJ(color6, nullptr) obj->data.l = color[6].get(*state);
  set_current_text_color(obj->data.l);
  obj->callbacks.print = &new_fg;
  END OBJ(color7, nullptr) obj->data.l = color[7].get(*state);
  set_current_text_color(obj->data.l);
  obj->callbacks.print = &new_fg;
  END OBJ(color8, nullptr) obj->data.l = color[8].get(*state);
  set_current_text_color(obj->data.l);
  obj->callbacks.print = &new_fg;
  END OBJ(color9, nullptr) obj->data.l = color[9].get(*state);
  set_current_text_color(obj->data.l);
  obj->callbacks.print = &new_fg;
  END OBJ(font, nullptr) scan_font(obj, arg);
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(font0, nullptr) scan_font(obj, font_template[0].get(*state).c_str());
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(font1, nullptr) scan_font(obj, font_template[1].get(*state).c_str());
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(font2, nullptr) scan_font(obj, font_template[2].get(*state).c_str());
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(font3, nullptr) scan_font(obj, font_template[3].get(*state).c_str());
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(font4, nullptr) scan_font(obj, font_template[4].get(*state).c_str());
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(font5, nullptr) scan_font(obj, font_template[5].get(*state).c_str());
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(font6, nullptr) scan_font(obj, font_template[6].get(*state).c_str());
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(font7, nullptr) scan_font(obj, font_template[7].get(*state).c_str());
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(font8, nullptr) scan_font(obj, font_template[8].get(*state).c_str());
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(font9, nullptr) scan_font(obj, font_template[9].get(*state).c_str());
  obj->callbacks.print = &new_font;
  obj->callbacks.free = &gen_free_opaque;
#endif /* BUILD_X11 */
  END OBJ(conky_version, nullptr) obj_be_plain_text(obj, VERSION);
  END OBJ(conky_build_date, nullptr) obj_be_plain_text(obj, BUILD_DATE);
  END OBJ(conky_build_arch, nullptr) obj_be_plain_text(obj, BUILD_ARCH);
  END OBJ(downspeed, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_downspeed;
  END OBJ(downspeedf, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_downspeedf;
#ifdef BUILD_X11
  END OBJ(downspeedgraph, &update_net_stats)
      parse_net_stat_graph_arg(obj, arg, free_at_crash);
  obj->callbacks.graphval = &downspeedgraphval;
#endif /* BUILD_X11 */
  END OBJ(else, nullptr) obj_be_ifblock_else(ifblock_opaque, obj);
  obj->callbacks.iftest = &gen_false_iftest;
  END OBJ(endif, nullptr) obj_be_ifblock_endif(ifblock_opaque, obj);
  obj->callbacks.print = &gen_print_nothing;
  END OBJ(eval, nullptr) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_evaluate;
  obj->callbacks.free = &gen_free_opaque;
#if defined(BUILD_IMLIB2) && defined(BUILD_X11)
  END OBJ(image, nullptr) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_image_callback;
  obj->callbacks.free = &gen_free_opaque;
#endif /* BUILD_IMLIB2 */
#ifdef BUILD_MYSQL
  END OBJ_ARG(mysql, 0, "mysql needs a query") obj->data.s = strdup(arg);
  obj->callbacks.print = &print_mysql;
#endif /* BUILD_MYSQL */
  END OBJ_ARG(no_update, nullptr, "no_update needs arguments")
      scan_no_update(obj, arg);
  obj->callbacks.print = &print_no_update;
  obj->callbacks.free = &free_no_update;
  END OBJ(cat, 0) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_cat;
  obj->callbacks.free = &gen_free_opaque;

#ifdef BUILD_X11
  END OBJ(key_num_lock, 0) obj->callbacks.print = &print_key_num_lock;
  END OBJ(key_caps_lock, 0) obj->callbacks.print = &print_key_caps_lock;
  END OBJ(key_scroll_lock, 0) obj->callbacks.print = &print_key_scroll_lock;
  END OBJ(keyboard_layout, 0) obj->callbacks.print = &print_keyboard_layout;
  END OBJ(mouse_speed, 0) obj->callbacks.print = &print_mouse_speed;
#endif /* BUILD_X11 */

#ifdef __FreeBSD__
  END OBJ(sysctlbyname, 0) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_sysctlbyname;
  obj->callbacks.free = &gen_free_opaque;
#endif /* __FreeBSD__ */

  END OBJ(password, 0) obj->data.s =
      strndup(arg ? arg : "20", text_buffer_size.get(*state));
  obj->callbacks.print = &print_password;
  obj->callbacks.free = &gen_free_opaque;

#ifdef __x86_64__
  END OBJ(freq2, 0) obj->callbacks.print = &print_freq2;
#endif /* __x86_64__ */
  END OBJ(startcase, 0) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_startcase;
  obj->callbacks.free = &gen_free_opaque;
  // Deprecated, for compatibility purposes only
  END OBJ(start_case, 0) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_startcase;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(lowercase, 0) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_lowercase;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(uppercase, 0) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_uppercase;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(catp, 0) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_catp;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_ARG(exec, nullptr, "exec needs arguments: <command>")
      scan_exec_arg(obj, arg, EF_EXEC);
  obj->parse = false;
  obj->thread = false;
  register_exec(obj);
  obj->callbacks.print = &print_exec;
  obj->callbacks.free = &free_exec;
  END OBJ_ARG(execi, nullptr, "execi needs arguments: <interval> <command>")
      scan_exec_arg(obj, arg, EF_EXECI);
  obj->parse = false;
  obj->thread = false;
  register_execi(obj);
  obj->callbacks.print = &print_exec;
  obj->callbacks.free = &free_execi;
  END OBJ_ARG(execp, nullptr, "execp needs arguments: <command>")
      scan_exec_arg(obj, arg, EF_EXEC);
  obj->parse = true;
  obj->thread = false;
  register_exec(obj);
  obj->callbacks.print = &print_exec;
  obj->callbacks.free = &free_exec;
  END OBJ_ARG(execpi, nullptr, "execpi needs arguments: <interval> <command>")
      scan_exec_arg(obj, arg, EF_EXECI);
  obj->parse = true;
  obj->thread = false;
  register_execi(obj);
  obj->callbacks.print = &print_exec;
  obj->callbacks.free = &free_execi;
  END OBJ_ARG(execbar, nullptr,
              "execbar needs arguments: [height],[width] <command>")
      scan_exec_arg(obj, arg, EF_EXEC | EF_BAR);
  register_exec(obj);
  obj->callbacks.barval = &execbarval;
  obj->callbacks.free = &free_exec;
  END OBJ_ARG(execibar, nullptr,
              "execibar needs arguments: <interval> [height],[width] <command>")
      scan_exec_arg(obj, arg, EF_EXECI | EF_BAR);
  register_execi(obj);
  obj->callbacks.barval = &execbarval;
  obj->callbacks.free = &free_execi;
#ifdef BUILD_X11
  END OBJ_ARG(execgauge, nullptr,
              "execgauge needs arguments: [height],[width] <command>")
      scan_exec_arg(obj, arg, EF_EXEC | EF_GAUGE);
  register_exec(obj);
  obj->callbacks.gaugeval = &execbarval;
  obj->callbacks.free = &free_exec;
  END OBJ_ARG(
      execigauge, nullptr,
      "execigauge needs arguments: <interval> [height],[width] <command>")
      scan_exec_arg(obj, arg, EF_EXECI | EF_GAUGE);
  register_execi(obj);
  obj->callbacks.gaugeval = &execbarval;
  obj->callbacks.free = &free_execi;
  END OBJ_ARG(execgraph, nullptr,
              "execgraph needs arguments: <command> [height],[width] [color1] "
              "[color2] [scale] [-t|-l]")
      scan_exec_arg(obj, arg, EF_EXEC | EF_GRAPH);
  register_exec(obj);
  obj->callbacks.graphval = &execbarval;
  obj->callbacks.free = &free_exec;
  END OBJ_ARG(execigraph, nullptr,
              "execigraph needs arguments: <interval> <command> "
              "[height],[width] [color1] [color2] [scale] [-t|-l]")
      scan_exec_arg(obj, arg, EF_EXECI | EF_GRAPH);
  register_execi(obj);
  obj->callbacks.graphval = &execbarval;
  obj->callbacks.free = &free_execi;
#endif /* BUILD_X11 */
  END OBJ_ARG(texeci, nullptr, "texeci needs arguments: <interval> <command>")
      scan_exec_arg(obj, arg, EF_EXECI);
  obj->parse = false;
  obj->thread = true;
  register_execi(obj);
  obj->callbacks.print = &print_exec;
  obj->callbacks.free = &free_execi;
  END OBJ_ARG(texecpi, nullptr, "texecpi needs arguments: <interval> <command>")
      scan_exec_arg(obj, arg, EF_EXECI);
  obj->parse = true;
  obj->thread = true;
  register_execi(obj);
  obj->callbacks.print = &print_exec;
  obj->callbacks.free = &free_execi;
  END OBJ(fs_bar, &update_fs_stats) init_fs_bar(obj, arg);
  obj->callbacks.barval = &fs_barval;
  END OBJ(fs_bar_free, &update_fs_stats) init_fs_bar(obj, arg);
  obj->callbacks.barval = &fs_free_barval;
  END OBJ(fs_free, &update_fs_stats) init_fs(obj, arg);
  obj->callbacks.print = &print_fs_free;
  END OBJ(fs_used_perc, &update_fs_stats) init_fs(obj, arg);
  obj->callbacks.percentage = &fs_used_percentage;
  END OBJ(fs_free_perc, &update_fs_stats) init_fs(obj, arg);
  obj->callbacks.percentage = &fs_free_percentage;
  END OBJ(fs_size, &update_fs_stats) init_fs(obj, arg);
  obj->callbacks.print = &print_fs_size;
  END OBJ(fs_type, &update_fs_stats) init_fs(obj, arg);
  obj->callbacks.print = &print_fs_type;
  END OBJ(fs_used, &update_fs_stats) init_fs(obj, arg);
  obj->callbacks.print = &print_fs_used;
#ifdef BUILD_X11
  END OBJ(hr, nullptr) obj->data.l = arg != nullptr ? atoi(arg) : 1;
  obj->callbacks.print = &new_hr;
#endif /* BUILD_X11 */
  END OBJ(nameserver, &update_dns_data) parse_nameserver_arg(obj, arg);
  obj->callbacks.print = &print_nameserver;
  obj->callbacks.free = &free_dns_data;
  END OBJ(offset, nullptr) obj->data.l = arg != nullptr ? atoi(arg) : 1;
  obj->callbacks.print = &new_offset;
  END OBJ(voffset, nullptr) obj->data.l = arg != nullptr ? atoi(arg) : 1;
  obj->callbacks.print = &new_voffset;
  END OBJ_ARG(goto, nullptr, "goto needs arguments") obj->data.l = atoi(arg);
  obj->callbacks.print = &new_goto;
#ifdef BUILD_X11
  END OBJ(tab, nullptr) scan_tab(obj, arg);
  obj->callbacks.print = &new_tab;
#endif /* BUILD_X11 */
#ifdef __linux__
  END OBJ_ARG(i2c, 0, "i2c needs arguments") parse_i2c_sensor(obj, arg);
  obj->callbacks.print = &print_sysfs_sensor;
  obj->callbacks.free = &free_sysfs_sensor;
  END OBJ_ARG(platform, 0, "platform needs arguments")
      parse_platform_sensor(obj, arg);
  obj->callbacks.print = &print_sysfs_sensor;
  obj->callbacks.free = &free_sysfs_sensor;
  END OBJ_ARG(hwmon, 0, "hwmon needs argumanets") parse_hwmon_sensor(obj, arg);
  obj->callbacks.print = &print_sysfs_sensor;
  obj->callbacks.free = &free_sysfs_sensor;
#endif /* __linux__ */
  END
      /* we have four different types of top (top, top_mem, top_time and
       * top_io). To avoid having almost-same code four times, we have this
       * special handler. */
      /* XXX: maybe fiddle them apart later, as print_top() does
       * nothing else than just that, using an ugly switch(). */
      if (strncmp(s, "top", 3) == EQUAL) {
    if (parse_top_args(s, arg, obj) != 0) {
#ifdef __linux__
      determine_longstat_file();
#endif
      obj->cb_handle = create_cb_handle(update_top);
    } else {
      free(obj);
      return nullptr;
    }
  }
  else OBJ(addr, &update_net_stats) parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_addr;
  END
#ifdef __linux__
      OBJ(addrs, &update_net_stats) parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_addrs;
#ifdef BUILD_IPV6
  END OBJ(v6addrs, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_v6addrs;
#endif /* BUILD_IPV6 */
  END
#endif /* __linux__ */
      OBJ_ARG(tail, nullptr, "tail needs arguments")
          init_tailhead("tail", arg, obj, free_at_crash);
  obj->callbacks.print = &print_tail;
  obj->callbacks.free = &free_tailhead;
  END OBJ_ARG(head, nullptr, "head needs arguments")
      init_tailhead("head", arg, obj, free_at_crash);
  obj->callbacks.print = &print_head;
  obj->callbacks.free = &free_tailhead;
  END OBJ_ARG(lines, nullptr, "lines needs an argument") obj->data.s =
      STRNDUP_ARG;
  obj->callbacks.print = &print_lines;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_ARG(words, nullptr, "words needs a argument") obj->data.s =
      STRNDUP_ARG;
  obj->callbacks.print = &print_words;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(loadavg, &update_load_average) scan_loadavg_arg(obj, arg);
  obj->callbacks.print = &print_loadavg;
  END OBJ_IF_ARG(if_empty, nullptr, "if_empty needs an argument") obj->sub =
      static_cast<text_object *>(malloc(sizeof(struct text_object)));
  extract_variable_text_internal(obj->sub, arg);
  obj->callbacks.iftest = &if_empty_iftest;
  END OBJ_IF_ARG(if_match, nullptr, "if_match needs arguments") obj->sub =
      static_cast<text_object *>(malloc(sizeof(struct text_object)));
  extract_variable_text_internal(obj->sub, arg);
  obj->callbacks.iftest = &check_if_match;
  END OBJ_IF_ARG(if_existing, nullptr, "if_existing needs an argument or two")
      obj->data.s = STRNDUP_ARG;
  obj->callbacks.iftest = &if_existing_iftest;
  obj->callbacks.free = &gen_free_opaque;
#if defined(__linux__) || defined(__FreeBSD__)
  END OBJ_IF_ARG(if_mounted, 0, "if_mounted needs an argument") obj->data.s =
      STRNDUP_ARG;
  obj->callbacks.iftest = &check_mount;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_IF_ARG(if_running, &update_top, "if_running needs an argument")
      top_running = 1;
  obj->data.s = STRNDUP_ARG;
  obj->callbacks.iftest = &if_running_iftest;
  obj->callbacks.free = &gen_free_opaque;
#elif defined(__APPLE__) && defined(__MACH__)
  END OBJ_IF_ARG(if_mounted, nullptr, "if_mounted needs an argument")
      obj->data.s = STRNDUP_ARG;
  obj->callbacks.iftest = &check_mount;
  obj->callbacks.free = &gen_free_opaque;

  /* System Integrity Protection */
  END OBJ(sip_status, &get_sip_status) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_sip_status;
  obj->callbacks.free = &gen_free_opaque;
#else
  END OBJ_IF_ARG(if_running, 0, "if_running needs an argument")

      char buf[DEFAULT_TEXT_BUFFER_SIZE];

  snprintf(buf, DEFAULT_TEXT_BUFFER_SIZE, "pidof %s >/dev/null", arg);
  obj->data.s = STRNDUP_ARG;
  /* XXX: maybe use a different callback here */
  obj->callbacks.iftest = &if_running_iftest;
#endif
  END OBJ(kernel, nullptr) obj->callbacks.print = &print_kernel;
  END OBJ(machine, nullptr) obj->callbacks.print = &print_machine;
#if defined(__DragonFly__)
  END OBJ(version, 0) obj->callbacks.print = &print_version;
#endif
  END OBJ(mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(new_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_new_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(seen_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_seen_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(unseen_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_unseen_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(flagged_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_flagged_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(unflagged_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_unflagged_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(forwarded_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_forwarded_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(unforwarded_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_unforwarded_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(replied_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_replied_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(unreplied_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_unreplied_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(draft_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_draft_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(trashed_mails, nullptr) parse_local_mail_args(obj, arg);
  obj->callbacks.print = &print_trashed_mails;
  obj->callbacks.free = &free_local_mails;
  END OBJ(mboxscan, nullptr) parse_mboxscan_arg(obj, arg);
  obj->callbacks.print = &print_mboxscan;
  obj->callbacks.free = &free_mboxscan;
  END OBJ(mem, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_mem;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(memwithbuffers, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_memwithbuffers;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(memeasyfree, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_memeasyfree;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(memfree, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_memfree;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(memmax, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_memmax;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(memperc, &update_meminfo) obj->callbacks.percentage = &mem_percentage;
#ifdef __linux__
  END OBJ(memdirty, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_memdirty;
  obj->callbacks.free = &gen_free_opaque;
#endif /* __linux__ */
#ifdef BUILD_X11
  END OBJ(memgauge, &update_meminfo) scan_gauge(obj, arg, 1);
  obj->callbacks.gaugeval = &mem_barval;
#endif /* BUILD_X11 */
  END OBJ(membar, &update_meminfo) scan_bar(obj, arg, 1);
  obj->callbacks.barval = &mem_barval;
  END OBJ(memwithbuffersbar, &update_meminfo) scan_bar(obj, arg, 1);
  obj->callbacks.barval = &mem_with_buffers_barval;
#ifdef BUILD_X11
  END OBJ(memgraph, &update_meminfo) char *buf = nullptr;
  buf = scan_graph(obj, arg, 1);
  free_and_zero(buf);
  obj->callbacks.graphval = &mem_barval;
  END OBJ(memwithbuffersgraph, &update_meminfo) char *buf = nullptr;
  buf = scan_graph(obj, arg, 1);
  free_and_zero(buf);
  obj->callbacks.graphval = &mem_with_buffers_barval;
#endif /* BUILD_X11*/
#ifdef HAVE_SOME_SOUNDCARD_H
  END OBJ(mixer, 0) parse_mixer_arg(obj, arg);
  obj->callbacks.percentage = &mixer_percentage;
  END OBJ(mixerl, 0) parse_mixer_arg(obj, arg);
  obj->callbacks.percentage = &mixerl_percentage;
  END OBJ(mixerr, 0) parse_mixer_arg(obj, arg);
  obj->callbacks.percentage = &mixerr_percentage;
  END OBJ(mixerbar, 0) scan_mixer_bar(obj, arg);
  obj->callbacks.barval = &mixer_barval;
  END OBJ(mixerlbar, 0) scan_mixer_bar(obj, arg);
  obj->callbacks.barval = &mixerl_barval;
  END OBJ(mixerrbar, 0) scan_mixer_bar(obj, arg);
  obj->callbacks.barval = &mixerr_barval;
  END OBJ_IF(if_mixer_mute, 0) parse_mixer_arg(obj, arg);
  obj->callbacks.iftest = &check_mixer_muted;
#endif /* HAVE_SOME_SOUNDCARD_H */
#ifdef BUILD_X11
  END OBJ(monitor, nullptr) obj->callbacks.print = &print_monitor;
  END OBJ(monitor_number, nullptr) obj->callbacks.print = &print_monitor_number;
  END OBJ(desktop, nullptr) obj->callbacks.print = &print_desktop;
  END OBJ(desktop_number, nullptr) obj->callbacks.print = &print_desktop_number;
  END OBJ(desktop_name, nullptr) obj->callbacks.print = &print_desktop_name;
#endif /* BUILD_X11 */
  END OBJ_ARG(format_time, nullptr, "format_time needs a pid as argument")
      obj->sub = static_cast<text_object *>(malloc(sizeof(struct text_object)));
  extract_variable_text_internal(obj->sub, arg);
  obj->callbacks.print = &print_format_time;
  END OBJ(nodename, nullptr) obj->callbacks.print = &print_nodename;
  END OBJ(nodename_short, nullptr) obj->callbacks.print = &print_nodename_short;
  END OBJ_ARG(cmdline_to_pid, nullptr,
              "cmdline_to_pid needs a command line as argument")
      scan_cmdline_to_pid_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_cmdline_to_pid;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_ARG(pid_chroot, nullptr, "pid_chroot needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_chroot;
  END OBJ_ARG(pid_cmdline, nullptr, "pid_cmdline needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_cmdline;
  END OBJ_ARG(pid_cwd, nullptr, "pid_cwd needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_cwd;
  END OBJ_ARG(pid_environ, nullptr, "pid_environ needs arguments")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_environ;
  END OBJ_ARG(pid_environ_list, nullptr,
              "pid_environ_list needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_environ_list;
  END OBJ_ARG(pid_exe, nullptr, "pid_exe needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_exe;
  END OBJ_ARG(pid_nice, nullptr, "pid_nice needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_nice;
  END OBJ_ARG(pid_openfiles, nullptr, "pid_openfiles needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_openfiles;
  END OBJ_ARG(pid_parent, nullptr, "pid_parent needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_parent;
  END OBJ_ARG(pid_priority, nullptr, "pid_priority needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_priority;
  END OBJ_ARG(pid_state, nullptr, "pid_state needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_state;
  END OBJ_ARG(pid_state_short, nullptr,
              "pid_state_short needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_state_short;
  END OBJ_ARG(pid_stderr, nullptr, "pid_stderr needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_stderr;
  END OBJ_ARG(pid_stdin, nullptr, "pid_stdin needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_stdin;
  END OBJ_ARG(pid_stdout, nullptr, "pid_stdout needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_stdout;
  END OBJ_ARG(pid_threads, nullptr, "pid_threads needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_threads;
  END OBJ_ARG(pid_thread_list, nullptr,
              "pid_thread_list needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_thread_list;
  END OBJ_ARG(pid_time_kernelmode, nullptr,
              "pid_time_kernelmode needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_time_kernelmode;
  END OBJ_ARG(pid_time_usermode, nullptr,
              "pid_time_usermode needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_time_usermode;
  END OBJ_ARG(pid_time, nullptr, "pid_time needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_time;
  END OBJ_ARG(pid_uid, nullptr, "pid_uid needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_uid;
  END OBJ_ARG(pid_euid, nullptr, "pid_euid needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_euid;
  END OBJ_ARG(pid_suid, nullptr, "pid_suid needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_suid;
  END OBJ_ARG(pid_fsuid, nullptr, "pid_fsuid needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_fsuid;
  END OBJ_ARG(pid_gid, nullptr, "pid_gid needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_gid;
  END OBJ_ARG(pid_egid, nullptr, "pid_egid needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_egid;
  END OBJ_ARG(pid_sgid, nullptr, "pid_sgid needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_sgid;
  END OBJ_ARG(pid_fsgid, nullptr, "pid_fsgid needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_fsgid;
  END OBJ_ARG(gid_name, nullptr, "gid_name needs a gid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_gid_name;
  END OBJ_ARG(uid_name, nullptr, "uid_name needs a uid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_uid_name;
  END OBJ_ARG(pid_read, nullptr, "pid_read needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_read;
  END OBJ_ARG(pid_vmpeak, nullptr, "pid_vmpeak needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_vmpeak;
  END OBJ_ARG(pid_vmsize, nullptr, "pid_vmsize needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_vmsize;
  END OBJ_ARG(pid_vmlck, nullptr, "pid_vmlck needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_vmlck;
  END OBJ_ARG(pid_vmhwm, nullptr, "pid_vmhwm needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_vmhwm;
  END OBJ_ARG(pid_vmrss, nullptr, "pid_vmrss needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_vmrss;
  END OBJ_ARG(pid_vmdata, nullptr, "pid_vmdata needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_vmdata;
  END OBJ_ARG(pid_vmstk, nullptr, "pid_vmstk needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_vmstk;
  END OBJ_ARG(pid_vmexe, nullptr, "pid_vmexe needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_vmexe;
  END OBJ_ARG(pid_vmlib, nullptr, "pid_vmlib needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_vmlib;
  END OBJ_ARG(pid_vmpte, nullptr, "pid_vmpte needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_vmpte;
  END OBJ_ARG(pid_write, nullptr, "pid_write needs a pid as argument")
      extract_object_args_to_sub(obj, arg);
  obj->callbacks.print = &print_pid_write;
#ifdef __DragonFly__
  END OBJ(processes, &update_top)
#else
  END OBJ(processes, &update_total_processes)
#endif
      obj->callbacks.print = &print_processes;
#ifdef __linux__
  END OBJ(distribution, 0) obj->callbacks.print = &print_distribution;
  END OBJ(running_processes, &update_top) top_running = 1;
  obj->callbacks.print = &print_running_processes;
  END OBJ(threads, &update_threads) obj->callbacks.print = &print_threads;
  END OBJ(running_threads, &update_stat) obj->callbacks.print =
      &print_running_threads;
#else
#if defined(__DragonFly__)
  END OBJ(running_processes, &update_top) obj->callbacks.print =
      &print_running_processes;
#elif (defined(__APPLE__) && defined(__MACH__))
  END OBJ(running_processes, &update_running_processes) obj->callbacks.print =
      &print_running_processes;
  END OBJ(threads, &update_threads) obj->callbacks.print = &print_threads;
  END OBJ(running_threads, &update_running_threads) obj->callbacks.print =
      &print_running_threads;
#else
  END OBJ(running_processes, &update_running_processes) obj->callbacks.print =
      &print_running_processes;
#endif
#endif /* __linux__ */
  END OBJ(shadecolor, nullptr)
#ifdef BUILD_X11
      obj->data.l =
      arg != nullptr ? get_x11_color(arg) : default_shade_color.get(*state);
  obj->callbacks.print = &new_bg;
#endif /* BUILD_X11 */
  END OBJ(outlinecolor, nullptr)
#ifdef BUILD_X11
      obj->data.l =
      arg != nullptr ? get_x11_color(arg) : default_outline_color.get(*state);
  obj->callbacks.print = &new_outline;
#endif /* BUILD_X11 */
  END OBJ(stippled_hr, nullptr)
#ifdef BUILD_X11
      scan_stippled_hr(obj, arg);
  obj->callbacks.print = &new_stippled_hr;
#endif /* BUILD_X11 */
  END OBJ(swap, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_swap;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(swapfree, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_swapfree;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(swapmax, &update_meminfo) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_swapmax;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ(swapperc, &update_meminfo) obj->callbacks.percentage =
      &swap_percentage;
  END OBJ(swapbar, &update_meminfo) scan_bar(obj, arg, 1);
  obj->callbacks.barval = &swap_barval;
  /* XXX: swapgraph, swapgauge? */
  END OBJ(sysname, nullptr) obj->callbacks.print = &print_sysname;
  END OBJ(time, nullptr) scan_time(obj, arg);
  obj->callbacks.print = &print_time;
  obj->callbacks.free = &free_time;
  END OBJ(utime, nullptr) scan_time(obj, arg);
  obj->callbacks.print = &print_utime;
  obj->callbacks.free = &free_time;
  END OBJ(tztime, nullptr) scan_tztime(obj, arg);
  obj->callbacks.print = &print_tztime;
  obj->callbacks.free = &free_tztime;
#ifdef BUILD_ICAL
  END OBJ_ARG(ical, 0, "ical requires arguments")
      parse_ical_args(obj, arg, free_at_crash, s);
  obj->callbacks.print = &print_ical;
  obj->callbacks.free = &free_ical;
#endif
#ifdef BUILD_IRC
  END OBJ_ARG(irc, 0, "irc requires arguments") parse_irc_args(obj, arg);
  obj->callbacks.print = &print_irc;
  obj->callbacks.free = &free_irc;
#endif
#ifdef BUILD_ICONV
  END OBJ_ARG(iconv_start, 0, "Iconv requires arguments")
      init_iconv_start(obj, free_at_crash, arg);
  obj->callbacks.print = &print_iconv_start;
  obj->callbacks.free = &free_iconv;
  END OBJ(iconv_stop, 0) init_iconv_stop();
  obj->callbacks.print = &print_iconv_stop;
#endif
  END OBJ(totaldown, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_totaldown;
  END OBJ(totalup, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_totalup;
  END OBJ(updates, nullptr) obj->callbacks.print = &print_updates;
  END OBJ_IF(if_updatenr, nullptr) obj->data.i = arg != nullptr ? atoi(arg) : 0;
  if (obj->data.i == 0) {
    CRIT_ERR(obj, free_at_crash,
             "if_updatenr needs a number above 0 as argument");
  }
  set_updatereset(obj->data.i > get_updatereset() ? obj->data.i
                                                  : get_updatereset());
  obj->callbacks.iftest = &updatenr_iftest;
  END OBJ(alignr, nullptr) obj->data.l = arg != nullptr ? atoi(arg) : 1;
  obj->callbacks.print = &new_alignr;
  END OBJ(alignc, nullptr) obj->data.l = arg != nullptr ? atoi(arg) : 0;
  obj->callbacks.print = &new_alignc;
  END OBJ(upspeed, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_upspeed;
  END OBJ(upspeedf, &update_net_stats)
      parse_net_stat_arg(obj, arg, free_at_crash);
  obj->callbacks.print = &print_upspeedf;
#ifdef BUILD_X11
  END OBJ(upspeedgraph, &update_net_stats)
      parse_net_stat_graph_arg(obj, arg, free_at_crash);
  obj->callbacks.graphval = &upspeedgraphval;
#endif
  END OBJ(uptime_short, &update_uptime) obj->callbacks.print =
      &print_uptime_short;
  END OBJ(uptime, &update_uptime) obj->callbacks.print = &print_uptime;
#if defined(__linux__)
  END OBJ(user_names, &update_users) obj->callbacks.print = &print_user_names;
  obj->callbacks.free = &free_user_names;
  END OBJ(user_times, &update_users) obj->callbacks.print = &print_user_times;
  obj->callbacks.free = &free_user_times;
  END OBJ_ARG(user_time, 0, "user time needs a console name as argument")
      obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_user_time;
  obj->callbacks.free = &free_user_time;
  END OBJ(user_terms, &update_users) obj->callbacks.print = &print_user_terms;
  obj->callbacks.free = &free_user_terms;
  END OBJ(user_number, &update_users) obj->callbacks.print = &print_user_number;
  END OBJ(gw_iface, &update_gateway_info) obj->callbacks.print =
      &print_gateway_iface;
  obj->callbacks.free = &free_gateway_info;
  END OBJ_IF(if_gw, &update_gateway_info) obj->callbacks.iftest =
      &gateway_exists;
  obj->callbacks.free = &free_gateway_info;
  END OBJ(gw_ip, &update_gateway_info) obj->callbacks.print = &print_gateway_ip;
  obj->callbacks.free = &free_gateway_info;
  END OBJ(iface, &update_gateway_info2) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_gateway_iface2;
  obj->callbacks.free = &gen_free_opaque;
#endif /* __linux__ */
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
     defined(__DragonFly__) || defined(__OpenBSD__)) &&     \
    (defined(i386) || defined(__i386__))
  END OBJ(apm_adapter, 0) obj->callbacks.print = &print_apm_adapter;
  END OBJ(apm_battery_life, 0) obj->callbacks.print = &print_apm_battery_life;
  END OBJ(apm_battery_time, 0) obj->callbacks.print = &print_apm_battery_time;
#endif /* __FreeBSD__ */
  END OBJ(imap_unseen, nullptr) parse_imap_mail_args(obj, arg);
  obj->callbacks.print = &print_imap_unseen;
  obj->callbacks.free = &free_mail_obj;
  END OBJ(imap_messages, nullptr) parse_imap_mail_args(obj, arg);
  obj->callbacks.print = &print_imap_messages;
  obj->callbacks.free = &free_mail_obj;
  END OBJ(pop3_unseen, nullptr) parse_pop3_mail_args(obj, arg);
  obj->callbacks.print = &print_pop3_unseen;
  obj->callbacks.free = &free_mail_obj;
  END OBJ(pop3_used, nullptr) parse_pop3_mail_args(obj, arg);
  obj->callbacks.print = &print_pop3_used;
  obj->callbacks.free = &free_mail_obj;
#ifdef BUILD_IBM
  END OBJ_ARG(smapi, 0, "smapi needs an argument") obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_smapi;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_IF_ARG(if_smapi_bat_installed, 0,
                 "if_smapi_bat_installed needs an argument") obj->data.s =
      STRNDUP_ARG;
  obj->callbacks.iftest = &smapi_bat_installed;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_ARG(smapi_bat_perc, 0, "smapi_bat_perc needs an argument")
      obj->data.s = STRNDUP_ARG;
  obj->callbacks.percentage = &smapi_bat_percentage;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_ARG(smapi_bat_temp, 0, "smapi_bat_temp needs an argument")
      obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_smapi_bat_temp;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_ARG(smapi_bat_power, 0, "smapi_bat_power needs an argument")
      obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_smapi_bat_power;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_ARG(smapi_bat_bar, 0, "smapi_bat_bar needs an argument") int cnt;
  if (sscanf(arg, "%i %n", &obj->data.i, &cnt) <= 0) {
    NORM_ERR("first argument to smapi_bat_bar must be an integer value");
    obj->data.i = -1;
  } else
    arg = scan_bar(obj, arg + cnt, 100);
  obj->callbacks.barval = &smapi_bat_barval;
#endif /* BUILD_IBM */
#ifdef BUILD_MPD
#define mpd_set_maxlen(name)                       \
  if (arg) {                                       \
    int i;                                         \
    sscanf(arg, "%d", &i);                         \
    if (i > 0)                                     \
      obj->data.i = i + 1;                         \
    else                                           \
      NORM_ERR(#name ": invalid length argument"); \
  }
  END OBJ(mpd_artist, nullptr) mpd_set_maxlen(mpd_artist);
  obj->callbacks.print = &print_mpd_artist;
  END OBJ(mpd_albumartist, nullptr) mpd_set_maxlen(mpd_albumartist);
  obj->callbacks.print = &print_mpd_albumartist;
  END OBJ(mpd_title, nullptr) mpd_set_maxlen(mpd_title);
  obj->callbacks.print = &print_mpd_title;
  END OBJ(mpd_date, nullptr) mpd_set_maxlen(mpd_date);
  obj->callbacks.print = &print_mpd_date;
  END OBJ(mpd_random, nullptr) obj->callbacks.print = &print_mpd_random;
  END OBJ(mpd_repeat, nullptr) obj->callbacks.print = &print_mpd_repeat;
  END OBJ(mpd_elapsed, nullptr) obj->callbacks.print = &print_mpd_elapsed;
  END OBJ(mpd_length, nullptr) obj->callbacks.print = &print_mpd_length;
  END OBJ(mpd_track, nullptr) mpd_set_maxlen(mpd_track);
  obj->callbacks.print = &print_mpd_track;
  END OBJ(mpd_name, nullptr) mpd_set_maxlen(mpd_name);
  obj->callbacks.print = &print_mpd_name;
  END OBJ(mpd_file, nullptr) mpd_set_maxlen(mpd_file);
  obj->callbacks.print = &print_mpd_file;
  END OBJ(mpd_percent, nullptr) obj->callbacks.percentage = &mpd_percentage;
  END OBJ(mpd_album, nullptr) mpd_set_maxlen(mpd_album);
  obj->callbacks.print = &print_mpd_album;
  END OBJ(mpd_vol, nullptr) obj->callbacks.print = &print_mpd_vol;
  END OBJ(mpd_bitrate, nullptr) obj->callbacks.print = &print_mpd_bitrate;
  END OBJ(mpd_status, nullptr) obj->callbacks.print = &print_mpd_status;
  END OBJ(mpd_bar, nullptr) scan_bar(obj, arg, 1);
  obj->callbacks.barval = &mpd_barval;
  END OBJ(mpd_smart, nullptr) mpd_set_maxlen(mpd_smart);
  obj->callbacks.print = &print_mpd_smart;
  END OBJ_IF(if_mpd_playing, nullptr) obj->callbacks.iftest =
      &check_mpd_playing;
#undef mpd_set_maxlen
#endif /* BUILD_MPD */
#ifdef BUILD_MOC
  END OBJ(moc_state, nullptr) obj->callbacks.print = &print_moc_state;
  END OBJ(moc_file, nullptr) obj->callbacks.print = &print_moc_file;
  END OBJ(moc_title, nullptr) obj->callbacks.print = &print_moc_title;
  END OBJ(moc_artist, nullptr) obj->callbacks.print = &print_moc_artist;
  END OBJ(moc_song, nullptr) obj->callbacks.print = &print_moc_song;
  END OBJ(moc_album, nullptr) obj->callbacks.print = &print_moc_album;
  END OBJ(moc_totaltime, nullptr) obj->callbacks.print = &print_moc_totaltime;
  END OBJ(moc_timeleft, nullptr) obj->callbacks.print = &print_moc_timeleft;
  END OBJ(moc_curtime, nullptr) obj->callbacks.print = &print_moc_curtime;
  END OBJ(moc_bitrate, nullptr) obj->callbacks.print = &print_moc_bitrate;
  END OBJ(moc_rate, nullptr) obj->callbacks.print = &print_moc_rate;
#endif /* BUILD_MOC */
#ifdef BUILD_CMUS
  END OBJ(cmus_state, 0) obj->callbacks.print = &print_cmus_state;
  END OBJ(cmus_file, 0) obj->callbacks.print = &print_cmus_file;
  END OBJ(cmus_title, 0) obj->callbacks.print = &print_cmus_title;
  END OBJ(cmus_artist, 0) obj->callbacks.print = &print_cmus_artist;
  END OBJ(cmus_album, 0) obj->callbacks.print = &print_cmus_album;
  END OBJ(cmus_totaltime, 0) obj->callbacks.print = &print_cmus_totaltime;
  END OBJ(cmus_timeleft, 0) obj->callbacks.print = &print_cmus_timeleft;
  END OBJ(cmus_curtime, 0) obj->callbacks.print = &print_cmus_curtime;
  END OBJ(cmus_random, 0) obj->callbacks.print = &print_cmus_random;
  END OBJ(cmus_state, 0) obj->callbacks.print = &print_cmus_state;
  END OBJ(cmus_file, 0) obj->callbacks.print = &print_cmus_file;
  END OBJ(cmus_title, 0) obj->callbacks.print = &print_cmus_title;
  END OBJ(cmus_artist, 0) obj->callbacks.print = &print_cmus_artist;
  END OBJ(cmus_album, 0) obj->callbacks.print = &print_cmus_album;
  END OBJ(cmus_totaltime, 0) obj->callbacks.print = &print_cmus_totaltime;
  END OBJ(cmus_timeleft, 0) obj->callbacks.print = &print_cmus_timeleft;
  END OBJ(cmus_curtime, 0) obj->callbacks.print = &print_cmus_curtime;
  END OBJ(cmus_random, 0) obj->callbacks.print = &print_cmus_random;
  END OBJ(cmus_repeat, 0) obj->callbacks.print = &print_cmus_repeat;
  END OBJ(cmus_aaa, 0) obj->callbacks.print = &print_cmus_aaa;
  END OBJ(cmus_track, 0) obj->callbacks.print = &print_cmus_track;
  END OBJ(cmus_genre, 0) obj->callbacks.print = &print_cmus_genre;
  END OBJ(cmus_date, 0) obj->callbacks.print = &print_cmus_date;
  END OBJ(cmus_progress, 0) scan_bar(obj, arg, 1);
  obj->callbacks.barval = &cmus_progress;
  END OBJ(cmus_percent, 0) obj->callbacks.percentage = &cmus_percent;
#endif /* BUILD_CMUS */
#ifdef BUILD_XMMS2
  END OBJ(xmms2_artist, &update_xmms2) obj->callbacks.print =
      &print_xmms2_artist;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_album, &update_xmms2) obj->callbacks.print = &print_xmms2_album;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_title, &update_xmms2) obj->callbacks.print = &print_xmms2_title;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_genre, &update_xmms2) obj->callbacks.print = &print_xmms2_genre;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_comment, &update_xmms2) obj->callbacks.print =
      &print_xmms2_comment;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_url, &update_xmms2) obj->callbacks.print = &print_xmms2_url;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_tracknr, &update_xmms2) obj->callbacks.print =
      &print_xmms2_tracknr;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_bitrate, &update_xmms2) obj->callbacks.print =
      &print_xmms2_bitrate;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_date, &update_xmms2) obj->callbacks.print = &print_xmms2_date;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_id, &update_xmms2) obj->callbacks.print = &print_xmms2_id;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_duration, &update_xmms2) obj->callbacks.print =
      &print_xmms2_duration;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_elapsed, &update_xmms2) obj->callbacks.print =
      &print_xmms2_elapsed;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_size, &update_xmms2) obj->callbacks.print = &print_xmms2_size;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_status, &update_xmms2) obj->callbacks.print =
      &print_xmms2_status;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_percent, &update_xmms2) obj->callbacks.print =
      &print_xmms2_percent;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_bar, &update_xmms2) scan_bar(obj, arg, 1);
  obj->callbacks.barval = &xmms2_barval;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_smart, &update_xmms2) obj->callbacks.print = &print_xmms2_smart;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_playlist, &update_xmms2) obj->callbacks.print =
      &print_xmms2_playlist;
  obj->callbacks.free = &free_xmms2;
  END OBJ(xmms2_timesplayed, &update_xmms2) obj->callbacks.print =
      &print_xmms2_timesplayed;
  obj->callbacks.free = &free_xmms2;
  END OBJ_IF(if_xmms2_connected, &update_xmms2) obj->callbacks.iftest =
      &if_xmms2_connected;
  obj->callbacks.free = &free_xmms2;
#endif /* BUILD_XMMS2 */
#ifdef BUILD_AUDACIOUS
  END OBJ(audacious_status, 0) obj->callbacks.print = &print_audacious_status;
  END OBJ_ARG(audacious_title, 0, "audacious_title needs an argument")
      sscanf(arg, "%d", &obj->data.i);
  if (obj->data.i > 0) {
    ++obj->data.i;
  } else {
    CRIT_ERR(obj, free_at_crash, "audacious_title: invalid length argument");
  }
  obj->callbacks.print = &print_audacious_title;
  END OBJ(audacious_length, 0) obj->callbacks.print = &print_audacious_length;
  END OBJ(audacious_length_seconds, 0) obj->callbacks.print =
      &print_audacious_length_seconds;
  END OBJ(audacious_position, 0) obj->callbacks.print =
      &print_audacious_position;
  END OBJ(audacious_position_seconds, 0) obj->callbacks.print =
      &print_audacious_position_seconds;
  END OBJ(audacious_bitrate, 0) obj->callbacks.print = &print_audacious_bitrate;
  END OBJ(audacious_frequency, 0) obj->callbacks.print =
      &print_audacious_frequency;
  END OBJ(audacious_channels, 0) obj->callbacks.print =
      &print_audacious_channels;
  END OBJ(audacious_filename, 0) obj->callbacks.print =
      &print_audacious_filename;
  END OBJ(audacious_playlist_length, 0) obj->callbacks.print =
      &print_audacious_playlist_length;
  END OBJ(audacious_playlist_position, 0) obj->callbacks.print =
      &print_audacious_playlist_position;
  END OBJ(audacious_main_volume, 0) obj->callbacks.print =
      &print_audacious_main_volume;
  END OBJ(audacious_bar, 0) scan_bar(obj, arg, 1);
  obj->callbacks.barval = &audacious_barval;
#endif /* BUILD_AUDACIOUS */
#ifdef BUILD_CURL
  END OBJ_ARG(curl, 0, "curl needs arguments: <uri> <interval in minutes>")
      curl_parse_arg(obj, arg);
  obj->callbacks.print = &curl_print;
  obj->callbacks.free = &curl_obj_free;
  END OBJ(github_notifications, 0) obj->callbacks.print = &print_github;
#endif /* BUILD_CURL */
#ifdef BUILD_RSS
  END OBJ_ARG(rss, 0,
              "rss needs arguments: <uri> <interval in minutes> <action> "
              "[act_par] [spaces in front]") rss_scan_arg(obj, arg);
  obj->callbacks.print = &rss_print_info;
  obj->callbacks.free = &rss_free_obj_info;
#endif /* BUILD_RSS */
#ifdef BUILD_WEATHER_METAR
  END OBJ_ARG(weather, 0, "weather still needs to written...")
      obj->callbacks.print = &print_weather;
#endif /* BUILD_WEATHER_METAR */
  END OBJ_ARG(lua, nullptr,
              "lua needs arguments: <function name> [function parameters]")
      obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_lua;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_ARG(
      lua_parse, nullptr,
      "lua_parse needs arguments: <function name> [function parameters]")
      obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_lua_parse;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_ARG(lua_bar, nullptr,
              "lua_bar needs arguments: <height>,<width> <function name> "
              "[function parameters]") arg = scan_bar(obj, arg, 100);
  if (arg != nullptr) {
    obj->data.s = STRNDUP_ARG;
  } else {
    CRIT_ERR(obj, free_at_crash,
             "lua_bar needs arguments: <height>,<width> <function name> "
             "[function parameters]");
  }
  obj->callbacks.barval = &lua_barval;
  obj->callbacks.free = &gen_free_opaque;
#ifdef BUILD_X11
  END OBJ_ARG(
      lua_graph, nullptr,
      "lua_graph needs arguments: <function name> [height],[width] [gradient "
      "colour 1] [gradient colour 2] [scale] [-t] [-l]") char *buf = nullptr;
  buf = scan_graph(obj, arg, 100);
  if (buf != nullptr) {
    obj->data.s = buf;
  } else {
    CRIT_ERR(obj, free_at_crash,
             "lua_graph needs arguments: <function name> [height],[width] "
             "[gradient colour 1] [gradient colour 2] [scale] [-t] [-l]");
  }
  obj->callbacks.graphval = &lua_barval;
  obj->callbacks.free = &gen_free_opaque;
  END OBJ_ARG(lua_gauge, nullptr,
              "lua_gauge needs arguments: <height>,<width> <function name> "
              "[function parameters]") arg = scan_gauge(obj, arg, 100);
  if (arg != nullptr) {
    obj->data.s = STRNDUP_ARG;
  } else {
    CRIT_ERR(obj, free_at_crash,
             "lua_gauge needs arguments: <height>,<width> <function name> "
             "[function parameters]");
  }
  obj->callbacks.gaugeval = &lua_barval;
  obj->callbacks.free = &gen_free_opaque;
#endif /* BUILD_X11 */
#ifdef BUILD_HDDTEMP
  END OBJ(hddtemp, &update_hddtemp) if (arg) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_hddtemp;
  obj->callbacks.free = &free_hddtemp;
#endif /* BUILD_HDDTEMP */
#ifdef BUILD_PORT_MONITORS
  END OBJ_ARG(tcp_portmon, &tcp_portmon_update, "tcp_portmon: needs arguments")
      tcp_portmon_init(obj, arg);
  obj->callbacks.print = &tcp_portmon_action;
  obj->callbacks.free = &tcp_portmon_free;
#endif /* BUILD_PORT_MONITORS */
  END OBJ(entropy_avail, &update_entropy) obj->callbacks.print =
      &print_entropy_avail;
  END OBJ(entropy_perc, &update_entropy) obj->callbacks.percentage =
      &entropy_percentage;
  END OBJ(entropy_poolsize, &update_entropy) obj->callbacks.print =
      &print_entropy_poolsize;
  END OBJ(entropy_bar, &update_entropy) scan_bar(obj, arg, 1);
  obj->callbacks.barval = &entropy_barval;
  END OBJ_ARG(blink, nullptr, "blink needs a argument") obj->sub =
      static_cast<text_object *>(malloc(sizeof(struct text_object)));
  extract_variable_text_internal(obj->sub, arg);
  obj->callbacks.print = &print_blink;
  END OBJ_ARG(to_bytes, nullptr, "to_bytes needs a argument") obj->sub =
      static_cast<text_object *>(malloc(sizeof(struct text_object)));
  extract_variable_text_internal(obj->sub, arg);
  obj->callbacks.print = &print_to_bytes;
#ifdef BUILD_CURL
  END OBJ_ARG(stock, 0, "stock needs arguments") stock_parse_arg(obj, arg);
  obj->callbacks.print = &print_stock;
  obj->callbacks.free = &free_stock;
#endif /* BUILD_CURL */
  END OBJ(scroll, nullptr)
#ifdef BUILD_X11
  /* allocate a follower to reset any color changes */
#endif /* BUILD_X11 */
      parse_scroll_arg(obj, arg, free_at_crash, s);
  obj->callbacks.print = &print_scroll;
  obj->callbacks.free = &free_scroll;
  END OBJ(combine, nullptr) try {
    parse_combine_arg(obj, arg);
  } catch (combine_needs_2_args_error &e) {
    free(obj);
    throw obj_create_error(e.what());
  }
  obj->callbacks.print = &print_combine;
  obj->callbacks.free = &free_combine;
#ifdef BUILD_NVIDIA
  END OBJ_ARG(
      nvidia, 0,
      "nvidia needs an argument") if (set_nvidia_query(obj, arg, NONSPECIAL)) {
    CRIT_ERR(obj, free_at_crash,
             "nvidia: invalid argument"
             " specified: '%s'",
             arg);
  }
  obj->callbacks.print = &print_nvidia_value;
  obj->callbacks.free = &free_nvidia;
  END OBJ_ARG(
      nvidiabar, 0,
      "nvidiabar needs an argument") if (set_nvidia_query(obj, arg, BAR)) {
    CRIT_ERR(obj, free_at_crash,
             "nvidiabar: invalid argument"
             " specified: '%s'",
             arg);
  }
  obj->callbacks.barval = &get_nvidia_barval;
  obj->callbacks.free = &free_nvidia;
  END OBJ_ARG(
      nvidiagraph, 0,
      "nvidiagraph needs an argument") if (set_nvidia_query(obj, arg, GRAPH)) {
    CRIT_ERR(obj, free_at_crash,
             "nvidiagraph: invalid argument"
             " specified: '%s'",
             arg);
  }
  obj->callbacks.graphval = &get_nvidia_barval;
  obj->callbacks.free = &free_nvidia;
  END OBJ_ARG(
      nvidiagauge, 0,
      "nvidiagauge needs an argument") if (set_nvidia_query(obj, arg, GAUGE)) {
    CRIT_ERR(obj, free_at_crash,
             "nvidiagauge: invalid argument"
             " specified: '%s'",
             arg);
  }
  obj->callbacks.gaugeval = &get_nvidia_barval;
  obj->callbacks.free = &free_nvidia;
#endif /* BUILD_NVIDIA */
#ifdef BUILD_APCUPSD
  END OBJ_ARG(
      apcupsd, &update_apcupsd,
      "apcupsd needs arguments: <host> <port>") if (apcupsd_scan_arg(arg) !=
                                                    0) {
    CRIT_ERR(obj, free_at_crash, "apcupsd needs arguments: <host> <port>");
  }
  obj->callbacks.print = &gen_print_nothing;
  END OBJ(apcupsd_name, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_name;
  END OBJ(apcupsd_model, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_model;
  END OBJ(apcupsd_upsmode, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_upsmode;
  END OBJ(apcupsd_cable, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_cable;
  END OBJ(apcupsd_status, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_status;
  END OBJ(apcupsd_linev, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_linev;
  END OBJ(apcupsd_load, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_load;
  END OBJ(apcupsd_loadbar, &update_apcupsd) scan_bar(obj, arg, 100);
  obj->callbacks.barval = &apcupsd_loadbarval;
#ifdef BUILD_X11
  END OBJ(apcupsd_loadgraph, &update_apcupsd) char *buf = nullptr;
  buf = scan_graph(obj, arg, 100);
  free_and_zero(buf);
  obj->callbacks.graphval = &apcupsd_loadbarval;
  END OBJ(apcupsd_loadgauge, &update_apcupsd) scan_gauge(obj, arg, 100);
  obj->callbacks.gaugeval = &apcupsd_loadbarval;
#endif /* BUILD_X11 */
  END OBJ(apcupsd_charge, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_charge;
  END OBJ(apcupsd_timeleft, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_timeleft;
  END OBJ(apcupsd_temp, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_temp;
  END OBJ(apcupsd_lastxfer, &update_apcupsd) obj->callbacks.print =
      &print_apcupsd_lastxfer;
#endif /* BUILD_APCUPSD */
#ifdef BUILD_JOURNAL
  END OBJ_ARG(journal, 0, "journal needs arguments")
      init_journal("journal", arg, obj, free_at_crash);
  obj->callbacks.print = &print_journal;
  obj->callbacks.free = &free_journal;
#endif /* BUILD_JOURNAL */
#ifdef BUILD_PULSEAUDIO
  END OBJ_IF(if_pa_sink_muted, 0) obj->callbacks.iftest = &puau_muted;
  obj->callbacks.free = &free_pulseaudio;
  init_pulseaudio(obj);
  END OBJ(pa_sink_description, 0) obj->callbacks.print =
      &print_puau_sink_description;
  obj->callbacks.free = &free_pulseaudio;
  init_pulseaudio(obj);
  END OBJ(pa_sink_active_port_name, 0) obj->callbacks.print =
      &print_puau_sink_active_port_name;
  obj->callbacks.free = &free_pulseaudio;
  init_pulseaudio(obj);
  END OBJ(pa_sink_active_port_description, 0) obj->callbacks.print =
      &print_puau_sink_active_port_description;
  obj->callbacks.free = &free_pulseaudio;
  init_pulseaudio(obj);
  END OBJ(pa_sink_volume, 0) obj->callbacks.percentage = &puau_vol;
  obj->callbacks.free = &free_pulseaudio;
  init_pulseaudio(obj);
  END OBJ(pa_sink_volumebar, 0) scan_bar(obj, arg, 1);
  init_pulseaudio(obj);
  obj->callbacks.barval = &puau_volumebarval;
  obj->callbacks.free = &free_pulseaudio;
  END OBJ(pa_card_active_profile, 0) obj->callbacks.print =
      &print_puau_card_active_profile;
  obj->callbacks.free = &free_pulseaudio;
  init_pulseaudio(obj);
  END OBJ(pa_card_name, 0) obj->callbacks.print = &print_puau_card_name;
  obj->callbacks.free = &free_pulseaudio;
  init_pulseaudio(obj);
#endif /* BUILD_PULSEAUDIO */
  END {
    auto *buf = static_cast<char *>(malloc(text_buffer_size.get(*state)));

    NORM_ERR("unknown variable '$%s'", s);
    snprintf(buf, text_buffer_size.get(*state), "${%s}", s);
    obj_be_plain_text(obj, buf);
    free(buf);
  }
#undef OBJ
#undef OBJ_IF
#undef OBJ_ARG
#undef OBJ_IF_ARG
#undef __OBJ_HEAD
#undef __OBJ_IF
#undef __OBJ_ARG
#undef END

  return obj;
}

/*
 * - assumes that *string is '#'
 * - removes the part from '#' to the end of line ('\n' or '\0')
 * - it removes the '\n'
 * - copies the last char into 'char *last' argument, which should be a pointer
 *   to a char rather than a string.
 */
static size_t remove_comment(char *string, char *last) {
  char *end = string;
  while (*end != '\0' && *end != '\n') { ++end; }
  if (last != nullptr) { *last = *end; }
  if (*end == '\n') { end++; }
  strfold(string, end - string);
  return end - string;
}

size_t remove_comments(char *string) {
  char *curplace;
  size_t folded = 0;
  for (curplace = string; *curplace != 0; curplace++) {
    if (*curplace == '\\' && *(curplace + 1) == '#') {
      // strcpy can't be used for overlapping strings
      strfold(curplace, 1);
      folded += 1;
    } else if (*curplace == '#') {
      folded += remove_comment(curplace, nullptr);
    }
  }
  return folded;
}

int extract_variable_text_internal(struct text_object *retval,
                                   const char *const_p) {
  struct text_object *obj;
  char *p, *s, *orig_p;
  long line;
  void *ifblock_opaque = nullptr;
  char *tmp_p;
  char *arg = nullptr;
  size_t len = 0;

  p = strndup(const_p, max_user_text.get(*state) - 1);
  while (text_contains_templates(p) != 0) {
    char *tmp;
    tmp = find_and_replace_templates(p);
    free(p);
    p = tmp;
  }
  s = orig_p = p;

  if (static_cast<int>(strcmp(p, const_p) != 0) != 0) {
    DBGP2("replaced all templates in text: input is\n'%s'\noutput is\n'%s'",
          const_p, p);
  } else {
    DBGP2("no templates to replace");
  }

  memset(retval, 0, sizeof(struct text_object));

  line = global_text_lines;

  while (*p != 0) {
    if (*p == '\n') { line++; }
    if (*p == '$') {
      *p = '\0';
      obj = create_plain_text(s);
      if (obj != nullptr) { append_object(retval, obj); }
      *p = '$';
      p++;
      s = p;

      if (*p != '$') {
        auto *buf = static_cast<char *>(malloc(text_buffer_size.get(*state)));
        const char *var;

        /* variable is either $foo or ${foo} */
        if (*p == '{') {
          unsigned int brl = 1, brr = 0;

          p++;
          s = p;
          while ((*p != 0) && brl != brr) {
            if (*p == '{') { brl++; }
            if (*p == '}') { brr++; }
            p++;
          }
          p--;
        } else {
          s = p;
          if (*p == '#') { p++; }
          while ((*p != 0) && ((isalnum(static_cast<unsigned char>(*p)) != 0) ||
                               *p == '_')) {
            p++;
          }
        }

        /* copy variable to buffer */
        len = (p - s > static_cast<int>(text_buffer_size.get(*state)) - 1)
                  ? static_cast<int>(text_buffer_size.get(*state)) - 1
                  : (p - s);
        strncpy(buf, s, len);
        buf[len] = '\0';

        if (*p == '}') { p++; }
        s = p;

        /* search for variable in environment */

        var = getenv(buf);
        if (var != nullptr) {
          obj = create_plain_text(var);
          if (obj != nullptr) { append_object(retval, obj); }
          free(buf);
          continue;
        }

        /* if variable wasn't found in environment, use some special */

        arg = nullptr;

        /* split arg */
        if (strchr(buf, ' ') != nullptr) {
          arg = strchr(buf, ' ');
          *arg = '\0';
          arg++;
          while (isspace(static_cast<unsigned char>(*arg)) != 0) { arg++; }
          if (*arg == 0) { arg = nullptr; }
        }

        /* lowercase variable name */
        tmp_p = buf;
        while (*tmp_p != 0) {
          *tmp_p = tolower(static_cast<unsigned char>(*tmp_p));
          tmp_p++;
        }

        try {
          obj = construct_text_object(buf, arg, line, &ifblock_opaque, orig_p);
        } catch (obj_create_error &e) {
          free(buf);
          free(orig_p);
          throw;
        }
        if (obj != nullptr) { append_object(retval, obj); }
        free(buf);
        continue;
      }
      obj = create_plain_text("$");
      s = p + 1;
      if (obj != nullptr) { append_object(retval, obj); }

    } else if (*p == '\\' && *(p + 1) == '#') {
      strfold(p, 1);
    } else if (*p == '#') {
      char c;
      if ((remove_comment(p, &c) != 0u) && p >= orig_p && c == '\n') {
        /* if remove_comment removed a newline, we need to 'back up' with p */
        p--;
      }
    }
    p++;
  }
  obj = create_plain_text(s);
  if (obj != nullptr) { append_object(retval, obj); }

  if (ifblock_stack_empty(&ifblock_opaque) == 0) {
    NORM_ERR("one or more $endif's are missing");
  }

  free(orig_p);
  return 0;
}

void extract_object_args_to_sub(struct text_object *obj, const char *args) {
  obj->sub =
      static_cast<struct text_object *>(malloc(sizeof(struct text_object)));
  memset(obj->sub, 0, sizeof(struct text_object));
  extract_variable_text_internal(obj->sub, args);
}

/* Frees the list of text objects root points to. */
void free_text_objects(struct text_object *root) {
  struct text_object *obj;

  if ((root != nullptr) && (root->prev != nullptr)) {
    for (obj = root->prev; obj != nullptr; obj = root->prev) {
      root->prev = obj->prev;
      if (obj->callbacks.free != nullptr) { (*obj->callbacks.free)(obj); }
      free_text_objects(obj->sub);
      free_and_zero(obj->sub);
      free_and_zero(obj->special_data);
      delete obj->cb_handle;

      free(obj);
    }
  }
}
