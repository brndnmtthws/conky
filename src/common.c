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
 */

#include "config.h"
#include "conky.h"
#include "fs.h"
#include "logging.h"
#include <ctype.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include "diskio.h"
#include <fcntl.h>

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "freebsd.h"
#elif defined(__OpenBSD__)
#include "openbsd.h"
#endif

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

/* Converts '~/...' paths to '/home/blah/...' assumes that 'dest' is at least
 * DEFAULT_TEXT_BUFFER_SIZE.  It's similar to variable_substitute, except only
 * cheques for $HOME and ~/ in path */
void to_real_path(char *dest, const char *source)
{
	char tmp[DEFAULT_TEXT_BUFFER_SIZE];
	if (sscanf(source, "~/%s", tmp) || sscanf(source, "$HOME/%s", tmp)) {
		char *homedir = getenv("HOME");
		if (homedir) {
			snprintf(dest, DEFAULT_TEXT_BUFFER_SIZE, "%s/%s", homedir, tmp);
		} else {
			NORM_ERR("$HOME environment variable doesn't exist");
			strncpy(dest, source, DEFAULT_TEXT_BUFFER_SIZE);
		}
	} else if (dest != source) {	//see changelog 2009-06-29 if you doubt that this check is necessary 
		strncpy(dest, source, DEFAULT_TEXT_BUFFER_SIZE);
	}
}

int open_fifo(const char *file, int *reported)
{
	char path[DEFAULT_TEXT_BUFFER_SIZE];
	int fd = 0;

	to_real_path(path, file);
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
	char path[DEFAULT_TEXT_BUFFER_SIZE];
	FILE *fp = 0;

	to_real_path(path, file);
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

struct net_stat *get_net_stat(const char *dev, void *free_at_crash1, void *free_at_crash2)
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
	for (i = 0; i < 16; i++) {
		if (netstats[i].dev == 0) {
			netstats[i].dev = strndup(dev, text_buffer_size);
			return &netstats[i];
		}
	}

	CRIT_ERR(free_at_crash1, free_at_crash2, "too many interfaces used (limit is 16)");
	return 0;
}

void clear_net_stats(void)
{
	int i;
	for (i = 0; i < 16; i++) {
		if (netstats[i].dev) {
			free(netstats[i].dev);
		}
	}
	memset(netstats, 0, sizeof(netstats));
}

/* We should check if this is ok with OpenBSD and NetBSD as well. */
int interface_up(const char *dev)
{
	int fd;
	struct ifreq ifr;

	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		CRIT_ERR(NULL, NULL, "could not create sockfd");
		return 0;
	}
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	if (ioctl(fd, SIOCGIFFLAGS, &ifr)) {
		/* if device does not exist, treat like not up */
		if (errno != ENODEV && errno != ENXIO)
			perror("SIOCGIFFLAGS");
		goto END_FALSE;
	}

	if (!(ifr.ifr_flags & IFF_UP)) /* iface is not up */
		goto END_FALSE;
	if (ifup_strictness == IFUP_UP)
		goto END_TRUE;

	if (!(ifr.ifr_flags & IFF_RUNNING))
		goto END_FALSE;
	if (ifup_strictness == IFUP_LINK)
		goto END_TRUE;

	if (ioctl(fd, SIOCGIFADDR, &ifr)) {
		perror("SIOCGIFADDR");
		goto END_FALSE;
	}
	if (((struct sockaddr_in *)&(ifr.ifr_ifru.ifru_addr))->sin_addr.s_addr)
		goto END_TRUE;

END_FALSE:
	close(fd);
	return 0;
END_TRUE:
	close(fd);
	return 1;
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

/* Linked list containing the functions to call upon each update interval.
 * Populated while initialising text objects in construct_text_object(). */
static struct update_cb {
	struct update_cb *next;
	void (*func)(void);
} update_cb_head = {
	.next = NULL,
};

/* Register an update callback. Don't allow duplicates, to minimise side
 * effects and overhead. */
void add_update_callback(void (*func)(void))
{
	struct update_cb *uc = &update_cb_head;

	if (!func)
		return;

	while (uc->next) {
		if (uc->next->func == func)
			return;
		uc = uc->next;
	}
	uc->next = malloc(sizeof(struct update_cb));
	memset(uc->next, 0, sizeof(struct update_cb));
	uc->next->func = func;
}

/* Free the list element uc and all decendants recursively. */
static void __free_update_callbacks(struct update_cb *uc)
{
	if (uc->next)
		__free_update_callbacks(uc->next);
	free(uc);
}

/* Free the whole list of update callbacks. */
void free_update_callbacks(void)
{
	if (update_cb_head.next)
		__free_update_callbacks(update_cb_head.next);
	update_cb_head.next = NULL;
}

int no_buffers;

void update_stuff(void)
{
	int i;
	struct update_cb *uc;

	/* clear speeds and up status in case device was removed and doesn't get
	 * updated */

	#ifdef HAVE_OPENMP
	#pragma omp parallel for schedule(dynamic,10)
	#endif /* HAVE_OPENMP */
	for (i = 0; i < 16; i++) {
		if (netstats[i].dev) {
			netstats[i].up = 0;
			netstats[i].recv_speed = 0.0;
			netstats[i].trans_speed = 0.0;
		}
	}

	prepare_update();

	for (uc = update_cb_head.next; uc; uc = uc->next)
		(*uc->func)();

	/* XXX: move the following into the update_meminfo() functions? */
	if (no_buffers) {
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

