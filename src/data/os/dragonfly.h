/* */

#ifndef DRAGONFLY_H_
#define DRAGONFLY_H_

#include <fcntl.h>
#include <kvm.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include "common.h"
#if (defined(i386) || defined(__i386__))
#include <machine/apm_bios.h>
#endif /* i386 || __i386__ */

int get_entropy_avail(unsigned int *);
int get_entropy_poolsize(unsigned int *);

#endif /*DRAGONFLY_H_*/
