/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
 *      (see AUTHORS)
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

#include "bsdcommon.h"
#include "logging.h"

#include <kvm.h>

#include <sys/param.h>
#include <sys/sysctl.h>

static kvm_t *kd = nullptr;
static bool kvm_initialised = false;
static bool cpu_initialised = false;

static struct bsdcommon::cpu_load *cpu_loads = nullptr;

bool bsdcommon::init_kvm()
{
  if (kvm_initialised) {
      return true;
  }

  kd = kvm_open(nullptr, nullptr, nullptr, KVM_NO_FILES, nullptr);
  if (kd == nullptr) {
    NORM_ERR("opening kvm");
    return false;
  }

  kvm_initialised = true;
  return false;
}

void bsdcommon::deinit_kvm() {
  if (!kvm_initialised || kd == nullptr) {
    return;
  }

  kvm_close(kd);
}

void bsdcommon::get_cpu_count(float **cpu_usage, unsigned int *cpu_count)
{
  int ncpu = 1;
  int mib[2] = {CTL_HW, HW_NCPU};
  size_t len = sizeof(ncpu);

  if (sysctl(mib, 2, &ncpu, &len, nullptr, 0) != 0) {
    NORM_ERR("error getting kern.ncpu, defaulting to 1");
    ncpu = 1;
  }

  if (*cpu_count != ncpu) {
    *cpu_count = ncpu;

    if (*cpu_usage != nullptr) {
      free(*cpu_usage);
      *cpu_usage = nullptr;
    }

    if (cpu_loads != nullptr) {
      free(cpu_loads);
      cpu_loads = nullptr;
    }
  }

  if (*cpu_usage == nullptr) {
    // [0] - Total CPU
    // [1, 2, ... ] - CPU0, CPU1, ...
    *cpu_usage = (float*)calloc(ncpu + 1, sizeof(float));
    if (*cpu_usage == nullptr) {
      CRIT_ERR("calloc of cpu_usage");
    }
  }

  if (cpu_loads == nullptr) {
    cpu_loads = (struct cpu_load*)calloc(ncpu + 1, sizeof(struct cpu_load));
    if (cpu_loads == nullptr) {
      CRIT_ERR("calloc of cpu_loads");
    }
  }
}

void bsdcommon::update_cpu_usage(float **cpu_usage, unsigned int *cpu_count)
{
  uint64_t cp_time0[CPUSTATES];
  int mib_cpu0[2] = {CTL_KERN, KERN_CP_TIME};
  uint64_t cp_timen[CPUSTATES];
  int mib_cpun[3] = {CTL_KERN, KERN_CP_TIME, 0};
  size_t size = 0;
  u_int64_t used = 0, total = 0;

  if (!cpu_initialised) {
    get_cpu_count(cpu_usage, cpu_count);
    cpu_initialised = true;
  }

  size = sizeof(cp_time0);
  if (sysctl(mib_cpu0, 2, &cp_time0, &size, nullptr, 0) != 0) {
      NORM_ERR("unable to get kern.cp_time for cpu0");
      return;
  }

  for (int j = 0; j < CPUSTATES; ++j) {
    total += cp_time0[j];
  }
  used = total - cp_time0[CP_IDLE];

  if ((total - cpu_loads[0].old_total) != 0) {
    const float diff_used = (float)(used - cpu_loads[0].old_used);
    const float diff_total = (float)(total - cpu_loads[0].old_total);
    (*cpu_usage)[0] = diff_used / diff_total;
  } else {
    (*cpu_usage)[0] = 0;
  }
  cpu_loads[0].old_used = used;
  cpu_loads[0].old_total = total;

  for (int i = 0; i < *cpu_count; ++i) {
    mib_cpun[2] = i;
    size = sizeof(cp_timen);
    if (sysctl(mib_cpun, 3, &cp_timen, &size, nullptr, 0) != 0) {
      NORM_ERR("unable to get kern.cp_time for cpu%d", i);
      return;
    }

    total = 0;
    used = 0;
    for (int j = 0; j < CPUSTATES; ++j) {
      total += cp_timen[j];
    }
    used = total - cp_timen[CP_IDLE];

    const int n = i + 1; // [0] is the total CPU, must shift by 1
    if ((total - cpu_loads[n].old_total) != 0) {
      const float diff_used = (float)(used - cpu_loads[n].old_used);
      const float diff_total = (float)(total - cpu_loads[n].old_total);
      (*cpu_usage)[n] = diff_used / diff_total;
    } else {
      (*cpu_usage)[n] = 0;
    }

    cpu_loads[n].old_used = used;
    cpu_loads[n].old_total = total;
  }
}

BSD_COMMON_PROC_STRUCT* bsdcommon::get_processes(short unsigned int *procs)
{
  if (!init_kvm()) {
    return nullptr;
  }

  int n_processes = 0;
  BSD_COMMON_PROC_STRUCT *ki = kvm_getproc2(kd, KERN_PROC_ALL, 0,
                                            sizeof(BSD_COMMON_PROC_STRUCT),
                                            &n_processes);
  if (ki == nullptr) {
    NORM_ERR("kvm_getproc2() failed");
    return nullptr;
  }

  *procs = n_processes;
  return ki;
}

static bool is_process_running(BSD_COMMON_PROC_STRUCT *p)
{
  return p->p_stat == LSRUN || p->p_stat == LSIDL || p->p_stat == LSONPROC;
}

void bsdcommon::get_number_of_running_processes(short unsigned int *run_procs)
{
  if (!init_kvm()) {
    return;
  }

  short unsigned int nprocs = 0;
  BSD_COMMON_PROC_STRUCT* ps = get_processes(&nprocs);
  if (ps == nullptr) {
    return;
  }

  short unsigned int ctr = 0;
  for (int i = 0; i < nprocs; ++i) {
    if (is_process_running(&ps[i])) {
      ++ctr;
    }
  }

  *run_procs = ctr;
}
