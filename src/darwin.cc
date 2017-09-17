//  darwin.cc
//  Nickolas Pylarinos
//
//	This is the equivalent of linux.cc, freebsd.cc, openbsd.cc etc. ( you get the idea )
//
//  LICENSED UNDER GPL v3

// TODO: fix update_meminfo for getting the same stats as Activity Monitor's --- There is small difference though
// TODO: update getcpucount as needed
// FIXED --- BUG: update_total_processes: gives different from Activity Monitor

/*  
    Take in consideration:
 
    NSProcessInfo can give the following:
 
    processorCount
    The number of processing cores available on the computer.
    activeProcessorCount
    The number of active processing cores available on the computer.
    physicalMemory
    The amount of physical memory on the computer in bytes.
    systemUptime
    The amount of time the system has been awake since the last time it was restarted.
 */

#include "darwin.h"
#include "conky.h"      // for struct info

/* These will be removed in upcoming versions of conky-for-macOS */
#define	CP_USER		0
#define	CP_NICE		1
#define	CP_SYS		2
#define	CP_INTR		3
#define	CP_IDLE		4
#define	CPUSTATES	5

#include <stdio.h>
#include <sys/mount.h>      // statfs
#include <sys/sysctl.h>

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#include <libproc.h>

#define	GETSYSCTL(name, var)	getsysctl(name, &(var), sizeof(var))

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
 *
 *-------------------------------------------------------------------------------------------------------------------------------------------------------------------
 */

static int swapmode(unsigned long *retavail, unsigned long *retfree)
{
    //  NOTE:
    //      Code should work on Tiger and later...
    //      Based on the theoretical notes written above about swapfiles on macOS:
    //
    //      retavail= sizeof(swapfile) and retfree= ( retavail - used )
    //
    
    // BUG: swapmode doesnt find correct swap size --- This is probably a problem of the sysctl implementation. ( it happens with htop port for macOS )
    //          MenuMeters sums the size of all the swapfiles to solve this problem.
    // **** For now, I will keep the implementation as is, because solving this issue will mean iterating through all files in /private/var/vm/, checking which have the prefix "swapfile"
    //          and using stat to calculate their size. ( This seems alot. )
    
    // TODO: In future release I will add the option for calculating exact size of swap
    
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
        fprintf(stderr, "Could not get uptime\n");
        info.uptime = 0;
    }
    
    return 0;
}

int check_mount(struct text_object *obj)
{
    printf( "check_mount: STUB\n" );
    
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
        info.memmax >>= 10;         // make it GiB
    }
    else {
        info.memmax = 0;
        perror( "sysctl" );
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
        
        info.mem >>= 10;        // make it GiB
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
    printf( "update_net_stats: STUB\n" );
    return 0;
}


#import <mach-o/arch.h>
#import <mach/mach.h>
#import <mach/mach_error.h>


int update_total_processes(void)
{
    /* FIXME: This block should be happening outside and only ONCE, when conky starts. */
    host_name_port_t 					machHost;
    processor_set_name_port_t			processorSet;
    
    {
        // Set up our mach host and default processor set for later calls
        machHost = mach_host_self();
        processor_set_default(machHost, &processorSet);
    }
        
    // get count of tasks
    struct processor_set_load_info loadInfo;
    mach_msg_type_number_t count = PROCESSOR_SET_LOAD_INFO_COUNT;
    kern_return_t err = processor_set_statistics(processorSet, PROCESSOR_SET_LOAD_INFO,
                                                 (processor_set_info_t)&loadInfo, &count);

    if (err == KERN_SUCCESS) {                      // NOTE: Maybe this check could be removed!
        info.procs = loadInfo.task_count;
    }
    
    return 0;
    
    
    // IMPLEMENTATION WAY NO.2 --- Disabled
    // https://stackoverflow.com/questions/8141913/is-there-a-lightweight-way-to-obtain-the-current-number-of-processes-in-linux
    
    //
    //  This method doesnt find the correct number of tasks.
    //
    //  This is probably because on macOS there is no option for KERN_PROC_KTHREAD like there is in FreeBSD
    //
    //  In FreeBSD's sysctl.h we can see the following:
    //
    //  KERN_PROC_KTHREAD   all processes (user-level plus kernel threads)
    //  KERN_PROC_ALL       all user-level processes
    //  KERN_PROC_PID       processes with process ID arg
    //  KERN_PROC_PGRP      processes with process group arg
    //  KERN_PROC_SESSION   processes with session arg
    //  KERN_PROC_TTY       processes with tty(4) arg
    //  KERN_PROC_UID       processes with effective user ID arg
    //  KERN_PROC_RUID      processes with real user ID arg
    //
    //  Though in macOS's sysctl.h there are only:
    //
    //  KERN_PROC_ALL		everything
    //  KERN_PROC_PID		by process id
    //  KERN_PROC_PGRP      by process group id
    //  KERN_PROC_SESSION	by session of pid
    //  KERN_PROC_TTY		by controlling tty
    //  KERN_PROC_UID		by effective uid
    //  KERN_PROC_RUID      by real uid
    //  KERN_PROC_LCID      by login context id
    //
    //  Probably by saying "everything" they mean that KERN_PROC_ALL gives all processes (user-level plus kernel threads)
    //  ( So basically this is the problem with the old implementation )
    //

    
    size_t length = 0;
    static const int names[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL };
    
    info.procs = sysctl( (int *)names, 3, NULL, &length, NULL, 0) == 0
                ? ( length/sizeof(kinfo_proc) )
                : ( 0 );
    
    return 0;
}

int update_running_processes(void)
{
    printf( "update_running_processes: STUB\n" );
    return 0;
}


//
//  Gets number of .... cpus
//
void get_cpu_count(void)
{
    //  Darwin man page for sysctl:
    //
    //  hw.ncpu:
    //  The number of cpus. This attribute is deprecated and it is recom-
    //  mended that hw.logicalcpu, hw.logicalcpu_max, hw.physicalcpu, or
    //  hw.physicalcpu_max be used instead.
    //
    
    int cpu_count = 0;
    //size_t cpu_count_len = sizeof(cpu_count);
    
    if (GETSYSCTL("hw.ncpu", cpu_count) == 0) {
        info.cpu_count = cpu_count;
    } else {
        fprintf(stderr, "Cannot get hw.ncpu\n");
        info.cpu_count = 0;
    }
    
    info.cpu_usage = (float *) malloc((info.cpu_count + 1) * sizeof(float));
    if (info.cpu_usage == NULL) {
        CRIT_ERR(NULL, NULL, "malloc");
    }
    
    printf( "get_cpu_count: %i\n", info.cpu_count );
}

struct cpu_info {
    long oldtotal;
    long oldused;
};

static short cpu_setup = 0;


int update_cpu_usage(void)
{
    /*
     *  Following implementation copied from FreeBSD.cc. Still enabled. To be removed.
     */
    
    int i, j = 0;
    long used, total;
    long *cp_time = NULL;
    size_t cp_len;
    static struct cpu_info *cpu = NULL;
    unsigned int malloc_cpu_size = 0;
    extern void* global_cpu;
    
    /* add check for !info.cpu_usage since that mem is freed on a SIGUSR1 */
    if ((cpu_setup == 0) || (!info.cpu_usage)) {
        get_cpu_count();
        cpu_setup = 1;
    }
    
    if (!global_cpu) {
        malloc_cpu_size = (info.cpu_count + 1) * sizeof(struct cpu_info);
        cpu = (cpu_info *) malloc(malloc_cpu_size);
        memset(cpu, 0, malloc_cpu_size);
        global_cpu = cpu;
    }
    
    /* cpu[0] is overall stats, get it from separate sysctl */
    cp_len = CPUSTATES * sizeof(long);
    cp_time = (long int *) malloc(cp_len);
    
    if (sysctlbyname("kern.cp_time", cp_time, &cp_len, NULL, 0) < 0) {
        fprintf(stderr, "Cannot get kern.cp_time\n");
    }
    
    total = 0;
    for (j = 0; j < CPUSTATES; j++)
        total += cp_time[j];
    
    used = total - cp_time[CP_IDLE];
    
    if ((total - cpu[0].oldtotal) != 0) {
        info.cpu_usage[0] = ((double) (used - cpu[0].oldused)) /
        (double) (total - cpu[0].oldtotal);
    } else {
        info.cpu_usage[0] = 0;
    }
    
    cpu[0].oldused = used;
    cpu[0].oldtotal = total;
    
    free(cp_time);
    
    /* per-core stats */
    cp_len = CPUSTATES * sizeof(long) * info.cpu_count;
    cp_time = (long int *) malloc(cp_len);
    
    /* on e.g. i386 SMP we may have more values than actual cpus; this will just drop extra values */
    if (sysctlbyname("kern.cp_times", cp_time, &cp_len, NULL, 0) < 0 && errno != ENOMEM) {
        fprintf(stderr, "Cannot get kern.cp_times\n");
    }
    
    for (i = 0; i < info.cpu_count; i++)
    {
        total = 0;
        for (j = 0; j < CPUSTATES; j++)
            total += cp_time[i*CPUSTATES + j];
        
        used = total - cp_time[i*CPUSTATES + CP_IDLE];
        
        if ((total - cpu[i+1].oldtotal) != 0) {
            info.cpu_usage[i+1] = ((double) (used - cpu[i+1].oldused)) /
            (double) (total - cpu[i+1].oldtotal);
        } else {
            info.cpu_usage[i+1] = 0;
        }
        
        cpu[i+1].oldused = used;
        cpu[i+1].oldtotal = total;
    }
    
    free(cp_time);
    return 0;
}

int update_load_average(void)
{
    printf( "update_load_average: STUB\n" );

    return 0;
}

double get_acpi_temperature(int fd)
{
    printf( "get_acpi_temperature: STUB\n" );
    
    return 0.0;
}

static void get_battery_stats(int *battime, int *batcapacity, int *batstate, int *ac) {
    printf( "get_battery_stats: STUB\n" );
}

void get_battery_stuff(char *buf, unsigned int n, const char *bat, int item)
{
    printf( "get_battery_stuff: STUB\n" );
}

static int check_bat(const char *bat)
{
    printf( "check_bat: STUB\n" );
    
    return 1;
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

/* While topless is obviously better, top is also not bad. */

void get_top_info(void)
{
    printf( "get_top_info: STUB\n" );
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
