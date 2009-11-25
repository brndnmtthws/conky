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

#include "conky.h"
#include "logging.h"
#include "fs.h"
#include "specials.h"
#include "text_object.h"
#include <ctype.h>
#include <unistd.h>
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

#if !defined(HAVE_STRUCT_STATFS_F_FSTYPENAME) && !defined (__OpenBSD__) && !defined(__FreeBSD__)
#include <mntent.h>
#endif

#define MAX_FS_STATS 64

static struct fs_stat fs_stats_[MAX_FS_STATS];
struct fs_stat *fs_stats = fs_stats_;

static void update_fs_stat(struct fs_stat *fs);

void get_fs_type(const char *path, char *result);

void update_fs_stats(void)
{
	unsigned i;
	static double last_fs_update = 0.0;

	if (current_update_time - last_fs_update < 13)
		return;

	for (i = 0; i < MAX_FS_STATS; ++i) {
		if (fs_stats[i].set) {
			update_fs_stat(&fs_stats[i]);
		}
	}
	last_fs_update = current_update_time;
}

void clear_fs_stats(void)
{
	unsigned i;
	for (i = 0; i < MAX_FS_STATS; ++i) {
		memset(&fs_stats[i], 0, sizeof(struct fs_stat));
	}
}

struct fs_stat *prepare_fs_stat(const char *s)
{
	struct fs_stat *new = 0;
	unsigned i;

	/* lookup existing or get new */
	for (i = 0; i < MAX_FS_STATS; ++i) {
		if (fs_stats[i].set) {
			if (strncmp(fs_stats[i].path, s, DEFAULT_TEXT_BUFFER_SIZE) == 0) {
				return &fs_stats[i];
			}
		} else {
			new = &fs_stats[i];
		}
	}
	/* new path */
	if (!new) {
		NORM_ERR("too many fs stats");
		return 0;
	}
	strncpy(new->path, s, DEFAULT_TEXT_BUFFER_SIZE);
	new->set = 1;
	update_fs_stat(new);
	return new;
}

static void update_fs_stat(struct fs_stat *fs)
{
	struct statfs s;

	if (statfs(fs->path, &s) == 0) {
		fs->size = (long long)s.f_blocks * s.f_bsize;
		/* bfree (root) or bavail (non-roots) ? */
		fs->avail = (long long)s.f_bavail * s.f_bsize;
		fs->free = (long long)s.f_bfree * s.f_bsize;
		get_fs_type(fs->path, fs->type);
	} else {
		fs->size = 0;
		fs->avail = 0;
		fs->free = 0;
		strncpy(fs->type, "unknown", DEFAULT_TEXT_BUFFER_SIZE);
		NORM_ERR("statfs '%s': %s", fs->path, strerror(errno));
	}
}

void get_fs_type(const char *path, char *result)
{

#if defined(HAVE_STRUCT_STATFS_F_FSTYPENAME) || defined(__FreeBSD__) || defined (__OpenBSD__)

	struct statfs s;
	if (statfs(path, &s) == 0) {
		strncpy(result, s.f_fstypename, DEFAULT_TEXT_BUFFER_SIZE);
	} else {
		NORM_ERR("statfs '%s': %s", path, strerror(errno));
	}
	return;

#else				/* HAVE_STRUCT_STATFS_F_FSTYPENAME */

	struct mntent *me;
	FILE *mtab = setmntent("/etc/mtab", "r");
	char *search_path;
	int match;
	char *slash;

	if (mtab == NULL) {
		NORM_ERR("setmntent /etc/mtab: %s", strerror(errno));
		strncpy(result, "unknown", DEFAULT_TEXT_BUFFER_SIZE);
		return;
	}

	me = getmntent(mtab);

	// find our path in the mtab
	search_path = strdup(path);
	do {
		while ((match = strcmp(search_path, me->mnt_dir))
				&& getmntent(mtab));
		if (!match)
			break;
		fseek(mtab, 0, SEEK_SET);
		slash = strrchr(search_path, '/');
		if (slash == NULL)
			CRIT_ERR(NULL, NULL, "invalid path '%s'", path);
		if (strlen(slash) == 1)		/* trailing slash */
			*(slash) = '\0';
		else if (strlen(slash) > 1)
			*(slash + 1) = '\0';
		else
			CRIT_ERR(NULL, NULL, "found a crack in the matrix!");
	} while (strlen(search_path) > 0);
	free(search_path);

	endmntent(mtab);

	if (me && !match) {
		strncpy(result, me->mnt_type, DEFAULT_TEXT_BUFFER_SIZE);
		return;
	}
#endif				/* HAVE_STRUCT_STATFS_F_FSTYPENAME */

	strncpy(result, "unknown", DEFAULT_TEXT_BUFFER_SIZE);

}

void init_fs_bar(struct text_object *obj, const char *arg)
{
	arg = scan_bar(obj, arg);
	if (arg) {
		while (isspace(*arg)) {
			arg++;
		}
		if (*arg == '\0') {
			arg = "/";
		}
	} else {
		arg = "/";
	}
	obj->data.opaque = prepare_fs_stat(arg);
}

static uint8_t do_fs_barval(struct text_object *obj, int be_free_bar)
{
	double val = 1.0;
	struct fs_stat *fs = obj->data.opaque;

	if (!fs)
		return 0;

	if (fs->size)
		val = (double)fs->avail / (double)fs->size;

	if (!be_free_bar)
		val = 1.0 - val;

	return round_to_int(val * 255.0);
}

uint8_t fs_barval(struct text_object *obj)
{
	return do_fs_barval(obj, 0);
}

uint8_t fs_free_barval(struct text_object *obj)
{
	return do_fs_barval(obj, 1);
}

void init_fs(struct text_object *obj, const char *arg)
{
	obj->data.opaque = prepare_fs_stat(arg ? arg : "/");
}

static uint8_t fs_percentage(struct text_object *obj, int be_free)
{
	struct fs_stat *fs = obj->data.opaque;
	int val = 100;

	if (!fs)
		return 0;

	if (fs->size)
		val = fs->avail * 100 / fs->size;

	if (!be_free)
		val = 100 - val;

	return val;
}

uint8_t fs_free_percentage(struct text_object *obj)
{
	return fs_percentage(obj, 1);
}

uint8_t fs_used_percentage(struct text_object *obj)
{
	return fs_percentage(obj, 0);
}

#define HUMAN_PRINT_FS_GENERATOR(name, expr)                           \
void print_fs_##name(struct text_object *obj, char *p, int p_max_size) \
{                                                                      \
	struct fs_stat *fs = obj->data.opaque;                             \
	if (fs)                                                            \
		human_readable(expr, p, p_max_size);                           \
}

HUMAN_PRINT_FS_GENERATOR(free, fs->avail)
HUMAN_PRINT_FS_GENERATOR(size, fs->size)
HUMAN_PRINT_FS_GENERATOR(used, fs->size - fs->free)

void print_fs_type(struct text_object *obj, char *p, int p_max_size)
{
	struct fs_stat *fs = obj->data.opaque;

	if (fs)
		snprintf(p, p_max_size, "%s", fs->type);
}
