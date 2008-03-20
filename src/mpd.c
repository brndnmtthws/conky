/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2007 Brenden Matthews, Philip Kovacs, et. al.
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
 * $Id$ */

#include "conky.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libmpdclient.h"

timed_thread *mpd_timed_thread = NULL;

void clear_mpd_stats(struct information *current_info);

void init_mpd_stats(struct information *current_info)
{
	if (current_info->mpd.artist == NULL) {
		current_info->mpd.artist = malloc(text_buffer_size);
	}
	if (current_info->mpd.album == NULL) {
		current_info->mpd.album = malloc(text_buffer_size);
	}
	if (current_info->mpd.title == NULL) {
		current_info->mpd.title = malloc(text_buffer_size);
	}
	if (current_info->mpd.random == NULL) {
		current_info->mpd.random = malloc(text_buffer_size);
	}
	if (current_info->mpd.repeat == NULL) {
		current_info->mpd.repeat = malloc(text_buffer_size);
	}
	if (current_info->mpd.track == NULL) {
		current_info->mpd.track = malloc(text_buffer_size);
	}
	if (current_info->mpd.status == NULL) {
		current_info->mpd.status = malloc(text_buffer_size);
	}
	if (current_info->mpd.name == NULL) {
		current_info->mpd.name = malloc(text_buffer_size);
	}
	if (current_info->mpd.file == NULL) {
		current_info->mpd.file = malloc(text_buffer_size);
	}
	clear_mpd_stats(current_info);
}

void free_mpd_vars(struct information *current_info)
{
	if (current_info->mpd.title) {
		free(current_info->mpd.title);
		current_info->mpd.title = NULL;
	}
	if (current_info->mpd.artist) {
		free(current_info->mpd.artist);
		current_info->mpd.artist = NULL;
	}
	if (current_info->mpd.album) {
		free(current_info->mpd.album);
		current_info->mpd.album = NULL;
	}
	if (current_info->mpd.random) {
		free(current_info->mpd.random);
		current_info->mpd.random = NULL;
	}
	if (current_info->mpd.repeat) {
		free(current_info->mpd.repeat);
		current_info->mpd.repeat = NULL;
	}
	if (current_info->mpd.track) {
		free(current_info->mpd.track);
		current_info->mpd.track = NULL;
	}
	if (current_info->mpd.name) {
		free(current_info->mpd.name);
		current_info->mpd.name = NULL;
	}
	if (current_info->mpd.file) {
		free(current_info->mpd.file);
		current_info->mpd.file = NULL;
	}
	if (current_info->mpd.status) {
		free(current_info->mpd.status);
		current_info->mpd.status = NULL;
	}
	if (current_info->conn) {
		mpd_closeConnection(current_info->conn);
		current_info->conn = 0;
	}
}

void clear_mpd_stats(struct information *current_info)
{
	*current_info->mpd.name = 0;
	*current_info->mpd.file = 0;
	*current_info->mpd.artist = 0;
	*current_info->mpd.album = 0;
	*current_info->mpd.title = 0;
	*current_info->mpd.random = 0;
	*current_info->mpd.repeat = 0;
	*current_info->mpd.track = 0;
	*current_info->mpd.status = 0;
	current_info->mpd.bitrate = 0;
	current_info->mpd.progress = 0;
	current_info->mpd.elapsed = 0;
	current_info->mpd.length = 0;
}

void *update_mpd(void)
{
	struct information *current_info = &info;

	while (1) {
		if (!current_info->conn) {
			current_info->conn = mpd_newConnection(current_info->mpd.host,
				current_info->mpd.port, 10);
		}
		if (strlen(current_info->mpd.password) > 1) {
			mpd_sendPasswordCommand(current_info->conn,
				current_info->mpd.password);
			mpd_finishCommand(current_info->conn);
		}

		timed_thread_lock(mpd_timed_thread);

		if (current_info->conn->error || current_info->conn == NULL) {
			// ERR("%MPD error: s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
			clear_mpd_stats(current_info);

			strncpy(current_info->mpd.status, "MPD not responding",
				text_buffer_size - 1);
			timed_thread_unlock(mpd_timed_thread);
			if (timed_thread_test(mpd_timed_thread)) {
				timed_thread_exit(mpd_timed_thread);
			}
			continue;
		}

		mpd_Status *status;
		mpd_InfoEntity *entity;

		mpd_sendStatusCommand(current_info->conn);
		if ((status = mpd_getStatus(current_info->conn)) == NULL) {
			// ERR("MPD error: %s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
			clear_mpd_stats(current_info);

			strncpy(current_info->mpd.status, "MPD not responding",
				text_buffer_size - 1);
			timed_thread_unlock(mpd_timed_thread);
			if (timed_thread_test(mpd_timed_thread)) {
				timed_thread_exit(mpd_timed_thread);
			}
			continue;
		}
		mpd_finishCommand(current_info->conn);
		if (current_info->conn->error) {
			// fprintf(stderr, "%s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
			timed_thread_unlock(mpd_timed_thread);
			if (timed_thread_test(mpd_timed_thread)) {
				timed_thread_exit(mpd_timed_thread);
			}
			continue;
		}

		current_info->mpd.volume = status->volume;
		/* if (status->error) {
			printf("error: %s\n", status->error);
		} */

		if (status->state == MPD_STATUS_STATE_PLAY) {
			strncpy(current_info->mpd.status, "Playing", text_buffer_size - 1);
		}
		if (status->state == MPD_STATUS_STATE_STOP) {
			clear_mpd_stats(current_info);
			strncpy(current_info->mpd.status, "Stopped", text_buffer_size - 1);
		}
		if (status->state == MPD_STATUS_STATE_PAUSE) {
			strncpy(current_info->mpd.status, "Paused", text_buffer_size - 1);
		}
		if (status->state == MPD_STATUS_STATE_UNKNOWN) {
			clear_mpd_stats(current_info);
			*current_info->mpd.status = 0;
		}
		if (status->state == MPD_STATUS_STATE_PLAY
				|| status->state == MPD_STATUS_STATE_PAUSE) {
			current_info->mpd.bitrate = status->bitRate;
			current_info->mpd.progress = (float) status->elapsedTime /
				status->totalTime;
			current_info->mpd.elapsed = status->elapsedTime;
			current_info->mpd.length = status->totalTime;
			if (status->random == 0) {
				strcpy(current_info->mpd.random, "Off");
			} else if (status->random == 1) {
				strcpy(current_info->mpd.random, "On");
			} else {
				*current_info->mpd.random = 0;
			}
			if (status->repeat == 0) {
				strcpy(current_info->mpd.repeat, "Off");
			} else if (status->repeat == 1) {
				strcpy(current_info->mpd.repeat, "On");
			} else {
				*current_info->mpd.repeat = 0;
			}
		}

		if (current_info->conn->error) {
			// fprintf(stderr, "%s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
			timed_thread_unlock(mpd_timed_thread);
			if (timed_thread_test(mpd_timed_thread)) {
				timed_thread_exit(mpd_timed_thread);
			}
			continue;
		}

		mpd_sendCurrentSongCommand(current_info->conn);
		while ((entity = mpd_getNextInfoEntity(current_info->conn))) {
			mpd_Song *song = entity->info.song;

			if (entity->type != MPD_INFO_ENTITY_TYPE_SONG) {
				mpd_freeInfoEntity(entity);
				continue;
			}

			if (song->artist) {
				strncpy(current_info->mpd.artist, song->artist,
					text_buffer_size - 1);
			} else {
				*current_info->mpd.artist = 0;
			}
			if (song->album) {
				strncpy(current_info->mpd.album, song->album,
					text_buffer_size - 1);
			} else {
				*current_info->mpd.album = 0;
			}
			if (song->title) {
				strncpy(current_info->mpd.title, song->title,
					text_buffer_size - 1);
			} else {
				*current_info->mpd.title = 0;
			}
			if (song->track) {
				strncpy(current_info->mpd.track, song->track,
					text_buffer_size - 1);
			} else {
				*current_info->mpd.track = 0;
			}
			if (song->name) {
				strncpy(current_info->mpd.name, song->name,
					text_buffer_size - 1);
			} else {
				*current_info->mpd.name = 0;
			}
			if (song->file) {
				strncpy(current_info->mpd.file, song->file,
					text_buffer_size - 1);
			} else {
				*current_info->mpd.file = 0;
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
		mpd_finishCommand(current_info->conn);
		if (current_info->conn->error) {
			// fprintf(stderr, "%s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
			timed_thread_unlock(mpd_timed_thread);
			if (timed_thread_test(mpd_timed_thread)) {
				timed_thread_exit(mpd_timed_thread);
			}
			continue;
		}

		timed_thread_unlock(mpd_timed_thread);
		if (current_info->conn->error) {
			// fprintf(stderr, "%s\n", current_info->conn->errorStr);
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
			if (timed_thread_test(mpd_timed_thread)) {
				timed_thread_exit(mpd_timed_thread);
			}
			continue;
		}

		mpd_freeStatus(status);
		/* if (current_info->conn) {
			mpd_closeConnection(current_info->conn);
			current_info->conn = 0;
		} */
		if (timed_thread_test(mpd_timed_thread)) {
			timed_thread_exit(mpd_timed_thread);
		}
		continue;
	}
	return 0;
}
