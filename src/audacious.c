/* $Id$ */

/*
 * audacious.c:  conky support for audacious music player
 *
 * Copyright (C) 2005-2007 Philip Kovacs pkovacs@users.sourceforge.net
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 */

#include <stdio.h>
#include <string.h>

#include <glib.h>
#ifndef AUDACIOUS_LEGACY
#include <glib-object.h>
#include <audacious/audctrl.h>
#include <audacious/dbus.h>
#else
#include <audacious/beepctrl.h>
#define audacious_remote_is_running(x)          xmms_remote_is_running(x) 
#define audacious_remote_is_paused(x)           xmms_remote_is_paused(x)
#define audacious_remote_is_playing(x)          xmms_remote_is_playing(x)
#define audacious_remote_get_playlist_pos(x)    xmms_remote_get_playlist_pos(x)
#define audacious_remote_get_playlist_title(x)  xmms_remote_get_playlist_title(x)
#define audacious_remote_get_playlist_time(x)   xmms_remote_get_playlist_time(x)
#define audacious_remote_get_output_time(x)     xmms_remote_get_output_time(x)
#define audacious_remote_get_info(x)            xmms_remote_get_info(x)
#define audacious_remote_get_playlist_file(x)   xmms_remote_get_playlist_file(x)
#define audacious_remote_get_playlist_length(x) xmms_remote_get_playlist_length(x)
#endif

#include "config.h"
#include "conky.h"
#include "audacious.h"
#include "timed_thread.h"

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
  if (!info.audacious.p_timed_thread)
    return;

  timed_thread_lock (info.audacious.p_timed_thread);
  memcpy(&info.audacious.items,audacious_items,sizeof(audacious_items));
  timed_thread_unlock (info.audacious.p_timed_thread);
}


/* ------------------------------------------------------------
 * Create a worker thread for audacious media player status.
 *
 * Returns 0 on success, -1 on error. 
 * ------------------------------------------------------------*/
int create_audacious_thread(void)
{
  if (!info.audacious.p_timed_thread)
    info.audacious.p_timed_thread = 
      timed_thread_create (audacious_thread_func, NULL, info.music_player_interval * 1000000);

  if (!info.audacious.p_timed_thread || timed_thread_run (info.audacious.p_timed_thread))
    return (-1);

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
  if (!info.audacious.p_timed_thread)
    return(0);

  timed_thread_destroy (info.audacious.p_timed_thread, &info.audacious.p_timed_thread);

  return 0;
}

/* ---------------------------------------------------
 * Worker thread function for audacious data sampling.
 * --------------------------------------------------- */ 
void *audacious_thread_func(void *pvoid)
{
  static audacious_t items;
  gint playpos,frames,length;
  gint rate,freq,chans;
  gchar *psong,*pfilename;

#ifndef AUDACIOUS_LEGACY
  DBusGProxy *session = NULL;
  DBusGConnection *connection = NULL;
#else
  gint session;
#endif


  pvoid=(void *)pvoid;  /* avoid warning */
  session=0;
  psong=NULL;
  pfilename=NULL;

#ifndef AUDACIOUS_LEGACY
  g_type_init ();
  connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  if (!connection) {
    CRIT_ERR ("unable to establish dbus connection");
  }
  session = dbus_g_proxy_new_for_name (connection, 
                                       AUDACIOUS_DBUS_SERVICE,
                                       AUDACIOUS_DBUS_PATH,
                                       AUDACIOUS_DBUS_INTERFACE);
  if (!session) {
    CRIT_ERR ("unable to establish dbus proxy");
  }
#endif /* AUDACIOUS_LEGACY */

  /* Loop until the main thread sets the runnable signal to 0i via timed_thread_destroy. */
  while (1) {

    if (!audacious_remote_is_running (session)) 
    {
      memset(&items,0,sizeof(items));
      strcpy(items[AUDACIOUS_STATUS],"Not running");
        goto next_iter;
    }

    /* Player status */
    if (audacious_remote_is_paused (session))
      strcpy(items[AUDACIOUS_STATUS],"Paused");
    else if (audacious_remote_is_playing (session))
      strcpy(items[AUDACIOUS_STATUS],"Playing");
    else
      strcpy(items[AUDACIOUS_STATUS],"Stopped");

    /* Current song title */
    playpos = audacious_remote_get_playlist_pos (session);
    psong = audacious_remote_get_playlist_title (session, playpos);
    if (psong) 
    {
      strncpy(items[AUDACIOUS_TITLE],psong,sizeof(items[AUDACIOUS_TITLE])-1);
      g_free (psong);
      psong=NULL;
    }

    /* Current song length as MM:SS */
    frames = audacious_remote_get_playlist_time (session,playpos);
    length = frames / 1000;
    snprintf(items[AUDACIOUS_LENGTH],sizeof(items[AUDACIOUS_LENGTH])-1, "%d:%.2d", length / 60, length % 60);

    /* Current song length in seconds */
    snprintf(items[AUDACIOUS_LENGTH_SECONDS],sizeof(items[AUDACIOUS_LENGTH_SECONDS])-1, "%d", length);

    /* Current song position as MM:SS */
    frames = audacious_remote_get_output_time (session);
    length = frames / 1000;
    snprintf(items[AUDACIOUS_POSITION],sizeof(items[AUDACIOUS_POSITION])-1,
             "%d:%.2d", length / 60, length % 60);

    /* Current song position in seconds */
    snprintf(items[AUDACIOUS_POSITION_SECONDS],sizeof(items[AUDACIOUS_POSITION_SECONDS])-1, "%d", length);

    /* Current song bitrate */
    audacious_remote_get_info (session, &rate, &freq, &chans);
    snprintf(items[AUDACIOUS_BITRATE],sizeof(items[AUDACIOUS_BITRATE])-1, "%d", rate);

    /* Current song frequency */
    snprintf(items[AUDACIOUS_FREQUENCY],sizeof(items[AUDACIOUS_FREQUENCY])-1, "%d", freq);

    /* Current song channels */
    snprintf(items[AUDACIOUS_CHANNELS],sizeof(items[AUDACIOUS_CHANNELS])-1, "%d", chans);

    /* Current song filename */
    pfilename = audacious_remote_get_playlist_file (session,playpos);
    if (pfilename) 
    {
      strncpy(items[AUDACIOUS_FILENAME],pfilename,sizeof(items[AUDACIOUS_FILENAME])-1);
      g_free (pfilename);
      pfilename=NULL;
    }

    /* Length of the Playlist (number of songs) */
    length = audacious_remote_get_playlist_length (session);
    snprintf(items[AUDACIOUS_PLAYLIST_LENGTH],sizeof(items[AUDACIOUS_PLAYLIST_LENGTH])-1, "%d", length);

    /* Playlist position (index of song) */
    snprintf(items[AUDACIOUS_PLAYLIST_POSITION],sizeof(items[AUDACIOUS_PLAYLIST_POSITION])-1, 
           "%d", playpos+1);

next_iter:

    /* Deliver the refreshed items array to audacious_items. */
    timed_thread_lock (info.audacious.p_timed_thread);
    memcpy(&audacious_items,items,sizeof(items));
    timed_thread_unlock (info.audacious.p_timed_thread);

    if (timed_thread_test (info.audacious.p_timed_thread))
      timed_thread_exit (info.audacious.p_timed_thread);
  }
}
