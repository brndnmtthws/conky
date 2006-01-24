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

#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS)
#include <glib.h>
#include <dlfcn.h>
#endif

#if defined(INFOPIPE)
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

static char *xmms_project_name[] = {
        "none",
        "xmms",
        "bmp",
        "audacious",
        "infopipe"
};


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


/* ------------------------------------------------------------
 * Create a worker thread for xmms-related media player status.
 *
 * Returns 0 on success, -1 on error. 
 * ------------------------------------------------------------*/
int create_xmms_thread(void)
{
    /* Was an an available project requested? */
    if (!TEST_XMMS_PROJECT_AVAILABLE(info.xmms.project_mask, info.xmms.current_project)) {
	ERR("xmms_player '%s' not configured", xmms_project_name[info.xmms.current_project]);
        return(-1);
    }
    
    /* The project should not be PROJECT_NONE */
    if (info.xmms.current_project==PROJECT_NONE)
        return(-1);

    /* Is a worker is thread already running? */
    if (info.xmms.thread)
	return(-1);

    /* Joinable thread for xmms activity */
    pthread_attr_init(&info.xmms.thread_attr);
    pthread_attr_setdetachstate(&info.xmms.thread_attr, PTHREAD_CREATE_JOINABLE);
    /* Init mutexes */
    pthread_mutex_init(&info.xmms.item_mutex, NULL);
    pthread_mutex_init(&info.xmms.runnable_mutex, NULL);
    /* Init runnable condition for worker thread */
    pthread_mutex_lock(&info.xmms.runnable_mutex);
    info.xmms.runnable=1;
    pthread_mutex_unlock(&info.xmms.runnable_mutex);
#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS)	    
    if (info.xmms.current_project==PROJECT_XMMS || 
        info.xmms.current_project==PROJECT_BMP || 
	info.xmms.current_project==PROJECT_AUDACIOUS) {
        if (pthread_create(&info.xmms.thread, &info.xmms.thread_attr, xmms_thread_func_dynamic, NULL))
            return(-1);
    }
#endif
#if defined(INFOPIPE)
    if (info.xmms.current_project==PROJECT_INFOPIPE) {
	if (pthread_create(&info.xmms.thread, &info.xmms.thread_attr, xmms_thread_func_infopipe, NULL))
            return(-1);
    }
#endif

    return 0;
}

/* ------------------------------------------------
 * Destroy xmms-related media player status thread.
 *
 * Returns 0 on success, -1 on error.
 * ------------------------------------------------ */
int destroy_xmms_thread(void)
{
    /* Is a worker is thread running? If not, no error. */
    if (!info.xmms.thread)
        return(0);

    /* Signal xmms worker thread to terminate */
    pthread_mutex_lock(&info.xmms.runnable_mutex);
    info.xmms.runnable=0;
    pthread_mutex_unlock(&info.xmms.runnable_mutex);
    /* Destroy thread attribute and wait for thread */
    pthread_attr_destroy(&info.xmms.thread_attr);
    if (pthread_join(info.xmms.thread, NULL))
        return(-1);
    /* Destroy mutexes */
    pthread_mutex_destroy(&info.xmms.item_mutex);
    pthread_mutex_destroy(&info.xmms.runnable_mutex);

    info.xmms.thread=(pthread_t)0;
    return 0;
}

#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS)
void check_dlerror(void)
{
    static const char *error;

    if ((error = dlerror()) != NULL) {
        ERR("error grabbing function symbol");
        pthread_exit(NULL);
    }
}

/* ------------------------------------------------------------
 * Worker thread function for XMMS/BMP/Audacious data sampling.
 * ------------------------------------------------------------ */ 
void *xmms_thread_func_dynamic(void *pvoid)
{
    void *handle,*glib_v1_2_handle;
    int runnable;
    static xmms_t items;
    gint session,playpos,frames,length;
    gint rate,freq,chans;
    gchar *psong,*pfilename;

    /* Function pointers for the functions we load dynamically */
    void (*g_free_v1_2)(gpointer mem);
    gboolean (*xmms_remote_is_running)(gint session);
    gboolean (*xmms_remote_is_paused)(gint session);
    gboolean (*xmms_remote_is_playing)(gint session);
    gint (*xmms_remote_get_playlist_pos)(gint session);
    gchar *(*xmms_remote_get_playlist_title)(gint session, gint pos);
    gint (*xmms_remote_get_playlist_time)(gint session, gint pos);
    gint (*xmms_remote_get_output_time)(gint session);
    void (*xmms_remote_get_info)(gint session, gint *rate, gint *freq, gint *chans);
    gchar *(*xmms_remote_get_playlist_file)(gint session, gint pos);
    gint (*xmms_remote_get_playlist_length)(gint session);

    pvoid=(void *)pvoid;  /* avoid warning */
    session=0;
    psong=NULL;
    pfilename=NULL;
    handle=NULL;
    glib_v1_2_handle=NULL;
    g_free_v1_2=NULL;

    /* Conky will likely be linked to libglib-2.0.so and not libglib-1.2.so.0.  If conky is receiving
     * gchar * data from xmms, these strings need to be freed using g_free() from libglib-1.2.so.0.
     * This macro selects the g_free() from the correct library. */
    #define G_FREE(mem) (info.xmms.current_project==PROJECT_XMMS ? (*g_free_v1_2)(mem) : g_free(mem))

    switch(info.xmms.current_project) {

    case (PROJECT_XMMS) :
	    /* make an effort to find the glib 1.2 shared lib */
	    glib_v1_2_handle = dlopen("libglib-1.2.so.0", RTLD_LAZY) ||
		    	       dlopen("libglib12.so", RTLD_LAZY) ||
			       dlopen("libglib.so", RTLD_LAZY);
	    if (!glib_v1_2_handle) {
		ERR("unable to find glib 1.2 shared object lib!");
		pthread_exit(NULL);
	    }
	    g_free_v1_2=dlsym(glib_v1_2_handle, "g_free");
	    check_dlerror();

	    handle = dlopen("libxmms.so", RTLD_LAZY);
	    if (!handle) {
	        ERR("unable to open libxmms.so");
		pthread_exit(NULL);
	    }
	    break;
		    
    case (PROJECT_BMP) :
	    handle = dlopen("libbeep.so", RTLD_LAZY);
	    if (!handle) {
		 ERR("unable to open libbeep.so");
		 pthread_exit(NULL);
            }
	    break;

    case (PROJECT_AUDACIOUS) :
	    handle = dlopen("libaudacious.so", RTLD_LAZY);
	    if (!handle) {
		 ERR("unable to open libaudacious.so");
	         pthread_exit(NULL);
            }
	    break;

    case (PROJECT_NONE) :
    default :
         pthread_exit(NULL);
    }

    /* Grab the function pointers from the library */
    xmms_remote_is_running = dlsym(handle, "xmms_remote_is_running");
    check_dlerror();

    xmms_remote_is_paused = dlsym(handle, "xmms_remote_is_paused");
    check_dlerror();

    xmms_remote_is_playing = dlsym(handle, "xmms_remote_is_playing");
    check_dlerror();

    xmms_remote_get_playlist_pos = dlsym(handle, "xmms_remote_get_playlist_pos");
    check_dlerror();

    xmms_remote_get_playlist_title = dlsym(handle, "xmms_remote_get_playlist_title");
    check_dlerror();

    xmms_remote_get_playlist_time = dlsym(handle, "xmms_remote_get_playlist_time");
    check_dlerror();

    xmms_remote_get_output_time = dlsym(handle, "xmms_remote_get_output_time");
    check_dlerror();

    xmms_remote_get_info = dlsym(handle, "xmms_remote_get_info");

    xmms_remote_get_playlist_file = dlsym(handle, "xmms_remote_get_playlist_file");
    check_dlerror();

    xmms_remote_get_playlist_length = dlsym(handle, "xmms_remote_get_playlist_length");
    check_dlerror();

    /* Grab the runnable signal.  Should be non-zero here or we do nothing. */
    pthread_mutex_lock(&info.xmms.runnable_mutex);
    runnable=info.xmms.runnable;
    pthread_mutex_unlock(&info.xmms.runnable_mutex );

    /* Loop until the main thread sets the runnable signal to 0. */
    while(runnable) {

        for (;;) {  /* convenience loop so we can break below */

            if (!(*xmms_remote_is_running)(session)) {
                memset(&items,0,sizeof(items));
                strcpy(items[XMMS_STATUS],"Not running");
                break;
            }

            /* Player status */
            if ((*xmms_remote_is_paused)(session))
                strcpy(items[XMMS_STATUS],"Paused");
            else if ((*xmms_remote_is_playing)(session))
                 strcpy(items[XMMS_STATUS],"Playing");
            else
                 strcpy(items[XMMS_STATUS],"Stopped");

            /* Current song title */
            playpos = (*xmms_remote_get_playlist_pos)(session);
            psong = (*xmms_remote_get_playlist_title)(session, playpos);
            if (psong) {
                strncpy(items[XMMS_TITLE],psong,sizeof(items[XMMS_TITLE])-1);
                G_FREE(psong);
                psong=NULL;
            }

            /* Current song length as MM:SS */
            frames = (*xmms_remote_get_playlist_time)(session,playpos);
            length = frames / 1000;
            snprintf(items[XMMS_LENGTH],sizeof(items[XMMS_LENGTH])-1,
                     "%d:%.2d", length / 60, length % 60);

            /* Current song length in seconds */
            snprintf(items[XMMS_LENGTH_SECONDS],sizeof(items[XMMS_LENGTH_SECONDS])-1,
                     "%d", length);

            /* Current song position as MM:SS */
            frames = (*xmms_remote_get_output_time)(session);
            length = frames / 1000;
            snprintf(items[XMMS_POSITION],sizeof(items[XMMS_POSITION])-1,
                     "%d:%.2d", length / 60, length % 60);

            /* Current song position in seconds */
            snprintf(items[XMMS_POSITION_SECONDS],sizeof(items[XMMS_POSITION_SECONDS])-1,
                     "%d", length);

            /* Current song bitrate */
            (*xmms_remote_get_info)(session, &rate, &freq, &chans);
            snprintf(items[XMMS_BITRATE],sizeof(items[XMMS_BITRATE])-1, "%d", rate);

            /* Current song frequency */
            snprintf(items[XMMS_FREQUENCY],sizeof(items[XMMS_FREQUENCY])-1, "%d", freq);

            /* Current song channels */
            snprintf(items[XMMS_CHANNELS],sizeof(items[XMMS_CHANNELS])-1, "%d", chans);

            /* Current song filename */
            pfilename = (*xmms_remote_get_playlist_file)(session,playpos);
            if (pfilename) {
                strncpy(items[XMMS_FILENAME],pfilename,sizeof(items[XMMS_FILENAME])-1);
                G_FREE(pfilename);
                pfilename=NULL;
            }

            /* Length of the Playlist (number of songs) */
            length = (*xmms_remote_get_playlist_length)(session);
            snprintf(items[XMMS_PLAYLIST_LENGTH],sizeof(items[XMMS_PLAYLIST_LENGTH])-1, "%d", length);

            /* Playlist position (index of song) */
            snprintf(items[XMMS_PLAYLIST_POSITION],sizeof(items[XMMS_PLAYLIST_POSITION])-1, "%d", playpos+1);

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

    if (handle)
        dlclose(handle);
    if (glib_v1_2_handle)
	dlclose(glib_v1_2_handle);

    pthread_exit(NULL);
}
#endif

#if defined(INFOPIPE)
/* --------------------------------------------------
 * Worker thread function for InfoPipe data sampling.
 * -------------------------------------------------- */ 
void *xmms_thread_func_infopipe(void *pvoid)
{
    int i,rc,fd,runnable;
    fd_set readset;
    struct timeval tm;
    static char buf[2048],line[128];
    static xmms_t items;
    char *pbuf,c;

    pvoid=(void*)pvoid; /* avoid warning */

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
		strcpy(items[XMMS_STATUS],"Not running");
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
			case INFOPIPE_PROTOCOL:
				break;
                        case INFOPIPE_VERSION:
				break;
                        case INFOPIPE_STATUS:
				strncpy(items[XMMS_STATUS],line,sizeof(items[XMMS_STATUS])-1);
				break;
                        case INFOPIPE_PLAYLIST_TUNES:
				strncpy(items[XMMS_PLAYLIST_LENGTH],line,sizeof(items[XMMS_PLAYLIST_LENGTH])-1);
				break;
                        case INFOPIPE_PLAYLIST_CURRTUNE:
				strncpy(items[XMMS_PLAYLIST_POSITION],line,sizeof(items[XMMS_PLAYLIST_POSITION])-1);
				break;
                        case INFOPIPE_USEC_POSITION:
				snprintf(items[XMMS_POSITION_SECONDS],sizeof(items[XMMS_POSITION_SECONDS])-1,
					 "%d", atoi(line) / 1000);
				break;
                        case INFOPIPE_POSITION:
				strncpy(items[XMMS_POSITION],line,sizeof(items[XMMS_POSITION])-1);
				break;
                        case INFOPIPE_USEC_TIME:
				snprintf(items[XMMS_LENGTH_SECONDS],sizeof(items[XMMS_LENGTH_SECONDS])-1,
					 "%d", atoi(line) / 1000);
				break;
                        case INFOPIPE_TIME:
			 	strncpy(items[XMMS_LENGTH],line,sizeof(items[XMMS_LENGTH])-1);
				break;
                        case INFOPIPE_BITRATE:
				strncpy(items[XMMS_BITRATE],line,sizeof(items[XMMS_BITRATE])-1);
				break;
                        case INFOPIPE_FREQUENCY:
				strncpy(items[XMMS_FREQUENCY],line,sizeof(items[XMMS_FREQUENCY])-1);
				break;
                        case INFOPIPE_CHANNELS:
				strncpy(items[XMMS_CHANNELS],line,sizeof(items[XMMS_CHANNELS])-1);
				break;
                        case INFOPIPE_TITLE:
				strncpy(items[XMMS_TITLE],line,sizeof(items[XMMS_TITLE])-1);
				break;
                        case INFOPIPE_FILE:
				strncpy(items[XMMS_FILENAME],line,sizeof(items[XMMS_FILENAME])-1);
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
