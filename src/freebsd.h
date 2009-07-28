/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*- */

#ifndef FREEBSD_H_
#define FREEBSD_H_

#include "common.h"
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/ucred.h>
#include <fcntl.h>
#include <kvm.h>
#if (defined(i386) || defined(__i386__))
#include <machine/apm_bios.h>
#endif /* i386 || __i386__ */

kvm_t *kd;

#endif /*FREEBSD_H_*/
