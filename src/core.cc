/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
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
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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
#include "bsdapm.h"
#include "colours.h"
#include "combine.h"
#include "diskio.h"
#include "entropy.h"
#include "exec.h"
#include "i8k.h"
#ifdef BUILD_IMLIB2
#include "imlib2.h"
#endif
#include "proc.h"
#ifdef BUILD_MYSQL
#include "mysql.h"
#endif
#ifdef BUILD_ICAL
#include "ical.h"
#endif
#ifdef BUILD_IRC
#include "irc.h"
#endif
#ifdef BUILD_X11
#include "fonts.h"
#endif
#include "fs.h"
#ifdef BUILD_IBM
#include "ibm.h"
#include "smapi.h"
#endif
#ifdef BUILD_ICONV
#include "iconv_tools.h"
#endif
#include "llua.h"
#include "logging.h"
#include "mixer.h"
#include "mail.h"
#include "mboxscan.h"
#include "nc.h"
#include "net_stat.h"
#ifdef BUILD_NVIDIA
#include "nvidia.h"
#endif
#include "read_tcpip.h"
#include "scroll.h"
#include "specials.h"
#include "temphelper.h"
#include "template.h"
#include "tailhead.h"
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
#endif
#ifdef BUILD_CMUS
#include "cmus.h"
#endif

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "freebsd.h"
#elif defined(__DragonFly__)
#include "dragonfly.h"
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
	struct text_object *obj = (text_object*)malloc(sizeof(struct text_object));
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

	obj_be_plain_text(obj, s);
	return obj;
}

#ifdef BUILD_CURL
void stock_parse_arg(struct text_object *obj, const char *arg)
{
	char stock[8];
	char data[16];

	obj->data.s = NULL;
	if(sscanf(arg, "%7s %15s", stock, data) != 2) {
		NORM_ERR("wrong number of arguments for $stock");
		return;
	}
	if(!strcasecmp("ask", data)) strcpy(data, "a");
	else if(!strcasecmp("adv", data)) strcpy(data, "a2");
	else if(!strcasecmp("asksize", data)) strcpy(data, "a5");
	else if(!strcasecmp("bid", data)) strcpy(data, "b");
	else if(!strcasecmp("askrt", data)) strcpy(data, "b2");
	else if(!strcasecmp("bidrt", data)) strcpy(data, "b3");
	else if(!strcasecmp("bookvalue", data)) strcpy(data, "b4");
	else if(!strcasecmp("bidsize", data)) strcpy(data, "b6");
	else if(!strcasecmp("change", data)) strcpy(data, "c1");
	else if(!strcasecmp("commission", data)) strcpy(data, "c3");
	else if(!strcasecmp("changert", data)) strcpy(data, "c6");
	else if(!strcasecmp("ahcrt", data)) strcpy(data, "c8");
	else if(!strcasecmp("ds", data)) strcpy(data, "d");
	else if(!strcasecmp("ltd", data)) strcpy(data, "d1");
	else if(!strcasecmp("tradedate", data)) strcpy(data, "d2");
	else if(!strcasecmp("es", data)) strcpy(data, "e");
	else if(!strcasecmp("ei", data)) strcpy(data, "e1");
	else if(!strcasecmp("epsecy", data)) strcpy(data, "e7");
	else if(!strcasecmp("epseny", data)) strcpy(data, "e8");
	else if(!strcasecmp("epsenq", data)) strcpy(data, "e9");
	else if(!strcasecmp("floatshares", data)) strcpy(data, "f6");
	else if(!strcasecmp("dayslow", data)) strcpy(data, "g");
	else if(!strcasecmp("dayshigh", data)) strcpy(data, "h");
	else if(!strcasecmp("52weeklow", data)) strcpy(data, "j");
	else if(!strcasecmp("52weekhigh", data)) strcpy(data, "k");
	else if(!strcasecmp("hgp", data)) strcpy(data, "g1");
	else if(!strcasecmp("ag", data)) strcpy(data, "g3");
	else if(!strcasecmp("hg", data)) strcpy(data, "g4");
	else if(!strcasecmp("hgprt", data)) strcpy(data, "g5");
	else if(!strcasecmp("hgrt", data)) strcpy(data, "g6");
	else if(!strcasecmp("moreinfo", data)) strcpy(data, "i");
	else if(!strcasecmp("obrt", data)) strcpy(data, "i5");
	else if(!strcasecmp("mc", data)) strcpy(data, "j1");
	else if(!strcasecmp("mcrt", data)) strcpy(data, "j3");
	else if(!strcasecmp("ebitda", data)) strcpy(data, "j4");
	else if(!strcasecmp("c52wlow", data)) strcpy(data, "j5");
	else if(!strcasecmp("pc52wlow", data)) strcpy(data, "j6");
	else if(!strcasecmp("cprt", data)) strcpy(data, "k2");
	else if(!strcasecmp("lts", data)) strcpy(data, "k3");
	else if(!strcasecmp("c52whigh", data)) strcpy(data, "k4");
	else if(!strcasecmp("pc52whigh", data)) strcpy(data, "k5");
	else if(!strcasecmp("ltp", data)) strcpy(data, "l1");
	else if(!strcasecmp("hl", data)) strcpy(data, "l2");
	else if(!strcasecmp("ll", data)) strcpy(data, "l3");
	else if(!strcasecmp("dr", data)) strcpy(data, "m");
	else if(!strcasecmp("drrt", data)) strcpy(data, "m2");
	else if(!strcasecmp("50ma", data)) strcpy(data, "m3");
	else if(!strcasecmp("200ma", data)) strcpy(data, "m4");
	else if(!strcasecmp("c200ma", data)) strcpy(data, "m5");
	else if(!strcasecmp("pc200ma", data)) strcpy(data, "m6");
	else if(!strcasecmp("c50ma", data)) strcpy(data, "m7");
	else if(!strcasecmp("pc50ma", data)) strcpy(data, "m8");
	else if(!strcasecmp("name", data)) strcpy(data, "n");
	else if(!strcasecmp("notes", data)) strcpy(data, "n4");
	else if(!strcasecmp("open", data)) strcpy(data, "o");
	else if(!strcasecmp("pc", data)) strcpy(data, "p");
	else if(!strcasecmp("pricepaid", data)) strcpy(data, "p1");
	else if(!strcasecmp("cip", data)) strcpy(data, "p2");
	else if(!strcasecmp("ps", data)) strcpy(data, "p5");
	else if(!strcasecmp("pb", data)) strcpy(data, "p6");
	else if(!strcasecmp("edv", data)) strcpy(data, "q");
	else if(!strcasecmp("per", data)) strcpy(data, "r");
	else if(!strcasecmp("dpd", data)) strcpy(data, "r1");
	else if(!strcasecmp("perrt", data)) strcpy(data, "r2");
	else if(!strcasecmp("pegr", data)) strcpy(data, "r5");
	else if(!strcasecmp("pepsecy", data)) strcpy(data, "r6");
	else if(!strcasecmp("pepseny", data)) strcpy(data, "r7");
	else if(!strcasecmp("symbol", data)) strcpy(data, "s");
	else if(!strcasecmp("sharesowned", data)) strcpy(data, "s1");
	else if(!strcasecmp("shortratio", data)) strcpy(data, "s7");
	else if(!strcasecmp("ltt", data)) strcpy(data, "t1");
	else if(!strcasecmp("tradelinks", data)) strcpy(data, "t6");
	else if(!strcasecmp("tt", data)) strcpy(data, "t7");
	else if(!strcasecmp("1ytp", data)) strcpy(data, "t8");
	else if(!strcasecmp("volume", data)) strcpy(data, "v");
	else if(!strcasecmp("hv", data)) strcpy(data, "v1");
	else if(!strcasecmp("hvrt", data)) strcpy(data, "v7");
	else if(!strcasecmp("52weekrange", data)) strcpy(data, "w");
	else if(!strcasecmp("dvc", data)) strcpy(data, "w1");
	else if(!strcasecmp("dvcrt", data)) strcpy(data, "w4");
	else if(!strcasecmp("se", data)) strcpy(data, "x");
	else if(!strcasecmp("dy", data)) strcpy(data, "y");
	else {
		NORM_ERR("\"%s\" is not supported by $stock. Supported: 1ytp, 200ma, 50ma, 52weeklow, 52weekhigh, 52weekrange, adv, ag, ahcrt, ask, askrt, asksize, bid, bidrt, bidsize, bookvalue, c200ma, c50ma, c52whigh, c52wlow, change, changert, cip, commission, cprt, dayshigh, dayslow, dpd, dr, drrt, ds, dvc, dvcrt, dy, ebitda, edv, ei, epsecy, epsenq, epseny, es, floatshares, hg, hgp, hgprt, hl, hv, hvrt, ll, ltd, ltp, lts, ltt, mc, mcrt, moreinfo, name, notes, obrt, open, pb, pc, pc200ma, pc50ma, pc52whigh, pc52wlow, pegr, pepsecy, pepseny, per, perrt, pricepaid, ps, se, sharesowned, shortratio, symbol, tradedate, tradelinks, tt, volume", data);
		return;
	}
#define MAX_FINYAH_URL_LENGTH 64
	obj->data.s = (char*) malloc(MAX_FINYAH_URL_LENGTH);
	snprintf(obj->data.s, MAX_FINYAH_URL_LENGTH, "http://download.finance.yahoo.com/d/quotes.csv?s=%s&f=%s", stock, data);
}
#endif /* BUILD_CURL */

legacy_cb_handle *create_cb_handle(int (*fn)())
{
	if(fn)
		return new legacy_cb_handle(conky::register_cb<legacy_cb>(1, fn));
	else
		return NULL;
}

/* construct_text_object() creates a new text_object */
struct text_object *construct_text_object(char *s, const char *arg,
		long line, void **ifblock_opaque, void *free_at_crash)
{
	// struct text_object *obj = new_text_object();
	struct text_object *obj = new_text_object_internal();

	obj->line = line;

/* helper defines for internal use only */
#define __OBJ_HEAD(a, n) if (!strcmp(s, #a)) { \
	obj->cb_handle = create_cb_handle(n);
#define __OBJ_IF obj_be_ifblock_if(ifblock_opaque, obj)
#define __OBJ_ARG(...) if (!arg) { free(s); CRIT_ERR(obj, free_at_crash, __VA_ARGS__); }

/* defines to be used below */
#define OBJ(a, n) __OBJ_HEAD(a, n) {
#define OBJ_ARG(a, n, ...) __OBJ_HEAD(a, n) __OBJ_ARG(__VA_ARGS__) {
#define OBJ_IF(a, n) __OBJ_HEAD(a, n) __OBJ_IF; {
#define OBJ_IF_ARG(a, n, ...) __OBJ_HEAD(a, n) __OBJ_ARG(__VA_ARGS__) __OBJ_IF; {
#define END } } else

#ifdef BUILD_X11
	if (s[0] == '#') {
		obj->data.l = get_x11_color(s);
		obj->callbacks.print = &new_fg;
	} else
#endif /* BUILD_X11 */
#ifndef __OpenBSD__
	OBJ(acpitemp, 0)
		obj->data.i = open_acpi_temperature(arg);
		obj->callbacks.print = &print_acpitemp;
		obj->callbacks.free = &free_acpitemp;
	END OBJ(acpiacadapter, 0)
		if(arg) {
#ifdef __linux__
			if(strpbrk(arg, "/.") != NULL) {
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
		obj->callbacks.print = &print_freq;
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
		obj->callbacks.print = &print_freq_g;
	END OBJ_ARG(read_tcp, 0, "read_tcp: Needs \"(host) port\" as argument(s)")
		parse_read_tcpip_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_read_tcp;
		obj->callbacks.free = &free_read_tcpip;
	END OBJ_ARG(read_udp, 0, "read_udp: Needs \"(host) port\" as argument(s)")
		parse_read_tcpip_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_read_udp;
		obj->callbacks.free = &free_read_tcpip;
	END OBJ_ARG(tcp_ping, 0, "tcp_ping: Needs \"host (port)\" as argument(s)")
		parse_tcp_ping_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_tcp_ping;
		obj->callbacks.free = &free_tcp_ping;
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
		obj->callbacks.print = &print_voltage_mv;
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
		obj->callbacks.print = &print_voltage_v;

#ifdef BUILD_WLAN
	END OBJ(wireless_essid, &update_net_stats)
		obj->data.opaque = get_net_stat(arg, obj, free_at_crash);
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

#endif /* __linux__ */

#ifndef __OpenBSD__
	END OBJ(acpifan, 0)
		obj->callbacks.print = &print_acpifan;
	END OBJ(battery, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size.get(*state));
		obj->callbacks.print = &print_battery;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ(battery_short, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size.get(*state));
		obj->callbacks.print = &print_battery_short;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ(battery_time, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size.get(*state));
		obj->callbacks.print = &print_battery_time;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ(battery_percent, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size.get(*state));
		obj->callbacks.percentage = &battery_percentage;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ(battery_bar, 0)
		char bat[64];

		arg = scan_bar(obj, arg, 100);
		if (arg && strlen(arg)>0) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size.get(*state));
		obj->callbacks.barval = &get_battery_perct_bar;
		obj->callbacks.free = &gen_free_opaque;
#endif /* !__OpenBSD__ */

#if defined(__linux__)
	END OBJ_ARG(disk_protect, 0, "disk_protect needs an argument")
		obj->data.s = strndup(dev_name(arg), text_buffer_size.get(*state));
		obj->callbacks.print = &print_disk_protect_queue;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ(i8k_version, &update_i8k)
		obj->callbacks.print = &print_i8k_version;
	END OBJ(i8k_bios, &update_i8k)
		obj->callbacks.print = &print_i8k_bios;
	END OBJ(i8k_serial, &update_i8k)
		obj->callbacks.print = &print_i8k_serial;
	END OBJ(i8k_cpu_temp, &update_i8k)
		obj->callbacks.print = &print_i8k_cpu_temp;
	END OBJ(i8k_left_fan_status, &update_i8k)
		obj->callbacks.print = &print_i8k_left_fan_status;
	END OBJ(i8k_right_fan_status, &update_i8k)
		obj->callbacks.print = &print_i8k_right_fan_status;
	END OBJ(i8k_left_fan_rpm, &update_i8k)
		obj->callbacks.print = &print_i8k_left_fan_rpm;
	END OBJ(i8k_right_fan_rpm, &update_i8k)
		obj->callbacks.print = &print_i8k_right_fan_rpm;
	END OBJ(i8k_ac_status, &update_i8k)
		obj->callbacks.print = &print_i8k_ac_status;
	END OBJ(i8k_buttons_status, &update_i8k)
		obj->callbacks.print = &print_i8k_buttons_status;
#if defined(BUILD_IBM)
	END OBJ(ibm_fan, 0)
		obj->callbacks.print = &get_ibm_acpi_fan;
	END OBJ_ARG(ibm_temps, &get_ibm_acpi_temps, "ibm_temps: needs an argument")
		parse_ibm_temps_arg(obj, arg);
		obj->callbacks.print = &print_ibm_temps;
	END OBJ(ibm_volume, 0)
		obj->callbacks.print = &get_ibm_acpi_volume;
	END OBJ(ibm_brightness, 0)
		obj->callbacks.print = &get_ibm_acpi_brightness;
	END OBJ(ibm_thinklight, 0)
		obj->callbacks.print = &get_ibm_acpi_thinklight;
#endif
	/* information from sony_laptop kernel module
	 * /sys/devices/platform/sony-laptop */
	END OBJ(sony_fanspeed, 0)
		obj->callbacks.print = &get_sony_fanspeed;
	END OBJ_IF(if_gw, &update_gateway_info)
		obj->callbacks.iftest = &gateway_exists;
		obj->callbacks.free = &free_gateway_info;
	END OBJ_ARG(ioscheduler, 0, "get_ioscheduler needs an argument (e.g. hda)")
		obj->data.s = strndup(dev_name(arg), text_buffer_size.get(*state));
		obj->callbacks.print = &print_ioscheduler;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ(laptop_mode, 0)
		obj->callbacks.print = &print_laptop_mode;
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
		obj->callbacks.print = get_powerbook_batt_info;
#endif /* __linux__ */
#if (defined(__FreeBSD__) || defined(__linux__) || defined(__DragonFly__))
	END OBJ_IF_ARG(if_up, 0, "if_up needs an argument")
		parse_if_up_arg(obj, arg);
		obj->callbacks.iftest = &interface_up;
		obj->callbacks.free = &free_if_up;
#endif
#if defined(__OpenBSD__)
	END OBJ_ARG(obsd_sensors_temp, 0, "obsd_sensors_temp: needs an argument")
		parse_obsd_sensor(obj, arg);
		obj->callbacks.print = &print_obsd_sensors_temp;
	END OBJ_ARG(obsd_sensors_fan, 0, "obsd_sensors_fan: needs 2 arguments (device and sensor number)")
		parse_obsd_sensor(obj, arg);
		obj->callbacks.print = &print_obsd_sensors_fan;
	END OBJ_ARG(obsd_sensors_volt, 0, "obsd_sensors_volt: needs 2 arguments (device and sensor number)")
		parse_obsd_sensor(obj, arg);
		obj->callbacks.print = &print_obsd_sensors_volt;
	END OBJ(obsd_vendor, 0)
		obj->callbacks.print = &get_obsd_vendor;
	END OBJ(obsd_product, 0)
		obj->callbacks.print = &get_obsd_product;
#endif /* __OpenBSD__ */
	END OBJ(buffers, &update_meminfo)
		obj->callbacks.print = &print_buffers;
	END OBJ(cached, &update_meminfo)
		obj->callbacks.print = &print_cached;
#define SCAN_CPU(__arg, __var) { \
	int __offset = 0; \
	if (__arg && sscanf(__arg, " cpu%d %n", &__var, &__offset) > 0) \
		__arg += __offset; \
	else \
		__var = 0; \
}
	END OBJ(cpu, &update_cpu_usage)
		SCAN_CPU(arg, obj->data.i);
		obj->callbacks.percentage = &cpu_percentage;
		DBGP2("Adding $cpu for CPU %d", obj->data.i);
#ifdef BUILD_X11
	END OBJ(cpugauge, &update_cpu_usage)
		SCAN_CPU(arg, obj->data.i);
		scan_gauge(obj, arg, 1);
		obj->callbacks.gaugeval = &cpu_barval;
		DBGP2("Adding $cpugauge for CPU %d", obj->data.i);
#endif
	END OBJ(cpubar, &update_cpu_usage)
		SCAN_CPU(arg, obj->data.i);
		scan_bar(obj, arg, 1);
		obj->callbacks.barval = &cpu_barval;
		DBGP2("Adding $cpubar for CPU %d", obj->data.i);
#ifdef BUILD_X11
	END OBJ(cpugraph, &update_cpu_usage)
		char *buf = 0;
		SCAN_CPU(arg, obj->data.i);
		buf = scan_graph(obj, arg, 1);
		DBGP2("Adding $cpugraph for CPU %d", obj->data.i);
		free_and_zero(buf);
		obj->callbacks.graphval = &cpu_barval;
	END OBJ(loadgraph, &update_load_average)
		scan_loadgraph_arg(obj, arg);
		obj->callbacks.graphval = &loadgraphval;
#endif /* BUILD_X11 */
	END OBJ(diskio, &update_diskio)
		parse_diskio_arg(obj, arg);
		obj->callbacks.print = &print_diskio;
	END OBJ(diskio_read, &update_diskio)
		parse_diskio_arg(obj, arg);
		obj->callbacks.print = &print_diskio_read;
	END OBJ(diskio_write, &update_diskio)
		parse_diskio_arg(obj, arg);
		obj->callbacks.print = &print_diskio_write;
#ifdef BUILD_X11
	END OBJ(diskiograph, &update_diskio)
		parse_diskiograph_arg(obj, arg);
		obj->callbacks.graphval = &diskiographval;
	END OBJ(diskiograph_read, &update_diskio)
		parse_diskiograph_arg(obj, arg);
		obj->callbacks.graphval = &diskiographval_read;
	END OBJ(diskiograph_write, &update_diskio)
		parse_diskiograph_arg(obj, arg);
		obj->callbacks.graphval = &diskiographval_write;
#endif /* BUILD_X11 */
	END OBJ(color, 0)
#ifdef BUILD_X11
		if (out_to_x.get(*state)) {
			obj->data.l = arg ? get_x11_color(arg) : default_color.get(*state);
			set_current_text_color(obj->data.l);
		}
#endif /* BUILD_X11 */
#ifdef BUILD_NCURSES
		if (out_to_ncurses.get(*state)) {
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
#endif /* BUILD_NCURSES */
		obj->callbacks.print = &new_fg;
#ifdef BUILD_X11
	END OBJ(color0, 0)
		obj->data.l = color[0].get(*state);
		set_current_text_color(obj->data.l);
		obj->callbacks.print = &new_fg;
	END OBJ(color1, 0)
		obj->data.l = color[1].get(*state);
		set_current_text_color(obj->data.l);
		obj->callbacks.print = &new_fg;
	END OBJ(color2, 0)
		obj->data.l = color[2].get(*state);
		set_current_text_color(obj->data.l);
		obj->callbacks.print = &new_fg;
	END OBJ(color3, 0)
		obj->data.l = color[3].get(*state);
		set_current_text_color(obj->data.l);
		obj->callbacks.print = &new_fg;
	END OBJ(color4, 0)
		obj->data.l = color[4].get(*state);
		set_current_text_color(obj->data.l);
		obj->callbacks.print = &new_fg;
	END OBJ(color5, 0)
		obj->data.l = color[5].get(*state);
		set_current_text_color(obj->data.l);
		obj->callbacks.print = &new_fg;
	END OBJ(color6, 0)
		obj->data.l = color[6].get(*state);
		set_current_text_color(obj->data.l);
		obj->callbacks.print = &new_fg;
	END OBJ(color7, 0)
		obj->data.l = color[7].get(*state);
		set_current_text_color(obj->data.l);
		obj->callbacks.print = &new_fg;
	END OBJ(color8, 0)
		obj->data.l = color[8].get(*state);
		set_current_text_color(obj->data.l);
		obj->callbacks.print = &new_fg;
	END OBJ(color9, 0)
		obj->data.l = color[9].get(*state);
		set_current_text_color(obj->data.l);
		obj->callbacks.print = &new_fg;
	END OBJ(font, 0)
		scan_font(obj, arg);
		obj->callbacks.print = &new_font;
		obj->callbacks.free = &gen_free_opaque;
#endif /* BUILD_X11 */
	END OBJ(conky_version, 0)
		obj_be_plain_text(obj, VERSION);
	END OBJ(conky_build_date, 0)
		obj_be_plain_text(obj, BUILD_DATE);
	END OBJ(conky_build_arch, 0)
		obj_be_plain_text(obj, BUILD_ARCH);
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
	END OBJ(else, 0)
		obj_be_ifblock_else(ifblock_opaque, obj);
		obj->callbacks.iftest = &gen_false_iftest;
	END OBJ(endif, 0)
		obj_be_ifblock_endif(ifblock_opaque, obj);
		obj->callbacks.print = &gen_print_nothing;
	END OBJ(eval, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size.get(*state));
		obj->callbacks.print = &print_evaluate;
		obj->callbacks.free = &gen_free_opaque;
#if defined(BUILD_IMLIB2) && defined(BUILD_X11)
	END OBJ(image, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size.get(*state));
		obj->callbacks.print = &print_image_callback;
		obj->callbacks.free = &gen_free_opaque;
#endif /* BUILD_IMLIB2 */
#ifdef BUILD_MYSQL
	END OBJ_ARG(mysql, 0, "mysql needs a query")
		obj->data.s = strdup(arg);
		obj->callbacks.print = &print_mysql;
#endif /* BUILD_MYSQL */
	END OBJ_ARG(no_update, 0, "no_update needs arguments")
		scan_no_update(obj, arg);
		obj->callbacks.print = &print_no_update;
		obj->callbacks.free = &free_no_update;
	END OBJ(exec, 0)
		scan_exec_arg(obj, arg);
		obj->parse = false;
		obj->thread = false;
		obj->callbacks.print = &print_exec;
		obj->callbacks.free = &free_exec;
	END OBJ(execp, 0)
		scan_exec_arg(obj, arg);
		obj->parse = true;
		obj->thread = false;
		obj->callbacks.print = &print_exec;
		obj->callbacks.free = &free_exec;
	END OBJ(execbar, 0)
		scan_exec_arg(obj, arg);
		obj->callbacks.barval = &execbarval;
		obj->callbacks.free = &free_exec;
	END OBJ(execgauge, 0)
		scan_exec_arg(obj, arg);
		obj->callbacks.gaugeval = &execbarval;
		obj->callbacks.free = &free_exec;
#ifdef BUILD_X11
	END OBJ(execgraph, 0)
		scan_execgraph_arg(obj, arg);
		obj->callbacks.graphval = &execbarval;
		obj->callbacks.free = &free_exec;
#endif /* BUILD_X11 */
	END OBJ_ARG(execibar, 0, "execibar needs arguments")
		scan_execi_bar_arg(obj, arg);
		obj->callbacks.barval = &execi_barval;
		obj->callbacks.free = &free_execi;
#ifdef BUILD_X11
	END OBJ_ARG(execigraph, 0, "execigraph needs arguments")
		scan_execgraph_arg(obj, arg);
		obj->callbacks.graphval = &execi_barval;
		obj->callbacks.free = &free_execi;
	END OBJ_ARG(execigauge, 0, "execigauge needs arguments")
		scan_execi_gauge_arg(obj, arg);
		obj->callbacks.gaugeval = &execi_barval;
		obj->callbacks.free = &free_execi;
#endif /* BUILD_X11 */
	END OBJ_ARG(execi, 0, "execi needs arguments")
		scan_execi_arg(obj, arg);
		obj->parse = false;
		obj->thread = false;
		obj->callbacks.print = &print_execi;
		obj->callbacks.free = &free_execi;
	END OBJ_ARG(execpi, 0, "execpi needs arguments")
		scan_execi_arg(obj, arg);
		obj->parse = true;
		obj->thread = false;
		obj->callbacks.print = &print_execi;
		obj->callbacks.free = &free_execi;
	END OBJ_ARG(texeci, 0, "texeci needs arguments")
		scan_execi_arg(obj, arg);
		obj->parse = false;
		obj->thread = true;
		obj->callbacks.print = &print_execi;
		obj->callbacks.free = &free_execi;
	END OBJ_ARG(texecpi, 0, "texecpi needs arguments")
		scan_execi_arg(obj, arg);
		obj->parse = true;
		obj->thread = true;
		obj->callbacks.print = &print_execi;
		obj->callbacks.free = &free_execi;
	END OBJ(fs_bar, &update_fs_stats)
		init_fs_bar(obj, arg);
		obj->callbacks.barval = &fs_barval;
	END OBJ(fs_bar_free, &update_fs_stats)
		init_fs_bar(obj, arg);
		obj->callbacks.barval = &fs_free_barval;
	END OBJ(fs_free, &update_fs_stats)
		init_fs(obj, arg);
		obj->callbacks.print = &print_fs_free;
	END OBJ(fs_used_perc, &update_fs_stats)
		init_fs(obj, arg);
		obj->callbacks.percentage = &fs_used_percentage;
	END OBJ(fs_free_perc, &update_fs_stats)
		init_fs(obj, arg);
		obj->callbacks.percentage = &fs_free_percentage;
	END OBJ(fs_size, &update_fs_stats)
		init_fs(obj, arg);
		obj->callbacks.print = &print_fs_size;
	END OBJ(fs_type, &update_fs_stats)
		init_fs(obj, arg);
		obj->callbacks.print = &print_fs_type;
	END OBJ(fs_used, &update_fs_stats)
		init_fs(obj, arg);
		obj->callbacks.print = &print_fs_used;
#ifdef BUILD_X11
	END OBJ(hr, 0)
		obj->data.l = arg ? atoi(arg) : 1;
		obj->callbacks.print = &new_hr;
#endif /* BUILD_X11 */
	END OBJ(nameserver, &update_dns_data)
		parse_nameserver_arg(obj, arg);
		obj->callbacks.print = &print_nameserver;
		obj->callbacks.free = &free_dns_data;
	END OBJ(offset, 0)
		obj->data.l = arg ? atoi(arg) : 1;
		obj->callbacks.print = &new_offset;
	END OBJ(voffset, 0)
		obj->data.l = arg ? atoi(arg) : 1;
		obj->callbacks.print = &new_voffset;
	END OBJ_ARG(goto, 0, "goto needs arguments")
		obj->data.l = atoi(arg);
		obj->callbacks.print = &new_goto;
#ifdef BUILD_X11
	END OBJ(tab, 0)
		scan_tab(obj, arg);
		obj->callbacks.print = &new_tab;
#endif /* BUILD_X11 */
#ifdef __linux__
	END OBJ_ARG(i2c, 0, "i2c needs arguments")
		parse_i2c_sensor(obj, arg);
		obj->callbacks.print = &print_sysfs_sensor;
		obj->callbacks.free = &free_sysfs_sensor;
	END OBJ_ARG(platform, 0, "platform needs arguments")
		parse_platform_sensor(obj, arg);
		obj->callbacks.print = &print_sysfs_sensor;
		obj->callbacks.free = &free_sysfs_sensor;
	END OBJ_ARG(hwmon, 0, "hwmon needs argumanets")
		parse_hwmon_sensor(obj, arg);
		obj->callbacks.print = &print_sysfs_sensor;
		obj->callbacks.free = &free_sysfs_sensor;
#endif /* __linux__ */
	END
	/* we have four different types of top (top, top_mem, top_time and top_io). To
	 * avoid having almost-same code four times, we have this special
	 * handler. */
	/* XXX: maybe fiddle them apart later, as print_top() does
	 * nothing else than just that, using an ugly switch(). */
	if (strncmp(s, "top", 3) == EQUAL) {
		if (parse_top_args(s, arg, obj)) {
#ifdef __linux__
			determine_longstat_file();
#endif
			obj->cb_handle = create_cb_handle(update_top);
		} else {
			free(obj);
			return NULL;
		}
	} else
#ifdef __linux__
	OBJ(addr, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_addr;
	END OBJ(addrs, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_addrs;
#ifdef BUILD_IPV6
	END OBJ(v6addrs, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_v6addrs;
#endif /* BUILD_IPV6 */
	END
#endif /* __linux__ */
	OBJ_ARG(tail, 0, "tail needs arguments")
		init_tailhead("tail", arg, obj, free_at_crash);
		obj->callbacks.print = &print_tail;
		obj->callbacks.free = &free_tailhead;
	END OBJ_ARG(head, 0, "head needs arguments")
		init_tailhead("head", arg, obj, free_at_crash);
		obj->callbacks.print = &print_head;
		obj->callbacks.free = &free_tailhead;
	END OBJ_ARG(lines, 0, "lines needs an argument")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.print = &print_lines;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_ARG(words, 0, "words needs a argument")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.print = &print_words;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ(loadavg, &update_load_average)
		scan_loadavg_arg(obj, arg);
		obj->callbacks.print = &print_loadavg;
	END OBJ_IF_ARG(if_empty, 0, "if_empty needs an argument")
		obj->sub = (text_object*)malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
		obj->callbacks.iftest = &if_empty_iftest;
	END OBJ_IF_ARG(if_match, 0, "if_match needs arguments")
		obj->sub = (text_object*)malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
		obj->callbacks.iftest = &check_if_match;
	END OBJ_IF_ARG(if_existing, 0, "if_existing needs an argument or two")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.iftest = &if_existing_iftest;
		obj->callbacks.free = &gen_free_opaque;
#ifdef __linux__
	END OBJ_IF_ARG(if_mounted, 0, "if_mounted needs an argument")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.iftest = &check_mount;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_IF_ARG(if_running, &update_top, "if_running needs an argument")
		top_running = 1;
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.iftest = &if_running_iftest;
		obj->callbacks.free = &gen_free_opaque;
#else
	END OBJ_IF_ARG(if_running, 0, "if_running needs an argument")
		char buf[text_buffer_size.get(*state)];

		snprintf(buf, text_buffer_size.get(*state), "pidof %s >/dev/null", arg);
		obj->data.s = strndup(buf, text_buffer_size.get(*state));
		/* XXX: maybe use a different callback here */
		obj->callbacks.iftest = &if_running_iftest;
#endif
	END OBJ(kernel, 0)
		obj->callbacks.print = &print_kernel;
	END OBJ(machine, 0)
		obj->callbacks.print = &print_machine;
#if defined(__DragonFly__)
    END OBJ(version, 0)
		obj->callbacks.print = &print_version;
#endif
	END OBJ(mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(new_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_new_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(seen_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_seen_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(unseen_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_unseen_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(flagged_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_flagged_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(unflagged_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_unflagged_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(forwarded_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_forwarded_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(unforwarded_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_unforwarded_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(replied_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_replied_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(unreplied_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_unreplied_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(draft_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_draft_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(trashed_mails, 0)
		parse_local_mail_args(obj, arg);
		obj->callbacks.print = &print_trashed_mails;
		obj->callbacks.free = &free_local_mails;
	END OBJ(mboxscan, 0)
		parse_mboxscan_arg(obj, arg);
		obj->callbacks.print = &print_mboxscan;
		obj->callbacks.free = &free_mboxscan;
	END OBJ(mem, &update_meminfo)
		obj->callbacks.print = &print_mem;
	END OBJ(memwithbuffers, &update_meminfo)
		obj->callbacks.print = &print_memwithbuffers;
	END OBJ(memeasyfree, &update_meminfo)
		obj->callbacks.print = &print_memeasyfree;
	END OBJ(memfree, &update_meminfo)
		obj->callbacks.print = &print_memfree;
	END OBJ(memmax, &update_meminfo)
		obj->callbacks.print = &print_memmax;
	END OBJ(memperc, &update_meminfo)
		obj->callbacks.percentage = &mem_percentage;
#ifdef __linux__
	END OBJ(memdirty, &update_meminfo)
		obj->callbacks.print = &print_memdirty;
#endif
#ifdef BUILD_X11
	END OBJ(memgauge, &update_meminfo)
		scan_gauge(obj, arg, 1);
		obj->callbacks.gaugeval = &mem_barval;
#endif
	END OBJ(membar, &update_meminfo)
		scan_bar(obj, arg, 1);
		obj->callbacks.barval = &mem_barval;
	END OBJ(memwithbuffersbar, &update_meminfo)
		scan_bar(obj, arg, 1);
		obj->callbacks.barval = &mem_with_buffers_barval;
#ifdef BUILD_X11
	END OBJ(memgraph, &update_meminfo)
		char *buf = 0;
		buf = scan_graph(obj, arg, 1);

		free_and_zero(buf);
		obj->callbacks.graphval = &mem_barval;
#endif /* BUILD_X11*/
	END OBJ(mixer, 0)
		parse_mixer_arg(obj, arg);
		obj->callbacks.percentage = &mixer_percentage;
	END OBJ(mixerl, 0)
		parse_mixer_arg(obj, arg);
		obj->callbacks.percentage = &mixerl_percentage;
	END OBJ(mixerr, 0)
		parse_mixer_arg(obj, arg);
		obj->callbacks.percentage = &mixerr_percentage;
	END OBJ(mixerbar, 0)
		scan_mixer_bar(obj, arg);
		obj->callbacks.barval = &mixer_barval;
	END OBJ(mixerlbar, 0)
		scan_mixer_bar(obj, arg);
		obj->callbacks.barval = &mixerl_barval;
	END OBJ(mixerrbar, 0)
		scan_mixer_bar(obj, arg);
		obj->callbacks.barval = &mixerr_barval;
	END OBJ_IF(if_mixer_mute, 0)
		parse_mixer_arg(obj, arg);
		obj->callbacks.iftest = &check_mixer_muted;
#ifdef BUILD_X11
	END OBJ(monitor, 0)
		obj->callbacks.print = &print_monitor;
	END OBJ(monitor_number, 0)
		obj->callbacks.print = &print_monitor_number;
	END OBJ(desktop, 0)
		obj->callbacks.print = &print_desktop;
	END OBJ(desktop_number, 0)
		obj->callbacks.print = &print_desktop_number;
	END OBJ(desktop_name, 0)
		obj->callbacks.print = &print_desktop_name;
#endif /* BUILD_X11 */
	END OBJ_ARG(format_time, 0, "format_time needs a pid as argument")
		obj->sub = (text_object*)malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
		obj->callbacks.print = &print_format_time;
	END OBJ(nodename, 0)
		obj->callbacks.print = &print_nodename;
	END OBJ(nodename_short, 0)
		obj->callbacks.print = &print_nodename_short;
	END OBJ_ARG(cmdline_to_pid, 0, "cmdline_to_pid needs a command line as argument")
		scan_cmdline_to_pid_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_cmdline_to_pid;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_ARG(pid_chroot, 0, "pid_chroot needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_chroot;
	END OBJ_ARG(pid_cmdline, 0, "pid_cmdline needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_cmdline;
	END OBJ_ARG(pid_cwd, 0, "pid_cwd needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_cwd;
	END OBJ_ARG(pid_environ, 0, "pid_environ needs arguments")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_environ;
	END OBJ_ARG(pid_environ_list, 0, "pid_environ_list needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_environ_list;
	END OBJ_ARG(pid_exe, 0, "pid_exe needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_exe;
	END OBJ_ARG(pid_nice, 0, "pid_nice needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_nice;
	END OBJ_ARG(pid_openfiles, 0, "pid_openfiles needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_openfiles;
	END OBJ_ARG(pid_parent, 0, "pid_parent needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_parent;
	END OBJ_ARG(pid_priority, 0, "pid_priority needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_priority;
	END OBJ_ARG(pid_state, 0, "pid_state needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_state;
	END OBJ_ARG(pid_state_short, 0, "pid_state_short needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_state_short;
	END OBJ_ARG(pid_stderr, 0, "pid_stderr needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_stderr;
	END OBJ_ARG(pid_stdin, 0, "pid_stdin needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_stdin;
	END OBJ_ARG(pid_stdout, 0, "pid_stdout needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_stdout;
	END OBJ_ARG(pid_threads, 0, "pid_threads needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_threads;
	END OBJ_ARG(pid_thread_list, 0, "pid_thread_list needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_thread_list;
	END OBJ_ARG(pid_time_kernelmode, 0, "pid_time_kernelmode needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_time_kernelmode;
	END OBJ_ARG(pid_time_usermode, 0, "pid_time_usermode needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_time_usermode;
	END OBJ_ARG(pid_time, 0, "pid_time needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_time;
	END OBJ_ARG(pid_uid, 0, "pid_uid needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_uid;
	END OBJ_ARG(pid_euid, 0, "pid_euid needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_euid;
	END OBJ_ARG(pid_suid, 0, "pid_suid needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_suid;
	END OBJ_ARG(pid_fsuid, 0, "pid_fsuid needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_fsuid;
	END OBJ_ARG(pid_gid, 0, "pid_gid needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_gid;
	END OBJ_ARG(pid_egid, 0, "pid_egid needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_egid;
	END OBJ_ARG(pid_sgid, 0, "pid_sgid needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_sgid;
	END OBJ_ARG(pid_fsgid, 0, "pid_fsgid needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_fsgid;
	END OBJ_ARG(gid_name, 0, "gid_name needs a gid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_gid_name;
	END OBJ_ARG(uid_name, 0, "uid_name needs a uid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_uid_name;
	END OBJ_ARG(pid_read, 0, "pid_read needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_read;
	END OBJ_ARG(pid_vmpeak, 0, "pid_vmpeak needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_vmpeak;
	END OBJ_ARG(pid_vmsize, 0, "pid_vmsize needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_vmsize;
	END OBJ_ARG(pid_vmlck, 0, "pid_vmlck needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_vmlck;
	END OBJ_ARG(pid_vmhwm, 0, "pid_vmhwm needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_vmhwm;
	END OBJ_ARG(pid_vmrss, 0, "pid_vmrss needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_vmrss;
	END OBJ_ARG(pid_vmdata, 0, "pid_vmdata needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_vmdata;
	END OBJ_ARG(pid_vmstk, 0, "pid_vmstk needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_vmstk;
	END OBJ_ARG(pid_vmexe, 0, "pid_vmexe needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_vmexe;
	END OBJ_ARG(pid_vmlib, 0, "pid_vmlib needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_vmlib;
	END OBJ_ARG(pid_vmpte, 0, "pid_vmpte needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_vmpte;
	END OBJ_ARG(pid_write, 0, "pid_write needs a pid as argument")
		extract_object_args_to_sub(obj, arg);
		obj->callbacks.print = &print_pid_write;
#ifdef __DragonFly__
	END OBJ(processes, &update_top)
#else
	END OBJ(processes, &update_total_processes)
#endif
		obj->callbacks.print = &print_processes;
#ifdef __linux__
	END OBJ(distribution, 0)
		obj->callbacks.print = &print_distribution;
	END OBJ(running_processes, &update_top)
		top_running = 1;
		obj->callbacks.print = &print_running_processes;
	END OBJ(threads, &update_threads)
		obj->callbacks.print = &print_threads;
	END OBJ(running_threads, &update_stat)
		obj->callbacks.print = &print_running_threads;
#else
#if defined(__DragonFly__)
	END OBJ(running_processes, &update_top)
	obj->callbacks.print = &print_running_processes;
#else
	END OBJ(running_processes, &update_running_processes)
	obj->callbacks.print = &print_running_processes;
#endif
#endif /* __linux__ */
	END OBJ(shadecolor, 0)
#ifdef BUILD_X11
		obj->data.l = arg ? get_x11_color(arg) : default_shade_color.get(*state);
		obj->callbacks.print = &new_bg;
#endif /* BUILD_X11 */
	END OBJ(outlinecolor, 0)
#ifdef BUILD_X11
		obj->data.l = arg ? get_x11_color(arg) : default_outline_color.get(*state);
		obj->callbacks.print = &new_outline;
#endif /* BUILD_X11 */
	END OBJ(stippled_hr, 0)
#ifdef BUILD_X11
		scan_stippled_hr(obj, arg);
		obj->callbacks.print = &new_stippled_hr;
#endif /* BUILD_X11 */
	END OBJ(swap, &update_meminfo)
		obj->callbacks.print = &print_swap;
	END OBJ(swapfree, &update_meminfo)
		obj->callbacks.print = &print_swapfree;
	END OBJ(swapmax, &update_meminfo)
		obj->callbacks.print = &print_swapmax;
	END OBJ(swapperc, &update_meminfo)
		obj->callbacks.percentage = &swap_percentage;
	END OBJ(swapbar, &update_meminfo)
		scan_bar(obj, arg, 1);
		obj->callbacks.barval = &swap_barval;
	/* XXX: swapgraph, swapgauge? */
	END OBJ(sysname, 0)
		obj->callbacks.print = &print_sysname;
	END OBJ(time, 0)
		scan_time(obj, arg);
		obj->callbacks.print = &print_time;
		obj->callbacks.free = &free_time;
	END OBJ(utime, 0)
		scan_time(obj, arg);
		obj->callbacks.print = &print_utime;
		obj->callbacks.free = &free_time;
	END OBJ(tztime, 0)
		scan_tztime(obj, arg);
		obj->callbacks.print = &print_tztime;
		obj->callbacks.free = &free_tztime;
#ifdef BUILD_ICAL
	END OBJ_ARG(ical, 0, "ical requires arguments")
		parse_ical_args(obj, arg, free_at_crash, s);
		obj->callbacks.print = &print_ical;
		obj->callbacks.free = &free_ical;
#endif
#ifdef BUILD_IRC
	END OBJ_ARG(irc, 0, "irc requires arguments")
		parse_irc_args(obj, arg);
		obj->callbacks.print = &print_irc;
		obj->callbacks.free = &free_irc;
#endif
#ifdef BUILD_ICONV
	END OBJ_ARG(iconv_start, 0, "Iconv requires arguments")
		init_iconv_start(obj, free_at_crash, arg);
		obj->callbacks.print = &print_iconv_start;
		obj->callbacks.free = &free_iconv;
	END OBJ(iconv_stop, 0)
		init_iconv_stop();
		obj->callbacks.print = &print_iconv_stop;
#endif
	END OBJ(totaldown, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_totaldown;
	END OBJ(totalup, &update_net_stats)
		parse_net_stat_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_totalup;
	END OBJ(updates, 0)
		obj->callbacks.print = &print_updates;
	END OBJ_IF(if_updatenr, 0)
		obj->data.i = arg ? atoi(arg) : 0;
		if(obj->data.i == 0) CRIT_ERR(obj, free_at_crash, "if_updatenr needs a number above 0 as argument");
		set_updatereset(obj->data.i > get_updatereset() ? obj->data.i : get_updatereset());
		obj->callbacks.iftest = &updatenr_iftest;
	END OBJ(alignr, 0)
		obj->data.l = arg ? atoi(arg) : 1;
		obj->callbacks.print = &new_alignr;
	END OBJ(alignc, 0)
		obj->data.l = arg ? atoi(arg) : 0;
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
	END OBJ(uptime_short, &update_uptime)
		obj->callbacks.print = &print_uptime_short;
	END OBJ(uptime, &update_uptime)
		obj->callbacks.print = &print_uptime;
#if defined(__linux__)
	END OBJ(user_names, &update_users)
		obj->callbacks.print = &print_user_names;
		obj->callbacks.free = &free_user_names;
	END OBJ(user_times, &update_users)
		obj->callbacks.print = &print_user_times;
		obj->callbacks.free = &free_user_times;
	END OBJ_ARG(user_time, 0, "user time needs a console name as argument")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.print = &print_user_time;
		obj->callbacks.free = &free_user_time;
	END OBJ(user_terms, &update_users)
		obj->callbacks.print = &print_user_terms;
		obj->callbacks.free = &free_user_terms;
	END OBJ(user_number, &update_users)
		obj->callbacks.print = &print_user_number;
	END OBJ(gw_iface, &update_gateway_info)
		obj->callbacks.print = &print_gateway_iface;
		obj->callbacks.free = &free_gateway_info;
	END OBJ(gw_ip, &update_gateway_info)
		obj->callbacks.print = &print_gateway_ip;
		obj->callbacks.free = &free_gateway_info;
#endif /* !__linux__ */
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
	|| defined(__DragonFly__) \
	|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
	END OBJ(apm_adapter, 0)
		obj->callbacks.print = &print_apm_adapter;
	END OBJ(apm_battery_life, 0)
		obj->callbacks.print = &print_apm_battery_life;
	END OBJ(apm_battery_time, 0)
		obj->callbacks.print = &print_apm_battery_time;
#endif /* __FreeBSD__ */
	END OBJ(imap_unseen, 0)
		parse_imap_mail_args(obj, arg);
		obj->callbacks.print = &print_imap_unseen;
		obj->callbacks.free = &free_mail_obj;
	END OBJ(imap_messages, 0)
		parse_imap_mail_args(obj, arg);
		obj->callbacks.print = &print_imap_messages;
		obj->callbacks.free = &free_mail_obj;
	END OBJ(pop3_unseen, 0)
		parse_pop3_mail_args(obj, arg);
		obj->callbacks.print = &print_pop3_unseen;
		obj->callbacks.free = &free_mail_obj;
	END OBJ(pop3_used, 0)
		parse_pop3_mail_args(obj, arg);
		obj->callbacks.print = &print_pop3_used;
		obj->callbacks.free = &free_mail_obj;
#ifdef BUILD_IBM
	END OBJ_ARG(smapi, 0, "smapi needs an argument")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.print = &print_smapi;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_IF_ARG(if_smapi_bat_installed, 0, "if_smapi_bat_installed needs an argument")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.iftest = &smapi_bat_installed;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_ARG(smapi_bat_perc, 0, "smapi_bat_perc needs an argument")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.percentage = &smapi_bat_percentage;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_ARG(smapi_bat_temp, 0, "smapi_bat_temp needs an argument")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.print = &print_smapi_bat_temp;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_ARG(smapi_bat_power, 0, "smapi_bat_power needs an argument")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.print = &print_smapi_bat_power;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_ARG(smapi_bat_bar, 0, "smapi_bat_bar needs an argument")
		int cnt;
		if(sscanf(arg, "%i %n", &obj->data.i, &cnt) <= 0) {
			NORM_ERR("first argument to smapi_bat_bar must be an integer value");
			obj->data.i = -1;
		} else
			arg = scan_bar(obj, arg + cnt, 100);
		obj->callbacks.barval = &smapi_bat_barval;
#endif /* BUILD_IBM */
#ifdef BUILD_MPD
#define mpd_set_maxlen(name) \
		if (arg) { \
			int i; \
			sscanf(arg, "%d", &i); \
			if (i > 0) \
				obj->data.i = i + 1; \
			else \
				NORM_ERR(#name ": invalid length argument"); \
		}
	END OBJ(mpd_artist, 0)
		mpd_set_maxlen(mpd_artist);
		obj->callbacks.print = &print_mpd_artist;
	END OBJ(mpd_albumartist, 0)
		mpd_set_maxlen(mpd_albumartist);
		obj->callbacks.print = &print_mpd_albumartist;
	END OBJ(mpd_title, 0)
		mpd_set_maxlen(mpd_title);
		obj->callbacks.print = &print_mpd_title;
	END OBJ(mpd_date, 0)
		mpd_set_maxlen(mpd_date);
		obj->callbacks.print = &print_mpd_date;
	END OBJ(mpd_random, 0)
		obj->callbacks.print = &print_mpd_random;
	END OBJ(mpd_repeat, 0)
		obj->callbacks.print = &print_mpd_repeat;
	END OBJ(mpd_elapsed, 0)
		obj->callbacks.print = &print_mpd_elapsed;
	END OBJ(mpd_length, 0)
		obj->callbacks.print = &print_mpd_length;
	END OBJ(mpd_track, 0)
		mpd_set_maxlen(mpd_track);
		obj->callbacks.print = &print_mpd_track;
	END OBJ(mpd_name, 0)
		mpd_set_maxlen(mpd_name);
		obj->callbacks.print = &print_mpd_name;
	END OBJ(mpd_file, 0)
		mpd_set_maxlen(mpd_file);
		obj->callbacks.print = &print_mpd_file;
	END OBJ(mpd_percent, 0)
		obj->callbacks.percentage = &mpd_percentage;
	END OBJ(mpd_album, 0)
		mpd_set_maxlen(mpd_album);
		obj->callbacks.print = &print_mpd_album;
	END OBJ(mpd_vol, 0)
		obj->callbacks.print = &print_mpd_vol;
	END OBJ(mpd_bitrate, 0)
		obj->callbacks.print = &print_mpd_bitrate;
	END OBJ(mpd_status, 0)
		obj->callbacks.print = &print_mpd_status;
	END OBJ(mpd_bar, 0)
		scan_bar(obj, arg, 1);
		obj->callbacks.barval = &mpd_barval;
	END OBJ(mpd_smart, 0)
		mpd_set_maxlen(mpd_smart);
		obj->callbacks.print = &print_mpd_smart;
	END OBJ_IF(if_mpd_playing, 0)
		obj->callbacks.iftest = &check_mpd_playing;
#undef mpd_set_maxlen
#endif /* BUILD_MPD */
#ifdef BUILD_MOC
	END OBJ(moc_state, 0)
		obj->callbacks.print = &print_moc_state;
	END OBJ(moc_file, 0)
		obj->callbacks.print = &print_moc_file;
	END OBJ(moc_title, 0)
		obj->callbacks.print = &print_moc_title;
	END OBJ(moc_artist, 0)
		obj->callbacks.print = &print_moc_artist;
	END OBJ(moc_song, 0)
		obj->callbacks.print = &print_moc_song;
	END OBJ(moc_album, 0)
		obj->callbacks.print = &print_moc_album;
	END OBJ(moc_totaltime, 0)
		obj->callbacks.print = &print_moc_totaltime;
	END OBJ(moc_timeleft, 0)
		obj->callbacks.print = &print_moc_timeleft;
	END OBJ(moc_curtime, 0)
		obj->callbacks.print = &print_moc_curtime;
	END OBJ(moc_bitrate, 0)
		obj->callbacks.print = &print_moc_bitrate;
	END OBJ(moc_rate, 0)
		obj->callbacks.print = &print_moc_rate;
#endif /* BUILD_MOC */
#ifdef BUILD_CMUS
	END OBJ(cmus_state, 0)
		obj->callbacks.print = &print_cmus_state;
	END OBJ(cmus_file, 0)
		obj->callbacks.print = &print_cmus_file;
	END OBJ(cmus_title, 0)
		obj->callbacks.print = &print_cmus_title;
	END OBJ(cmus_artist, 0)
		obj->callbacks.print = &print_cmus_artist;
	END OBJ(cmus_album, 0)
		obj->callbacks.print = &print_cmus_album;
	END OBJ(cmus_totaltime, 0)
		obj->callbacks.print = &print_cmus_totaltime;
	END OBJ(cmus_timeleft, 0)
		obj->callbacks.print = &print_cmus_timeleft;
	END OBJ(cmus_curtime, 0)
		obj->callbacks.print = &print_cmus_curtime;
	END OBJ(cmus_random, 0)
		obj->callbacks.print = &print_cmus_random;
	END OBJ(cmus_state, 0)
		obj->callbacks.print = &print_cmus_state;
	END OBJ(cmus_file, 0)
		obj->callbacks.print = &print_cmus_file;
	END OBJ(cmus_title, 0)
		obj->callbacks.print = &print_cmus_title;
	END OBJ(cmus_artist, 0)
		obj->callbacks.print = &print_cmus_artist;
	END OBJ(cmus_album, 0)
		obj->callbacks.print = &print_cmus_album;
	END OBJ(cmus_totaltime, 0)
		obj->callbacks.print = &print_cmus_totaltime;
	END OBJ(cmus_timeleft, 0)
		obj->callbacks.print = &print_cmus_timeleft;
	END OBJ(cmus_curtime, 0)
		obj->callbacks.print = &print_cmus_curtime;
	END OBJ(cmus_random, 0)
		obj->callbacks.print = &print_cmus_random;
	END OBJ(cmus_repeat, 0)
		obj->callbacks.print = &print_cmus_repeat;
	END OBJ(cmus_aaa, 0)
		obj->callbacks.print = &print_cmus_aaa;
	END OBJ(cmus_track, 0)
		obj->callbacks.print = &print_cmus_track;
	END OBJ(cmus_genre, 0)
		obj->callbacks.print = &print_cmus_genre;
	END OBJ(cmus_date, 0)
		obj->callbacks.print = &print_cmus_date;
	END OBJ(cmus_progress, 0)
		scan_bar(obj, arg, 1);
		obj->callbacks.barval = &cmus_progress;
	END OBJ(cmus_percent, 0)
		obj->callbacks.percentage = &cmus_percent;
#endif /* BUILD_CMUS */
#ifdef BUILD_XMMS2
	END OBJ(xmms2_artist, &update_xmms2)
		obj->callbacks.print = &print_xmms2_artist;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_album, &update_xmms2)
		obj->callbacks.print = &print_xmms2_album;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_title, &update_xmms2)
		obj->callbacks.print = &print_xmms2_title;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_genre, &update_xmms2)
		obj->callbacks.print = &print_xmms2_genre;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_comment, &update_xmms2)
		obj->callbacks.print = &print_xmms2_comment;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_url, &update_xmms2)
		obj->callbacks.print = &print_xmms2_url;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_tracknr, &update_xmms2)
		obj->callbacks.print = &print_xmms2_tracknr;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_bitrate, &update_xmms2)
		obj->callbacks.print = &print_xmms2_bitrate;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_date, &update_xmms2)
		obj->callbacks.print = &print_xmms2_date;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_id, &update_xmms2)
		obj->callbacks.print = &print_xmms2_id;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_duration, &update_xmms2)
		obj->callbacks.print = &print_xmms2_duration;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_elapsed, &update_xmms2)
		obj->callbacks.print = &print_xmms2_elapsed;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_size, &update_xmms2)
		obj->callbacks.print = &print_xmms2_size;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_status, &update_xmms2)
		obj->callbacks.print = &print_xmms2_status;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_percent, &update_xmms2)
		obj->callbacks.print = &print_xmms2_percent;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_bar, &update_xmms2)
		scan_bar(obj, arg, 1);
		obj->callbacks.barval = &xmms2_barval;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_smart, &update_xmms2)
		obj->callbacks.print = &print_xmms2_smart;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_playlist, &update_xmms2)
		obj->callbacks.print = &print_xmms2_playlist;
		obj->callbacks.free = &free_xmms2;
	END OBJ(xmms2_timesplayed, &update_xmms2)
		obj->callbacks.print = &print_xmms2_timesplayed;
		obj->callbacks.free = &free_xmms2;
	END OBJ_IF(if_xmms2_connected, &update_xmms2)
		obj->callbacks.iftest = &if_xmms2_connected;
		obj->callbacks.free = &free_xmms2;
#endif /* BUILD_XMMS2 */
#ifdef BUILD_AUDACIOUS
	END OBJ(audacious_status, 0)
		obj->callbacks.print = &print_audacious_status;
	END OBJ_ARG(audacious_title, 0, "audacious_title needs an argument")
		sscanf(arg, "%d", &obj->data.i);
		if (obj->data.i > 0) {
			++ obj->data.i;
		} else {
			CRIT_ERR(obj, free_at_crash, "audacious_title: invalid length argument");
		}
		obj->callbacks.print = &print_audacious_title;
	END OBJ(audacious_length, 0)
		obj->callbacks.print = &print_audacious_length;
	END OBJ(audacious_length_seconds, 0)
		obj->callbacks.print = &print_audacious_length_seconds;
	END OBJ(audacious_position, 0)
		obj->callbacks.print = &print_audacious_position;
	END OBJ(audacious_position_seconds, 0)
		obj->callbacks.print = &print_audacious_position_seconds;
	END OBJ(audacious_bitrate, 0)
		obj->callbacks.print = &print_audacious_bitrate;
	END OBJ(audacious_frequency, 0)
		obj->callbacks.print = &print_audacious_frequency;
	END OBJ(audacious_channels, 0)
		obj->callbacks.print = &print_audacious_channels;
	END OBJ(audacious_filename, 0)
		obj->callbacks.print = &print_audacious_filename;
	END OBJ(audacious_playlist_length, 0)
		obj->callbacks.print = &print_audacious_playlist_length;
	END OBJ(audacious_playlist_position, 0)
		obj->callbacks.print = &print_audacious_playlist_position;
	END OBJ(audacious_main_volume, 0)
		obj->callbacks.print = &print_audacious_main_volume;
	END OBJ(audacious_bar, 0)
		scan_bar(obj, arg, 1);
		obj->callbacks.barval = &audacious_barval;
#endif /* BUILD_AUDACIOUS */
#ifdef BUIL_BMPX
	END OBJ(bmpx_title, &update_bmpx)
		obj->callbacks.print = &print_bmpx_title;
	END OBJ(bmpx_artist, &update_bmpx)
		obj->callbacks.print = &print_bmpx_artist;
	END OBJ(bmpx_album, &update_bmpx)
		obj->callbacks.print = &print_bmpx_album;
	END OBJ(bmpx_track, &update_bmpx)
		obj->callbacks.print = &print_bmpx_track;
	END OBJ(bmpx_uri, &update_bmpx)
		obj->callbacks.print = &print_bmpx_uri;
	END OBJ(bmpx_bitrate, &update_bmpx)
		obj->callbacks.print = &print_bmpx_bitrate;
#endif /* BUILD_BMPX */
#ifdef BUILD_EVE
	END OBJ_ARG(eve, 0, "eve needs arguments: <userid> <apikey> <characterid>")
		scan_eve(obj, arg);
		obj->callbacks.print = &print_eve;
		obj->callbacks.free = &free_eve;
#endif /* BUILD_EVE */
#ifdef BUILD_CURL
	END OBJ_ARG(curl, 0, "curl needs arguments: <uri> <interval in minutes>")
		curl_parse_arg(obj, arg);
		obj->callbacks.print = &curl_print;
		obj->callbacks.free = &curl_obj_free;
#endif /* BUILD_CURL */
#ifdef BUILD_RSS
	END OBJ_ARG(rss, 0, "rss needs arguments: <uri> <interval in minutes> <action> [act_par] [spaces in front]")
		rss_scan_arg(obj, arg);
		obj->callbacks.print = &rss_print_info;
		obj->callbacks.free = &rss_free_obj_info;
#endif /* BUILD_RSS */
#ifdef BUILD_WEATHER_METAR
	END OBJ_ARG(weather, 0, "weather needs arguments: <uri> <locID> <data_type> [interval in minutes]")
		scan_weather_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_weather;
		obj->callbacks.free = &free_weather;
#endif /* BUILD_WEATHER_METAR */
#ifdef BUILD_WEATHER_XOAP
	END OBJ_ARG(weather_forecast, 0, "weather_forecast needs arguments: <uri> <locID> <day> <data_type> [interval in minutes]")
		scan_weather_forecast_arg(obj, arg, free_at_crash);
		obj->callbacks.print = &print_weather_forecast;
		obj->callbacks.free = &free_weather;
#endif /* BUILD_WEATHER_XOAP */
	END OBJ_ARG(lua, 0, "lua needs arguments: <function name> [function parameters]")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.print = &print_lua;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_ARG(lua_parse, 0, "lua_parse needs arguments: <function name> [function parameters]")
		obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.print = &print_lua_parse;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_ARG(lua_bar, 0, "lua_bar needs arguments: <height>,<width> <function name> [function parameters]")
		arg = scan_bar(obj, arg, 100);
		if(arg) {
			obj->data.s = strndup(arg, text_buffer_size.get(*state));
		} else {
			CRIT_ERR(obj, free_at_crash, "lua_bar needs arguments: <height>,<width> <function name> [function parameters]");
		}
		obj->callbacks.barval = &lua_barval;
		obj->callbacks.free = &gen_free_opaque;
#ifdef BUILD_X11
	END OBJ_ARG(lua_graph, 0, "lua_graph needs arguments: <function name> [height],[width] [gradient colour 1] [gradient colour 2] [scale] [-t] [-l]")
		char *buf = 0;
		buf = scan_graph(obj, arg, 100);
		if (buf) {
			obj->data.s = buf;
		} else {
			CRIT_ERR(obj, free_at_crash, "lua_graph needs arguments: <function name> [height],[width] [gradient colour 1] [gradient colour 2] [scale] [-t] [-l]");
		}
		obj->callbacks.graphval = &lua_barval;
		obj->callbacks.free = &gen_free_opaque;
	END OBJ_ARG(lua_gauge, 0, "lua_gauge needs arguments: <height>,<width> <function name> [function parameters]")
		arg = scan_gauge(obj, arg, 100);
		if (arg) {
			obj->data.s = strndup(arg, text_buffer_size.get(*state));
		} else {
			CRIT_ERR(obj, free_at_crash, "lua_gauge needs arguments: <height>,<width> <function name> [function parameters]");
		}
		obj->callbacks.gaugeval = &lua_barval;
		obj->callbacks.free = &gen_free_opaque;
#endif /* BUILD_X11 */
#ifdef BUILD_HDDTEMP
	END OBJ(hddtemp, &update_hddtemp)
		if (arg)
			obj->data.s = strndup(arg, text_buffer_size.get(*state));
		obj->callbacks.print = &print_hddtemp;
		obj->callbacks.free = &free_hddtemp;
#endif /* BUILD_HDDTEMP */
#ifdef BUILD_PORT_MONITORS
	END OBJ_ARG(tcp_portmon, &tcp_portmon_update, "tcp_portmon: needs arguments")
		tcp_portmon_init(obj, arg);
		obj->callbacks.print = &tcp_portmon_action;
		obj->callbacks.free = &tcp_portmon_free;
#endif /* BUILD_PORT_MONITORS */
	END OBJ(entropy_avail, &update_entropy)
		obj->callbacks.print = &print_entropy_avail;
	END OBJ(entropy_perc, &update_entropy)
		obj->callbacks.percentage = &entropy_percentage;
	END OBJ(entropy_poolsize, &update_entropy)
		obj->callbacks.print = &print_entropy_poolsize;
	END OBJ(entropy_bar, &update_entropy)
		scan_bar(obj, arg, 1);
		obj->callbacks.barval = &entropy_barval;
	END OBJ_ARG(blink, 0, "blink needs a argument")
		obj->sub = (text_object*)malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
		obj->callbacks.print = &print_blink;
	END OBJ_ARG(to_bytes, 0, "to_bytes needs a argument")
		obj->sub = (text_object*)malloc(sizeof(struct text_object));
		extract_variable_text_internal(obj->sub, arg);
		obj->callbacks.print = &print_to_bytes;
#ifdef BUILD_CURL
	END OBJ_ARG(stock, 0, "stock needs arguments")
		stock_parse_arg(obj, arg);
		obj->callbacks.print = &print_stock;
		obj->callbacks.free = &free_stock;
#endif /* BUILD_CURL */
	END OBJ(scroll, 0)
#ifdef BUILD_X11
		/* allocate a follower to reset any color changes */
#endif /* BUILD_X11 */
		parse_scroll_arg(obj, arg, free_at_crash, s);
		obj->callbacks.print = &print_scroll;
		obj->callbacks.free = &free_scroll;
	END OBJ(combine, 0)
		try {
			parse_combine_arg(obj, arg);
		}
		catch(combine_needs_2_args_error &e) {
			free(obj);
			throw obj_create_error(e.what());
		}
		obj->callbacks.print = &print_combine;
		obj->callbacks.free = &free_combine;
#ifdef BUILD_NVIDIA
	END OBJ_ARG(nvidia, 0, "nvidia needs an argument")
		if (set_nvidia_type(obj, arg)) {
			CRIT_ERR(obj, free_at_crash, "nvidia: invalid argument"
				 " specified: '%s'\n", arg);
		}
		obj->callbacks.print = &print_nvidia_value;
		obj->callbacks.free = &free_nvidia;
#endif /* BUILD_NVIDIA */
#ifdef BUILD_APCUPSD
	END OBJ_ARG(apcupsd, &update_apcupsd, "apcupsd needs arguments: <host> <port>")
		if (apcupsd_scan_arg(arg)) {
			CRIT_ERR(obj, free_at_crash, "apcupsd needs arguments: <host> <port>");
		}
		obj->callbacks.print = &gen_print_nothing;
	END OBJ(apcupsd_name, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_name;
	END OBJ(apcupsd_model, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_model;
	END OBJ(apcupsd_upsmode, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_upsmode;
	END OBJ(apcupsd_cable, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_cable;
	END OBJ(apcupsd_status, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_status;
	END OBJ(apcupsd_linev, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_linev;
	END OBJ(apcupsd_load, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_load;
	END OBJ(apcupsd_loadbar, &update_apcupsd)
		scan_bar(obj, arg, 100);
		obj->callbacks.barval = &apcupsd_loadbarval;
#ifdef BUILD_X11
	END OBJ(apcupsd_loadgraph, &update_apcupsd)
		char* buf = 0;
		buf = scan_graph(obj, arg, 100);
		free_and_zero(buf);
		obj->callbacks.graphval = &apcupsd_loadbarval;
	END OBJ(apcupsd_loadgauge, &update_apcupsd)
		scan_gauge(obj, arg, 100);
		obj->callbacks.gaugeval = &apcupsd_loadbarval;
#endif /* BUILD_X11 */
	END OBJ(apcupsd_charge, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_charge;
	END OBJ(apcupsd_timeleft, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_timeleft;
	END OBJ(apcupsd_temp, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_temp;
	END OBJ(apcupsd_lastxfer, &update_apcupsd)
		obj->callbacks.print = &print_apcupsd_lastxfer;
#endif /* BUILD_APCUPSD */
	END {
		char *buf = (char *)malloc(text_buffer_size.get(*state));

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

	p = strndup(const_p, max_user_text.get(*state) - 1);
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
				char *buf = (char *)malloc(text_buffer_size.get(*state));
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
				len = (p - s > (int)text_buffer_size.get(*state)-1) ? (int)text_buffer_size.get(*state)-1 : (p - s);
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
					free(buf);
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

				try {
					obj = construct_text_object(buf, arg,
						line, &ifblock_opaque, orig_p);
				}
				catch(obj_create_error &e) {
					free(buf);
					free(orig_p);
					throw e;
				}
				if (obj != NULL) {
					append_object(retval, obj);
				}
				free(buf);
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
			if (remove_comment(p, &c) && p >= orig_p && c == '\n') {
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

void extract_object_args_to_sub(struct text_object *obj, const char *args)
{
	obj->sub = (struct text_object *)malloc(sizeof(struct text_object));
	memset(obj->sub, 0, sizeof(struct text_object));
	extract_variable_text_internal(obj->sub, args);
}

/* Frees the list of text objects root points to. */
void free_text_objects(struct text_object *root)
{
	struct text_object *obj;

	if(root && root->prev) {
		for (obj = root->prev; obj; obj = root->prev) {
			root->prev = obj->prev;

			if (obj->callbacks.free) {
				(*obj->callbacks.free)(obj);
			}
			free_text_objects(obj->sub);
			free_and_zero(obj->sub);
			free_and_zero(obj->special_data);
			delete obj->cb_handle;

			free(obj);
		}
	}
}

