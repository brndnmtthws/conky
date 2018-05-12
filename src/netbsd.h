/* */

#ifndef NETBSD_H_
#define NETBSD_H_

#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <time.h>
#include <unistd.h>

#include <sys/envsys.h>
#include <sys/sched.h>
#include <sys/socket.h>
#include <sys/swap.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/user.h>

#include <net/if.h>

#include <uvm/uvm_extern.h>

#include <machine/param.h>

#include "common.h"
#include "conky.h"

int get_entropy_avail(unsigned int *);
int get_entropy_poolsize(unsigned int *);

#endif /*NETBSD_H_*/
