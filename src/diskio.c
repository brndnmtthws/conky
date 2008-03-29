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
 * Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al.
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

#include <limits.h>
#include "conky.h"

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
	new->dev = strdup(s);
	new->current = 0;
	new->current_read = 0;
	new ->current_write = 0;
	new->last = UINT_MAX;
	new->last_read = UINT_MAX;
	new->last_write = UINT_MAX;
	return new;
}
