/*
 *  This file is part of conky.
 *
 *  conky is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  conky is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with conky.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

//  darwin.h
//  Nickolas Pylarinos

#ifndef DARWIN_H
#define DARWIN_H

#include <sys/param.h>
#include <sys/mount.h>
#include <strings.h>
#include <stdio.h>


#include <mach/vm_statistics.h>
#include <sys/sysctl.h>
/*
 * General sample information.
 *
 * Fields prefix meanings:
 *
 *   b_ : Value for first sample.
 *   p_ : Value for previous sample (same as b_ if p_seq is 0).
 */
typedef struct {
    /*
     * Sample sequence number, incremented for every sample.  The first
     * sample has a sequence number of 1.
     */
    //uint32_t        seq;
    
    /* VM page size. */
    vm_size_t       pagesize;
    
    /* Physical memory size. */
    uint64_t        memsize;
    
    /* VM statistics. */
    vm_statistics64_data_t    vm_stat;
    //vm_statistics64_data_t    b_vm_stat;
    //vm_statistics64_data_t    p_vm_stat;
    
    boolean_t       purgeable_is_valid;
    
    uint64_t        pages_stolen;
} libtop_tsamp_t;

/*
 *  on versions prior to Sierra clock_gettime is not implemented.
 */
#ifndef HAVE_CLOCK_GETTIME

/* only CLOCK_REALTIME and CLOCK_MONOTONIC are emulated */
#ifndef CLOCK_REALTIME
# define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_MONOTONIC
# define CLOCK_MONOTONIC 1
#endif

int clock_gettime(int clock_id, struct timespec *ts);
#endif /* ifndef HAVE_CLOCK_GETTIME */

int update_running_threads(void);

int get_entropy_avail(unsigned int *);
int get_entropy_poolsize(unsigned int *);

/* System Integrity Protection */
int get_sip_status(void);
void print_sip_status(struct text_object *obj, char *p, int p_max_size);

#endif /*DARWIN_H*/
