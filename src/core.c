/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
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
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 */

/* local headers */
#include "core.h"
#include "text_object.h"
#include "algebra.h"
#include "build.h"
#include "colours.h"
#include "diskio.h"
#ifdef X11
#include "fonts.h"
#endif
#include "fs.h"
#include "logging.h"
#include "mixer.h"
#include "mail.h"
#include "mboxscan.h"
#ifdef NCURSES
#include "ncurses.h"
#endif
#include "specials.h"
#include "temphelper.h"
#include "tailhead.h"
#include "top.h"

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

#ifdef HAVE_ICONV
#include <iconv.h>

#define ICONV_CODEPAGE_LENGTH 20

int register_iconv(iconv_t *new_iconv);

long iconv_selected;
long iconv_count = 0;
char iconv_converting;
static iconv_t **iconv_cd = 0;

char is_iconv_converting(void)
{
	return iconv_converting;
}

void set_iconv_converting(char i)
{
	iconv_converting = i;
}

long get_iconv_selected(void)
{
	return iconv_selected;
}

void set_iconv_selected(long i)
{
	iconv_selected = i;
}

int register_iconv(iconv_t *new_iconv)
{
	iconv_cd = realloc(iconv_cd, sizeof(iconv_t *) * (iconv_count + 1));
	if (!iconv_cd) {
		CRIT_ERR(NULL, NULL, "Out of memory");
	}
	iconv_cd[iconv_count] = malloc(sizeof(iconv_t));
	if (!iconv_cd[iconv_count]) {
		CRIT_ERR(NULL, NULL, "Out of memory");
	}
	memcpy(iconv_cd[iconv_count], new_iconv, sizeof(iconv_t));
	iconv_count++;
	return iconv_count;
}

void free_iconv(void)
{
	if (iconv_cd) {
		long i;

		for (i = 0; i < iconv_count; i++) {
			if (iconv_cd[i]) {
				iconv_close(*iconv_cd[i]);
				free(iconv_cd[i]);
			}
		}
		free(iconv_cd);
	}
	iconv_cd = 0;
}

void iconv_convert(size_t a, char *buff_in, char *p, size_t p_max_size)
{
	if (a > 0 && is_iconv_converting() && get_iconv_selected() > 0
			&& (iconv_cd[iconv_selected - 1] != (iconv_t) (-1))) {
		int bytes;
		size_t dummy1, dummy2;
#ifdef __FreeBSD__
		const char *ptr = buff_in;
#else
		char *ptr = buff_in;
#endif
		char *outptr = p;

		dummy1 = dummy2 = a;

		strncpy(buff_in, p, p_max_size);

		iconv(*iconv_cd[iconv_selected - 1], NULL, NULL, NULL, NULL);
		while (dummy1 > 0) {
			bytes = iconv(*iconv_cd[iconv_selected - 1], &ptr, &dummy1,
					&outptr, &dummy2);
			if (bytes == -1) {
				NORM_ERR("Iconv codeset conversion failed");
				break;
			}
		}

		/* It is nessecary when we are converting from multibyte to
		 * singlebyte codepage */
		a = outptr - p;
	}
}
#endif /* HAVE_ICONV */

/* strip a leading /dev/ if any, following symlinks first
 *
 * BEWARE: this function returns a pointer to static content
 *         which gets overwritten in consecutive calls. I.e.:
 *         this function is NOT reentrant.
 */
static const char *dev_name(const char *path)
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

#define OBJ(a, n) if (strcmp(s, #a) == 0) { \
	obj->type = OBJ_##a; need_mask |= (1ULL << n); {
#define OBJ_IF(a, n) if (strcmp(s, #a) == 0) { \
	obj->type = OBJ_##a; need_mask |= (1ULL << n); \
	obj_be_ifblock_if(ifblock_opaque, obj); {
#define END } } else

#define SIZE_DEFAULTS(arg) { \
	obj->a = default_##arg##_width; \
	obj->b = default_##arg##_height; \
}

#ifdef X11
	if (s[0] == '#') {
		obj->type = OBJ_color;
		obj->data.l = get_x11_color(s);
	} else
#endif /* X11 */
#ifdef __OpenBSD__
	OBJ(freq, INFO_FREQ)
#else
	OBJ(acpitemp, 0)
		obj->data.i = open_acpi_temperature(arg);
	END OBJ(acpiacadapter, 0)
	END OBJ(freq, INFO_FREQ)
#endif /* !__OpenBSD__ */
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| (unsigned int) atoi(&arg[0]) > info.cpu_count) {
			obj->data.cpu_index = 1;
			/* NORM_ERR("freq: Invalid CPU number or you don't have that many CPUs! "
				"Displaying the clock for CPU 1."); */
		} else {
			obj->data.cpu_index = atoi(&arg[0]);
		}
		obj->a = 1;
	END OBJ(freq_g, INFO_FREQ)
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| (unsigned int) atoi(&arg[0]) > info.cpu_count) {
			obj->data.cpu_index = 1;
			/* NORM_ERR("freq_g: Invalid CPU number or you don't have that many "
				"CPUs! Displaying the clock for CPU 1."); */
		} else {
			obj->data.cpu_index = atoi(&arg[0]);
		}
		obj->a = 1;
	END OBJ(read_tcp, 0)
		if (arg) {
			obj->data.read_tcp.host = malloc(text_buffer_size);
			sscanf(arg, "%s", obj->data.read_tcp.host);
			sscanf(arg+strlen(obj->data.read_tcp.host), "%u", &(obj->data.read_tcp.port));
			if(obj->data.read_tcp.port == 0) {
				obj->data.read_tcp.port = atoi(obj->data.read_tcp.host);
				strcpy(obj->data.read_tcp.host,"localhost");
			}
			obj->data.read_tcp.port = htons(obj->data.read_tcp.port);
			if(obj->data.read_tcp.port < 1 || obj->data.read_tcp.port > 65535) {
				CRIT_ERR(obj, free_at_crash, "read_tcp: Needs \"(host) port\" as argument(s)");
			}
		}else{
			CRIT_ERR(obj, free_at_crash, "read_tcp: Needs \"(host) port\" as argument(s)");
		}
#if defined(__linux__)
	END OBJ(voltage_mv, 0)
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| (unsigned int) atoi(&arg[0]) > info.cpu_count) {
			obj->data.cpu_index = 1;
			/* NORM_ERR("voltage_mv: Invalid CPU number or you don't have that many "
				"CPUs! Displaying voltage for CPU 1."); */
		} else {
			obj->data.cpu_index = atoi(&arg[0]);
		}
		obj->a = 1;
	END OBJ(voltage_v, 0)
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| (unsigned int) atoi(&arg[0]) > info.cpu_count) {
			obj->data.cpu_index = 1;
			/* NORM_ERR("voltage_v: Invalid CPU number or you don't have that many "
				"CPUs! Displaying voltage for CPU 1."); */
		} else {
			obj->data.cpu_index = atoi(&arg[0]);
		}
		obj->a = 1;

#ifdef HAVE_IWLIB
	END OBJ(wireless_essid, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(wireless_mode, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(wireless_bitrate, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(wireless_ap, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(wireless_link_qual, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(wireless_link_qual_max, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(wireless_link_qual_perc, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(wireless_link_bar, INFO_NET)
		SIZE_DEFAULTS(bar);
		if (arg) {
			arg = scan_bar(arg, &obj->a, &obj->b);
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
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
		SIZE_DEFAULTS(bar);
		obj->b = 6;
		if (arg) {
			arg = scan_bar(arg, &obj->a, &obj->b);
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size);
#endif /* !__OpenBSD__ */

#if defined(__linux__)
	END OBJ(disk_protect, 0)
		if (arg)
			obj->data.s = strndup(dev_name(arg), text_buffer_size);
		else
			CRIT_ERR(obj, free_at_crash, "disk_protect needs an argument");
	END OBJ(i8k_version, INFO_I8K)
	END OBJ(i8k_bios, INFO_I8K)
	END OBJ(i8k_serial, INFO_I8K)
	END OBJ(i8k_cpu_temp, INFO_I8K)
	END OBJ(i8k_left_fan_status, INFO_I8K)
	END OBJ(i8k_right_fan_status, INFO_I8K)
	END OBJ(i8k_left_fan_rpm, INFO_I8K)
	END OBJ(i8k_right_fan_rpm, INFO_I8K)
	END OBJ(i8k_ac_status, INFO_I8K)
	END OBJ(i8k_buttons_status, INFO_I8K)
#if defined(IBM)
	END OBJ(ibm_fan, 0)
	END OBJ(ibm_temps, 0)
		if (!arg) {
			CRIT_ERR(obj, free_at_crash, "ibm_temps: needs an argument");
		}
		if (!isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) >= 8) {
			obj->data.sensor = 0;
			NORM_ERR("Invalid temperature sensor! Sensor number must be 0 to 7. "
				"Using 0 (CPU temp sensor).");
		}
		obj->data.sensor = atoi(&arg[0]);
	END OBJ(ibm_volume, 0)
	END OBJ(ibm_brightness, 0)
#endif
	/* information from sony_laptop kernel module
	 * /sys/devices/platform/sony-laptop */
	END OBJ(sony_fanspeed, 0)
	END OBJ_IF(if_gw, INFO_GW)
	END OBJ(ioscheduler, 0)
		if (!arg) {
			CRIT_ERR(obj, free_at_crash, "get_ioscheduler needs an argument (e.g. hda)");
			obj->data.s = 0;
		} else
			obj->data.s = strndup(dev_name(arg), text_buffer_size);
	END OBJ(laptop_mode, 0)
	END OBJ(pb_battery, 0)
		if (arg && strcmp(arg, "status") == EQUAL) {
			obj->data.i = PB_BATT_STATUS;
		} else if (arg && strcmp(arg, "percent") == EQUAL) {
			obj->data.i = PB_BATT_PERCENT;
		} else if (arg && strcmp(arg, "time") == EQUAL) {
			obj->data.i = PB_BATT_TIME;
		} else {
			NORM_ERR("pb_battery: needs one argument: status, percent or time");
			free(obj);
			return NULL;
		}

#endif /* __linux__ */
#if (defined(__FreeBSD__) || defined(__linux__))
	END OBJ_IF(if_up, 0)
		if (!arg) {
			NORM_ERR("if_up needs an argument");
			obj->data.ifblock.s = 0;
		} else {
			obj->data.ifblock.s = strndup(arg, text_buffer_size);
		}
#endif
#if defined(__OpenBSD__)
	END OBJ(obsd_sensors_temp, 0)
		if (!arg) {
			CRIT_ERR(obj, free_at_crash, "obsd_sensors_temp: needs an argument");
		}
		if (!isdigit(arg[0]) || atoi(&arg[0]) < 0
				|| atoi(&arg[0]) > OBSD_MAX_SENSORS - 1) {
			obj->data.sensor = 0;
			NORM_ERR("Invalid temperature sensor number!");
		}
		obj->data.sensor = atoi(&arg[0]);
	END OBJ(obsd_sensors_fan, 0)
		if (!arg) {
			CRIT_ERR(obj, free_at_crash, "obsd_sensors_fan: needs 2 arguments (device and sensor "
				"number)");
		}
		if (!isdigit(arg[0]) || atoi(&arg[0]) < 0
				|| atoi(&arg[0]) > OBSD_MAX_SENSORS - 1) {
			obj->data.sensor = 0;
			NORM_ERR("Invalid fan sensor number!");
		}
		obj->data.sensor = atoi(&arg[0]);
	END OBJ(obsd_sensors_volt, 0)
		if (!arg) {
			CRIT_ERR(obj, free_at_crash, "obsd_sensors_volt: needs 2 arguments (device and sensor "
				"number)");
		}
		if (!isdigit(arg[0]) || atoi(&arg[0]) < 0
				|| atoi(&arg[0]) > OBSD_MAX_SENSORS - 1) {
			obj->data.sensor = 0;
			NORM_ERR("Invalid voltage sensor number!");
		}
		obj->data.sensor = atoi(&arg[0]);
	END OBJ(obsd_vendor, 0)
	END OBJ(obsd_product, 0)
#endif /* __OpenBSD__ */
	END OBJ(buffers, INFO_BUFFERS)
	END OBJ(cached, INFO_BUFFERS)
#define SCAN_CPU(__arg, __var) { \
	int __offset = 0; \
	if (__arg && sscanf(__arg, " cpu%u %n", &__var, &__offset) > 0) \
		__arg += __offset; \
	else \
		__var = 0; \
}
	END OBJ(cpu, INFO_CPU)
		SCAN_CPU(arg, obj->data.cpu_index);
		DBGP2("Adding $cpu for CPU %d", obj->data.cpu_index);
#ifdef X11
	END OBJ(cpugauge, INFO_CPU)
		SIZE_DEFAULTS(gauge);
		SCAN_CPU(arg, obj->data.cpu_index);
		scan_gauge(arg, &obj->a, &obj->b);
		DBGP2("Adding $cpugauge for CPU %d", obj->data.cpu_index);
#endif /* X11 */
	END OBJ(cpubar, INFO_CPU)
		SIZE_DEFAULTS(bar);
		SCAN_CPU(arg, obj->data.cpu_index);
		scan_bar(arg, &obj->a, &obj->b);
		DBGP2("Adding $cpubar for CPU %d", obj->data.cpu_index);
#ifdef X11
	END OBJ(cpugraph, INFO_CPU)
		char *buf = 0;
		SIZE_DEFAULTS(graph);
		SCAN_CPU(arg, obj->data.cpu_index);
		buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
			&obj->e, &obj->char_a, &obj->char_b);
		DBGP2("Adding $cpugraph for CPU %d", obj->data.cpu_index);
		if (buf) free(buf);
	END OBJ(loadgraph, INFO_LOADAVG)
		char *buf = 0;
		SIZE_DEFAULTS(graph);
		buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
				&obj->e, &obj->char_a, &obj->char_b);
		if (buf) {
			int a = 1, r = 3;
			if (arg) {
				r = sscanf(arg, "%d", &a);
			}
			obj->data.loadavg[0] = (r >= 1) ? (unsigned char) a : 0;
			free(buf);
		}
#endif /* X11 */
	END OBJ(diskio, INFO_DISKIO)
		obj->data.diskio = prepare_diskio_stat(dev_name(arg));
	END OBJ(diskio_read, INFO_DISKIO)
		obj->data.diskio = prepare_diskio_stat(dev_name(arg));
	END OBJ(diskio_write, INFO_DISKIO)
		obj->data.diskio = prepare_diskio_stat(dev_name(arg));
#ifdef X11
	END OBJ(diskiograph, INFO_DISKIO)
		char *buf = 0;
		SIZE_DEFAULTS(graph);
		buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
				&obj->e, &obj->char_a, &obj->char_b);

		obj->data.diskio = prepare_diskio_stat(dev_name(buf));
		if (buf) free(buf);
	END OBJ(diskiograph_read, INFO_DISKIO)
		char *buf = 0;
		SIZE_DEFAULTS(graph);
		buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
				&obj->e, &obj->char_a, &obj->char_b);

		obj->data.diskio = prepare_diskio_stat(dev_name(buf));
		if (buf) free(buf);
	END OBJ(diskiograph_write, INFO_DISKIO)
		char *buf = 0;
		SIZE_DEFAULTS(graph);
		buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
				&obj->e, &obj->char_a, &obj->char_b);

		obj->data.diskio = prepare_diskio_stat(dev_name(buf));
		if (buf) free(buf);
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
	END OBJ(conky_build_date, 0)
	END OBJ(conky_build_arch, 0)
	END OBJ(downspeed, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(downspeedf, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
#ifdef X11
	END OBJ(downspeedgraph, INFO_NET)
		char *buf = 0;
		SIZE_DEFAULTS(graph);
		buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
				&obj->e, &obj->char_a, &obj->char_b);

		// default to DEFAULTNETDEV
		buf = strndup(buf ? buf : DEFAULTNETDEV, text_buffer_size);
		obj->data.net = get_net_stat(buf, obj, free_at_crash);
		free(buf);
#endif /* X11 */
	END OBJ(else, 0)
		obj_be_ifblock_else(ifblock_opaque, obj);
	END OBJ(endif, 0)
		obj_be_ifblock_endif(ifblock_opaque, obj);
	END OBJ(eval, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
	END OBJ(image, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
	END OBJ(exec, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
	END OBJ(execp, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
	END OBJ(execbar, 0)
		SIZE_DEFAULTS(bar);
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
#ifdef X11
	END OBJ(execgauge, 0)
		SIZE_DEFAULTS(gauge);
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
	END OBJ(execgraph, 0)
		SIZE_DEFAULTS(graph);
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
#endif /* X11 */
	END OBJ(execibar, 0)
		int n;
		SIZE_DEFAULTS(bar);

		if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
			char buf[256];

			NORM_ERR("${execibar <interval> command}");
			obj->type = OBJ_text;
			snprintf(buf, 256, "${%s}", s);
			obj->data.s = strndup(buf, text_buffer_size);
		} else {
			obj->data.execi.cmd = strndup(arg + n, text_buffer_size);
		}
#ifdef X11
	END OBJ(execigraph, 0)
		int n;
		SIZE_DEFAULTS(graph);

		if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
			char buf[256];

			NORM_ERR("${execigraph <interval> command}");
			obj->type = OBJ_text;
			snprintf(buf, 256, "${%s}", s);
			obj->data.s = strndup(buf, text_buffer_size);
		} else {
			obj->data.execi.cmd = strndup(arg + n, text_buffer_size);
		}
	END OBJ(execigauge, 0)
		int n;
		SIZE_DEFAULTS(gauge);

		if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
			char buf[256];

			NORM_ERR("${execigauge <interval> command}");
			obj->type = OBJ_text;
			snprintf(buf, 256, "${%s}", s);
			obj->data.s = strndup(buf, text_buffer_size);
		} else {
			obj->data.execi.cmd = strndup(arg + n, text_buffer_size);
		}
#endif /* X11 */
	END OBJ(execi, 0)
		int n;

		if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
			char buf[256];

			NORM_ERR("${execi <interval> command}");
			obj->type = OBJ_text;
			snprintf(buf, 256, "${%s}", s);
			obj->data.s = strndup(buf, text_buffer_size);
		} else {
			obj->data.execi.cmd = strndup(arg + n, text_buffer_size);
			obj->data.execi.buffer = malloc(text_buffer_size);
		}
	END OBJ(execpi, 0)
		int n;

		if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
			char buf[256];

			NORM_ERR("${execi <interval> command}");
			obj->type = OBJ_text;
			snprintf(buf, 256, "${%s}", s);
			obj->data.s = strndup(buf, text_buffer_size);
		} else {
			obj->data.execi.cmd = strndup(arg + n, text_buffer_size);
			obj->data.execi.buffer = malloc(text_buffer_size);
		}
	END OBJ(texeci, 0)
			int n;

			if (!arg || sscanf(arg, "%f %n", &obj->data.texeci.interval, &n) <= 0) {
				char buf[256];

				NORM_ERR("${texeci <interval> command}");
				obj->type = OBJ_text;
				snprintf(buf, 256, "${%s}", s);
				obj->data.s = strndup(buf, text_buffer_size);
			} else {
				obj->data.texeci.cmd = strndup(arg + n, text_buffer_size);
				obj->data.texeci.buffer = malloc(text_buffer_size);
			}
			obj->data.texeci.p_timed_thread = NULL;
	END OBJ(pre_exec, 0)
		obj->type = OBJ_text;
		if (arg) {
			char buf[2048];

			do_read_exec(arg, buf, sizeof(buf));
			obj->data.s = strndup(buf, text_buffer_size);
		} else {
			obj->data.s = strndup("", text_buffer_size);
		}
	END OBJ(fs_bar, INFO_FS)
		SIZE_DEFAULTS(bar);
		arg = scan_bar(arg, &obj->data.fsbar.w, &obj->data.fsbar.h);
		if (arg) {
			while (isspace(*arg)) {
				arg++;
			}
			if (*arg == '\0') {
				arg = "/";
			}
		} else {
			arg = "/";
		}
		obj->data.fsbar.fs = prepare_fs_stat(arg);
	END OBJ(fs_bar_free, INFO_FS)
		SIZE_DEFAULTS(bar);
		arg = scan_bar(arg, &obj->data.fsbar.w, &obj->data.fsbar.h);
		if (arg) {
			while (isspace(*arg)) {
				arg++;
			}
			if (*arg == '\0') {
				arg = "/";
			}
		} else {
			arg = "/";
		}

		obj->data.fsbar.fs = prepare_fs_stat(arg);
	END OBJ(fs_free, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_used_perc, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_free_perc, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_size, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_type, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_used, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(hr, 0)
		obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(nameserver, INFO_DNS)
		obj->data.i = arg ? atoi(arg) : 0;
	END OBJ(offset, 0)
		obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(voffset, 0)
		obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(goto, 0)

		if (!arg) {
			NORM_ERR("goto needs arguments");
			obj->type = OBJ_text;
			obj->data.s = strndup("${goto}", text_buffer_size);
			return NULL;
		}

		obj->data.i = atoi(arg);

	END OBJ(tab, 0)
		int a = 10, b = 0;

		if (arg) {
			if (sscanf(arg, "%d %d", &a, &b) != 2) {
				sscanf(arg, "%d", &b);
			}
		}
		if (a <= 0) {
			a = 1;
		}
		obj->data.pair.a = a;
		obj->data.pair.b = b;

#ifdef __linux__
	END OBJ(i2c, INFO_SYSFS)
		char buf1[64], buf2[64];
		float factor, offset;
		int n, found = 0;

		if (!arg) {
			NORM_ERR("i2c needs arguments");
			obj->type = OBJ_text;
			// obj->data.s = strndup("${i2c}", text_buffer_size);
			return NULL;
		}

#define HWMON_RESET() {\
		buf1[0] = 0; \
		factor = 1.0; \
		offset = 0.0; }

		if (sscanf(arg, "%63s %d %f %f", buf2, &n, &factor, &offset) == 4) found = 1; else HWMON_RESET();
		if (!found && sscanf(arg, "%63s %63s %d %f %f", buf1, buf2, &n, &factor, &offset) == 5) found = 1; else if (!found) HWMON_RESET();
		if (!found && sscanf(arg, "%63s %63s %d", buf1, buf2, &n) == 3) found = 1; else if (!found) HWMON_RESET();
		if (!found && sscanf(arg, "%63s %d", buf2, &n) == 2) found = 1; else if (!found) HWMON_RESET();

		if (!found) {
			NORM_ERR("i2c failed to parse arguments");
			obj->type = OBJ_text;
			return NULL;
		}
		DBGP("parsed i2c args: '%s' '%s' %d %f %f\n", buf1, buf2, n, factor, offset);
		obj->data.sysfs.fd = open_i2c_sensor((*buf1) ? buf1 : 0, buf2, n,
				&obj->data.sysfs.arg, obj->data.sysfs.devtype);
		strncpy(obj->data.sysfs.type, buf2, 63);
		obj->data.sysfs.factor = factor;
		obj->data.sysfs.offset = offset;

	END OBJ(platform, INFO_SYSFS)
		char buf1[64], buf2[64];
		float factor, offset;
		int n, found = 0;

		if (!arg) {
			NORM_ERR("platform needs arguments");
			obj->type = OBJ_text;
			return NULL;
		}

		if (sscanf(arg, "%63s %d %f %f", buf2, &n, &factor, &offset) == 4) found = 1; else HWMON_RESET();
		if (!found && sscanf(arg, "%63s %63s %d %f %f", buf1, buf2, &n, &factor, &offset) == 5) found = 1; else if (!found) HWMON_RESET();
		if (!found && sscanf(arg, "%63s %63s %d", buf1, buf2, &n) == 3) found = 1; else if (!found) HWMON_RESET();
		if (!found && sscanf(arg, "%63s %d", buf2, &n) == 2) found = 1; else if (!found) HWMON_RESET();

		if (!found) {
			NORM_ERR("platform failed to parse arguments");
			obj->type = OBJ_text;
			return NULL;
		}
		DBGP("parsed platform args: '%s' '%s' %d %f %f", buf1, buf2, n, factor, offset);
		obj->data.sysfs.fd = open_platform_sensor((*buf1) ? buf1 : 0, buf2, n,
				&obj->data.sysfs.arg, obj->data.sysfs.devtype);
		strncpy(obj->data.sysfs.type, buf2, 63);
		obj->data.sysfs.factor = factor;
		obj->data.sysfs.offset = offset;

	END OBJ(hwmon, INFO_SYSFS)
		char buf1[64], buf2[64];
		float factor, offset;
		int n, found = 0;

		if (!arg) {
			NORM_ERR("hwmon needs argumanets");
			obj->type = OBJ_text;
			return NULL;
		}

		if (sscanf(arg, "%63s %d %f %f", buf2, &n, &factor, &offset) == 4) found = 1; else HWMON_RESET();
		if (!found && sscanf(arg, "%63s %63s %d %f %f", buf1, buf2, &n, &factor, &offset) == 5) found = 1; else if (!found) HWMON_RESET();
		if (!found && sscanf(arg, "%63s %63s %d", buf1, buf2, &n) == 3) found = 1; else if (!found) HWMON_RESET();
		if (!found && sscanf(arg, "%63s %d", buf2, &n) == 2) found = 1; else if (!found) HWMON_RESET();

#undef HWMON_RESET

		if (!found) {
			NORM_ERR("hwmon failed to parse arguments");
			obj->type = OBJ_text;
			return NULL;
		}
		DBGP("parsed hwmon args: '%s' '%s' %d %f %f\n", buf1, buf2, n, factor, offset);
		obj->data.sysfs.fd = open_hwmon_sensor((*buf1) ? buf1 : 0, buf2, n,
				&obj->data.sysfs.arg, obj->data.sysfs.devtype);
		strncpy(obj->data.sysfs.type, buf2, 63);
		obj->data.sysfs.factor = factor;
		obj->data.sysfs.offset = offset;

#endif /* !__OpenBSD__ */

	END
	/* we have four different types of top (top, top_mem, top_time and top_io). To
	 * avoid having almost-same code four times, we have this special
	 * handler. */
	if (strncmp(s, "top", 3) == EQUAL) {
		if (!parse_top_args(s, arg, obj)) {
			return NULL;
		}
	} else OBJ(addr, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
#if defined(__linux__)
	END OBJ(addrs, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
#endif /* __linux__ */
	END OBJ(tail, 0)
		init_tailhead("tail", arg, obj, free_at_crash);
	END OBJ(head, 0)
		init_tailhead("head", arg, obj, free_at_crash);
	END OBJ(lines, 0)
		if (arg) {
			obj->data.s = strndup(arg, text_buffer_size);
		}else{
			CRIT_ERR(obj, free_at_crash, "lines needs a argument");
		}
	END OBJ(words, 0)
		if (arg) {
			obj->data.s = strndup(arg, text_buffer_size);
		}else{
			CRIT_ERR(obj, free_at_crash, "words needs a argument");
		}
	END OBJ(loadavg, INFO_LOADAVG)
		int a = 1, b = 2, c = 3, r = 3;

		if (arg) {
			r = sscanf(arg, "%d %d %d", &a, &b, &c);
			if (r >= 3 && (c < 1 || c > 3)) {
				r--;
			}
			if (r >= 2 && (b < 1 || b > 3)) {
				r--, b = c;
			}
			if (r >= 1 && (a < 1 || a > 3)) {
				r--, a = b, b = c;
			}
		}
		obj->data.loadavg[0] = (r >= 1) ? (unsigned char) a : 0;
		obj->data.loadavg[1] = (r >= 2) ? (unsigned char) b : 0;
		obj->data.loadavg[2] = (r >= 3) ? (unsigned char) c : 0;
	END OBJ_IF(if_empty, 0)
		if (!arg) {
			NORM_ERR("if_empty needs an argument");
			obj->data.ifblock.s = 0;
		} else {
			obj->data.ifblock.s = strndup(arg, text_buffer_size);
			obj->sub = malloc(sizeof(struct text_object));
			extract_variable_text_internal(obj->sub,
			                               obj->data.ifblock.s);
		}
	END OBJ_IF(if_match, 0)
		if (!arg) {
			NORM_ERR("if_match needs arguments");
			obj->data.ifblock.s = 0;
		} else {
			obj->data.ifblock.s = strndup(arg, text_buffer_size);
			obj->sub = malloc(sizeof(struct text_object));
			extract_variable_text_internal(obj->sub,
			                               obj->data.ifblock.s);
		}
	END OBJ_IF(if_existing, 0)
		if (!arg) {
			NORM_ERR("if_existing needs an argument or two");
			obj->data.ifblock.s = NULL;
			obj->data.ifblock.str = NULL;
		} else {
			char buf1[256], buf2[256];
			int r = sscanf(arg, "%255s %255[^\n]", buf1, buf2);

			if (r == 1) {
				obj->data.ifblock.s = strndup(buf1, text_buffer_size);
				obj->data.ifblock.str = NULL;
			} else {
				obj->data.ifblock.s = strndup(buf1, text_buffer_size);
				obj->data.ifblock.str = strndup(buf2, text_buffer_size);
			}
		}
		DBGP("if_existing: '%s' '%s'", obj->data.ifblock.s, obj->data.ifblock.str);
	END OBJ_IF(if_mounted, 0)
		if (!arg) {
			NORM_ERR("if_mounted needs an argument");
			obj->data.ifblock.s = 0;
		} else {
			obj->data.ifblock.s = strndup(arg, text_buffer_size);
		}
#ifdef __linux__
	END OBJ_IF(if_running, INFO_TOP)
		if (arg) {
			obj->data.ifblock.s = strndup(arg, text_buffer_size);
#else
	END OBJ_IF(if_running, 0)
		if (arg) {
			char buf[256];

			snprintf(buf, 256, "pidof %s >/dev/null", arg);
			obj->data.ifblock.s = strndup(buf, text_buffer_size);
#endif
		} else {
			NORM_ERR("if_running needs an argument");
			obj->data.ifblock.s = 0;
		}
	END OBJ(kernel, 0)
	END OBJ(machine, 0)
	END OBJ(mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			/* Kapil: Changed from MAIL_FILE to
			   current_mail_spool since the latter
			   is a copy of the former if undefined
			   but the latter should take precedence
			   if defined */
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(new_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(seen_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(unseen_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(flagged_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(unflagged_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(forwarded_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(unforwarded_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(replied_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(unreplied_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(draft_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(trashed_mails, 0)
		float n1;
		char mbox[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			strncpy(mbox, current_mail_spool, sizeof(mbox));
		} else {
			if (sscanf(arg, "%s %f", mbox, &n1) != 2) {
				n1 = 9.5;
				strncpy(mbox, arg, sizeof(mbox));
			}
		}

		variable_substitute(mbox, dst, sizeof(dst));
		obj->data.local_mail.mbox = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(mboxscan, 0)
		obj->data.mboxscan.args = (char *) malloc(text_buffer_size);
		obj->data.mboxscan.output = (char *) malloc(text_buffer_size);
		/* if '1' (in mboxscan.c) then there was SIGUSR1, hmm */
		obj->data.mboxscan.output[0] = 1;
		strncpy(obj->data.mboxscan.args, arg, text_buffer_size);
	END OBJ(mem, INFO_MEM)
	END OBJ(memeasyfree, INFO_MEM)
	END OBJ(memfree, INFO_MEM)
	END OBJ(memmax, INFO_MEM)
	END OBJ(memperc, INFO_MEM)
#ifdef X11
	END OBJ(memgauge, INFO_MEM)
		SIZE_DEFAULTS(gauge);
		scan_gauge(arg, &obj->data.pair.a, &obj->data.pair.b);
#endif /* X11*/
	END OBJ(membar, INFO_MEM)
		SIZE_DEFAULTS(bar);
		scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
#ifdef X11
	END OBJ(memgraph, INFO_MEM)
		char *buf = 0;
		SIZE_DEFAULTS(graph);
		buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
				&obj->e, &obj->char_a, &obj->char_b);

		if (buf) free(buf);
#endif /* X11*/
	END OBJ(mixer, INFO_MIXER)
		obj->data.l = mixer_init(arg);
	END OBJ(mixerl, INFO_MIXER)
		obj->data.l = mixer_init(arg);
	END OBJ(mixerr, INFO_MIXER)
		obj->data.l = mixer_init(arg);
#ifdef X11
	END OBJ(mixerbar, INFO_MIXER)
		SIZE_DEFAULTS(bar);
		scan_mixer_bar(arg, &obj->data.mixerbar.l, &obj->data.mixerbar.w,
			&obj->data.mixerbar.h);
	END OBJ(mixerlbar, INFO_MIXER)
		SIZE_DEFAULTS(bar);
		scan_mixer_bar(arg, &obj->data.mixerbar.l, &obj->data.mixerbar.w,
			&obj->data.mixerbar.h);
	END OBJ(mixerrbar, INFO_MIXER)
		SIZE_DEFAULTS(bar);
		scan_mixer_bar(arg, &obj->data.mixerbar.l, &obj->data.mixerbar.w,
			&obj->data.mixerbar.h);
#endif
	END OBJ_IF(if_mixer_mute, INFO_MIXER)
		obj->data.ifblock.i = mixer_init(arg);
#ifdef X11
	END OBJ(monitor, INFO_X11)
	END OBJ(monitor_number, INFO_X11)
	END OBJ(desktop, INFO_X11)
	END OBJ(desktop_number, INFO_X11)
	END OBJ(desktop_name, INFO_X11)
#endif
	END OBJ(nodename, 0)
	END OBJ(processes, INFO_PROCS)
	END OBJ(running_processes, INFO_RUN_PROCS)
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
		int a = get_stippled_borders(), b = 1;

		if (arg) {
			if (sscanf(arg, "%d %d", &a, &b) != 2) {
				sscanf(arg, "%d", &b);
			}
		}
		if (a <= 0) {
			a = 1;
		}
		obj->data.pair.a = a;
		obj->data.pair.b = b;
#endif /* X11 */
	END OBJ(swap, INFO_MEM)
	END OBJ(swapfree, INFO_MEM)
	END OBJ(swapmax, INFO_MEM)
	END OBJ(swapperc, INFO_MEM)
	END OBJ(swapbar, INFO_MEM)
		SIZE_DEFAULTS(bar);
		scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
	END OBJ(sysname, 0)
	END OBJ(time, 0)
		obj->data.s = strndup(arg ? arg : "%F %T", text_buffer_size);
	END OBJ(utime, 0)
		obj->data.s = strndup(arg ? arg : "%F %T", text_buffer_size);
	END OBJ(tztime, 0)
		char buf1[256], buf2[256], *fmt, *tz;

		fmt = tz = NULL;
		if (arg) {
			int nArgs = sscanf(arg, "%255s %255[^\n]", buf1, buf2);

			switch (nArgs) {
				case 2:
					tz = buf1;
				case 1:
					fmt = buf2;
			}
		}

		obj->data.tztime.fmt = strndup(fmt ? fmt : "%F %T", text_buffer_size);
		obj->data.tztime.tz = tz ? strndup(tz, text_buffer_size) : NULL;
#ifdef HAVE_ICONV
	END OBJ(iconv_start, 0)
		if (is_iconv_converting()) {
			CRIT_ERR(obj, free_at_crash, "You must stop your last iconv conversion before "
				"starting another");
		}
		if (arg) {
			char iconv_from[ICONV_CODEPAGE_LENGTH];
			char iconv_to[ICONV_CODEPAGE_LENGTH];

			if (sscanf(arg, "%s %s", iconv_from, iconv_to) != 2) {
				CRIT_ERR(obj, free_at_crash, "Invalid arguments for iconv_start");
			} else {
				iconv_t new_iconv;

				new_iconv = iconv_open(iconv_to, iconv_from);
				if (new_iconv == (iconv_t) (-1)) {
					NORM_ERR("Can't convert from %s to %s.", iconv_from, iconv_to);
				} else {
					obj->a = register_iconv(&new_iconv);
					set_iconv_converting(1);
				}
			}
		} else {
			CRIT_ERR(obj, free_at_crash, "Iconv requires arguments");
		}
	END OBJ(iconv_stop, 0)
		set_iconv_converting(0);

#endif
	END OBJ(totaldown, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(totalup, INFO_NET)
		obj->data.net = get_net_stat(arg, obj, free_at_crash);
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(updates, 0)
	END OBJ_IF(if_updatenr, 0)
		obj->data.ifblock.i = arg ? atoi(arg) : 0;
		if(obj->data.ifblock.i == 0) CRIT_ERR(obj, free_at_crash, "if_updatenr needs a number above 0 as argument");
		set_updatereset(obj->data.ifblock.i > get_updatereset() ? obj->data.ifblock.i : get_updatereset());
	END OBJ(alignr, 0)
		obj->data.i = arg ? atoi(arg) : 0;
	END OBJ(alignc, 0)
		obj->data.i = arg ? atoi(arg) : 0;
	END OBJ(upspeed, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}
	END OBJ(upspeedf, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg, obj, free_at_crash);
		} else {
			// default to DEFAULTNETDEV
			char *buf = strndup(DEFAULTNETDEV, text_buffer_size);
			obj->data.net = get_net_stat(buf, obj, free_at_crash);
			free(buf);
		}

#ifdef X11
	END OBJ(upspeedgraph, INFO_NET)
		char *buf = 0;
		SIZE_DEFAULTS(graph);
		buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
				&obj->e, &obj->char_a, &obj->char_b);

		// default to DEFAULTNETDEV
		buf = strndup(buf ? buf : DEFAULTNETDEV, text_buffer_size);
		obj->data.net = get_net_stat(buf, obj, free_at_crash);
		free(buf);
#endif
	END OBJ(uptime_short, INFO_UPTIME)
	END OBJ(uptime, INFO_UPTIME)
	END OBJ(user_names, INFO_USERS)
	END OBJ(user_times, INFO_USERS)
	END OBJ(user_terms, INFO_USERS)
	END OBJ(user_number, INFO_USERS)
#if defined(__linux__)
	END OBJ(gw_iface, INFO_GW)
	END OBJ(gw_ip, INFO_GW)
#endif /* !__linux__ */
#ifndef __OpenBSD__
	END OBJ(adt746xcpu, 0)
	END OBJ(adt746xfan, 0)
#endif /* !__OpenBSD__ */
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
		|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
	END OBJ(apm_adapter, 0)
	END OBJ(apm_battery_life, 0)
	END OBJ(apm_battery_time, 0)
#endif /* __FreeBSD__ */
	END OBJ(imap_unseen, 0)
		if (arg) {
			// proccss
			obj->data.mail = parse_mail_args(IMAP_TYPE, arg);
			obj->char_b = 0;
		} else {
			obj->char_b = 1;
		}
	END OBJ(imap_messages, 0)
		if (arg) {
			// proccss
			obj->data.mail = parse_mail_args(IMAP_TYPE, arg);
			obj->char_b = 0;
		} else {
			obj->char_b = 1;
		}
	END OBJ(pop3_unseen, 0)
		if (arg) {
			// proccss
			obj->data.mail = parse_mail_args(POP3_TYPE, arg);
			obj->char_b = 0;
		} else {
			obj->char_b = 1;
		}
	END OBJ(pop3_used, 0)
		if (arg) {
			// proccss
			obj->data.mail = parse_mail_args(POP3_TYPE, arg);
			obj->char_b = 0;
		} else {
			obj->char_b = 1;
		}
#ifdef IBM
	END OBJ(smapi, 0)
		if (arg)
			obj->data.s = strndup(arg, text_buffer_size);
		else
			NORM_ERR("smapi needs an argument");
	END OBJ_IF(if_smapi_bat_installed, 0)
		if (!arg) {
			NORM_ERR("if_smapi_bat_installed needs an argument");
			obj->data.ifblock.s = 0;
		} else
			obj->data.ifblock.s = strndup(arg, text_buffer_size);
	END OBJ(smapi_bat_perc, 0)
		if (arg)
			obj->data.s = strndup(arg, text_buffer_size);
		else
			NORM_ERR("smapi_bat_perc needs an argument");
	END OBJ(smapi_bat_temp, 0)
		if (arg)
			obj->data.s = strndup(arg, text_buffer_size);
		else
			NORM_ERR("smapi_bat_temp needs an argument");
	END OBJ(smapi_bat_power, 0)
		if (arg)
			obj->data.s = strndup(arg, text_buffer_size);
		else
			NORM_ERR("smapi_bat_power needs an argument");
#ifdef X11
	END OBJ(smapi_bat_bar, 0)
		SIZE_DEFAULTS(bar);
		if(arg) {
			int cnt;
			if(sscanf(arg, "%i %n", &obj->data.i, &cnt) <= 0) {
				NORM_ERR("first argument to smapi_bat_bar must be an integer value");
				obj->data.i = -1;
			} else {
				obj->b = 4;
				arg = scan_bar(arg + cnt, &obj->a, &obj->b);
			}
		} else
			NORM_ERR("smapi_bat_bar needs an argument");
#endif /* X11 */
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
	END OBJ(mpd_artist, INFO_MPD)
		mpd_set_maxlen(mpd_artist);
		init_mpd();
	END OBJ(mpd_title, INFO_MPD)
		mpd_set_maxlen(mpd_title);
		init_mpd();
	END OBJ(mpd_random, INFO_MPD) init_mpd();
	END OBJ(mpd_repeat, INFO_MPD) init_mpd();
	END OBJ(mpd_elapsed, INFO_MPD) init_mpd();
	END OBJ(mpd_length, INFO_MPD) init_mpd();
	END OBJ(mpd_track, INFO_MPD)
		mpd_set_maxlen(mpd_track);
		init_mpd();
	END OBJ(mpd_name, INFO_MPD)
		mpd_set_maxlen(mpd_name);
		init_mpd();
	END OBJ(mpd_file, INFO_MPD)
		mpd_set_maxlen(mpd_file);
		init_mpd();
	END OBJ(mpd_percent, INFO_MPD) init_mpd();
	END OBJ(mpd_album, INFO_MPD)
		mpd_set_maxlen(mpd_album);
		init_mpd();
	END OBJ(mpd_vol, INFO_MPD) init_mpd();
	END OBJ(mpd_bitrate, INFO_MPD) init_mpd();
	END OBJ(mpd_status, INFO_MPD) init_mpd();
	END OBJ(mpd_bar, INFO_MPD)
		SIZE_DEFAULTS(bar);
		scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
		init_mpd();
	END OBJ(mpd_smart, INFO_MPD)
		mpd_set_maxlen(mpd_smart);
		init_mpd();
	END OBJ_IF(if_mpd_playing, INFO_MPD)
		init_mpd();
#undef mpd_set_maxlen
#endif /* MPD */
#ifdef MOC
	END OBJ(moc_state, INFO_MOC)
	END OBJ(moc_file, INFO_MOC)
	END OBJ(moc_title, INFO_MOC)
	END OBJ(moc_artist, INFO_MOC)
	END OBJ(moc_song, INFO_MOC)
	END OBJ(moc_album, INFO_MOC)
	END OBJ(moc_totaltime, INFO_MOC)
	END OBJ(moc_timeleft, INFO_MOC)
	END OBJ(moc_curtime, INFO_MOC)
	END OBJ(moc_bitrate, INFO_MOC)
	END OBJ(moc_rate, INFO_MOC)
#endif /* MOC */
#ifdef XMMS2
	END OBJ(xmms2_artist, INFO_XMMS2)
	END OBJ(xmms2_album, INFO_XMMS2)
	END OBJ(xmms2_title, INFO_XMMS2)
	END OBJ(xmms2_genre, INFO_XMMS2)
	END OBJ(xmms2_comment, INFO_XMMS2)
	END OBJ(xmms2_url, INFO_XMMS2)
	END OBJ(xmms2_tracknr, INFO_XMMS2)
	END OBJ(xmms2_bitrate, INFO_XMMS2)
	END OBJ(xmms2_date, INFO_XMMS2)
	END OBJ(xmms2_id, INFO_XMMS2)
	END OBJ(xmms2_duration, INFO_XMMS2)
	END OBJ(xmms2_elapsed, INFO_XMMS2)
	END OBJ(xmms2_size, INFO_XMMS2)
	END OBJ(xmms2_status, INFO_XMMS2)
	END OBJ(xmms2_percent, INFO_XMMS2)
#ifdef X11
	END OBJ(xmms2_bar, INFO_XMMS2)
		SIZE_DEFAULTS(bar);
		scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
#endif /* X11 */
	END OBJ(xmms2_smart, INFO_XMMS2)
	END OBJ(xmms2_playlist, INFO_XMMS2)
	END OBJ(xmms2_timesplayed, INFO_XMMS2)
	END OBJ_IF(if_xmms2_connected, INFO_XMMS2)
#endif
#ifdef AUDACIOUS
	END OBJ(audacious_status, INFO_AUDACIOUS)
	END OBJ(audacious_title, INFO_AUDACIOUS)
		if (arg) {
			sscanf(arg, "%d", &info.audacious.max_title_len);
			if (info.audacious.max_title_len > 0) {
				info.audacious.max_title_len++;
			} else {
				CRIT_ERR(obj, free_at_crash, "audacious_title: invalid length argument");
			}
		}
	END OBJ(audacious_length, INFO_AUDACIOUS)
	END OBJ(audacious_length_seconds, INFO_AUDACIOUS)
	END OBJ(audacious_position, INFO_AUDACIOUS)
	END OBJ(audacious_position_seconds, INFO_AUDACIOUS)
	END OBJ(audacious_bitrate, INFO_AUDACIOUS)
	END OBJ(audacious_frequency, INFO_AUDACIOUS)
	END OBJ(audacious_channels, INFO_AUDACIOUS)
	END OBJ(audacious_filename, INFO_AUDACIOUS)
	END OBJ(audacious_playlist_length, INFO_AUDACIOUS)
	END OBJ(audacious_playlist_position, INFO_AUDACIOUS)
	END OBJ(audacious_main_volume, INFO_AUDACIOUS)
#ifdef X11
	END OBJ(audacious_bar, INFO_AUDACIOUS)
		SIZE_DEFAULTS(bar);
		scan_bar(arg, &obj->a, &obj->b);
#endif /* X11 */
#endif
#ifdef BMPX
	END OBJ(bmpx_title, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_artist, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_album, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_track, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_uri, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_bitrate, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
#endif
#ifdef EVE
	END OBJ(eve, 0)
		if(arg) {
			int argc;
			char *userid = (char *) malloc(20 * sizeof(char));
			char *apikey = (char *) malloc(64 * sizeof(char));
			char *charid = (char *) malloc(20 * sizeof(char));

			argc = sscanf(arg, "%20s %64s %20s", userid, apikey, charid);
			obj->data.eve.charid = charid;
			obj->data.eve.userid = userid;
			obj->data.eve.apikey = apikey;

			init_eve();
		} else {
			CRIT_ERR(obj, free_at_crash, "eve needs arguments: <userid> <apikey> <characterid>");
		}
#endif
#ifdef HAVE_CURL
	END OBJ(curl, 0)
		if (arg) {
			int argc;
			float interval = 0;
			char *uri = (char *) malloc(128 * sizeof(char));

			argc = sscanf(arg, "%127s %f", uri, &interval);
			if (argc == 2) {
				obj->data.curl.uri = uri;
				obj->data.curl.interval = interval > 0 ? interval * 60 : 15*60;
			} else {
				NORM_ERR("wrong number of arguments for $curl");
			}
		} else {
			CRIT_ERR(obj, free_at_crash, "curl needs arguments: <uri> <interval in minutes>");
		}
#endif
#ifdef RSS
	END OBJ(rss, 0)
		if (arg) {
			float interval = 0;
			int argc, act_par = 0;
			unsigned int nrspaces = 0;
			char *uri = (char *) malloc(128 * sizeof(char));
			char *action = (char *) malloc(64 * sizeof(char));

			argc = sscanf(arg, "%127s %f %63s %d %u", uri, &interval, action,
					&act_par, &nrspaces);
			if (argc >= 3) {
				obj->data.rss.uri = uri;
				obj->data.rss.interval = interval > 0 ? interval * 60 : 15*60;
				obj->data.rss.action = action;
				obj->data.rss.act_par = act_par;
				obj->data.rss.nrspaces = nrspaces;
			} else {
				NORM_ERR("wrong number of arguments for $rss");
			}
		} else {
			CRIT_ERR(obj, free_at_crash, "rss needs arguments: <uri> <interval in minutes> <action> "
					"[act_par] [spaces in front]");
		}
#endif
#ifdef WEATHER
	END OBJ(weather, 0)
		if (arg) {
			int argc;
			float interval = 0;
			char *locID = (char *) malloc(9 * sizeof(char));
			char *uri = (char *) malloc(128 * sizeof(char));
			char *data_type = (char *) malloc(32 * sizeof(char));

			argc = sscanf(arg, "%119s %8s %31s %f", uri, locID, data_type, &interval);

			if (argc >= 3) {
				if (process_weather_uri(uri, locID, 0)) {
					free(data_type);
					free(uri);
					free(locID);
					CRIT_ERR(obj, free_at_crash, \
							"could not recognize the weather uri");
				}

				obj->data.weather.uri = uri;
				obj->data.weather.data_type = data_type;

				/* Limit the data retrieval interval to half hour min */
				if (interval < 30) {
					interval = 30;
				}

				/* Convert to seconds */
				obj->data.weather.interval = interval * 60;
				free(locID);

				DBGP("weather: fetching %s from %s every %d seconds", \
						data_type, uri, obj->data.weather.interval);
			} else {
				free(data_type);
				free(uri);
				free(locID);
				CRIT_ERR(obj, free_at_crash, "wrong number of arguments for $weather");
			}
		} else {
			CRIT_ERR(obj, free_at_crash, "weather needs arguments: <uri> <locID> <data_type> [interval in minutes]");
		}
#endif
#ifdef XOAP
	END OBJ(weather_forecast, 0)
		if (arg) {
			int argc;
			unsigned int day;
			float interval = 0;
			char *locID = (char *) malloc(9 * sizeof(char));
			char *uri = (char *) malloc(128 * sizeof(char));
			char *data_type = (char *) malloc(32 * sizeof(char));

			argc = sscanf(arg, "%119s %8s %1u %31s %f", uri, locID, &day, data_type, &interval);

			if (argc >= 4) {
				if (process_weather_uri(uri, locID, 1)) {
					free(data_type);
					free(uri);
					free(locID);
					CRIT_ERR(obj, free_at_crash, \
							"could not recognize the weather forecast uri");
				}

				obj->data.weather_forecast.uri = uri;
				obj->data.weather_forecast.data_type = data_type;

				/* Limit the day between 0 (today) and FORECAST_DAYS */
				if (day >= FORECAST_DAYS) {
					day = FORECAST_DAYS-1;
				}
				obj->data.weather_forecast.day = day;

				/* Limit the data retrieval interval to 3 hours and an half */
				if (interval < 210) {
					interval = 210;
				}

				/* Convert to seconds */
				obj->data.weather_forecast.interval = interval * 60;
				free(locID);

				DBGP("weather_forecast: fetching %s for day %d from %s every %d seconds", \
					 data_type, day, uri, obj->data.weather_forecast.interval);
			} else {
				free(data_type);
				free(uri);
				free(locID);
				CRIT_ERR(obj, free_at_crash, "wrong number of arguments for $weather_forecast");
			}
		} else {
			CRIT_ERR(obj, free_at_crash, "weather_forecast needs arguments: <uri> <locID> <day> <data_type> [interval in minutes]");
		}
#endif
#ifdef HAVE_LUA
	END OBJ(lua, 0)
		if (arg) {
			obj->data.s = strndup(arg, text_buffer_size);
		} else {
			CRIT_ERR(obj, free_at_crash, "lua needs arguments: <function name> [function parameters]");
		}
	END OBJ(lua_parse, 0)
		if (arg) {
			obj->data.s = strndup(arg, text_buffer_size);
		} else {
			CRIT_ERR(obj, free_at_crash, "lua_parse needs arguments: <function name> [function parameters]");
		}
	END OBJ(lua_bar, 0)
		SIZE_DEFAULTS(bar);
		if (arg) {
			arg = scan_bar(arg, &obj->a, &obj->b);
			if(arg) {
				obj->data.s = strndup(arg, text_buffer_size);
			} else {
				CRIT_ERR(obj, free_at_crash, "lua_bar needs arguments: <height>,<width> <function name> [function parameters]");
			}
		} else {
			CRIT_ERR(obj, free_at_crash, "lua_bar needs arguments: <height>,<width> <function name> [function parameters]");
		}
#ifdef X11
	END OBJ(lua_graph, 0)
		SIZE_DEFAULTS(graph);
		if (arg) {
			char *buf = 0;
			buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
					&obj->e, &obj->char_a, &obj->char_b);
			if (buf) {
				obj->data.s = buf;
			} else {
				CRIT_ERR(obj, free_at_crash, "lua_graph needs arguments: <function name> [height],[width] [gradient colour 1] [gradient colour 2] [scale] [-t] [-l]");
			}
		} else {
			CRIT_ERR(obj, free_at_crash, "lua_graph needs arguments: <function name> [height],[width] [gradient colour 1] [gradient colour 2] [scale] [-t] [-l]");
	}
	END OBJ(lua_gauge, 0)
		SIZE_DEFAULTS(gauge);
		if (arg) {
			arg = scan_gauge(arg, &obj->a, &obj->b);
			if (arg) {
				obj->data.s = strndup(arg, text_buffer_size);
			} else {
				CRIT_ERR(obj, free_at_crash, "lua_gauge needs arguments: <height>,<width> <function name> [function parameters]");
			}
		} else {
			CRIT_ERR(obj, free_at_crash, "lua_gauge needs arguments: <height>,<width> <function name> [function parameters]");
		}
#endif /* X11 */
#endif /* HAVE_LUA */
#ifdef HDDTEMP
	END OBJ(hddtemp, 0)
		if (scan_hddtemp(arg, &obj->data.hddtemp.dev,
		                 &obj->data.hddtemp.addr, &obj->data.hddtemp.port)) {
			NORM_ERR("hddtemp needs arguments");
			obj->type = OBJ_text;
			obj->data.s = strndup("${hddtemp}", text_buffer_size);
			obj->data.hddtemp.update_time = 0;
		} else
			obj->data.hddtemp.temp = NULL;
#endif /* HDDTEMP */
#ifdef TCP_PORT_MONITOR
	END OBJ(tcp_portmon, INFO_TCP_PORT_MONITOR)
		tcp_portmon_init(arg, &obj->data.tcp_port_monitor);
#endif /* TCP_PORT_MONITOR */
	END OBJ(entropy_avail, INFO_ENTROPY)
	END OBJ(entropy_perc, INFO_ENTROPY)
	END OBJ(entropy_poolsize, INFO_ENTROPY)
	END OBJ(entropy_bar, INFO_ENTROPY)
		SIZE_DEFAULTS(bar);
		scan_bar(arg, &obj->a, &obj->b);
	END OBJ(include, 0)
		if(arg) {
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
		} else {
			CRIT_ERR(obj, free_at_crash, "include needs a argument");
		}
	END OBJ(blink, 0)
		if(arg) {
			obj->sub = malloc(sizeof(struct text_object));
			extract_variable_text_internal(obj->sub, arg);
		}else{
			CRIT_ERR(obj, free_at_crash, "blink needs a argument");
		}
	END OBJ(to_bytes, 0)
		if(arg) {
			obj->sub = malloc(sizeof(struct text_object));
			extract_variable_text_internal(obj->sub, arg);
		}else{
			CRIT_ERR(obj, free_at_crash, "to_bytes needs a argument");
		}
	END OBJ(scroll, 0)
		int n1 = 0, n2 = 0;

		obj->data.scroll.resetcolor = get_current_text_color();
		obj->data.scroll.step = 1;
		if (arg && sscanf(arg, "%u %n", &obj->data.scroll.show, &n1) > 0) {
			sscanf(arg + n1, "%u %n", &obj->data.scroll.step, &n2);
			if (*(arg + n1 + n2)) {
				n1 += n2;
			} else {
				obj->data.scroll.step = 1;
			}
			obj->data.scroll.text = malloc(strlen(arg + n1) + obj->data.scroll.show + 1);
			for(n2 = 0; (unsigned int) n2 < obj->data.scroll.show; n2++) {
				obj->data.scroll.text[n2] = ' ';
			}
			obj->data.scroll.text[n2] = 0;
			strcat(obj->data.scroll.text, arg + n1);
			obj->data.scroll.start = 0;
			obj->sub = malloc(sizeof(struct text_object));
			extract_variable_text_internal(obj->sub,
					obj->data.scroll.text);
		} else {
			CRIT_ERR(obj, free_at_crash, "scroll needs arguments: <length> [<step>] <text>");
		}
	END OBJ(combine, 0)
		if(arg) {
			unsigned int i,j;
			unsigned int indenting = 0;	//vars can be used as args for other vars
			int startvar[2];
			int endvar[2];
			startvar[0] = endvar[0] = startvar[1] = endvar[1] = -1;
			j=0;
			for (i=0; arg[i] != 0 && j < 2; i++) {
				if(startvar[j] == -1) {
					if(arg[i] == '$') {
						startvar[j] = i;
					}
				}else if(endvar[j] == -1) {
					if(arg[i] == '{') {
						indenting++;
					}else if(arg[i] == '}') {
						indenting--;
					}
					if (indenting == 0 && arg[i+1] < 48) {	//<48 has 0, $, and the most used chars not used in varnames but not { or }
						endvar[j]=i+1;
						j++;
					}
				}
			}
			if(startvar[0] >= 0 && endvar[0] >= 0 && startvar[1] >= 0 && endvar[1] >= 0) {
				obj->data.combine.left = malloc(endvar[0]-startvar[0] + 1);
				obj->data.combine.seperation = malloc(startvar[1] - endvar[0] + 1);
				obj->data.combine.right= malloc(endvar[1]-startvar[1] + 1);

				strncpy(obj->data.combine.left, arg + startvar[0], endvar[0] - startvar[0]);
				obj->data.combine.left[endvar[0] - startvar[0]] = 0;

				strncpy(obj->data.combine.seperation, arg + endvar[0], startvar[1] - endvar[0]);
				obj->data.combine.seperation[startvar[1] - endvar[0]] = 0;

				strncpy(obj->data.combine.right, arg + startvar[1], endvar[1] - startvar[1]);
				obj->data.combine.right[endvar[1] - startvar[1]] = 0;

				obj->sub = malloc(sizeof(struct text_object));
				extract_variable_text_internal(obj->sub, obj->data.combine.left);
				obj->sub->sub = malloc(sizeof(struct text_object));
				extract_variable_text_internal(obj->sub->sub, obj->data.combine.right);
			} else {
				CRIT_ERR(obj, free_at_crash, "combine needs arguments: <text1> <text2>");
			}
		} else {
			CRIT_ERR(obj, free_at_crash, "combine needs arguments: <text1> <text2>");
		}
#ifdef NVIDIA
	END OBJ(nvidia, 0)
		if (!arg) {
			CRIT_ERR(obj, free_at_crash, "nvidia needs an argument\n");
		} else if (set_nvidia_type(&obj->data.nvidia, arg)) {
			CRIT_ERR(obj, free_at_crash, "nvidia: invalid argument"
				 " specified: '%s'\n", arg);
		}
#endif /* NVIDIA */
#ifdef APCUPSD
		init_apcupsd();
		END OBJ(apcupsd, INFO_APCUPSD)
			if (arg) {
				char host[64];
				int port;
				if (sscanf(arg, "%63s %d", host, &port) != 2) {
					CRIT_ERR(obj, free_at_crash, "apcupsd needs arguments: <host> <port>");
				} else {
					info.apcupsd.port = htons(port);
					strncpy(info.apcupsd.host, host, sizeof(info.apcupsd.host));
				}
			} else {
				CRIT_ERR(obj, free_at_crash, "apcupsd needs arguments: <host> <port>");
			}
			END OBJ(apcupsd_name, INFO_APCUPSD)
			END OBJ(apcupsd_model, INFO_APCUPSD)
			END OBJ(apcupsd_upsmode, INFO_APCUPSD)
			END OBJ(apcupsd_cable, INFO_APCUPSD)
			END OBJ(apcupsd_status, INFO_APCUPSD)
			END OBJ(apcupsd_linev, INFO_APCUPSD)
			END OBJ(apcupsd_load, INFO_APCUPSD)
			END OBJ(apcupsd_loadbar, INFO_APCUPSD)
				SIZE_DEFAULTS(bar);
				scan_bar(arg, &obj->a, &obj->b);
#ifdef X11
			END OBJ(apcupsd_loadgraph, INFO_APCUPSD)
				char* buf = 0;
				SIZE_DEFAULTS(graph);
				buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
						&obj->e, &obj->char_a, &obj->char_b);
				if (buf) free(buf);
			END OBJ(apcupsd_loadgauge, INFO_APCUPSD)
				SIZE_DEFAULTS(gauge);
				scan_gauge(arg, &obj->a, &obj->b);
#endif /* X11 */
			END OBJ(apcupsd_charge, INFO_APCUPSD)
			END OBJ(apcupsd_timeleft, INFO_APCUPSD)
			END OBJ(apcupsd_temp, INFO_APCUPSD)
			END OBJ(apcupsd_lastxfer, INFO_APCUPSD)
#endif /* APCUPSD */
	END {
		char buf[256];

		NORM_ERR("unknown variable %s", s);
		obj->type = OBJ_text;
		snprintf(buf, 256, "${%s}", s);
		obj->data.s = strndup(buf, text_buffer_size);
	}
#undef OBJ

	return obj;
}

/* backslash_escape - do the actual substitution task for template objects
 *
 * The field templates is used for substituting the \N occurences. Set it to
 * NULL to leave them as they are.
 */
static char *backslash_escape(const char *src, char **templates, unsigned int template_count)
{
	char *src_dup;
	const char *p;
	unsigned int dup_idx = 0, dup_len;

	dup_len = strlen(src) + 1;
	src_dup = malloc(dup_len * sizeof(char));

	p = src;
	while (*p) {
		switch (*p) {
		case '\\':
			if (!*(p + 1))
				break;
			if (*(p + 1) == '\\') {
				src_dup[dup_idx++] = '\\';
				p++;
			} else if (*(p + 1) == ' ') {
				src_dup[dup_idx++] = ' ';
				p++;
			} else if (*(p + 1) == 'n') {
				src_dup[dup_idx++] = '\n';
				p++;
			} else if (templates) {
				unsigned int tmpl_num;
				int digits;
				if ((sscanf(p + 1, "%u%n", &tmpl_num, &digits) <= 0) ||
				    (tmpl_num > template_count))
					break;
				dup_len += strlen(templates[tmpl_num - 1]);
				src_dup = realloc(src_dup, dup_len * sizeof(char));
				sprintf(src_dup + dup_idx, "%s", templates[tmpl_num - 1]);
				dup_idx += strlen(templates[tmpl_num - 1]);
				p += digits;
			}
			break;
		default:
			src_dup[dup_idx++] = *p;
			break;
		}
		p++;
	}
	src_dup[dup_idx] = '\0';
	src_dup = realloc(src_dup, (strlen(src_dup) + 1) * sizeof(char));
	return src_dup;
}

/* handle_template_object - core logic of the template object
 *
 * use config variables like this:
 * template1 = "$\1\2"
 * template2 = "\1: ${fs_bar 4,100 \2} ${fs_used \2} / ${fs_size \2}"
 *
 * and use them like this:
 * ${template1 node name}
 * ${template2 root /}
 * ${template2 cdrom /mnt/cdrom}
 */
static char *handle_template(const char *tmpl, const char *args)
{
	char *args_dup = NULL;
	char *p, *p_old;
	char **argsp = NULL;
	unsigned int argcnt = 0, template_idx, i;
	char *eval_text;

	if ((sscanf(tmpl, "template%u", &template_idx) != 1) ||
	    (template_idx >= MAX_TEMPLATES))
		return NULL;

	if(args) {
		args_dup = strdup(args);
		p = args_dup;
		while (*p) {
			while (*p && (*p == ' ' && (p == args_dup || *(p - 1) != '\\')))
				p++;
			if (p > args_dup && *(p - 1) == '\\')
				p--;
			p_old = p;
			while (*p && (*p != ' ' || (p > args_dup && *(p - 1) == '\\')))
				p++;
			if (*p) {
				(*p) = '\0';
				p++;
			}
			argsp = realloc(argsp, ++argcnt * sizeof(char *));
			argsp[argcnt - 1] = p_old;
		}
		for (i = 0; i < argcnt; i++) {
			char *tmp;
			tmp = backslash_escape(argsp[i], NULL, 0);
			DBGP2("%s: substituted arg '%s' to '%s'", tmpl, argsp[i], tmp);
			argsp[i] = tmp;
		}
	}

	eval_text = backslash_escape(get_templates()[template_idx], argsp, argcnt);
	DBGP("substituted %s, output is '%s'", tmpl, eval_text);
	free(args_dup);
	for (i = 0; i < argcnt; i++)
		free(argsp[i]);
	free(argsp);
	return eval_text;
}

static char *find_and_replace_templates(const char *inbuf)
{
	char *outbuf, *indup, *p, *o, *templ, *args, *tmpl_out;
	int stack, outlen;

	outlen = strlen(inbuf) + 1;
	o = outbuf = calloc(outlen, sizeof(char));
	memset(outbuf, 0, outlen * sizeof(char));

	p = indup = strdup(inbuf);
	while (*p) {
		while (*p && *p != '$')
			*(o++) = *(p++);

		if (!(*p))
			break;

		if (strncmp(p, "$template", 9) && strncmp(p, "${template", 10)) {
			*(o++) = *(p++);
			continue;
		}

		if (*(p + 1) == '{') {
			p += 2;
			templ = p;
			while (*p && !isspace(*p) && *p != '{' && *p != '}')
				p++;
			if (*p == '}')
				args = NULL;
			else
				args = p;

			stack = 1;
			while (*p && stack > 0) {
				if (*p == '{')
					stack++;
				else if (*p == '}')
					stack--;
				p++;
			}
			if (stack == 0) {
				// stack is empty. that means the previous char was }, so we zero it
				*(p - 1) = '\0';
			} else {
				// we ran into the end of string without finding a closing }, bark
				CRIT_ERR(NULL, NULL, "cannot find a closing '}' in template expansion");
			}
		} else {
			templ = p + 1;
			while (*p && !isspace(*p))
				p++;
			args = NULL;
		}
		tmpl_out = handle_template(templ, args);
		if (tmpl_out) {
			outlen += strlen(tmpl_out);
			*o = '\0';
			outbuf = realloc(outbuf, outlen * sizeof(char));
			strcat (outbuf, tmpl_out);
			free(tmpl_out);
			o = outbuf + strlen(outbuf);
		} else {
			NORM_ERR("failed to handle template '%s' with args '%s'", templ, args);
		}
	}
	*o = '\0';
	outbuf = realloc(outbuf, (strlen(outbuf) + 1) * sizeof(char));
	free(indup);
	return outbuf;
}

static int text_contains_templates(const char *text)
{
	if (strcasestr(text, "${template") != NULL)
		return 1;
	if (strcasestr(text, "$template") != NULL)
		return 1;
	return 0;
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
		DBGP("replaced all templates in text: input is\n'%s'\noutput is\n'%s'", const_p, p);
	} else {
		DBGP("no templates to replace");
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
				char buf[256];
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
				len = (p - s > 255) ? 255 : (p - s);
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
				close(data.sysfs.fd);
				break;
#endif /* __linux__ */
			case OBJ_read_tcp:
				free(data.read_tcp.host);
				break;
			case OBJ_time:
			case OBJ_utime:
				free(data.s);
				break;
			case OBJ_tztime:
				free(data.tztime.tz);
				free(data.tztime.fmt);
				break;
			case OBJ_mboxscan:
				free(data.mboxscan.args);
				free(data.mboxscan.output);
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
				free(data.local_mail.mbox);
				break;
			case OBJ_imap_unseen:
				if (!obj->char_b) {
					free(data.mail);
				}
				break;
			case OBJ_imap_messages:
				if (!obj->char_b) {
					free(data.mail);
				}
				break;
			case OBJ_pop3_unseen:
				if (!obj->char_b) {
					free(data.mail);
				}
				break;
			case OBJ_pop3_used:
				if (!obj->char_b) {
					free(data.mail);
				}
				break;
			case OBJ_if_empty:
			case OBJ_if_match:
				free_text_objects(obj->sub, 1);
				free(obj->sub);
				/* fall through */
			case OBJ_if_existing:
			case OBJ_if_mounted:
			case OBJ_if_running:
				free(data.ifblock.s);
				free(data.ifblock.str);
				break;
			case OBJ_head:
			case OBJ_tail:
				free(data.headtail.logfile);
				if(data.headtail.buffer) {
					free(data.headtail.buffer);
				}
				break;
			case OBJ_text:
			case OBJ_font:
			case OBJ_image:
			case OBJ_eval:
			case OBJ_exec:
			case OBJ_execbar:
#ifdef X11
			case OBJ_execgauge:
			case OBJ_execgraph:
#endif
			case OBJ_execp:
				free(data.s);
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
				free(data.ifblock.s);
				free(data.ifblock.str);
			case OBJ_gw_iface:
			case OBJ_gw_ip:
				if (info.gw_info.iface) {
					free(info.gw_info.iface);
					info.gw_info.iface = 0;
				}
				if (info.gw_info.ip) {
					free(info.gw_info.ip);
					info.gw_info.ip = 0;
				}
				break;
			case OBJ_ioscheduler:
				if(data.s)
					free(data.s);
				break;
#endif
#if (defined(__FreeBSD__) || defined(__linux__))
			case OBJ_if_up:
				free(data.ifblock.s);
				free(data.ifblock.str);
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
				break;
#endif
#ifdef HAVE_CURL
			case OBJ_curl:
				free(data.curl.uri);
				break;
#endif
#ifdef RSS
			case OBJ_rss:
				free(data.rss.uri);
				free(data.rss.action);
				break;
#endif
#ifdef WEATHER
			case OBJ_weather:
				free(data.weather.uri);
				free(data.weather.data_type);
				break;
#endif
#ifdef XOAP
			case OBJ_weather_forecast:
				free(data.weather_forecast.uri);
				free(data.weather_forecast.data_type);
				break;
#endif
#ifdef HAVE_LUA
			case OBJ_lua:
			case OBJ_lua_parse:
			case OBJ_lua_bar:
#ifdef X11
			case OBJ_lua_graph:
			case OBJ_lua_gauge:
#endif /* X11 */
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
#ifdef X11
			case OBJ_execigraph:
			case OBJ_execigauge:
#endif /* X11 */
				free(data.execi.cmd);
				free(data.execi.buffer);
				break;
			case OBJ_texeci:
				if (data.texeci.p_timed_thread) timed_thread_destroy(data.texeci.p_timed_thread, &data.texeci.p_timed_thread);
				free(data.texeci.cmd);
				free(data.texeci.buffer);
				break;
			case OBJ_nameserver:
				free_dns_data();
				break;
			case OBJ_top:
			case OBJ_top_mem:
			case OBJ_top_time:
#ifdef IOSTATS
			case OBJ_top_io:
#endif
				if (info.first_process && !internal) {
					free_all_processes();
					info.first_process = NULL;
				}
				if (data.top.s) free(data.top.s);
				break;
#ifdef HDDTEMP
			case OBJ_hddtemp:
				free(data.hddtemp.dev);
				free(data.hddtemp.addr);
				if (data.hddtemp.temp)
					free(data.hddtemp.temp);
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
#ifdef IBM
			case OBJ_smapi:
			case OBJ_smapi_bat_perc:
			case OBJ_smapi_bat_temp:
			case OBJ_smapi_bat_power:
				free(data.s);
				break;
			case OBJ_if_smapi_bat_installed:
				free(data.ifblock.s);
				free(data.ifblock.str);
				break;
#endif /* IBM */
#ifdef NVIDIA
			case OBJ_nvidia:
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
				free(data.scroll.text);
				free_text_objects(obj->sub, 1);
				free(obj->sub);
				break;
			case OBJ_combine:
				free(data.combine.left);
				free(data.combine.seperation);
				free(data.combine.right);
				free_text_objects(obj->sub, 1);
				free(obj->sub);
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
			case OBJ_apcupsd_loadgauge:
#endif /* X11 */
			case OBJ_apcupsd_charge:
			case OBJ_apcupsd_timeleft:
			case OBJ_apcupsd_temp:
			case OBJ_apcupsd_lastxfer:
				break;
#endif /* APCUPSD */
#ifdef X11
			case OBJ_desktop:
			case OBJ_desktop_number:
			case OBJ_desktop_name:
			        if(info.x11.desktop.name) {
				  free(info.x11.desktop.name);
				  info.x11.desktop.name = NULL;
			        }
			        if(info.x11.desktop.all_names) {
				  free(info.x11.desktop.all_names);
				  info.x11.desktop.all_names = NULL;
			        }
				break;
#endif /* X11 */
		}
		free(obj);
	}
#undef data
}

