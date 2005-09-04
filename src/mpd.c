#include "conky.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libmpdclient.h"


void update_mpd()
{
	struct information *current_info = &info;
	current_info->conn =
			mpd_newConnection(current_info->mpd.host,
					  current_info->mpd.port, 10);
	if (current_info->conn->error) {
		//fprintf(stderr, "%s\n", current_info->conn->errorStr);
		mpd_closeConnection(current_info->conn);
		if (current_info->mpd.artist == NULL)
			current_info->mpd.artist =
					malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.album == NULL)
			current_info->mpd.album = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.title == NULL)
			current_info->mpd.title = malloc(TEXT_BUFFER_SIZE);
		strcpy(current_info->mpd.artist, "Unknown");
		strcpy(current_info->mpd.album, "Unknown");
		strcpy(current_info->mpd.title, "Unknown");
		current_info->mpd.status = "MPD not responding";
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
		//fprintf(stderr, "%s\n", current_info->conn->errorStr);
		mpd_closeConnection(current_info->conn);
		if (current_info->mpd.artist == NULL)
			current_info->mpd.artist =
					malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.album == NULL)
			current_info->mpd.album = malloc(TEXT_BUFFER_SIZE);
		if (current_info->mpd.title == NULL)
			current_info->mpd.title = malloc(TEXT_BUFFER_SIZE);
		strcpy(current_info->mpd.artist, "Unknown");
		strcpy(current_info->mpd.album, "Unknown");
		strcpy(current_info->mpd.title, "Unknown");
		current_info->mpd.status = "MPD not responding";
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
		current_info->mpd.status = "Playing";
	}
	if (status->state == MPD_STATUS_STATE_STOP) {
		current_info->mpd.status = "Stopped";
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
		strcpy(current_info->mpd.artist, "Stopped");
		strcpy(current_info->mpd.album, "Stopped");
		strcpy(current_info->mpd.title, "Stopped");
	}
	if (status->state == MPD_STATUS_STATE_PAUSE) {
		current_info->mpd.status = "Paused";
	}
	if (status->state == MPD_STATUS_STATE_UNKNOWN) {
		current_info->mpd.status = "Unknown";
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
		strcpy(current_info->mpd.artist, "Unknown");
		strcpy(current_info->mpd.album, "Unknown");
		strcpy(current_info->mpd.title, "Unknown");
	}
	if (status->state == MPD_STATUS_STATE_PLAY ||
		   status->state == MPD_STATUS_STATE_PAUSE) {
		current_info->mpd.bitrate = status->bitRate;
		current_info->mpd.progress =
				(float) status->elapsedTime / status->totalTime;
		current_info->mpd.elapsed = status->elapsedTime;
		current_info->mpd.length = status->totalTime;
		   }


		   if (current_info->conn->error) {
		//fprintf(stderr, "%s\n", current_info->conn->errorStr);
			   mpd_closeConnection(current_info->conn);
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
			   if (song->artist) {
				   strcpy(current_info->mpd.artist, song->artist);
			   } else {
				   strcpy(current_info->mpd.artist, "Unknown");
			   }
			   if (song->album) {
				   strcpy(current_info->mpd.album, song->album);
			   } else {
				   strcpy(current_info->mpd.album, "Unknown");
			   }
			   if (song->title) {
				   strcpy(current_info->mpd.title, song->title);
			   } else {
				   strcpy(current_info->mpd.title, "Unknown");
			   }
			   if (entity != NULL) {
				   mpd_freeInfoEntity(entity);
			   }
		   }
		   if (entity != NULL) {
			   mpd_freeInfoEntity(entity);
		   }

		   if (current_info->conn->error) {
		//fprintf(stderr, "%s\n", current_info->conn->errorStr);
			   mpd_closeConnection(current_info->conn);
			   return;
		   }

		   mpd_finishCommand(current_info->conn);
		   if (current_info->conn->error) {
		//fprintf(stderr, "%s\n", current_info->conn->errorStr);
			   mpd_closeConnection(current_info->conn);
			   return;
		   }
		   mpd_freeStatus(status);
		   mpd_closeConnection(current_info->conn);
}
