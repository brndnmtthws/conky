/*
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
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cctype>
#include <cerrno>
#include "conky.h"
#include "fs.h"
#include "logging.h"
#include "specials.h"
#include "text_object.h"

#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif

#if defined(__sun)
#include <sys/statvfs.h>
#include <sys/types.h>
#endif

#if defined(__FreeBSD__)
#include "freebsd.h"
#elif defined(__DragonFly__)
#include "dragonfly.h"
#elif defined(__HAIKU__)
#include "haiku.h"
#elif defined(__APPLE__) && defined(__MACH__)
#include "darwin.h"
#endif

#if !defined(HAVE_STRUCT_STATFS_F_FSTYPENAME) && !defined(__OpenBSD__) &&  \
    !defined(__FreeBSD__) && !defined(__DragonFly__) && !defined(__sun) && \
    !defined(__HAIKU__) && !(defined(__APPLE__) && defined(__MACH__))
#include <mntent.h>
#endif

#define MAX_FS_STATS 64

static struct fs_stat fs_stats_[MAX_FS_STATS];
struct fs_stat *fs_stats = fs_stats_;

static void update_fs_stat(struct fs_stat *fs);

void get_fs_type(const char *path, char *result);

int update_fs_stats() {
  unsigned i;
  static double last_fs_update = 0.0;

  if (current_update_time - last_fs_update < 13) { return 0; }

  for (i = 0; i < MAX_FS_STATS; ++i) {
    if (fs_stats[i].set != 0) { update_fs_stat(&fs_stats[i]); }
  }
  last_fs_update = current_update_time;
  return 0;
}

void clear_fs_stats() {
  unsigned i;
  for (i = 0; i < MAX_FS_STATS; ++i) {
    memset(&fs_stats[i], 0, sizeof(struct fs_stat));
  }
}

struct fs_stat *prepare_fs_stat(const char *s) {
  struct fs_stat *next = nullptr;
  unsigned i;

  /* lookup existing or get new */
  for (i = 0; i < MAX_FS_STATS; ++i) {
    if (fs_stats[i].set != 0) {
      if (strncmp(fs_stats[i].path, s, DEFAULT_TEXT_BUFFER_SIZE) == 0) {
        return &fs_stats[i];
      }
    } else {
      next = &fs_stats[i];
    }
  }
  /* new path */
  if (next == nullptr) {
    NORM_ERR("too many fs stats");
    return nullptr;
  }
  strncpy(next->path, s, DEFAULT_TEXT_BUFFER_SIZE);
  next->set = 1;
  update_fs_stat(next);
  return next;
}

static void update_fs_stat(struct fs_stat *fs) {
#if defined(__sun)
  struct statvfs s;

  if (statvfs(fs->path, &s) == 0) {
    fs->size = (long long)s.f_blocks * s.f_frsize;
    fs->avail = (long long)s.f_bavail * s.f_frsize;
    fs->free = (long long)s.f_bfree * s.f_frsize;
    (void)strncpy(fs->type, s.f_basetype, sizeof(fs->type));
#else
  struct statfs64 s {};

  if (statfs64(fs->path, &s) == 0) {
    fs->size = static_cast<long long>(s.f_blocks) * s.f_bsize;
    /* bfree (root) or bavail (non-roots) ? */
    fs->avail = static_cast<long long>(s.f_bavail) * s.f_bsize;
    fs->free = static_cast<long long>(s.f_bfree) * s.f_bsize;
    get_fs_type(fs->path, fs->type);
#endif
  } else {
    NORM_ERR("statfs64 '%s': %s", fs->path, strerror(errno));
    fs->size = 0;
    fs->avail = 0;
    fs->free = 0;
    strncpy(fs->type, "unknown", DEFAULT_TEXT_BUFFER_SIZE);
  }
}

void get_fs_type(const char *path, char *result) {
#if defined(HAVE_STRUCT_STATFS_F_FSTYPENAME) || defined(__FreeBSD__) ||     \
    defined(__OpenBSD__) || defined(__DragonFly__) || defined(__HAIKU__) || \
    (defined(__APPLE__) && defined(__MACH__))

  struct statfs64 s {};
  if (statfs64(path, &s) == 0) {
    strncpy(result, s.f_fstypename, DEFAULT_TEXT_BUFFER_SIZE);
  } else {
    NORM_ERR("statfs64 '%s': %s", path, strerror(errno));
  }
  return;
#elif defined(__sun)
  assert(0); /* not used - see update_fs_stat() */
#else  /* HAVE_STRUCT_STATFS_F_FSTYPENAME */

  struct mntent *me;
  FILE *mtab = setmntent("/proc/mounts", "r");
  char *search_path;
  int match;
  char *slash;

  if (mtab == nullptr) {
    NORM_ERR("setmntent /proc/mounts: %s", strerror(errno));
    strncpy(result, "unknown", DEFAULT_TEXT_BUFFER_SIZE);
    return;
  }

  me = getmntent(mtab);

  // find our path in the mtab
  search_path = strdup(path);
  do {
    while ((match = strcmp(search_path, me->mnt_dir)) && getmntent(mtab))
      ;
    if (!match) break;
    fseek(mtab, 0, SEEK_SET);
    slash = strrchr(search_path, '/');
    if (slash == nullptr) {
      CRIT_ERR(NULL, NULL, "invalid path '%s'", path);
    } else {
      if (strlen(slash) == 1) /* trailing slash */
        *(slash) = '\0';
      else if (strlen(slash) > 1)
        *(slash + 1) = '\0';
      else
        CRIT_ERR(nullptr, NULL, "found a crack in the matrix!");
    }
  } while (strlen(search_path) > 0);
  free(search_path);

  endmntent(mtab);

  if (me && !match) {
    strncpy(result, me->mnt_type, DEFAULT_TEXT_BUFFER_SIZE);
    return;
  }
#endif /* HAVE_STRUCT_STATFS_F_FSTYPENAME */

  strncpy(result, "unknown", DEFAULT_TEXT_BUFFER_SIZE);
}

void init_fs_bar(struct text_object *obj, const char *arg) {
  arg = scan_bar(obj, arg, 1);
  if (arg != nullptr) {
    while (isspace((unsigned char)*arg) != 0) { arg++; }
    if (*arg == '\0') { arg = "/"; }
  } else {
    arg = "/";
  }
  obj->data.opaque = prepare_fs_stat(arg);
}

static double get_fs_perc(struct text_object *obj, bool get_free) {
  auto *fs = static_cast<struct fs_stat *>(obj->data.opaque);
  double ret = 0.0;

  if ((fs != nullptr) && (fs->size != 0)) {
    if (get_free) {
      ret = fs->avail;
    } else {
      ret = fs->size - fs->free;
    }
    ret /= fs->size;
  }

  return ret;
}

double fs_barval(struct text_object *obj) { return get_fs_perc(obj, false); }

double fs_free_barval(struct text_object *obj) {
  return get_fs_perc(obj, true);
}

void init_fs(struct text_object *obj, const char *arg) {
  obj->data.opaque = prepare_fs_stat(arg != nullptr ? arg : "/");
}

uint8_t fs_free_percentage(struct text_object *obj) {
  return get_fs_perc(obj, true) * 100;
}

uint8_t fs_used_percentage(struct text_object *obj) {
  return get_fs_perc(obj, false) * 100;
}

#define HUMAN_PRINT_FS_GENERATOR(name, expr)                               \
  void print_fs_##name(struct text_object *obj, char *p,                   \
		                                unsigned int p_max_size) { \
    struct fs_stat *fs = (struct fs_stat *)obj->data.opaque;               \
    if (fs) human_readable(expr, p, p_max_size);                           \
  }

HUMAN_PRINT_FS_GENERATOR(free, fs->avail)
HUMAN_PRINT_FS_GENERATOR(size, fs->size)
HUMAN_PRINT_FS_GENERATOR(used, fs->size - fs->free)

void print_fs_type(struct text_object *obj, char *p, unsigned int p_max_size) {
  auto *fs = static_cast<struct fs_stat *>(obj->data.opaque);

  if (fs != nullptr) { snprintf(p, p_max_size, "%s", fs->type); }
}
