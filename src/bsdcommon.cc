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

#include <string.h>

#if defined(__NetBSD__)
  #include <uvm/uvm_param.h>
  #include <uvm/uvm_extern.h>
#endif

#include "top.h"

static kvm_t *kd = nullptr;
static char kvm_errbuf[_POSIX2_LINE_MAX];
static bool kvm_initialised = false;
static bool cpu_initialised = false;

static struct bsdcommon::cpu_load *cpu_loads = nullptr;

bool bsdcommon::init_kvm() {
  if (kvm_initialised) {
      return true;
  }

  kd = kvm_open(nullptr, nullptr, nullptr, KVM_NO_FILES, kvm_errbuf);
  if (kd == nullptr) {
    NORM_ERR("opening kvm :%s", kvm_errbuf);
    return false;
  }

  kvm_initialised = true;
  return true;
}

void bsdcommon::deinit_kvm() {
  if (!kvm_initialised || kd == nullptr) {
    return;
  }

  kvm_close(kd);
}

void bsdcommon::get_cpu_count(float **cpu_usage, unsigned int *cpu_count) {
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

void bsdcommon::update_cpu_usage(float **cpu_usage, unsigned int *cpu_count) {
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

BSD_COMMON_PROC_STRUCT *bsdcommon::get_processes(short unsigned int *procs) {
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

static bool is_process_running(BSD_COMMON_PROC_STRUCT *p) {
#if defined(__NetBSD__)
  return p->p_stat == LSRUN || p->p_stat == LSIDL || p->p_stat == LSONPROC;
#else
  #error Not supported BSD system 
#endif
}

void bsdcommon::get_number_of_running_processes(short unsigned int *run_procs) {
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

static bool is_top_process(BSD_COMMON_PROC_STRUCT *p) {
#if defined(__NetBSD__)
  return !((p->p_flag & P_SYSTEM)) && p->p_comm[0] != 0;
#else
  #error Not supported BSD system 
#endif
}

static int32_t get_pid(BSD_COMMON_PROC_STRUCT *p) {
#if defined(__NetBSD__)
  return p->p_pid;
#else
  #error Not supported BSD system 
#endif
}

// conky uses time in hundredths of seconds (centiseconds)
static unsigned long to_conky_time(u_int32_t sec, u_int32_t usec) {
  return sec * 100 + (unsigned long)(usec * 0.0001);
}

static void proc_from_bsdproc(struct process *proc, BSD_COMMON_PROC_STRUCT *p) {
  free_and_zero(proc->name);
  free_and_zero(proc->basename);

  unsigned long user_time = 0;
  unsigned long kernel_time = 0;

#if defined(__NetBSD__)
  // https://github.com/netbsd/src/blob/trunk/sys/sys/sysctl.h
  proc->time_stamp = g_time;
  proc->user_time = to_conky_time(p->p_uutime_sec, p->p_uutime_usec);
  proc->kernel_time = to_conky_time(p->p_ustime_sec, p->p_ustime_usec);
  proc->uid = p->p_uid;
  proc->name = strndup(p->p_comm, text_buffer_size.get(*::state));
  proc->basename = strndup(p->p_comm, text_buffer_size.get(*::state));
  proc->amount = 100.0 * p->p_pctcpu / FSCALE;
  proc->vsize = p->p_vm_vsize * getpagesize();
  proc->rss = p->p_vm_rssize * getpagesize();
  proc->total_cpu_time = to_conky_time(p->p_rtime_sec, p->p_rtime_usec);
#else
  #error Not supported BSD system 
#endif

  if (proc->previous_user_time == ULONG_MAX) {
    proc->previous_user_time = proc->user_time;
  }

  if (proc->previous_kernel_time == ULONG_MAX) {
    proc->previous_kernel_time = proc->kernel_time;
  }

  /* strangely, the values aren't monotonous (from Linux) */
  if (proc->previous_user_time > proc->user_time) {
    proc->previous_user_time = proc->user_time;
  }

  if (proc->previous_kernel_time > proc->kernel_time) {
    proc->previous_kernel_time = proc->kernel_time;
  }

  /* store the difference of the user_time */
  user_time = proc->user_time - proc->previous_user_time;
  kernel_time = proc->kernel_time - proc->previous_kernel_time;

  /* backup the process->user_time for next time around */
  proc->previous_user_time = proc->user_time;
  proc->previous_kernel_time = proc->kernel_time;

  /* store only the difference of the user_time here... */
  proc->user_time = user_time;
  proc->kernel_time = kernel_time;
}

void bsdcommon::update_top_info() {
  if (!init_kvm()) {
    return;
  }

  struct process *proc = nullptr;
  short unsigned int nprocs = 0;

  BSD_COMMON_PROC_STRUCT *ps = get_processes(&nprocs);
  if (ps == nullptr) {
    return;
  }

  for (int i = 0; i < nprocs; ++i) {
    BSD_COMMON_PROC_STRUCT *p = &ps[i];

    if (!is_top_process(p)) {
      continue;
    }

    proc = get_process(get_pid(p));
    if (!proc) {
      continue;
    }

    proc_from_bsdproc(proc, p);
  }
}

static bool is_process(BSD_COMMON_PROC_STRUCT *p, const char *name) {
#if defined(__NetBSD__)
  return p->p_comm[0] != 0 && strcmp(p->p_comm, name) == 0;
#else
  #error Not supported BSD system 
#endif
}

bool bsdcommon::is_conky_already_running() {
  if (!init_kvm()) {
    return false;
  }

  struct process *proc = nullptr;
  short unsigned int nprocs = 0;
  int instances = 0;

  BSD_COMMON_PROC_STRUCT *ps = get_processes(&nprocs);
  if (ps == nullptr) {
    return false;
  }

  for (int i = 0; i < nprocs && instances < 2; ++i) {
    BSD_COMMON_PROC_STRUCT *p = &ps[i];

    if (is_process(p, "conky")) {
      ++instances;
    }
  }

  return instances > 1;
}

// conyk uses kilobytes
static unsigned long long to_conky_size(uint64_t size, uint64_t pagesize) {
  return (size >> 10) * pagesize;
}

void bsdcommon::update_meminfo(struct information &info) {
  size_t len;
#if defined(__NetBSD__)
  int mib[2] = {CTL_VM, VM_UVMEXP2};
  // NOTE(gmb): https://github.com/NetBSD/src/blob/trunk/sys/uvm/uvm_extern.h
  struct uvmexp_sysctl meminfo;
#else
  #error Not supported BSD system.
#endif

  len = sizeof(meminfo);
  if (sysctl(mib, 2, &meminfo, &len, NULL, 0) == -1 ) {
    NORM_ERR("sysctl() failed");
    return;
  }

#if defined(__NetBSD__)
  // TODO(gmb): Try to fill all memory related fields.
  info.memmax = to_conky_size(meminfo.npages, meminfo.pagesize);
  info.memfree = info.memeasyfree = to_conky_size(meminfo.free, meminfo.pagesize);
  info.mem = info.memmax - info.memfree;

  info.swapmax = to_conky_size(meminfo.swpages, meminfo.pagesize);
  info.swap = to_conky_size(meminfo.swpginuse, meminfo.pagesize);
  info.swapfree = info.swapmax - info.swap;
#else
  #error Not supported BSD system.
#endif
}
