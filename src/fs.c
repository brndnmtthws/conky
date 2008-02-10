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
 * Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al. (see AUTHORS)
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

/* linux */
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif

/* freebsd && netbsd */
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#define MAX_FS_STATS 64

static struct fs_stat fs_stats_[MAX_FS_STATS];
struct fs_stat *fs_stats = fs_stats_;

static void update_fs_stat(struct fs_stat* fs);

void update_fs_stats()
{
	unsigned i;
	for(i=0; i<MAX_FS_STATS; ++i)
		if(fs_stats[i].path)
			update_fs_stat(&fs_stats[i]);
}

void clear_fs_stats()
{
	unsigned i;
	for(i=0; i<MAX_FS_STATS; ++i)
		if(fs_stats[i].path) {
			free(fs_stats[i].path);
			fs_stats[i].path = 0;
		}
}

struct fs_stat *prepare_fs_stat(const char *s)
{
	struct fs_stat* new = 0;
	unsigned i;
	/* lookup existing or get new */
	for(i=0; i<MAX_FS_STATS; ++i) {
		if(fs_stats[i].path) {
			if(strcmp(fs_stats[i].path, s) == 0)
				return &fs_stats[i];
		} else
			new = &fs_stats[i];
	}
	/* new path */
	if(!new) {
		ERR("too many fs stats");
		return 0;
	}
	new->path = strdup(s);
	update_fs_stat(new);
	return new;
}

static
void update_fs_stat(struct fs_stat* fs)
{
	struct statfs s;
	if(statfs(fs->path, &s) == 0) {
		fs->size = (long long) s.f_blocks * s.f_bsize;
		/* bfree (root) or bavail (non-roots) ? */
		fs->avail = (long long) s.f_bavail* s.f_bsize;
		fs->free = (long long) s.f_bfree * s.f_bsize;
	} else {
		fs->size = 0;
		fs->avail = 0;
		fs->free = 0;
		ERR("statfs '%s': %s", fs->path, strerror(errno));
	}
}
