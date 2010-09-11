/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * MOC Conky integration
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2008, Henri Häkkinen
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
#include "logging.h"
#include "text_object.h"
#include "timed-thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>

static struct {
	char *state;
	char *file;
	char *title;
	char *artist;
	char *song;
	char *album;
	char *totaltime;
	char *timeleft;
	char *curtime;
	char *bitrate;
	char *rate;
} moc;

static timed_thread_ptr moc_thread;

void free_moc(struct text_object *obj)
{
	(void)obj;
	free_and_zero(moc.state);
	free_and_zero(moc.file);
	free_and_zero(moc.title);
	free_and_zero(moc.artist);
	free_and_zero(moc.song);
	free_and_zero(moc.album);
	free_and_zero(moc.totaltime);
	free_and_zero(moc.timeleft);
	free_and_zero(moc.curtime);
	free_and_zero(moc.bitrate);
	free_and_zero(moc.rate);
}

static void update_infos(void)
{
	FILE *fp;

	free_moc(NULL);
	fp = popen("mocp -i", "r");
	if (!fp) {
		moc.state = strndup("Can't run 'mocp -i'", text_buffer_size.get(*state));
		return;
	}

	while (1) {
		char line[100];
		char *p;

		/* Read a line from the pipe and strip the possible '\n'. */
		if (!fgets(line, 100, fp))
			break;
		if ((p = strrchr(line, '\n')))
			*p = '\0';

		/* Parse infos. */
		if (strncmp(line, "State:", 6) == 0)
			moc.state = strndup(line + 7, text_buffer_size.get(*state));
		else if (strncmp(line, "File:", 5) == 0)
			moc.file = strndup(line + 6, text_buffer_size.get(*state));
		else if (strncmp(line, "Title:", 6) == 0)
			moc.title = strndup(line + 7, text_buffer_size.get(*state));
		else if (strncmp(line, "Artist:", 7) == 0)
			moc.artist = strndup(line + 8, text_buffer_size.get(*state));
		else if (strncmp(line, "SongTitle:", 10) == 0)
			moc.song = strndup(line + 11, text_buffer_size.get(*state));
		else if (strncmp(line, "Album:", 6) == 0)
			moc.album = strndup(line + 7, text_buffer_size.get(*state));
		else if (strncmp(line, "TotalTime:", 10) == 0)
			moc.totaltime = strndup(line + 11, text_buffer_size.get(*state));
		else if (strncmp(line, "TimeLeft:", 9) == 0)
			moc.timeleft = strndup(line + 10, text_buffer_size.get(*state));
		else if (strncmp(line, "CurrentTime:", 12) == 0)
			moc.curtime = strndup(line + 13, text_buffer_size.get(*state));
		else if (strncmp(line, "Bitrate:", 8) == 0)
			moc.bitrate = strndup(line + 9, text_buffer_size.get(*state));
		else if (strncmp(line, "Rate:", 5) == 0)
			moc.rate = strndup(line + 6, text_buffer_size.get(*state));
	}

	pclose(fp);
}

static void update_moc_loop(thread_handle &handle)
{
	while (1) {
		{
			std::lock_guard<std::mutex> lock(handle.mutex());
			update_infos();
		}
		if (handle.test(0)) {
			return;
		}
	}
	/* never reached */
}

static int run_moc_thread(std::chrono::microseconds interval)
{
	if (moc_thread)
		return 0;

	moc_thread = timed_thread::create(std::bind(update_moc_loop, std::placeholders::_1), interval);
	if (!moc_thread) {
		NORM_ERR("Failed to create MOC timed thread");
		return 1;
	}
	return 0;
}

int update_moc(void)
{
	run_moc_thread(std::chrono::microseconds(long(music_player_interval.get(*state) * 1000000)));
	return 0;
}

#define MOC_PRINT_GENERATOR(type, alt) \
void print_moc_##type(struct text_object *obj, char *p, int p_max_size) \
{ \
	(void)obj; \
	snprintf(p, p_max_size, "%s", (moc.type ? moc.type : alt)); \
}

MOC_PRINT_GENERATOR(state, "??")
MOC_PRINT_GENERATOR(file, "no file")
MOC_PRINT_GENERATOR(title, "no title")
MOC_PRINT_GENERATOR(artist, "no artist")
MOC_PRINT_GENERATOR(song, "no song")
MOC_PRINT_GENERATOR(album, "no album")
MOC_PRINT_GENERATOR(totaltime, "0:00")
MOC_PRINT_GENERATOR(timeleft, "0:00")
MOC_PRINT_GENERATOR(curtime, "0:00")
MOC_PRINT_GENERATOR(bitrate, "0Kbps")
MOC_PRINT_GENERATOR(rate, "0KHz")

#undef MOC_PRINT_GENERATOR
