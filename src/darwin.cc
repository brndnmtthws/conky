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

//  darwin.cc
//  Nickolas Pylarinos
//
//	This is the equivalent of linux.cc, freebsd.cc, openbsd.cc etc. ( you get the idea )
//  For implementing functions I took ideas from FreeBSD.cc! Thanks for the great code!
//

// keywords used are TODO, FIXME, BUG

// TODO: fix update_meminfo for getting the same stats as Activity Monitor's --- There is small difference though
// TODO: convert get_cpu_count() to use mib instead of namedsysctl
// TODO: test getcpucount further   -- Changed to hw.logicalcpumax
// TODO: see linux.cc for more info into implementation of certain functions

#include "darwin.h"
#include "conky.h"              // for struct info

#include <stdio.h>
#include <sys/mount.h>          // statfs
#include <sys/sysctl.h>

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#include <mach/mach.h>          // update_total_processes

#include <libproc.h>            // get_top_info
#include "top.h"                // get_top_info

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
 *  o   Every swapfile has size of 1GB
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
    
    // NO, this seems to be normal because probably conky does some kind of rounding to the total swap size value => 2048MB becomes 2.00GB----- BUG: swapmode doesnt find correct swap size --- This is probably a problem of the sysctl implementation. ( it happens with htop port for macOS )
    //          MenuMeters sums the size of all the swapfiles to solve this problem.
    // THUS, there is no need in implementing a more accurate implementation because conky will always round the values...
    
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
        info.memmax /= 1024;         // make it GiB
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
    printf( "update_net_stats: STUB\n" );
    return 0;
}

//
//  NOTE: This function is disabled for now, conky isnt using it.
//
int update_threads(void)
{
    // TODO: add deallocation section for deallocating when conky exits!
    
    static bool machStuffInitialised = false;                   /*
                                                                 *  Set this to true when the block that initialises machHost and processorSet has executed ONCE.
                                                                 *  This way we ensure that upon each update_total_processes() only ONCE the block executes.
                                                                 */
    
    static host_name_port_t             machHost;               /* make them static to keep the local and at the same time keep their initial value */
    static processor_set_name_port_t	processorSet = 0;
    
    
    /* FIXED but find a better solution ---- FIXME: This block should be happening outside and only ONCE, when conky starts. */
    
    if (!machStuffInitialised)
    {
        printf( "\n\n\nRunning ONLY ONCE the mach--init block\n\n\n" );
        
        // Set up our mach host and default processor set for later calls
        machHost = mach_host_self();
        processor_set_default(machHost, &processorSet);
        
        machStuffInitialised = true;
    }
    
    // get count of ALL threads!
    struct processor_set_load_info loadInfo;
    mach_msg_type_number_t count = PROCESSOR_SET_LOAD_INFO_COUNT;
    kern_return_t err = processor_set_statistics(processorSet, PROCESSOR_SET_LOAD_INFO,
                                                 (processor_set_info_t)&loadInfo, &count);
    
    if (err == KERN_SUCCESS) {                      // NOTE: Maybe this check could be removed!
        info.threads = loadInfo.thread_count;
        printf( "conky: got thread count: %i\n", loadInfo.thread_count );
    }
    
    return 0;
}

//
//  NOTE: This function is disabled for now, conky isnt using it.
//
int update_running_threads(void)
{
    return 0;
}

//
//  NOTE: finds thread count, too, for now!
//
int update_total_processes(void)
{
    // TODO: add deallocation section for deallocating when conky exits!
    
    static bool machStuffInitialised = false;                   /*
                                                                 *  Set this to true when the block that initialises machHost and processorSet has executed ONCE.
                                                                 *  This way we ensure that upon each update_total_processes() only ONCE the block executes.
                                                                 */
    
    static host_name_port_t             machHost;               /* make them static to keep the local and at the same time keep their initial value */
    static processor_set_name_port_t	processorSet = 0;
 
    
    /* FIXED but find a better solution ---- FIXME: This block should be happening outside and only ONCE, when conky starts. */
    
    if (!machStuffInitialised)
    {
        printf( "\n\n\nRunning ONLY ONCE the mach--init block\n\n\n" );
        
        // Set up our mach host and default processor set for later calls
        machHost = mach_host_self();
        processor_set_default(machHost, &processorSet);
        
        machStuffInitialised = true;
    }
    
    // get count of ALL tasks
    struct processor_set_load_info loadInfo;
    mach_msg_type_number_t count = PROCESSOR_SET_LOAD_INFO_COUNT;
    kern_return_t err = processor_set_statistics(processorSet, PROCESSOR_SET_LOAD_INFO,
                                                 (processor_set_info_t)&loadInfo, &count);
    
    if (err == KERN_SUCCESS) {                      // NOTE: Maybe this check could be removed!
        info.procs = loadInfo.task_count;
        info.threads = loadInfo.thread_count;       // TODO: This will need to be moved inside update_threads() and core.cc must be tweaked to enable update_threads() for macOS
    }
    
    return 0;
    
    // IMPLEMENTATION WAY NO.2 is problematic for **US**
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
}

int update_running_processes(void)
{
    int err = 0;
    struct kinfo_proc *p = NULL;
    size_t length = 0;
    
    static const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    
    // Call sysctl with a NULL buffer to get proper length
    
    err = sysctl((int *)name, (sizeof(name) / sizeof(*name)) - 1, NULL, &length, NULL, 0);
    if (err) {
        perror(NULL);
        free(p);
        return 0;
    }
    
    // Allocate buffer
    p = (kinfo_proc*)malloc(length);
    if (!p) {
        perror(NULL);
        free(p);
        return 0;
    }
    
    // Get the actual process list
    err = sysctl((int *)name, (sizeof(name) / sizeof(*name)) - 1, p, &length, NULL, 0);
    if (err)
    {
        perror(NULL);
        free(p);
        return 0;
    }
    
    int proc_count = length / sizeof(struct kinfo_proc);
    
    int run_procs = 0;
    
    for (int i = 0; i < proc_count; i++) {
        if (p[i].kp_proc.p_stat == SRUN)
            run_procs++;
    }
    
    info.run_procs = run_procs;
    
    free(p);
    return 0;
}


//
//  Gets number of max logical cpus that could be available at this boot
//
void get_cpu_count(void)
{
    //  Darwin man page for sysctl:
    //
    //  hw.ncpu:
    //  The number of cpus. This attribute is **DEPRECATED** and it is recom-
    //  mended that hw.logicalcpu, hw.logicalcpu_max, hw.physicalcpu, or
    //  hw.physicalcpu_max be used instead.
    //
    
    int cpu_count = 0;
    
    if (GETSYSCTL("hw.logicalcpu_max", cpu_count) == 0) {
        info.cpu_count = cpu_count;
    } else {
        fprintf(stderr, "Cannot get hw.logicalcpu_max\n");
        info.cpu_count = 0;
    }
    
    info.cpu_usage = (float *) malloc((info.cpu_count + 1) * sizeof(float));
    if (info.cpu_usage == NULL) {
        CRIT_ERR(NULL, NULL, "malloc");
    }
    
    printf( "get_cpu_count: %i\n", info.cpu_count );
}


#define CPU_SAMPLE_COUNT 15
struct cpu_info {
    unsigned long long cpu_user;
    unsigned long long cpu_system;
    unsigned long long cpu_nice;
    unsigned long long cpu_idle;
    unsigned long long cpu_iowait;
    unsigned long long cpu_irq;
    unsigned long long cpu_softirq;
    unsigned long long cpu_steal;
    unsigned long long cpu_total;
    unsigned long long cpu_active_total;
    unsigned long long cpu_last_total;
    unsigned long long cpu_last_active_total;
    double cpu_val[CPU_SAMPLE_COUNT];
};

int update_cpu_usage(void)
{
    // TODO: add deallocation section for deallocating when conky exits!
    
    //
    //  Help taken from https://stackoverflow.com/questions/6785069/get-cpu-percent-usage?noredirect=1&lq=1 and freebsd.h and linux.cc
    //
    
    processor_info_array_t cpuInfo;
    mach_msg_type_number_t numCpuInfo;
    
    static short cpu_setup = 0;     // in FreeBSD.h this is public
    
    /* add check for !info.cpu_usage since that mem is freed on a SIGUSR1 */
    if ((cpu_setup == 0) || (!info.cpu_usage)) {
        get_cpu_count();
        cpu_setup = 1;
    }
    
    natural_t numCPUsU = 0U;
    kern_return_t err = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &numCPUsU, &cpuInfo, &numCpuInfo);

    if (err != ERR_SUCCESS) {
        printf("update_cpu_usage: error\n");
        return 0;
    }

    int j = 0;
    extern void* global_cpu;
    static struct cpu_info *cpu = NULL;
    double curtmp;
    double delta;
    
    if (!global_cpu) {
        int malloc_cpu_size = (info.cpu_count + 1) * sizeof(struct cpu_info);
        cpu = (struct cpu_info *)malloc(malloc_cpu_size);
        memset(cpu, 0, malloc_cpu_size);
        global_cpu = cpu;
    }
    
    for(unsigned i = 0U; i < numCPUsU; ++i) {
        cpu[i].cpu_user = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_USER];
        cpu[i].cpu_nice = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_NICE];
        cpu[i].cpu_system = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_SYSTEM];
        cpu[i].cpu_idle = cpuInfo[(CPU_STATE_MAX * i) + CPU_STATE_IDLE];

        cpu[i].cpu_total = cpu[i].cpu_user + cpu[i].cpu_nice +
        cpu[i].cpu_system + cpu[i].cpu_idle +
        cpu[i].cpu_iowait + cpu[i].cpu_irq +
        cpu[i].cpu_softirq + cpu[i].cpu_steal;
        
        cpu[i].cpu_active_total = cpu[i].cpu_total -
        (cpu[i].cpu_idle + cpu[i].cpu_iowait);
        
        delta = current_update_time - last_update_time;
        
        if (delta <= 0.001) {   // TODO: should we set the values to 0 here???
            break;
        }
        
        cpu[i].cpu_val[0] = (cpu[i].cpu_active_total -
                               cpu[i].cpu_last_active_total) /
        (float) (cpu[i].cpu_total - cpu[i].cpu_last_total);
        curtmp = 0;
        
        int samples = cpu_avg_samples.get(*state);
#ifdef HAVE_OPENMP
#pragma omp parallel for reduction(+:curtmp) schedule(dynamic,10)
#endif /* HAVE_OPENMP */
        for (j = 0; j < samples; j++) {
            curtmp = curtmp + cpu[i].cpu_val[j];
        }
        info.cpu_usage[i] = curtmp / samples;
        
        cpu[i].cpu_last_total = cpu[i].cpu_total;
        cpu[i].cpu_last_active_total = cpu[i].cpu_active_total;
#ifdef HAVE_OPENMP
#pragma omp parallel for schedule(dynamic,10)
#endif /* HAVE_OPENMP */
        for (j = samples - 1; j > 0; j--) {
            cpu[i].cpu_val[j] = cpu[i].cpu_val[j - 1];
        }
    }
    
    return 0;
}

int update_load_average(void)                   /* TODO: experimental */
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

/******************************************
 * Calculate each processes cpu			  *
 ******************************************/

#ifdef BUILD_IOSTATS
static void calc_io_each(void)
{
    //printf("calc_io_each: experimental\n");
    
    struct process *p;
    unsigned long long sum = 0;
    
    for (p = first_process; p; p = p->next)
    sum += p->read_bytes + p->write_bytes;
    
    if(sum == 0)
    sum = 1; /* to avoid having NANs if no I/O occured */
    for (p = first_process; p; p = p->next)
    p->io_perc = 100.0 * (p->read_bytes + p->write_bytes) / (float) sum;
}
#endif /* BUILD_IOSTATS */

/*
 *  get resident memory size (bytes) of a process with a specific pid
 *  helper function for get_top_info()
 */
void conky_get_rss_for_pid( pid_t pid, unsigned long long * rss )
{
    struct proc_taskinfo pti;
    
    if(sizeof(pti) == proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti))) {
        *rss =  pti.pti_resident_size;
    }
}

/* While topless is obviously better, top is also not bad. */

void get_top_info(void)
{
    int err = 0;
    struct kinfo_proc *p = NULL;
    struct process *proc;
    size_t length = 0;
    
    static const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    
    // Call sysctl with a NULL buffer to get proper length
    err = sysctl((int *)name, (sizeof(name) / sizeof(*name)) - 1, NULL, &length, NULL, 0);
    if (err) {
        perror(NULL);
        free(p);
        return;
    }
    
    // Allocate buffer
    p = (kinfo_proc*)malloc(length);
    if (!p) {
        perror(NULL);
        free(p);
        return;
    }
    
    // Get the actual process list
    err = sysctl((int *)name, (sizeof(name) / sizeof(*name)) - 1, p, &length, NULL, 0);
    if (err)
    {
        perror(NULL);
        free(p);
        return;
    }
    
    int proc_count = length / sizeof(struct kinfo_proc);
    
    for (int i = 0; i < proc_count; i++) {
        if (!((p[i].kp_proc.p_flag & P_SYSTEM)) && p[i].kp_proc.p_comm[0] != '\0') {            // TODO: check if this is the right way to do it... I have replaced kp_flag with kp_proc.p_flag though not sure if it is right
            proc = get_process(p[i].kp_proc.p_pid);
            
            proc->time_stamp = g_time;
            proc->name = strndup(p[i].kp_proc.p_comm, text_buffer_size.get(*state));            // TODO: What does this do?
            proc->basename = strndup(p[i].kp_proc.p_comm, text_buffer_size.get(*state));
            proc->amount = 100.0 * p[i].kp_proc.p_pctcpu / FSCALE;
            
            //proc->vsize = 0;  // NOT IMPLEMENTED YET
            
            // TODO: fix this, doesn't seem to provide conky with the desired values ( though I think the conky_get_rss_for_pid function works fine!!! )
            //          ALSO, when you start conky you see that the MEM% column is full of 0.00 but IF YOU type on terminal for example top or do anything it fills some values!!! STRANGE...
            conky_get_rss_for_pid( p[i].kp_proc.p_pid, &proc->rss );        // NOTE: I noticed a small difference between htop's calculation of rss and our implementation's
                                                                            // What is more, i think that the value as is right now is fine... But we may need some more stuff to implement first!
            
            // ki_runtime is in microseconds, total_cpu_time in centiseconds.
            // Therefore we divide by 10000.
            //proc->total_cpu_time = 0;   // NOT IMPLEMENTED YET
        
#ifdef BUILD_IOSTATS
            calc_io_each();			/* percentage of I/O for each task */
#endif /* BUILD_IOSTATS */
        }
    }
    
    free(p);
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
