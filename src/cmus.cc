/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * CMUS Conky integration
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
#include <math.h>

#include "update-cb.hh"

namespace {
	struct cmus_result {
		std::string state;
		std::string file;
		std::string title;
		std::string artist;
		std::string album;
		std::string totaltime;
		std::string curtime;
		std::string random;
		std::string repeat;
		std::string aaa;
		std::string track;
		std::string genre;
		std::string date;
		float progress;
		float timeleft;
	};

	class cmus_cb: public conky::callback<cmus_result> {
		typedef conky::callback<cmus_result> Base;

	protected:
		virtual void work();

	public:
		cmus_cb(uint32_t period)
			: Base(period, false, Tuple())
		{}
	};

	void cmus_cb::work()
	{
		cmus_result cmus;
		FILE *fp;

		fp = popen("cmus-remote -Q 2>/dev/null", "r");
		if (!fp) {
			cmus.state = "Can't run 'cmus-remote -Q'";
		} else {
			while (1) {
				char line[255];
				char *p;

				/* Read a line from the pipe and strip the possible '\n'. */
				if (!fgets(line, 255, fp))
					break;
				if ((p = strrchr(line, '\n')))
					*p = '\0';

				/* Parse infos. */
				if (strncmp(line, "status ", 7) == 0)
				    cmus.state = line + 7;

				else if (strncmp(line, "file ", 5) == 0)
				    cmus.file = line + 5;

				else if (strncmp(line, "tag artist ", 11) == 0)
				    cmus.artist = line + 11;

				else if (strncmp(line, "tag title ", 10) == 0)
				    cmus.title = line + 10;

				else if (strncmp(line, "tag album ", 10) == 0)
				    cmus.album = line + 10;

				else if (strncmp(line, "duration ", 9) == 0)
				    cmus.totaltime = line + 9;

				else if (strncmp(line, "position ", 9) == 0)
				{
				    cmus.curtime = line + 9;
				    cmus.timeleft = atoi(cmus.totaltime.c_str()) - atoi(cmus.curtime.c_str());
				    if (cmus.curtime.size() > 0)
					cmus.progress = (float) atoi(cmus.curtime.c_str()) / atoi(cmus.totaltime.c_str());
				    else
					cmus.progress = 0;
				}

				else if (strncmp(line, "set shuffle ", 12) == 0)
					cmus.random = (strncmp(line+12, "true", 4) == 0 ?
					    "on" : "off" );

				else if (strncmp(line, "set repeat ", 11) == 0)
					cmus.repeat = (strncmp((line+11), "true", 4) == 0 ?
					    "all" : "off" );

				else if (strncmp(line, "set repeat_current ", 19) == 0)
					cmus.repeat = (strncmp((line + 19), "true", 4) == 0 ?
					    "song" : cmus.repeat );
				else if (strncmp(line, "set aaa_mode ", 13) == 0)
					cmus.aaa = line + 13;

				else if (strncmp(line, "tag tracknumber ", 16) == 0)
					cmus.track = line + 16;
				else if (strncmp(line, "tag genre ", 10) == 0)
					cmus.genre = line + 10;
				else if (strncmp(line, "tag date ", 9) == 0)
					cmus.date = line + 9;
			}
		}

		pclose(fp);

		std::lock_guard<std::mutex> l(result_mutex);
		result = cmus;
	}
}

#define CMUS_PRINT_GENERATOR(type, alt) \
void print_cmus_##type(struct text_object *obj, char *p, int p_max_size) \
{ \
	(void)obj; \
	uint32_t period = std::max( \
				lround(music_player_interval.get(*state)/active_update_interval()), 1l \
			); \
	const cmus_result &cmus = conky::register_cb<cmus_cb>(period)->get_result_copy(); \
	snprintf(p, p_max_size, "%s", (cmus.type.length() ? cmus.type.c_str() : alt)); \
}

CMUS_PRINT_GENERATOR(state, "Off")
CMUS_PRINT_GENERATOR(file, "no file")
CMUS_PRINT_GENERATOR(title, "no title")
CMUS_PRINT_GENERATOR(artist, "no artist")
CMUS_PRINT_GENERATOR(album, "no album")
CMUS_PRINT_GENERATOR(random, "")
CMUS_PRINT_GENERATOR(repeat, "")
CMUS_PRINT_GENERATOR(aaa, "all")
CMUS_PRINT_GENERATOR(track, "no track")
CMUS_PRINT_GENERATOR(genre, "")
CMUS_PRINT_GENERATOR(date, "")

uint8_t cmus_percent(struct text_object *obj)
{
	(void)obj;
	uint32_t period = std::max(
	    lround(music_player_interval.get(*state)/active_update_interval()), 1l);
	const cmus_result &cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
	return (uint8_t) round(cmus.progress * 100.0f);
}

double cmus_progress(struct text_object *obj)
{
	(void)obj;
	uint32_t period = std::max(
	    lround(music_player_interval.get(*state)/active_update_interval()), 1l);
	const cmus_result &cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
	return (double) cmus.progress;
}

void print_cmus_totaltime(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	uint32_t period = std::max(
	    lround(music_player_interval.get(*state)/active_update_interval()), 1l);
	const cmus_result &cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
	format_seconds_short(p, p_max_size, atol(cmus.totaltime.c_str()));
}

void print_cmus_timeleft(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	uint32_t period = std::max(
	    lround(music_player_interval.get(*state)/active_update_interval()), 1l);
	const cmus_result &cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
	//format_seconds_short(p, p_max_size, atol(cmus.timeleft.c_str()));
	format_seconds_short(p, p_max_size, (long)cmus.timeleft);
}

void print_cmus_curtime(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	uint32_t period = std::max(
	    lround(music_player_interval.get(*state)/active_update_interval()), 1l);
	const cmus_result &cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
	format_seconds_short(p, p_max_size, atol(cmus.curtime.c_str()));
}

#undef CMUS_PRINT_GENERATOR
