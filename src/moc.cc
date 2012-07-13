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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <mutex>

#include "update-cb.hh"

namespace {
	struct moc_result {
		std::string state;
		std::string file;
		std::string title;
		std::string artist;
		std::string song;
		std::string album;
		std::string totaltime;
		std::string timeleft;
		std::string curtime;
		std::string bitrate;
		std::string rate;
	};

	class moc_cb: public conky::callback<moc_result> {
		typedef conky::callback<moc_result> Base;

	protected:
		virtual void work();

	public:
		moc_cb(uint32_t period)
			: Base(period, false, Tuple())
		{}
	};

	void moc_cb::work()
	{
		moc_result moc;
		FILE *fp;

		fp = popen("mocp -i", "r");
		if (!fp) {
			moc.state = "Can't run 'mocp -i'";
		} else {
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
					moc.state = line + 7;
				else if (strncmp(line, "File:", 5) == 0)
					moc.file = line + 6;
				else if (strncmp(line, "Title:", 6) == 0)
					moc.title = line + 7;
				else if (strncmp(line, "Artist:", 7) == 0)
					moc.artist = line + 8;
				else if (strncmp(line, "SongTitle:", 10) == 0)
					moc.song = line + 11;
				else if (strncmp(line, "Album:", 6) == 0)
					moc.album = line + 7;
				else if (strncmp(line, "TotalTime:", 10) == 0)
					moc.totaltime = line + 11;
				else if (strncmp(line, "TimeLeft:", 9) == 0)
					moc.timeleft = line + 10;
				else if (strncmp(line, "CurrentTime:", 12) == 0)
					moc.curtime = line + 13;
				else if (strncmp(line, "Bitrate:", 8) == 0)
					moc.bitrate = line + 9;
				else if (strncmp(line, "Rate:", 5) == 0)
					moc.rate = line + 6;
			}
		}

		pclose(fp);

		std::lock_guard<std::mutex> l(result_mutex);
		result = moc;
	}
}

#define MOC_PRINT_GENERATOR(type, alt) \
void print_moc_##type(struct text_object *obj, char *p, int p_max_size) \
{ \
	(void)obj; \
	uint32_t period = std::max( \
				lround(music_player_interval.get(*state)/active_update_interval()), 1l \
			); \
	const moc_result &moc = conky::register_cb<moc_cb>(period)->get_result_copy(); \
	snprintf(p, p_max_size, "%s", (moc.type.length() ? moc.type.c_str() : alt)); \
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
