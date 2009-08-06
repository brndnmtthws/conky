/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
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
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 */

/* local headers */
#include "obj_destroy.h"

/*
 * Frees the list of text objects root points to.  When internal = 1, it won't
 * free global objects.
 */
void free_text_objects(struct text_object *root, int internal)
{
	struct text_object *obj;

	if (!root->prev) {
		return;
	}

#define data obj->data
	for (obj = root->prev; obj; obj = root->prev) {
		root->prev = obj->prev;
		switch (obj->type) {
#ifndef __OpenBSD__
			case OBJ_acpitemp:
				close(data.i);
				break;
#endif /* !__OpenBSD__ */
#ifdef __linux__
			case OBJ_i2c:
			case OBJ_platform:
			case OBJ_hwmon:
				close(data.sysfs.fd);
				break;
#endif /* __linux__ */
			case OBJ_read_tcp:
				free(data.read_tcp.host);
				break;
			case OBJ_time:
			case OBJ_utime:
				free(data.s);
				break;
			case OBJ_tztime:
				free(data.tztime.tz);
				free(data.tztime.fmt);
				break;
			case OBJ_mboxscan:
				free(data.mboxscan.args);
				free(data.mboxscan.output);
				break;
			case OBJ_mails:
			case OBJ_new_mails:
			case OBJ_seen_mails:
			case OBJ_unseen_mails:
			case OBJ_flagged_mails:
			case OBJ_unflagged_mails:
			case OBJ_forwarded_mails:
			case OBJ_unforwarded_mails:
			case OBJ_replied_mails:
			case OBJ_unreplied_mails:
			case OBJ_draft_mails:
			case OBJ_trashed_mails:
				free(data.local_mail.mbox);
				break;
			case OBJ_imap_unseen:
				if (!obj->char_b) {
					free(data.mail);
				}
				break;
			case OBJ_imap_messages:
				if (!obj->char_b) {
					free(data.mail);
				}
				break;
			case OBJ_pop3_unseen:
				if (!obj->char_b) {
					free(data.mail);
				}
				break;
			case OBJ_pop3_used:
				if (!obj->char_b) {
					free(data.mail);
				}
				break;
			case OBJ_if_empty:
			case OBJ_if_match:
				free_text_objects(obj->sub, 1);
				free(obj->sub);
				/* fall through */
			case OBJ_if_existing:
			case OBJ_if_mounted:
			case OBJ_if_running:
				free(data.ifblock.s);
				free(data.ifblock.str);
				break;
			case OBJ_head:
			case OBJ_tail:
				free(data.headtail.logfile);
				if(data.headtail.buffer) {
					free(data.headtail.buffer);
				}
				break;
			case OBJ_text:
			case OBJ_font:
			case OBJ_image:
			case OBJ_eval:
			case OBJ_exec:
			case OBJ_execbar:
#ifdef X11
			case OBJ_execgauge:
			case OBJ_execgraph:
#endif
			case OBJ_execp:
				free(data.s);
				break;
#ifdef HAVE_ICONV
			case OBJ_iconv_start:
				free_iconv();
				break;
#endif
#ifdef __linux__
			case OBJ_disk_protect:
				free(data.s);
				break;
			case OBJ_if_gw:
				free(data.ifblock.s);
				free(data.ifblock.str);
			case OBJ_gw_iface:
			case OBJ_gw_ip:
				if (info.gw_info.iface) {
					free(info.gw_info.iface);
					info.gw_info.iface = 0;
				}
				if (info.gw_info.ip) {
					free(info.gw_info.ip);
					info.gw_info.ip = 0;
				}
				break;
			case OBJ_ioscheduler:
				if(data.s)
					free(data.s);
				break;
#endif
#if (defined(__FreeBSD__) || defined(__linux__))
			case OBJ_if_up:
				free(data.ifblock.s);
				free(data.ifblock.str);
				break;
#endif
#ifdef XMMS2
			case OBJ_xmms2_artist:
				if (info.xmms2.artist) {
					free(info.xmms2.artist);
					info.xmms2.artist = 0;
				}
				break;
			case OBJ_xmms2_album:
				if (info.xmms2.album) {
					free(info.xmms2.album);
					info.xmms2.album = 0;
				}
				break;
			case OBJ_xmms2_title:
				if (info.xmms2.title) {
					free(info.xmms2.title);
					info.xmms2.title = 0;
				}
				break;
			case OBJ_xmms2_genre:
				if (info.xmms2.genre) {
					free(info.xmms2.genre);
					info.xmms2.genre = 0;
				}
				break;
			case OBJ_xmms2_comment:
				if (info.xmms2.comment) {
					free(info.xmms2.comment);
					info.xmms2.comment = 0;
				}
				break;
			case OBJ_xmms2_url:
				if (info.xmms2.url) {
					free(info.xmms2.url);
					info.xmms2.url = 0;
				}
				break;
			case OBJ_xmms2_date:
				if (info.xmms2.date) {
					free(info.xmms2.date);
					info.xmms2.date = 0;
				}
				break;
			case OBJ_xmms2_status:
				if (info.xmms2.status) {
					free(info.xmms2.status);
					info.xmms2.status = 0;
				}
				break;
			case OBJ_xmms2_playlist:
				if (info.xmms2.playlist) {
					free(info.xmms2.playlist);
					info.xmms2.playlist = 0;
				}
				break;
			case OBJ_xmms2_smart:
				if (info.xmms2.artist) {
					free(info.xmms2.artist);
					info.xmms2.artist = 0;
				}
				if (info.xmms2.title) {
					free(info.xmms2.title);
					info.xmms2.title = 0;
				}
				if (info.xmms2.url) {
					free(info.xmms2.url);
					info.xmms2.url = 0;
				}
				break;
#endif
#ifdef BMPX
			case OBJ_bmpx_title:
			case OBJ_bmpx_artist:
			case OBJ_bmpx_album:
			case OBJ_bmpx_track:
			case OBJ_bmpx_uri:
			case OBJ_bmpx_bitrate:
				break;
#endif
#ifdef EVE
			case OBJ_eve:
				break;
#endif
#ifdef HAVE_CURL
			case OBJ_curl:
				free(data.curl.uri);
				break;
#endif
#ifdef RSS
			case OBJ_rss:
				free(data.rss.uri);
				free(data.rss.action);
				break;
#endif
#ifdef WEATHER
			case OBJ_weather:
				free(data.weather.uri);
				free(data.weather.data_type);
				break;
#endif
#ifdef XOAP
			case OBJ_weather_forecast:
				free(data.weather_forecast.uri);
				free(data.weather_forecast.data_type);
				break;
#endif
#ifdef HAVE_LUA
			case OBJ_lua:
			case OBJ_lua_parse:
			case OBJ_lua_bar:
#ifdef X11
			case OBJ_lua_graph:
			case OBJ_lua_gauge:
#endif /* X11 */
				free(data.s);
				break;
#endif /* HAVE_LUA */
			case OBJ_pre_exec:
				break;
#ifndef __OpenBSD__
			case OBJ_battery:
				free(data.s);
				break;
			case OBJ_battery_short:
				free(data.s);
				break;
			case OBJ_battery_time:
				free(data.s);
				break;
#endif /* !__OpenBSD__ */
			case OBJ_execpi:
			case OBJ_execi:
			case OBJ_execibar:
#ifdef X11
			case OBJ_execigraph:
			case OBJ_execigauge:
#endif /* X11 */
				free(data.execi.cmd);
				free(data.execi.buffer);
				break;
			case OBJ_texeci:
				if (data.texeci.p_timed_thread) timed_thread_destroy(data.texeci.p_timed_thread, &data.texeci.p_timed_thread);
				free(data.texeci.cmd);
				free(data.texeci.buffer);
				break;
			case OBJ_nameserver:
				free_dns_data();
				break;
			case OBJ_top:
			case OBJ_top_mem:
			case OBJ_top_time:
#ifdef IOSTATS
			case OBJ_top_io:
#endif
				if (info.first_process && !internal) {
					free_all_processes();
					info.first_process = NULL;
				}
				if (data.top.s) free(data.top.s);
				break;
#ifdef HDDTEMP
			case OBJ_hddtemp:
				free(data.hddtemp.dev);
				free(data.hddtemp.addr);
				if (data.hddtemp.temp)
					free(data.hddtemp.temp);
				break;
#endif /* HDDTEMP */
			case OBJ_entropy_avail:
			case OBJ_entropy_perc:
			case OBJ_entropy_poolsize:
			case OBJ_entropy_bar:
				break;
			case OBJ_user_names:
				if (info.users.names) {
					free(info.users.names);
					info.users.names = 0;
				}
				break;
			case OBJ_user_terms:
				if (info.users.terms) {
					free(info.users.terms);
					info.users.terms = 0;
				}
				break;
			case OBJ_user_times:
				if (info.users.times) {
					free(info.users.times);
					info.users.times = 0;
				}
				break;
#ifdef IBM
			case OBJ_smapi:
			case OBJ_smapi_bat_perc:
			case OBJ_smapi_bat_temp:
			case OBJ_smapi_bat_power:
				free(data.s);
				break;
			case OBJ_if_smapi_bat_installed:
				free(data.ifblock.s);
				free(data.ifblock.str);
				break;
#endif /* IBM */
#ifdef NVIDIA
			case OBJ_nvidia:
				break;
#endif /* NVIDIA */
#ifdef MPD
			case OBJ_mpd_title:
			case OBJ_mpd_artist:
			case OBJ_mpd_album:
			case OBJ_mpd_random:
			case OBJ_mpd_repeat:
			case OBJ_mpd_vol:
			case OBJ_mpd_bitrate:
			case OBJ_mpd_status:
			case OBJ_mpd_bar:
			case OBJ_mpd_elapsed:
			case OBJ_mpd_length:
			case OBJ_mpd_track:
			case OBJ_mpd_name:
			case OBJ_mpd_file:
			case OBJ_mpd_percent:
			case OBJ_mpd_smart:
			case OBJ_if_mpd_playing:
				free_mpd();
				break;
#endif /* MPD */
#ifdef MOC
			case OBJ_moc_state:
			case OBJ_moc_file:
			case OBJ_moc_title:
			case OBJ_moc_artist:
			case OBJ_moc_song:
			case OBJ_moc_album:
			case OBJ_moc_totaltime:
			case OBJ_moc_timeleft:
			case OBJ_moc_curtime:
			case OBJ_moc_bitrate:
			case OBJ_moc_rate:
				free_moc();
				break;
#endif /* MOC */
			case OBJ_include:
			case OBJ_blink:
			case OBJ_to_bytes:
				if(obj->sub) {
					free_text_objects(obj->sub, 1);
					free(obj->sub);
				}
				break;
			case OBJ_scroll:
				free(data.scroll.text);
				free_text_objects(obj->sub, 1);
				free(obj->sub);
				break;
			case OBJ_combine:
				free(data.combine.left);
				free(data.combine.seperation);
				free(data.combine.right);
				free_text_objects(obj->sub, 1);
				free(obj->sub);
				break;
#ifdef APCUPSD
			case OBJ_apcupsd:
			case OBJ_apcupsd_name:
			case OBJ_apcupsd_model:
			case OBJ_apcupsd_upsmode:
			case OBJ_apcupsd_cable:
			case OBJ_apcupsd_status:
			case OBJ_apcupsd_linev:
			case OBJ_apcupsd_load:
			case OBJ_apcupsd_loadbar:
#ifdef X11
			case OBJ_apcupsd_loadgraph:
			case OBJ_apcupsd_loadgauge:
#endif /* X11 */
			case OBJ_apcupsd_charge:
			case OBJ_apcupsd_timeleft:
			case OBJ_apcupsd_temp:
			case OBJ_apcupsd_lastxfer:
				break;
#endif /* APCUPSD */
#ifdef X11
			case OBJ_desktop:
			case OBJ_desktop_number:
			case OBJ_desktop_name:
			        if(info.x11.desktop.name) {
				  free(info.x11.desktop.name);
				  info.x11.desktop.name = NULL;
			        }
			        if(info.x11.desktop.all_names) {
				  free(info.x11.desktop.all_names);
				  info.x11.desktop.all_names = NULL;
			        }
				break;
#endif /* X11 */
		}
		free(obj);
	}
#undef data
}

