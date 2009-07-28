/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef _FS_H
#define _FS_H

#include "conky.h"	/* DEFAULT_TEXT_BUFFER_SIZE */

/* needed here and by fs.c */
struct fs_stat {
	char path[DEFAULT_TEXT_BUFFER_SIZE];
	char type[DEFAULT_TEXT_BUFFER_SIZE];
	long long size;
	long long avail;
	long long free;
	char set;
};

void update_fs_stats(void);
struct fs_stat *prepare_fs_stat(const char *path);
void clear_fs_stats(void);

#endif /* _FS_H */
