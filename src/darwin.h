/*
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2018-2019, npyl <n.pylarinos@hotmail.com>
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

#ifndef DARWIN_H
#define DARWIN_H

#include <stdio.h>
#include <strings.h>
#include <sys/mount.h>
#include <sys/param.h>

/*
 *  on versions prior to Sierra clock_gettime is not implemented.
 */
#ifndef HAVE_CLOCK_GETTIME

/* only CLOCK_REALTIME and CLOCK_MONOTONIC are emulated */
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

int clock_gettime(int clock_id, struct timespec *ts);
#endif /* ifndef HAVE_CLOCK_GETTIME */

int update_running_threads(void);

int get_entropy_avail(const unsigned int *);
int get_entropy_poolsize(const unsigned int *);

/* System Integrity Protection */
int get_sip_status(void);
void print_sip_status(struct text_object *obj, char *p, unsigned int p_max_size);

void deallocate_cpu_sample(struct text_object *obj);

#endif /*DARWIN_H*/
