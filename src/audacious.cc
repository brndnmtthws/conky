/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
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

#include <config.h>
#include "conky.h"
#include "logging.h"
#include "audacious.h"
#include <mutex>

#include <glib.h>
#ifdef NEW_AUDACIOUS_FOUND
#include <glib-object.h>
#include <audacious/audctrl.h>
#include <audacious/dbus.h>
#else /* NEW_AUDACIOUS_FOUND */
#include <audacious/beepctrl.h>
#define audacious_remote_is_running(x)				\
	xmms_remote_is_running(x)
#define audacious_remote_is_paused(x)				\
	xmms_remote_is_paused(x)
#define audacious_remote_is_playing(x)				\
	xmms_remote_is_playing(x)
#define audacious_remote_get_playlist_pos(x)		\
	xmms_remote_get_playlist_pos(x)
#define audacious_remote_get_playlist_title(x, y)	\
	xmms_remote_get_playlist_title(x, y)
#define audacious_remote_get_playlist_time(x, y)	\
	xmms_remote_get_playlist_time(x, y)
#define audacious_remote_get_output_time(x)			\
	xmms_remote_get_output_time(x)
#define audacious_remote_get_info(w, x, y, z)		\
	xmms_remote_get_info(w, x, y, z)
#define audacious_remote_get_playlist_file(x, y)	\
	xmms_remote_get_playlist_file(x, y)
#define audacious_remote_get_playlist_length(x)		\
	xmms_remote_get_playlist_length(x)
#endif /* NEW_AUDACIOUS_FOUND */

/* access to this item array is synchronized */
static audacious_t audacious_items;

/* -----------------------------------------
 * Conky update function for audacious data.
 * ----------------------------------------- */
int update_audacious(void)
{
	/* The worker thread is updating audacious_items array asynchronously
	 * to the main conky thread.
	 * We merely copy the audacious_items array into the main thread's info
	 * structure when the main thread's update cycle fires. */
	if (!info.audacious.p_timed_thread) {
		if (create_audacious_thread() != 0) {
			CRIT_ERR(NULL, NULL, "unable to create audacious thread!");
		}
	}

	std::lock_guard<std::mutex> lock(info.audacious.p_timed_thread->mutex());
	memcpy(&info.audacious.items, audacious_items, sizeof(audacious_items));
	return 0;
}

/* ---------------------------------------------------------
 * Create a worker thread for audacious media player status.
 *
 * Returns 0 on success, -1 on error.
 * --------------------------------------------------------- */
int create_audacious_thread(void)
{
	if (!info.audacious.p_timed_thread) {
		info.audacious.p_timed_thread =
			timed_thread::create(std::bind(audacious_thread_func, std::placeholders::_1),
					std::chrono::microseconds(long(music_player_interval.get(*state) * 1000000)));
	}

	if (!info.audacious.p_timed_thread) {
		return -1;
	}

	return 0;
}

/* ---------------------------------------
 * Destroy audacious player status thread.
 *
 * Returns 0 on success, -1 on error.
 * --------------------------------------- */
int destroy_audacious_thread(void)
{
	/* Is a worker is thread running? If not, no error. */
	if (info.audacious.p_timed_thread) {
		info.audacious.p_timed_thread.reset();
	}

	return 0;
}

/* ---------------------------------------------------
 * Worker thread function for audacious data sampling.
 * --------------------------------------------------- */
void audacious_thread_func(thread_handle &handle)
{
	static audacious_t items;
	gint playpos, frames, length;
	gint rate, freq, chans, vol;
	gchar *psong, *pfilename;

#ifdef NEW_AUDACIOUS_FOUND
	DBusGProxy *session = NULL;
	DBusGConnection *connection = NULL;
#else
	gint session;
#endif

	session = 0;
	psong = NULL;
	pfilename = NULL;

#ifdef NEW_AUDACIOUS_FOUND
	g_type_init();
	connection = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	if (!connection) {
		CRIT_ERR(NULL, NULL, "unable to establish dbus connection");
	}
	session = dbus_g_proxy_new_for_name(connection, AUDACIOUS_DBUS_SERVICE,
			AUDACIOUS_DBUS_PATH, AUDACIOUS_DBUS_INTERFACE);
	if (!session) {
		CRIT_ERR(NULL, NULL, "unable to create dbus proxy");
	}
#endif /* NEW_AUDACIOUS_FOUND */

	/* Loop until the main thread resets the runnable signal. */
	while (1) {

		do {
			if (!audacious_remote_is_running(session)) {
				memset(&items, 0, sizeof(items));
				strcpy(items[AUDACIOUS_STATUS], "Not running");
				break;
			}

			/* Player status */
			if (audacious_remote_is_paused(session)) {
				strcpy(items[AUDACIOUS_STATUS], "Paused");
			} else if (audacious_remote_is_playing(session)) {
				strcpy(items[AUDACIOUS_STATUS], "Playing");
			} else {
				strcpy(items[AUDACIOUS_STATUS], "Stopped");
			}

			/* Current song title */
			playpos = audacious_remote_get_playlist_pos(session);
			psong = audacious_remote_get_playlist_title(session, playpos);
			if (psong) {
				strncpy(items[AUDACIOUS_TITLE], psong,
						sizeof(items[AUDACIOUS_TITLE]) - 1);
				g_free(psong);
				psong = NULL;
			}

			/* Current song length as MM:SS */
			frames = audacious_remote_get_playlist_time(session, playpos);
			length = frames / 1000;
			snprintf(items[AUDACIOUS_LENGTH], sizeof(items[AUDACIOUS_LENGTH]) - 1,
					"%d:%.2d", length / 60, length % 60);

			/* Current song length in seconds */
			snprintf(items[AUDACIOUS_LENGTH_SECONDS],
					sizeof(items[AUDACIOUS_LENGTH_SECONDS]) - 1, "%d", length);

			/* Current song position as MM:SS */
			frames = audacious_remote_get_output_time(session);
			length = frames / 1000;
			snprintf(items[AUDACIOUS_POSITION],
					sizeof(items[AUDACIOUS_POSITION]) - 1, "%d:%.2d", length / 60,
					length % 60);

			/* Current song position in seconds */
			snprintf(items[AUDACIOUS_POSITION_SECONDS],
					sizeof(items[AUDACIOUS_POSITION_SECONDS]) - 1, "%d", length);

			/* Current song bitrate */
			audacious_remote_get_info(session, &rate, &freq, &chans);
			snprintf(items[AUDACIOUS_BITRATE], sizeof(items[AUDACIOUS_BITRATE]) - 1,
					"%d", rate);

			/* Current song frequency */
			snprintf(items[AUDACIOUS_FREQUENCY],
					sizeof(items[AUDACIOUS_FREQUENCY]) - 1, "%d", freq);

			/* Current song channels */
			snprintf(items[AUDACIOUS_CHANNELS],
					sizeof(items[AUDACIOUS_CHANNELS]) - 1, "%d", chans);

			/* Current song filename */
			pfilename = audacious_remote_get_playlist_file(session, playpos);
			if (pfilename) {
				strncpy(items[AUDACIOUS_FILENAME], pfilename,
						sizeof(items[AUDACIOUS_FILENAME]) - 1);
				g_free(pfilename);
				pfilename = NULL;
			}

			/* Length of the Playlist (number of songs) */
			length = audacious_remote_get_playlist_length(session);
			snprintf(items[AUDACIOUS_PLAYLIST_LENGTH],
					sizeof(items[AUDACIOUS_PLAYLIST_LENGTH]) - 1, "%d", length);

			/* Playlist position (index of song) */
			snprintf(items[AUDACIOUS_PLAYLIST_POSITION],
					sizeof(items[AUDACIOUS_PLAYLIST_POSITION]) - 1, "%d", playpos + 1);
			/* Main volume */
			vol = audacious_remote_get_main_volume(session);
			snprintf(items[AUDACIOUS_MAIN_VOLUME],
					sizeof(items[AUDACIOUS_MAIN_VOLUME]) - 1, "%d", vol);

		} while (0);
		{
			/* Deliver the refreshed items array to audacious_items. */
			std::lock_guard<std::mutex> lock(handle.mutex());
			memcpy(&audacious_items, items, sizeof(items));
		}

		if (handle.test(0)) {
#ifdef NEW_AUDACIOUS_FOUND
			/* release reference to dbus proxy */
			g_object_unref(session);
#endif
			return;
		}
	}
}

void print_audacious_title(struct text_object *, char *p, int p_max_size)
{
	snprintf(p, info.audacious.max_title_len > 0
			? info.audacious.max_title_len : p_max_size, "%s",
			info.audacious.items[AUDACIOUS_TITLE]);
}

double audacious_barval(struct text_object *)
{

	return atof(info.audacious.items[AUDACIOUS_POSITION_SECONDS]) /
		atof(info.audacious.items[AUDACIOUS_LENGTH_SECONDS]);
}

#define AUDACIOUS_PRINT_GENERATOR(name, idx) \
void print_audacious_##name(struct text_object *, char *p, int p_max_size) \
{ \
	snprintf(p, p_max_size, "%s", info.audacious.items[AUDACIOUS_##idx]); \
}

AUDACIOUS_PRINT_GENERATOR(status, STATUS)
AUDACIOUS_PRINT_GENERATOR(length, LENGTH)
AUDACIOUS_PRINT_GENERATOR(length_seconds, LENGTH_SECONDS)
AUDACIOUS_PRINT_GENERATOR(position, POSITION)
AUDACIOUS_PRINT_GENERATOR(position_seconds, POSITION_SECONDS)
AUDACIOUS_PRINT_GENERATOR(bitrate, BITRATE)
AUDACIOUS_PRINT_GENERATOR(frequency, FREQUENCY)
AUDACIOUS_PRINT_GENERATOR(channels, CHANNELS)
AUDACIOUS_PRINT_GENERATOR(filename, FILENAME)
AUDACIOUS_PRINT_GENERATOR(playlist_length, PLAYLIST_LENGTH)
AUDACIOUS_PRINT_GENERATOR(playlist_position, PLAYLIST_POSITION)
AUDACIOUS_PRINT_GENERATOR(main_volume, MAIN_VOLUME)

#undef AUDACIOUS_PRINT_GENERATOR
