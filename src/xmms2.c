/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "conky.h"
#include <xmmsclient/xmmsclient.h>

#define CONN_INIT	0
#define CONN_OK		1
#define CONN_NO		2

/* callbacks */

static void xmms_alloc(struct information *ptr)
{
	if (ptr->xmms2.status == NULL) {
		ptr->xmms2.status = malloc(text_buffer_size);
		ptr->xmms2.status[0] = '\0';
	}

	if (ptr->xmms2.artist == NULL) {
		ptr->xmms2.artist = malloc(text_buffer_size);
		ptr->xmms2.artist[0] = '\0';
	}

	if (ptr->xmms2.album == NULL) {
		ptr->xmms2.album = malloc(text_buffer_size);
		ptr->xmms2.album[0] = '\0';
	}

	if (ptr->xmms2.title == NULL) {
		ptr->xmms2.title = malloc(text_buffer_size);
		ptr->xmms2.title[0] = '\0';
	}

	if (ptr->xmms2.genre == NULL) {
		ptr->xmms2.genre = malloc(text_buffer_size);
		ptr->xmms2.genre[0] = '\0';
	}

	if (ptr->xmms2.comment == NULL) {
		ptr->xmms2.comment = malloc(text_buffer_size);
		ptr->xmms2.comment[0] = '\0';
	}

	if (ptr->xmms2.url == NULL) {
		ptr->xmms2.url = malloc(text_buffer_size);
		ptr->xmms2.url[0] = '\0';
	}

	if (ptr->xmms2.date == NULL) {
		ptr->xmms2.date = malloc(text_buffer_size);
		ptr->xmms2.date[0] = '\0';
	}
}

static void xmms_clear(struct information *ptr)
{
	xmms_alloc(ptr);
	ptr->xmms2.artist[0] = '\0';
	ptr->xmms2.album[0] = '\0';
	ptr->xmms2.title[0] = '\0';
	ptr->xmms2.genre[0] = '\0';
	ptr->xmms2.comment[0] = '\0';
	ptr->xmms2.url[0] = '\0';
	ptr->xmms2.date[0] = '\0';

	ptr->xmms2.tracknr = 0;
	ptr->xmms2.id = 0;
	ptr->xmms2.bitrate = 0;
	ptr->xmms2.duration = 0;
	ptr->xmms2.elapsed = 0;
	ptr->xmms2.size = 0;
	ptr->xmms2.progress = 0;
	ptr->xmms2.timesplayed = -1;
}

void connection_lost(void *p)
{
	struct information *ptr = p;
	ptr->xmms2_conn_state = CONN_NO;

	fprintf(stderr,PACKAGE_NAME": xmms2 connection failed. %s\n",
                    xmmsc_get_last_error ( ptr->xmms2_conn ));
        fflush(stderr);

	xmms_clear(ptr);
}

void handle_curent_id(xmmsc_result_t *res, void *p)
{
	uint current_id;
	struct information *ptr = p;

	if (xmmsc_result_get_uint(res, &current_id)) {

		xmmsc_result_t *res2;

		res2 = xmmsc_medialib_get_info(ptr->xmms2_conn, current_id);
		xmmsc_result_wait(res2);

		xmms_clear(ptr);

		ptr->xmms2.id = current_id;

		char *temp;

		xmmsc_result_get_dict_entry_string(res2, "artist", &temp);
		if (temp != NULL) {
			strncpy(ptr->xmms2.artist, temp, text_buffer_size - 1);
		} else {
			strncpy(ptr->xmms2.artist, "[Unknown]", text_buffer_size - 1);
		}

		xmmsc_result_get_dict_entry_string(res2, "title", &temp);
		if (temp != NULL) {
			strncpy(ptr->xmms2.title, temp, text_buffer_size - 1);
		} else {
			strncpy(ptr->xmms2.title, "[Unknown]", text_buffer_size - 1);
		}

		xmmsc_result_get_dict_entry_string(res2, "album", &temp);
		if (temp != NULL) {
			strncpy(ptr->xmms2.album, temp, text_buffer_size - 1);
		} else {
			strncpy(ptr->xmms2.album, "[Unknown]", text_buffer_size - 1);
		}

		xmmsc_result_get_dict_entry_string(res2, "genre", &temp);
		if (temp != NULL) {

			strncpy(ptr->xmms2.genre, temp, text_buffer_size - 1);
		} else {
			strncpy(ptr->xmms2.genre, "[Unknown]", text_buffer_size - 1);
		}

		xmmsc_result_get_dict_entry_string(res2, "comment", &temp);
		if (temp != NULL) {
			strncpy(ptr->xmms2.comment, temp, text_buffer_size - 1);
		} else {
			strncpy(ptr->xmms2.comment, "", text_buffer_size - 1);
		}

		xmmsc_result_get_dict_entry_string(res2, "url", &temp);
		if (temp != NULL) {
			strncpy(ptr->xmms2.url, temp, text_buffer_size - 1);
		} else {
			strncpy(ptr->xmms2.url, "[Unknown]", text_buffer_size - 1);
		}

		xmmsc_result_get_dict_entry_string(res2, "date", &temp);
		if (temp != NULL) {
			strncpy(ptr->xmms2.date, temp, text_buffer_size - 1);
		} else {
			strncpy(ptr->xmms2.date, "????", text_buffer_size - 1);
		}

		int itemp;

		xmmsc_result_get_dict_entry_int(res2, "tracknr", &itemp);
		ptr->xmms2.tracknr = itemp;

		xmmsc_result_get_dict_entry_int(res2, "duration", &itemp);
		ptr->xmms2.duration = itemp;

		xmmsc_result_get_dict_entry_int(res2, "bitrate", &itemp);
		ptr->xmms2.bitrate = itemp / 1000;

		xmmsc_result_get_dict_entry_int(res2, "size", &itemp);
		ptr->xmms2.size = (float) itemp / 1048576;

		xmmsc_result_get_dict_entry_int( res2, "timesplayed", &itemp );
		ptr->xmms2.timesplayed = itemp;

		xmmsc_result_unref(res2);
	}
}

void handle_playtime(xmmsc_result_t *res, void *p)
{
	struct information *ptr = p;
	xmmsc_result_t *res2;
	uint play_time;

	if (xmmsc_result_iserror(res)) {
		return;
	}

	if (!xmmsc_result_get_uint(res, &play_time)) {
		return;
	}

	res2 = xmmsc_result_restart(res);
	xmmsc_result_unref(res2);

	ptr->xmms2.elapsed = play_time;
	ptr->xmms2.progress = (float) play_time / ptr->xmms2.duration;
}

void handle_playback_state_change(xmmsc_result_t *res, void *p)
{
	struct information *ptr = p;
	uint pb_state = 0;

	if (xmmsc_result_iserror(res)) {
		return;
	}

	if (!xmmsc_result_get_uint(res, &pb_state)) {
		return;
	}

	switch (pb_state) {
		case XMMS_PLAYBACK_STATUS_PLAY:
			strncpy(ptr->xmms2.status, "Playing", text_buffer_size - 1);
			break;
		case XMMS_PLAYBACK_STATUS_PAUSE:
			strncpy(ptr->xmms2.status, "Paused", text_buffer_size - 1);
			break;
		case XMMS_PLAYBACK_STATUS_STOP:
			strncpy(ptr->xmms2.status, "Stopped", text_buffer_size - 1);
			break;
		default:
			strncpy(ptr->xmms2.status, "Unknown", text_buffer_size - 1);
	}
}

void handle_playlist_loaded(xmmsc_result_t *res, void *p) {
	struct information *ptr = p;

	if (ptr->xmms2.playlist == NULL) {
		ptr->xmms2.playlist = malloc(text_buffer_size);
		ptr->xmms2.playlist[0] = '\0';
	}

	if (!xmmsc_result_get_string(res, &ptr->xmms2.playlist))  {
		ptr->xmms2.playlist[0] = '\0';
	}

}

void update_xmms2()
{
	struct information *current_info = &info;

	/* initialize connection */
	if (current_info->xmms2_conn_state == CONN_INIT) {

		if (current_info->xmms2_conn == NULL) {
			current_info->xmms2_conn = xmmsc_init(PACKAGE);
		}

		/* did init fail? */
		if (current_info->xmms2_conn == NULL) {
			fprintf(stderr, PACKAGE_NAME": xmms2 init failed. %s\n",
					xmmsc_get_last_error(current_info->xmms2_conn));
			fflush(stderr);
			return;
		}

		/* init ok but not connected yet.. */
		current_info->xmms2_conn_state = CONN_NO;

		/* clear all values */
		xmms_clear(current_info);

		/* fprintf(stderr, PACKAGE_NAME": xmms2 init ok.\n");
		fflush(stderr); */
	}

	/* connect */
	if (current_info->xmms2_conn_state == CONN_NO) {

		char *path = getenv("XMMS_PATH");

		if (!xmmsc_connect(current_info->xmms2_conn, path)) {
			fprintf(stderr, PACKAGE_NAME": xmms2 connection failed. %s\n",
				xmmsc_get_last_error(current_info->xmms2_conn));
			fflush(stderr);
			current_info->xmms2_conn_state = CONN_NO;
			return;
		}

		/* set callbacks */
		xmmsc_disconnect_callback_set(current_info->xmms2_conn, connection_lost,
			current_info);
		XMMS_CALLBACK_SET(current_info->xmms2_conn,
			xmmsc_broadcast_playback_current_id, handle_curent_id,
			current_info);
		XMMS_CALLBACK_SET(current_info->xmms2_conn,
			xmmsc_signal_playback_playtime, handle_playtime, current_info);
		XMMS_CALLBACK_SET(current_info->xmms2_conn,
			xmmsc_broadcast_playback_status, handle_playback_state_change,
			current_info);
		XMMS_CALLBACK_SET(current_info->xmms2_conn,
			xmmsc_broadcast_playlist_loaded, handle_playlist_loaded,
			current_info);
		XMMS_CALLBACK_SET(current_info->xmms2_conn,
			xmmsc_broadcast_medialib_entry_changed, handle_curent_id,
			current_info);

		/* get playback status, current id and active playlist */
		XMMS_CALLBACK_SET(current_info->xmms2_conn,
				xmmsc_playback_current_id, handle_curent_id, current_info);
		XMMS_CALLBACK_SET(current_info->xmms2_conn,
				xmmsc_playback_status, handle_playback_state_change, current_info);
		XMMS_CALLBACK_SET(current_info->xmms2_conn,
				xmmsc_playlist_current_active, handle_playlist_loaded, current_info);

		/* everything seems to be ok */
		current_info->xmms2_conn_state = CONN_OK;

		/* fprintf(stderr, PACKAGE_NAME": xmms2 connected.\n");
		fflush(stderr); */
	}

	/* handle callbacks */
	if (current_info->xmms2_conn_state == CONN_OK) {
		
		xmmsc_io_in_handle(current_info->xmms2_conn);
		if (xmmsc_io_want_out(current_info->xmms2_conn)) {
			xmmsc_io_out_handle(current_info->xmms2_conn);
		}
	}
}
