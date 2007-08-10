/*
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005 Adi Zaimi, Dan Piponi <dan@tanelorn.demon.co.uk>,
 *                    Dave Clark <clarkd@skynet.ca>
 * Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al. (see AUTHORS)
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
 *  $Id$
 */

/*
 * Ensure there's an operating system defined. There is *no* default
 * because every OS has it's own way of revealing CPU/memory usage.
 * compile with gcc -DOS ...
 */

/******************************************/
/* Includes                               */
/******************************************/

#include "conky.h"
#define CPU_THRESHHOLD   0	/* threshhold for the cpu diff to appear */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <regex.h>

/******************************************/
/* Defines                                */
/******************************************/


/*
 * XXX: I shouldn't really use this BUFFER_LEN variable but scanf is so
 * lame and it'll take me a while to write a replacement.
 */
#define BUFFER_LEN 1024

#define PROCFS_TEMPLATE "/proc/%d/stat"
#define PROCFS_TEMPLATE_MEM "/proc/%d/statm"
#define PROCFS_CMDLINE_TEMPLATE "/proc/%d/cmdline"
#define MAX_SP 10  //number of elements to sort


/******************************************/
/* Globals                                */
/******************************************/

/******************************************/
/* Process class                          */
/******************************************/

struct sorted_process {
	struct sorted_process *greater;
	struct sorted_process *less;	
	struct process *proc;
}; 

/*
 * Pointer to head of process list
 */
void process_find_top(struct process **, struct process **);
