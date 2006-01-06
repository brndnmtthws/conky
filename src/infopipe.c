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
static infopipe_t g_items;

/* ----------------------------------------
 * Conky update function for InfoPipe data.
 * ---------------------------------------- */
void update_infopipe(void)
{
    /* 
      The worker thread is updating ihe g_items array asynchronously to the main 
      conky thread.  We merely copy the g_items array into the main thread's info
      structure when the main thread's update cycle fires.   Note that using the
      mutexes here makes it easier since we won't have to do any sync in conky.c.
    */
    pthread_mutex_lock(&info.infopipe.item_mutex);
    memcpy(&info.infopipe.items,g_items,sizeof(g_items));
    pthread_mutex_unlock(&info.infopipe.item_mutex);
}


/* --------------------------------------------------
 * Worker thread function for InfoPipe data sampling.
 * -------------------------------------------------- */ 
void *infopipe_thread_func(void *pvoid)
{
    int i,rc,fd,runnable;
    fd_set readset;
    struct timeval tm;
    static char buf[2048];  /* should equal or exceed sizeof(infopipe_t) */
    static infopipe_t items;
    char *pbuf;

    pvoid=(void*)pvoid; /* useless cast to avoid unused var warning */

    /* I/O multiplexing timer */
    tm.tv_sec=30;  /* high enough to reduce persistent select() failures */
    tm.tv_usec=0;

    /* Grab the runnable signal.  Should be non-zero here or we do nothing. */
    pthread_mutex_lock(&info.infopipe.runnable_mutex);
    runnable=info.infopipe.runnable;
    pthread_mutex_unlock(&info.infopipe.runnable_mutex );

    /* Loop until the main thread sets the runnable signal to 0. */
    while(runnable) {

	for (;;) {  /* convenience loop so we can break below */

	    memset(buf,0,sizeof(buf));

	    if ((fd=open(INFOPIPE_NAMED_PIPE, O_RDONLY | O_NONBLOCK)) < 0) {
		memset(items,0,sizeof(items));
		strcpy(items[INFOPIPE_STATUS],"Not running");
	        break;
	    }

            FD_ZERO(&readset);
	    FD_SET(fd,&readset);

	    /* The select() below can block for time tm and is ideally suited
               for a worker thread such as this.  We don't want to slow down
               user interface updates in the main thread as there is already 
	       excess latency there. */
	    rc=select(fd+1,&readset,NULL,NULL,&tm);
	    if (rc == -1)
		perror("infopipe select()");
	    else if (rc && FD_ISSET(fd,&readset)) {
		    
                if (read(fd,buf,sizeof(buf)) > 0) { /* buf has data */
		    
		    pbuf=buf;
		    for (i=0;i<14;i++) {
			/* 14 lines of key: value pairs presented in a known order */
                        sscanf(pbuf,"%*[^:]: %[^\n]",items[i]);
		        while(*pbuf++ != '\n');
		    }

                    /* -- debug to console --
		    for(i=0;i<14;i++)
		        printf("%s\n",items[i]);
                    */
		} 
	    }
	    else
		printf("no infopipe data to read.\n");

	    close(fd);

	    break;
        }

	/* Deliver the refreshed items array to g_items. */
	pthread_mutex_lock(&info.infopipe.item_mutex);
        memcpy(&g_items,items,sizeof(items));
	pthread_mutex_unlock(&info.infopipe.item_mutex);

	/* Grab the runnable signal for next loop. */
        pthread_mutex_lock(&info.infopipe.runnable_mutex);
        runnable=info.infopipe.runnable;
        pthread_mutex_unlock(&info.infopipe.runnable_mutex);

	sleep(1);
    }

    pthread_exit(NULL);
}
