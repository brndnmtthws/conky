/*
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2018, npyl <n.pylarinos@hotmail.com>
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
 ***********************************************************************************************
 *
 *  darwin.cc
 *  Nickolas Pylarinos
 *
 *  ~ To mrt and vggol ~
 *
 ***********************************************************************************************
 *
 *  Code for SIP taken from Pike R. Alpha's csrstat tool
 *https://github.com/Piker-Alpha/csrstat csrstat version 1.8 ( works for OS up
 *to High Sierra )
 *
 *  My patches:
 *      made csr_get_active_config weak link and added check for finding if it
 *is available. patched the _csr_check function to return the bool bit instead.
 */

#include "conky.h"  // for struct info
#include "darwin.h"

#include <AvailabilityMacros.h>

#include <sys/mount.h>  // statfs
#include <sys/sysctl.h>
#include <cstdio>

#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/mach_types.h>
#include <mach/machine.h>
#include <mach/vm_statistics.h>

#include <mach/mach.h>  // update_total_processes

#include <dispatch/dispatch.h>  // get_top_info
#include <libproc.h>            // get_top_info
#include "top.h"                // get_top_info

#include <ifaddrs.h>   // update_net_stats
#include "net_stat.h"  // update_net_stats

#include "darwin_sip.h"  // sip status

#include <vector>

#ifdef BUILD_IPGFREQ
#include <IntelPowerGadget/EnergyLib.h>
#endif

#ifdef BUILD_WLAN
#import <CoreWLAN/CoreWLAN.h>
#endif

/* clock_gettime includes */
#ifndef HAVE_CLOCK_GETTIME
#include <errno.h>
#include <mach/clock.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <time.h>
#endif

/* debugging defines */
#define DEBUG_MODE

/* (E)nhanced printf */
#ifdef DEBUG_MODE
#include <cstdarg>
void eprintf(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}
#else
#define eprintf(...) /* ... */
#endif

#define GETSYSCTL(name, var) getsysctl(name, &(var), sizeof(var))

/*
 * used by calc_cpu_each() for get_top_info()
 */
static conky::simple_config_setting<bool> top_cpu_separate("top_cpu_separate",
                                                           false, true);
/*
 * used by update_cpu_usage()
 */
struct cpusample *sample_handle = nullptr;

static int getsysctl(const char *name, void *ptr, size_t len) {
  size_t nlen = len;

  if (sysctlbyname(name, ptr, &nlen, nullptr, 0) == -1) { return -1; }

  if (nlen != len && errno == ENOMEM) { return -1; }

  return 0;
}

/*
 *  clock_gettime is not implemented on versions prior to Sierra!
 *  code taken from
 * https://github.com/lorrden/darwin-posix-rt/blob/master/clock_gettime.c
 */
#ifndef HAVE_CLOCK_GETTIME

int clock_gettime(int clock_id, struct timespec *ts) {
  mach_timespec_t mts;
  static clock_serv_t rt_clock_serv = 0;
  static clock_serv_t mono_clock_serv = 0;

  switch (clock_id) {
    case CLOCK_REALTIME:
      if (rt_clock_serv == 0) {
        (void)host_get_clock_service(mach_host_self(), CALENDAR_CLOCK,
                                     &rt_clock_serv);
      }
      (void)clock_get_time(rt_clock_serv, &mts);
      ts->tv_sec = mts.tv_sec;
      ts->tv_nsec = mts.tv_nsec;
      return 0;
    case CLOCK_MONOTONIC:
      if (mono_clock_serv == 0) {
        (void)host_get_clock_service(mach_host_self(), SYSTEM_CLOCK,
                                     &mono_clock_serv);
      }
      (void)clock_get_time(mono_clock_serv, &mts);
      ts->tv_sec = mts.tv_sec;
      ts->tv_nsec = mts.tv_nsec;
      return 0;
    default:
      errno = EINVAL;
      return -1;
  }
}
#endif /* ifndef HAVE_CLOCK_GETTIME */

/*
 *  helper_update_threads_processes()
 *
 *  Helper function for update_threads() and update_running_threads()
 *  Uses mach API to get load info ( task_count, thread_count )
 *
 */
static void helper_update_threads_processes() {
  static host_name_port_t machHost;
  static processor_set_name_port_t processorSet = 0;
  static bool machStuffInitialised = false;

  /* Set up our mach host and default processor set for later calls */
  if (!machStuffInitialised) {
    machHost = mach_host_self();
    processor_set_default(machHost, &processorSet);

    /* set this to true so we don't ever initialise stuff again */
    machStuffInitialised = true;
  }

  /* get load info */
  struct processor_set_load_info loadInfo {};
  mach_msg_type_number_t count = PROCESSOR_SET_LOAD_INFO_COUNT;
  kern_return_t err = processor_set_statistics(
      processorSet, PROCESSOR_SET_LOAD_INFO,
      reinterpret_cast<processor_set_info_t>(&loadInfo), &count);

  if (err != KERN_SUCCESS) { return; }

  info.procs = loadInfo.task_count;
  info.threads = loadInfo.thread_count;
}

/*
 * useful info about the cpu used by functions such as update_cpu_usage() and
 * get_top_info()
 */
struct cpusample {
  uint64_t totalUserTime;   /* ticks of CPU in userspace */
  uint64_t totalSystemTime; /* ticks of CPU in kernelspace */
  uint64_t totalIdleTime;   /* ticks in idleness */

  uint64_t total;          /* delta of current and previous */
  uint64_t current_total;  /* total CPU ticks of current iteration */
  uint64_t previous_total; /* total CPU tick of previous iteration */
};

/*
 * Memory sample
 */
typedef struct memorysample {
  vm_statistics64_data_t vm_stat; /* general VM information */
  uint64_t pages_stolen;          /* # of stolen pages */
  vm_size_t pagesize;             /* pagesize (in bytes) */
  boolean_t purgeable_is_valid; /* check if we have data for purgeable memory */
} libtop_tsamp_t;

/*
 * get_cpu_sample()
 *
 * Gets systemTime, userTime and idleTime for CPU
 * MenuMeters has been great inspiration for this function
 */
static void get_cpu_sample(struct cpusample **sample) {
  host_name_port_t machHost;
  natural_t processorCount;
  processor_cpu_load_info_t processorTickInfo;
  mach_msg_type_number_t processorInfoCount;
  
  machHost = mach_host_self();

  kern_return_t err = host_processor_info(
      machHost, PROCESSOR_CPU_LOAD_INFO, &processorCount,
      reinterpret_cast<processor_info_array_t *>(&processorTickInfo),
      &processorInfoCount);
  if (err != KERN_SUCCESS) {
    printf("host_statistics: %s\n", mach_error_string(err));
    return;
  }

  /*
   * start from samples[1] because samples[0] is overall CPU usage
   */
  for (natural_t i = 1; i < processorCount + 1; i++) {
    (*sample)[i].totalSystemTime =
        processorTickInfo[i - 1].cpu_ticks[CPU_STATE_SYSTEM],
    (*sample)[i].totalUserTime =
        processorTickInfo[i - 1].cpu_ticks[CPU_STATE_USER],
    (*sample)[i].totalIdleTime =
        processorTickInfo[i - 1].cpu_ticks[CPU_STATE_IDLE];
  }

  /*
   * sum up all totals
   */
  for (natural_t i = 1; i < processorCount + 1; i++) {
    (*sample)[0].totalSystemTime += (*sample)[i].totalSystemTime;
    (*sample)[0].totalUserTime += (*sample)[i].totalUserTime;
    (*sample)[0].totalIdleTime += (*sample)[i].totalIdleTime;
  }

  /*
   * Dealloc
   */
  vm_deallocate(mach_task_self(), (vm_address_t)processorTickInfo,
                static_cast<vm_size_t>(processorInfoCount * sizeof(natural_t)));
}

void allocate_cpu_sample(struct cpusample **sample) {
  if (*sample != nullptr)
    return;
  
  /*
   * allocate ncpus+1 cpusample structs (one foreach CPU)
   * ** sample_handle[0] is overal CPU usage
   */
  *sample = reinterpret_cast<struct cpusample *>(malloc(sizeof(cpusample) * (info.cpu_count + 1)));
  memset(*sample, 0, sizeof(cpusample) * (info.cpu_count + 1));
  
  sample_handle = *sample; /* use a public handle for deallocating */
}

void deallocate_cpu_sample(struct text_object *obj) {
  if (sample_handle != nullptr) {
    free(sample_handle);
    sample_handle = nullptr;
  }
}

/*
 * helper_get_proc_list()
 *
 * helper function that returns the count of processes
 * and provides a list of kinfo_proc structs representing each.
 *
 * ERRORS: returns -1 if something failed
 *
 * ATTENTION: Do not forget to free the array once you are done with it,
 *  it is not freed automatically.
 */
static int helper_get_proc_list(struct kinfo_proc **p) {
  int err = 0;
  size_t length = 0;
  static const int name[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};

  /* Call sysctl with a nullptr buffer to get proper length */
  err = sysctl((int *)name, (sizeof(name) / sizeof(*name)) - 1, nullptr,
               &length, nullptr, 0);
  if (err != 0) {
    perror(nullptr);
    return (-1);
  }

  /* Allocate buffer */
  *p = static_cast<kinfo_proc *>(malloc(length));
  if (p == nullptr) {
    perror(nullptr);
    return (-1);
  }

  /* Get the actual process list */
  err = sysctl((int *)name, (sizeof(name) / sizeof(*name)) - 1, *p, &length,
               nullptr, 0);
  if (err != 0) {
    perror(nullptr);
    return (-1);
  }

  int proc_count = length / sizeof(struct kinfo_proc);
  return proc_count;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------
 *  macOS Swapfiles Logic...
 *
 *  o   There is NO separate partition for swap storage ( unlike most Unix-based
 *OSes ) --- Instead swap memory is stored in the currently used partition
 *inside files
 *
 *  o   macOS can use ALL the available space on the used partition
 *
 *  o   Swap memory files are called swapfiles, stored inside /private/var/vm/
 *
 *  o   Every swapfile has index number eg. swapfile0, swapfile1, ...
 *
 *  o   Anyone can change the location of the swapfiles by editing the plist:
 * /System/Library/LaunchDaemons/com.apple.dynamic_pager.plist ( Though it seems
 *like this is not supported by the dynamic_pager application as can be observed
 *from the code:
 *          https://github.com/practicalswift/osx/blob/master/src/system_cmds/dynamic_pager.tproj/dynamic_pager.c
 *) o   Every swapfile has size of 1GB
 *
 *-------------------------------------------------------------------------------------------------------------------------------------------------------------------
 */
static int swapmode(unsigned long *retavail, unsigned long *retfree) {
  /*
   *  COMPATIBILITY:  Tiger+
   */

  int swapMIB[] = {CTL_VM, 5};
  struct xsw_usage swapUsage {};
  size_t swapUsageSize = sizeof(swapUsage);
  memset(&swapUsage, 0, sizeof(swapUsage));
  if (sysctl(swapMIB, 2, &swapUsage, &swapUsageSize, nullptr, 0) == 0) {
    *retfree = swapUsage.xsu_avail / 1024;
    *retavail = swapUsage.xsu_total / 1024;
  } else {
    perror("sysctl");
    return (-1);
  }

  return 1;
}

void prepare_update() {
  // in freebsd.cc this is empty so leaving it here too!
}

int update_uptime() {
  int mib[2] = {CTL_KERN, KERN_BOOTTIME};
  struct timeval boottime {};
  time_t now;
  size_t size = sizeof(boottime);

  if ((sysctl(mib, 2, &boottime, &size, nullptr, 0) != -1) &&
      (boottime.tv_sec != 0)) {
    time(&now);
    info.uptime = now - boottime.tv_sec;
  } else {
    fprintf(stderr, "could not get uptime\n");
    info.uptime = 0;
  }

  return 0;
}

/*
 *  check_mount
 *
 *  Notes on macOS implementation:
 *  1.  path mustn't contain a '/' at the end! ( eg. /Volumes/MacOS/ is not
 * correct but this is correct: /Volumes/MacOS )
 *  2.  it works the same as the FreeBSD function
 */
int check_mount(struct text_object *obj) {
  int num_mounts = 0;
  struct statfs *mounts;

  if (obj->data.s == nullptr) { return 0; }

  num_mounts = getmntinfo(&mounts, MNT_WAIT);

  if (num_mounts < 0) {
    NORM_ERR("could not get mounts using getmntinfo");
    return 0;
  }

  for (int i = 0; i < num_mounts; i++) {
    if (strcmp(mounts[i].f_mntonname, obj->data.s) == 0) { return 1; }
  }

  return 0;
}

/*
 * required by update_pages_stolen().
 * Taken from apple's top.
 * The code is intact.
 */
/* This is for <rdar://problem/6410098>. */
static uint64_t round_down_wired(uint64_t value) {
  return (value & ~((128 * 1024 * 1024ULL) - 1));
}

/*
 * must be called before libtop_tsamp_update_vm_stats()
 *  to calculate the pages_stolen variable.
 * Taken from apple's top.
 * The code is intact.
 */
/* This is for <rdar://problem/6410098>. */
static void update_pages_stolen(libtop_tsamp_t *tsamp) {
  static int mib_reserved[CTL_MAXNAME];
  static int mib_unusable[CTL_MAXNAME];
  static int mib_other[CTL_MAXNAME];
  static size_t mib_reserved_len = 0;
  static size_t mib_unusable_len = 0;
  static size_t mib_other_len = 0;
  int r;

  tsamp->pages_stolen = 0;

  /* This can be used for testing: */
  // tsamp->pages_stolen = (256 * 1024 * 1024ULL) / tsamp->pagesize;

  if (0 == mib_reserved_len) {
    mib_reserved_len = CTL_MAXNAME;

    r = sysctlnametomib("machdep.memmap.Reserved", mib_reserved,
                        &mib_reserved_len);

    if (-1 == r) {
      mib_reserved_len = 0;
      return;
    }

    mib_unusable_len = CTL_MAXNAME;

    r = sysctlnametomib("machdep.memmap.Unusable", mib_unusable,
                        &mib_unusable_len);

    if (-1 == r) {
      mib_reserved_len = 0;
      return;
    }

    mib_other_len = CTL_MAXNAME;

    r = sysctlnametomib("machdep.memmap.Other", mib_other, &mib_other_len);

    if (-1 == r) {
      mib_reserved_len = 0;
      return;
    }
  }

  if (mib_reserved_len > 0 && mib_unusable_len > 0 && mib_other_len > 0) {
    uint64_t reserved = 0, unusable = 0, other = 0;
    size_t reserved_len;
    size_t unusable_len;
    size_t other_len;

    reserved_len = sizeof(reserved);
    unusable_len = sizeof(unusable);
    other_len = sizeof(other);

    /* These are all declared as QUAD/uint64_t sysctls in the kernel. */

    if (-1 == sysctl(mib_reserved, mib_reserved_len, &reserved, &reserved_len,
                     nullptr, 0)) {
      return;
    }

    if (-1 == sysctl(mib_unusable, mib_unusable_len, &unusable, &unusable_len,
                     nullptr, 0)) {
      return;
    }

    if (-1 ==
        sysctl(mib_other, mib_other_len, &other, &other_len, nullptr, 0)) {
      return;
    }

    if (reserved_len == sizeof(reserved) && unusable_len == sizeof(unusable) &&
        other_len == sizeof(other)) {
      uint64_t stolen = reserved + unusable + other;
      uint64_t mb128 = 128 * 1024 * 1024ULL;

      if (stolen >= mb128) {
        tsamp->pages_stolen = round_down_wired(stolen) / tsamp->pagesize;
      }
    }
  }
}

/**
 * libtop_tsamp_update_vm_stats
 *
 * taken from apple's top (libtop.c)
 * Changes for conky:
 *  - remove references to p_* and b_* named variables
 *  - remove reference to seq variable
 *  - libtop_port replaced with mach_host_self()
 */
static int libtop_tsamp_update_vm_stats(libtop_tsamp_t *tsamp) {
  kern_return_t kr;

  mach_msg_type_number_t count = sizeof(tsamp->vm_stat) / sizeof(natural_t);
  kr = host_statistics64(mach_host_self(), HOST_VM_INFO64,
                         reinterpret_cast<host_info64_t>(&tsamp->vm_stat),
                         &count);
  if (kr != KERN_SUCCESS) { return kr; }

  if (tsamp->pages_stolen > 0) {
    tsamp->vm_stat.wire_count += tsamp->pages_stolen;
  }

  // Check whether we got purgeable memory statistics
  tsamp->purgeable_is_valid = static_cast<boolean_t>(
      count == (sizeof(tsamp->vm_stat) / sizeof(natural_t)));
  if (tsamp->purgeable_is_valid == 0u) {
    tsamp->vm_stat.purgeable_count = 0;
    tsamp->vm_stat.purges = 0;
  }

  return kr;
}

/*
 * helper function for update_meminfo()
 * return physical memory in bytes
 */
uint64_t get_physical_memory() {
  int mib[2] = {CTL_HW, HW_MEMSIZE};

  int64_t physical_memory = 0;
  size_t length = sizeof(int64_t);

  if (sysctl(mib, 2, &physical_memory, &length, nullptr, 0) == -1) {
    physical_memory = 0;
  }

  return physical_memory;
}

int update_meminfo() {
  
  /* XXX See #34 */
  
  vm_size_t page_size = getpagesize();  // get pagesize in bytes
  unsigned long swap_avail, swap_free;

  static libtop_tsamp_t *tsamp = nullptr;
  if (tsamp == nullptr) {
    tsamp = new libtop_tsamp_t;
    if (tsamp == nullptr) { return 0; }

    memset(tsamp, 0, sizeof(libtop_tsamp_t));
    tsamp->pagesize = page_size;
  }

  /* get physical memory */
  uint64_t physical_memory = get_physical_memory() / 1024;
  info.memmax = physical_memory;

  /*
   *  get general memory stats
   *  but first update pages stolen count
   */
  update_pages_stolen(tsamp);
  if (libtop_tsamp_update_vm_stats(tsamp) == KERN_FAILURE) { return 0; }

  /*
   * This is actually a tricky part.
   *
   * MenuMeters, Activity Monitor and top show different values.
   * Our code uses top's implementation because:
   *  - it is apple's tool
   *  - professional projects such as osquery follow it
   *  - Activity Monitor seems to be hiding the whole truth (e.g. for being user
   * friendly)
   *
   * STEPS:
   * - get stolen pages count
   * Occassionaly host_statistics64 doesn't return correct values (see
   * https://stackoverflow.com/questions/14789672/why-does-host-statistics64-return-inconsistent-results)
   * We need to get the count of stolen pages and add it to wired pages count.
   * This is a known bug and apple has implemented the function
   * update_pages_stolen().
   *
   * - use vm_stat.free_count instead of the sum of wired, active and inactive
   * Based on
   * https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
   *  summing up wired, active and inactive is what we should do BUT, based on
   * top, this is incorrect. Seems like apple keeps some info "secret"!
   */
  info.mem = physical_memory - (tsamp->vm_stat.free_count * page_size / 1024);

  /* rest memory related variables */
  info.memwithbuffers = info.mem;
  info.memeasyfree = info.memfree = info.memmax - info.mem;

  if ((swapmode(&swap_avail, &swap_free)) >= 0) {
    info.swapmax = swap_avail;
    info.swap = (swap_avail - swap_free);
    info.swapfree = swap_free;
  } else {
    info.swapmax = 0;
    info.swap = 0;
    info.swapfree = 0;
  }

  return 0;
}

#ifdef BUILD_WLAN

void update_wlan_stats(struct net_stat *ns) {
  
}

#endif

int update_net_stats() {
  struct net_stat *ns;
  double delta;
  long long r, t, last_recv, last_trans;
  struct ifaddrs *ifap, *ifa;
  struct if_data *ifd;

  /* get delta */
  delta = current_update_time - last_update_time;
  if (delta <= 0.0001) { return 0; }

  if (getifaddrs(&ifap) < 0) { return 0; }

  for (ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
    ns = get_net_stat((const char *)ifa->ifa_name, nullptr, nullptr);

    if ((ifa->ifa_flags & IFF_UP) != 0u) {
      struct ifaddrs *iftmp;

#ifdef BUILD_WLAN
      update_wlan_stats(ns);
#endif
      
      ns->up = 1;
      last_recv = ns->recv;
      last_trans = ns->trans;

      if (ifa->ifa_addr->sa_family != AF_LINK) { continue; }

      for (iftmp = ifa->ifa_next;
           iftmp != nullptr && strcmp(ifa->ifa_name, iftmp->ifa_name) == 0;
           iftmp = iftmp->ifa_next) {
        if (iftmp->ifa_addr->sa_family == AF_INET) {
          memcpy(&(ns->addr), iftmp->ifa_addr, iftmp->ifa_addr->sa_len);
        }
      }

      ifd = static_cast<struct if_data *>(ifa->ifa_data);
      r = ifd->ifi_ibytes;
      t = ifd->ifi_obytes;

      if (r < ns->last_read_recv) {
        ns->recv +=
            (static_cast<long long>(4294967295U) - ns->last_read_recv) + r;
      } else {
        ns->recv += (r - ns->last_read_recv);
      }

      ns->last_read_recv = r;

      if (t < ns->last_read_trans) {
        ns->trans +=
            (static_cast<long long>(4294967295U) - ns->last_read_trans) + t;
      } else {
        ns->trans += (t - ns->last_read_trans);
      }

      ns->last_read_trans = t;

      /* calculate speeds */
      ns->recv_speed = (ns->recv - last_recv) / delta;
      ns->trans_speed = (ns->trans - last_trans) / delta;
    } else {
      ns->up = 0;
    }
  }

  freeifaddrs(ifap);
  return 0;
}

int update_threads() {
  helper_update_threads_processes();
  return 0;
}

/*
 * XXX This seems to be wrong... We must first find the threads (using
 * thread_info() )
 *
 * Get list of all processes.
 * Foreach process check its state.
 * If it is RUNNING it means that it actually has some threads
 *  that are in TH_STATE_RUNNING.
 * Foreach pid and foreach pid's threads check their state and increment
 *  the run_threads counter acordingly.
 */
int update_running_threads() {
  struct kinfo_proc *p = nullptr;
  int proc_count = 0;
  int run_threads = 0;

  proc_count = helper_get_proc_list(&p);

  if (proc_count == -1) { return 0; }

  for (int i = 0; i < proc_count; i++) {
    if ((p[i].kp_proc.p_stat & SRUN) != 0) {
      pid_t pid = 0;
      struct proc_taskinfo pti {};
      struct proc_threadinfo pthi {};
      int num_threads = 0;

      pid = p[i].kp_proc.p_pid;

      /* get total number of threads this pid has */
      if (sizeof(pti) ==
          proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti))) {
        num_threads = pti.pti_threadnum;
      } else {
        continue;
      }

      /* foreach thread check its state */
      for (int i = 0; i < num_threads; i++) {
        if (sizeof(pthi) ==
            proc_pidinfo(pid, PROC_PIDTHREADINFO, i, &pthi, sizeof(pthi))) {
          if (pthi.pth_run_state == TH_STATE_RUNNING) { run_threads++; }
        } else {
          continue;
        }
      }
    }
  }

  free(p);
  info.run_threads = run_threads;
  return 0;
}

int update_total_processes() {
  helper_update_threads_processes();
  return 0;

  /*
   *  WARNING: You may stumble upon this implementation:
   *  https://stackoverflow.com/questions/8141913/is-there-a-lightweight-way-to-obtain-the-current-number-of-processes-in-linux
   *
   *  This method DOESN'T find the correct number of tasks.
   *
   *  This is probably (??) because on macOS there is no option for
   *  KERN_PROC_KTHREAD like there is in FreeBSD
   *
   *  In FreeBSD's sysctl.h we can see the following:
   *
   *  KERN_PROC_KTHREAD   all processes (user-level plus kernel threads)
   *  KERN_PROC_ALL       all user-level processes
   *  KERN_PROC_PID       processes with process ID arg
   *  KERN_PROC_PGRP      processes with process group arg
   *  KERN_PROC_SESSION   processes with session arg
   *  KERN_PROC_TTY       processes with tty(4) arg
   *  KERN_PROC_UID       processes with effective user ID arg
   *  KERN_PROC_RUID      processes with real user ID arg
   *
   *  Though in macOS's sysctl.h there are only:
   *
   *  KERN_PROC_ALL        everything
   *  KERN_PROC_PID        by process id
   *  KERN_PROC_PGRP      by process group id
   *  KERN_PROC_SESSION    by session of pid
   *  KERN_PROC_TTY        by controlling tty
   *  KERN_PROC_UID        by effective uid
   *  KERN_PROC_RUID      by real uid
   *  KERN_PROC_LCID      by login context id
   *
   *  Probably by saying "everything" they mean that KERN_PROC_ALL gives all
   *  processes (user-level plus kernel threads) ( So basically this is the
   *  problem with the old implementation )
   */
}

int update_running_processes() {
  struct kinfo_proc *p = nullptr;
  int proc_count = 0;
  int run_procs = 0;

  proc_count = helper_get_proc_list(&p);

  if (proc_count == -1) { return 0; }

  for (int i = 0; i < proc_count; i++) {
    int state = p[i].kp_proc.p_stat;

    if (state == SRUN) {  // XXX this check needs to be fixed...
      run_procs++;
    }
  }

  free(p);
  info.run_procs = run_procs;
  return 0;
}

/*
 * get_cpu_count
 *
 * The macOS implementation gets the number of active cpus
 * in compliance with linux implementation.
 */
void get_cpu_count() {
  int cpu_count = 0;

  if (GETSYSCTL("hw.activecpu", cpu_count) == 0) {
    info.cpu_count = cpu_count;
  } else {
    fprintf(stderr, "Cannot get hw.activecpu\n");
    info.cpu_count = 0;
  }

  if (info.cpu_usage == nullptr) {
    /*
     * Allocate ncpus+1 slots because cpu_usage[0] is overall usage.
     */
    info.cpu_usage =
        static_cast<float *>(malloc((info.cpu_count + 1) * sizeof(float)));
    if (info.cpu_usage == nullptr) { CRIT_ERR(nullptr, nullptr, "malloc"); }
  }
}

/*
 * used by update_cpu_usage()
 */
struct cpu_info {
  long oldtotal;
  long oldused;
};

int update_cpu_usage() {
  static bool cpu_setup = 0;

  long used, total;
  static struct cpu_info *cpu = nullptr;
  unsigned int malloc_cpu_size = 0;
  extern void *global_cpu;
  
  static struct cpusample *sample = nullptr;

  static pthread_mutex_t last_stat_update_mutex = PTHREAD_MUTEX_INITIALIZER;
  static double last_stat_update = 0.0;

  /* since we use wrappers for this function, the update machinery
   * can't eliminate double invocations of this function. Check for
   * them here, otherwise cpu_usage counters are freaking out. */
  pthread_mutex_lock(&last_stat_update_mutex);
  if (last_stat_update == current_update_time) {
    pthread_mutex_unlock(&last_stat_update_mutex);
    return 0;
  }
  last_stat_update = current_update_time;
  pthread_mutex_unlock(&last_stat_update_mutex);

  /* add check for !info.cpu_usage since that mem is freed on a SIGUSR1 */
  if ((static_cast<int>(cpu_setup) == 0) || (info.cpu_usage == nullptr)) {
    get_cpu_count();
    cpu_setup = 1;
  }

  if (global_cpu == nullptr) {
    /*
     * Allocate ncpus+1 slots because cpu_usage[0] is overall usage.
     */
    malloc_cpu_size = (info.cpu_count + 1) * sizeof(struct cpu_info);
    cpu = static_cast<cpu_info *>(malloc(malloc_cpu_size));
    memset(cpu, 0, malloc_cpu_size);
    global_cpu = cpu;
  }

  allocate_cpu_sample(&sample);
  
  get_cpu_sample(&sample);

  /*
   * Setup conky's structs for-each core
   */
  for (int i = 1; i < info.cpu_count + 1; i++) {
    int j = i - 1;

    total = sample[i].totalUserTime + sample[i].totalIdleTime + sample[i].totalSystemTime;
    used = total - sample[i].totalIdleTime;

    if ((total - cpu[j].oldtotal) != 0) {
      info.cpu_usage[j] = (static_cast<double>(used - cpu[j].oldused)) /
      static_cast<double>(total - cpu[j].oldtotal);
    } else {
      info.cpu_usage[j] = 0;
    }

    cpu[j].oldused = used;
    cpu[j].oldtotal = total;
  }

  return 0;
}

int update_load_average() {
  double v[3];

  getloadavg(v, 3);

  info.loadavg[0] = v[0];
  info.loadavg[1] = v[1];
  info.loadavg[2] = v[2];

  return 0;
}

double get_acpi_temperature(int /*fd*/) {
  printf("get_acpi_temperature: STUB\n");
  return 0.0;
}

void get_battery_stuff(char * /*buf*/, unsigned int /*n*/, const char * /*bat*/,
                       int /*item*/) {
  printf("get_battery_stuff: STUB\n");
}

int get_battery_perct(const char * /*bat*/) {
  printf("get_battery_perct: STUB\n");
  return 1;
}

double get_battery_perct_bar(struct text_object * /*obj*/) {
  printf("get_battery_perct_bar: STUB\n");
  return 0.0;
}

int open_acpi_temperature(const char *name) {
  printf("open_acpi_temperature: STUB\n");

  (void)name;
  /* Not applicable for FreeBSD. */
  return 0;
}

void get_acpi_ac_adapter(char * /*p_client_buffer*/,
                         size_t /*client_buffer_size*/,
                         const char * /*adapter*/) {
  printf("get_acpi_ac_adapter: STUB\n");
}

void get_acpi_fan(char * /*p_client_buffer*/, size_t /*client_buffer_size*/) {
  printf("get_acpi_fan: STUB\n");
}

/* void */
char get_freq(char *p_client_buffer, size_t client_buffer_size,
              const char *p_format, int divisor, unsigned int cpu) {
  
  if ((p_client_buffer == nullptr) || client_buffer_size <= 0 ||
      (p_format == nullptr) || divisor <= 0) {
    return 0;
  }
  
#ifdef BUILD_IPGFREQ
  /*
   * Our data is always the same for every core, so ignore |cpu| argument.
   */
  
  static bool initialised = false;
  
  if (!initialised) {
    IntelEnergyLibInitialize();
    initialised = true;
  }
  
  int freq = 0;
  GetIAFrequency(cpu, &freq);
  
  snprintf(p_client_buffer, client_buffer_size, p_format,
           static_cast<float>(freq) / divisor);
#else
  /*
   * We get the factory cpu frequency, not **current** cpu frequency
   * (Also, it is always the same for every core, so ignore |cpu| argument)
   * Enable BUILD_IPGFREQ for getting current frequency.
   */
  
  int mib[2];
  unsigned int freq;
  size_t len;
  
  mib[0] = CTL_HW;
  mib[1] = HW_CPU_FREQ;
  len = sizeof(freq);
  
  if (sysctl(mib, 2, &freq, &len, nullptr, 0) == 0) {
    /*
     * convert to MHz
     */
    divisor *= 1000000;
    
    snprintf(p_client_buffer, client_buffer_size, p_format,
             static_cast<float>(freq) / divisor);
  } else {
    snprintf(p_client_buffer, client_buffer_size, p_format, 0.0f);
    return 0;
  }
#endif
  
  return 1;
}

int update_diskio() {
  printf("update_diskio: STUB\n");
  return 0;
}

void get_battery_short_status(char * /*buffer*/, unsigned int /*n*/,
                              const char * /*bat*/) {
  printf("get_battery_short_status: STUB\n");
}

int get_entropy_avail(const unsigned int *val) {
  (void)val;
  return 1;
}
int get_entropy_poolsize(const unsigned int *val) {
  (void)val;
  return 1;
}

/******************************************
 *          get top info section          *
 ******************************************/

/*
 * Calculate a process' cpu usage percentage
 */
static void calc_cpu_usage_for_proc(struct process *proc, uint64_t total) {
  float mul = 100.0;
  if (top_cpu_separate.get(*state)) { mul *= info.cpu_count; }

  proc->amount =
      mul * (proc->user_time + proc->kernel_time) / static_cast<float>(total);
}

/*
 * calculate total CPU usage based on total CPU usage
 * of previous iteration stored inside |process| struct
 */
static void calc_cpu_total(struct process *proc, uint64_t *total) {
  uint64_t current_total = 0; /* of current iteration */
  struct cpusample *sample = nullptr;
  
  allocate_cpu_sample(&sample);
  
  get_cpu_sample(&sample);
  current_total =
      sample[0].totalUserTime + sample[0].totalIdleTime + sample[0].totalSystemTime;

  *total = current_total - proc->previous_total_cpu_time;
  proc->previous_total_cpu_time = current_total;

  *total = ((*total / sysconf(_SC_CLK_TCK)) * 100) / info.cpu_count;
}

/*
 * calc_cpu_time_for_proc
 *
 * calculates user_time and kernel_time and sets the contents of the |process|
 * struct excessively based on process_parse_stat() from linux.cc
 */
static void calc_cpu_time_for_proc(struct process *process,
                                   const struct proc_taskinfo *pti) {
  unsigned long long user_time = 0;
  unsigned long long kernel_time = 0;

  process->user_time = pti->pti_total_user;
  process->kernel_time = pti->pti_total_system;

  /* user_time and kernel_time are in nanoseconds, total_cpu_time in
   * centiseconds. Therefore we divide by 10^7 . */
  process->user_time /= 10000000;
  process->kernel_time /= 10000000;

  process->total_cpu_time = process->user_time + process->kernel_time;
  if (process->previous_user_time == ULONG_MAX) {
    process->previous_user_time = process->user_time;
  }
  if (process->previous_kernel_time == ULONG_MAX) {
    process->previous_kernel_time = process->kernel_time;
  }

  /* store the difference of the user_time */
  user_time = process->user_time - process->previous_user_time;
  kernel_time = process->kernel_time - process->previous_kernel_time;

  /* backup the process->user_time for next time around */
  process->previous_user_time = process->user_time;
  process->previous_kernel_time = process->kernel_time;

  /* store only the difference of the user_time here... */
  process->user_time = user_time;
  process->kernel_time = kernel_time;
}

/*
 * finds top-information only for one process which is represented by a
 * kinfo_proc struct this function is called mutliple types ( one foreach
 * process ) to implement get_top_info()
 */
static void get_top_info_for_kinfo_proc(struct kinfo_proc *p) {
  struct process *proc = nullptr;
  struct proc_taskinfo pti {};
  pid_t pid;

  pid = p->kp_proc.p_pid;
  proc = get_process(pid);

  free_and_zero(proc->name);
  free_and_zero(proc->basename);

  proc->name = strndup(p->kp_proc.p_comm, text_buffer_size.get(*state));
  proc->basename = strndup(p->kp_proc.p_comm, text_buffer_size.get(*state));
  proc->uid = p->kp_eproc.e_pcred.p_ruid;
  proc->time_stamp = g_time;

  if (sizeof(pti) ==
      proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti))) {
    /* vsize, rss are in bytes thus we dont have to convert */
    proc->vsize = pti.pti_virtual_size;
    proc->rss = pti.pti_resident_size;

    __block uint64_t t = 0;

    __block bool calc_cpu_total_finished = false;
    __block bool calc_proc_total_finished = false;

    dispatch_async(
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
          /* calc CPU time for process */
          calc_cpu_time_for_proc(proc, &pti);

          calc_proc_total_finished = true;
        });

    dispatch_async(
        dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
          /* calc total CPU time (considering current process) */
          calc_cpu_total(proc, &t);

          calc_cpu_total_finished = true;
        });

    /*
     * wait until done
     */
    while (!(calc_cpu_total_finished && calc_proc_total_finished)) { usleep(500); }

    /* calc the amount(%) of CPU the process used  */
    calc_cpu_usage_for_proc(proc, t);
  }
}

/* While topless is obviously better, top is also not bad. */

void get_top_info() {
  int proc_count = 0;
  struct kinfo_proc *p = nullptr;

  /*
   * See #16
   */
  get_cpu_count();

  /*
   *  get processes count
   *  and create the processes list
   */
  proc_count = helper_get_proc_list(&p);

  if (proc_count == -1) { return; }

  /*
   *  get top info for-each process
   */
  for (int i = 0; i < proc_count; i++) {
    if ((((p[i].kp_proc.p_flag & P_SYSTEM)) == 0) &&
        *p[i].kp_proc.p_comm != '\0') {
      get_top_info_for_kinfo_proc(&p[i]);
    }
  }

  free(p);
}

/*********************************************************************************************
 *                                  System Integrity Protection *
 *********************************************************************************************/

#if (MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_9)

/*
 *  Check if a flag is enabled based on the csr_config variable
 *  Also, flip the result on occasion
 */
bool _csr_check(int aMask, bool aFlipflag) {
  bool bit = (info.csr_config & aMask) != 0u;

  if (aFlipflag) { return !bit; }

  return bit;
}

/*
 *  Extract info from the csr_config variable and set the flags struct
 */
void fill_csr_config_flags_struct() {
  info.csr_config_flags.csr_allow_apple_internal =
      _csr_check(CSR_ALLOW_APPLE_INTERNAL, 0);
  info.csr_config_flags.csr_allow_untrusted_kexts =
      _csr_check(CSR_ALLOW_UNTRUSTED_KEXTS, 1);
  info.csr_config_flags.csr_allow_task_for_pid =
      _csr_check(CSR_ALLOW_TASK_FOR_PID, 1);
  info.csr_config_flags.csr_allow_unrestricted_fs =
      _csr_check(CSR_ALLOW_UNRESTRICTED_FS, 1);
  info.csr_config_flags.csr_allow_kernel_debugger =
      _csr_check(CSR_ALLOW_KERNEL_DEBUGGER, 1);
  info.csr_config_flags.csr_allow_unrestricted_dtrace =
      _csr_check(CSR_ALLOW_UNRESTRICTED_DTRACE, 1);
  info.csr_config_flags.csr_allow_unrestricted_nvram =
      _csr_check(CSR_ALLOW_UNRESTRICTED_NVRAM, 1);
  info.csr_config_flags.csr_allow_device_configuration =
      _csr_check(CSR_ALLOW_DEVICE_CONFIGURATION, 0);
  info.csr_config_flags.csr_allow_any_recovery_os =
      _csr_check(CSR_ALLOW_ANY_RECOVERY_OS, 1);
  info.csr_config_flags.csr_allow_user_approved_kexts =
      _csr_check(CSR_ALLOW_UNAPPROVED_KEXTS, 1);
}

/*
 *  Get SIP configuration   ( sets csr_config and csr_config_flags )
 */
int get_sip_status() {
  if (csr_get_active_config ==
      nullptr) /*  check if weakly linked symbol exists    */
  {
    NORM_ERR("$sip_status will not work on this version of macOS\n");
    return 0;
  }

  csr_get_active_config(&info.csr_config);
  fill_csr_config_flags_struct();

  return 0;
}

/*
 *  Prints SIP status or a specific SIP feature status depending on the argument
 *  passed to $sip_status command
 *
 *  Variables that can be passed to $sip_status command
 *
 *  nothing --> print enabled / disabled
 *  0   --> apple internal
 *  1   --> forbid untrusted kexts
 *  2   --> forbid task-for-pid
 *  3   --> restrict filesystem
 *  4   --> forbid kernel-debugger
 *  5   --> restrict dtrace
 *  6   --> restrict nvram
 *  7   --> forbid device-configuration
 *  8   --> forbid any-recovery-os
 *  9   --> forbid user-approved-kexts
 *  a   --> check if unsupported configuration  ---> this is not an apple SIP
 *  flag. This is for us.
 *
 *  The print function is designed to show 'YES' if a specific protection
 *  measure is ENABLED. For example, if SIP is configured to disallow untrusted
 *  kexts, then our function will print 'YES'.
 *
 *  For this reason, your conkyrc should say for example: Untrusted Kexts
 *  Protection: ${sip_status 1} You should not write: "Allow Untrusted Kexts",
 *  this is wrong.
 */
void print_sip_status(struct text_object *obj, char *p, int p_max_size) {
  if (csr_get_active_config ==
      nullptr) /*  check if weakly linked symbol exists    */
  {
    snprintf(p, p_max_size, "%s", "unsupported");
    NORM_ERR("$sip_status will not work on this version of macOS\n");
    return;
  }

  /* conky window output */
  (void)obj;

  if (obj->data.s == nullptr) { return; }

  if (strlen(obj->data.s) == 0) {
    snprintf(p, p_max_size, "%s",
             (info.csr_config == CSR_VALID_FLAGS) ? "disabled" : "enabled");
  } else if (strlen(obj->data.s) == 1) {
    switch (obj->data.s[0]) {
      case '0':
        snprintf(p, p_max_size, "%s",
                 info.csr_config_flags.csr_allow_apple_internal ? "YES" : "NO");
        break;
      case '1':
        snprintf(
            p, p_max_size, "%s",
            info.csr_config_flags.csr_allow_untrusted_kexts ? "YES" : "NO");
        break;
      case '2':
        snprintf(p, p_max_size, "%s",
                 info.csr_config_flags.csr_allow_task_for_pid ? "YES" : "NO");
        break;
      case '3':
        snprintf(
            p, p_max_size, "%s",
            info.csr_config_flags.csr_allow_unrestricted_fs ? "YES" : "NO");
        break;
      case '4':
        snprintf(
            p, p_max_size, "%s",
            info.csr_config_flags.csr_allow_kernel_debugger ? "YES" : "NO");
        break;
      case '5':
        snprintf(
            p, p_max_size, "%s",
            info.csr_config_flags.csr_allow_unrestricted_dtrace ? "YES" : "NO");
        break;
      case '6':
        snprintf(
            p, p_max_size, "%s",
            info.csr_config_flags.csr_allow_unrestricted_nvram ? "YES" : "NO");
        break;
      case '7':
        snprintf(p, p_max_size, "%s",
                 info.csr_config_flags.csr_allow_device_configuration ? "YES"
                                                                      : "NO");
        break;
      case '8':
        snprintf(
            p, p_max_size, "%s",
            info.csr_config_flags.csr_allow_any_recovery_os ? "YES" : "NO");
        break;
      case '9':
        snprintf(
            p, p_max_size, "%s",
            info.csr_config_flags.csr_allow_user_approved_kexts ? "YES" : "NO");
        break;
      case 'a':
        snprintf(p, p_max_size, "%s",
                 ((info.csr_config != 0u) &&
                  (info.csr_config != CSR_ALLOW_APPLE_INTERNAL))
                     ? "unsupported configuration, beware!"
                     : "configuration is ok");
        break;
      default:
        snprintf(p, p_max_size, "%s", "unsupported");
        NORM_ERR(
            "print_sip_status: unsupported argument passed to $sip_status");
        break;
    }
  } else { /* bad argument */
    snprintf(p, p_max_size, "%s", "unsupported");
    NORM_ERR("print_sip_status: unsupported argument passed to $sip_status");
  }
}

#else /* Mavericks and before */
/*
 * Versions prior to Yosemite DONT EVEN DEFINE csr_get_active_config()
 * function.  Thus we must avoid calling this function!
 */

int get_sip_status(void) {
  /* Does not do anything intentionally */
  return 0;
}

void print_sip_status(struct text_object *obj, char *p, int p_max_size) {
  /* conky window output */
  (void)obj;

  if (!obj->data.s) return;

  if (strlen(obj->data.s) == 0) {
    snprintf(p, p_max_size, "%s", "error unsupported");
  } else if (strlen(obj->data.s) == 1) {
    switch (obj->data.s[0]) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'a':
        snprintf(p, p_max_size, "%s", "error unsupported");
        break;
      default:
        snprintf(p, p_max_size, "%s", "unsupported");
        NORM_ERR(
            "print_sip_status: unsupported argument passed to $sip_status");
        break;
    }
  } else { /* bad argument */
    snprintf(p, p_max_size, "%s", "unsupported");
    NORM_ERR("print_sip_status: unsupported argument passed to $sip_status");
  }
}

#endif
