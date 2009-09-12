/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
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
#include "moc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define xfree(x) if (x) free(x); x = 0

struct moc_s moc;
static timed_thread *moc_thread = NULL;

void free_moc(void)
{
	xfree(moc.state);
	xfree(moc.file);
	xfree(moc.title);
	xfree(moc.artist);
	xfree(moc.song);
	xfree(moc.album);
	xfree(moc.totaltime);
	xfree(moc.timeleft);
	xfree(moc.curtime);
	xfree(moc.bitrate);
	xfree(moc.rate);
}

static void update_infos(void)
{
	FILE *fp;

	free_moc();
	fp = popen("mocp -i", "r");
	if (!fp) {
		moc.state = strndup("Can't run 'mocp -i'", text_buffer_size);
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
			moc.state = strndup(line + 7, text_buffer_size);
		else if (strncmp(line, "File:", 5) == 0)
			moc.file = strndup(line + 6, text_buffer_size);
		else if (strncmp(line, "Title:", 6) == 0)
			moc.title = strndup(line + 7, text_buffer_size);
		else if (strncmp(line, "Artist:", 7) == 0)
			moc.artist = strndup(line + 8, text_buffer_size);
		else if (strncmp(line, "SongTitle:", 10) == 0)
			moc.song = strndup(line + 11, text_buffer_size);
		else if (strncmp(line, "Album:", 6) == 0)
			moc.album = strndup(line + 7, text_buffer_size);
		else if (strncmp(line, "TotalTime:", 10) == 0)
			moc.totaltime = strndup(line + 11, text_buffer_size);
		else if (strncmp(line, "TimeLeft:", 9) == 0)
			moc.timeleft = strndup(line + 10, text_buffer_size);
		else if (strncmp(line, "CurrentTime:", 12) == 0)
			moc.curtime = strndup(line + 13, text_buffer_size);
		else if (strncmp(line, "Bitrate:", 8) == 0)
			moc.bitrate = strndup(line + 9, text_buffer_size);
		else if (strncmp(line, "Rate:", 5) == 0)
			moc.rate = strndup(line + 6, text_buffer_size);
	}

	pclose(fp);
}

static void *update_moc_loop(void *) __attribute__((noreturn));

static void *update_moc_loop(void *arg)
{
	(void)arg;

	while (1) {
		timed_thread_lock(moc_thread);
		update_infos();
		timed_thread_unlock(moc_thread);
		if (timed_thread_test(moc_thread, 0)) {
			timed_thread_exit(moc_thread);
		}
	}
	/* never reached */
}

static int run_moc_thread(double interval)
{
	if (moc_thread)
		return 0;

	moc_thread = timed_thread_create(&update_moc_loop, NULL, interval);
	if (!moc_thread) {
		NORM_ERR("Failed to create MOC timed thread");
		return 1;
	}
	timed_thread_register(moc_thread, &moc_thread);
	if (timed_thread_run(moc_thread)) {
		NORM_ERR("Failed to run MOC timed thread");
		return 2;
	}
	return 0;
}

void update_moc(void)
{
	run_moc_thread(info.music_player_interval * 100000);
}
