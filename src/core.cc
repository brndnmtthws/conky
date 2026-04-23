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

#include "config.h"

/* local headers */
#include "common.h"
#include "content/algebra.h"
#include "core.h"
#include "parse/variables.hh"

#include "build.h"
#include "lua/colour-settings.hh"
#include "content/colours.hh"
#include "content/combine.h"
#include "content/text_object.h"
#include "data/entropy.h"
#include "data/exec.h"
#include "data/hardware/bsdapm.h"
#include "data/hardware/diskio.h"
#ifdef BUILD_IMLIB2
#include "conky-imlib2.h"
#endif /* BUILD_IMLIB2 */
#ifdef BUILD_MYSQL
#include "data/mysql.h"
#endif /* BUILD_MYSQL */
#ifdef BUILD_ICAL
#include "data/ical.h"
#endif /* BUILD_ICAL */
#ifdef BUILD_IRC
#include "data/network/irc.h"
#endif /* BUILD_IRC */
#ifdef BUILD_GUI
#include "lua/fonts.h"
#include "output/gui.h"
#endif /* BUILD_GUI */
#include "data/fs.h"
#ifdef BUILD_IBM
#include "data/hardware/smapi.h"
#endif /* BUILD_IBM */
#ifdef BUILD_ICONV
#include "data/iconv_tools.h"
#endif /* BUILD_ICONV */
#include "data/network/mail.h"
#include "data/network/mboxscan.h"
#include "logging.h"
#include "lua/llua.h"
#include "output/nc.h"
#ifdef BUILD_NVIDIA
#include "data/hardware/nvidia.h"
#endif /* BUILD_NVIDIA */
#include <inttypes.h>
#include "content/scroll.h"
#include "content/specials.h"
#include "content/temphelper.h"
#include "content/template.h"
#include "data/hardware/cpu.h"
#include "data/network/read_tcpip.h"
#include "data/tailhead.h"
#include "data/timeinfo.h"
#include "data/user.h"
#include "data/users.h"
#ifdef BUILD_CURL
#include "data/network/ccurl_thread.h"
#endif /* BUILD_CURL */
#ifdef BUILD_RSS
#include "data/network/rss.h"
#endif /* BUILD_RSS */
#ifdef BUILD_JOURNAL
#include "data/os/journal.h"
#endif /* BUILD_JOURNAL */
#ifdef BUILD_INTEL_BACKLIGHT
#include "data/hardware/intel_backlight.h"
#endif /* BUILD_INTEL_BACKLIGHT */

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "data/os/linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "data/os/freebsd.h"
#elif defined(__DragonFly__)
#include "data/os/dragonfly.h"
#elif defined(__OpenBSD__)
#include "data/os/openbsd.h"
#elif defined(__APPLE__) && defined(__MACH__)
#include "data/os/darwin.h"
#endif

#define STRNDUP_ARG strndup(arg ? arg : "", text_buffer_size.get(*state))

#include <cctype>
#include <cstring>

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
  auto *result = conky::text_object::construct_text_object(s, arg, line,
                                                           ifblock_opaque,
                                                           free_at_crash);
  if (result != nullptr) { return result; }

  // struct text_object *obj = new_text_object();
  struct text_object *obj = new_text_object_internal();

  obj->line = line;

/* helper defines for internal use only */
#define __OBJ_HEAD(a, n) \
  if (!strcmp(s, #a)) {  \
    obj->cb_handle = create_cb_handle(n);
#define __OBJ_IF obj_be_ifblock_if(ifblock_opaque, obj)
#define __OBJ_ARG(...)                              \
  if (!arg) {                                       \
    free(s);                                        \
    CRIT_ERR_FREE(obj, free_at_crash, __VA_ARGS__); \
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

#ifdef BUILD_GUI
  if (s[0] == '#') {
    obj->data.l = parse_color(s).to_argb32();
    obj->callbacks.print = &new_fg;
  } else
#endif /* BUILD_GUI */
  OBJ_ARG(read_tcp, nullptr,
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
  if (!arg || strlen(arg) >= 3 || strtol(&arg[0], nullptr, 10) == 0 ||
      (unsigned int)strtol(&arg[0], nullptr, 10) > info.cpu_count) {
    obj->data.i = 1;
    LOG_WARNING("invalid CPU number '{}', falling back to CPU 1", arg ? arg : "(null)");
  } else {
    obj->data.i = strtol(&arg[0], nullptr, 10);
  }
  obj->callbacks.print = &print_voltage_mv;
  END OBJ(voltage_v, 0) get_cpu_count();
  if (!arg || strlen(arg) >= 3 || strtol(&arg[0], nullptr, 10) == 0 ||
      (unsigned int)strtol(&arg[0], nullptr, 10) > info.cpu_count) {
    obj->data.i = 1;
    LOG_WARNING("invalid CPU number '{}', falling back to CPU 1", arg ? arg : "(null)");
  } else {
    obj->data.i = strtol(&arg[0], nullptr, 10);
  }
  obj->callbacks.print = &print_voltage_v;

#endif /* __linux__ */

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
  LOG_TRACE("adding $cpu for CPU {}", obj->data.i);
#ifdef BUILD_GUI
  END OBJ(cpugauge, &update_cpu_usage) get_cpu_count();
  SCAN_CPU(arg, obj->data.i);
  scan_gauge(obj, arg, 1);
  obj->callbacks.gaugeval = &cpu_barval;
  obj->callbacks.free = &free_cpu;
  LOG_TRACE("adding $cpugauge for CPU {}", obj->data.i);
#endif
  END OBJ(cpubar, &update_cpu_usage) get_cpu_count();
  SCAN_CPU(arg, obj->data.i);
  scan_bar(obj, arg, 1);
  obj->callbacks.barval = &cpu_barval;
  obj->callbacks.free = &free_cpu;
  LOG_TRACE("adding $cpubar for CPU {}", obj->data.i);
#ifdef BUILD_GUI
  END OBJ(cpugraph, &update_cpu_usage) get_cpu_count();
  SCAN_CPU(arg, obj->data.i);
  scan_graph(obj, arg, 1, FALSE);
  LOG_TRACE("adding $cpugraph for CPU {}", obj->data.i);
  obj->callbacks.graphval = &cpu_barval;
  obj->callbacks.free = &free_cpu;
#endif /* BUILD_GUI */
  END OBJ(diskio, &update_diskio) parse_diskio_arg(obj, arg);
  obj->callbacks.print = &print_diskio;
  END OBJ(diskio_read, &update_diskio) parse_diskio_arg(obj, arg);
  obj->callbacks.print = &print_diskio_read;
  END OBJ(diskio_write, &update_diskio) parse_diskio_arg(obj, arg);
  obj->callbacks.print = &print_diskio_write;
#ifdef BUILD_GUI
  END OBJ(diskiograph, &update_diskio) parse_diskiograph_arg(obj, arg);
  obj->callbacks.graphval = &diskiographval;
  END OBJ(diskiograph_read, &update_diskio) parse_diskiograph_arg(obj, arg);
  obj->callbacks.graphval = &diskiographval_read;
  END OBJ(diskiograph_write, &update_diskio) parse_diskiograph_arg(obj, arg);
  obj->callbacks.graphval = &diskiographval_write;
#endif /* BUILD_GUI */
  END OBJ(conky_version, nullptr) obj_be_plain_text(obj, VERSION);
  END OBJ(conky_build_arch, nullptr) obj_be_plain_text(obj, BUILD_ARCH);
  END OBJ(else, nullptr) obj_be_ifblock_else(ifblock_opaque, obj);
  obj->callbacks.iftest = &gen_false_iftest;
  END OBJ(endif, nullptr) obj_be_ifblock_endif(ifblock_opaque, obj);
  obj->callbacks.print = &gen_print_nothing;
#if defined(BUILD_IMLIB2) && defined(BUILD_GUI)
  END OBJ(image, nullptr) obj->data.s = STRNDUP_ARG;
  obj->callbacks.print = &print_image_callback;
  obj->callbacks.free = &gen_free_opaque;
#endif /* BUILD_IMLIB2 */
#ifdef BUILD_MYSQL
  END OBJ_ARG(mysql, 0, "mysql needs a query") obj->data.s = strdup(arg);
  obj->callbacks.print = &print_mysql;
#endif /* BUILD_MYSQL */
#ifdef BUILD_X11
  END OBJ(key_num_lock, 0) obj->callbacks.print = &print_key_num_lock;
  END OBJ(key_caps_lock, 0) obj->callbacks.print = &print_key_caps_lock;
  END OBJ(key_scroll_lock, 0) obj->callbacks.print = &print_key_scroll_lock;
  END OBJ(keyboard_layout, 0) obj->callbacks.print = &print_keyboard_layout;
  END OBJ(mouse_speed, 0) obj->callbacks.print = &print_mouse_speed;
#endif /* BUILD_GUI */

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
#ifdef BUILD_GUI
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
#endif /* BUILD_GUI */
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
#ifdef BUILD_GUI
  END OBJ(hr, nullptr) obj->data.l =
      arg != nullptr ? strtol(arg, nullptr, 10) : 1;
  obj->callbacks.print = &new_hr;
#endif /* BUILD_GUI */
  END OBJ(offset, nullptr) obj->data.l =
      arg != nullptr ? strtol(arg, nullptr, 10) : 1;
  obj->callbacks.print = &new_offset;
  END OBJ(voffset, nullptr) obj->data.l =
      arg != nullptr ? strtol(arg, nullptr, 10) : 1;
  obj->callbacks.print = &new_voffset;
  END OBJ(save_coordinates, nullptr) obj->data.l =
      arg != nullptr ? strtol(arg, nullptr, 10) : 0;
  obj->callbacks.print = &new_save_coordinates;
  END OBJ_ARG(goto, nullptr, "goto needs arguments") obj->data.l =
      strtol(arg, nullptr, 10);
  obj->callbacks.print = &new_goto;
#ifdef BUILD_GUI
  END OBJ(tab, nullptr) scan_tab(obj, arg);
  obj->callbacks.print = &new_tab;
#endif /* BUILD_GUI */
  END OBJ_ARG(tail, nullptr, "tail needs arguments")
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
  END OBJ_IF_ARG(if_match, nullptr, "if_match needs arguments") obj->sub =
      static_cast<text_object *>(malloc(sizeof(struct text_object)));
  extract_variable_text_internal(obj->sub, arg);
  obj->callbacks.iftest = &check_if_match;
#if defined(__linux__) || defined(__FreeBSD__)
  END OBJ_IF_ARG(if_mounted, 0, "if_mounted needs an argument") obj->data.s =
      STRNDUP_ARG;
  obj->callbacks.iftest = &check_mount;
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
#ifdef BUILD_GUI
  END OBJ(monitor, nullptr) obj->callbacks.print = &print_monitor;
  END OBJ(monitor_number, nullptr) obj->callbacks.print = &print_monitor_number;
  END OBJ(desktop, nullptr) obj->callbacks.print = &print_desktop;
  END OBJ(desktop_number, nullptr) obj->callbacks.print = &print_desktop_number;
  END OBJ(desktop_name, nullptr) obj->callbacks.print = &print_desktop_name;
#endif /* BUILD_GUI */
  END OBJ_ARG(format_time, nullptr, "format_time needs a pid as argument")
      obj->sub = static_cast<text_object *>(malloc(sizeof(struct text_object)));
  extract_variable_text_internal(obj->sub, arg);
  obj->callbacks.print = &print_format_time;
  END OBJ(shadecolor, nullptr)
#ifdef BUILD_GUI
      obj->data.l =
      (arg != nullptr ? parse_color(arg) : default_shade_color.get(*state))
          .to_argb32();
  obj->callbacks.print = &new_bg;
#endif /* BUILD_GUI */
  END OBJ(outlinecolor, nullptr)
#ifdef BUILD_GUI
      obj->data.l =
      (arg != nullptr ? parse_color(arg) : default_outline_color.get(*state))
          .to_argb32();
  obj->callbacks.print = &new_outline;
#endif /* BUILD_GUI */
  END OBJ(stippled_hr, nullptr)
#ifdef BUILD_GUI
      scan_stippled_hr(obj, arg);
  obj->callbacks.print = &new_stippled_hr;
#endif /* BUILD_GUI */
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
  END OBJ(alignr, nullptr) obj->data.l =
      arg != nullptr ? strtol(arg, nullptr, 10) : 1;
  obj->callbacks.print = &new_alignr;
  END OBJ(alignc, nullptr) obj->data.l =
      arg != nullptr ? strtol(arg, nullptr, 10) : 0;
  obj->callbacks.print = &new_alignc;
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
    LOG_ERROR("first argument to smapi_bat_bar must be an integer (got '{}')", arg ? arg : "(null)");
    obj->data.i = -1;
  } else
    arg = scan_bar(obj, arg + cnt, 100);
  obj->callbacks.barval = &smapi_bat_barval;
#endif /* BUILD_IBM */
#ifdef BUILD_CURL
  END OBJ_ARG(curl, 0, "curl needs arguments: <uri> <interval in minutes>")
      curl_parse_arg(obj, arg);
  obj->callbacks.print = &curl_print;
  obj->callbacks.free = &curl_obj_free;
#endif /* BUILD_CURL */
#ifdef BUILD_RSS
  END OBJ_ARG(rss, 0,
              "rss needs arguments: <uri> <interval in minutes> <action> "
              "[act_par] [spaces in front]") rss_scan_arg(obj, arg);
  obj->callbacks.print = &rss_print_info;
  obj->callbacks.free = &rss_free_obj_info;
#endif /* BUILD_RSS */
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
    CRIT_ERR_FREE(obj, free_at_crash,
                  "lua_bar needs arguments: <height>,<width> <function name> "
                  "[function parameters]");
  }
  obj->callbacks.barval = &lua_barval;
  obj->callbacks.free = &gen_free_opaque;
#ifdef BUILD_GUI
  END OBJ_ARG(
      lua_graph, nullptr,
      "lua_graph needs arguments: <function name> [height],[width] [gradient "
      "colour 1] [gradient colour 2] [scale] [-t] [-l]") auto [buf, skip] =
      scan_command(arg);
  scan_graph(obj, arg + skip, 100, FALSE);
  if (buf != nullptr) {
    obj->data.s = buf;
  } else {
    CRIT_ERR_FREE(obj, free_at_crash,
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
    CRIT_ERR_FREE(obj, free_at_crash,
                  "lua_gauge needs arguments: <height>,<width> <function name> "
                  "[function parameters]");
  }
  obj->callbacks.gaugeval = &lua_barval;
  obj->callbacks.free = &gen_free_opaque;
#endif /* BUILD_GUI */
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
  END OBJ_ARG(to_bytes, nullptr, "to_bytes needs a argument") obj->sub =
      static_cast<text_object *>(malloc(sizeof(struct text_object)));
  extract_variable_text_internal(obj->sub, arg);
  obj->callbacks.print = &print_to_bytes;
  END OBJ(scroll, nullptr)
#ifdef BUILD_GUI
  /* allocate a follower to reset any color changes */
#endif /* BUILD_GUI */
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
  END OBJ_ARG(nvidia, 0, "nvidia needs an argument") if (set_nvidia_query(
                                                             obj, arg,
                                                             text_node_t::
                                                                 NONSPECIAL)) {
    CRIT_ERR_FREE(obj, free_at_crash,
                  "nvidia: invalid argument"
                  " specified: '%s'",
                  arg);
  }
  obj->callbacks.print = &print_nvidia_value;
  obj->callbacks.free = &free_nvidia;
  END OBJ_ARG(
      nvidiabar, 0,
      "nvidiabar needs an argument") if (set_nvidia_query(obj, arg,
                                                          text_node_t::BAR)) {
    CRIT_ERR_FREE(obj, free_at_crash,
                  "nvidiabar: invalid argument"
                  " specified: '%s'",
                  arg);
  }
  obj->callbacks.barval = &get_nvidia_barval;
  obj->callbacks.free = &free_nvidia;
  END OBJ_ARG(
      nvidiagraph, 0,
      "nvidiagraph needs an argument") if (set_nvidia_query(obj, arg,
                                                            text_node_t::
                                                                GRAPH)) {
    CRIT_ERR_FREE(obj, free_at_crash,
                  "nvidiagraph: invalid argument"
                  " specified: '%s'",
                  arg);
  }
  obj->callbacks.graphval = &get_nvidia_barval;
  obj->callbacks.free = &free_nvidia;
  END OBJ_ARG(
      nvidiagauge, 0,
      "nvidiagauge needs an argument") if (set_nvidia_query(obj, arg,
                                                            text_node_t::
                                                                GAUGE)) {
    CRIT_ERR_FREE(obj, free_at_crash,
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
    CRIT_ERR_FREE(obj, free_at_crash, "apcupsd needs arguments: <host> <port>");
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
#ifdef BUILD_GUI
  END OBJ(apcupsd_loadgraph, &update_apcupsd) scan_graph(obj, arg, 100, FALSE);
  obj->callbacks.graphval = &apcupsd_loadbarval;
  END OBJ(apcupsd_loadgauge, &update_apcupsd) scan_gauge(obj, arg, 100);
  obj->callbacks.gaugeval = &apcupsd_loadbarval;
#endif /* BUILD_GUI */
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
#ifdef BUILD_INTEL_BACKLIGHT
  END OBJ(intel_backlight, 0) obj->callbacks.print = &print_intel_backlight;
  obj->callbacks.free = &free_intel_backlight;
  init_intel_backlight(obj);
#endif /* BUILD_INTEL_BACKLIGHT */
  END {
    auto *buf = static_cast<char *>(malloc(text_buffer_size.get(*state)));

    LOG_ERROR("unknown variable '${}'", s);
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
    LOG_TRACE("replaced all templates in text: input is\n'{}'\noutput is\n'{}'",
          const_p, p);
  } else {
    LOG_TRACE("no templates to replace");
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
    LOG_ERROR("one or more $endif's are missing");
  }

  free(orig_p);
  return 0;
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
