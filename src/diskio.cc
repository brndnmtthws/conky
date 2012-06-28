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
 * (see AUTHORS)
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
#include "conky.h"	/* text_buffer_size */
#include "core.h"
#include "logging.h"
#include "diskio.h"
#include "common.h"
#include "specials.h"
#include "text_object.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <vector>

/* this is the root of all per disk stats,
 * also containing the totals. */
struct diskio_stat stats;

void clear_diskio_stats(void)
{
	struct diskio_stat *cur;
	while (stats.next) {
		cur = stats.next;
		stats.next = stats.next->next;
		free_and_zero(cur->dev);
		free(cur);
	}
}

struct diskio_stat *prepare_diskio_stat(const char *s)
{
	struct stat sb;
	std::vector<char> stat_name(text_buffer_size.get(*state)), device_name(text_buffer_size.get(*state));
	struct diskio_stat *cur = &stats;

	if (!s)
		return &stats;

#if defined(__FreeBSD__) || defined(__DragonFly__)
	if (strncmp(s, "/dev/", 5) == 0) {
		// supplied a /dev/device arg, so cut off the /dev part
		strncpy(&(device_name[0]), s + 5, text_buffer_size.get(*state));
	} else
#endif
	strncpy(&(device_name[0]), s, text_buffer_size.get(*state));

	snprintf(&(stat_name[0]), text_buffer_size.get(*state), "/dev/%s", &(device_name[0]));

	if (stat(&(stat_name[0]), &sb)) {
		NORM_ERR("diskio device '%s' does not exist", s);
	}

	/* lookup existing */
	while (cur->next) {
		cur = cur->next;
		if (!strcmp(cur->dev, &(device_name[0]))) {
			return cur;
		}
	}

	/* no existing found, make a new one */
	cur->next = new diskio_stat;
	cur = cur->next;
	cur->dev = strndup(&(device_name[0]), text_buffer_size.get(*state));
	cur->last = UINT_MAX;
	cur->last_read = UINT_MAX;
	cur->last_write = UINT_MAX;

	return cur;
}

void parse_diskio_arg(struct text_object *obj, const char *arg)
{
	obj->data.opaque = prepare_diskio_stat(arg);
}

/* dir indicates the direction:
 * -1: read
 *  0: read + write
 *  1: write
 */
static void print_diskio_dir(struct text_object *obj, int dir, char *p, int p_max_size)
{
	struct diskio_stat *diskio = (struct diskio_stat *)obj->data.opaque;
	double val;

	if (!diskio)
		return;

	if (dir < 0)
		val = diskio->current_read;
	else if (dir == 0)
		val = diskio->current;
	else
		val = diskio->current_write;

	/* TODO: move this correction from kB to kB/s elsewhere
	 * (or get rid of it??) */
	human_readable((val / active_update_interval()) * 1024LL, p, p_max_size);
}

void print_diskio(struct text_object *obj, char *p, int p_max_size)
{
	print_diskio_dir(obj, 0, p, p_max_size);
}

void print_diskio_read(struct text_object *obj, char *p, int p_max_size)
{
	print_diskio_dir(obj, -1, p, p_max_size);
}

void print_diskio_write(struct text_object *obj, char *p, int p_max_size)
{
	print_diskio_dir(obj, 1, p, p_max_size);
}

#ifdef BUILD_X11
void parse_diskiograph_arg(struct text_object *obj, const char *arg)
{
	char *buf = 0;
	buf = scan_graph(obj, arg, 0);

	obj->data.opaque = prepare_diskio_stat(dev_name(buf));
	free_and_zero(buf);
}

double diskiographval(struct text_object *obj)
{
	struct diskio_stat *diskio = (struct diskio_stat *)obj->data.opaque;

	return (diskio ? diskio->current : 0);
}

double diskiographval_read(struct text_object *obj)
{
	struct diskio_stat *diskio = (struct diskio_stat *)obj->data.opaque;

	return (diskio ? diskio->current_read : 0);
}

double diskiographval_write(struct text_object *obj)
{
	struct diskio_stat *diskio = (struct diskio_stat *)obj->data.opaque;

	return (diskio ? diskio->current_write : 0);
}
#endif /* BUILD_X11 */

void update_diskio_values(struct diskio_stat *ds,
		unsigned int reads, unsigned int writes)
{
	int i;
	double sum=0, sum_r=0, sum_w=0;

	if (reads < ds->last_read || writes < ds->last_write) {
		/* counter overflow or reset - rebase to sane values */
		ds->last = reads+writes;
		ds->last_read = reads;
		ds->last_write = writes;
	}
	/* since the values in /proc/diskstats are absolute, we have to subtract
	 * our last reading. The numbers stand for "sectors read", and we therefore
	 * have to divide by two to get KB */
	ds->sample_read[0] = (reads - ds->last_read) / 2;
	ds->sample_write[0] = (writes - ds->last_write) / 2;
	ds->sample[0] = ds->sample_read[0] + ds->sample_write[0];

	/* compute averages */
	int samples = diskio_avg_samples.get(*state);
	for (i = 0; i < samples; i++) {
		sum += ds->sample[i];
		sum_r += ds->sample_read[i];
		sum_w += ds->sample_write[i];
	}
	ds->current = sum / (double) samples;
	ds->current_read = sum_r / (double) samples;
	ds->current_write = sum_w / (double) samples;

	/* shift sample history */
	for (i = samples-1; i > 0; i--) {
		ds->sample[i] = ds->sample[i-1];
		ds->sample_read[i] = ds->sample_read[i-1];
		ds->sample_write[i] = ds->sample_write[i-1];
	}

	/* save last */
	ds->last_read = reads;
	ds->last_write = writes;
	ds->last = ds->last_read + ds->last_write;
}

