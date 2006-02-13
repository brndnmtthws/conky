#include "conky.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libmpdclient.h"


void update_mpd()
{
	struct information *current_info = &info;
	if (current_info->conn == NULL) {
		current_info->conn = mpd_newConnection(current_info->mpd.host, current_info->mpd.port, 10);
	}
	if (strlen(current_info->mpd.password) > 1) {
		mpd_sendPasswordCommand(current_info->conn,
					current_info->mpd.password);
		mpd_finishCommand(current_info->conn);
	}
	if (current_info->conn->error) {
		//ERR("%MPD error: s\n", current_info->conn->errorStr);
		mpd_closeConnection(current_info->conn);
		current_info->conn = 0;
		if (current_info->mpd.artist == NULL)
			current_info->mpd.artist =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.album == NULL)
			current_info->mpd.album = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.title == NULL)
			current_info->mpd.title = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.random == NULL)
			current_info->mpd.random =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.repeat == NULL)
			current_info->mpd.repeat =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.track == NULL)
			current_info->mpd.track = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.status == NULL)
			current_info->mpd.status =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.name == NULL)
			current_info->mpd.name = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.file == NULL)
			current_info->mpd.file = malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->mpd.name, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.file, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.artist, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.album, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.title, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.random, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.repeat, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.track, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.status, "MPD not responding",	TEXT_BUFFER_SIZE - 1);
		current_info->mpd.bitrate = 0;
		current_info->mpd.progress = 0;
		current_info->mpd.elapsed = 0;
		current_info->mpd.length = 0;
		return;
	}

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
		if (current_info->mpd.artist == NULL)
			current_info->mpd.artist =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.album == NULL)
			current_info->mpd.album = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.title == NULL)
			current_info->mpd.title = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.random == NULL)
			current_info->mpd.random =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.repeat == NULL)
			current_info->mpd.repeat =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.track == NULL)
			current_info->mpd.track = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.status == NULL)
			current_info->mpd.status = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.name == NULL)
			current_info->mpd.name = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.file == NULL)
			current_info->mpd.file = malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->mpd.name, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.file, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.artist, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.album, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.title, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.random, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.repeat, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.track, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.status, "MPD not responding", TEXT_BUFFER_SIZE - 1);
		current_info->mpd.bitrate = 0;
		current_info->mpd.progress = 0;
		current_info->mpd.elapsed = 0;
		current_info->mpd.length = 0;
		return;
	}
	current_info->mpd.volume = status->volume;
	//if (status->error)
	//printf("error: %s\n", status->error);

	if (status->state == MPD_STATUS_STATE_PLAY) {
		if (current_info->mpd.status == NULL)
			current_info->mpd.status =
			    malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->mpd.status, "Playing",
			TEXT_BUFFER_SIZE - 1);
	}
	if (status->state == MPD_STATUS_STATE_STOP) {
		current_info->mpd.bitrate = 0;
		current_info->mpd.progress = 0;
		current_info->mpd.elapsed = 0;
		current_info->mpd.length = 0;
		if (current_info->mpd.artist == NULL)
			current_info->mpd.artist =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.album == NULL)
			current_info->mpd.album = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.title == NULL)
			current_info->mpd.title = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.random == NULL)
			current_info->mpd.random =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.repeat == NULL)
			current_info->mpd.repeat =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.track == NULL)
			current_info->mpd.track = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.status == NULL)
			current_info->mpd.status =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.name == NULL)
			current_info->mpd.name = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.file == NULL)
			current_info->mpd.file = malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->mpd.name, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.file, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.artist, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.album, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.title, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.random, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.repeat, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.track, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.status, "Unknown",
			TEXT_BUFFER_SIZE - 1);
	}
	if (status->state == MPD_STATUS_STATE_PAUSE) {
		if (current_info->mpd.status == NULL)
			current_info->mpd.status =
			    malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->mpd.status, "Paused",
			TEXT_BUFFER_SIZE - 1);
	}
	if (status->state == MPD_STATUS_STATE_UNKNOWN) {
		current_info->mpd.bitrate = 0;
		current_info->mpd.progress = 0;
		current_info->mpd.elapsed = 0;
		current_info->mpd.length = 0;
		if (current_info->mpd.artist == NULL)
			current_info->mpd.artist =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.album == NULL)
			current_info->mpd.album = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.title == NULL)
			current_info->mpd.title = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.random == NULL)
			current_info->mpd.random =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.repeat == NULL)
			current_info->mpd.repeat =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.track == NULL)
			current_info->mpd.track = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.status == NULL)
			current_info->mpd.status =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.name == NULL)
			current_info->mpd.name = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.file == NULL)
			current_info->mpd.file = malloc(TEXT_BUFFER_SIZE);
		strncpy(current_info->mpd.name, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.file, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.artist, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.album, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.title, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.random, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.repeat, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.track, "Unknown",
			TEXT_BUFFER_SIZE - 1);
		strncpy(current_info->mpd.status, "Unknown",
			TEXT_BUFFER_SIZE - 1);
	}
	if (status->state == MPD_STATUS_STATE_PLAY ||
	    status->state == MPD_STATUS_STATE_PAUSE) {
		current_info->mpd.bitrate = status->bitRate;
		current_info->mpd.progress =
		    (float) status->elapsedTime / status->totalTime;
		current_info->mpd.elapsed = status->elapsedTime;
		current_info->mpd.length = status->totalTime;
		if (current_info->mpd.random == NULL)
			current_info->mpd.random =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.repeat == NULL)
			current_info->mpd.repeat =
			    malloc(TEXT_BUFFER_SIZE);
		if (status->random == 0) {
			strcpy(current_info->mpd.random, "Off");
		} else if (status->random == 1) {
			strcpy(current_info->mpd.random, "On");
		} else {
			strcpy(current_info->mpd.random, "Unknown");
		}
		if (status->repeat == 0) {
			strcpy(current_info->mpd.repeat, "Off");
		} else if (status->repeat == 1) {
			strcpy(current_info->mpd.repeat, "On");
		} else {
			strcpy(current_info->mpd.repeat, "Unknown");
		}
	}

	if (current_info->conn->error) {
		//fprintf(stderr, "%s\n", current_info->conn->errorStr);
		mpd_closeConnection(current_info->conn);
		current_info->conn = 0;
		return;
	}

	mpd_nextListOkCommand(current_info->conn);

	while ((entity = mpd_getNextInfoEntity(current_info->conn))) {
		mpd_Song *song = entity->info.song;
		if (entity->type != MPD_INFO_ENTITY_TYPE_SONG) {
			mpd_freeInfoEntity(entity);
			continue;
		}

		if (current_info->mpd.artist == NULL)
			current_info->mpd.artist =
			    malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.album == NULL)
			current_info->mpd.album = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.title == NULL)
			current_info->mpd.title = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.track == NULL)
			current_info->mpd.track = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.name == NULL)
			current_info->mpd.name = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.file == NULL)
			current_info->mpd.file = malloc(TEXT_BUFFER_SIZE);
		if (song->artist) {
			strncpy(current_info->mpd.artist, song->artist,
				TEXT_BUFFER_SIZE - 1);
		} else {
			strcpy(current_info->mpd.artist, "Unknown");
		}
		if (song->album) {
			strncpy(current_info->mpd.album, song->album,
				TEXT_BUFFER_SIZE - 1);
		} else {
			strcpy(current_info->mpd.album, "Unknown");
		}
		if (song->title) {
			strncpy(current_info->mpd.title, song->title,
				TEXT_BUFFER_SIZE - 1);
		} else {
			strcpy(current_info->mpd.title, "Unknown");
		}
		if (song->track) {
			strncpy(current_info->mpd.track, song->track,
				TEXT_BUFFER_SIZE - 1);
		} else {
			strcpy(current_info->mpd.track, "Unknown");
		}
		if (song->name) {
			strncpy(current_info->mpd.name, song->name,
				TEXT_BUFFER_SIZE - 1);
		} else {
			strcpy(current_info->mpd.name, "Unknown");
		}
		if (song->file) {
			strncpy(current_info->mpd.file,
				song->file, TEXT_BUFFER_SIZE - 1);
		} else {
			strcpy(current_info->mpd.file, "Unknown");
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

	if (current_info->conn->error) {
		//fprintf(stderr, "%s\n", current_info->conn->errorStr);
		mpd_closeConnection(current_info->conn);
		current_info->conn = 0;
		return;
	}

	mpd_finishCommand(current_info->conn);
	if (current_info->conn->error) {
		//fprintf(stderr, "%s\n", current_info->conn->errorStr);
		mpd_closeConnection(current_info->conn);
		current_info->conn = 0;
		return;
	}
	mpd_freeStatus(status);
//	mpd_closeConnection(current_info->conn);
}
