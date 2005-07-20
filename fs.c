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

/* TODO: benchmark which is faster, fstatvfs() or pre-opened fd and
 * statvfs() (fstatvfs() would handle mounts I think...) */

static struct fs_stat fs_stats_[64];
struct fs_stat *fs_stats = fs_stats_;

void update_fs_stats()
{
	unsigned int i;
	struct statfs s;
	for (i = 0; i < 16; i++) {
		if (fs_stats[i].fd <= 0)
			break;

		fstatfs(fs_stats[i].fd, &s);

		fs_stats[i].size = (long long) s.f_blocks * s.f_bsize;
		/* bfree (root) or bavail (non-roots) ? */
		fs_stats[i].avail = (long long) s.f_bavail * s.f_bsize;
	}
}

void clear_fs_stats()
{
	unsigned int i;
	for (i = 0; i < 16; i++) {
		if (fs_stats[i].fd) {
			close(fs_stats[i].fd);
			fs_stats[i].fd = -1;
		}

		if (fs_stats[i].path != NULL) {
			free(fs_stats[i].path);
			fs_stats[i].path = NULL;
		}
	}
}

struct fs_stat *prepare_fs_stat(const char *s)
{
	unsigned int i;

	for (i = 0; i < 16; i++) {
		struct fs_stat *fs = &fs_stats[i];

		if (fs->path && strcmp(fs->path, s) == 0)
			return fs;

		if (fs->fd <= 0) {
			/* when compiled with icc, it crashes when leaving function and open()
			 * is used, I don't know why */

			/* this icc workaround didn't seem to work */
#if 0
			{
				FILE *fp = fopen(s, "r");
				if (fp)
					fs->fd = fileno(fp);
				else
					fs->fd = -1;
			}
#endif

			fs->fd = open(s, O_RDONLY);

			if (fs->fd <= 0) {	/* 0 isn't error but actually it is :) */
				ERR("open '%s': %s", s, strerror(errno));
				return 0;
			}

			fs->path = strdup(s);
			update_fs_stats();
			return fs;
		}
	}

	ERR("too many fs stats");
	return 0;
}
