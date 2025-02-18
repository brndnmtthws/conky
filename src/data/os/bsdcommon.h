/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
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
 */

/*
 * Shared or very similar code across BSDs.
 */

#ifndef BSDCOMMON_H_
#define BSDCOMMON_H_

#define BSD_COMMON

#if defined(__NetBSD__)
  #include <sys/sysctl.h>
  #define BSD_COMMON_PROC_STRUCT struct kinfo_proc2
#elif defined(__OpenBSD__)
  #include <sys/types.h>
  #include <sys/sysctl.h>
  #define BSD_COMMON_PROC_STRUCT struct kinfo_proc
#else
  #error Not supported BSD system
#endif

#include <stdint.h>

#include "../../conky.h"

namespace bsdcommon {
  struct cpu_load {
    uint64_t old_used;
    uint64_t old_total;
  };

  bool init_kvm();
  void deinit_kvm();

  void get_cpu_count(float **cpu_usage, unsigned int *cpu_count);
  void update_cpu_usage(float **cpu_usage, unsigned int *cpu_count);

  BSD_COMMON_PROC_STRUCT* get_processes(short unsigned int *procs);

  void get_number_of_running_processes(short unsigned int *run_procs);
  void update_top_info();
  bool is_conky_already_running();

  void update_meminfo(struct information &info);
}

#endif /*BSDCOMMON_H_*/
