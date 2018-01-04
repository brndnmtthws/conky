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
 *  Code for SIP taken from Pike R. Alpha's csrstat tool https://github.com/Piker-Alpha/csrstat
 *  csrstat version 1.8 ( works for OS up to High Sierra )
 *
 *  My patches:
 *      made csr_get_active_config weak link and added check for finding if it is available.
 *      patched the _csr_check function to return the bool bit instead.
 */

/*
 * Apologies for the code style...
 * In my eyes it feels better to have
 * different styles at some specific places... :)
 */

#include "darwin.h"
#include "conky.h"              // for struct info

#include <AvailabilityMacros.h>

#include <stdio.h>
#include <sys/mount.h>          // statfs
#include <sys/sysctl.h>

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#include <mach/mach.h>          // update_total_processes

#include <dispatch/dispatch.h>  // get_top_info
#include <libproc.h>            // get_top_info
#include "top.h"                // get_top_info

#include "darwin_sip.h"         // sip status

/* clock_gettime includes */
#ifndef HAVE_CLOCK_GETTIME
#include <mach/mach.h>
#include <mach/clock.h>
#include <mach/mach_time.h>
#include <time.h>
#include <errno.h>
#endif

/* debugging defines */
#define DEBUG_MODE

/* (E)nhanced printf */
#ifdef DEBUG_MODE
#include <stdarg.h>
void eprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
#else
#define eprintf(...) /* ... */
#endif

#define	GETSYSCTL(name, var)	getsysctl(name, &(var), sizeof(var))

/*
 * used by calc_cpu_each() for get_top_info()
 */
static conky::simple_config_setting<bool> top_cpu_separate("top_cpu_separate", false, true);

static int getsysctl(const char *name, void *ptr, size_t len)
{
    size_t nlen = len;
    
    if (sysctlbyname(name, ptr, &nlen, NULL, 0) == -1) {
        return -1;
    }
    
    if (nlen != len && errno == ENOMEM) {
        return -1;
    }
    
    return 0;
}

/*
 *  clock_gettime is not implemented on versions prior to Sierra!
 *  code taken from https://github.com/lorrden/darwin-posix-rt/blob/master/clock_gettime.c
 */
#ifndef HAVE_CLOCK_GETTIME

int clock_gettime(int clock_id, struct timespec *ts)
{
    mach_timespec_t mts;
    static clock_serv_t rt_clock_serv = 0;
    static clock_serv_t mono_clock_serv = 0;
    
    switch (clock_id) {
        case CLOCK_REALTIME:
            if (rt_clock_serv == 0) {
                (void) host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &rt_clock_serv);
            }
            (void) clock_get_time(rt_clock_serv, &mts);
            ts->tv_sec = mts.tv_sec;
            ts->tv_nsec = mts.tv_nsec;
            return 0;
        case CLOCK_MONOTONIC:
            if (mono_clock_serv == 0) {
                (void) host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &mono_clock_serv);
            }
            (void) clock_get_time(mono_clock_serv, &mts);
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
static void helper_update_threads_processes(void)
{
    static host_name_port_t             machHost;
    static processor_set_name_port_t    processorSet = 0;
    static bool                         machStuffInitialised = false;
    
    /* Set up our mach host and default processor set for later calls */
    if (!machStuffInitialised)
    {
        machHost = mach_host_self();
        processor_set_default(machHost, &processorSet);
        
        /* set this to true so we don't ever initialise stuff again */
        machStuffInitialised = true;
    }
    
    /* get load info */
    struct processor_set_load_info loadInfo;
    mach_msg_type_number_t count = PROCESSOR_SET_LOAD_INFO_COUNT;
    kern_return_t err = processor_set_statistics(processorSet, PROCESSOR_SET_LOAD_INFO,
                                                 (processor_set_info_t)&loadInfo, &count);
    
    if (err != KERN_SUCCESS)
        return;
    
    info.run_procs = loadInfo.task_count;
    info.run_threads = loadInfo.thread_count;
}

/*
 * useful info about the cpu used by functions such as update_cpu_usage() and get_top_info()
 */
struct cpusample
{
    uint64_t totalUserTime;                     /* ticks of CPU in userspace */
    uint64_t totalSystemTime;                   /* ticks of CPU in kernelspace */
    uint64_t totalIdleTime;                     /* ticks in idleness */
    
    uint64_t total;                             /* delta of current and previous */
    uint64_t current_total;                     /* total CPU ticks of current iteration */
    uint64_t previous_total;                    /* total CPU tick of previous iteration */
};

/*
 * get_cpu_sample()
 *
 * Gets systemTime, userTime and idleTime for CPU
 * plagiarised from https://stackoverflow.com/questions/20471920/how-to-get-total-cpu-idle-time-in-objective-c-c-on-os-x
 */
static void get_cpu_sample(struct cpusample *sample)
{
    kern_return_t kr;
    mach_msg_type_number_t count;
    host_cpu_load_info_data_t r_load;
    
    count = HOST_CPU_LOAD_INFO_COUNT;
    kr = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (int *)&r_load, &count);
    
    if (kr != KERN_SUCCESS)
    {
        printf("host_statistics: %s\n", mach_error_string(kr));
        return;
    }
    
    sample->totalSystemTime = r_load.cpu_ticks[CPU_STATE_SYSTEM];
    sample->totalUserTime = r_load.cpu_ticks[CPU_STATE_USER] + r_load.cpu_ticks[CPU_STATE_NICE];
    sample->totalIdleTime = r_load.cpu_ticks[CPU_STATE_IDLE];
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
static int helper_get_proc_list( struct kinfo_proc **p = NULL )
{
    int err = 0;
    size_t length = 0;
    static const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    
    /* Call sysctl with a NULL buffer to get proper length */
    err = sysctl((int *)name, (sizeof(name) / sizeof(*name)) - 1, NULL, &length, NULL, 0);
    if (err) {
        perror(NULL);
        return (-1);
    }
    
    /* Allocate buffer */
    *p = (kinfo_proc*)malloc(length);
    if (!p) {
        perror(NULL);
        return (-1);
    }
    
    /* Get the actual process list */
    err = sysctl((int *)name, (sizeof(name) / sizeof(*name)) - 1, *p, &length, NULL, 0);
    if (err)
    {
        perror(NULL);
        return (-1);
    }
    
    int proc_count = length / sizeof(struct kinfo_proc);
    return proc_count;
}

/*-------------------------------------------------------------------------------------------------------------------------------------------------------------------
 *  macOS Swapfiles Logic...
 *
 *  o   There is NO separate partition for swap storage ( unlike most Unix-based OSes ) --- Instead swap memory is stored in the currently used partition inside files
 *
 *  o   macOS can use ALL the available space on the used partition
 *
 *  o   Swap memory files are called swapfiles, stored inside /private/var/vm/
 *
 *  o   Every swapfile has index number eg. swapfile0, swapfile1, ...
 *
 *  o   Anyone can change the location of the swapfiles by editing the plist: /System/Library/LaunchDaemons/com.apple.dynamic_pager.plist
 *      ( Though it seems like this is not supported by the dynamic_pager application as can be observed from the code: 
 *          https://github.com/practicalswift/osx/blob/master/src/system_cmds/dynamic_pager.tproj/dynamic_pager.c )
 *  o   Every swapfile has size of 1GB
 *
 *-------------------------------------------------------------------------------------------------------------------------------------------------------------------
 */

static int swapmode(unsigned long *retavail, unsigned long *retfree)
{
    /*
     *  COMPATIBILITY:  Tiger+
     */
    
    int	swapMIB[] = { CTL_VM, 5 };
    struct xsw_usage swapUsage;
    size_t swapUsageSize = sizeof(swapUsage);
    memset(&swapUsage, 0, sizeof(swapUsage));
    if (sysctl(swapMIB, 2, &swapUsage, &swapUsageSize, NULL, 0) == 0) {
        
        *retfree = swapUsage.xsu_avail / 1024;
        *retavail = swapUsage.xsu_total / 1024;
    } else {
        perror("sysctl");
        return (-1);
    }
    
    return 1;
}

void prepare_update(void)
{
    // in freebsd.cc this is empty so leaving it here too!
}

int update_uptime(void)
{
    int mib[2] = { CTL_KERN, KERN_BOOTTIME };
    struct timeval boottime;
    time_t now;
    size_t size = sizeof(boottime);
    
    if ((sysctl(mib, 2, &boottime, &size, NULL, 0) != -1)
        && (boottime.tv_sec != 0)) {
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
 *  1.  path mustn't contain a '/' at the end! ( eg. /Volumes/MacOS/ is not correct but this is correct: /Volumes/MacOS )
 *  2.  it works the same as the FreeBSD function
 */
int check_mount(struct text_object *obj)
{
    int             num_mounts = 0;
    struct statfs*  mounts;

    if (!obj->data.s)
        return 0;
    
    num_mounts = getmntinfo(&mounts, MNT_WAIT);
 
    if (num_mounts < 0)
    {
        NORM_ERR("could not get mounts using getmntinfo");
        return 0;
    }
    
    for (int i = 0; i < num_mounts; i++)
        if (strcmp(mounts[i].f_mntonname, obj->data.s) == 0)
        {
            return 1;
        }
    
    return 0;
}

int update_meminfo(void)
{
    //
    //  This is awesome:
    //  https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
    //
    //  it helped me with update_meminfo() and swapmode()
    //
    
    int                     mib[2] = { CTL_HW, HW_MEMSIZE };
    size_t                  length = sizeof( int64_t );
    
    vm_size_t               page_size;
    mach_port_t             mach_port;
    mach_msg_type_number_t  count;
    vm_statistics64_data_t  vm_stats;
    
    unsigned long           swap_avail, swap_free;
    
    //
    //  get machine's memory
    //
    
    if( sysctl( mib, 2, &info.memmax, &length, NULL, 0 ) == 0 )
    {
        info.memmax /= 1024;         // make it GiB
    }
    else {
        info.memmax = 0;
        perror("sysctl");
    }
    
    //
    //  get used memory
    //
    
    mach_port = mach_host_self();
    count = sizeof(vm_stats) / sizeof(natural_t);
    if (KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
        KERN_SUCCESS == host_statistics64(mach_port, HOST_VM_INFO,
                                          (host_info64_t)&vm_stats, &count))
    {
        info.mem = ((int64_t)vm_stats.active_count +
                    (int64_t)vm_stats.inactive_count +
                    (int64_t)vm_stats.wire_count) *  (int64_t)page_size;
        
        info.mem /= 1024;        // make it GiB
    } else {
        info.mem = 0;  // this is to indicate error getting the free mem.
    }
    
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

int update_net_stats(void)
{
    printf("update_net_stats: STUB\n");
    return 0;
}

int update_threads(void)
{
    helper_update_threads_processes();
    return 0;
}

int update_running_threads(void)
{
    return 0;
}

int update_total_processes(void)
{
    helper_update_threads_processes();
    return 0;
    
    /*
     *  WARNING: You may stumble upon this implementation:
     *  https://stackoverflow.com/questions/8141913/is-there-a-lightweight-way-to-obtain-the-current-number-of-processes-in-linux
     *
     *  This method DOESN'T find the correct number of tasks.
     *
     *  This is probably (??) because on macOS there is no option for KERN_PROC_KTHREAD like there is in FreeBSD
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
     *  KERN_PROC_ALL		everything
     *  KERN_PROC_PID		by process id
     *  KERN_PROC_PGRP      by process group id
     *  KERN_PROC_SESSION	by session of pid
     *  KERN_PROC_TTY		by controlling tty
     *  KERN_PROC_UID		by effective uid
     *  KERN_PROC_RUID      by real uid
     *  KERN_PROC_LCID      by login context id
     *
     *  Probably by saying "everything" they mean that KERN_PROC_ALL gives all processes (user-level plus kernel threads)
     *  ( So basically this is the problem with the old implementation )
     */
}

int update_running_processes(void)
{
    struct kinfo_proc *p = NULL;
    int proc_count = 0;
    int run_procs = 0;
    
    proc_count = helper_get_proc_list(&p);
    
    if (proc_count == -1)
        return 0;
    
    for (int i = 0; i < proc_count; i++) {
        if (p[i].kp_proc.p_stat == SRUN)
            run_procs++;
    }
    
    info.run_procs = run_procs;
    
    free(p);
    return 0;
}

/*
 * get_cpu_count
 *
 * The macOS implementation gets the number of active cpus
 * in compliance with linux implementation.
 */
void get_cpu_count(void)
{
    /* XXX
     * Memory leak existed because of allocating memory for info.cpu_usage
     * Fixed by adding check to see if memory has been allocated or not.
     *
     * Probably move the info.cpu_usage allocation inside the update_cpu_usage() function...
     * Why is it here anyway?
     */
    
    int cpu_count = 0;
    
    if (GETSYSCTL("hw.activecpu", cpu_count) == 0) {
        info.cpu_count = cpu_count;
    } else {
        fprintf(stderr, "Cannot get hw.activecpu\n");
        info.cpu_count = 0;
    }
    
    if (!info.cpu_usage)
    {
        info.cpu_usage = (float *) malloc((info.cpu_count + 1) * sizeof(float));
        if (info.cpu_usage == NULL) {
            CRIT_ERR(NULL, NULL, "malloc");
        }
    }
}

int update_cpu_usage(void)
{
    return 0;
}

int update_load_average(void)
{
    double v[3];
    
    getloadavg(v, 3);
    
    info.loadavg[0] = (double) v[0];
    info.loadavg[1] = (double) v[1];
    info.loadavg[2] = (double) v[2];
    
    return 0;
}

double get_acpi_temperature(int fd)
{
    printf( "get_acpi_temperature: STUB\n" );
    
    return 0.0;
}

void get_battery_stuff(char *buf, unsigned int n, const char *bat, int item)
{
    printf( "get_battery_stuff: STUB\n" );
}

int get_battery_perct(const char *bat)
{
    printf( "get_battery_perct: STUB\n" );
    
    return 1;
}

double get_battery_perct_bar(struct text_object *obj)
{
    printf( "get_battery_perct_bar: STUB\n" );
    
    return 0.0;
}

int open_acpi_temperature(const char *name)
{
    printf( "open_acpi_temperature: STUB\n" );
    
    (void)name;
    /* Not applicable for FreeBSD. */
    return 0;
}

void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size, const char *adapter)
{
    printf( "get_acpi_ac_adapter: STUB\n" );
}

void get_acpi_fan(char *p_client_buffer, size_t client_buffer_size)
{
    printf( "get_acpi_fan: STUB\n" );
}
/* void */
char get_freq(char *p_client_buffer, size_t client_buffer_size, const char *p_format,
              int divisor, unsigned int cpu)
{
    printf( "get_freq: STUB!\n" );
    
    return 1;
}

#if 0
void update_wifi_stats(void)
{
    printf( "update_wifi_stats: STUB but also in #if 0\n" );
}
#endif

int update_diskio(void)
{
    printf( "update_diskio: STUB\n" );
    return 0;
}

void get_battery_short_status(char *buffer, unsigned int n, const char *bat)
{
    printf( "get_battery_short_status: STUB\n" );
}


int get_entropy_avail(unsigned int * val)
{
    (void)val;
    printf( "get_entropy_avail: STUB\n!" );
    return 1;
}
int get_entropy_poolsize(unsigned int * val)
{
    (void)val;
    printf( "get_entropy_poolsize: STUB\n!" );
    return 1;
}

/******************************************
 *          get top info section          *
 ******************************************/

/*
 * Calculate a process' cpu usage percentage
 */
static void calc_cpu_usage_for_proc(struct process *proc, uint64_t total)
{
    float mul = 100.0;
    if(top_cpu_separate.get(*state))
        mul *= info.cpu_count;
    
    proc->amount = mul * (proc->user_time + proc->kernel_time) / (float) total;
}

/*
 * calculate total CPU usage based on total CPU usage
 * of previous iteration stored inside |process| struct
 */
static void calc_cpu_total(struct process *proc, uint64_t *total)
{
    uint64_t current_total = 0;     /* of current iteration */
    //uint64_t total = 0;             /* delta */
    struct cpusample sample;
    
    get_cpu_sample(&sample);
    current_total = sample.totalUserTime + sample.totalIdleTime + sample.totalSystemTime;
    
    *total = current_total - proc->previous_total_cpu_time;
    proc->previous_total_cpu_time = current_total;
    
    *total = ((*total / sysconf(_SC_CLK_TCK)) * 100) / info.cpu_count;
}

/*
 * calc_cpu_time_for_proc
 *
 * calculates user_time and kernel_time and sets the contents of the |process| struct
 * excessively based on process_parse_stat() from linux.cc
 */
static void calc_cpu_time_for_proc(struct process *process, const struct proc_taskinfo *pti)
{
    unsigned long long user_time = 0;
    unsigned long long kernel_time = 0;
    
    process->user_time = pti->pti_total_user;
    process->kernel_time = pti->pti_total_system;
    
    /* user_time and kernel_time are in nanoseconds, total_cpu_time in centiseconds.
     * Therefore we divide by 10^7 . */
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
 * finds top-information only for one process which is represented by a kinfo_proc struct
 * this function is called mutliple types ( one foreach process ) to implement get_top_info()
 */
static void get_top_info_for_kinfo_proc(struct kinfo_proc *p)
{
    struct process *proc = NULL;
    struct proc_taskinfo pti;
    pid_t pid;
    
    uint64_t t = 0;
    
    pid = p->kp_proc.p_pid;
    proc = get_process(pid);
    
    free_and_zero(proc->name);
    free_and_zero(proc->basename);
    
    proc->name = strndup(p->kp_proc.p_comm, text_buffer_size.get(*state));
    proc->basename = strndup(p->kp_proc.p_comm, text_buffer_size.get(*state));
    proc->uid = p->kp_eproc.e_pcred.p_ruid;
    proc->time_stamp = g_time;
    
    if(sizeof(pti) == proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti)))
    {
        /* vsize, rss are in bytes thus we dont have to convert */
        proc->vsize = pti.pti_virtual_size;
        proc->rss = pti.pti_resident_size;
        
        bool calc_cpu_total_finished = false;
        bool calc_proc_total_finished = false;
        
        /* calc CPU time for process */
        calc_cpu_time_for_proc(proc, &pti);
        
        /* calc total CPU time (considering current process) */
        calc_cpu_total(proc, &t);
        
        /* calc the amount(%) of CPU the process used  */
        calc_cpu_usage_for_proc(proc, t);
    }
}

/* While topless is obviously better, top is also not bad. */

void get_top_info(void)
{
    int                 proc_count = 0;
    struct kinfo_proc   *p = NULL;
    
    /*
     *  get processes count
     *  and create the processes list
     */
    proc_count = helper_get_proc_list(&p);
    
    if (proc_count == -1)
        return;
    
    /*
     *  get top info for-each process
     */
    for (int i = 0; i < proc_count; i++)
    {
        if (!((p[i].kp_proc.p_flag & P_SYSTEM)) && *p[i].kp_proc.p_comm != '\0')
        {
            get_top_info_for_kinfo_proc(&p[i]);
        }
    }
    
    free(p);
}

/*********************************************************************************************
 *                                  System Integrity Protection                              *
 *********************************************************************************************/

#if (MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_9)

/*
 *  Check if a flag is enabled based on the csr_config variable
 *  Also, flip the result on occasion
 */
bool _csr_check(int aMask, bool aFlipflag)
{
    bool bit = (info.csr_config & aMask);
    
    if (aFlipflag)
        return !bit;
    
    return bit;
}

/*
 *  Extract info from the csr_config variable and set the flags struct
 */
void fill_csr_config_flags_struct(void)
{
    info.csr_config_flags.csr_allow_apple_internal         = _csr_check(CSR_ALLOW_APPLE_INTERNAL, 0);
    info.csr_config_flags.csr_allow_untrusted_kexts        = _csr_check(CSR_ALLOW_UNTRUSTED_KEXTS, 1);
    info.csr_config_flags.csr_allow_task_for_pid           = _csr_check(CSR_ALLOW_TASK_FOR_PID, 1);
    info.csr_config_flags.csr_allow_unrestricted_fs        = _csr_check(CSR_ALLOW_UNRESTRICTED_FS, 1);
    info.csr_config_flags.csr_allow_kernel_debugger        = _csr_check(CSR_ALLOW_KERNEL_DEBUGGER, 1);
    info.csr_config_flags.csr_allow_unrestricted_dtrace    = _csr_check(CSR_ALLOW_UNRESTRICTED_DTRACE, 1);
    info.csr_config_flags.csr_allow_unrestricted_nvram     = _csr_check(CSR_ALLOW_UNRESTRICTED_NVRAM, 1);
    info.csr_config_flags.csr_allow_device_configuration   = _csr_check(CSR_ALLOW_DEVICE_CONFIGURATION, 0);
    info.csr_config_flags.csr_allow_any_recovery_os        = _csr_check(CSR_ALLOW_ANY_RECOVERY_OS, 1);
    info.csr_config_flags.csr_allow_user_approved_kexts    = _csr_check(CSR_ALLOW_UNAPPROVED_KEXTS, 1);
}

/*
 *  Get SIP configuration   ( sets csr_config and csr_config_flags )
 */
int get_sip_status(void)
{
    if (csr_get_active_config == nullptr)   /*  check if weakly linked symbol exists    */
    {
        NORM_ERR("$sip_status will not work on this version of macOS\n");
        return 0;
    }
    
    csr_get_active_config(&info.csr_config);
    fill_csr_config_flags_struct();
    
    return 0;
}

/*
 *  Prints SIP status or a specific SIP feature status depending on the argument passed
 *      to $sip_status command
 *
 *  Variables that can be passed to $sip_status command
 *
 *  nothing --> print enabled / disabled
 *  0   --> allow_apple_internal
 *  1   --> allow_untrusted_kexts
 *  2   --> allow_task_for_pid
 *  3   --> allow_unrestricted_fs
 *  4   --> allow_kernel_debugger
 *  5   --> allow_unrestricted_dtrace
 *  6   --> allow_unrestricted_nvram
 *  7   --> allow_device_configuration
 *  8   --> allow_any_recovery_os
 *  9   --> allow_user_approved_kexts
 *  a   --> check if unsupported configuration  ---> this is not an apple SIP flag. This is for us.
 *
 *  The print function is designed to show 'YES' if a specific protection measure is ENABLED.
 *  For example, if SIP is configured to disallow untrusted kexts, then our function will print 'YES'.
 *      Thus, it doesnt print 'YES' in the case SIP allows untrusted kexts.
 *
 *  For this reason, your conkyrc should say for example: Untrusted Kexts Protection: ${sip_status 1}
 *  You should not write: "Allow Untrusted Kexts", this is wrong.
 */
void print_sip_status(struct text_object *obj, char *p, int p_max_size)
{
    if (csr_get_active_config == nullptr)   /*  check if weakly linked symbol exists    */
    {
        snprintf(p, p_max_size, "%s", "unsupported");
        NORM_ERR("$sip_status will not work on this version of macOS\n");
        return;
    }
    
    /* conky window output */
    (void)obj;
    
    if (!obj->data.s)
        return;
    
    if (strlen(obj->data.s) == 0)
    {
        snprintf(p, p_max_size, "%s", (info.csr_config == CSR_VALID_FLAGS) ? "disabled" : "enabled");
    }
    else if(strlen(obj->data.s) == 1)
    {
        switch (obj->data.s[0])
        {
            case '0':
                snprintf(p, p_max_size, "%s", info.csr_config_flags.csr_allow_apple_internal ? "YES" : "NO");
                break;
            case '1':
                snprintf(p, p_max_size, "%s", info.csr_config_flags.csr_allow_untrusted_kexts ? "YES" : "NO");
                break;
            case '2':
                snprintf(p, p_max_size, "%s", info.csr_config_flags.csr_allow_task_for_pid ? "YES" : "NO");
                break;
            case '3':
                snprintf(p, p_max_size, "%s", info.csr_config_flags.csr_allow_unrestricted_fs ? "YES" : "NO");
                break;
            case '4':
                snprintf(p, p_max_size, "%s", info.csr_config_flags.csr_allow_kernel_debugger ? "YES" : "NO");
                break;
            case '5':
                snprintf(p, p_max_size, "%s", info.csr_config_flags.csr_allow_unrestricted_dtrace ? "YES" : "NO");
                break;
            case '6':
                snprintf(p, p_max_size, "%s", info.csr_config_flags.csr_allow_unrestricted_nvram ? "YES" : "NO");
                break;
            case '7':
                snprintf(p, p_max_size, "%s", info.csr_config_flags.csr_allow_device_configuration ? "YES" : "NO");
                break;
            case '8':
                snprintf(p, p_max_size, "%s", info.csr_config_flags.csr_allow_any_recovery_os ? "YES" : "NO");
                break;
            case '9':
                snprintf(p, p_max_size, "%s", info.csr_config_flags.csr_allow_user_approved_kexts ? "YES" : "NO");
                break;
            case 'a':
                snprintf(p, p_max_size, "%s", (info.csr_config && (info.csr_config != CSR_ALLOW_APPLE_INTERNAL)) ? "unsupported configuration, beware!" : "configuration is ok");
                break;
            default:
                snprintf(p, p_max_size, "%s", "unsupported");
                NORM_ERR("print_sip_status: unsupported argument passed to $sip_status");
                break;
        }
    } else {    /* bad argument */
        snprintf(p, p_max_size, "%s", "unsupported");
        NORM_ERR("print_sip_status: unsupported argument passed to $sip_status");
    }
}

#else   /* Mavericks and before */
/*
 *  Versions prior to Yosemite DONT EVEN DEFINE csr_get_active_config() function.  Thus we must avoid calling this function!
 */

int get_sip_status(void)
{
    /* Does not do anything intentionally */
    return 0;
}

void print_sip_status(struct text_object *obj, char *p, int p_max_size)
{
    /* conky window output */
    (void)obj;
    
    if (!obj->data.s)
        return;
    
    if (strlen(obj->data.s) == 0)
    {
        snprintf(p, p_max_size, "%s", "error unsupported");
    }
    else if(strlen(obj->data.s) == 1)
    {
        switch (obj->data.s[0])
        {
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
                NORM_ERR("print_sip_status: unsupported argument passed to $sip_status");
                break;
        }
    } 
    else 
    {    /* bad argument */
        snprintf(p, p_max_size, "%s", "unsupported");
        NORM_ERR("print_sip_status: unsupported argument passed to $sip_status");
    }
}

#endif
