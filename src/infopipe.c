/* -------------------------------------------------------------------------
 * infopipe.c:  conky support for XMMS/BMP InfoPipe plugin
 *
 * http://www.beastwithin.org/users/wwwwolf/code/xmms/infopipe.html
 *
 * Copyright (C) 2005  Philip Kovacs kovacsp3@comcast.net
 * 
 * Based on original ideas and code graciously presented by: 
 * Ulrich Jansen - ulrich( dot )jansen( at )rwth-aachen.de
 *
 * $Id$
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * --------------------------------------------------------------------------- */

#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "conky.h"

/* access to this item array is synchronized with mutexes */
static char g_item[14][256];

/* ----------------------------------------
 * Conky update function for InfoPipe data.
 * ---------------------------------------- */
void update_infopipe(void)
{
    /* 
      The worker thread is updating ihe g_item array asynchronously to the main 
      conky thread.  We merely copy the g_item array into the main thread's info
      structure when the main thread's update cycle fires.   Note that using the
      mutexes here makes it easier since we won't have to do any sync in conky.c.
    */
    pthread_mutex_lock(&info.infopipe.item_mutex);
    memcpy(&info.infopipe.item,g_item,sizeof(g_item));
    pthread_mutex_unlock(&info.infopipe.item_mutex);
}


/* --------------------------------------------------
 * Worker thread function for InfoPipe data sampling.
 * -------------------------------------------------- */ 
void *infopipe_service(void *pvoid)
{
    int i,fd,runnable;
    fd_set readset;
    struct timeval tm;
    char buf[2048],*pbuf;

    pvoid=(void*)pvoid; /* useless cast to avoid unused var warning */

    /* I/O multiplexing timer is set for one second select() */
    tm.tv_sec=1;
    tm.tv_usec=0;

    /* Grab the runnable signal.  Should be non-zero here or we do nothing. */
    pthread_mutex_lock(&info.infopipe.runnable_mutex);
    runnable=info.infopipe.runnable;
    pthread_mutex_unlock(&info.infopipe.runnable_mutex );

    /* Loop until the main thread sets the runnable signal to 0. */
    while(runnable) {

	for (;;) {  /* convenience loop so we can break below */

	    memset(buf,0,sizeof(buf));
	    memset(g_item,0,sizeof(g_item));

	    if ((fd=open(INFOPIPE_NAMED_PIPE, O_RDONLY | O_NONBLOCK)) < 0)
	        break;

            FD_ZERO(&readset);
	    FD_SET(fd,&readset);

	    /* This select() can block for a brief while (tm time value) and is 
               ideally suited for a worker thread such as this. We don't want to 
               slow down ui updates in the main thread as there is already excess 
               latency there. */
            if (select(fd+1,&readset,NULL,NULL,&tm) != 1) /* nothing to read yet */
                break;
		    
            if (read(fd,buf,sizeof(buf))>0) {
		    
		pbuf=buf;
		pthread_mutex_lock(&info.infopipe.item_mutex);
		for (i=0;i<14;i++) {
                    sscanf(pbuf,"%*[^:]: %[^\n]",g_item[i]);
		    while(*pbuf++ != '\n');
		}
		pthread_mutex_unlock(&info.infopipe.item_mutex);

                /* -- debug to console --
		for(i=0;i<14;i++)
		    printf("%s\n",g_item[i]);
                */

	    }

	    if (close(fd)<0)
                break;

	    break;
        }

	sleep(2);   /* need a var here */

	/* Grab the runnable signal for next loop. */
        pthread_mutex_lock(&info.infopipe.runnable_mutex);
        runnable=info.infopipe.runnable;
        pthread_mutex_unlock(&info.infopipe.runnable_mutex);
    }

    pthread_exit(NULL);
}
