/*
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
#include "logging.h"
#include "diskio.h"
#include "common.h"
#include <stdlib.h>
#include <limits.h>
/* The following ifdefs were adapted from gkrellm */
#include <linux/major.h>

#if !defined(MD_MAJOR)
#define MD_MAJOR 9
#endif

#if !defined(LVM_BLK_MAJOR)
#define LVM_BLK_MAJOR 58
#endif

#if !defined(NBD_MAJOR)
#define NBD_MAJOR 43
#endif

/* this is the root of all per disk stats,
 * also containing the totals. */
static struct diskio_stat stats = {
	.next = NULL,
	.sample = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	.sample_read = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	.sample_write = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	.current = 0,
	.current_read = 0,
	.current_write = 0,
	.last = UINT_MAX,
	.last_read = UINT_MAX,
	.last_write = UINT_MAX,
};

void clear_diskio_stats(void)
{
	struct diskio_stat *cur;
	while (stats.next) {
		cur = stats.next;
		stats.next = stats.next->next;
		free(cur);
	}
}

struct diskio_stat *prepare_diskio_stat(const char *s)
{
	struct diskio_stat *cur = &stats;

	if (!s)
		return &stats;

	/* lookup existing */
	while (cur->next) {
		cur = cur->next;
		if (!strcmp(cur->dev, s))
			return cur;
	}

	/* no existing found, make a new one */
	cur->next = malloc(sizeof(struct diskio_stat));
	cur = cur->next;
	memset(cur, 0, sizeof(struct diskio_stat));
	cur->dev = strndup(s, text_buffer_size);
	cur->last = UINT_MAX;
	cur->last_read = UINT_MAX;
	cur->last_write = UINT_MAX;
	return cur;
}

static void update_diskio_values(struct diskio_stat *ds,
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
	/* since the values in /proc/diskstats are absolute, we have to substract
	 * our last reading. The numbers stand for "sectors read", and we therefore
	 * have to divide by two to get KB */
	ds->sample_read[0] = (reads - ds->last_read) / 2;
	ds->sample_write[0] = (writes - ds->last_write) / 2;
	ds->sample[0] = ds->sample_read[0] + ds->sample_write[0];

	/* compute averages */
	for (i = 0; i < (signed) info.diskio_avg_samples; i++) {
		sum += ds->sample[i];
		sum_r += ds->sample_read[i];
		sum_w += ds->sample_write[i];
	}
	ds->current = sum / (double) info.diskio_avg_samples;
	ds->current_read = sum_r / (double) info.diskio_avg_samples;
	ds->current_write = sum_w / (double) info.diskio_avg_samples;

	/* shift sample history */
	for (i = info.diskio_avg_samples-1; i > 0; i--) {
		ds->sample[i] = ds->sample[i-1];
		ds->sample_read[i] = ds->sample_read[i-1];
		ds->sample_write[i] = ds->sample_write[i-1];
	}

	/* save last */
	ds->last_read = reads;
	ds->last_write = writes;
	ds->last = ds->last_read + ds->last_write;
}

void update_diskio(void)
{
	FILE *fp;
	static int rep = 0;

	struct diskio_stat *cur;
	char buf[512], devbuf[64];
	unsigned int major, minor;
	unsigned int reads, writes;
	unsigned int total_reads=0, total_writes=0;
	int col_count = 0;

	stats.current = 0;
	stats.current_read = 0;
	stats.current_write = 0;

	if (!(fp = open_file("/proc/diskstats", &rep))) {
		return;
	}

	/* read reads and writes from all disks (minor = 0), including cd-roms
	 * and floppies, and sum them up */
	while (fgets(buf, 512, fp)) {
		col_count = sscanf(buf, "%u %u %s %*u %*u %u %*u %*u %*u %u", &major,
			&minor, devbuf, &reads, &writes);
		/* ignore subdevices (they have only 3 matching entries in their line)
		 * and virtual devices (LVM, network block devices, RAM disks, Loopback)
		 *
		 * XXX: ignore devices which are part of a SW RAID (MD_MAJOR) */
		if (col_count == 5 && major != LVM_BLK_MAJOR && major != NBD_MAJOR
				&& major != RAMDISK_MAJOR && major != LOOP_MAJOR && minor==0) {
			total_reads += reads;
			total_writes += writes;
		} else {
			col_count = sscanf(buf, "%u %u %s %*u %u %*u %u",
				&major, &minor, devbuf, &reads, &writes);
			if (col_count != 5) {
				continue;
			}
		}
		cur = stats.next;
		while (cur && strcmp(devbuf, cur->dev))
			cur = cur->next;

		if (cur)
			update_diskio_values(cur, reads, writes);
	}
	update_diskio_values(&stats, total_reads, total_writes);
	fclose(fp);
}

