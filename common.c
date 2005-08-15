/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */

#include "conky.h"
#include "remoted.h"
#include "remotec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>

struct information info;

void update_uname()
{
	uname(&info.uname_s);
}

double get_time()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec + tv.tv_usec / 1000000.0;
}

FILE *open_file(const char *file, int *reported)
{
	FILE *fp = fopen(file, "r");
	if (!fp) {
		if (!reported || *reported == 0) {
			CRIT_ERR("can't open %s: %s", file, strerror(errno));
			if (reported)
				*reported = 1;
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
					while (*s && *s != '}')
						s++;
				} else {
					a = s;
					while (*s && (isalnum((int) *s)
						      || *s == '_'))
						s++;
				}

				/* copy variable to buffer and look it up */
				len = (s - a > 255) ? 255 : (s - a);
				strncpy(buf, a, len);
				buf[len] = '\0';

				if (*s == '}')
					s++;

				var = getenv(buf);

				if (var) {
					/* add var to dest */
					len = strlen(var);
					if (len >= n)
						len = n - 1;
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

	if (!dev)
		return 0;

	/* find interface stat */
	for (i = 0; i < 16; i++) {
		if (netstats[i].dev && strcmp(netstats[i].dev, dev) == 0)
			return &netstats[i];
	}

	/* wasn't found? add it */
	if (i == 16) {
		for (i = 0; i < 16; i++) {
			if (netstats[i].dev == 0) {
				netstats[i].dev = strdup(dev);
				return &netstats[i];
			}
		}
	}

	CRIT_ERR("too many interfaces used (limit is 16)");
	return 0;
}

void format_seconds(char *buf, unsigned int n, long t)
{
	if (t >= 24 * 60 * 60)	/* hours necessary when there are days? */
		snprintf(buf, n, "%ldd %ldh %ldm", t / 60 / 60 / 24,
			 (t / 60 / 60) % 24, (t / 60) % 60);
	else if (t >= 60 * 60)
		snprintf(buf, n, "%ldh %ldm", (t / 60 / 60) % 24,
			 (t / 60) % 60);
	else
		snprintf(buf, n, "%ldm %lds", t / 60, t % 60);
}

void format_seconds_short(char *buf, unsigned int n, long t)
{
	if (t >= 24 * 60 * 60)
		snprintf(buf, n, "%ldd %ldh", t / 60 / 60 / 24,
			 (t / 60 / 60) % 24);
	else if (t >= 60 * 60)
		snprintf(buf, n, "%ldh %ldm", (t / 60 / 60) % 24,
			 (t / 60) % 60);
	else
		snprintf(buf, n, "%ldm", t / 60);
}

static double last_meminfo_update;
static double last_fs_update;

unsigned long long need_mask;

void update_stuff()
{
	unsigned int i;
	info.mask = 0;

	if (no_buffers)
		need_mask |= 1 << INFO_BUFFERS;

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
	/* client(); this is approximately where the client should be called */
#define NEED(a) ((need_mask & (1 << a)) && ((info.mask & (1 << a)) == 0))

	if (NEED(INFO_UPTIME))
		update_uptime();

	if (NEED(INFO_PROCS))
		update_total_processes();

	if (NEED(INFO_RUN_PROCS))
		update_running_processes();

	if (NEED(INFO_CPU))
		update_cpu_usage();

	if (NEED(INFO_NET))
		update_net_stats();

	if (NEED(INFO_WIFI))
		update_wifi_stats();

	if (NEED(INFO_MAIL))
		update_mail_count();

	if (NEED(INFO_TOP))
		update_top();

#ifdef MLDONKEY
	if (NEED(INFO_MLDONKEY))
		get_mldonkey_status(&mlconfig, &mlinfo);
#endif

#ifdef SETI
	if (NEED(INFO_SETI))
		update_seti();
#endif

#ifdef MPD
	if (NEED(INFO_MPD))
		update_mpd();
#endif

	if (NEED(INFO_LOADAVG))
		update_load_average();

#ifdef METAR
	if (NEED(INFO_METAR)
	    && current_update_time - last_metar_update > 1200.9
	    && info.looped) {
		update_metar();
		last_metar_update = current_update_time;
	}
#endif
	if ((NEED(INFO_MEM) || NEED(INFO_BUFFERS)) &&
	    current_update_time - last_meminfo_update > 6.9) {
		update_meminfo();
		if (no_buffers)
			info.mem -= info.bufmem;
		last_meminfo_update = current_update_time;
	}

	/* update_fs_stat() won't do anything if there aren't fs -things */
	if (NEED(INFO_FS) && current_update_time - last_fs_update > 12.9) {
		update_fs_stats();
		last_fs_update = current_update_time;
	}
}
