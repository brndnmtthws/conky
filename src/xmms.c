/* -------------------------------------------------------------------------
 * xmms.c:  conky support for XMMS-related projects
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

#include "config.h"
#include "conky.h"
#include "xmms.h"

#if defined(XMMS)
#include <xmms/xmmsctrl.h>

#elif defined(BMP)
#include <bmp/beepctrl.h>

#elif defined(AUDACIOUS)
#include <audacious/beepctrl.h>

#elif defined(INFOPIPE)
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

#define INFOPIPE_NAMED_PIPE "/tmp/xmms-info"

/* 14 keys comprise the output of the infopipe plugin. */
enum _infopipe_keys {
        INFOPIPE_PROTOCOL=0,
        INFOPIPE_VERSION,
        INFOPIPE_STATUS,
        INFOPIPE_PLAYLIST_TUNES,
        INFOPIPE_PLAYLIST_CURRTUNE,
        INFOPIPE_USEC_POSITION,
        INFOPIPE_POSITION,
        INFOPIPE_USEC_TIME,
        INFOPIPE_TIME,
        INFOPIPE_BITRATE,
        INFOPIPE_FREQUENCY,
        INFOPIPE_CHANNELS,
        INFOPIPE_TITLE,
        INFOPIPE_FILE
};
#endif


/* access to this item array is synchronized with mutexes */
static xmms_t g_items;

/* ------------------------------------
 * Conky update function for XMMS data.
 * ------------------------------------ */
void update_xmms(void)
{
    /* 
      The worker thread is updating the g_items array asynchronously to the main 
      conky thread.  We merely copy the g_items array into the main thread's info
      structure when the main thread's update cycle fires.   Note that using the
      mutexes here makes it easier since we won't have to do any sync in conky.c.
    */
    pthread_mutex_lock(&info.xmms.item_mutex);
    memcpy(&info.xmms.items,g_items,sizeof(g_items));
    pthread_mutex_unlock(&info.xmms.item_mutex);
}


#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS)
/* ------------------------------------------------------------
 * Worker thread function for XMMS/BMP/Audacious data sampling.
 * ------------------------------------------------------------ */ 
void *xmms_thread_func(void *pvoid)
{
    int runnable;
    static xmms_t items;
    int session,playpos,frames,length;
    int rate,freq,chans;
    char *psong,*pfilename;

    pvoid=(void*)pvoid; /* useless cast to avoid unused var warning */
    session=0;

    /* Grab the runnable signal.  Should be non-zero here or we do nothing. */
    pthread_mutex_lock(&info.xmms.runnable_mutex);
    runnable=info.xmms.runnable;
    pthread_mutex_unlock(&info.xmms.runnable_mutex );

    /* Loop until the main thread sets the runnable signal to 0. */
    while(runnable) {

	for (;;) {  /* convenience loop so we can break below */
	
            if (!xmms_remote_is_running(session)) {
                memset(&items,0,sizeof(items));
		strcpy(items[XMMS_STATUS],"Not running");
		break;
            }

	    /* Player status */
	    if (xmms_remote_is_paused(session))
	        strcpy(items[XMMS_STATUS],"Paused");
	    else if (xmms_remote_is_playing(session))
		 strcpy(items[XMMS_STATUS],"Playing");
	    else
	         strcpy(items[XMMS_STATUS],"Stopped");

	    /* Current song title */
	    playpos = (int) xmms_remote_get_playlist_pos(session);
	    psong = (char *) xmms_remote_get_playlist_title(session, playpos);
	    if (psong)
                strncpy(items[XMMS_TITLE],psong,sizeof(items[XMMS_TITLE]));

	    /* Current song length as MM:SS */ 
            frames = xmms_remote_get_playlist_time(session,playpos);
	    length = frames / 1000;
            snprintf(items[XMMS_LENGTH],sizeof(items[XMMS_LENGTH]),
	             "%d:%.2d", length / 60, length % 60);
	 
	    /* Current song length in seconds */
	    snprintf(items[XMMS_LENGTH_SECONDS],sizeof(items[XMMS_LENGTH_SECONDS]),
	             "%d", length);

	    /* Current song position as MM:SS */ 
            frames = xmms_remote_get_output_time(session);
	    length = frames / 1000;
            snprintf(items[XMMS_POSITION],sizeof(items[XMMS_POSITION]),
	             "%d:%.2d", length / 60, length % 60);
	 
	    /* Current song position in seconds */
	    snprintf(items[XMMS_POSITION_SECONDS],sizeof(items[XMMS_POSITION_SECONDS]),
	             "%d", length);

            /* Current song bitrate */
            xmms_remote_get_info(session, &rate, &freq, &chans);
            snprintf(items[XMMS_BITRATE],sizeof(items[XMMS_BITRATE]), "%d", rate);

            /* Current song frequency */
            snprintf(items[XMMS_FREQUENCY],sizeof(items[XMMS_FREQUENCY]), "%d", freq);

            /* Current song channels */
            snprintf(items[XMMS_CHANNELS],sizeof(items[XMMS_CHANNELS]), "%d", chans);
            
            /* Current song filename */
	    pfilename = xmms_remote_get_playlist_file(session,playpos);
	    strncpy(items[XMMS_FILENAME],pfilename,sizeof(items[XMMS_FILENAME]));

	    /* Length of the Playlist (number of songs) */
	    length = xmms_remote_get_playlist_length(session);
	    snprintf(items[XMMS_PLAYLIST_LENGTH],sizeof(items[XMMS_PLAYLIST_LENGTH]), "%d", length);

	    /* Playlist position (index of song) */
	    snprintf(items[XMMS_PLAYLIST_POSITION],sizeof(items[XMMS_PLAYLIST_POSITION]), "%d", playpos+1);

	    break;
	}

	/* Deliver the refreshed items array to g_items. */
	pthread_mutex_lock(&info.xmms.item_mutex);
        memcpy(&g_items,items,sizeof(items));
	pthread_mutex_unlock(&info.xmms.item_mutex);

	/* Grab the runnable signal for next loop. */
        pthread_mutex_lock(&info.xmms.runnable_mutex);
        runnable=info.xmms.runnable;
        pthread_mutex_unlock(&info.xmms.runnable_mutex);

	sleep(1);
    }

    pthread_exit(NULL);
}

#elif defined(INFOPIPE)
/* --------------------------------------------------
 * Worker thread function for InfoPipe data sampling.
 * -------------------------------------------------- */ 
void *xmms_thread_func(void *pvoid)
{
    int i,rc,fd,runnable;
    fd_set readset;
    struct timeval tm;
    static char buf[2048],line[128];
    static xmms_t items;
    char *pbuf,c;

    pvoid=(void*)pvoid; /* useless cast to avoid unused var warning */

    /* Grab the runnable signal.  Should be non-zero here or we do nothing. */
    pthread_mutex_lock(&info.xmms.runnable_mutex);
    runnable=info.xmms.runnable;
    pthread_mutex_unlock(&info.xmms.runnable_mutex );

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
                        if ( sscanf(pbuf,"%*[^:]: %[^\n]",items[i]) == EOF )
			    break;
		        while((c = *pbuf++) && (c != '\n'));

			switch(i) {
			case INFOPIPE_PROTOCOL:
                        case INFOPIPE_VERSION:
                        case INFOPIPE_STATUS:
                        case INFOPIPE_PLAYLIST_TUNES:
                        case INFOPIPE_PLAYLIST_CURRTUNE:
                        case INFOPIPE_USEC_POSITION:
                        case INFOPIPE_POSITION:
                        case INFOPIPE_USEC_TIME:
                        case INFOPIPE_TIME:
                        case INFOPIPE_BITRATE:
                        case INFOPIPE_FREQUENCY:
                        case INFOPIPE_CHANNELS:
                        case INFOPIPE_TITLE:
                        case INFOPIPE_FILE:
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
	pthread_mutex_lock(&info.xmms.item_mutex);
        memcpy(&g_items,items,sizeof(items));
	pthread_mutex_unlock(&info.xmms.item_mutex);

	/* Grab the runnable signal for next loop. */
        pthread_mutex_lock(&info.xmms.runnable_mutex);
        runnable=info.xmms.runnable;
        pthread_mutex_unlock(&info.xmms.runnable_mutex);

	sleep(1);
    }

    pthread_exit(NULL);
}

#endif
