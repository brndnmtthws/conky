/*
 * top.c a slightly modified wmtop.c -- copied from the WindowMaker and gkrelltop
 * 
 * Modified by Brenden Matthews
 *
 * Modified by Adi Zaimi
 *
 * Derived by Dan Piponi dan@tanelorn.demon.co.uk
 * http://www.tanelorn.demon.co.uk
 * http://wmtop.sourceforge.net 
 * from code originally contained in wmsysmon by Dave Clark 
(clarkd@skynet.ca)
 * This software is licensed through the GNU General Public License.
 */

/*
 * Ensure there's an operating system defined. There is *no* default
 * because every OS has it's own way of revealing CPU/memory usage.
 * compile with gcc -DOS ...
 */

/******************************************/
/* Includes                               */
/******************************************/

#include "conky.h"
#define CPU_THRESHHOLD   0	/* threshhold for the cpu diff to appear */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <regex.h>

/******************************************/
/* Defines                                */
/******************************************/


/*
 * XXX: I shouldn't really use this BUFFER_LEN variable but scanf is so
 * lame and it'll take me a while to write a replacement.
 */
#define BUFFER_LEN 1024

#define PROCFS_TEMPLATE "/proc/%d/stat"
#define PROCFS_TEMPLATE_MEM "/proc/%d/statm"
#define PROCFS_CMDLINE_TEMPLATE "/proc/%d/cmdline"


/******************************************/
/* Globals                                */
/******************************************/







/******************************************/
/* Process class                          */
/******************************************/

/*
 * Pointer to head of process list
 */
void process_find_top(struct process **);
