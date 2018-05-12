/* */

#ifndef HAIKU_H_
#define HAIKU_H_

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include <net/if.h>

#include <kernel/fs_info.h>

#include "common.h"
#include "conky.h"

int get_entropy_avail(unsigned int *);
int get_entropy_poolsize(unsigned int *);

/* let's just mimic statfs64 */

struct statfs : public fs_info {};

inline int statfs(const char *path, struct statfs *buf) {
  return fs_stat_dev(dev_for_path(path), buf);
}

#define f_blocks total_blocks
#define f_bsize block_size
#define f_bavail free_blocks
#define f_bfree free_blocks
#define f_fstypename fsh_name

#endif /*HAIKU_H_*/
