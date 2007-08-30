/*
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al. (see AUTHORS)
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
 *  $Id$
 */

#include "conky.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libmpdclient.h"

timed_thread *mpd_timed_thread = NULL;

static void clear_mpd_stats(struct information *current_info)
{
	if (current_info->mpd.artist == NULL)
		current_info->mpd.artist = malloc(TEXT_BUFFER_SIZE);
	if (current_info->mpd.album == NULL)
		current_info->mpd.album = malloc(TEXT_BUFFER_SIZE);
	if (current_info->mpd.title == NULL)
		current_info->mpd.title = malloc(TEXT_BUFFER_SIZE);
	if (current_info->mpd.random == NULL)
		current_info->mpd.random = malloc(TEXT_BUFFER_SIZE);
	if (current_info->mpd.repeat == NULL)
		current_info->mpd.repeat = malloc(TEXT_BUFFER_SIZE);
	if (current_info->mpd.track == NULL)
		current_info->mpd.track = malloc(TEXT_BUFFER_SIZE);
	if (current_info->mpd.status == NULL)
		current_info->mpd.status = malloc(TEXT_BUFFER_SIZE);
	if (current_info->mpd.name == NULL)
		current_info->mpd.name = malloc(TEXT_BUFFER_SIZE);
	if (current_info->mpd.file == NULL)
		current_info->mpd.file = malloc(TEXT_BUFFER_SIZE);

	*current_info->mpd.name=0;
	*current_info->mpd.file=0;
	*current_info->mpd.artist=0;
	*current_info->mpd.album=0;
	*current_info->mpd.title=0;
	*current_info->mpd.random=0;
	*current_info->mpd.repeat=0;
	*current_info->mpd.track=0;
	current_info->mpd.bitrate = 0;
	current_info->mpd.progress = 0;
	current_info->mpd.elapsed = 0;
	current_info->mpd.length = 0;
}

void *update_mpd(void)
{
	while (1) {
		struct information *current_info = &info;
		if (current_info->conn == NULL) {
			current_info->conn = mpd_newConnection(current_info->mpd.host, current_info->mpd.port, 10);
		}
		if (strlen(current_info->mpd.password) > 1) {
			mpd_sendPasswordCommand(current_info->conn,
					current_info->mpd.password);
			mpd_finishCommand(current_info->conn);
		}

		// This makes sure everything we need is malloc'ed and clear
		clear_mpd_stats(current_info); 

		if (current_info->conn->error) {
			//ERR("%MPD error: s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;

			strncpy(current_info->mpd.status, "MPD not responding",	TEXT_BUFFER_SIZE - 1);
			if (timed_thread_test(mpd_timed_thread))
				timed_thread_exit(mpd_timed_thread);
			continue;
		}

		timed_thread_lock(mpd_timed_thread);
		mpd_Status *status;
		mpd_InfoEntity *entity;
		mpd_sendCommandListOkBegin(current_info->conn);
		mpd_sendStatusCommand(current_info->conn);
		mpd_sendCurrentSongCommand(current_info->conn);
		mpd_sendCommandListEnd(current_info->conn);
		if ((status = mpd_getStatus(current_info->conn)) == NULL) {
			//ERR("MPD error: %s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;

			strncpy(current_info->mpd.status, "MPD not responding", TEXT_BUFFER_SIZE - 1);
			timed_thread_unlock(mpd_timed_thread);
			if (timed_thread_test(mpd_timed_thread))
				timed_thread_exit(mpd_timed_thread);
			continue;
		}

		current_info->mpd.volume = status->volume;
		//if (status->error)
		//printf("error: %s\n", status->error);

		if (status->state == MPD_STATUS_STATE_PLAY) {
			strncpy(current_info->mpd.status, "Playing",
					TEXT_BUFFER_SIZE - 1);
		}
		if (status->state == MPD_STATUS_STATE_STOP) {
			strncpy(current_info->mpd.status, "Stopped",
					TEXT_BUFFER_SIZE - 1);
		}
		if (status->state == MPD_STATUS_STATE_PAUSE) {
			strncpy(current_info->mpd.status, "Paused",
					TEXT_BUFFER_SIZE - 1);
		}
		if (status->state == MPD_STATUS_STATE_UNKNOWN) {
			// current_info was already cleaned up by clear_mpd_stats()
		}
		if (status->state == MPD_STATUS_STATE_PLAY ||
				status->state == MPD_STATUS_STATE_PAUSE) {
			current_info->mpd.bitrate = status->bitRate;
			current_info->mpd.progress =
				(float) status->elapsedTime / status->totalTime;
			current_info->mpd.elapsed = status->elapsedTime;
			current_info->mpd.length = status->totalTime;
			if (status->random == 0) {
				strcpy(current_info->mpd.random, "Off");
			} else if (status->random == 1) {
				strcpy(current_info->mpd.random, "On");
			} else {
				*current_info->mpd.random=0;
			}
			if (status->repeat == 0) {
				strcpy(current_info->mpd.repeat, "Off");
			} else if (status->repeat == 1) {
				strcpy(current_info->mpd.repeat, "On");
			} else {
				*current_info->mpd.repeat=0;
			}
		}

		if (current_info->conn->error) {
			//fprintf(stderr, "%s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
			timed_thread_unlock(mpd_timed_thread);
			if (timed_thread_test(mpd_timed_thread))
				timed_thread_exit(mpd_timed_thread);
			continue;
		}

		mpd_nextListOkCommand(current_info->conn);

		while ((entity = mpd_getNextInfoEntity(current_info->conn))) {
			mpd_Song *song = entity->info.song;
			if (entity->type != MPD_INFO_ENTITY_TYPE_SONG) {
				mpd_freeInfoEntity(entity);
				continue;
			}

			if (song->artist) {
				strncpy(current_info->mpd.artist, song->artist,
						TEXT_BUFFER_SIZE - 1);
			} else {
				*current_info->mpd.artist=0;
			}
			if (song->album) {
				strncpy(current_info->mpd.album, song->album,
						TEXT_BUFFER_SIZE - 1);
			} else {
				*current_info->mpd.album=0;
			}
			if (song->title) {
				strncpy(current_info->mpd.title, song->title,
						TEXT_BUFFER_SIZE - 1);
			} else {
				*current_info->mpd.title=0;
			}
			if (song->track) {
				strncpy(current_info->mpd.track, song->track,
						TEXT_BUFFER_SIZE - 1);
			} else {
				*current_info->mpd.track=0;
			}
			if (song->name) {
				strncpy(current_info->mpd.name, song->name,
						TEXT_BUFFER_SIZE - 1);
			} else {
				*current_info->mpd.name=0;
			}
			if (song->file) {
				strncpy(current_info->mpd.file,
						song->file, TEXT_BUFFER_SIZE - 1);
			} else {
				*current_info->mpd.file=0;
			}
			if (entity != NULL) {
				mpd_freeInfoEntity(entity);
				entity = NULL;
			}
		}
		if (entity != NULL) {
			mpd_freeInfoEntity(entity);
			entity = NULL;
		}

		timed_thread_unlock(mpd_timed_thread);
		if (current_info->conn->error) {
			//fprintf(stderr, "%s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
			if (timed_thread_test(mpd_timed_thread))
				timed_thread_exit(mpd_timed_thread);
			continue;
		}

		mpd_finishCommand(current_info->conn);
		if (current_info->conn->error) {
			//fprintf(stderr, "%s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
			if (timed_thread_test(mpd_timed_thread))
				timed_thread_exit(mpd_timed_thread);
			continue;
		}
		mpd_freeStatus(status);
		if (current_info->conn) {
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
		}
		if (timed_thread_test(mpd_timed_thread))
			timed_thread_exit(mpd_timed_thread);
			continue;
	}
	return 0;
}
