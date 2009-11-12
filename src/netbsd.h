/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef NETBSD_H_
#define NETBSD_H_

#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <err.h>
#include <limits.h>
#include <paths.h>
#include <kvm.h>
#include <nlist.h>

#include <sys/time.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/socket.h>
#include <sys/swap.h>
#include <sys/sched.h>
#include <sys/envsys.h>

#include <net/if.h>

#include <uvm/uvm_extern.h>

#include <machine/param.h>

#include "conky.h"
#include "common.h"

int get_entropy_avail(unsigned int *);
int get_entropy_poolsize(unsigned int *);

#endif /*NETBSD_H_*/
