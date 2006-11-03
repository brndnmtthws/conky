/* -------------------------------------------------------------------------
 * audacious.c:  conky support for audacious music player
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

#include <glib.h>
#include <audacious/beepctrl.h>

#include "config.h"
#include "conky.h"
#include "audacious.h"

/* access to this item array is synchronized */
static audacious_t audacious_items;

/* -----------------------------------------
 * Conky update function for audacious data.
 * ----------------------------------------- */
void update_audacious(void)
{
    /* 
      The worker thread is updating audacious_items array asynchronously to the main 
      conky thread.  We merely copy the audacious_items array into the main thread's 
      info structure when the main thread's update cycle fires. 
    */
    pthread_mutex_lock(&info.audacious.item_mutex);
    memcpy(&info.audacious.items,audacious_items,sizeof(audacious_items));
    pthread_mutex_unlock(&info.audacious.item_mutex);
}


/* ------------------------------------------------------------
 * Create a worker thread for audacious media player status.
 *
 * Returns 0 on success, -1 on error. 
 * ------------------------------------------------------------*/
int create_audacious_thread(void)
{
    /* Is a worker is thread already running? */
    if (info.audacious.thread)
	return(-1);

    /* Joinable thread for audacious activity */
    pthread_attr_init(&info.audacious.thread_attr);
    pthread_attr_setdetachstate(&info.audacious.thread_attr, PTHREAD_CREATE_JOINABLE);
    /* Init mutexes */
    pthread_mutex_init(&info.audacious.item_mutex, NULL);
    pthread_mutex_init(&info.audacious.runnable_mutex, NULL);
    /* Init runnable condition for worker thread */
    pthread_mutex_lock(&info.audacious.runnable_mutex);
    info.audacious.runnable=1;
    pthread_mutex_unlock(&info.audacious.runnable_mutex);
    if (pthread_create(&info.audacious.thread, &info.audacious.thread_attr, audacious_thread_func, NULL))
        return(-1);

    return 0;
}

/* ------------------------------------------------
 * Destroy audacious player status thread. 
 *
 * Returns 0 on success, -1 on error.
 * ------------------------------------------------ */
int destroy_audacious_thread(void)
{
    /* Is a worker is thread running? If not, no error. */
    if (!info.audacious.thread)
        return(0);

    /* Signal audacious thread to terminate */
    pthread_mutex_lock(&info.audacious.runnable_mutex);
    info.audacious.runnable=0;
    pthread_mutex_unlock(&info.audacious.runnable_mutex);
    /* Destroy thread attribute and wait for thread */
    pthread_attr_destroy(&info.audacious.thread_attr);
    if (pthread_join(info.audacious.thread, NULL))
        return(-1);
    /* Destroy mutexes */
    pthread_mutex_destroy(&info.audacious.item_mutex);
    pthread_mutex_destroy(&info.audacious.runnable_mutex);

    info.audacious.thread=(pthread_t)0;
    return 0;
}

/* ---------------------------------------------------
 * Worker thread function for audacious data sampling.
 * --------------------------------------------------- */ 
void *audacious_thread_func(void *pvoid)
{
    int runnable;
    static audacious_t items;
    gint session,playpos,frames,length;
    gint rate,freq,chans;
    gchar *psong,*pfilename;

    pvoid=(void *)pvoid;  /* avoid warning */
    session=0;
    psong=NULL;
    pfilename=NULL;

    /* Grab the runnable signal.  Should be non-zero here or we do nothing. */
    pthread_mutex_lock(&info.audacious.runnable_mutex);
    runnable=info.audacious.runnable;
    pthread_mutex_unlock(&info.audacious.runnable_mutex );

    /* Loop until the main thread sets the runnable signal to 0. */
    while(runnable) {

        for (;;) {  /* convenience loop so we can break below */

            if (!xmms_remote_is_running (session)) {
                memset(&items,0,sizeof(items));
                strcpy(items[AUDACIOUS_STATUS],"Not running");
                break;
            }

            /* Player status */
            if (xmms_remote_is_paused (session))
                strcpy(items[AUDACIOUS_STATUS],"Paused");
            else if (xmms_remote_is_playing (session))
                 strcpy(items[AUDACIOUS_STATUS],"Playing");
            else
                 strcpy(items[AUDACIOUS_STATUS],"Stopped");

            /* Current song title */
            playpos = xmms_remote_get_playlist_pos (session);
            psong = xmms_remote_get_playlist_title (session, playpos);
            if (psong) {
                strncpy(items[AUDACIOUS_TITLE],psong,sizeof(items[AUDACIOUS_TITLE])-1);
                g_free (psong);
                psong=NULL;
            }

            /* Current song length as MM:SS */
            frames = xmms_remote_get_playlist_time (session,playpos);
            length = frames / 1000;
            snprintf(items[AUDACIOUS_LENGTH],sizeof(items[AUDACIOUS_LENGTH])-1,
                     "%d:%.2d", length / 60, length % 60);

            /* Current song length in seconds */
            snprintf(items[AUDACIOUS_LENGTH_SECONDS],sizeof(items[AUDACIOUS_LENGTH_SECONDS])-1,
                     "%d", length);

            /* Current song position as MM:SS */
            frames = xmms_remote_get_output_time (session);
            length = frames / 1000;
            snprintf(items[AUDACIOUS_POSITION],sizeof(items[AUDACIOUS_POSITION])-1,
                     "%d:%.2d", length / 60, length % 60);

            /* Current song position in seconds */
            snprintf(items[AUDACIOUS_POSITION_SECONDS],sizeof(items[AUDACIOUS_POSITION_SECONDS])-1,
                     "%d", length);

            /* Current song bitrate */
            xmms_remote_get_info (session, &rate, &freq, &chans);
            snprintf(items[AUDACIOUS_BITRATE],sizeof(items[AUDACIOUS_BITRATE])-1, "%d", rate);

            /* Current song frequency */
            snprintf(items[AUDACIOUS_FREQUENCY],sizeof(items[AUDACIOUS_FREQUENCY])-1, "%d", freq);

            /* Current song channels */
            snprintf(items[AUDACIOUS_CHANNELS],sizeof(items[AUDACIOUS_CHANNELS])-1, "%d", chans);

            /* Current song filename */
            pfilename = xmms_remote_get_playlist_file (session,playpos);
            if (pfilename) {
                strncpy(items[AUDACIOUS_FILENAME],pfilename,sizeof(items[AUDACIOUS_FILENAME])-1);
                g_free (pfilename);
                pfilename=NULL;
            }

            /* Length of the Playlist (number of songs) */
            length = xmms_remote_get_playlist_length (session);
            snprintf(items[AUDACIOUS_PLAYLIST_LENGTH],sizeof(items[AUDACIOUS_PLAYLIST_LENGTH])-1, "%d", length);

            /* Playlist position (index of song) */
            snprintf(items[AUDACIOUS_PLAYLIST_POSITION],sizeof(items[AUDACIOUS_PLAYLIST_POSITION])-1, "%d", playpos+1);

            break;
        }

        /* Deliver the refreshed items array to g_items. */
        pthread_mutex_lock(&info.audacious.item_mutex);
        memcpy(&audacious_items,items,sizeof(items));
        pthread_mutex_unlock(&info.audacious.item_mutex);

        /* Grab the runnable signal for next loop. */
        pthread_mutex_lock(&info.audacious.runnable_mutex);
        runnable=info.audacious.runnable;
        pthread_mutex_unlock(&info.audacious.runnable_mutex);

        sleep(1);
    }

    pthread_exit(NULL);
}
