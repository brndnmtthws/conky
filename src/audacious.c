/* -------------------------------------------------------------------------
 * audacious.c:  conky support for Audacious media player
 *
 * http://audacious-media-player.org
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
/*#include <glib.h>*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <audacious/beepctrl.h>
#include "audacious.h"
#include "conky.h"

/* access to this item array is synchronized with mutexes */
static audacious_t g_items;

/* ----------------------------------------
 * Conky update function for Audacious data.
 * ---------------------------------------- */
void update_audacious(void)
{
    /* 
      The worker thread is updating ihe g_items array asynchronously to the main 
      conky thread.  We merely copy the g_items array into the main thread's info
      structure when the main thread's update cycle fires.   Note that using the
      mutexes here makes it easier since we won't have to do any sync in conky.c.
    */
    pthread_mutex_lock(&info.audacious.item_mutex);
    memcpy(&info.audacious.items,g_items,sizeof(g_items));
    pthread_mutex_unlock(&info.audacious.item_mutex);
}


/* --------------------------------------------------
 * Worker thread function for Audacious data sampling.
 * -------------------------------------------------- */ 
void *audacious_thread_func(void *pvoid)
{
    int runnable;
    static audacious_t items;
    int session,playpos,frames,length;
    int rate,freq,chans;
    char *psong;

    pvoid=(void*)pvoid; /* useless cast to avoid unused var warning */
    session=0;

    /* Grab the runnable signal.  Should be non-zero here or we do nothing. */
    pthread_mutex_lock(&info.audacious.runnable_mutex);
    runnable=info.audacious.runnable;
    pthread_mutex_unlock(&info.audacious.runnable_mutex );

    /* Loop until the main thread sets the runnable signal to 0. */
    while(runnable) {

	for (;;) {  /* convenience loop so we can break below */
	
            if (!xmms_remote_is_running(session)) {
                memset(&items,0,sizeof(items));
		strcpy(items[AUDACIOUS_STATUS],"Not running");
		break;
            }

	    /* Player status */
	    if (xmms_remote_is_paused(session))
	        strcpy(items[AUDACIOUS_STATUS],"Paused");
	    else if (xmms_remote_is_playing(session))
		 strcpy(items[AUDACIOUS_STATUS],"Playing");
	    else
	         strcpy(items[AUDACIOUS_STATUS],"Stopped");

	    /* Current song title */
	    playpos = (int) xmms_remote_get_playlist_pos(session);
	    psong = (char *) xmms_remote_get_playlist_title(session, playpos);
	    if (psong)
                strncpy(items[AUDACIOUS_SONG],psong,sizeof(items[AUDACIOUS_SONG]));

	    /* Current song length as MM:SS */ 
            frames = xmms_remote_get_playlist_time(session,playpos);
	    length = frames / 1000;
            snprintf(items[AUDACIOUS_SONG_LENGTH],sizeof(items[AUDACIOUS_SONG_LENGTH]),
	             "%d:%.2d", length / 60, length % 60);
	 
	    /* Current song length in seconds */
	    snprintf(items[AUDACIOUS_SONG_LENGTH_SECONDS],sizeof(items[AUDACIOUS_SONG_LENGTH_SECONDS]),
	             "%d", length);

	    /* Current song length in frames */
            snprintf(items[AUDACIOUS_SONG_LENGTH_FRAMES],sizeof(items[AUDACIOUS_SONG_LENGTH_FRAMES]),
	             "%d", frames);

	    /* Current song output length as MM:SS */ 
            frames = xmms_remote_get_output_time(session);
	    length = frames / 1000;
            snprintf(items[AUDACIOUS_SONG_OUTPUT_LENGTH],sizeof(items[AUDACIOUS_SONG_OUTPUT_LENGTH]),
	             "%d:%.2d", length / 60, length % 60);
	 
	    /* Current song output length in seconds */
	    snprintf(items[AUDACIOUS_SONG_OUTPUT_LENGTH_SECONDS],sizeof(items[AUDACIOUS_SONG_OUTPUT_LENGTH_SECONDS]),
	             "%d", length);

	    /* Current song output length in frames */
            snprintf(items[AUDACIOUS_SONG_OUTPUT_LENGTH_FRAMES],sizeof(items[AUDACIOUS_SONG_OUTPUT_LENGTH_FRAMES]),
	             "%d", frames);

            /* Current song bitrate */
            xmms_remote_get_info(session, &rate, &freq, &chans);
            snprintf(items[AUDACIOUS_SONG_BITRATE],sizeof(items[AUDACIOUS_SONG_BITRATE]), "%d", rate);

            /* Current song frequency */
            snprintf(items[AUDACIOUS_SONG_FREQUENCY],sizeof(items[AUDACIOUS_SONG_FREQUENCY]), "%d", freq);

            /* Current song channels */
            snprintf(items[AUDACIOUS_SONG_CHANNELS],sizeof(items[AUDACIOUS_SONG_CHANNELS]), "%d", chans);
            

	    break;
	}

	/* Deliver the refreshed items array to g_items. */
	pthread_mutex_lock(&info.audacious.item_mutex);
        memcpy(&g_items,items,sizeof(items));
	pthread_mutex_unlock(&info.audacious.item_mutex);

	/* Grab the runnable signal for next loop. */
        pthread_mutex_lock(&info.audacious.runnable_mutex);
        runnable=info.audacious.runnable;
        pthread_mutex_unlock(&info.audacious.runnable_mutex);

	sleep(1);
    }

    pthread_exit(NULL);
}
