/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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

#include "conky.h"
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

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

void update_uname(void)
{
	uname(&info.uname_s);
}

double get_time(void)
{
	struct timeval tv;

	gettimeofday(&tv, 0);
	return tv.tv_sec + (tv.tv_usec / 1000000.0);
}

FILE *open_file(const char *file, int *reported)
{
	FILE *fp = fopen(file, "r");

	if (!fp) {
		if (!reported || *reported == 0) {
			ERR("can't open %s: %s", file, strerror(errno));
			if (reported) {
				*reported = 1;
			}
		}
		return 0;
	}

	return fp;
}

void variable_substitute(const char *s, char *dest, unsigned int n)
{
	while (*s && n > 1) {
		if (*s == '$') {
			s++;
			if (*s != '$') {
				char buf[256];
				const char *a, *var;
				unsigned int len;

				/* variable is either $foo or ${foo} */
				if (*s == '{') {
					s++;
					a = s;
					while (*s && *s != '}') {
						s++;
					}
				} else {
					a = s;
					while (*s && (isalnum((int) *s) || *s == '_')) {
						s++;
					}
				}

				/* copy variable to buffer and look it up */
				len = (s - a > 255) ? 255 : (s - a);
				strncpy(buf, a, len);
				buf[len] = '\0';

				if (*s == '}') {
					s++;
				}

				var = getenv(buf);

				if (var) {
					/* add var to dest */
					len = strlen(var);
					if (len >= n) {
						len = n - 1;
					}
					strncpy(dest, var, len);
					dest += len;
					n -= len;
				}
				continue;
			}
		}

		*dest++ = *s++;
		n--;
	}

	*dest = '\0';
}

/* network interface stuff */

static struct net_stat netstats[16];

struct net_stat *get_net_stat(const char *dev)
{
	unsigned int i;

	if (!dev) {
		return 0;
	}

	/* find interface stat */
	for (i = 0; i < 16; i++) {
		if (netstats[i].dev && strcmp(netstats[i].dev, dev) == 0) {
			return &netstats[i];
		}
	}

	/* wasn't found? add it */
	if (i == 16) {
		for (i = 0; i < 16; i++) {
			if (netstats[i].dev == 0) {
				netstats[i].dev = strndup(dev, text_buffer_size);
				return &netstats[i];
			}
		}
	}

	CRIT_ERR("too many interfaces used (limit is 16)");
	return 0;
}

void clear_net_stats(void)
{
	memset(netstats, 0, sizeof(netstats));
}

void free_dns_data(void)
{
	int i;
	struct dns_data *data = &info.nameserver_info;
	for (i = 0; i < data->nscount; i++)
		free(data->ns_list[i]);
	if (data->ns_list)
		free(data->ns_list);
	memset(data, 0, sizeof(struct dns_data));
}

//static double last_dns_update;

void update_dns_data(void)
{
	FILE *fp;
	char line[256];
	struct dns_data *data = &info.nameserver_info;

	/* maybe updating too often causes higher load because of /etc lying on a real FS
	if (current_update_time - last_dns_update < 10.0)
		return;
	else
		last_dns_update = current_update_time;
	*/

	free_dns_data();

	if ((fp = fopen("/etc/resolv.conf", "r")) == NULL)
		return;
	while(!feof(fp)) {
		if (fgets(line, 255, fp) == NULL) {
			break;
		}
		if (!strncmp(line, "nameserver ", 11)) {
			line[strlen(line) - 1] = '\0';	// remove trailing newline
			data->nscount++;
			data->ns_list = realloc(data->ns_list, data->nscount * sizeof(char *));
			data->ns_list[data->nscount - 1] = strndup(line + 11, text_buffer_size);
		}
	}
	fclose(fp);
}

void format_seconds(char *buf, unsigned int n, long seconds)
{
	long days;
	int hours, minutes;

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

static double last_meminfo_update;
static double last_fs_update;

unsigned long long need_mask;

#define NEED(a) ((need_mask & (1 << a)) && ((info.mask & (1 << a)) == 0))

void update_stuff(void)
{
	unsigned int i;

	info.mask = 0;

	if (no_buffers) {
		need_mask |= 1 << INFO_BUFFERS;
	}

	/* clear speeds and up status in case device was removed and doesn't get
	 * updated */

	for (i = 0; i < 16; i++) {
		if (netstats[i].dev) {
			netstats[i].up = 0;
			netstats[i].recv_speed = 0.0;
			netstats[i].trans_speed = 0.0;
		}
	}

	prepare_update();

	if (NEED(INFO_UPTIME)) {
		update_uptime();
	}

	if (NEED(INFO_PROCS)) {
		update_total_processes();
	}

	if (NEED(INFO_RUN_PROCS)) {
		update_running_processes();
	}

	if (NEED(INFO_CPU)) {
		update_cpu_usage();
	}

	if (NEED(INFO_NET)) {
		update_net_stats();
	}

	if (NEED(INFO_DISKIO)) {
		update_diskio();
	}

#if defined(__linux__)
	if (NEED(INFO_I8K)) {
		update_i8k();
	}
#endif /* __linux__ */

#ifdef MPD
	if (NEED(INFO_MPD)) {
		if (!info.mpd.timed_thread) {
			init_mpd_stats(&info.mpd);
			info.mpd.timed_thread = timed_thread_create(&update_mpd,
				(void *) &info.mpd, info.music_player_interval * 1000000);
			if (!info.mpd.timed_thread) {
				ERR("Failed to create MPD timed thread");
			}
			timed_thread_register(info.mpd.timed_thread, &info.mpd.timed_thread);
			if (timed_thread_run(info.mpd.timed_thread)) {
				ERR("Failed to run MPD timed thread");
			}
		}
	}
#endif

#ifdef MOC
  if (NEED(INFO_MOC)) {
    if (!info.moc.timed_thread) {
      init_moc(&info.moc);
      info.moc.timed_thread = timed_thread_create(&update_moc,
        (void *) &info.moc, info.music_player_interval * 100000);
      if (!info.moc.timed_thread) {
        ERR("Failed to create MOC timed thread");
      }
      timed_thread_register(info.moc.timed_thread, &info.moc.timed_thread);
      if (timed_thread_run(info.moc.timed_thread)) {
        ERR("Failed to run MOC timed thread");
      }
    }
  }
#endif
  
#ifdef XMMS2
	if (NEED(INFO_XMMS2)) {
		update_xmms2();
	}
#endif

#ifdef AUDACIOUS
	if (NEED(INFO_AUDACIOUS)) {
		update_audacious();
	}
#endif

#ifdef BMPX
	if (NEED(INFO_BMPX)) {
		update_bmpx();
	}
#endif

	if (NEED(INFO_LOADAVG)) {
		update_load_average();
	}

	if ((NEED(INFO_MEM) || NEED(INFO_BUFFERS) || NEED(INFO_TOP))
			&& current_update_time - last_meminfo_update > 6.9) {
		update_meminfo();
		if (no_buffers) {
			info.mem -= info.bufmem;
			info.memeasyfree += info.bufmem;
		}
		last_meminfo_update = current_update_time;
	}
	
#ifdef X11
	if (NEED(INFO_X11)) {
		update_x11info();
	}
#endif

	if (NEED(INFO_TOP)) {
		update_top();
	}

	/* update_fs_stat() won't do anything if there aren't fs -things */
	if (NEED(INFO_FS) && current_update_time - last_fs_update > 12.9) {
		update_fs_stats();
		last_fs_update = current_update_time;
	}
#ifdef TCP_PORT_MONITOR
	if (NEED(INFO_TCP_PORT_MONITOR)) {
		tcp_portmon_update();
	}
#endif
	if (NEED(INFO_ENTROPY)) {
		update_entropy();
	}
#if defined(__linux__)
	if (NEED(INFO_USERS)) {
		update_users();
	}
	if (NEED(INFO_GW)) {
		update_gateway_info();
	}
#endif /* __linux__ */
	if (NEED(INFO_DNS)) {
		update_dns_data();
	}
}

int round_to_int(float f)
{
	if (f >= 0.0) {
		return (int) (f + 0.5);
	} else {
		return (int) (f - 0.5);
	}
}
