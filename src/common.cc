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

#include "config.h"
#include "conky.h"
#include "core.h"
#include "fs.h"
#include "logging.h"
#include "net_stat.h"
#include "specials.h"
#include "temphelper.h"
#include "timeinfo.h"
#include "top.h"
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

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

#include "update-cb.hh"

#ifdef BUILD_CURL
#include "ccurl_thread.h"
#endif /* BUILD_CURL */

/* folds a string over top of itself, like so:
 *
 * if start is "blah", and you call it with count = 1, the result will be "lah"
 */
void strfold(char *start, int count)
{
	char *curplace;
	for (curplace = start + count; *curplace != 0; curplace++) {
		*(curplace - count) = *curplace;
	}
	*(curplace - count) = 0;
}

#ifndef HAVE_STRNDUP
// use our own strndup() if it's not available
char *strndup(const char *s, size_t n)
{
	if (strlen(s) > n) {
		char *ret = malloc(n + 1);
		strncpy(ret, s, n);
		ret[n] = 0;
		return ret;
	} else {
		return strdup(s);
	}
}
#endif /* HAVE_STRNDUP */

int update_uname(void)
{
	uname(&info.uname_s);

#if defined(__DragonFly__)
	{
		size_t desc_n; char desc[256];

		if (sysctlbyname("kern.version", NULL, &desc_n, NULL, 0) == -1 ||
			sysctlbyname("kern.version", desc, &desc_n, NULL, 0) == -1)
			perror("kern.version");
		else {
			char *start = desc;
			strsep(&start, " ");
			strcpy(info.uname_v, strsep(&start, " "));
		}

		if (errno == ENOMEM) printf("desc_n %lu\n", desc_n);
	}
#endif

	return 0;
}

double get_time(void)
{
	struct timespec tv;

	clock_gettime(CLOCK_MONOTONIC, &tv);
	return tv.tv_sec + (tv.tv_nsec * 1e-9);
}

/* Converts '~/...' paths to '/home/blah/...'
 * It's similar to variable_substitute, except only cheques for $HOME and ~/ in path */
std::string to_real_path(const std::string &source)
{
	const char *homedir = getenv("HOME");
	if(source.find("~/") == 0)
		return homedir + source.substr(1);
	else if(source.find("$HOME/") == 0)
		return homedir + source.substr(5);
	else
		return source;
}

int open_fifo(const char *file, int *reported)
{
	int fd = 0;

	fd = open(file, O_RDONLY | O_NONBLOCK);

	if (fd == -1) {
		if (!reported || *reported == 0) {
			NORM_ERR("can't open %s: %s", file, strerror(errno));
			if (reported) {
				*reported = 1;
			}
		}
		return -1;
	}

	return fd;
}

FILE *open_file(const char *file, int *reported)
{
	FILE *fp = 0;

	fp = fopen(file, "r");

	if (!fp) {
		if (!reported || *reported == 0) {
			NORM_ERR("can't open %s: %s", file, strerror(errno));
			if (reported) {
				*reported = 1;
			}
		}
		return NULL;
	}

	return fp;
}

std::string variable_substitute(std::string s)
{
	std::string::size_type pos = 0;
	while((pos = s.find('$', pos)) != std::string::npos) {
		if(pos + 1 >= s.size())
			break;

		if(s[pos+1] == '$') {
			s.erase(pos, 1);
			++pos;
		} else {
			std::string var;
			std::string::size_type l = 0;

			if(isalpha(s[pos+1])) {
				l = 1;
				while(pos+l < s.size() && isalnum(s[pos+l]))
					++l;
				var = s.substr(pos+1, l-1);
			} else if(s[pos+1] == '{') {
				l = s.find('}', pos);
				if(l == std::string::npos)
					break;
				l -= pos - 1;
				var = s.substr(pos+2, l-3);
			} else
				++pos;

			if(l) {
				s.erase(pos, l);
				const char *val = getenv(var.c_str());
				if(val) {
					s.insert(pos, val);
					pos += strlen(val);
				}

			}
		}
	}

	return s;
}

void format_seconds(char *buf, unsigned int n, long seconds)
{
	long days;
	int hours, minutes;

	if (times_in_seconds.get(*state)) {
		snprintf(buf, n, "%ld", seconds);
		return;
	}

	days = seconds / 86400;
	seconds %= 86400;
	hours = seconds / 3600;
	seconds %= 3600;
	minutes = seconds / 60;
	seconds %= 60;

	if (days > 0) {
		snprintf(buf, n, "%ldd %dh %dm", days, hours, minutes);
	} else {
		snprintf(buf, n, "%dh %dm %lds", hours, minutes, seconds);
	}
}

void format_seconds_short(char *buf, unsigned int n, long seconds)
{
	long days;
	int hours, minutes;

	if (times_in_seconds.get(*state)) {
		snprintf(buf, n, "%ld", seconds);
		return;
	}

	days = seconds / 86400;
	seconds %= 86400;
	hours = seconds / 3600;
	seconds %= 3600;
	minutes = seconds / 60;
	seconds %= 60;

	if (days > 0) {
		snprintf(buf, n, "%ldd %dh", days, hours);
	} else if (hours > 0) {
		snprintf(buf, n, "%dh %dm", hours, minutes);
	} else {
		snprintf(buf, n, "%dm %lds", minutes, seconds);
	}
}

conky::simple_config_setting<bool> no_buffers("no_buffers", true, true);

void update_stuff(void)
{
	int i;

	/* clear speeds, addresses and up status in case device was removed and
	 *  doesn't get updated */

	#ifdef HAVE_OPENMP
	#pragma omp parallel for schedule(dynamic,10)
	#endif /* HAVE_OPENMP */
	for (i = 0; i < MAX_NET_INTERFACES; i++) {
		if (netstats[i].dev) {
			netstats[i].up = 0;
			netstats[i].recv_speed = 0.0;
			netstats[i].trans_speed = 0.0;
			netstats[i].addr.sa_data[2] = 0;
			netstats[i].addr.sa_data[3] = 0;
			netstats[i].addr.sa_data[4] = 0;
			netstats[i].addr.sa_data[5] = 0;
		}
	}

	prepare_update();

	conky::run_all_callbacks();

	/* XXX: move the following into the update_meminfo() functions? */
	if (no_buffers.get(*state)) {
		info.mem -= info.bufmem;
		info.memeasyfree += info.bufmem;
	}
}

/* Ohkie to return negative values for temperatures */
int round_to_int_temp(float f)
{
	if (f >= 0.0) {
		return (int) (f + 0.5);
	} else {
		return (int) (f - 0.5);
	}
}
/* Don't return negative values for cpugraph, bar, gauge, percentage.
 * Causes unreasonable numbers to show */
unsigned int round_to_int(float f)
{
	if (f >= 0.0) {
		return (int) (f + 0.5);
	} else {
		return 0;
	}
}

void scan_loadavg_arg(struct text_object *obj, const char *arg)
{
	obj->data.i = 0;
	if (arg && !arg[1] && isdigit(arg[0])) {
		obj->data.i = atoi(arg);
		if (obj->data.i > 3 || obj->data.i < 1) {
			NORM_ERR("loadavg arg needs to be in range (1,3)");
			obj->data.i = 0;
		}
	}
	/* convert to array index (or the default (-1)) */
	obj->data.i--;
}

void print_loadavg(struct text_object *obj, char *p, int p_max_size)
{
	float *v = info.loadavg;

	if (obj->data.i < 0) {
		snprintf(p, p_max_size, "%.2f %.2f %.2f", v[0], v[1], v[2]);
	} else {
		snprintf(p, p_max_size, "%.2f", v[obj->data.i]);
	}
}

void scan_no_update(struct text_object *obj, const char *arg)
{
	obj->data.s = (char*) malloc(text_buffer_size.get(*state));
	evaluate(arg, obj->data.s, text_buffer_size.get(*state));
	obj->data.s = (char*) realloc(obj->data.s, strlen(obj->data.s) + 1);
}

void free_no_update(struct text_object *obj) {
	free(obj->data.s);
}

void print_no_update(struct text_object *obj, char *p, int p_max_size)
{
	snprintf(p, p_max_size, "%s", obj->data.s);
}

#ifdef BUILD_X11
void scan_loadgraph_arg(struct text_object *obj, const char *arg)
{
	char *buf = 0;

	buf = scan_graph(obj, arg, 0);
	free_and_zero(buf);
}

double loadgraphval(struct text_object *obj)
{
	(void)obj;

	return info.loadavg[0];
}
#endif /* BUILD_X11 */

uint8_t cpu_percentage(struct text_object *obj)
{
	if (obj->data.i > info.cpu_count) {
		NORM_ERR("obj->data.i %i info.cpu_count %i",
				obj->data.i, info.cpu_count);
		CRIT_ERR(NULL, NULL, "attempting to use more CPUs than you have!");
	}
	if (info.cpu_usage) {
		return round_to_int(info.cpu_usage[obj->data.i] * 100.0);
	} else {
		return 0;
	}
}

double cpu_barval(struct text_object *obj)
{
	if (info.cpu_usage) {
		return info.cpu_usage[obj->data.i];
	} else {
		return 0.;
	}
}

#define PRINT_HR_GENERATOR(name) \
void print_##name(struct text_object *obj, char *p, int p_max_size) \
{ \
	(void)obj; \
	human_readable(info.name * 1024, p, p_max_size); \
}

PRINT_HR_GENERATOR(mem)
PRINT_HR_GENERATOR(memwithbuffers)
PRINT_HR_GENERATOR(memeasyfree)
PRINT_HR_GENERATOR(memfree)
PRINT_HR_GENERATOR(memmax)
PRINT_HR_GENERATOR(memdirty)
PRINT_HR_GENERATOR(swap)
PRINT_HR_GENERATOR(swapfree)
PRINT_HR_GENERATOR(swapmax)

uint8_t mem_percentage(struct text_object *obj)
{
	(void)obj;

	return (info.memmax ? round_to_int(info.mem * 100 / info.memmax) : 0);
}

double mem_barval(struct text_object *obj)
{
	(void)obj;

	return info.memmax ? ((double)info.mem / info.memmax) : 0;
}

double mem_with_buffers_barval(struct text_object *obj)
{
	(void)obj;

	return info.memmax ? ((double)info.memwithbuffers / info.memmax) : 0;
}

uint8_t swap_percentage(struct text_object *obj)
{
	(void)obj;

	return (info.swapmax ? round_to_int(info.swap * 100 / info.swapmax) : 0);
}

double swap_barval(struct text_object *obj)
{
	(void)obj;

	return info.swapmax ? ((double)info.swap / info.swapmax) : 0;
}

void print_kernel(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%s", info.uname_s.release);
}

void print_machine(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%s", info.uname_s.machine);
}

void print_nodename(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%s", info.uname_s.nodename);
}

void print_nodename_short(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%s", info.uname_s.nodename);
	for(int i=0; p[i] != 0; i++) {
		if(p[i] == '.') {
			p[i] = 0;
			break;
		}
	}
}

void print_sysname(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%s", info.uname_s.sysname);
}

#if defined(__DragonFly__)
void print_version(struct text_object *obj, char *p, int p_max_size)
{
    (void)obj;
    snprintf(p, p_max_size, "%s", info.uname_v);
}
#endif

void print_uptime(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	format_seconds(p, p_max_size, (int)info.uptime);
}

void print_uptime_short(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	format_seconds_short(p, p_max_size, (int)info.uptime);
}

void print_processes(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	spaced_print(p, p_max_size, "%hu", 4, info.procs);
}

void print_running_processes(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	spaced_print(p, p_max_size, "%hu", 4, info.run_procs);
}

void print_running_threads(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	spaced_print(p, p_max_size, "%hu", 4, info.run_threads);
}

void print_threads(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	spaced_print(p, p_max_size, "%hu", 4, info.threads);
}

void print_buffers(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	human_readable(info.buffers * 1024, p, p_max_size);
}

void print_cached(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	human_readable(info.cached * 1024, p, p_max_size);
}

void print_evaluate(struct text_object *obj, char *p, int p_max_size)
{
	evaluate(obj->data.s, p, p_max_size);
}

int if_empty_iftest(struct text_object *obj)
{
	std::vector<char> buf(max_user_text.get(*state));
	int result = 1;

	generate_text_internal(&(buf[0]), max_user_text.get(*state), *obj->sub);

	if (strlen(&(buf[0])) != 0) {
		result = 0;
	}
	return result;
}

static int check_contains(char *f, char *s)
{
	int ret = 0;
	FILE *where = open_file(f, 0);

	if (where) {
		char buf1[256];

		while (fgets(buf1, 256, where)) {
			if (strstr(buf1, s)) {
				ret = 1;
				break;
			}
		}
		fclose(where);
	} else {
		NORM_ERR("Could not open the file");
	}
	return ret;
}

int if_existing_iftest(struct text_object *obj)
{
	char *spc;
	int result = 1;

	spc = strchr(obj->data.s, ' ');
	if (!spc && access(obj->data.s, F_OK)) {
		result = 0;
	} else if (spc) {
		*spc = '\0';
		if (check_contains(obj->data.s, spc + 1))
			result = 0;
		*spc = ' ';
	}
	return result;
}

int if_running_iftest(struct text_object *obj)
{
#ifdef __linux__
	if (!get_process_by_name(obj->data.s)) {
#else
	if ((obj->data.s) && system(obj->data.s)) {
#endif
		return 0;
	}
	return 1;
}

#ifndef __OpenBSD__
void print_acpitemp(struct text_object *obj, char *p, int p_max_size)
{
	temp_print(p, p_max_size, get_acpi_temperature(obj->data.i), TEMP_CELSIUS);
}

void free_acpitemp(struct text_object *obj)
{
	close(obj->data.i);
}
#endif /* !__OpenBSD__ */

void print_freq(struct text_object *obj, char *p, int p_max_size)
{
	static int ok = 1;
	if (ok) {
		ok = get_freq(p, p_max_size, "%.0f", 1, obj->data.i);
	}
}

void print_freq_g(struct text_object *obj, char *p, int p_max_size)
{
	static int ok = 1;
	if (ok) {
#ifndef __OpenBSD__
		ok = get_freq(p, p_max_size, "%'.2f", 1000, obj->data.i);
#else
		/* OpenBSD has no such flag (SUSv2) */
		ok = get_freq(p, p_max_size, "%.2f", 1000, obj->data.i);
#endif /* __OpenBSD */
	}
}

#ifndef __OpenBSD__
void print_acpifan(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	get_acpi_fan(p, p_max_size);
}

void print_acpiacadapter(struct text_object *obj, char *p, int p_max_size)
{
	get_acpi_ac_adapter(p, p_max_size, (const char *)obj->data.opaque);
}

void print_battery(struct text_object *obj, char *p, int p_max_size)
{
	get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_STATUS);
}

void print_battery_time(struct text_object *obj, char *p, int p_max_size)
{
	get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_TIME);
}

uint8_t battery_percentage(struct text_object *obj)
{
	return get_battery_perct(obj->data.s);
}

void print_battery_short(struct text_object *obj, char *p, int p_max_size)
{
	get_battery_short_status(p, p_max_size, obj->data.s);
}
#endif /* !__OpenBSD__ */


void print_blink(struct text_object *obj, char *p, int p_max_size)
{
	//blinking like this can look a bit ugly if the chars in the font don't have the same width
	std::vector<char> buf(max_user_text.get(*state));
	static int visible = 1;
	static int last_len = 0;
	int i;

	if (visible) {
		generate_text_internal(&(buf[0]), max_user_text.get(*state), *obj->sub);
		last_len = strlen(&(buf[0]));
	} else {
		for (i = 0; i < last_len; i++)
			buf[i] = ' ';
	}

	snprintf(p, p_max_size, "%s", &(buf[0]));
	visible = !visible;
}

void print_include(struct text_object *obj, char *p, int p_max_size)
{
	std::vector<char> buf(max_user_text.get(*state));

	if (!obj->sub)
		return;

	generate_text_internal(&(buf[0]), max_user_text.get(*state), *obj->sub);
	snprintf(p, p_max_size, "%s", &(buf[0]));
}

#ifdef BUILD_CURL
void print_stock(struct text_object *obj, char *p, int p_max_size)
{
	if( ! obj->data.s) {
		p[0] = 0;
		return;
	}
	ccurl_process_info(p, p_max_size, obj->data.s, 1);
}

void free_stock(struct text_object *obj)
{
	free(obj->data.s);
}
#endif /* BUILD_CURL */

void print_to_bytes(struct text_object *obj, char *p, int p_max_size)
{
	std::vector<char> buf(max_user_text.get(*state));
	long long bytes;
	char unit[16];	// 16 because we can also have long names (like mega-bytes)

	generate_text_internal(&(buf[0]), max_user_text.get(*state), *obj->sub);
	if(sscanf(&(buf[0]), "%lli%s", &bytes, unit) == 2 && strlen(unit) < 16){
		if(strncasecmp("b", unit, 1) == 0) snprintf(&(buf[0]), max_user_text.get(*state), "%lli", bytes);
		else if(strncasecmp("k", unit, 1) == 0) snprintf(&(buf[0]), max_user_text.get(*state), "%lli", bytes * 1024);
		else if(strncasecmp("m", unit, 1) == 0) snprintf(&(buf[0]), max_user_text.get(*state), "%lli", bytes * 1024 * 1024);
		else if(strncasecmp("g", unit, 1) == 0) snprintf(&(buf[0]), max_user_text.get(*state), "%lli", bytes * 1024 * 1024 * 1024);
		else if(strncasecmp("t", unit, 1) == 0) snprintf(&(buf[0]), max_user_text.get(*state), "%lli", bytes * 1024 * 1024 * 1024 * 1024);
	}
	snprintf(p, p_max_size, "%s", &(buf[0]));
}


void print_updates(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%d", get_total_updates());
}

int updatenr_iftest(struct text_object *obj)
{
	if (get_total_updates() % get_updatereset() != obj->data.i - 1)
		return 0;
	return 1;
}
