/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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

#include <cmath>
#include <mutex>
#include "conky.h"
#include "logging.h"
#include "timeinfo.h"
#include "libmpdclient.h"
#include "mpd.h"
#include "update-cb.hh"

namespace {

	/* this is true if the current host was set from MPD_HOST */
	bool mpd_environment_host = false;

	class mpd_host_setting: public conky::simple_config_setting<std::string> {
		typedef conky::simple_config_setting<std::string> Base;
	
	protected:
		virtual void lua_setter(lua::state &l, bool init);

	public:
		mpd_host_setting()
			: Base("mpd_host", "localhost", false)
		{}
	};

	void mpd_host_setting::lua_setter(lua::state &l, bool init)
	{
		lua::stack_sentry s(l, -2);

		if(l.isnil(-2)) {
			// get the value from environment
			mpd_environment_host = true;
			const char *t = getenv("MPD_HOST");
			if(t) {
				l.checkstack(1);
				const char *h = strchr(t, '@');
				if(h) {
					if(h[1])
						l.pushstring(h+1);
				} else
					l.pushstring(t);
				l.replace(-3);
			}

		}

		Base::lua_setter(l, init);

		++s;
	}

	class mpd_password_setting: public conky::simple_config_setting<std::string> {
		typedef conky::simple_config_setting<std::string> Base;
	
	protected:
		virtual void lua_setter(lua::state &l, bool init);

	public:
		mpd_password_setting()
			: Base("mpd_password", std::string(), false)
		{}
	};

	void mpd_password_setting::lua_setter(lua::state &l, bool init)
	{
		lua::stack_sentry s(l, -2);

		/* for security, dont use environment password when user specifies host in config */
		if(l.isnil(-2) && mpd_environment_host) {
			// get the value from environment
			const char *t = getenv("MPD_HOST");
			if(t) {
				const char *p = strchr(t, '@');
				if(p) {
					l.checkstack(1);
					l.pushstring(t, p-t);
					l.replace(-3);
				}
			}

		}

		Base::lua_setter(l, init);

		++s;
	}

	conky::range_config_setting<in_port_t> mpd_port("mpd_port", 1, 65535, 6600, false);
	mpd_host_setting                 mpd_host;
	mpd_password_setting			 mpd_password;

	struct mpd_result {
		std::string title;
		std::string artist;
		std::string albumartist;
		std::string album;
		std::string date;
		std::string status;
		std::string random;
		std::string repeat;
		std::string track;
		std::string name;
		std::string file;
		int is_playing;
		int vol;
		float progress;
		int bitrate;
		int length;
		int elapsed;
	};

	class mpd_cb: public conky::callback<mpd_result> {
		typedef conky::callback<mpd_result> Base;
	
		mpd_Connection *conn;

	protected:
		virtual void work();
	
	public:
		mpd_cb(uint32_t period)
			: Base(period, false, Tuple()), conn(NULL)
		{}

		~mpd_cb()
		{
			if(conn)
				mpd_closeConnection(conn);
		}
	};

	void mpd_cb::work()
	{
		mpd_Status *status;
		mpd_InfoEntity *entity;
		mpd_result mpd_info;
		
		do {
			if (!conn)
				conn = mpd_newConnection(mpd_host.get(*state).c_str(), mpd_port.get(*state), 10);

			if (mpd_password.get(*state).size()) {
				mpd_sendPasswordCommand(conn, mpd_password.get(*state).c_str());
				mpd_finishCommand(conn);
			}

			if (conn->error) {
				NORM_ERR("MPD error: %s\n", conn->errorStr);
				mpd_closeConnection(conn);
				conn = 0;

				mpd_info.status = "MPD not responding";
				break;
			}

			mpd_sendStatusCommand(conn);
			if ((status = mpd_getStatus(conn)) == NULL) {
				NORM_ERR("MPD error: %s\n", conn->errorStr);
				mpd_closeConnection(conn);
				conn = 0;

				mpd_info.status = "MPD not responding";
			}
			mpd_finishCommand(conn);
			if (conn->error) {
				// fprintf(stderr, "%s\n", conn->errorStr);
				mpd_closeConnection(conn);
				conn = 0;
				break;
			}

			mpd_info.vol = status->volume;
			if (status->random == 0) {
				mpd_info.random = "Off";
			} else if (status->random == 1) {
				mpd_info.random = "On";
			} else {
				mpd_info.random = "";
			}
			if (status->repeat == 0) {
				mpd_info.repeat = "Off";
			} else if (status->repeat == 1) {
				mpd_info.repeat = "On";
			} else {
				mpd_info.repeat = "";
			}
			/* if (status->error) {
			   printf("error: %s\n", status->error);
			   } */

			switch (status->state) {
				case MPD_STATUS_STATE_PLAY:
					mpd_info.status = "Playing";
					break;
				case MPD_STATUS_STATE_STOP:
					mpd_info.status = "Stopped";
					break;
				case MPD_STATUS_STATE_PAUSE:
					mpd_info.status = "Paused";
					break;
				default:
					mpd_info.status = "";
					break;
			}

			if (status->state == MPD_STATUS_STATE_PLAY ||
					status->state == MPD_STATUS_STATE_PAUSE) {
				mpd_info.is_playing = 1;
				mpd_info.bitrate = status->bitRate;
				mpd_info.progress = (float) status->elapsedTime /
					status->totalTime;
				mpd_info.elapsed = status->elapsedTime;
				mpd_info.length = status->totalTime;
			} else {
				mpd_info.progress = 0;
				mpd_info.is_playing = 0;
				mpd_info.elapsed = 0;
			}

			if (conn->error) {
				// fprintf(stderr, "%s\n", conn->errorStr);
				mpd_closeConnection(conn);
				conn = 0;
				break;
			}

			mpd_sendCurrentSongCommand(conn);
			while ((entity = mpd_getNextInfoEntity(conn))) {
				mpd_Song *song = entity->info.song;

				if (entity->type != MPD_INFO_ENTITY_TYPE_SONG) {
					mpd_freeInfoEntity(entity);
					continue;
				}
#define SETSTRING(a,b) \
if (b) a=b; else a="";
				SETSTRING(mpd_info.artist, song->artist);
				SETSTRING(mpd_info.album, song->album);
				SETSTRING(mpd_info.title, song->title);
				SETSTRING(mpd_info.date, song->date);
				SETSTRING(mpd_info.track, song->track);
				SETSTRING(mpd_info.name, song->name);
				SETSTRING(mpd_info.file, song->file);
				if (entity != NULL) {
					mpd_freeInfoEntity(entity);
					entity = NULL;
				}
			}
			mpd_finishCommand(conn);
			if (conn->error) {
				// fprintf(stderr, "%s\n", conn->errorStr);
				mpd_closeConnection(conn);
				conn = 0;
				break;
			}


			if (conn->error) {
				// fprintf(stderr, "%s\n", conn->errorStr);
				mpd_closeConnection(conn);
				conn = 0;
				break;
			}

			mpd_freeStatus(status);
			/* if (conn) {
			   mpd_closeConnection(conn);
			   conn = 0;
			   } */
		} while (0);
		result = mpd_info; // don't forget to save results!
	}

	mpd_result get_mpd()
	{
		uint32_t period = std::max(
					lround(music_player_interval.get(*state)/active_update_interval()), 1l
				);
		return conky::register_cb<mpd_cb>(period)->get_result_copy();
	}
}

static inline void format_media_player_time(char *buf, const int size,
		int seconds)
{
	int days, hours, minutes;

	if (times_in_seconds.get(*state)) {
		snprintf(buf, size, "%d", seconds);
		return;
	}

	days = seconds / (24 * 60 * 60);
	seconds %= (24 * 60 * 60);
	hours = seconds / (60 * 60);
	seconds %= (60 * 60);
	minutes = seconds / 60;
	seconds %= 60;

	if (days > 0) {
		snprintf(buf, size, "%i days %i:%02i:%02i", days,
				hours, minutes, seconds);
	} else if (hours > 0) {
		snprintf(buf, size, "%i:%02i:%02i", hours, minutes,
				seconds);
	} else {
		snprintf(buf, size, "%i:%02i", minutes, seconds);
	}
}

void print_mpd_elapsed(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	format_media_player_time(p, p_max_size, get_mpd().elapsed);
}

void print_mpd_length(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	format_media_player_time(p, p_max_size, get_mpd().length);
}

uint8_t mpd_percentage(struct text_object *obj)
{
	(void)obj;
	return round_to_int(get_mpd().progress * 100.0f);
}

double mpd_barval(struct text_object *obj)
{
	(void)obj;
	return get_mpd().progress;
}

void print_mpd_smart(struct text_object *obj, char *p, int p_max_size)
{
	const mpd_result &mpd_info = get_mpd();
	int len = obj->data.i;
	if (len == 0 || len > p_max_size)
		len = p_max_size;

	memset(p, 0, p_max_size);
	if (mpd_info.artist.size() && mpd_info.title.size()) {
		snprintf(p, len, "%s - %s", mpd_info.artist.c_str(), mpd_info.title.c_str());
	} else if (get_mpd().title.size()) {
		snprintf(p, len, "%s", mpd_info.title.c_str());
	} else if (mpd_info.artist.size()) {
		snprintf(p, len, "%s", mpd_info.artist.c_str());
	} else if (mpd_info.file.size()) {
		snprintf(p, len, "%s", mpd_info.file.c_str());
	} else {
		*p = 0;
	}
}

int check_mpd_playing(struct text_object *obj)
{
	(void)obj;
	return get_mpd().is_playing;
}

#define MPD_PRINT_GENERATOR(name, fmt, acc) \
void print_mpd_##name(struct text_object *obj, char *p, int p_max_size) \
{ \
	if (obj->data.i && obj->data.i < p_max_size) \
		p_max_size = obj->data.i; \
	snprintf(p, p_max_size, fmt, get_mpd().name acc); \
}

MPD_PRINT_GENERATOR(title, "%s", .c_str())
MPD_PRINT_GENERATOR(artist, "%s", .c_str())
MPD_PRINT_GENERATOR(albumartist, "%s", .c_str())
MPD_PRINT_GENERATOR(album, "%s", .c_str())
MPD_PRINT_GENERATOR(date, "%s", .c_str())
MPD_PRINT_GENERATOR(random, "%s", .c_str())
MPD_PRINT_GENERATOR(repeat, "%s", .c_str())
MPD_PRINT_GENERATOR(track, "%s", .c_str())
MPD_PRINT_GENERATOR(name, "%s", .c_str())
MPD_PRINT_GENERATOR(file, "%s", .c_str())
MPD_PRINT_GENERATOR(vol, "%d", )
MPD_PRINT_GENERATOR(bitrate, "%d", )
MPD_PRINT_GENERATOR(status, "%s", .c_str())

#undef MPD_PRINT_GENERATOR
