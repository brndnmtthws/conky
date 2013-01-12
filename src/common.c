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
#include "fs.h"
#include "logging.h"
#include "net_stat.h"
#include "specials.h"
#include "timeinfo.h"
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
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

int update_uname(void)
{
	uname(&info.uname_s);
	return 0;
}

double get_time(void)
{
	struct timespec tv;

	clock_gettime(CLOCK_MONOTONIC, &tv);
	return tv.tv_sec + (tv.tv_nsec * 1e-9);
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
			/*strncpy(dest, source, DEFAULT_TEXT_BUFFER_SIZE);*/
			strncpy(dest, tmp, DEFAULT_TEXT_BUFFER_SIZE);
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

void format_seconds(char *buf, unsigned int n, long seconds)
{
	long days;
	int hours, minutes;

	if (times_in_seconds()) {
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

	if (times_in_seconds()) {
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

/* Linked list containing the functions to call upon each update interval.
 * Populated while initialising text objects in construct_text_object(). */
static struct update_cb {
	struct update_cb *next;
	int (*func)(void);
	pthread_t thread;
	sem_t start_wait, end_wait;

    /* set to 1 when starting the thread
	 * set to 0 to request thread termination */
	volatile char running;
} update_cb_head = {
	.next = NULL,
	.func = NULL,
};

static void *run_update_callback(void *) __attribute__((noreturn));

static int threading_started = 0;

/* Register an update callback. Don't allow duplicates, to minimise side
 * effects and overhead. */
void add_update_callback(int (*func)(void))
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
	uc = uc->next;

	memset(uc, 0, sizeof(struct update_cb));
	uc->func = func;
	sem_init(&uc->start_wait, 0, 0);
	sem_init(&uc->end_wait, 0, 0);

	if (threading_started) {
		if (!uc->running) {
			uc->running = 1;
			pthread_create(&uc->thread, NULL, &run_update_callback, uc);
		}
	}
}

/* Free the list element uc and all decendants recursively. */
static void __free_update_callbacks(struct update_cb *uc)
{
	if (uc->next)
		__free_update_callbacks(uc->next);

	if (uc->running) {
		/* send cancellation request, then trigger and join the thread */
		uc->running = 0;
		sem_post(&uc->start_wait);
	}
	if (pthread_join(uc->thread, NULL)) {
		NORM_ERR("Error destroying thread");
	}

	/* finally destroy the semaphores */
	sem_destroy(&uc->start_wait);
	sem_destroy(&uc->end_wait);

	free(uc);
}

/* Free the whole list of update callbacks. */
void free_update_callbacks(void)
{
	if (update_cb_head.next)
		__free_update_callbacks(update_cb_head.next);
	update_cb_head.next = NULL;
}

/* We cannot start threads before we forked to background, because the threads
 * would remain in the wrong process. Because of that, add_update_callback()
 * doesn't create threads before start_update_threading() is called.
 * start_update_threading() starts all threads previously registered, and sets a
 * flag so that future threads are automagically started by
 * add_update_callback().
 * This text is almost longer than the actual code.
 */
void start_update_threading(void)
{
	struct update_cb *uc;

	threading_started = 1;

	for (uc = update_cb_head.next; uc; uc = uc->next) {
		if (!uc->running) {
			uc->running = 1;
			pthread_create(&uc->thread, NULL, &run_update_callback, uc);
		}
	}
}

static void *run_update_callback(void *data)
{
	struct update_cb *ucb = data;

	if (!ucb || !ucb->func) pthread_exit(NULL);

	while (1) {
		if (sem_wait(&ucb->start_wait)) pthread_exit(NULL);
		if (ucb->running == 0) pthread_exit(NULL);
		if((*ucb->func)()) {
			ucb->next = ucb;	//this is normally not be possible, so we use it to show that there was a critical error
			sem_post(&ucb->end_wait);
			sem_post(&ucb->end_wait);
			pthread_exit(NULL);
		}
		if (sem_post(&ucb->end_wait)) pthread_exit(NULL);
	}
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
	for (i = 0; i < MAX_NET_INTERFACES; i++) {
		if (netstats[i].dev) {
			netstats[i].up = 0;
			netstats[i].recv_speed = 0.0;
			netstats[i].trans_speed = 0.0;
		}
	}

	prepare_update();

	for (uc = update_cb_head.next; uc; uc = uc->next) {
		if (sem_post(&uc->start_wait)) {
			NORM_ERR("Semaphore error");
		}
	}
	/* need to synchronise here, otherwise locking is needed (as data
	 * would be printed with some update callbacks still running) */
	for (uc = update_cb_head.next; uc; uc = uc->next) {
		sem_wait(&uc->end_wait);
		if(uc == uc->next) {
			pthread_join(uc->thread, NULL);
			free(uc);
			exit(EXIT_FAILURE);
		}
	}

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

#ifdef X11
void scan_loadgraph_arg(struct text_object *obj, const char *arg)
{
	char *buf = 0;

	buf = scan_graph(obj, arg, 0);
	if (buf)
		free(buf);
}

void print_loadgraph(struct text_object *obj, char *p, int p_max_size)
{
	new_graph(obj, p, p_max_size, info.loadavg[0]);
}
#endif /* X11 */
