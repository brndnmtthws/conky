/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
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
 * Copyright (c) 2005-2010 Brenden Matthews, Philip Kovacs, et. al.
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
#include "text_object.h"
#include "algebra.h"
#include "build.h"
#include "colours.h"
#include "combine.h"
#include "diskio.h"
#include "entropy.h"
#include "exec.h"
#include "i8k.h"
#include "proc.h"
#ifdef X11
#include "fonts.h"
#endif
#include "fs.h"
#ifdef HAVE_ICONV
#include "iconv_tools.h"
#endif
#include "logging.h"
#include "mixer.h"
#include "mail.h"
#include "mboxscan.h"
#include "net_stat.h"
#ifdef NVIDIA
#include "nvidia.h"
#endif
#include "read_tcp.h"
#include "scroll.h"
#include "specials.h"
#include "temphelper.h"
#include "template.h"
#include "tailhead.h"
#include "timeinfo.h"
#include "top.h"

#ifdef NCURSES
#include <ncurses.h>
#endif

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "freebsd.h"
#elif defined(__OpenBSD__)
#include "openbsd.h"
#endif

#include <string.h>
#include <ctype.h>

/* strip a leading /dev/ if any, following symlinks first
 *
 * BEWARE: this function returns a pointer to static content
 *         which gets overwritten in consecutive calls. I.e.:
 *         this function is NOT reentrant.
 */
const char *dev_name(const char *path)
{
	static char buf[255];	/* should be enough for pathnames */
	ssize_t buflen;

	if (!path)
		return NULL;

#define DEV_NAME(x) \
  x != NULL && strlen(x) > 5 && strncmp(x, "/dev/", 5) == 0 ? x + 5 : x
	if ((buflen = readlink(path, buf, 254)) == -1)
		return DEV_NAME(path);
	buf[buflen] = '\0';
	return DEV_NAME(buf);
#undef DEV_NAME
}

static struct text_object *new_text_object_internal(void)
{
	struct text_object *obj = malloc(sizeof(struct text_object));
	memset(obj, 0, sizeof(struct text_object));
	return obj;
}

static struct text_object *create_plain_text(const char *s)
{
	struct text_object *obj;

	if (s == NULL || *s == '\0') {
		return NULL;
	}

	obj = new_text_object_internal();

	obj->type = OBJ_text;
	obj->data.s = strndup(s, text_buffer_size);
	return obj;
}

/* construct_text_object() creates a new text_object */
struct text_object *construct_text_object(const char *s, const char *arg, long
		line, void **ifblock_opaque, void *free_at_crash)
{
	// struct text_object *obj = new_text_object();
	struct text_object *obj = new_text_object_internal();

	obj->line = line;

/* helper defines for internal use only */
#define __OBJ_HEAD(a, n) if (!strcmp(s, #a)) { \
	obj->type = OBJ_##a; add_update_callback(n);
#define __OBJ_IF obj_be_ifblock_if(ifblock_opaque, obj)
#define __OBJ_ARG(...) if (!arg) { CRIT_ERR(obj, free_at_crash, __VA_ARGS__); }

/* defines to be used below */
#define OBJ(a, n) __OBJ_HEAD(a, n) {
#define OBJ_ARG(a, n, ...) __OBJ_HEAD(a, n) __OBJ_ARG(__VA_ARGS__) {
#define OBJ_IF(a, n) __OBJ_HEAD(a, n) __OBJ_IF; {
#define OBJ_IF_ARG(a, n, ...) __OBJ_HEAD(a, n) __OBJ_ARG(__VA_ARGS__) __OBJ_IF; {
#define END } } else

#ifdef X11
	if (s[0] == '#') {
		obj->type = OBJ_color;
		obj->data.l = get_x11_color(s);
	} else
#endif /* X11 */
#ifndef __OpenBSD__
	OBJ(acpitemp, 0)
		obj->data.i = open_acpi_temperature(arg);
	END OBJ(acpiacadapter, 0)
#endif /* !__OpenBSD__ */
	END OBJ(freq, 0)
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| atoi(&arg[0]) > info.cpu_count) {
			obj->data.i = 1;
			/* NORM_ERR("freq: Invalid CPU number or you don't have that many CPUs! "
				"Displaying the clock for CPU 1."); */
		} else {
			obj->data.i = atoi(&arg[0]);
		}
	END OBJ(freq_g, 0)
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| atoi(&arg[0]) > info.cpu_count) {
			obj->data.i = 1;
			/* NORM_ERR("freq_g: Invalid CPU number or you don't have that many "
				"CPUs! Displaying the clock for CPU 1."); */
		} else {
			obj->data.i = atoi(&arg[0]);
		}
	END OBJ_ARG(read_tcp, 0, "read_tcp: Needs \"(host) port\" as argument(s)")
		parse_read_tcp_arg(obj, arg, free_at_crash);
#if defined(__linux__)
	END OBJ(voltage_mv, 0)
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| atoi(&arg[0]) > info.cpu_count) {
			obj->data.i = 1;
			/* NORM_ERR("voltage_mv: Invalid CPU number or you don't have that many "
				"CPUs! Displaying voltage for CPU 1."); */
		} else {
			obj->data.i = atoi(&arg[0]);
		}
	END OBJ(voltage_v, 0)
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| atoi(&arg[0]) > info.cpu_count) {
			obj->data.i = 1;
			/* NORM_ERR("voltage_v: Invalid CPU number or you don't have that many "
				"CPUs! Displaying voltage for CPU 1."); */
		} else {
			obj->data.i = atoi(&arg[0]);
		}

#ifdef HAVE_IWLIB
	END OBJ(wireless_essid, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(wireless_mode, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(wireless_bitrate, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(wireless_ap, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(wireless_link_qual, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(wireless_link_qual_max, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(wireless_link_qual_perc, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(wireless_link_bar, &update_net_stats)
		parse_net_stat_bar_arg(obj, arg, free_at_crash);
#endif /* HAVE_IWLIB */

#endif /* __linux__ */

#ifndef __OpenBSD__
	END OBJ(acpifan, 0)
	END OBJ(battery, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size);
	END OBJ(battery_short, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size);
	END OBJ(battery_time, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size);
	END OBJ(battery_percent, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size);
	END OBJ(battery_bar, 0)
		char bat[64];
		if (arg) {
			arg = scan_bar(obj, arg);
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size);
#endif /* !__OpenBSD__ */

#if defined(__linux__)
	END OBJ_ARG(disk_protect, 0, "disk_protect needs an argument")
		obj->data.s = strndup(dev_name(arg), text_buffer_size);
	END OBJ(i8k_version, &update_i8k)
	END OBJ(i8k_bios, &update_i8k)
	END OBJ(i8k_serial, &update_i8k)
	END OBJ(i8k_cpu_temp, &update_i8k)
	END OBJ(i8k_left_fan_status, &update_i8k)
	END OBJ(i8k_right_fan_status, &update_i8k)
	END OBJ(i8k_left_fan_rpm, &update_i8k)
	END OBJ(i8k_right_fan_rpm, &update_i8k)
	END OBJ(i8k_ac_status, &update_i8k)
	END OBJ(i8k_buttons_status, &update_i8k)
#if defined(IBM)
	END OBJ(ibm_fan, 0)
	END OBJ_ARG(ibm_temps, &get_ibm_acpi_temps, "ibm_temps: needs an argument")
		parse_ibm_temps_arg(obj, arg);
	END OBJ(ibm_volume, 0)
	END OBJ(ibm_brightness, 0)
#endif
	/* information from sony_laptop kernel module
	 * /sys/devices/platform/sony-laptop */
	END OBJ(sony_fanspeed, 0)
	END OBJ_IF(if_gw, &update_gateway_info)
	END OBJ_ARG(ioscheduler, 0, "get_ioscheduler needs an argument (e.g. hda)")
		obj->data.s = strndup(dev_name(arg), text_buffer_size);
	END OBJ(laptop_mode, 0)
	END OBJ_ARG(pb_battery, 0, "pb_battery: needs one argument: status, percent or time")
		if (strcmp(arg, "status") == EQUAL) {
			obj->data.i = PB_BATT_STATUS;
		} else if (strcmp(arg, "percent") == EQUAL) {
			obj->data.i = PB_BATT_PERCENT;
		} else if (strcmp(arg, "time") == EQUAL) {
			obj->data.i = PB_BATT_TIME;
		} else {
			NORM_ERR("pb_battery: illegal argument '%s', defaulting to status", arg);
			obj->data.i = PB_BATT_STATUS;
		}
#endif /* __linux__ */
#if (defined(__FreeBSD__) || defined(__linux__))
	END OBJ_IF_ARG(if_up, 0, "if_up needs an argument")
		parse_if_up_arg(obj, arg);
#endif
#if defined(__OpenBSD__)
	END OBJ_ARG(obsd_sensors_temp, 0, "obsd_sensors_temp: needs an argument")
		parse_obsd_sensor(obj, arg);
	END OBJ_ARG(obsd_sensors_fan, 0, "obsd_sensors_fan: needs 2 arguments (device and sensor number)")
		parse_obsd_sensor(obj, arg);
	END OBJ_ARG(obsd_sensors_volt, 0, "obsd_sensors_volt: needs 2 arguments (device and sensor number)")
		parse_obsd_sensor(obj, arg);
	END OBJ(obsd_vendor, 0)
	END OBJ(obsd_product, 0)
#endif /* __OpenBSD__ */
	END OBJ(buffers, &update_meminfo)
	END OBJ(cached, &update_meminfo)
#define SCAN_CPU(__arg, __var) { \
	int __offset = 0; \
	if (__arg && sscanf(__arg, " cpu%d %n", &__var, &__offset) > 0) \
		__arg += __offset; \
	else \
		__var = 0; \
}
	END OBJ(cpu, &update_cpu_usage)
		SCAN_CPU(arg, obj->data.i);
		DBGP2("Adding $cpu for CPU %d", obj->data.i);
	END OBJ(cpugauge, &update_cpu_usage)
		SCAN_CPU(arg, obj->data.i);
		scan_gauge(obj, arg);
		DBGP2("Adding $cpugauge for CPU %d", obj->data.i);
	END OBJ(cpubar, &update_cpu_usage)
		SCAN_CPU(arg, obj->data.i);
		scan_bar(obj, arg);
		DBGP2("Adding $cpubar for CPU %d", obj->data.i);
#ifdef X11
	END OBJ(cpugraph, &update_cpu_usage)
		char *buf = 0;
		SCAN_CPU(arg, obj->data.i);
		buf = scan_graph(obj, arg, 100);
		DBGP2("Adding $cpugraph for CPU %d", obj->data.i);
		if (buf) free(buf);
	END OBJ(loadgraph, &update_load_average)
		scan_loadgraph_arg(obj, arg);
#endif /* X11 */
	END OBJ(diskio, &update_diskio)
		parse_diskio_arg(obj, arg);
	END OBJ(diskio_read, &update_diskio)
		parse_diskio_arg(obj, arg);
	END OBJ(diskio_write, &update_diskio)
		parse_diskio_arg(obj, arg);
#ifdef X11
	END OBJ(diskiograph, &update_diskio)
		parse_diskiograph_arg(obj, arg);
	END OBJ(diskiograph_read, &update_diskio)
		parse_diskiograph_arg(obj, arg);
	END OBJ(diskiograph_write, &update_diskio)
		parse_diskiograph_arg(obj, arg);
#endif /* X11 */
	END OBJ(color, 0)
#ifdef X11
		if (output_methods & TO_X) {
			obj->data.l = arg ? get_x11_color(arg) : default_fg_color;
			set_current_text_color(obj->data.l);
		}
#endif /* X11 */
#ifdef NCURSES
		if (output_methods & TO_NCURSES) {
			obj->data.l = COLOR_WHITE;
			if(arg) {
				if(strcasecmp(arg, "red") == 0) {
					obj->data.l = COLOR_RED;
				}else if(strcasecmp(arg, "green") == 0) {
					obj->data.l = COLOR_GREEN;
				}else if(strcasecmp(arg, "yellow") == 0) {
					obj->data.l = COLOR_YELLOW;
				}else if(strcasecmp(arg, "blue") == 0) {
					obj->data.l = COLOR_BLUE;
				}else if(strcasecmp(arg, "magenta") == 0) {
					obj->data.l = COLOR_MAGENTA;
				}else if(strcasecmp(arg, "cyan") == 0) {
					obj->data.l = COLOR_CYAN;
				}else if(strcasecmp(arg, "black") == 0) {
					obj->data.l = COLOR_BLACK;
				}
			}
			set_current_text_color(obj->data.l);
			init_pair(obj->data.l, obj->data.l, COLOR_BLACK);
		}
#endif /* NCURSES */
	END OBJ(color0, 0)
		obj->data.l = color0;
		set_current_text_color(obj->data.l);
	END OBJ(color1, 0)
		obj->data.l = color1;
		set_current_text_color(obj->data.l);
	END OBJ(color2, 0)
		obj->data.l = color2;
		set_current_text_color(obj->data.l);
	END OBJ(color3, 0)
		obj->data.l = color3;
		set_current_text_color(obj->data.l);
	END OBJ(color4, 0)
		obj->data.l = color4;
		set_current_text_color(obj->data.l);
	END OBJ(color5, 0)
		obj->data.l = color5;
		set_current_text_color(obj->data.l);
	END OBJ(color6, 0)
		obj->data.l = color6;
		set_current_text_color(obj->data.l);
	END OBJ(color7, 0)
		obj->data.l = color7;
		set_current_text_color(obj->data.l);
	END OBJ(color8, 0)
		obj->data.l = color8;
		set_current_text_color(obj->data.l);
	END OBJ(color9, 0)
		obj->data.l = color9;
		set_current_text_color(obj->data.l);
#ifdef X11
	END OBJ(font, 0)
		obj->data.s = scan_font(arg);
#endif /* X11 */
	END OBJ(conky_version, 0)
		obj->type = OBJ_text;
		obj->data.s = strdup(VERSION);
	END OBJ(conky_build_date, 0)
		obj->type = OBJ_text;
		obj->data.s = strdup(BUILD_DATE);
	END OBJ(conky_build_arch, 0)
		obj->type = OBJ_text;
		obj->data.s = strdup(BUILD_ARCH);
	END OBJ(downspeed, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(downspeedf, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
#ifdef X11
	END OBJ(downspeedgraph, &update_net_stats)
		parse_net_stat_graph_arg(obj, arg, free_at_crash);
#endif /* X11 */
	END OBJ(else, 0)
		obj_be_ifblock_else(ifblock_opaque, obj);
	END OBJ(endif, 0)
		obj_be_ifblock_endif(ifblock_opaque, obj);
	END OBJ(eval, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
#if defined(IMLIB2) && defined(X11)
	END OBJ(image, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
#endif /* IMLIB2 */
	END OBJ(exec, 0)
		scan_exec_arg(obj, arg);
	END OBJ(execp, 0)
		scan_exec_arg(obj, arg);
	END OBJ(execbar, 0)
		scan_exec_arg(obj, arg);
	END OBJ(execgauge, 0)
		scan_exec_arg(obj, arg);
#ifdef X11
	END OBJ(execgraph, 0)
		scan_execgraph_arg(obj, arg);
#endif /* X11 */
	END OBJ_ARG(execibar, 0, "execibar needs arguments")
		scan_execi_arg(obj, arg);
#ifdef X11
	END OBJ_ARG(execigraph, 0, "execigraph needs arguments")
		scan_execgraph_arg(obj, arg);
#endif /* X11 */
	END OBJ_ARG(execigauge, 0, "execigauge needs arguments")
		scan_execi_arg(obj, arg);
	END OBJ_ARG(execi, 0, "execi needs arguments")
		scan_execi_arg(obj, arg);
	END OBJ_ARG(execpi, 0, "execpi needs arguments")
		scan_execi_arg(obj, arg);
	END OBJ_ARG(texeci, 0, "texeci needs arguments")
		scan_execi_arg(obj, arg);
	END OBJ(pre_exec, 0)
		scan_pre_exec_arg(obj, arg);
	END OBJ(fs_bar, &update_fs_stats)
		init_fs_bar(obj, arg);
	END OBJ(fs_bar_free, &update_fs_stats)
		init_fs_bar(obj, arg);
	END OBJ(fs_free, &update_fs_stats)
		init_fs(obj, arg);
	END OBJ(fs_used_perc, &update_fs_stats)
		init_fs(obj, arg);
	END OBJ(fs_free_perc, &update_fs_stats)
		init_fs(obj, arg);
	END OBJ(fs_size, &update_fs_stats)
		init_fs(obj, arg);
	END OBJ(fs_type, &update_fs_stats)
		init_fs(obj, arg);
	END OBJ(fs_used, &update_fs_stats)
		init_fs(obj, arg);
	END OBJ(hr, 0)
		obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(nameserver, &update_dns_data)
		parse_nameserver_arg(obj, arg);
	END OBJ(offset, 0)
		obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(voffset, 0)
		obj->data.i = arg ? atoi(arg) : 1;
	END OBJ_ARG(goto, 0, "goto needs arguments")
		obj->data.i = atoi(arg);
#ifdef X11
	END OBJ(tab, 0)
		scan_tab(obj, arg);
#endif /* X11 */
#ifdef __linux__
	END OBJ_ARG(i2c, 0, "i2c needs arguments")
		parse_i2c_sensor(obj, arg);
	END OBJ_ARG(platform, 0, "platform needs arguments")
		parse_platform_sensor(obj, arg);
	END OBJ_ARG(hwmon, 0, "hwmon needs argumanets")
		parse_hwmon_sensor(obj, arg);
	END
	/* we have four different types of top (top, top_mem, top_time and top_io). To
	 * avoid having almost-same code four times, we have this special
	 * handler. */
	if (strncmp(s, "top", 3) == EQUAL) {
		add_update_callback(&update_meminfo);
		add_update_callback(&update_top);
		if (!parse_top_args(s, arg, obj)) {
			return NULL;
		}
	} else OBJ(addr, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(addrs, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
#endif /* __linux__ */
	END OBJ_ARG(tail, 0, "tail needs arguments")
		init_tailhead("tail", arg, obj, free_at_crash);
	END OBJ_ARG(head, 0, "head needs arguments")
		init_tailhead("head", arg, obj, free_at_crash);
	END OBJ_ARG(lines, 0, "lines needs an argument")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ_ARG(words, 0, "words needs a argument")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ(loadavg, &update_load_average)
		scan_loadavg_arg(obj, arg);
	END OBJ_IF_ARG(if_empty, 0, "if_empty needs an argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_IF_ARG(if_match, 0, "if_match needs arguments")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_IF_ARG(if_existing, 0, "if_existing needs an argument or two")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ_IF_ARG(if_mounted, 0, "if_mounted needs an argument")
		obj->data.s = strndup(arg, text_buffer_size);
#ifdef __linux__
	END OBJ_IF_ARG(if_running, &update_top, "if_running needs an argument")
		top_running = 1;
		obj->data.s = strndup(arg, text_buffer_size);
#else
	END OBJ_IF_ARG(if_running, 0, "if_running needs an argument")
		char buf[text_buffer_size];

		snprintf(buf, text_buffer_size, "pidof %s >/dev/null", arg);
		obj->data.s = strndup(buf, text_buffer_size);
#endif
	END OBJ(kernel, 0)
	END OBJ(machine, 0)
	END OBJ(mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(new_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(seen_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(unseen_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(flagged_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(unflagged_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(forwarded_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(unforwarded_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(replied_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(unreplied_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(draft_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(trashed_mails, 0)
		parse_local_mail_args(obj, arg);
	END OBJ(mboxscan, 0)
		parse_mboxscan_arg(obj, arg);
	END OBJ(mem, &update_meminfo)
	END OBJ(memeasyfree, &update_meminfo)
	END OBJ(memfree, &update_meminfo)
	END OBJ(memmax, &update_meminfo)
	END OBJ(memperc, &update_meminfo)
	END OBJ(memgauge, &update_meminfo)
		scan_gauge(obj, arg);
	END OBJ(membar, &update_meminfo)
		scan_bar(obj, arg);
#ifdef X11
	END OBJ(memgraph, &update_meminfo)
		char *buf = 0;
		buf = scan_graph(obj, arg, 100);

		if (buf) free(buf);
#endif /* X11*/
	END OBJ(mixer, 0)
		parse_mixer_arg(obj, arg);
	END OBJ(mixerl, 0)
		parse_mixer_arg(obj, arg);
	END OBJ(mixerr, 0)
		parse_mixer_arg(obj, arg);
	END OBJ(mixerbar, 0)
		scan_mixer_bar(obj, arg);
	END OBJ(mixerlbar, 0)
		scan_mixer_bar(obj, arg);
	END OBJ(mixerrbar, 0)
		scan_mixer_bar(obj, arg);
	END OBJ_IF(if_mixer_mute, 0)
		parse_mixer_arg(obj, arg);
#ifdef X11
	END OBJ(monitor, &update_x11info)
	END OBJ(monitor_number, &update_x11info)
	END OBJ(desktop, &update_x11info)
	END OBJ(desktop_number, &update_x11info)
	END OBJ(desktop_name, &update_x11info)
#endif
	END OBJ_ARG(format_time, 0, "format_time needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ(nodename, 0)
	END OBJ_ARG(cmdline_to_pid, 0, "cmdline_to_pid needs a command line as argument")
		scan_cmdline_to_pid_arg(obj, arg, free_at_crash);
	END OBJ_ARG(pid_chroot, 0, "pid_chroot needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_cmdline, 0, "pid_cmdline needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_cwd, 0, "pid_cwd needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_environ, 0, "pid_environ needs arguments")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_environ_list, 0, "pid_environ_list needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_exe, 0, "pid_exe needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_nice, 0, "pid_nice needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_openfiles, 0, "pid_openfiles needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_parent, 0, "pid_parent needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_priority, 0, "pid_priority needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_state, 0, "pid_state needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_state_short, 0, "pid_state_short needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_stderr, 0, "pid_stderr needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_stdin, 0, "pid_stdin needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_stdout, 0, "pid_stdout needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_threads, 0, "pid_threads needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_thread_list, 0, "pid_thread_list needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_time_kernelmode, 0, "pid_time_kernelmode needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_time_usermode, 0, "pid_time_usermode needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_time, 0, "pid_time needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_uid, 0, "pid_uid needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_euid, 0, "pid_euid needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_suid, 0, "pid_suid needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_fsuid, 0, "pid_fsuid needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_gid, 0, "pid_gid needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_egid, 0, "pid_egid needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_sgid, 0, "pid_sgid needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_fsgid, 0, "pid_fsgid needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(gid_name, 0, "gid_name needs a gid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(uid_name, 0, "uid_name needs a uid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_read, 0, "pid_read needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_vmpeak, 0, "pid_vmpeak needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_vmsize, 0, "pid_vmsize needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_vmlck, 0, "pid_vmlck needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_vmhwm, 0, "pid_vmhwm needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_vmrss, 0, "pid_vmrss needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_vmdata, 0, "pid_vmdata needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_vmstk, 0, "pid_vmstk needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_vmexe, 0, "pid_vmexe needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_vmlib, 0, "pid_vmlib needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_vmpte, 0, "pid_vmpte needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(pid_write, 0, "pid_write needs a pid as argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ(processes, &update_total_processes)
#ifdef __linux__
	END OBJ(running_processes, &update_top)
		top_running = 1;
	END OBJ(threads, &update_threads)
	END OBJ(running_threads, &update_stat)
#else
	END OBJ(running_processes, &update_running_processes)
#endif /* __linux__ */
	END OBJ(shadecolor, 0)
#ifdef X11
		obj->data.l = arg ? get_x11_color(arg) : default_bg_color;
#endif /* X11 */
	END OBJ(outlinecolor, 0)
#ifdef X11
		obj->data.l = arg ? get_x11_color(arg) : default_out_color;
#endif /* X11 */
	END OBJ(stippled_hr, 0)
#ifdef X11
		scan_stippled_hr(obj, arg);
#endif /* X11 */
	END OBJ(swap, &update_meminfo)
	END OBJ(swapfree, &update_meminfo)
	END OBJ(swapmax, &update_meminfo)
	END OBJ(swapperc, &update_meminfo)
	END OBJ(swapbar, &update_meminfo)
		scan_bar(obj, arg);
	END OBJ(sysname, 0)
	END OBJ(time, 0)
		scan_time(obj, arg);
	END OBJ(utime, 0)
		scan_time(obj, arg);
	END OBJ(tztime, 0)
		scan_tztime(obj, arg);
#ifdef HAVE_ICONV
	END OBJ_ARG(iconv_start, 0, "Iconv requires arguments")
		init_iconv_start(obj, free_at_crash, arg);
	END OBJ(iconv_stop, 0)
		init_iconv_stop();
#endif
	END OBJ(totaldown, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(totalup, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(updates, 0)
	END OBJ_IF(if_updatenr, 0)
		obj->data.i = arg ? atoi(arg) : 0;
		if(obj->data.i == 0) CRIT_ERR(obj, free_at_crash, "if_updatenr needs a number above 0 as argument");
		set_updatereset(obj->data.i > get_updatereset() ? obj->data.i : get_updatereset());
	END OBJ(alignr, 0)
		obj->data.i = arg ? atoi(arg) : 0;
	END OBJ(alignc, 0)
		obj->data.i = arg ? atoi(arg) : 0;
	END OBJ(upspeed, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
	END OBJ(upspeedf, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
#ifdef X11
	END OBJ(upspeedgraph, &update_net_stats)
		parse_net_stat_graph_arg(obj, arg, free_at_crash);
#endif
	END OBJ(uptime_short, &update_uptime)
	END OBJ(uptime, &update_uptime)
#if defined(__linux__)
	END OBJ(user_names, &update_users)
	END OBJ(user_times, &update_users)
	END OBJ_ARG(user_time, 0, "user time needs a console name as argument")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ(user_terms, &update_users)
	END OBJ(user_number, &update_users)
	END OBJ(gw_iface, &update_gateway_info)
	END OBJ(gw_ip, &update_gateway_info)
#endif /* !__linux__ */
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
		|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
	END OBJ(apm_adapter, 0)
	END OBJ(apm_battery_life, 0)
	END OBJ(apm_battery_time, 0)
#endif /* __FreeBSD__ */
	END OBJ(imap_unseen, 0)
		parse_imap_mail_args(obj, arg);
	END OBJ(imap_messages, 0)
		parse_imap_mail_args(obj, arg);
	END OBJ(pop3_unseen, 0)
		parse_pop3_mail_args(obj, arg);
	END OBJ(pop3_used, 0)
		parse_pop3_mail_args(obj, arg);
#ifdef IBM
	END OBJ_ARG(smapi, 0, "smapi needs an argument")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ_IF_ARG(if_smapi_bat_installed, 0, "if_smapi_bat_installed needs an argument")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ_ARG(smapi_bat_perc, 0, "smapi_bat_perc needs an argument")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ_ARG(smapi_bat_temp, 0, "smapi_bat_temp needs an argument")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ_ARG(smapi_bat_power, 0, "smapi_bat_power needs an argument")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ_ARG(smapi_bat_bar, 0, "smapi_bat_bar needs an argument")
		int cnt;
		if(sscanf(arg, "%i %n", &obj->data.i, &cnt) <= 0) {
			NORM_ERR("first argument to smapi_bat_bar must be an integer value");
			obj->data.i = -1;
		} else
			arg = scan_bar(obj, arg + cnt);
#endif /* IBM */
#ifdef MPD
#define mpd_set_maxlen(name) \
		if (arg) { \
			int i; \
			sscanf(arg, "%d", &i); \
			if (i > 0) \
				obj->data.i = i + 1; \
			else \
				NORM_ERR(#name ": invalid length argument"); \
		}
	END OBJ(mpd_artist, &update_mpd)
		mpd_set_maxlen(mpd_artist);
		init_mpd();
	END OBJ(mpd_title, &update_mpd)
		mpd_set_maxlen(mpd_title);
		init_mpd();
	END OBJ(mpd_random, &update_mpd) init_mpd();
	END OBJ(mpd_repeat, &update_mpd) init_mpd();
	END OBJ(mpd_elapsed, &update_mpd) init_mpd();
	END OBJ(mpd_length, &update_mpd) init_mpd();
	END OBJ(mpd_track, &update_mpd)
		mpd_set_maxlen(mpd_track);
		init_mpd();
	END OBJ(mpd_name, &update_mpd)
		mpd_set_maxlen(mpd_name);
		init_mpd();
	END OBJ(mpd_file, &update_mpd)
		mpd_set_maxlen(mpd_file);
		init_mpd();
	END OBJ(mpd_percent, &update_mpd) init_mpd();
	END OBJ(mpd_album, &update_mpd)
		mpd_set_maxlen(mpd_album);
		init_mpd();
	END OBJ(mpd_vol, &update_mpd) init_mpd();
	END OBJ(mpd_bitrate, &update_mpd) init_mpd();
	END OBJ(mpd_status, &update_mpd) init_mpd();
	END OBJ(mpd_bar, &update_mpd)
		scan_bar(obj, arg);
		init_mpd();
	END OBJ(mpd_smart, &update_mpd)
		mpd_set_maxlen(mpd_smart);
		init_mpd();
	END OBJ_IF(if_mpd_playing, &update_mpd)
		init_mpd();
#undef mpd_set_maxlen
#endif /* MPD */
#ifdef MOC
	END OBJ(moc_state, &update_moc)
	END OBJ(moc_file, &update_moc)
	END OBJ(moc_title, &update_moc)
	END OBJ(moc_artist, &update_moc)
	END OBJ(moc_song, &update_moc)
	END OBJ(moc_album, &update_moc)
	END OBJ(moc_totaltime, &update_moc)
	END OBJ(moc_timeleft, &update_moc)
	END OBJ(moc_curtime, &update_moc)
	END OBJ(moc_bitrate, &update_moc)
	END OBJ(moc_rate, &update_moc)
#endif /* MOC */
#ifdef XMMS2
	END OBJ(xmms2_artist, &update_xmms2)
	END OBJ(xmms2_album, &update_xmms2)
	END OBJ(xmms2_title, &update_xmms2)
	END OBJ(xmms2_genre, &update_xmms2)
	END OBJ(xmms2_comment, &update_xmms2)
	END OBJ(xmms2_url, &update_xmms2)
	END OBJ(xmms2_tracknr, &update_xmms2)
	END OBJ(xmms2_bitrate, &update_xmms2)
	END OBJ(xmms2_date, &update_xmms2)
	END OBJ(xmms2_id, &update_xmms2)
	END OBJ(xmms2_duration, &update_xmms2)
	END OBJ(xmms2_elapsed, &update_xmms2)
	END OBJ(xmms2_size, &update_xmms2)
	END OBJ(xmms2_status, &update_xmms2)
	END OBJ(xmms2_percent, &update_xmms2)
	END OBJ(xmms2_bar, &update_xmms2)
		scan_bar(obj, arg);
	END OBJ(xmms2_smart, &update_xmms2)
	END OBJ(xmms2_playlist, &update_xmms2)
	END OBJ(xmms2_timesplayed, &update_xmms2)
	END OBJ_IF(if_xmms2_connected, &update_xmms2)
#endif
#ifdef AUDACIOUS
	END OBJ(audacious_status, &update_audacious)
	END OBJ_ARG(audacious_title, &update_audacious, "audacious_title needs an argument")
		sscanf(arg, "%d", &info.audacious.max_title_len);
		if (info.audacious.max_title_len > 0) {
			info.audacious.max_title_len++;
		} else {
			CRIT_ERR(obj, free_at_crash, "audacious_title: invalid length argument");
		}
	END OBJ(audacious_length, &update_audacious)
	END OBJ(audacious_length_seconds, &update_audacious)
	END OBJ(audacious_position, &update_audacious)
	END OBJ(audacious_position_seconds, &update_audacious)
	END OBJ(audacious_bitrate, &update_audacious)
	END OBJ(audacious_frequency, &update_audacious)
	END OBJ(audacious_channels, &update_audacious)
	END OBJ(audacious_filename, &update_audacious)
	END OBJ(audacious_playlist_length, &update_audacious)
	END OBJ(audacious_playlist_position, &update_audacious)
	END OBJ(audacious_main_volume, &update_audacious)
	END OBJ(audacious_bar, &update_audacious)
		scan_bar(obj, arg);
#endif
#ifdef BMPX
	END OBJ(bmpx_title, &update_bmpx)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_artist, &update_bmpx)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_album, &update_bmpx)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_track, &update_bmpx)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_uri, &update_bmpx)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_bitrate, &update_bmpx)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
#endif
#ifdef EVE
	END OBJ_ARG(eve, 0, "eve needs arguments: <userid> <apikey> <characterid>")
		scan_eve(obj, arg);
#endif
#ifdef HAVE_CURL
	END OBJ_ARG(curl, 0, "curl needs arguments: <uri> <interval in minutes>")
		curl_parse_arg(obj, arg);
#endif
#ifdef RSS
	END OBJ_ARG(rss, 0, "rss needs arguments: <uri> <interval in minutes> <action> [act_par] [spaces in front]")
		rss_scan_arg(obj, arg);
#endif
#ifdef WEATHER
	END OBJ_ARG(weather, 0, "weather needs arguments: <uri> <locID> <data_type> [interval in minutes]")
		scan_weather_arg(obj, arg, free_at_crash);
#endif
#ifdef XOAP
	END OBJ_ARG(weather_forecast, 0, "weather_forecast needs arguments: <uri> <locID> <day> <data_type> [interval in minutes]")
		scan_weather_forecast_arg(obj, arg, free_at_crash);
#endif
#ifdef HAVE_LUA
	END OBJ_ARG(lua, 0, "lua needs arguments: <function name> [function parameters]")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ_ARG(lua_parse, 0, "lua_parse needs arguments: <function name> [function parameters]")
		obj->data.s = strndup(arg, text_buffer_size);
	END OBJ_ARG(lua_bar, 0, "lua_bar needs arguments: <height>,<width> <function name> [function parameters]")
		arg = scan_bar(obj, arg);
		if(arg) {
			obj->data.s = strndup(arg, text_buffer_size);
		} else {
			CRIT_ERR(obj, free_at_crash, "lua_bar needs arguments: <height>,<width> <function name> [function parameters]");
		}
#ifdef X11
	END OBJ_ARG(lua_graph, 0, "lua_graph needs arguments: <function name> [height],[width] [gradient colour 1] [gradient colour 2] [scale] [-t] [-l]")
		char *buf = 0;
		buf = scan_graph(obj, arg, 0);
		if (buf) {
			obj->data.s = buf;
		} else {
			CRIT_ERR(obj, free_at_crash, "lua_graph needs arguments: <function name> [height],[width] [gradient colour 1] [gradient colour 2] [scale] [-t] [-l]");
		}
#endif /* X11 */
	END OBJ_ARG(lua_gauge, 0, "lua_gauge needs arguments: <height>,<width> <function name> [function parameters]")
		arg = scan_gauge(obj, arg);
		if (arg) {
			obj->data.s = strndup(arg, text_buffer_size);
		} else {
			CRIT_ERR(obj, free_at_crash, "lua_gauge needs arguments: <height>,<width> <function name> [function parameters]");
		}
#endif /* HAVE_LUA */
#ifdef HDDTEMP
	END OBJ(hddtemp, &update_hddtemp)
		if (arg)
			obj->data.s = strndup(arg, text_buffer_size);
#endif /* HDDTEMP */
#ifdef TCP_PORT_MONITOR
	END OBJ_ARG(tcp_portmon, &tcp_portmon_update, "tcp_portmon: needs arguments")
		tcp_portmon_init(obj, arg);
#endif /* TCP_PORT_MONITOR */
	END OBJ(entropy_avail, &update_entropy)
	END OBJ(entropy_perc, &update_entropy)
	END OBJ(entropy_poolsize, &update_entropy)
	END OBJ(entropy_bar, &update_entropy)
		scan_bar(obj, arg);
	END OBJ_ARG(include, 0, "include needs a argument")
		struct conftree *leaf = conftree_add(currentconffile, arg);
		if(leaf) {
			if (load_config_file(arg) == TRUE) {
				obj->sub = malloc(sizeof(struct text_object));
				currentconffile = leaf;
				extract_variable_text_internal(obj->sub, get_global_text());
				currentconffile = leaf->back;
			} else {
				NORM_ERR("Can't load configfile '%s'.", arg);
			}
		} else {
			NORM_ERR("You are trying to load '%s' recursively, I'm only going to load it once to prevent an infinite loop.", arg);
		}
	END OBJ_ARG(blink, 0, "blink needs a argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ_ARG(to_bytes, 0, "to_bytes needs a argument")
		obj->sub = malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
	END OBJ(scroll, 0)
		parse_scroll_arg(obj, arg, free_at_crash);
	END OBJ_ARG(combine, 0, "combine needs arguments: <text1> <text2>")
		parse_combine_arg(obj, arg, free_at_crash);
#ifdef NVIDIA
	END OBJ_ARG(nvidia, 0, "nvidia needs an argument")
		if (set_nvidia_type(obj, arg)) {
			CRIT_ERR(obj, free_at_crash, "nvidia: invalid argument"
				 " specified: '%s'\n", arg);
		}
#endif /* NVIDIA */
#ifdef APCUPSD
	END OBJ_ARG(apcupsd, &update_apcupsd, "apcupsd needs arguments: <host> <port>")
		char host[64];
		int port;
		if (sscanf(arg, "%63s %d", host, &port) != 2) {
			CRIT_ERR(obj, free_at_crash, "apcupsd needs arguments: <host> <port>");
		} else {
			info.apcupsd.port = htons(port);
			strncpy(info.apcupsd.host, host, sizeof(info.apcupsd.host));
		}
	END OBJ(apcupsd_name, &update_apcupsd)
	END OBJ(apcupsd_model, &update_apcupsd)
	END OBJ(apcupsd_upsmode, &update_apcupsd)
	END OBJ(apcupsd_cable, &update_apcupsd)
	END OBJ(apcupsd_status, &update_apcupsd)
	END OBJ(apcupsd_linev, &update_apcupsd)
	END OBJ(apcupsd_load, &update_apcupsd)
	END OBJ(apcupsd_loadbar, &update_apcupsd)
		scan_bar(obj, arg);
#ifdef X11
	END OBJ(apcupsd_loadgraph, &update_apcupsd)
		char* buf = 0;
		buf = scan_graph(obj, arg, 0);
		if (buf) free(buf);
#endif /* X11 */
	END OBJ(apcupsd_loadgauge, &update_apcupsd)
		scan_gauge(obj, arg);
	END OBJ(apcupsd_charge, &update_apcupsd)
	END OBJ(apcupsd_timeleft, &update_apcupsd)
	END OBJ(apcupsd_temp, &update_apcupsd)
	END OBJ(apcupsd_lastxfer, &update_apcupsd)
#endif /* APCUPSD */
	END {
		char buf[text_buffer_size];

		NORM_ERR("unknown variable %s", s);
		obj->type = OBJ_text;
		snprintf(buf, text_buffer_size, "${%s}", s);
		obj->data.s = strndup(buf, text_buffer_size);
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
static size_t remove_comment(char *string, char *last)
{
	char *end = string;
	while (*end != '\0' && *end != '\n') {
		++end;
	}
	if (last) *last = *end;
	if (*end == '\n') end++;
	strfold(string, end - string);
	return end - string;
}

size_t remove_comments(char *string)
{
	char *curplace;
	size_t folded = 0;
	for (curplace = string; *curplace != 0; curplace++) {
		if (*curplace == '\\' && *(curplace + 1) == '#') {
			// strcpy can't be used for overlapping strings
			strfold(curplace, 1);
			folded += 1;
		} else if (*curplace == '#') {
			folded += remove_comment(curplace, 0);
		}
	}
	return folded;
}

int extract_variable_text_internal(struct text_object *retval, const char *const_p)
{
	struct text_object *obj;
	char *p, *s, *orig_p;
	long line;
	void *ifblock_opaque = NULL;
	char *tmp_p;
	char *arg = 0;
	size_t len = 0;

	p = strndup(const_p, max_user_text - 1);
	while (text_contains_templates(p)) {
		char *tmp;
		tmp = find_and_replace_templates(p);
		free(p);
		p = tmp;
	}
	s = orig_p = p;

	if (strcmp(p, const_p)) {
		DBGP2("replaced all templates in text: input is\n'%s'\noutput is\n'%s'", const_p, p);
	} else {
		DBGP2("no templates to replace");
	}

	memset(retval, 0, sizeof(struct text_object));

	line = global_text_lines;

	while (*p) {
		if (*p == '\n') {
			line++;
		}
		if (*p == '$') {
			*p = '\0';
			obj = create_plain_text(s);
			if (obj != NULL) {
				append_object(retval, obj);
			}
			*p = '$';
			p++;
			s = p;

			if (*p != '$') {
				char buf[text_buffer_size];
				const char *var;

				/* variable is either $foo or ${foo} */
				if (*p == '{') {
					unsigned int brl = 1, brr = 0;

					p++;
					s = p;
					while (*p && brl != brr) {
						if (*p == '{') {
							brl++;
						}
						if (*p == '}') {
							brr++;
						}
						p++;
					}
					p--;
				} else {
					s = p;
					if (*p == '#') {
						p++;
					}
					while (*p && (isalnum((int) *p) || *p == '_')) {
						p++;
					}
				}

				/* copy variable to buffer */
				len = ((unsigned int) (p - s) > text_buffer_size - 1) ? text_buffer_size - 1 : (unsigned int) (p - s);
				strncpy(buf, s, len);
				buf[len] = '\0';

				if (*p == '}') {
					p++;
				}
				s = p;

				/* search for variable in environment */

				var = getenv(buf);
				if (var) {
					obj = create_plain_text(var);
					if (obj) {
						append_object(retval, obj);
					}
					continue;
				}

				/* if variable wasn't found in environment, use some special */

				arg = 0;

				/* split arg */
				if (strchr(buf, ' ')) {
					arg = strchr(buf, ' ');
					*arg = '\0';
					arg++;
					while (isspace((int) *arg)) {
						arg++;
					}
					if (!*arg) {
						arg = 0;
					}
				}

				/* lowercase variable name */
				tmp_p = buf;
				while (*tmp_p) {
					*tmp_p = tolower(*tmp_p);
					tmp_p++;
				}

				obj = construct_text_object(buf, arg,
						line, &ifblock_opaque, orig_p);
				if (obj != NULL) {
					append_object(retval, obj);
				}
				continue;
			} else {
				obj = create_plain_text("$");
				s = p + 1;
				if (obj != NULL) {
					append_object(retval, obj);
				}
			}
		} else if (*p == '\\' && *(p+1) == '#') {
			strfold(p, 1);
		} else if (*p == '#') {
			char c;
			if (remove_comment(p, &c) && p > orig_p && c == '\n') {
				/* if remove_comment removed a newline, we need to 'back up' with p */
				p--;
			}
		}
		p++;
	}
	obj = create_plain_text(s);
	if (obj != NULL) {
		append_object(retval, obj);
	}

	if (!ifblock_stack_empty(&ifblock_opaque)) {
		NORM_ERR("one or more $endif's are missing");
	}

	free(orig_p);
	return 0;
}

/*
 * Frees the list of text objects root points to.  When internal = 1, it won't
 * free global objects.
 */
void free_text_objects(struct text_object *root, int internal)
{
	struct text_object *obj;

	if (!root->prev) {
		return;
	}

#define data obj->data
	for (obj = root->prev; obj; obj = root->prev) {
		root->prev = obj->prev;
		switch (obj->type) {
#ifndef __OpenBSD__
			case OBJ_acpitemp:
				close(data.i);
				break;
#endif /* !__OpenBSD__ */
#ifdef __linux__
			case OBJ_i2c:
			case OBJ_platform:
			case OBJ_hwmon:
				free_sysfs_sensor(obj);
				break;
#endif /* __linux__ */
			case OBJ_cmdline_to_pid:
				free(data.s);
				break;
			case OBJ_format_time:
			case OBJ_pid_environ:
			case OBJ_pid_chroot:
			case OBJ_pid_cmdline:
			case OBJ_pid_cwd:
			case OBJ_pid_environ_list:
			case OBJ_pid_exe:
			case OBJ_pid_nice:
			case OBJ_pid_openfiles:
			case OBJ_pid_parent:
			case OBJ_pid_priority:
			case OBJ_pid_state:
			case OBJ_pid_state_short:
			case OBJ_pid_stderr:
			case OBJ_pid_stdin:
			case OBJ_pid_stdout:
			case OBJ_pid_threads:
			case OBJ_pid_thread_list:
			case OBJ_pid_time_kernelmode:
			case OBJ_pid_time_usermode:
			case OBJ_pid_time:
			case OBJ_pid_uid:
			case OBJ_pid_euid:
			case OBJ_pid_suid:
			case OBJ_pid_fsuid:
			case OBJ_pid_gid:
			case OBJ_pid_egid:
			case OBJ_pid_sgid:
			case OBJ_pid_fsgid:
			case OBJ_pid_read:
			case OBJ_pid_vmpeak:
			case OBJ_pid_vmsize:
			case OBJ_pid_vmlck:
			case OBJ_pid_vmhwm:
			case OBJ_pid_vmrss:
			case OBJ_pid_vmdata:
			case OBJ_pid_vmstk:
			case OBJ_pid_vmexe:
			case OBJ_pid_vmlib:
			case OBJ_pid_vmpte:
			case OBJ_pid_write:
			case OBJ_gid_name:
				if(obj->sub) {
					free_text_objects(obj->sub, 1);
					free(obj->sub);
				}
				break;
			case OBJ_uid_name:
				if(obj->sub) {
					free_text_objects(obj->sub, 1);
					free(obj->sub);
				}
				break;
			case OBJ_read_tcp:
				free_read_tcp(obj);
				break;
			case OBJ_time:
			case OBJ_utime:
				free_time(obj);
				break;
			case OBJ_tztime:
				free_tztime(obj);
				break;
			case OBJ_mboxscan:
				free_mboxscan(obj);
				break;
			case OBJ_mails:
			case OBJ_new_mails:
			case OBJ_seen_mails:
			case OBJ_unseen_mails:
			case OBJ_flagged_mails:
			case OBJ_unflagged_mails:
			case OBJ_forwarded_mails:
			case OBJ_unforwarded_mails:
			case OBJ_replied_mails:
			case OBJ_unreplied_mails:
			case OBJ_draft_mails:
			case OBJ_trashed_mails:
				free_local_mails(obj);
				break;
			case OBJ_imap_unseen:
			case OBJ_imap_messages:
			case OBJ_pop3_unseen:
			case OBJ_pop3_used:
				free_mail_obj(obj);
				break;
			case OBJ_if_empty:
			case OBJ_if_match:
				free_text_objects(obj->sub, 1);
				free(obj->sub);
				break;
			case OBJ_if_existing:
			case OBJ_if_mounted:
			case OBJ_if_running:
				free(data.s);
				break;
			case OBJ_head:
			case OBJ_tail:
				free_tailhead(obj);
				break;
			case OBJ_text:
			case OBJ_font:
			case OBJ_image:
			case OBJ_eval:
				free(data.s);
				break;
			case OBJ_exec:
			case OBJ_execbar:
			case OBJ_execgauge:
#ifdef X11
			case OBJ_execgraph:
#endif
			case OBJ_execp:
				free_exec(obj);
				break;
#ifdef HAVE_ICONV
			case OBJ_iconv_start:
				free_iconv();
				break;
#endif
#ifdef __linux__
			case OBJ_disk_protect:
				free(data.s);
				break;
			case OBJ_if_gw:
			case OBJ_gw_iface:
			case OBJ_gw_ip:
				free_gateway_info();
				break;
			case OBJ_ioscheduler:
				if(data.s)
					free(data.s);
				break;
#endif
#if (defined(__FreeBSD__) || defined(__linux__))
			case OBJ_if_up:
				free_if_up(obj);
				break;
#endif
#ifdef XMMS2
			case OBJ_xmms2_artist:
				if (info.xmms2.artist) {
					free(info.xmms2.artist);
					info.xmms2.artist = 0;
				}
				break;
			case OBJ_xmms2_album:
				if (info.xmms2.album) {
					free(info.xmms2.album);
					info.xmms2.album = 0;
				}
				break;
			case OBJ_xmms2_title:
				if (info.xmms2.title) {
					free(info.xmms2.title);
					info.xmms2.title = 0;
				}
				break;
			case OBJ_xmms2_genre:
				if (info.xmms2.genre) {
					free(info.xmms2.genre);
					info.xmms2.genre = 0;
				}
				break;
			case OBJ_xmms2_comment:
				if (info.xmms2.comment) {
					free(info.xmms2.comment);
					info.xmms2.comment = 0;
				}
				break;
			case OBJ_xmms2_url:
				if (info.xmms2.url) {
					free(info.xmms2.url);
					info.xmms2.url = 0;
				}
				break;
			case OBJ_xmms2_date:
				if (info.xmms2.date) {
					free(info.xmms2.date);
					info.xmms2.date = 0;
				}
				break;
			case OBJ_xmms2_status:
				if (info.xmms2.status) {
					free(info.xmms2.status);
					info.xmms2.status = 0;
				}
				break;
			case OBJ_xmms2_playlist:
				if (info.xmms2.playlist) {
					free(info.xmms2.playlist);
					info.xmms2.playlist = 0;
				}
				break;
			case OBJ_xmms2_smart:
				if (info.xmms2.artist) {
					free(info.xmms2.artist);
					info.xmms2.artist = 0;
				}
				if (info.xmms2.title) {
					free(info.xmms2.title);
					info.xmms2.title = 0;
				}
				if (info.xmms2.url) {
					free(info.xmms2.url);
					info.xmms2.url = 0;
				}
				break;
#endif
#ifdef BMPX
			case OBJ_bmpx_title:
			case OBJ_bmpx_artist:
			case OBJ_bmpx_album:
			case OBJ_bmpx_track:
			case OBJ_bmpx_uri:
			case OBJ_bmpx_bitrate:
				break;
#endif
#ifdef EVE
			case OBJ_eve:
				free_eve(obj);
				break;
#endif
#ifdef HAVE_CURL
			case OBJ_curl:
				curl_obj_free(obj);
				break;
#endif
#ifdef RSS
			case OBJ_rss:
				rss_free_obj_info(obj);
				break;
#endif
#ifdef WEATHER
			case OBJ_weather:
				free_weather(obj);
				break;
#endif
#ifdef XOAP
			case OBJ_weather_forecast:
				free_weather(obj);
				break;
#endif
#ifdef HAVE_LUA
			case OBJ_lua:
			case OBJ_lua_parse:
			case OBJ_lua_bar:
#ifdef X11
			case OBJ_lua_graph:
#endif /* X11 */
			case OBJ_lua_gauge:
				free(data.s);
				break;
#endif /* HAVE_LUA */
			case OBJ_pre_exec:
				break;
#ifndef __OpenBSD__
			case OBJ_battery:
				free(data.s);
				break;
			case OBJ_battery_short:
				free(data.s);
				break;
			case OBJ_battery_time:
				free(data.s);
				break;
#endif /* !__OpenBSD__ */
			case OBJ_execpi:
			case OBJ_execi:
			case OBJ_execibar:
			case OBJ_texeci:
#ifdef X11
			case OBJ_execigraph:
#endif /* X11 */
			case OBJ_execigauge:
				free_execi(obj);
				break;
			case OBJ_nameserver:
				free_dns_data();
				break;
#ifdef __linux__
			case OBJ_top:
			case OBJ_top_mem:
			case OBJ_top_time:
#ifdef IOSTATS
			case OBJ_top_io:
#endif
				free_top(obj, internal);
				break;
#endif /* __linux__ */
#ifdef HDDTEMP
			case OBJ_hddtemp:
				if (data.s) {
					free(data.s);
					data.s = NULL;
				}
				free_hddtemp();
				break;
#endif /* HDDTEMP */
			case OBJ_entropy_avail:
			case OBJ_entropy_perc:
			case OBJ_entropy_poolsize:
			case OBJ_entropy_bar:
				break;
			case OBJ_user_names:
				if (info.users.names) {
					free(info.users.names);
					info.users.names = 0;
				}
				break;
			case OBJ_user_terms:
				if (info.users.terms) {
					free(info.users.terms);
					info.users.terms = 0;
				}
				break;
			case OBJ_user_times:
				if (info.users.times) {
					free(info.users.times);
					info.users.times = 0;
				}
				break;
			case OBJ_user_time:
				if (info.users.ctime) {
					free(info.users.ctime);
					info.users.ctime = 0;
				}
				if (data.s) {
					free(data.s);
				}
				break;
#ifdef IBM
			case OBJ_smapi:
			case OBJ_smapi_bat_perc:
			case OBJ_smapi_bat_temp:
			case OBJ_smapi_bat_power:
				free(data.s);
				break;
			case OBJ_if_smapi_bat_installed:
				free(data.s);
				break;
#endif /* IBM */
#ifdef NVIDIA
			case OBJ_nvidia:
				free_nvidia(obj);
				break;
#endif /* NVIDIA */
#ifdef MPD
			case OBJ_mpd_title:
			case OBJ_mpd_artist:
			case OBJ_mpd_album:
			case OBJ_mpd_random:
			case OBJ_mpd_repeat:
			case OBJ_mpd_vol:
			case OBJ_mpd_bitrate:
			case OBJ_mpd_status:
			case OBJ_mpd_bar:
			case OBJ_mpd_elapsed:
			case OBJ_mpd_length:
			case OBJ_mpd_track:
			case OBJ_mpd_name:
			case OBJ_mpd_file:
			case OBJ_mpd_percent:
			case OBJ_mpd_smart:
			case OBJ_if_mpd_playing:
				free_mpd();
				break;
#endif /* MPD */
#ifdef MOC
			case OBJ_moc_state:
			case OBJ_moc_file:
			case OBJ_moc_title:
			case OBJ_moc_artist:
			case OBJ_moc_song:
			case OBJ_moc_album:
			case OBJ_moc_totaltime:
			case OBJ_moc_timeleft:
			case OBJ_moc_curtime:
			case OBJ_moc_bitrate:
			case OBJ_moc_rate:
				free_moc();
				break;
#endif /* MOC */
			case OBJ_include:
			case OBJ_blink:
			case OBJ_to_bytes:
				if(obj->sub) {
					free_text_objects(obj->sub, 1);
					free(obj->sub);
				}
				break;
			case OBJ_scroll:
				free_scroll(obj);
				break;
			case OBJ_combine:
				free_combine(obj);
				break;
#ifdef APCUPSD
			case OBJ_apcupsd:
			case OBJ_apcupsd_name:
			case OBJ_apcupsd_model:
			case OBJ_apcupsd_upsmode:
			case OBJ_apcupsd_cable:
			case OBJ_apcupsd_status:
			case OBJ_apcupsd_linev:
			case OBJ_apcupsd_load:
			case OBJ_apcupsd_loadbar:
#ifdef X11
			case OBJ_apcupsd_loadgraph:
#endif /* X11 */
			case OBJ_apcupsd_loadgauge:
			case OBJ_apcupsd_charge:
			case OBJ_apcupsd_timeleft:
			case OBJ_apcupsd_temp:
			case OBJ_apcupsd_lastxfer:
				break;
#endif /* APCUPSD */
#ifdef TCP_PORT_MONITOR
			case OBJ_tcp_portmon:
				tcp_portmon_free(obj);
				break;
#endif /* TCP_PORT_MONITOR */
#ifdef X11
			case OBJ_desktop:
			case OBJ_desktop_number:
			case OBJ_desktop_name:
			        if(info.x11.desktop.name && !internal) {
				  free(info.x11.desktop.name);
				  info.x11.desktop.name = NULL;
			        }
			        if(info.x11.desktop.all_names && !internal) {
				  free(info.x11.desktop.all_names);
				  info.x11.desktop.all_names = NULL;
			        }
				break;
#endif /* X11 */
		}
		if(obj->special_data)
			free(obj->special_data);
		free(obj);
	}
#undef data
}

