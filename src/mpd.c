/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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

void init_mpd_stats(struct mpd_s *mpd)
{
	if (mpd->artist == NULL) {
		mpd->artist = malloc(text_buffer_size);
	}
	if (mpd->album == NULL) {
		mpd->album = malloc(text_buffer_size);
	}
	if (mpd->title == NULL) {
		mpd->title = malloc(text_buffer_size);
	}
	if (mpd->random == NULL) {
		mpd->random = malloc(text_buffer_size);
	}
	if (mpd->repeat == NULL) {
		mpd->repeat = malloc(text_buffer_size);
	}
	if (mpd->track == NULL) {
		mpd->track = malloc(text_buffer_size);
	}
	if (mpd->status == NULL) {
		mpd->status = malloc(text_buffer_size);
	}
	if (mpd->name == NULL) {
		mpd->name = malloc(text_buffer_size);
	}
	if (mpd->file == NULL) {
		mpd->file = malloc(text_buffer_size);
	}
	clear_mpd_stats(mpd);
}

void free_mpd_vars(struct mpd_s *mpd)
{
	if (mpd->title) {
		free(mpd->title);
		mpd->title = NULL;
	}
	if (mpd->artist) {
		free(mpd->artist);
		mpd->artist = NULL;
	}
	if (mpd->album) {
		free(mpd->album);
		mpd->album = NULL;
	}
	if (mpd->random) {
		free(mpd->random);
		mpd->random = NULL;
	}
	if (mpd->repeat) {
		free(mpd->repeat);
		mpd->repeat = NULL;
	}
	if (mpd->track) {
		free(mpd->track);
		mpd->track = NULL;
	}
	if (mpd->name) {
		free(mpd->name);
		mpd->name = NULL;
	}
	if (mpd->file) {
		free(mpd->file);
		mpd->file = NULL;
	}
	if (mpd->status) {
		free(mpd->status);
		mpd->status = NULL;
	}
	if (mpd->conn) {
		mpd_closeConnection(mpd->conn);
		mpd->conn = 0;
	}
}

void clear_mpd_stats(struct mpd_s *mpd)
{
	*mpd->name = 0;
	*mpd->file = 0;
	*mpd->artist = 0;
	*mpd->album = 0;
	*mpd->title = 0;
	*mpd->random = 0;
	*mpd->repeat = 0;
	*mpd->track = 0;
	*mpd->status = 0;
	mpd->bitrate = 0;
	mpd->progress = 0;
	mpd->elapsed = 0;
	mpd->length = 0;
}

void *update_mpd(void *arg)
{
	struct mpd_s *mpd;

	if (arg == NULL) {
		CRIT_ERR("update_mpd called with a null argument!");
	}

	mpd = (struct mpd_s *) arg;

	while (1) {
		mpd_Status *status;
		mpd_InfoEntity *entity;

		if (!mpd->conn) {
			mpd->conn = mpd_newConnection(mpd->host,
				mpd->port, 10);
		}
		if (strlen(mpd->password) > 1) {
			mpd_sendPasswordCommand(mpd->conn,
				mpd->password);
			mpd_finishCommand(mpd->conn);
		}

		timed_thread_lock(mpd->timed_thread);

		if (mpd->conn->error || mpd->conn == NULL) {
			ERR("MPD error: %s\n", mpd->conn->errorStr);
			mpd_closeConnection(mpd->conn);
			mpd->conn = 0;
			clear_mpd_stats(mpd);

			strncpy(mpd->status, "MPD not responding",
				text_buffer_size - 1);
			timed_thread_unlock(mpd->timed_thread);
			if (timed_thread_test(mpd->timed_thread)) {
				timed_thread_exit(mpd->timed_thread);
			}
			continue;
		}

		mpd_sendStatusCommand(mpd->conn);
		if ((status = mpd_getStatus(mpd->conn)) == NULL) {
			ERR("MPD error: %s\n", mpd->conn->errorStr);
			mpd_closeConnection(mpd->conn);
			mpd->conn = 0;
			clear_mpd_stats(mpd);

			strncpy(mpd->status, "MPD not responding",
				text_buffer_size - 1);
			timed_thread_unlock(mpd->timed_thread);
			if (timed_thread_test(mpd->timed_thread)) {
				timed_thread_exit(mpd->timed_thread);
			}
			continue;
		}
		mpd_finishCommand(mpd->conn);
		if (mpd->conn->error) {
			// fprintf(stderr, "%s\n", mpd->conn->errorStr);
			mpd_closeConnection(mpd->conn);
			mpd->conn = 0;
			timed_thread_unlock(mpd->timed_thread);
			if (timed_thread_test(mpd->timed_thread)) {
				timed_thread_exit(mpd->timed_thread);
			}
			continue;
		}

		mpd->volume = status->volume;
		/* if (status->error) {
			printf("error: %s\n", status->error);
		} */

		if (status->state == MPD_STATUS_STATE_PLAY) {
			strncpy(mpd->status, "Playing", text_buffer_size - 1);
		}
		if (status->state == MPD_STATUS_STATE_STOP) {
			clear_mpd_stats(mpd);
			strncpy(mpd->status, "Stopped", text_buffer_size - 1);
		}
		if (status->state == MPD_STATUS_STATE_PAUSE) {
			strncpy(mpd->status, "Paused", text_buffer_size - 1);
		}
		if (status->state == MPD_STATUS_STATE_UNKNOWN) {
			clear_mpd_stats(mpd);
			*mpd->status = 0;
		}
		if (status->state == MPD_STATUS_STATE_PLAY
				|| status->state == MPD_STATUS_STATE_PAUSE) {
			mpd->bitrate = status->bitRate;
			mpd->progress = (float) status->elapsedTime /
				status->totalTime;
			mpd->elapsed = status->elapsedTime;
			mpd->length = status->totalTime;
			if (status->random == 0) {
				strcpy(mpd->random, "Off");
			} else if (status->random == 1) {
				strcpy(mpd->random, "On");
			} else {
				*mpd->random = 0;
			}
			if (status->repeat == 0) {
				strcpy(mpd->repeat, "Off");
			} else if (status->repeat == 1) {
				strcpy(mpd->repeat, "On");
			} else {
				*mpd->repeat = 0;
			}
		}

		if (mpd->conn->error) {
			// fprintf(stderr, "%s\n", mpd->conn->errorStr);
			mpd_closeConnection(mpd->conn);
			mpd->conn = 0;
			timed_thread_unlock(mpd->timed_thread);
			if (timed_thread_test(mpd->timed_thread)) {
				timed_thread_exit(mpd->timed_thread);
			}
			continue;
		}

		mpd_sendCurrentSongCommand(mpd->conn);
		while ((entity = mpd_getNextInfoEntity(mpd->conn))) {
			mpd_Song *song = entity->info.song;

			if (entity->type != MPD_INFO_ENTITY_TYPE_SONG) {
				mpd_freeInfoEntity(entity);
				continue;
			}

			if (song->artist) {
				strncpy(mpd->artist, song->artist,
					text_buffer_size - 1);
			} else {
				*mpd->artist = 0;
			}
			if (song->album) {
				strncpy(mpd->album, song->album,
					text_buffer_size - 1);
			} else {
				*mpd->album = 0;
			}
			if (song->title) {
				strncpy(mpd->title, song->title,
					text_buffer_size - 1);
			} else {
				*mpd->title = 0;
			}
			if (song->track) {
				strncpy(mpd->track, song->track,
					text_buffer_size - 1);
			} else {
				*mpd->track = 0;
			}
			if (song->name) {
				strncpy(mpd->name, song->name,
					text_buffer_size - 1);
			} else {
				*mpd->name = 0;
			}
			if (song->file) {
				strncpy(mpd->file, song->file,
					text_buffer_size - 1);
			} else {
				*mpd->file = 0;
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
		mpd_finishCommand(mpd->conn);
		if (mpd->conn->error) {
			// fprintf(stderr, "%s\n", mpd->conn->errorStr);
			mpd_closeConnection(mpd->conn);
			mpd->conn = 0;
			timed_thread_unlock(mpd->timed_thread);
			if (timed_thread_test(mpd->timed_thread)) {
				timed_thread_exit(mpd->timed_thread);
			}
			continue;
		}

		timed_thread_unlock(mpd->timed_thread);
		if (mpd->conn->error) {
			// fprintf(stderr, "%s\n", mpd->conn->errorStr);
			mpd_closeConnection(mpd->conn);
			mpd->conn = 0;
			if (timed_thread_test(mpd->timed_thread)) {
				timed_thread_exit(mpd->timed_thread);
			}
			continue;
		}

		mpd_freeStatus(status);
		/* if (mpd->conn) {
			mpd_closeConnection(mpd->conn);
			mpd->conn = 0;
		} */
		if (timed_thread_test(mpd->timed_thread)) {
			timed_thread_exit(mpd->timed_thread);
		}
		continue;
	}
	/* never reached */
}
