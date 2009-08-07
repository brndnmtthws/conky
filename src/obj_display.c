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
#include "obj_display.h"

//remove backspaced chars, example: "dog^H^H^Hcat" becomes "cat"
//string has to end with \0 and it's length should fit in a int
#define BACKSPACE 8
void remove_deleted_chars(char *string){
	int i = 0;
	while(string[i] != 0){
		if(string[i] == BACKSPACE){
			if(i != 0){
				strcpy( &(string[i-1]), &(string[i+1]) );
				i--;
			}else strcpy( &(string[i]), &(string[i+1]) ); //necessary for ^H's at the start of a string
		}else i++;
	}
}

static inline void format_media_player_time(char *buf, const int size,
		int seconds)
{
	int days, hours, minutes;

	days = seconds / (24 * 60 * 60);
	seconds %= (24 * 60 * 60);
	hours = seconds / (60 * 60);
	seconds %= (60 * 60);
	minutes = seconds / 60;
	seconds %= 60;

	if (days > 0) {
		snprintf(buf, size, "%i days %i:%02i:%02i", days,
				hours, minutes, seconds);
	} else if (hours > 0) {
		snprintf(buf, size, "%i:%02i:%02i", hours, minutes,
				seconds);
	} else {
		snprintf(buf, size, "%i:%02i", minutes, seconds);
	}
}

static inline double get_barnum(char *buf)
{
	char *c = buf;
	double barnum;

	while (*c) {
		if (*c == '\001') {
			*c = ' ';
		}
		c++;
	}

	if (sscanf(buf, "%lf", &barnum) == 0) {
		NORM_ERR("reading exec value failed (perhaps it's not the "
				"correct format?)");
		return -1;
	}
	if (barnum > 100.0 || barnum < 0.0) {
		NORM_ERR("your exec value is not between 0 and 100, "
				"therefore it will be ignored");
		return -1;
	}
	return barnum;
}

char *format_time(unsigned long timeval, const int width)
{
	char buf[10];
	unsigned long nt;	// narrow time, for speed on 32-bit
	unsigned cc;		// centiseconds
	unsigned nn;		// multi-purpose whatever

	nt = timeval;
	cc = nt % 100;		// centiseconds past second
	nt /= 100;			// total seconds
	nn = nt % 60;		// seconds past the minute
	nt /= 60;			// total minutes
	if (width >= snprintf(buf, sizeof buf, "%lu:%02u.%02u",
				nt, nn, cc)) {
		return strndup(buf, text_buffer_size);
	}
	if (width >= snprintf(buf, sizeof buf, "%lu:%02u", nt, nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn = nt % 60;		// minutes past the hour
	nt /= 60;			// total hours
	if (width >= snprintf(buf, sizeof buf, "%lu,%02u", nt, nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn = nt;			// now also hours
	if (width >= snprintf(buf, sizeof buf, "%uh", nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn /= 24;			// now days
	if (width >= snprintf(buf, sizeof buf, "%ud", nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn /= 7;			// now weeks
	if (width >= snprintf(buf, sizeof buf, "%uw", nn)) {
		return strndup(buf, text_buffer_size);
	}
	// well shoot, this outta' fit...
	return strndup("<inf>", text_buffer_size);
}

/* Prints anything normally printed with snprintf according to the current value
 * of use_spacer.  Actually slightly more flexible than snprintf, as you can
 * safely specify the destination buffer as one of your inputs.  */
int spaced_print(char *buf, int size, const char *format, int width, ...)
{
	int len = 0;
	va_list argp;
	char *tempbuf;

	if (size < 1) {
		return 0;
	}
	tempbuf = malloc(size * sizeof(char));

	// Passes the varargs along to vsnprintf
	va_start(argp, width);
	vsnprintf(tempbuf, size, format, argp);
	va_end(argp);

	switch (use_spacer) {
		case NO_SPACER:
			len = snprintf(buf, size, "%s", tempbuf);
			break;
		case LEFT_SPACER:
			len = snprintf(buf, size, "%*s", width, tempbuf);
			break;
		case RIGHT_SPACER:
			len = snprintf(buf, size, "%-*s", width, tempbuf);
			break;
	}
	free(tempbuf);
	return len;
}

/* print percentage values
 *
 * - i.e., unsigned values between 0 and 100
 * - respect the value of pad_percents */
static int percent_print(char *buf, int size, unsigned value)
{
	return spaced_print(buf, size, "%u", pad_percents, value);
}

static const char *suffixes[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "" };

/* converts from bytes to human readable format (K, M, G, T)
 *
 * The algorithm always divides by 1024, as unit-conversion of byte
 * counts suggests. But for output length determination we need to
 * compare with 1000 here, as we print in decimal form. */
static void human_readable(long long num, char *buf, int size)
{
	const char **suffix = suffixes;
	float fnum;
	int precision;
	int width;
	const char *format;

	/* Possibly just output as usual, for example for stdout usage */
	if (!format_human_readable) {
		spaced_print(buf, size, "%d", 6, round_to_int(num));
		return;
	}
	if (short_units) {
		width = 5;
		format = "%.*f%.1s";
	} else {
		width = 7;
		format = "%.*f%-3s";
	}

	if (llabs(num) < 1000LL) {
		spaced_print(buf, size, format, width, 0, (float)num, *suffix);
		return;
	}

	while (llabs(num / 1024) >= 1000LL && **(suffix + 2)) {
		num /= 1024;
		suffix++;
	}

	suffix++;
	fnum = num / 1024.0;

	/* fnum should now be < 1000, so looks like 'AAA.BBBBB'
	 *
	 * The goal is to always have a significance of 3, by
	 * adjusting the decimal part of the number. Sample output:
	 *  123MiB
	 * 23.4GiB
	 * 5.12B
	 * so the point of alignment resides between number and unit. The
	 * upside of this is that there is minimal padding necessary, though
	 * there should be a way to make alignment take place at the decimal
	 * dot (then with fixed width decimal part).
	 *
	 * Note the repdigits below: when given a precision value, printf()
	 * rounds the float to it, not just cuts off the remaining digits. So
	 * e.g. 99.95 with a precision of 1 gets 100.0, which again should be
	 * printed with a precision of 0. Yay. */

	precision = 0;		/* print 100-999 without decimal part */
	if (fnum < 99.95)
		precision = 1;	/* print 10-99 with one decimal place */
	if (fnum < 9.995)
		precision = 2;	/* print 0-9 with two decimal places */

	spaced_print(buf, size, format, width, precision, fnum, *suffix);
}

/* substitutes all occurrences of '\n' with SECRIT_MULTILINE_CHAR, which allows
 * multiline objects like $exec work with $align[rc] and friends
 */
void substitute_newlines(char *p, long l)
{
	char *s = p;
	if (l < 0) return;
	while (p && *p && p < s + l) {
		if (*p == '\n') {
			/* only substitute if it's not the last newline */
			*p = SECRIT_MULTILINE_CHAR;
		}
		p++;
	}
}

void evaluate(const char *text, char *buffer)
{
	struct information *tmp_info;
	struct text_object subroot;

	tmp_info = malloc(sizeof(struct information));
	memcpy(tmp_info, &ctx->info, sizeof(struct information));
	parse_conky_vars(&subroot, text, buffer, tmp_info);
	DBGP("evaluated '%s' to '%s'", text, buffer);

	free_text_objects(&subroot, 1);
	free(tmp_info);
}

void generate_text_internal(char *p, int p_max_size, struct text_object root,
		struct information *cur)
{
	struct text_object *obj;
#ifdef X11
	int need_to_load_fonts = 0;
#endif /* X11 */

	/* for the OBJ_top* handler */
	struct process **needed = 0;

#ifdef HAVE_ICONV
	char buff_in[p_max_size];
	buff_in[0] = 0;
	set_iconv_converting(0);
#endif /* HAVE_ICONV */

	p[0] = 0;
	obj = root.next;
	while (obj && p_max_size > 0) {
		needed = 0; /* reset for top stuff */

/* IFBLOCK jumping algorithm
 *
 * This is easier as it looks like:
 * - each IF checks it's condition
 *   - on FALSE: call DO_JUMP
 *   - on TRUE: don't care
 * - each ELSE calls DO_JUMP unconditionally
 * - each ENDIF is silently being ignored
 *
 * Why this works:
 * DO_JUMP overwrites the "obj" variable of the loop and sets it to the target
 * (i.e. the corresponding ELSE or ENDIF). After that, processing for the given
 * object can continue, free()ing stuff e.g., then the for-loop does the rest: as
 * regularly, "obj" is being updated to point to obj->next, so object parsing
 * continues right after the corresponding ELSE or ENDIF. This means that if we
 * find an ELSE, it's corresponding IF must not have jumped, so we need to jump
 * always. If we encounter an ENDIF, it's corresponding IF or ELSE has not
 * jumped, and there is nothing to do.
 */
#define DO_JUMP { \
	DBGP2("jumping"); \
	obj = obj->data.ifblock.next; \
}

#define OBJ(a) break; case OBJ_##a:

		switch (obj->type) {
			default:
				NORM_ERR("not implemented obj type %d", obj->type);
			OBJ(read_tcp) {
				int sock, received;
				struct sockaddr_in addr;
				struct hostent* he = gethostbyname(obj->data.read_tcp.host);
				if(he != NULL) {
					sock = socket(he->h_addrtype, SOCK_STREAM, 0);
					if(sock != -1) {
						memset(&addr, 0, sizeof(addr));
						addr.sin_family = AF_INET;
						addr.sin_port = obj->data.read_tcp.port;
						memcpy(&addr.sin_addr, he->h_addr, he->h_length);
						if (connect(sock, (struct sockaddr*)&addr, sizeof(struct sockaddr)) == 0) {
							fd_set readfds;
							struct timeval tv;
							FD_ZERO(&readfds);
							FD_SET(sock, &readfds);
							tv.tv_sec = 1;
							tv.tv_usec = 0;
							if(select(sock + 1, &readfds, NULL, NULL, &tv) > 0){
								received = recv(sock, p, p_max_size, 0);
								p[received] = 0;
							}
							close(sock);
						} else {
							NORM_ERR("read_tcp: Couldn't create a connection");
						}
					}else{
						NORM_ERR("read_tcp: Couldn't create a socket");
					}
				}else{
					NORM_ERR("read_tcp: Problem with resolving the hostname");
				}
			}
#ifndef __OpenBSD__
			OBJ(acpitemp) {
				temp_print(p, p_max_size, get_acpi_temperature(obj->data.i), TEMP_CELSIUS);
			}
#endif /* !__OpenBSD__ */
			OBJ(freq) {
				if (obj->a) {
					obj->a = get_freq(p, p_max_size, "%.0f", 1,
							obj->data.cpu_index);
				}
			}
			OBJ(freq_g) {
				if (obj->a) {
#ifndef __OpenBSD__
					obj->a = get_freq(p, p_max_size, "%'.2f", 1000,
							obj->data.cpu_index);
#else
					/* OpenBSD has no such flag (SUSv2) */
					obj->a = get_freq(p, p_max_size, "%.2f", 1000,
							obj->data.cpu_index);
#endif /* __OpenBSD */
				}
			}
#if defined(__linux__)
			OBJ(voltage_mv) {
				if (obj->a) {
					obj->a = get_voltage(p, p_max_size, "%.0f", 1,
							obj->data.cpu_index);
				}
			}
			OBJ(voltage_v) {
				if (obj->a) {
					obj->a = get_voltage(p, p_max_size, "%'.3f", 1000,
							obj->data.cpu_index);
				}
			}

#ifdef HAVE_IWLIB
			OBJ(wireless_essid) {
				snprintf(p, p_max_size, "%s", obj->data.net->essid);
			}
			OBJ(wireless_mode) {
				snprintf(p, p_max_size, "%s", obj->data.net->mode);
			}
			OBJ(wireless_bitrate) {
				snprintf(p, p_max_size, "%s", obj->data.net->bitrate);
			}
			OBJ(wireless_ap) {
				snprintf(p, p_max_size, "%s", obj->data.net->ap);
			}
			OBJ(wireless_link_qual) {
				spaced_print(p, p_max_size, "%d", 4,
						obj->data.net->link_qual);
			}
			OBJ(wireless_link_qual_max) {
				spaced_print(p, p_max_size, "%d", 4,
						obj->data.net->link_qual_max);
			}
			OBJ(wireless_link_qual_perc) {
				if (obj->data.net->link_qual_max > 0) {
					spaced_print(p, p_max_size, "%.0f", 5,
							(double) obj->data.net->link_qual /
							obj->data.net->link_qual_max * 100);
				} else {
					spaced_print(p, p_max_size, "unk", 5);
				}
			}
			OBJ(wireless_link_bar) {
#ifdef X11
				if(output_methods & TO_X) {
					new_bar(p, obj->a, obj->b, ((double) obj->data.net->link_qual /
						obj->data.net->link_qual_max) * 255.0);
				}else{
#endif /* X11 */
					if(!obj->a) obj->a = DEFAULT_BAR_WIDTH_NO_X;
					new_bar_in_shell(p, p_max_size, ((double) obj->data.net->link_qual /
						obj->data.net->link_qual_max) * 100.0, obj->a);
#ifdef X11
				}
#endif /* X11 */
			}
#endif /* HAVE_IWLIB */

#endif /* __linux__ */

#ifndef __OpenBSD__
			OBJ(adt746xcpu) {
				get_adt746x_cpu(p, p_max_size);
			}
			OBJ(adt746xfan) {
				get_adt746x_fan(p, p_max_size);
			}
			OBJ(acpifan) {
				get_acpi_fan(p, p_max_size);
			}
			OBJ(acpiacadapter) {
				get_acpi_ac_adapter(p, p_max_size);
			}
			OBJ(battery) {
				get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_STATUS);
			}
			OBJ(battery_time) {
				get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_TIME);
			}
			OBJ(battery_percent) {
				percent_print(p, p_max_size, get_battery_perct(obj->data.s));
			}
			OBJ(battery_bar) {
#ifdef X11
				if(output_methods & TO_X) {
					new_bar(p, obj->a, obj->b, get_battery_perct_bar(obj->data.s));
				}else{
#endif /* X11 */
					if(!obj->a) obj->a = DEFAULT_BAR_WIDTH_NO_X;
					new_bar_in_shell(p, p_max_size, get_battery_perct_bar(obj->data.s) / 2.55, obj->a);
#ifdef X11
				}
#endif /* X11 */
			}
			OBJ(battery_short) {
				get_battery_short_status(p, p_max_size, obj->data.s);
			}
#endif /* __OpenBSD__ */

			OBJ(buffers) {
				human_readable(cur->buffers * 1024, p, 255);
			}
			OBJ(cached) {
				human_readable(cur->cached * 1024, p, 255);
			}
			OBJ(cpu) {
				if (obj->data.cpu_index > info.cpu_count) {
					NORM_ERR("obj->data.cpu_index %i info.cpu_count %i",
							obj->data.cpu_index, info.cpu_count);
					CRIT_ERR(NULL, NULL, "attempting to use more CPUs than you have!");
				}
				percent_print(p, p_max_size,
				              round_to_int(cur->cpu_usage[obj->data.cpu_index] * 100.0));
			}
#ifdef X11
			OBJ(cpugauge)
				new_gauge(p, obj->a, obj->b,
						round_to_int(cur->cpu_usage[obj->data.cpu_index] * 255.0));
#endif /* X11 */
			OBJ(cpubar) {
#ifdef X11
				if(output_methods & TO_X) {
					new_bar(p, obj->a, obj->b,
						round_to_int(cur->cpu_usage[obj->data.cpu_index] * 255.0));
				}else{
#endif /* X11 */
					if(!obj->a) obj->a = DEFAULT_BAR_WIDTH_NO_X;
					new_bar_in_shell(p, p_max_size, round_to_int(cur->cpu_usage[obj->data.cpu_index] * 100), obj->a);
#ifdef X11
				}
#endif /* X11 */
			}
#ifdef X11
			OBJ(cpugraph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
						round_to_int(cur->cpu_usage[obj->data.cpu_index] * 100),
						100, 1, obj->char_a, obj->char_b);
			}
			OBJ(loadgraph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d, cur->loadavg[0],
						obj->e, 1, obj->char_a, obj->char_b);
			}
#endif /* X11 */
			OBJ(color) {
				new_fg(p, obj->data.l);
			}
#ifdef X11
			OBJ(color0) {
				new_fg(p, color0);
			}
			OBJ(color1) {
				new_fg(p, color1);
			}
			OBJ(color2) {
				new_fg(p, color2);
			}
			OBJ(color3) {
				new_fg(p, color3);
			}
			OBJ(color4) {
				new_fg(p, color4);
			}
			OBJ(color5) {
				new_fg(p, color5);
			}
			OBJ(color6) {
				new_fg(p, color6);
			}
			OBJ(color7) {
				new_fg(p, color7);
			}
			OBJ(color8) {
				new_fg(p, color8);
			}
			OBJ(color9) {
				new_fg(p, color9);
			}
#endif /* X11 */
			OBJ(conky_version) {
				snprintf(p, p_max_size, "%s", VERSION);
			}
			OBJ(conky_build_date) {
				snprintf(p, p_max_size, "%s", BUILD_DATE);
			}
			OBJ(conky_build_arch) {
				snprintf(p, p_max_size, "%s", BUILD_ARCH);
			}
#if defined(__linux__)
			OBJ(disk_protect) {
				snprintf(p, p_max_size, "%s",
						get_disk_protect_queue(obj->data.s));
			}
			OBJ(i8k_version) {
				snprintf(p, p_max_size, "%s", i8k.version);
			}
			OBJ(i8k_bios) {
				snprintf(p, p_max_size, "%s", i8k.bios);
			}
			OBJ(i8k_serial) {
				snprintf(p, p_max_size, "%s", i8k.serial);
			}
			OBJ(i8k_cpu_temp) {
				int cpu_temp;

				sscanf(i8k.cpu_temp, "%d", &cpu_temp);
				temp_print(p, p_max_size, (double)cpu_temp, TEMP_CELSIUS);
			}
			OBJ(i8k_left_fan_status) {
				int left_fan_status;

				sscanf(i8k.left_fan_status, "%d", &left_fan_status);
				if (left_fan_status == 0) {
					snprintf(p, p_max_size, "off");
				}
				if (left_fan_status == 1) {
					snprintf(p, p_max_size, "low");
				}
				if (left_fan_status == 2) {
					snprintf(p, p_max_size, "high");
				}
			}
			OBJ(i8k_right_fan_status) {
				int right_fan_status;

				sscanf(i8k.right_fan_status, "%d", &right_fan_status);
				if (right_fan_status == 0) {
					snprintf(p, p_max_size, "off");
				}
				if (right_fan_status == 1) {
					snprintf(p, p_max_size, "low");
				}
				if (right_fan_status == 2) {
					snprintf(p, p_max_size, "high");
				}
			}
			OBJ(i8k_left_fan_rpm) {
				snprintf(p, p_max_size, "%s", i8k.left_fan_rpm);
			}
			OBJ(i8k_right_fan_rpm) {
				snprintf(p, p_max_size, "%s", i8k.right_fan_rpm);
			}
			OBJ(i8k_ac_status) {
				int ac_status;

				sscanf(i8k.ac_status, "%d", &ac_status);
				if (ac_status == -1) {
					snprintf(p, p_max_size, "disabled (read i8k docs)");
				}
				if (ac_status == 0) {
					snprintf(p, p_max_size, "off");
				}
				if (ac_status == 1) {
					snprintf(p, p_max_size, "on");
				}
			}
			OBJ(i8k_buttons_status) {
				snprintf(p, p_max_size, "%s", i8k.buttons_status);
			}
#if defined(IBM)
			OBJ(ibm_fan) {
				get_ibm_acpi_fan(p, p_max_size);
			}
			OBJ(ibm_temps) {
				get_ibm_acpi_temps();
				temp_print(p, p_max_size,
				           ibm_acpi.temps[obj->data.sensor], TEMP_CELSIUS);
			}
			OBJ(ibm_volume) {
				get_ibm_acpi_volume(p, p_max_size);
			}
			OBJ(ibm_brightness) {
				get_ibm_acpi_brightness(p, p_max_size);
			}
#endif /* IBM */
			/* information from sony_laptop kernel module
			 * /sys/devices/platform/sony-laptop */
			OBJ(sony_fanspeed) {
				get_sony_fanspeed(p, p_max_size);
			}
			OBJ(if_gw) {
				if (!cur->gw_info.count) {
					DO_JUMP;
				}
			}
			OBJ(gw_iface) {
				snprintf(p, p_max_size, "%s", cur->gw_info.iface);
			}
			OBJ(gw_ip) {
				snprintf(p, p_max_size, "%s", cur->gw_info.ip);
			}
			OBJ(laptop_mode) {
				snprintf(p, p_max_size, "%d", get_laptop_mode());
			}
			OBJ(pb_battery) {
				get_powerbook_batt_info(p, p_max_size, obj->data.i);
			}
#endif /* __linux__ */
#if (defined(__FreeBSD__) || defined(__linux__))
			OBJ(if_up) {
				if ((obj->data.ifblock.s)
						&& (!interface_up(obj->data.ifblock.s))) {
					DO_JUMP;
				}
			}
#endif
#ifdef __OpenBSD__
			OBJ(obsd_sensors_temp) {
				obsd_sensors.device = sensor_device;
				update_obsd_sensors();
				temp_print(p, p_max_size,
				           obsd_sensors.temp[obsd_sensors.device][obj->data.sensor],
					   TEMP_CELSIUS);
			}
			OBJ(obsd_sensors_fan) {
				obsd_sensors.device = sensor_device;
				update_obsd_sensors();
				snprintf(p, p_max_size, "%d",
						obsd_sensors.fan[obsd_sensors.device][obj->data.sensor]);
			}
			OBJ(obsd_sensors_volt) {
				obsd_sensors.device = sensor_device;
				update_obsd_sensors();
				snprintf(p, p_max_size, "%.2f",
						obsd_sensors.volt[obsd_sensors.device][obj->data.sensor]);
			}
			OBJ(obsd_vendor) {
				get_obsd_vendor(p, p_max_size);
			}
			OBJ(obsd_product) {
				get_obsd_product(p, p_max_size);
			}
#endif /* __OpenBSD__ */
#ifdef X11
			OBJ(font) {
				new_font(p, obj->data.s);
				need_to_load_fonts = 1;
			}
#endif /* X11 */
			/* TODO: move this correction from kB to kB/s elsewhere
			 * (or get rid of it??) */
			OBJ(diskio) {
				human_readable((obj->data.diskio->current / update_interval) * 1024LL,
						p, p_max_size);
			}
			OBJ(diskio_write) {
				human_readable((obj->data.diskio->current_write / update_interval) * 1024LL,
						p, p_max_size);
			}
			OBJ(diskio_read) {
				human_readable((obj->data.diskio->current_read / update_interval) * 1024LL,
						p, p_max_size);
			}
#ifdef X11
			OBJ(diskiograph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
				          obj->data.diskio->current, obj->e, 1, obj->char_a, obj->char_b);
			}
			OBJ(diskiograph_read) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
				          obj->data.diskio->current_read, obj->e, 1, obj->char_a, obj->char_b);
			}
			OBJ(diskiograph_write) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
				          obj->data.diskio->current_write, obj->e, 1, obj->char_a, obj->char_b);
			}
#endif /* X11 */
			OBJ(downspeed) {
				human_readable(obj->data.net->recv_speed, p, 255);
			}
			OBJ(downspeedf) {
				spaced_print(p, p_max_size, "%.1f", 8,
						obj->data.net->recv_speed / 1024.0);
			}
#ifdef X11
			OBJ(downspeedgraph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
					obj->data.net->recv_speed / 1024.0, obj->e, 1, obj->char_a, obj->char_b);
			}
#endif /* X11 */
			OBJ(else) {
				/* Since we see you, you're if has not jumped.
				 * Do Ninja jump here: without leaving traces.
				 * This is to prevent us from stale jumped flags.
				 */
				obj = obj->data.ifblock.next;
				continue;
			}
			OBJ(endif) {
				/* harmless object, just ignore */
			}
			OBJ(addr) {
				if ((obj->data.net->addr.sa_data[2] & 255) == 0
						&& (obj->data.net->addr.sa_data[3] & 255) == 0
						&& (obj->data.net->addr.sa_data[4] & 255) == 0
						&& (obj->data.net->addr.sa_data[5] & 255) == 0) {
					snprintf(p, p_max_size, "No Address");
				} else {
					snprintf(p, p_max_size, "%u.%u.%u.%u",
						obj->data.net->addr.sa_data[2] & 255,
						obj->data.net->addr.sa_data[3] & 255,
						obj->data.net->addr.sa_data[4] & 255,
						obj->data.net->addr.sa_data[5] & 255);
				}
			}
#if defined(__linux__)
			OBJ(addrs) {
				if (NULL != obj->data.net->addrs && strlen(obj->data.net->addrs) > 2) {
					obj->data.net->addrs[strlen(obj->data.net->addrs) - 2] = 0; /* remove ", " from end of string */
					strcpy(p, obj->data.net->addrs);
				} else {
					strcpy(p, "0.0.0.0");
				}
			}
#endif /* __linux__ */
#if defined(IMLIB2) && defined(X11)
			OBJ(image) {
				/* doesn't actually draw anything, just queues it omp.  the
				 * image will get drawn after the X event loop */
				cimlib_add_image(obj->data.s);
			}
#endif /* IMLIB2 */
			OBJ(eval) {
				evaluate(obj->data.s, p);
			}
			OBJ(exec) {
				read_exec(obj->data.s, p, text_buffer_size);
				remove_deleted_chars(p);
			}
			OBJ(execp) {
				struct information *tmp_info;
				struct text_object subroot;

				read_exec(obj->data.s, p, text_buffer_size);

				tmp_info = malloc(sizeof(struct information));
				memcpy(tmp_info, cur, sizeof(struct information));
				parse_conky_vars(&subroot, p, p, tmp_info);

				free_text_objects(&subroot, 1);
				free(tmp_info);
			}
#ifdef X11
			OBJ(execgauge) {
				double barnum;

				read_exec(obj->data.s, p, text_buffer_size);
				barnum = get_barnum(p); /*using the same function*/

				if (barnum >= 0.0) {
					barnum /= 100;
					new_gauge(p, obj->a, obj->b, round_to_int(barnum * 255.0));
				}
			}
#endif /* X11 */
			OBJ(execbar) {
				double barnum;

				read_exec(obj->data.s, p, text_buffer_size);
				barnum = get_barnum(p);

				if (barnum >= 0.0) {
#ifdef X11
					if(output_methods & TO_X) {
						barnum /= 100;
						new_bar(p, obj->a, obj->b, round_to_int(barnum * 255.0));
					}else{
#endif /* X11 */
						if(!obj->a) obj->a = DEFAULT_BAR_WIDTH_NO_X;
						new_bar_in_shell(p, p_max_size, barnum, obj->a);
#ifdef X11
					}
#endif /* X11 */
				}
			}
#ifdef X11
			OBJ(execgraph) {
				char showaslog = FALSE;
				char tempgrad = FALSE;
				double barnum;
				char *cmd = obj->data.s;

				if (strstr(cmd, " "TEMPGRAD) && strlen(cmd) > strlen(" "TEMPGRAD)) {
					tempgrad = TRUE;
					cmd += strlen(" "TEMPGRAD);
				}
				if (strstr(cmd, " "LOGGRAPH) && strlen(cmd) > strlen(" "LOGGRAPH)) {
					showaslog = TRUE;
					cmd += strlen(" "LOGGRAPH);
				}
				read_exec(cmd, p, text_buffer_size);
				barnum = get_barnum(p);

				if (barnum > 0) {
					new_graph(p, obj->a, obj->b, obj->c, obj->d, round_to_int(barnum),
							100, 1, showaslog, tempgrad);
				}
			}
#endif /* X11 */
			OBJ(execibar) {
				if (current_update_time - obj->data.execi.last_update
						>= obj->data.execi.interval) {
					double barnum;

					read_exec(obj->data.execi.cmd, p, text_buffer_size);
					barnum = get_barnum(p);

					if (barnum >= 0.0) {
						obj->f = barnum;
					}
					obj->data.execi.last_update = current_update_time;
				}
#ifdef X11
				if(output_methods & TO_X) {
					new_bar(p, obj->a, obj->b, round_to_int(obj->f * 2.55));
				} else {
#endif /* X11 */
					if(!obj->a) obj->a = DEFAULT_BAR_WIDTH_NO_X;
					new_bar_in_shell(p, p_max_size, round_to_int(obj->f), obj->a);
#ifdef X11
				}
#endif /* X11 */
			}
#ifdef X11
			OBJ(execigraph) {
				if (current_update_time - obj->data.execi.last_update
						>= obj->data.execi.interval) {
					double barnum;
					char showaslog = FALSE;
					char tempgrad = FALSE;
					char *cmd = obj->data.execi.cmd;

					if (strstr(cmd, " "TEMPGRAD) && strlen(cmd) > strlen(" "TEMPGRAD)) {
						tempgrad = TRUE;
						cmd += strlen(" "TEMPGRAD);
					}
					if (strstr(cmd, " "LOGGRAPH) && strlen(cmd) > strlen(" "LOGGRAPH)) {
						showaslog = TRUE;
						cmd += strlen(" "LOGGRAPH);
					}
					obj->char_a = showaslog;
					obj->char_b = tempgrad;
					read_exec(cmd, p, text_buffer_size);
					barnum = get_barnum(p);

					if (barnum >= 0.0) {
						obj->f = barnum;
					}
					obj->data.execi.last_update = current_update_time;
				}
				new_graph(p, obj->a, obj->b, obj->c, obj->d, (int) (obj->f), 100, 1, obj->char_a, obj->char_b);
			}
			OBJ(execigauge) {
				if (current_update_time - obj->data.execi.last_update
						>= obj->data.execi.interval) {
					double barnum;

					read_exec(obj->data.execi.cmd, p, text_buffer_size);
					barnum = get_barnum(p);

					if (barnum >= 0.0) {
						obj->f = 255 * barnum / 100.0;
					}
					obj->data.execi.last_update = current_update_time;
				}
				new_gauge(p, obj->a, obj->b, round_to_int(obj->f));
			}
#endif /* X11 */
			OBJ(execi) {
				if (current_update_time - obj->data.execi.last_update
						>= obj->data.execi.interval
						&& obj->data.execi.interval != 0) {
					read_exec(obj->data.execi.cmd, obj->data.execi.buffer,
						text_buffer_size);
					obj->data.execi.last_update = current_update_time;
				}
				snprintf(p, text_buffer_size, "%s", obj->data.execi.buffer);
			}
			OBJ(execpi) {
				struct text_object subroot;
				struct information *tmp_info =
					malloc(sizeof(struct information));
				memcpy(tmp_info, cur, sizeof(struct information));

				if (current_update_time - obj->data.execi.last_update
						< obj->data.execi.interval
						|| obj->data.execi.interval == 0) {
					parse_conky_vars(&subroot, obj->data.execi.buffer, p, tmp_info);
				} else {
					char *output = obj->data.execi.buffer;
					FILE *fp = pid_popen(obj->data.execi.cmd, "r", &childpid);
					int length = fread(output, 1, text_buffer_size, fp);

					pclose(fp);

					output[length] = '\0';
					if (length > 0 && output[length - 1] == '\n') {
						output[length - 1] = '\0';
					}

					parse_conky_vars(&subroot, obj->data.execi.buffer, p, tmp_info);
					obj->data.execi.last_update = current_update_time;
				}
				free_text_objects(&subroot, 1);
				free(tmp_info);
			}
			OBJ(texeci) {
				if (!obj->data.texeci.p_timed_thread) {
					obj->data.texeci.p_timed_thread =
						timed_thread_create(&threaded_exec,
						(void *) obj, obj->data.texeci.interval * 1000000);
					if (!obj->data.texeci.p_timed_thread) {
						NORM_ERR("Error creating texeci timed thread");
					}
					/*
					 * note that we don't register this thread with the
					 * timed_thread list, because we destroy it manually
					 */
					if (timed_thread_run(obj->data.texeci.p_timed_thread)) {
						NORM_ERR("Error running texeci timed thread");
					}
				} else {
					timed_thread_lock(obj->data.texeci.p_timed_thread);
					snprintf(p, text_buffer_size, "%s", obj->data.texeci.buffer);
					timed_thread_unlock(obj->data.texeci.p_timed_thread);
				}
			}
			OBJ(imap_unseen) {
				struct mail_s *mail = ensure_mail_thread(obj, imap_thread, "imap");

				if (mail && mail->p_timed_thread) {
					timed_thread_lock(mail->p_timed_thread);
					snprintf(p, p_max_size, "%lu", mail->unseen);
					timed_thread_unlock(mail->p_timed_thread);
				}
			}
			OBJ(imap_messages) {
				struct mail_s *mail = ensure_mail_thread(obj, imap_thread, "imap");

				if (mail && mail->p_timed_thread) {
					timed_thread_lock(mail->p_timed_thread);
					snprintf(p, p_max_size, "%lu", mail->messages);
					timed_thread_unlock(mail->p_timed_thread);
				}
			}
			OBJ(pop3_unseen) {
				struct mail_s *mail = ensure_mail_thread(obj, pop3_thread, "pop3");

				if (mail && mail->p_timed_thread) {
					timed_thread_lock(mail->p_timed_thread);
					snprintf(p, p_max_size, "%lu", mail->unseen);
					timed_thread_unlock(mail->p_timed_thread);
				}
			}
			OBJ(pop3_used) {
				struct mail_s *mail = ensure_mail_thread(obj, pop3_thread, "pop3");

				if (mail && mail->p_timed_thread) {
					timed_thread_lock(mail->p_timed_thread);
					snprintf(p, p_max_size, "%.1f",
						mail->used / 1024.0 / 1024.0);
					timed_thread_unlock(mail->p_timed_thread);
				}
			}
			OBJ(fs_bar) {
				if (obj->data.fs != NULL) {
					if (obj->data.fs->size == 0) {
#ifdef X11
						if(output_methods & TO_X) {
							new_bar(p, obj->data.fsbar.w, obj->data.fsbar.h, 255);
						}else{
#endif /* X11 */
							if(!obj->data.fsbar.w) obj->data.fsbar.w = DEFAULT_BAR_WIDTH_NO_X;
							new_bar_in_shell(p, p_max_size, 100, obj->data.fsbar.w);
#ifdef X11
						}
#endif /* X11 */
					} else {
#ifdef X11
						if(output_methods & TO_X) {
							new_bar(p, obj->data.fsbar.w, obj->data.fsbar.h,
								(int) (255 - obj->data.fsbar.fs->avail * 255 /
								obj->data.fs->size));
						}else{
#endif /* X11 */
							if(!obj->data.fsbar.w) obj->data.fsbar.w = DEFAULT_BAR_WIDTH_NO_X;
							new_bar_in_shell(p, p_max_size,
								(int) (100 - obj->data.fsbar.fs->avail * 100 / obj->data.fs->size), obj->data.fsbar.w);
#ifdef X11
						}
#endif /* X11 */
					}
				}
			}
			OBJ(fs_free) {
				if (obj->data.fs != NULL) {
					human_readable(obj->data.fs->avail, p, 255);
				}
			}
			OBJ(fs_free_perc) {
				if (obj->data.fs != NULL) {
					int val = 0;

					if (obj->data.fs->size) {
						val = obj->data.fs->avail * 100 / obj->data.fs->size;
					}

					percent_print(p, p_max_size, val);
				}
			}
			OBJ(fs_size) {
				if (obj->data.fs != NULL) {
					human_readable(obj->data.fs->size, p, 255);
				}
			}
			OBJ(fs_type) {
				if (obj->data.fs != NULL)
					snprintf(p, p_max_size, "%s", obj->data.fs->type);
			}
			OBJ(fs_used) {
				if (obj->data.fs != NULL) {
					human_readable(obj->data.fs->size - obj->data.fs->free, p,
							255);
				}
			}
			OBJ(fs_bar_free) {
				if (obj->data.fs != NULL) {
					if (obj->data.fs->size == 0) {
#ifdef X11
						if(output_methods & TO_X) {
							new_bar(p, obj->data.fsbar.w, obj->data.fsbar.h, 255);
						}else{
#endif /* X11 */
							if(!obj->data.fsbar.w) obj->data.fsbar.w = DEFAULT_BAR_WIDTH_NO_X;
							new_bar_in_shell(p, p_max_size, 100, obj->data.fsbar.w);
#ifdef X11
						}
#endif /* X11 */
					} else {
#ifdef X11
						if(output_methods & TO_X) {
							new_bar(p, obj->data.fsbar.w, obj->data.fsbar.h,
								(int) (obj->data.fsbar.fs->avail * 255 /
								obj->data.fs->size));
						}else{
#endif /* X11 */
							if(!obj->data.fsbar.w) obj->data.fsbar.w = DEFAULT_BAR_WIDTH_NO_X;
							new_bar_in_shell(p, p_max_size,
								(int) (obj->data.fsbar.fs->avail * 100 / obj->data.fs->size), obj->data.fsbar.w);
#ifdef X11
						}
#endif /* X11 */
					}
				}
			}
			OBJ(fs_used_perc) {
				if (obj->data.fs != NULL) {
					int val = 0;

					if (obj->data.fs->size) {
						val = obj->data.fs->free
								* 100 /
							obj->data.fs->size;
					}

					percent_print(p, p_max_size, 100 - val);
				}
			}
			OBJ(loadavg) {
				float *v = info.loadavg;

				if (obj->data.loadavg[2]) {
					snprintf(p, p_max_size, "%.2f %.2f %.2f",
						v[obj->data.loadavg[0] - 1],
						v[obj->data.loadavg[1] - 1],
						v[obj->data.loadavg[2] - 1]);
				} else if (obj->data.loadavg[1]) {
					snprintf(p, p_max_size, "%.2f %.2f",
						v[obj->data.loadavg[0] - 1],
						v[obj->data.loadavg[1] - 1]);
				} else if (obj->data.loadavg[0]) {
					snprintf(p, p_max_size, "%.2f",
						v[obj->data.loadavg[0] - 1]);
				}
			}
			OBJ(goto) {
				new_goto(p, obj->data.i);
			}
			OBJ(tab) {
				new_tab(p, obj->data.pair.a, obj->data.pair.b);
			}
#ifdef X11
			OBJ(hr) {
				new_hr(p, obj->data.i);
			}
#endif
			OBJ(nameserver) {
				if (cur->nameserver_info.nscount > obj->data.i)
					snprintf(p, p_max_size, "%s",
							cur->nameserver_info.ns_list[obj->data.i]);
			}
#ifdef EVE
			OBJ(eve) {
				char *skill = eve(obj->data.eve.userid, obj->data.eve.apikey, obj->data.eve.charid);
				snprintf(p, p_max_size, "%s", skill);
			}
#endif
#ifdef HAVE_CURL
			OBJ(curl) {
				if (obj->data.curl.uri != NULL) {
					ccurl_process_info(p, p_max_size, obj->data.curl.uri, obj->data.curl.interval);
				} else {
					NORM_ERR("error processing Curl data");
				}
			}
#endif
#ifdef RSS
			OBJ(rss) {
				if (obj->data.rss.uri != NULL) {
					rss_process_info(p, p_max_size, obj->data.rss.uri, obj->data.rss.action, obj->data.rss.act_par, obj->data.rss.interval, obj->data.rss.nrspaces);
				} else {
					NORM_ERR("error processing RSS data");
				}
			}
#endif
#ifdef WEATHER
			OBJ(weather) {
				if (obj->data.weather.uri != NULL) {
					weather_process_info(p, p_max_size, obj->data.weather.uri, obj->data.weather.data_type, obj->data.weather.interval);
				} else {
					NORM_ERR("error processing weather data, check that you have a valid XOAP key if using XOAP.");
				}
			}
#endif
#ifdef XOAP
			OBJ(weather_forecast) {
				if (obj->data.weather_forecast.uri != NULL) {
					weather_forecast_process_info(p, p_max_size, obj->data.weather_forecast.uri, obj->data.weather_forecast.day, obj->data.weather_forecast.data_type, obj->data.weather_forecast.interval);
				} else {
					NORM_ERR("error processing weather forecast data, check that you have a valid XOAP key if using XOAP.");
				}
			}
#endif
#ifdef HAVE_LUA
			OBJ(lua) {
				char *str = llua_getstring(obj->data.s);
				if (str) {
					snprintf(p, p_max_size, "%s", str);
					free(str);
				}
			}
			OBJ(lua_parse) {
				char *str = llua_getstring(obj->data.s);
				if (str) {
					evaluate(str, p);
					free(str);
				}
			}
			OBJ(lua_bar) {
				double per;
				if (llua_getnumber(obj->data.s, &per)) {
#ifdef X11
					if(output_methods & TO_X) {
						new_bar(p, obj->a, obj->b, (per/100.0 * 255));
					} else {
#endif /* X11 */
						if(!obj->a) obj->a = DEFAULT_BAR_WIDTH_NO_X;
						new_bar_in_shell(p, p_max_size, per, obj->a);
#ifdef X11
					}
#endif /* X11 */
				}
			}
#ifdef X11
			OBJ(lua_graph) {
				double per;
				if (llua_getnumber(obj->data.s, &per)) {
					new_graph(p, obj->a, obj->b, obj->c, obj->d,
							per, obj->e, 1, obj->char_a, obj->char_b);
				}
			}
			OBJ(lua_gauge) {
				double per;
				if (llua_getnumber(obj->data.s, &per)) {
					new_gauge(p, obj->a, obj->b, (per/100.0 * 255));
				}
			}
#endif /* X11 */
#endif /* HAVE_LUA */
#ifdef HDDTEMP
			OBJ(hddtemp) {
				char *endptr, unit;
				long val;
				if (obj->data.hddtemp.update_time < current_update_time - 30) {
					if (obj->data.hddtemp.temp)
						free(obj->data.hddtemp.temp);
					obj->data.hddtemp.temp = get_hddtemp_info(obj->data.hddtemp.dev,
							obj->data.hddtemp.addr, obj->data.hddtemp.port);
					obj->data.hddtemp.update_time = current_update_time;
				}
				if (!obj->data.hddtemp.temp) {
					snprintf(p, p_max_size, "N/A");
				} else {
					val = strtol(obj->data.hddtemp.temp + 1, &endptr, 10);
					unit = obj->data.hddtemp.temp[0];

					if (*endptr != '\0')
						snprintf(p, p_max_size, "N/A");
					else if (unit == 'C')
						temp_print(p, p_max_size, (double)val, TEMP_CELSIUS);
					else if (unit == 'F')
						temp_print(p, p_max_size, (double)val, TEMP_FAHRENHEIT);
					else
						snprintf(p, p_max_size, "N/A");
				}
			}
#endif
			OBJ(offset) {
				new_offset(p, obj->data.i);
			}
			OBJ(voffset) {
				new_voffset(p, obj->data.i);
			}
#ifdef __linux__
			OBJ(i2c) {
				double r;

				r = get_sysfs_info(&obj->data.sysfs.fd, obj->data.sysfs.arg,
					obj->data.sysfs.devtype, obj->data.sysfs.type);

				r = r * obj->data.sysfs.factor + obj->data.sysfs.offset;

				if (!strncmp(obj->data.sysfs.type, "temp", 4)) {
					temp_print(p, p_max_size, r, TEMP_CELSIUS);
				} else if (r >= 100.0 || r == 0) {
					snprintf(p, p_max_size, "%d", (int) r);
				} else {
					snprintf(p, p_max_size, "%.1f", r);
				}
			}
			OBJ(platform) {
				double r;

				r = get_sysfs_info(&obj->data.sysfs.fd, obj->data.sysfs.arg,
					obj->data.sysfs.devtype, obj->data.sysfs.type);

				r = r * obj->data.sysfs.factor + obj->data.sysfs.offset;

				if (!strncmp(obj->data.sysfs.type, "temp", 4)) {
					temp_print(p, p_max_size, r, TEMP_CELSIUS);
				} else if (r >= 100.0 || r == 0) {
					snprintf(p, p_max_size, "%d", (int) r);
				} else {
					snprintf(p, p_max_size, "%.1f", r);
				}
			}
			OBJ(hwmon) {
				double r;

				r = get_sysfs_info(&obj->data.sysfs.fd, obj->data.sysfs.arg,
					obj->data.sysfs.devtype, obj->data.sysfs.type);

				r = r * obj->data.sysfs.factor + obj->data.sysfs.offset;

				if (!strncmp(obj->data.sysfs.type, "temp", 4)) {
					temp_print(p, p_max_size, r, TEMP_CELSIUS);
				} else if (r >= 100.0 || r == 0) {
					snprintf(p, p_max_size, "%d", (int) r);
				} else {
					snprintf(p, p_max_size, "%.1f", r);
				}
			}
#endif /* __linux__ */
			OBJ(alignr) {
				new_alignr(p, obj->data.i);
			}
			OBJ(alignc) {
				new_alignc(p, obj->data.i);
			}
			OBJ(if_empty) {
				char buf[max_user_text];
				struct information *tmp_info =
					malloc(sizeof(struct information));
				memcpy(tmp_info, cur, sizeof(struct information));
				generate_text_internal(buf, max_user_text,
				                       *obj->sub, tmp_info);

				if (strlen(buf) != 0) {
					DO_JUMP;
				}
				free(tmp_info);
			}
			OBJ(if_match) {
				char expression[max_user_text];
				int val;
				struct information *tmp_info;

				tmp_info = malloc(sizeof(struct information));
				memcpy(tmp_info, cur, sizeof(struct information));
				generate_text_internal(expression, max_user_text,
				                       *obj->sub, tmp_info);
				DBGP("parsed arg into '%s'", expression);

				val = compare(expression);
				if (val == -2) {
					NORM_ERR("compare failed for expression '%s'",
							expression);
				} else if (!val) {
					DO_JUMP;
				}
				free(tmp_info);
			}
			OBJ(if_existing) {
				if (obj->data.ifblock.str
				    && !check_contains(obj->data.ifblock.s,
				                       obj->data.ifblock.str)) {
					DO_JUMP;
				} else if (obj->data.ifblock.s
				           && access(obj->data.ifblock.s, F_OK)) {
					DO_JUMP;
				}
			}
			OBJ(if_mounted) {
				if ((obj->data.ifblock.s)
						&& (!check_mount(obj->data.ifblock.s))) {
					DO_JUMP;
				}
			}
			OBJ(if_running) {
#ifdef __linux__
				if (!get_process_by_name(obj->data.ifblock.s)) {
#else
				if ((obj->data.ifblock.s) && system(obj->data.ifblock.s)) {
#endif
					DO_JUMP;
				}
			}
#if defined(__linux__)
			OBJ(ioscheduler) {
				snprintf(p, p_max_size, "%s", get_ioscheduler(obj->data.s));
			}
#endif
			OBJ(kernel) {
				snprintf(p, p_max_size, "%s", cur->uname_s.release);
			}
			OBJ(machine) {
				snprintf(p, p_max_size, "%s", cur->uname_s.machine);
			}

			/* memory stuff */
			OBJ(mem) {
				human_readable(cur->mem * 1024, p, 255);
			}
			OBJ(memeasyfree) {
				human_readable(cur->memeasyfree * 1024, p, 255);
			}
			OBJ(memfree) {
				human_readable(cur->memfree * 1024, p, 255);
			}
			OBJ(memmax) {
				human_readable(cur->memmax * 1024, p, 255);
			}
			OBJ(memperc) {
				if (cur->memmax)
					percent_print(p, p_max_size, cur->mem * 100 / cur->memmax);
			}
#ifdef X11
			OBJ(memgauge){
				new_gauge(p, obj->data.pair.a, obj->data.pair.b,
					cur->memmax ? (cur->mem * 255) / (cur->memmax) : 0);
			}
#endif /* X11 */
			OBJ(membar) {
#ifdef X11
				if(output_methods & TO_X) {
					new_bar(p, obj->data.pair.a, obj->data.pair.b,
						cur->memmax ? (cur->mem * 255) / (cur->memmax) : 0);
				}else{
#endif /* X11 */
					if(!obj->data.pair.a) obj->data.pair.a = DEFAULT_BAR_WIDTH_NO_X;
					new_bar_in_shell(p, p_max_size, cur->memmax ? (cur->mem * 100) / (cur->memmax) : 0, obj->data.pair.a);
#ifdef X11
				}
#endif /* X11 */
			}
#ifdef X11
			OBJ(memgraph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
					cur->memmax ? (cur->mem * 100.0) / (cur->memmax) : 0.0,
					100, 1, obj->char_a, obj->char_b);
			}
#endif /* X11 */
			/* mixer stuff */
			OBJ(mixer) {
				percent_print(p, p_max_size, mixer_get_avg(obj->data.l));
			}
			OBJ(mixerl) {
				percent_print(p, p_max_size, mixer_get_left(obj->data.l));
			}
			OBJ(mixerr) {
				percent_print(p, p_max_size, mixer_get_right(obj->data.l));
			}
#ifdef X11
			OBJ(mixerbar) {
				new_bar(p, obj->data.mixerbar.w, obj->data.mixerbar.h,
					mixer_to_255(obj->data.mixerbar.l,mixer_get_avg(obj->data.mixerbar.l)));
			}
			OBJ(mixerlbar) {
				new_bar(p, obj->data.mixerbar.w, obj->data.mixerbar.h,
					mixer_to_255(obj->data.mixerbar.l,mixer_get_left(obj->data.mixerbar.l)));
			}
			OBJ(mixerrbar) {
				new_bar(p, obj->data.mixerbar.w, obj->data.mixerbar.h,
					mixer_to_255(obj->data.mixerbar.l,mixer_get_right(obj->data.mixerbar.l)));
			}
#endif /* X11 */
			OBJ(if_mixer_mute) {
				if (!mixer_is_mute(obj->data.ifblock.i)) {
					DO_JUMP;
				}
			}
#ifdef X11
#define NOT_IN_X "Not running in X"
			OBJ(monitor) {
				if(x_initialised != YES) {
					strncpy(p, NOT_IN_X, p_max_size);
				}else{
					snprintf(p, p_max_size, "%d", cur->x11.monitor.current);
				}
			}
			OBJ(monitor_number) {
				if(x_initialised != YES) {
					strncpy(p, NOT_IN_X, p_max_size);
				}else{
					snprintf(p, p_max_size, "%d", cur->x11.monitor.number);
				}
			}
			OBJ(desktop) {
				if(x_initialised != YES) {
					strncpy(p, NOT_IN_X, p_max_size);
				}else{
					snprintf(p, p_max_size, "%d", cur->x11.desktop.current);
				}
			}
			OBJ(desktop_number) {
				if(x_initialised != YES) {
					strncpy(p, NOT_IN_X, p_max_size);
				}else{
					snprintf(p, p_max_size, "%d", cur->x11.desktop.number);
				}
			}
			OBJ(desktop_name) {
				if(x_initialised != YES) {
					strncpy(p, NOT_IN_X, p_max_size);
				}else if(cur->x11.desktop.name != NULL) {
					strncpy(p, cur->x11.desktop.name, p_max_size);
				}
			}
#endif /* X11 */

			/* mail stuff */
			OBJ(mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d", obj->data.local_mail.mail_count);
			}
			OBJ(new_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.new_mail_count);
			}
			OBJ(seen_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.seen_mail_count);
			}
			OBJ(unseen_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.unseen_mail_count);
			}
			OBJ(flagged_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.flagged_mail_count);
			}
			OBJ(unflagged_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.unflagged_mail_count);
			}
			OBJ(forwarded_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.forwarded_mail_count);
			}
			OBJ(unforwarded_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.unforwarded_mail_count);
			}
			OBJ(replied_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.replied_mail_count);
			}
			OBJ(unreplied_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.unreplied_mail_count);
			}
			OBJ(draft_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.draft_mail_count);
			}
			OBJ(trashed_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.trashed_mail_count);
			}
			OBJ(mboxscan) {
				mbox_scan(obj->data.mboxscan.args, obj->data.mboxscan.output,
					text_buffer_size);
				snprintf(p, p_max_size, "%s", obj->data.mboxscan.output);
			}
			OBJ(nodename) {
				snprintf(p, p_max_size, "%s", cur->uname_s.nodename);
			}
			OBJ(outlinecolor) {
				new_outline(p, obj->data.l);
			}
			OBJ(processes) {
				spaced_print(p, p_max_size, "%hu", 4, cur->procs);
			}
			OBJ(running_processes) {
				spaced_print(p, p_max_size, "%hu", 4, cur->run_procs);
			}
			OBJ(text) {
				snprintf(p, p_max_size, "%s", obj->data.s);
			}
#ifdef X11
			OBJ(shadecolor) {
				new_bg(p, obj->data.l);
			}
			OBJ(stippled_hr) {
				new_stippled_hr(p, obj->data.pair.a, obj->data.pair.b);
			}
#endif /* X11 */
			OBJ(swap) {
				human_readable(cur->swap * 1024, p, 255);
			}
			OBJ(swapfree) {
				human_readable(cur->swapfree * 1024, p, 255);
			}
			OBJ(swapmax) {
				human_readable(cur->swapmax * 1024, p, 255);
			}
			OBJ(swapperc) {
				if (cur->swapmax == 0) {
					strncpy(p, "No swap", p_max_size);
				} else {
					percent_print(p, p_max_size, cur->swap * 100 / cur->swapmax);
				}
			}
			OBJ(swapbar) {
#ifdef X11
				if(output_methods & TO_X) {
					new_bar(p, obj->data.pair.a, obj->data.pair.b,
						cur->swapmax ? (cur->swap * 255) / (cur->swapmax) : 0);
				}else{
#endif /* X11 */
					if(!obj->data.pair.a) obj->data.pair.a = DEFAULT_BAR_WIDTH_NO_X;
					new_bar_in_shell(p, p_max_size, cur->swapmax ? (cur->swap * 100) / (cur->swapmax) : 0, obj->data.pair.a);
#ifdef X11
				}
#endif /* X11 */
			}
			OBJ(sysname) {
				snprintf(p, p_max_size, "%s", cur->uname_s.sysname);
			}
			OBJ(time) {
				time_t t = time(NULL);
				struct tm *tm = localtime(&t);

				setlocale(LC_TIME, "");
				strftime(p, p_max_size, obj->data.s, tm);
			}
			OBJ(utime) {
				time_t t = time(NULL);
				struct tm *tm = gmtime(&t);

				strftime(p, p_max_size, obj->data.s, tm);
			}
			OBJ(tztime) {
				char *oldTZ = NULL;
				time_t t;
				struct tm *tm;

				if (obj->data.tztime.tz) {
					oldTZ = getenv("TZ");
					setenv("TZ", obj->data.tztime.tz, 1);
					tzset();
				}
				t = time(NULL);
				tm = localtime(&t);

				setlocale(LC_TIME, "");
				strftime(p, p_max_size, obj->data.tztime.fmt, tm);
				if (oldTZ) {
					setenv("TZ", oldTZ, 1);
					tzset();
				} else {
					unsetenv("TZ");
				}
				// Needless to free oldTZ since getenv gives ptr to static data
			}
			OBJ(totaldown) {
				human_readable(obj->data.net->recv, p, 255);
			}
			OBJ(totalup) {
				human_readable(obj->data.net->trans, p, 255);
			}
			OBJ(updates) {
				snprintf(p, p_max_size, "%d", total_updates);
			}
			OBJ(if_updatenr) {
				if(total_updates % updatereset != obj->data.ifblock.i - 1) {
					DO_JUMP;
				}
			}
			OBJ(upspeed) {
				human_readable(obj->data.net->trans_speed, p, 255);
			}
			OBJ(upspeedf) {
				spaced_print(p, p_max_size, "%.1f", 8,
					obj->data.net->trans_speed / 1024.0);
			}
#ifdef X11
			OBJ(upspeedgraph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
					obj->data.net->trans_speed / 1024.0, obj->e, 1, obj->char_a, obj->char_b);
			}
#endif /* X11 */
			OBJ(uptime_short) {
				format_seconds_short(p, p_max_size, (int) cur->uptime);
			}
			OBJ(uptime) {
				format_seconds(p, p_max_size, (int) cur->uptime);
			}
			OBJ(user_names) {
				snprintf(p, p_max_size, "%s", cur->users.names);
			}
			OBJ(user_terms) {
				snprintf(p, p_max_size, "%s", cur->users.terms);
			}
			OBJ(user_times) {
				snprintf(p, p_max_size, "%s", cur->users.times);
			}
			OBJ(user_number) {
				snprintf(p, p_max_size, "%d", cur->users.number);
			}
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
		|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
			OBJ(apm_adapter) {
				char *msg;

				msg = get_apm_adapter();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
			OBJ(apm_battery_life) {
				char *msg;

				msg = get_apm_battery_life();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
			OBJ(apm_battery_time) {
				char *msg;

				msg = get_apm_battery_time();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
#endif /* __FreeBSD__ __OpenBSD__ */

#ifdef MPD
#define mpd_printf(fmt, val) \
	snprintf(p, p_max_size, fmt, mpd_get_info()->val)
#define mpd_sprintf(val) { \
	if (!obj->data.i || obj->data.i > p_max_size) \
		mpd_printf("%s", val); \
	else \
		snprintf(p, obj->data.i, "%s", mpd_get_info()->val); \
}
			OBJ(mpd_title)
				mpd_sprintf(title);
			OBJ(mpd_artist)
				mpd_sprintf(artist);
			OBJ(mpd_album)
				mpd_sprintf(album);
			OBJ(mpd_random)
				mpd_printf("%s", random);
			OBJ(mpd_repeat)
				mpd_printf("%s", repeat);
			OBJ(mpd_track)
				mpd_sprintf(track);
			OBJ(mpd_name)
				mpd_sprintf(name);
			OBJ(mpd_file)
				mpd_sprintf(file);
			OBJ(mpd_vol)
				mpd_printf("%d", volume);
			OBJ(mpd_bitrate)
				mpd_printf("%d", bitrate);
			OBJ(mpd_status)
				mpd_printf("%s", status);
			OBJ(mpd_elapsed) {
				format_media_player_time(p, p_max_size, mpd_get_info()->elapsed);
			}
			OBJ(mpd_length) {
				format_media_player_time(p, p_max_size, mpd_get_info()->length);
			}
			OBJ(mpd_percent) {
				percent_print(p, p_max_size, (int)(mpd_get_info()->progress * 100));
			}
			OBJ(mpd_bar) {
#ifdef X11
				if(output_methods & TO_X) {
					new_bar(p, obj->data.pair.a, obj->data.pair.b,
						(int) (mpd_get_info()->progress * 255.0f));
				} else {
#endif /* X11 */
					if(!obj->data.pair.a) obj->data.pair.a = DEFAULT_BAR_WIDTH_NO_X;
					new_bar_in_shell(p, p_max_size, (int) (mpd_get_info()->progress * 100.0f), obj->data.pair.a);
#ifdef X11
				}
#endif /* X11 */
			}
			OBJ(mpd_smart) {
				struct mpd_s *mpd = mpd_get_info();
				int len = obj->data.i;
				if (len == 0 || len > p_max_size)
					len = p_max_size;

				memset(p, 0, p_max_size);
				if (mpd->artist && *mpd->artist &&
				    mpd->title && *mpd->title) {
					snprintf(p, len, "%s - %s", mpd->artist,
						mpd->title);
				} else if (mpd->title && *mpd->title) {
					snprintf(p, len, "%s", mpd->title);
				} else if (mpd->artist && *mpd->artist) {
					snprintf(p, len, "%s", mpd->artist);
				} else if (mpd->file && *mpd->file) {
					snprintf(p, len, "%s", mpd->file);
				} else {
					*p = 0;
				}
			}
			OBJ(if_mpd_playing) {
				if (!mpd_get_info()->is_playing) {
					DO_JUMP;
				}
			}
#undef mpd_sprintf
#undef mpd_printf
#endif

#ifdef MOC
#define MOC_PRINT(t, a) \
	snprintf(p, p_max_size, "%s", (moc.t ? moc.t : a))
			OBJ(moc_state) {
				MOC_PRINT(state, "??");
			}
			OBJ(moc_file) {
				MOC_PRINT(file, "no file");
			}
			OBJ(moc_title) {
				MOC_PRINT(title, "no title");
			}
			OBJ(moc_artist) {
				MOC_PRINT(artist, "no artist");
			}
			OBJ(moc_song) {
				MOC_PRINT(song, "no song");
			}
			OBJ(moc_album) {
				MOC_PRINT(album, "no album");
			}
			OBJ(moc_totaltime) {
				MOC_PRINT(totaltime, "0:00");
			}
			OBJ(moc_timeleft) {
				MOC_PRINT(timeleft, "0:00");
			}
			OBJ(moc_curtime) {
				MOC_PRINT(curtime, "0:00");
			}
			OBJ(moc_bitrate) {
				MOC_PRINT(bitrate, "0Kbps");
			}
			OBJ(moc_rate) {
				MOC_PRINT(rate, "0KHz");
			}
#undef MOC_PRINT
#endif /* MOC */
#ifdef XMMS2
			OBJ(xmms2_artist) {
				snprintf(p, p_max_size, "%s", cur->xmms2.artist);
			}
			OBJ(xmms2_album) {
				snprintf(p, p_max_size, "%s", cur->xmms2.album);
			}
			OBJ(xmms2_title) {
				snprintf(p, p_max_size, "%s", cur->xmms2.title);
			}
			OBJ(xmms2_genre) {
				snprintf(p, p_max_size, "%s", cur->xmms2.genre);
			}
			OBJ(xmms2_comment) {
				snprintf(p, p_max_size, "%s", cur->xmms2.comment);
			}
			OBJ(xmms2_url) {
				snprintf(p, p_max_size, "%s", cur->xmms2.url);
			}
			OBJ(xmms2_status) {
				snprintf(p, p_max_size, "%s", cur->xmms2.status);
			}
			OBJ(xmms2_date) {
				snprintf(p, p_max_size, "%s", cur->xmms2.date);
			}
			OBJ(xmms2_tracknr) {
				if (cur->xmms2.tracknr != -1) {
					snprintf(p, p_max_size, "%i", cur->xmms2.tracknr);
				}
			}
			OBJ(xmms2_bitrate) {
				snprintf(p, p_max_size, "%i", cur->xmms2.bitrate);
			}
			OBJ(xmms2_id) {
				snprintf(p, p_max_size, "%u", cur->xmms2.id);
			}
			OBJ(xmms2_size) {
				snprintf(p, p_max_size, "%2.1f", cur->xmms2.size);
			}
			OBJ(xmms2_elapsed) {
				snprintf(p, p_max_size, "%02d:%02d", cur->xmms2.elapsed / 60000,
					(cur->xmms2.elapsed / 1000) % 60);
			}
			OBJ(xmms2_duration) {
				snprintf(p, p_max_size, "%02d:%02d",
					cur->xmms2.duration / 60000,
					(cur->xmms2.duration / 1000) % 60);
			}
			OBJ(xmms2_percent) {
				snprintf(p, p_max_size, "%2.0f", cur->xmms2.progress * 100);
			}
#ifdef X11
			OBJ(xmms2_bar) {
				new_bar(p, obj->data.pair.a, obj->data.pair.b,
					(int) (cur->xmms2.progress * 255.0f));
			}
#endif /* X11 */
			OBJ(xmms2_playlist) {
				snprintf(p, p_max_size, "%s", cur->xmms2.playlist);
			}
			OBJ(xmms2_timesplayed) {
				snprintf(p, p_max_size, "%i", cur->xmms2.timesplayed);
			}
			OBJ(xmms2_smart) {
				if (strlen(cur->xmms2.title) < 2
						&& strlen(cur->xmms2.title) < 2) {
					snprintf(p, p_max_size, "%s", cur->xmms2.url);
				} else {
					snprintf(p, p_max_size, "%s - %s", cur->xmms2.artist,
						cur->xmms2.title);
				}
			}
			OBJ(if_xmms2_connected) {
				if (cur->xmms2.conn_state != 1) {
					DO_JUMP;
				}
			}
#endif /* XMMS */
#ifdef AUDACIOUS
			OBJ(audacious_status) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_STATUS]);
			}
			OBJ(audacious_title) {
				snprintf(p, cur->audacious.max_title_len > 0
					? cur->audacious.max_title_len : p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_TITLE]);
			}
			OBJ(audacious_length) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_LENGTH]);
			}
			OBJ(audacious_length_seconds) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_LENGTH_SECONDS]);
			}
			OBJ(audacious_position) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_POSITION]);
			}
			OBJ(audacious_position_seconds) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_POSITION_SECONDS]);
			}
			OBJ(audacious_bitrate) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_BITRATE]);
			}
			OBJ(audacious_frequency) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_FREQUENCY]);
			}
			OBJ(audacious_channels) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_CHANNELS]);
			}
			OBJ(audacious_filename) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_FILENAME]);
			}
			OBJ(audacious_playlist_length) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_PLAYLIST_LENGTH]);
			}
			OBJ(audacious_playlist_position) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_PLAYLIST_POSITION]);
			}
			OBJ(audacious_main_volume) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_MAIN_VOLUME]);
			}
#ifdef X11
			OBJ(audacious_bar) {
				double progress;

				progress =
					atof(cur->audacious.items[AUDACIOUS_POSITION_SECONDS]) /
					atof(cur->audacious.items[AUDACIOUS_LENGTH_SECONDS]);
				new_bar(p, obj->a, obj->b, (int) (progress * 255.0f));
			}
#endif /* X11 */
#endif /* AUDACIOUS */

#ifdef BMPX
			OBJ(bmpx_title) {
				snprintf(p, p_max_size, "%s", cur->bmpx.title);
			}
			OBJ(bmpx_artist) {
				snprintf(p, p_max_size, "%s", cur->bmpx.artist);
			}
			OBJ(bmpx_album) {
				snprintf(p, p_max_size, "%s", cur->bmpx.album);
			}
			OBJ(bmpx_uri) {
				snprintf(p, p_max_size, "%s", cur->bmpx.uri);
			}
			OBJ(bmpx_track) {
				snprintf(p, p_max_size, "%i", cur->bmpx.track);
			}
			OBJ(bmpx_bitrate) {
				snprintf(p, p_max_size, "%i", cur->bmpx.bitrate);
			}
#endif /* BMPX */
			/* we have four different types of top (top, top_mem,
			 * top_time and top_io). To avoid having almost-same code four
			 * times, we have this special handler. */
			break;
			case OBJ_top:
				parse_top_args("top", obj->data.top.s, obj);
				if (!needed) needed = cur->cpu;
			case OBJ_top_mem:
				parse_top_args("top_mem", obj->data.top.s, obj);
				if (!needed) needed = cur->memu;
			case OBJ_top_time:
				parse_top_args("top_time", obj->data.top.s, obj);
				if (!needed) needed = cur->time;
#ifdef IOSTATS
			case OBJ_top_io:
				parse_top_args("top_io", obj->data.top.s, obj);
				if (!needed) needed = cur->io;
#endif

				if (needed[obj->data.top.num]) {
					char *timeval;

					switch (obj->data.top.type) {
						case TOP_NAME:
							snprintf(p, top_name_width + 1, "%-*s", top_name_width,
									needed[obj->data.top.num]->name);
							break;
						case TOP_CPU:
							snprintf(p, 7, "%6.2f",
									needed[obj->data.top.num]->amount);
							break;
						case TOP_PID:
							snprintf(p, 6, "%5i",
									needed[obj->data.top.num]->pid);
							break;
						case TOP_MEM:
							snprintf(p, 7, "%6.2f",
									needed[obj->data.top.num]->totalmem);
							break;
						case TOP_TIME:
							timeval = format_time(
									needed[obj->data.top.num]->total_cpu_time, 9);
							snprintf(p, 10, "%9s", timeval);
							free(timeval);
							break;
						case TOP_MEM_RES:
							human_readable(needed[obj->data.top.num]->rss,
									p, 255);
							break;
						case TOP_MEM_VSIZE:
							human_readable(needed[obj->data.top.num]->vsize,
									p, 255);
							break;
#ifdef IOSTATS
						case TOP_READ_BYTES:
							human_readable(needed[obj->data.top.num]->read_bytes / update_interval,
									p, 255);
							break;
						case TOP_WRITE_BYTES:
							human_readable(needed[obj->data.top.num]->write_bytes / update_interval,
									p, 255);
							break;
						case TOP_IO_PERC:
							snprintf(p, 7, "%6.2f",
									needed[obj->data.top.num]->io_perc);
							break;
#endif
					}
				}
			OBJ(tail) {
				print_tailhead("tail", obj, p, p_max_size);
			}
			OBJ(head) {
				print_tailhead("head", obj, p, p_max_size);
			}
			OBJ(lines) {
				FILE *fp = open_file(obj->data.s, &obj->a);

				if(fp != NULL) {
/* FIXME: use something more general (see also tail.c, head.c */
#define BUFSZ 0x1000
					char buf[BUFSZ];
					int j, lines;

					lines = 0;
					while(fgets(buf, BUFSZ, fp) != NULL){
						for(j = 0; buf[j] != 0; j++) {
							if(buf[j] == '\n') {
								lines++;
							}
						}
					}
					sprintf(p, "%d", lines);
					fclose(fp);
				} else {
					sprintf(p, "File Unreadable");
				}
			}

			OBJ(words) {
				FILE *fp = open_file(obj->data.s, &obj->a);

				if(fp != NULL) {
					char buf[BUFSZ];
					int j, words;
					char inword = FALSE;

					words = 0;
					while(fgets(buf, BUFSZ, fp) != NULL){
						for(j = 0; buf[j] != 0; j++) {
							if(!isspace(buf[j])) {
								if(inword == FALSE) {
									words++;
									inword = TRUE;
								}
							} else {
								inword = FALSE;
							}
						}
					}
					sprintf(p, "%d", words);
					fclose(fp);
				} else {
					sprintf(p, "File Unreadable");
				}
			}
#ifdef TCP_PORT_MONITOR
			OBJ(tcp_portmon) {
				tcp_portmon_action(p, p_max_size,
				                   &obj->data.tcp_port_monitor);
			}
#endif /* TCP_PORT_MONITOR */

#ifdef HAVE_ICONV
			OBJ(iconv_start) {
				set_iconv_converting(1);
				set_iconv_selected(obj->a);
			}
			OBJ(iconv_stop) {
				set_iconv_converting(0);
				set_iconv_selected(0);
			}
#endif /* HAVE_ICONV */

			OBJ(entropy_avail) {
				snprintf(p, p_max_size, "%d", cur->entropy.entropy_avail);
			}
			OBJ(entropy_perc) {
				percent_print(p, p_max_size,
				              cur->entropy.entropy_avail *
					      100 / cur->entropy.poolsize);
			}
			OBJ(entropy_poolsize) {
				snprintf(p, p_max_size, "%d", cur->entropy.poolsize);
			}
			OBJ(entropy_bar) {
				double entropy_perc;

				entropy_perc = (double) cur->entropy.entropy_avail /
					(double) cur->entropy.poolsize;
#ifdef X11
				if(output_methods & TO_X) {
					new_bar(p, obj->a, obj->b, (int) (entropy_perc * 255.0f));
				} else {
#endif /* X11 */
					if(!obj->a) obj->a = DEFAULT_BAR_WIDTH_NO_X;
					new_bar_in_shell(p, p_max_size, (int) (entropy_perc * 100.0f), obj->a);
#ifdef X11
				}
#endif /* X11 */
			}
#ifdef IBM
			OBJ(smapi) {
				char *s;
				if(obj->data.s) {
					s = smapi_get_val(obj->data.s);
					snprintf(p, p_max_size, "%s", s);
					free(s);
				}
			}
			OBJ(if_smapi_bat_installed) {
				int idx;
				if(obj->data.ifblock.s && sscanf(obj->data.ifblock.s, "%i", &idx) == 1) {
					if(!smapi_bat_installed(idx)) {
						DO_JUMP;
					}
				} else
					NORM_ERR("argument to if_smapi_bat_installed must be an integer");
			}
			OBJ(smapi_bat_perc) {
				int idx, val;
				if(obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
					val = smapi_bat_installed(idx) ?
						smapi_get_bat_int(idx, "remaining_percent") : 0;
					percent_print(p, p_max_size, val);
				} else
					NORM_ERR("argument to smapi_bat_perc must be an integer");
			}
			OBJ(smapi_bat_temp) {
				int idx, val;
				if(obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
					val = smapi_bat_installed(idx) ?
						smapi_get_bat_int(idx, "temperature") : 0;
					/* temperature is in milli degree celsius */
					temp_print(p, p_max_size, val / 1000, TEMP_CELSIUS);
				} else
					NORM_ERR("argument to smapi_bat_temp must be an integer");
			}
			OBJ(smapi_bat_power) {
				int idx, val;
				if(obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
					val = smapi_bat_installed(idx) ?
						smapi_get_bat_int(idx, "power_now") : 0;
					/* power_now is in mW, set to W with one digit precision */
					snprintf(p, p_max_size, "%.1f", ((double)val / 1000));
				} else
					NORM_ERR("argument to smapi_bat_power must be an integer");
			}
#ifdef X11
			OBJ(smapi_bat_bar) {
				if(obj->data.i >= 0 && smapi_bat_installed(obj->data.i))
					new_bar(p, obj->a, obj->b, (int)
							(255 * smapi_get_bat_int(obj->data.i, "remaining_percent") / 100));
				else
					new_bar(p, obj->a, obj->b, 0);
			}
#endif /* X11 */
#endif /* IBM */
			OBJ(include) {
				if(obj->sub) {
					char buf[max_user_text];

					generate_text_internal(buf, max_user_text, *obj->sub, cur);
					snprintf(p, p_max_size, "%s", buf);
				} else {
					p[0] = 0;
				}
			}
			OBJ(blink) {
				//blinking like this can look a bit ugly if the chars in the font don't have the same width
				char buf[max_user_text];
				unsigned int j;

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				snprintf(p, p_max_size, "%s", buf);
				if(total_updates % 2) {
					for(j=0; p[j] != 0; j++) {
						p[j] = ' ';
					}
				}
			}
			OBJ(to_bytes) {
				char buf[max_user_text];
				long long bytes;
				char unit[16];	// 16 because we can also have long names (like mega-bytes)

				generate_text_internal(buf, max_user_text, *obj->sub, cur);
				if(sscanf(buf, "%lli%s", &bytes, unit) == 2 && strlen(unit) < 16){
					if(strncasecmp("b", unit, 1) == 0) snprintf(buf, max_user_text, "%lli", bytes);
					else if(strncasecmp("k", unit, 1) == 0) snprintf(buf, max_user_text, "%lli", bytes * 1024);
					else if(strncasecmp("m", unit, 1) == 0) snprintf(buf, max_user_text, "%lli", bytes * 1024 * 1024);
					else if(strncasecmp("g", unit, 1) == 0) snprintf(buf, max_user_text, "%lli", bytes * 1024 * 1024 * 1024);
					else if(strncasecmp("t", unit, 1) == 0) snprintf(buf, max_user_text, "%lli", bytes * 1024 * 1024 * 1024 * 1024);
				}
				snprintf(p, p_max_size, "%s", buf);
			}
			OBJ(scroll) {
				unsigned int j, colorchanges = 0, frontcolorchanges = 0, visibcolorchanges = 0, strend;
				char *pwithcolors;
				char buf[max_user_text];
				generate_text_internal(buf, max_user_text,
				                       *obj->sub, cur);
				for(j = 0; buf[j] != 0; j++) {
					switch(buf[j]) {
					case '\n':	//place all the lines behind each other with LINESEPARATOR between them
#define LINESEPARATOR '|'
						buf[j]=LINESEPARATOR;
						break;
					case SPECIAL_CHAR:
						colorchanges++;
						break;
					}
				}
				//no scrolling necessary if the length of the text to scroll is too short
				if (strlen(buf) - colorchanges <= obj->data.scroll.show) {
					snprintf(p, p_max_size, "%s", buf);
					break;
				}
				//make sure a colorchange at the front is not part of the string we are going to show
				while(*(buf + obj->data.scroll.start) == SPECIAL_CHAR) {
					obj->data.scroll.start++;
				}
				//place all chars that should be visible in p, including colorchanges
				for(j=0; j < obj->data.scroll.show + visibcolorchanges; j++) {
					p[j] = *(buf + obj->data.scroll.start + j);
					if(p[j] == SPECIAL_CHAR) {
						visibcolorchanges++;
					}
					//if there is still room fill it with spaces
					if( ! p[j]) break;
				}
				for(; j < obj->data.scroll.show + visibcolorchanges; j++) {
					p[j] = ' ';
				}
				p[j] = 0;
				//count colorchanges in front of the visible part and place that many colorchanges in front of the visible part
				for(j = 0; j < obj->data.scroll.start; j++) {
					if(buf[j] == SPECIAL_CHAR) frontcolorchanges++;
				}
				pwithcolors=malloc(strlen(p) + 1 + colorchanges - visibcolorchanges);
				for(j = 0; j < frontcolorchanges; j++) {
					pwithcolors[j] = SPECIAL_CHAR;
				}
				pwithcolors[j] = 0;
				strcat(pwithcolors,p);
				strend = strlen(pwithcolors);
				//and place the colorchanges not in front or in the visible part behind the visible part
				for(j = 0; j < colorchanges - frontcolorchanges - visibcolorchanges; j++) {
					pwithcolors[strend + j] = SPECIAL_CHAR;
				}
				pwithcolors[strend + j] = 0;
				strcpy(p, pwithcolors);
				free(pwithcolors);
				//scroll
				obj->data.scroll.start += obj->data.scroll.step;
				if(buf[obj->data.scroll.start] == 0){
					 obj->data.scroll.start = 0;
				}
#ifdef X11
				//reset color when scroll is finished
				new_fg(p + strlen(p), obj->data.scroll.resetcolor);
#endif
			}
			OBJ(combine) {
				char buf[2][max_user_text];
				int i, j;
				long longest=0;
				int nextstart;
				int nr_rows[2];
				struct llrows {
					char* row;
					struct llrows* next;
				};
				struct llrows *ll_rows[2], *current[2];
				struct text_object * objsub = obj->sub;

				p[0]=0;
				for(i=0; i<2; i++) {
					nr_rows[i] = 1;
					nextstart = 0;
					ll_rows[i] = malloc(sizeof(struct llrows));
					current[i] = ll_rows[i];
					for(j=0; j<i; j++) objsub = objsub->sub;
					generate_text_internal(buf[i], max_user_text, *objsub, cur);
					for(j=0; buf[i][j] != 0; j++) {
						if(buf[i][j] == '\t') buf[i][j] = ' ';
						if(buf[i][j] == '\n') {
							buf[i][j] = 0;
							current[i]->row = strdup(buf[i]+nextstart);
							if(i==0 && (long)strlen(current[i]->row) > longest) longest = (long)strlen(current[i]->row);
							current[i]->next = malloc(sizeof(struct llrows));
							current[i] = current[i]->next;
							nextstart = j + 1;
							nr_rows[i]++;
						}
					}
					current[i]->row = strdup(buf[i]+nextstart);
					if(i==0 && (long)strlen(current[i]->row) > longest) longest = (long)strlen(current[i]->row);
					current[i]->next = NULL;
					current[i] = ll_rows[i];
				}
				for(j=0; j < (nr_rows[0] > nr_rows[1] ? nr_rows[0] : nr_rows[1] ); j++) {
					if(current[0]) {
						strcat(p, current[0]->row);
						i=strlen(current[0]->row);
					}else i = 0;
					while(i < longest) {
						strcat(p, " ");
						i++;
					}
					if(current[1]) {
						strcat(p, obj->data.combine.seperation);
						strcat(p, current[1]->row);
					}
					strcat(p, "\n");
					#ifdef HAVE_OPENMP
					#pragma omp parallel for schedule(dynamic,10)
					#endif /* HAVE_OPENMP */
					for(i=0; i<2; i++) if(current[i]) current[i]=current[i]->next;
				}
				#ifdef HAVE_OPENMP
				#pragma omp parallel for schedule(dynamic,10)
				#endif /* HAVE_OPENMP */
				for(i=0; i<2; i++) {
					while(ll_rows[i] != NULL) {
						current[i]=ll_rows[i];
						free(current[i]->row);
						ll_rows[i]=current[i]->next;
						free(current[i]);
					}
				}
			}
#ifdef NVIDIA
			OBJ(nvidia) {
				int value = get_nvidia_value(obj->data.nvidia.type, display);
				if(value == -1)
					snprintf(p, p_max_size, "N/A");
				else if (obj->data.nvidia.type == NV_TEMP)
					temp_print(p, p_max_size, (double)value, TEMP_CELSIUS);
				else if (obj->data.nvidia.print_as_float &&
						value > 0 && value < 100)
					snprintf(p, p_max_size, "%.1f", (float)value);
				else
					snprintf(p, p_max_size, "%d", value);
			}
#endif /* NVIDIA */
#ifdef APCUPSD
			OBJ(apcupsd) {
				/* This is just a meta-object to set host:port */
			}
			OBJ(apcupsd_name) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_NAME]);
			}
			OBJ(apcupsd_model) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_MODEL]);
			}
			OBJ(apcupsd_upsmode) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_UPSMODE]);
			}
			OBJ(apcupsd_cable) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_CABLE]);
			}
			OBJ(apcupsd_status) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_STATUS]);
			}
			OBJ(apcupsd_linev) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_LINEV]);
			}
			OBJ(apcupsd_load) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_LOAD]);
			}
			OBJ(apcupsd_loadbar) {
				double progress;
#ifdef X11
				if(output_methods & TO_X) {
					progress = atof(cur->apcupsd.items[APCUPSD_LOAD]) / 100.0 * 255.0;
					new_bar(p, obj->a, obj->b, (int) progress);
				} else {
#endif /* X11 */
					progress = atof(cur->apcupsd.items[APCUPSD_LOAD]);
					if(!obj->a) obj->a = DEFAULT_BAR_WIDTH_NO_X;
					new_bar_in_shell(p, p_max_size, (int) progress, obj->a);
#ifdef X11
				}
#endif /* X11 */
			}
#ifdef X11
			OBJ(apcupsd_loadgraph) {
				double progress;
				progress =	atof(cur->apcupsd.items[APCUPSD_LOAD]);
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
						  (int)progress, 100, 1, obj->char_a, obj->char_b);
			}
			OBJ(apcupsd_loadgauge) {
				double progress;
				progress =	atof(cur->apcupsd.items[APCUPSD_LOAD]) / 100.0 * 255.0;
				new_gauge(p, obj->a, obj->b,
						  (int)progress);
			}
#endif /* X11 */
			OBJ(apcupsd_charge) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_CHARGE]);
			}
			OBJ(apcupsd_timeleft) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_TIMELEFT]);
			}
			OBJ(apcupsd_temp) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_TEMP]);
			}
			OBJ(apcupsd_lastxfer) {
				snprintf(p, p_max_size, "%s",
						 cur->apcupsd.items[APCUPSD_LASTXFER]);
			}
#endif /* APCUPSD */
			break;
		}
#undef DO_JUMP


		{
			size_t a = strlen(p);

#ifdef HAVE_ICONV
			iconv_convert(a, buff_in, p, p_max_size);
#endif /* HAVE_ICONV */
			if (obj->type != OBJ_text && obj->type != OBJ_execp && obj->type != OBJ_execpi) {
				substitute_newlines(p, a - 2);
			}
			p += a;
			p_max_size -= a;
		}
		obj = obj->next;
	}
#ifdef X11
	/* load any new fonts we may have had */
	if (need_to_load_fonts) {
		load_fonts();
	}
#endif /* X11 */
}

