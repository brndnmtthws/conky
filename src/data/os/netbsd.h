/* */

#ifndef NETBSD_H_
#define NETBSD_H_

#include <kvm.h>
#include <limits.h>
#include <strings.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/statvfs.h>

#include "../../common.h"
#include "../../conky.h"

#include "bsdcommon.h"

bool is_conky_already_running();

#endif /*NETBSD_H_*/
