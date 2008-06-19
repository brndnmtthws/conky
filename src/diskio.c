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
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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
 *  $Id$
 */

#include "conky.h"
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

static struct diskio_stat diskio_stats_[MAX_DISKIO_STATS];
struct diskio_stat *diskio_stats = diskio_stats_;

void clear_diskio_stats(void)
{
	unsigned i;
	for(i = 0; i < MAX_DISKIO_STATS; i++) {
		if (diskio_stats[i].dev) {
			free(diskio_stats[i].dev);
			diskio_stats[i].dev = 0;
		}
	}
}

struct diskio_stat *prepare_diskio_stat(const char *s)
{
	struct diskio_stat *new = 0;
	unsigned i;
	FILE *fp;
	int found = 0;
	char device[text_buffer_size], fbuf[text_buffer_size];
	static int rep = 0;
	/* lookup existing or get new */
	for (i = 0; i < MAX_DISKIO_STATS; i++) {
		if (diskio_stats[i].dev) {
			if (strcmp(diskio_stats[i].dev, s) == 0) {
				return &diskio_stats[i];
			}
		} else {
			new = &diskio_stats[i];
			break;
		}
	}
	/* new dev */
	if (!new) {
		ERR("too many diskio stats");
		return 0;
	}
	if (new->dev) {
		free(new->dev);
		new->dev = 0;
	}
	if (strncmp(s, "/dev/", 5) == 0) {
		// supplied a /dev/device arg, so cut off the /dev part
		new->dev = strndup(s + 5, text_buffer_size);
	} else {
		new->dev = strndup(s, text_buffer_size);
	}
	/*
	 * check that device actually exists
	 */

	if (!(fp = open_file("/proc/diskstats", &rep))) {
		ERR("cannot read from /proc/diskstats");
		return 0;
	}

	while (!feof(fp)) {
		fgets(fbuf, text_buffer_size, fp);
		if (sscanf(fbuf, "%*u %*u %255s %*u %*u %*u %*u %*u %*u %*u", device)) {
			// check for device match
			if (strncmp(new->dev, device, 256) == 0) {
				found = 1;
				break;
			}
		}
	}
	fclose(fp);
	fp = 0;
	if (!found) {
		ERR("diskio device '%s' does not exist", s);
		return 0;
	}
	new->current = 0;
	new->current_read = 0;
	new ->current_write = 0;
	new->last = UINT_MAX;
	new->last_read = UINT_MAX;
	new->last_write = UINT_MAX;
	return new;
}

void update_diskio(void)
{
	static unsigned int last = UINT_MAX;
	static unsigned int last_read = UINT_MAX;
	static unsigned int last_write = UINT_MAX;
	FILE *fp;
	static int rep = 0;

	char buf[512], devbuf[64];
	int i;
	unsigned int major, minor;
	unsigned int current = 0;
	unsigned int current_read = 0;
	unsigned int current_write = 0;
	unsigned int reads, writes = 0;
	int col_count = 0;
	int tot, tot_read, tot_write;

	if (!(fp = open_file("/proc/diskstats", &rep))) {
		info.diskio_value = 0;
		return;
	}

	/* read reads and writes from all disks (minor = 0), including cd-roms
	 * and floppies, and sum them up */
	while (!feof(fp)) {
		fgets(buf, 512, fp);
		col_count = sscanf(buf, "%u %u %s %*u %*u %u %*u %*u %*u %u", &major,
			&minor, devbuf, &reads, &writes);
		/* ignore subdevices (they have only 3 matching entries in their line)
		 * and virtual devices (LVM, network block devices, RAM disks, Loopback)
		 *
		 * XXX: ignore devices which are part of a SW RAID (MD_MAJOR) */
		if (col_count == 5 && major != LVM_BLK_MAJOR && major != NBD_MAJOR
				&& major != RAMDISK_MAJOR && major != LOOP_MAJOR) {
			current += reads + writes;
			current_read += reads;
			current_write += writes;
		} else {
			col_count = sscanf(buf, "%u %u %s %*u %u %*u %u",
				&major, &minor, devbuf, &reads, &writes);
			if (col_count != 5) {
				continue;
			}
		}
		for (i = 0; i < MAX_DISKIO_STATS; i++) {
			if (diskio_stats[i].dev &&
					strncmp(devbuf, diskio_stats[i].dev, text_buffer_size) == 0) {
				diskio_stats[i].current =
					(reads + writes - diskio_stats[i].last) / 2;
				diskio_stats[i].current_read =
					(reads - diskio_stats[i].last_read) / 2;
				diskio_stats[i].current_write =
					(writes - diskio_stats[i].last_write) / 2;
				if (reads + writes < diskio_stats[i].last) {
					diskio_stats[i].current = 0;
				}
				if (reads < diskio_stats[i].last_read) {
					diskio_stats[i].current_read = 0;
					diskio_stats[i].current = diskio_stats[i].current_write;
				}
				if (writes < diskio_stats[i].last_write) {
					diskio_stats[i].current_write = 0;
					diskio_stats[i].current = diskio_stats[i].current_read;
				}
				diskio_stats[i].last = reads + writes;
				diskio_stats[i].last_read = reads;
				diskio_stats[i].last_write = writes;
			}
		}
	}

	/* since the values in /proc/diststats are absolute, we have to substract
	 * our last reading. The numbers stand for "sectors read", and we therefore
	 * have to divide by two to get KB */
	tot = ((double) (current - last) / 2);
	tot_read = ((double) (current_read - last_read) / 2);
	tot_write = ((double) (current_write - last_write) / 2);

	if (last_read > current_read) {
		tot_read = 0;
	}
	if (last_write > current_write) {
		tot_write = 0;
	}

	if (last > current) {
		/* we hit this either if it's the very first time we run this, or
		 * when /proc/diskstats overflows; while 0 is not correct, it's at
		 * least not way off */
		tot = 0;
	}
	last = current;
	last_read = current_read;
	last_write = current_write;

	info.diskio_value = tot;
	info.diskio_read_value = tot_read;
	info.diskio_write_value = tot_write;

	fclose(fp);
}

