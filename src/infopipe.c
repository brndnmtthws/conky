/* -------------------------------------------------------------------------
 * infopipe.c: conky support for infopipe plugin
 *
 * Copyright (C) 2005  Philip Kovacs kovacsp3@comcast.net
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

#include "config.h"
#include "conky.h"
#include "infopipe.h"

#define INFOPIPE_NAMED_PIPE "/tmp/xmms-info"

/* 14 keys comprise the output of the infopipe plugin. */
enum _infopipe_keys {
        INFOPIPE_KEY_PROTOCOL=0,
        INFOPIPE_KEY_VERSION,
        INFOPIPE_KEY_STATUS,
        INFOPIPE_KEY_PLAYLIST_TUNES,
        INFOPIPE_KEY_PLAYLIST_CURRTUNE,
        INFOPIPE_KEY_USEC_POSITION,
        INFOPIPE_KEY_POSITION,
        INFOPIPE_KEY_USEC_TIME,
	INFOPIPE_KEY_TIME,
        INFOPIPE_KEY_BITRATE,
        INFOPIPE_KEY_FREQUENCY,
        INFOPIPE_KEY_CHANNELS,
        INFOPIPE_KEY_TITLE,
        INFOPIPE_KEY_FILE
};

/* access to this item array is synchronized */
static infopipe_t infopipe_items;

/* ----------------------------------------
 * Conky update function for infopipe data.
 * ---------------------------------------- */
void update_infopipe(void)
{
    /* 
      The worker thread is updating the infopipe_items array asynchronously to the main 
      conky thread.  We merely copy the infopipe_items array into the main thread's info
      structure when the main thread's update cycle fires.
    */
    pthread_mutex_lock(&info.infopipe.item_mutex);
    memcpy(&info.infopipe.items,infopipe_items,sizeof(infopipe_items));
    pthread_mutex_unlock(&info.infopipe.item_mutex);
}


/* ------------------------------------------------------------
 * Create a worker thread for infopipe media player status.
 *
 * Returns 0 on success, -1 on error. 
 * ------------------------------------------------------------*/
int create_infopipe_thread(void)
{
    /* Is a worker is thread already running? */
    if (info.infopipe.thread)
	return(-1);

    /* Joinable thread for infopipe activity */
    pthread_attr_init(&info.infopipe.thread_attr);
    pthread_attr_setdetachstate(&info.infopipe.thread_attr, PTHREAD_CREATE_JOINABLE);
    /* Init mutexes */
    pthread_mutex_init(&info.infopipe.item_mutex, NULL);
    pthread_mutex_init(&info.infopipe.runnable_mutex, NULL);
    /* Init runnable condition for worker thread */
    pthread_mutex_lock(&info.infopipe.runnable_mutex);
    info.infopipe.runnable=1;
    pthread_mutex_unlock(&info.infopipe.runnable_mutex);
    if (pthread_create(&info.infopipe.thread, &info.infopipe.thread_attr, infopipe_thread_func, NULL))
        return(-1);

    return 0;
}

/* ------------------------------------------------
 * Destroy infopipe status thread.
 *
 * Returns 0 on success, -1 on error.
 * ------------------------------------------------ */
int destroy_infopipe_thread(void)
{
    /* Is a worker is thread running? If not, no error. */
    if (!info.infopipe.thread)
        return(0);

    /* Signal infopipe worker thread to terminate */
    pthread_mutex_lock(&info.infopipe.runnable_mutex);
    info.infopipe.runnable=0;
    pthread_mutex_unlock(&info.infopipe.runnable_mutex);
    /* Destroy thread attribute and wait for thread */
    pthread_attr_destroy(&info.infopipe.thread_attr);
    if (pthread_join(info.infopipe.thread, NULL))
        return(-1);
    /* Destroy mutexes */
    pthread_mutex_destroy(&info.infopipe.item_mutex);
    pthread_mutex_destroy(&info.infopipe.runnable_mutex);

    info.infopipe.thread=(pthread_t)0;
    return 0;
}

/* --------------------------------------------------
 * Worker thread function for InfoPipe data sampling.
 * -------------------------------------------------- */ 
void *infopipe_thread_func(void *pvoid)
{
    int i,rc,fd,runnable;
    fd_set readset;
    struct timeval tm;
    static char buf[2048],line[128];
    static infopipe_t items;
    char *pbuf,c;

    pvoid=(void*)pvoid; /* avoid warning */

    /* Grab the runnable signal.  Should be non-zero here or we do nothing. */
    pthread_mutex_lock(&info.infopipe.runnable_mutex);
    runnable=info.infopipe.runnable;
    pthread_mutex_unlock(&info.infopipe.runnable_mutex );

    /* Loop until the main thread sets the runnable signal to 0. */
    while(runnable) {

	for (;;) {  /* convenience loop so we can break below */

	    memset(buf,0,sizeof(buf));

	    if ((fd=open(INFOPIPE_NAMED_PIPE, O_RDONLY | O_NONBLOCK)) < 0) {
		/* InfoPipe is not running */
		memset(items,0,sizeof(items));
		strcpy(items[INFOPIPE_STATUS],"Not running");
	        break;
	    }

            FD_ZERO(&readset);
	    FD_SET(fd,&readset);

	    /* On Linux, select() reduces the timer by the amount of time not slept,
	     * so we must reset the timer with each loop. */
	    tm.tv_sec=1;
	    tm.tv_usec=0;
	    rc=select(fd+1,&readset,NULL,NULL,&tm);

	    if (rc == -1) {
		/* -- debug -- 
		perror("infopipe select()"); 
		*/
	    }
	    else if (rc && FD_ISSET(fd,&readset)) {  /* ready to read */

                if (read(fd,buf,sizeof(buf)) > 0) { /* buf has data */
		    
		    pbuf=buf;
		    for (i=0;i<14;i++) {
			/* 14 lines of key: value pairs presented in a known order */
			memset(line,0,sizeof(line));
                        if ( sscanf(pbuf,"%*[^:]: %[^\n]",line) == EOF )
			    break;
		        while((c = *pbuf++) && (c != '\n'));

			switch(i) {
			case INFOPIPE_KEY_PROTOCOL:
				break;
                        case INFOPIPE_KEY_VERSION:
				break;
                        case INFOPIPE_KEY_STATUS:
				strncpy(items[INFOPIPE_STATUS],line,sizeof(items[INFOPIPE_STATUS])-1);
				break;
                        case INFOPIPE_KEY_PLAYLIST_TUNES:
				strncpy(items[INFOPIPE_PLAYLIST_LENGTH],line,sizeof(items[INFOPIPE_PLAYLIST_LENGTH])-1);
				break;
                        case INFOPIPE_KEY_PLAYLIST_CURRTUNE:
				strncpy(items[INFOPIPE_PLAYLIST_POSITION],line,sizeof(items[INFOPIPE_PLAYLIST_POSITION])-1);
				break;
                        case INFOPIPE_KEY_USEC_POSITION:
				snprintf(items[INFOPIPE_POSITION_SECONDS],sizeof(items[INFOPIPE_POSITION_SECONDS])-1,
					 "%d", atoi(line) / 1000);
				break;
                        case INFOPIPE_KEY_POSITION:
				strncpy(items[INFOPIPE_POSITION],line,sizeof(items[INFOPIPE_POSITION])-1);
				break;
                        case INFOPIPE_KEY_USEC_TIME:
				snprintf(items[INFOPIPE_LENGTH_SECONDS],sizeof(items[INFOPIPE_LENGTH_SECONDS])-1,
					 "%d", atoi(line) / 1000);
				break;
                        case INFOPIPE_KEY_TIME:
			 	strncpy(items[INFOPIPE_LENGTH],line,sizeof(items[INFOPIPE_LENGTH])-1);
				break;
                        case INFOPIPE_KEY_BITRATE:
				strncpy(items[INFOPIPE_BITRATE],line,sizeof(items[INFOPIPE_BITRATE])-1);
				break;
                        case INFOPIPE_KEY_FREQUENCY:
				strncpy(items[INFOPIPE_FREQUENCY],line,sizeof(items[INFOPIPE_FREQUENCY])-1);
				break;
                        case INFOPIPE_KEY_CHANNELS:
				strncpy(items[INFOPIPE_CHANNELS],line,sizeof(items[INFOPIPE_CHANNELS])-1);
				break;
                        case INFOPIPE_KEY_TITLE:
				strncpy(items[INFOPIPE_TITLE],line,sizeof(items[INFOPIPE_TITLE])-1);
				break;
                        case INFOPIPE_KEY_FILE:
				strncpy(items[INFOPIPE_FILENAME],line,sizeof(items[INFOPIPE_FILENAME])-1);
				break;
			default:
			    break;     
			}
		    }

                    /* -- debug --
		    for(i=0;i<14;i++)
		        printf("%s\n",items[i]);
                    */
		} 
	    }
	    else {
		/* -- debug --
		printf("no infopipe data\n"); 
		*/
            }

	    close(fd);

	    break;
        }

	/* Deliver the refreshed items array to g_items. */
	pthread_mutex_lock(&info.infopipe.item_mutex);
        memcpy(&infopipe_items,items,sizeof(items));
	pthread_mutex_unlock(&info.infopipe.item_mutex);

	/* Grab the runnable signal for next loop. */
        pthread_mutex_lock(&info.infopipe.runnable_mutex);
        runnable=info.infopipe.runnable;
        pthread_mutex_unlock(&info.infopipe.runnable_mutex);

	sleep(1);
    }

    pthread_exit(NULL);
}
