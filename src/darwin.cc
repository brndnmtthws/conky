//
//	This is the equivalent of linux.cc, freebsd.cc, openbsd.cc etc. ( you get the idea )
//

//
//  ** TODO ** Probably add MenuMeters support here!
//

#include "darwin.h"
#include "conky.h"      // for struct info

#include <stdio.h>
#include <sys/mount.h>      // statfs
#include <sys/sysctl.h>

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#define	GETSYSCTL(name, var)	getsysctl(name, &(var), sizeof(var))

static int getsysctl(const char *name, void *ptr, size_t len)
{
    //printf( "getsysctl: %s\n", name );
    
    size_t nlen = len;
    
    if (sysctlbyname(name, ptr, &nlen, NULL, 0) == -1) {
        return -1;
    }
    
    if (nlen != len && errno == ENOMEM) {
        return -1;
    }
    
    return 0;
}

#include <sys/stat.h>

// TODO: fix update_meminfo for getting the same stats as Activity Monitor's

//
//  ** TODO ** Find a way to add user the option to print stats about the actual swapfile in /private/var/vm
//

// TODO: handle multiple swap files
// TODO: add code for reading the plist and getting swapfile location (may be custom location, who knows)


/*
 *  MacOSX can be using many swapfiles thus calling swapmode() only for one swapfile is irrational.
 *
 *      We need to conform to conky's implementation of swapmode() but also should add the ability to print stats
 *          for all swapfiles.
 *      This will require introducing new functions for using in conkyrc file.
 *
 *      Internally we could a function like this:
 *
 *      int swapmode( int swapfd, unsigned long *retavail, unsigned long *retfree )
 *
 *      ( Takes swapfile-descriptor and the usual parameters )
 */

int swapmode( int swapfd, unsigned long *retavail, unsigned long *retfree )
{
    //  NOTE:
    //      Unlike most Unix-based operating systems, Mac OS X does not use a preallocated swap partition for virtual memory.
    //      Instead, it uses all of the available space on the machine’s boot partition.
    //
    //      macOS can use the whole partition BUT it creates smaller files ( swapfiles ) which theoretically can use the whole partition.
    //      Thus retavail= sizeof(swapfile) and retfree= retavail - used
    //
    
    /*
     *  Currently at macOS Sierra the swapfile location can be determined
     *  by reading the ProgramArguments from /System/Library/LaunchDaemons/com.apple.dynamic_pager.plist
     *
     */
    
    // TODO: The sysctl is supposed to work from Tiger to Sierra
    // TODO: Update for BOTH 'swapfile0' and 'swapfile' names because on my system it used to say swapfile but now it says swapfile0! ( I think )
    
    char default_filename[] = "/private/var/vm/swapfile";
    char actual_filename[strlen(default_filename)+1];
    
    strcpy( actual_filename, default_filename);
    actual_filename[strlen(default_filename)] = swapfd + '0';
    actual_filename[strlen(default_filename)+1] = '\0';
    
    printf( "swapmode: getting swap stat for %s\n", actual_filename );  // dbg
    
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

/*
 *  Returns swapfile stats only for first swapfile (swapfd = 0)
 *
 */
int swapmode_swapfile0(unsigned long *retavail, unsigned long *retfree)
{
    return swapmode(0, retavail, retfree);
}

/*
 *  Function needed by conky
 *  We patch this function for now, to return stats only for swapfile0 because conky doesnt
 *      support multi-swapfile systems such as MacOSX
 *
 */
static int swapmode(unsigned long *retavail, unsigned long *retfree)
{
    return swapmode_swapfile0(retavail, retfree);
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

int update_total_processes(void)
{
    printf( "update_total_processes: STUB\n" );
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

int update_cpu_usage(void)
{
    printf( "update_cpu_usage: STUB\n" );
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
