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
#ifndef _TEXT_OBJECT_H
#define _TEXT_OBJECT_H

#include "config.h"		/* for the defines */
#include "specials.h"		/* enum special_types */

enum text_object_type {
	OBJ_read_tcp,
	OBJ_addr,
#if defined(__linux__)
	OBJ_addrs,
#endif /* __linux__ */
#ifndef __OpenBSD__
	OBJ_acpiacadapter,
	OBJ_acpifan,
	OBJ_acpitemp,
	OBJ_battery,
	OBJ_battery_time,
	OBJ_battery_percent,
	OBJ_battery_bar,
	OBJ_battery_short,
#endif /* !__OpenBSD__ */
	OBJ_buffers,
	OBJ_cached,
	OBJ_color,
	OBJ_color0,
	OBJ_color1,
	OBJ_color2,
	OBJ_color3,
	OBJ_color4,
	OBJ_color5,
	OBJ_color6,
	OBJ_color7,
	OBJ_color8,
	OBJ_color9,
	OBJ_conky_version,
	OBJ_conky_build_date,
	OBJ_conky_build_arch,
	OBJ_font,
	OBJ_cpu,
	OBJ_cpubar,
	OBJ_cpugauge,
#ifdef X11
	OBJ_cpugraph,
	OBJ_loadgraph,
#endif /* X11 */
	OBJ_diskio,
	OBJ_diskio_read,
	OBJ_diskio_write,
#ifdef X11
	OBJ_diskiograph,
	OBJ_diskiograph_read,
	OBJ_diskiograph_write,
#endif /* X11 */
	OBJ_downspeed,
	OBJ_downspeedf,
#ifdef X11
	OBJ_downspeedgraph,
#endif /* X11 */
	OBJ_else,
	OBJ_endif,
	OBJ_eval,
	OBJ_image,
	OBJ_exec,
	OBJ_execi,
	OBJ_texeci,
	OBJ_execbar,
	OBJ_execibar,
	OBJ_execgauge,
	OBJ_execigauge,
#ifdef X11
	OBJ_execgraph,
	OBJ_execigraph,
#endif /* X11 */
	OBJ_execp,
	OBJ_execpi,
	OBJ_freq,
	OBJ_freq_g,
	OBJ_fs_bar,
	OBJ_fs_bar_free,
	OBJ_fs_free,
	OBJ_fs_free_perc,
	OBJ_fs_size,
	OBJ_fs_type,
	OBJ_fs_used,
	OBJ_fs_used_perc,
	OBJ_goto,
	OBJ_tab,
	OBJ_hr,
	OBJ_offset,
	OBJ_voffset,
	OBJ_alignr,
	OBJ_alignc,
	OBJ_i2c,
	OBJ_platform,
	OBJ_hwmon,
#if defined(__linux__)
	OBJ_disk_protect,
	OBJ_i8k_version,
	OBJ_i8k_bios,
	OBJ_i8k_serial,
	OBJ_i8k_cpu_temp,
	OBJ_i8k_left_fan_status,
	OBJ_i8k_right_fan_status,
	OBJ_i8k_left_fan_rpm,
	OBJ_i8k_right_fan_rpm,
	OBJ_i8k_ac_status,
	OBJ_i8k_buttons_status,
#if defined(IBM)
	OBJ_ibm_fan,
	OBJ_ibm_temps,
	OBJ_ibm_volume,
	OBJ_ibm_brightness,
	OBJ_smapi,
	OBJ_smapi_bat_bar,
	OBJ_smapi_bat_perc,
	OBJ_smapi_bat_temp,
	OBJ_smapi_bat_power,
	OBJ_if_smapi_bat_installed,
#endif /* IBM */
	/* information from sony_laptop kernel module
	 * /sys/devices/platform/sony-laptop */
	OBJ_sony_fanspeed,
	OBJ_if_gw,
	OBJ_ioscheduler,
	OBJ_gw_iface,
	OBJ_gw_ip,
	OBJ_laptop_mode,
	OBJ_pb_battery,
	OBJ_voltage_mv,
	OBJ_voltage_v,
	OBJ_wireless_essid,
	OBJ_wireless_mode,
	OBJ_wireless_bitrate,
	OBJ_wireless_ap,
	OBJ_wireless_link_qual,
	OBJ_wireless_link_qual_max,
	OBJ_wireless_link_qual_perc,
	OBJ_wireless_link_bar,
#endif /* __linux__ */
#if defined(__FreeBSD__) || defined(__linux__)
	OBJ_if_up,
#endif
	OBJ_if_empty,
	OBJ_if_match,
	OBJ_if_existing,
	OBJ_if_mounted,
	OBJ_if_running,
	OBJ_if_updatenr,
	OBJ_top,
	OBJ_top_mem,
	OBJ_top_time,
#ifdef IOSTATS
	OBJ_top_io,
#endif
	OBJ_tail,
	OBJ_head,
	OBJ_lines,
	OBJ_words,
	OBJ_kernel,
	OBJ_loadavg,
	OBJ_machine,
	OBJ_mails,
	OBJ_new_mails,
	OBJ_seen_mails,
	OBJ_unseen_mails,
	OBJ_flagged_mails,
	OBJ_unflagged_mails,
	OBJ_format_time,
	OBJ_forwarded_mails,
	OBJ_unforwarded_mails,
	OBJ_replied_mails,
	OBJ_unreplied_mails,
	OBJ_draft_mails,
	OBJ_trashed_mails,
	OBJ_mboxscan,
	OBJ_mem,
	OBJ_memeasyfree,
	OBJ_memfree,
	OBJ_memgauge,
#ifdef X11
	OBJ_memgraph,
#endif /* X11 */
	OBJ_membar,
	OBJ_memmax,
	OBJ_memperc,
	OBJ_mixer,
	OBJ_mixerl,
	OBJ_mixerr,
	OBJ_mixerbar,
	OBJ_mixerlbar,
	OBJ_mixerrbar,
	OBJ_if_mixer_mute,
#ifdef X11
	OBJ_monitor,
	OBJ_monitor_number,
	OBJ_desktop,
	OBJ_desktop_number,
	OBJ_desktop_name,
#endif /* X11 */
	OBJ_nameserver,
	OBJ_nodename,
	OBJ_nodename_short,
	OBJ_nvidia,
	OBJ_pre_exec,
	OBJ_cmdline_to_pid,
	OBJ_pid_chroot,
	OBJ_pid_cmdline,
	OBJ_pid_cwd,
	OBJ_pid_environ,
	OBJ_pid_environ_list,
	OBJ_pid_exe,
	OBJ_pid_nice,
	OBJ_pid_openfiles,
	OBJ_pid_parent,
	OBJ_pid_priority,
	OBJ_pid_state,
	OBJ_pid_state_short,
	OBJ_pid_stderr,
	OBJ_pid_stdin,
	OBJ_pid_stdout,
	OBJ_pid_threads,
	OBJ_pid_thread_list,
	OBJ_pid_time_kernelmode,
	OBJ_pid_time_usermode,
	OBJ_pid_time,
	OBJ_pid_uid,
	OBJ_pid_euid,
	OBJ_pid_suid,
	OBJ_pid_fsuid,
	OBJ_pid_gid,
	OBJ_pid_egid,
	OBJ_pid_sgid,
	OBJ_pid_fsgid,
	OBJ_pid_read,
	OBJ_pid_vmpeak,
	OBJ_pid_vmsize,
	OBJ_pid_vmlck,
	OBJ_pid_vmhwm,
	OBJ_pid_vmrss,
	OBJ_pid_vmdata,
	OBJ_pid_vmstk,
	OBJ_pid_vmexe,
	OBJ_pid_vmlib,
	OBJ_pid_vmpte,
	OBJ_pid_write,
	OBJ_gid_name,
	OBJ_uid_name,
	OBJ_processes,
	OBJ_running_processes,
	OBJ_shadecolor,
	OBJ_outlinecolor,
	OBJ_stippled_hr,
	OBJ_swap,
	OBJ_swapfree,
	OBJ_swapbar,
	OBJ_swapmax,
	OBJ_swapperc,
	OBJ_sysname,
	OBJ_text,
	OBJ_threads,
	OBJ_running_threads,
	OBJ_time,
	OBJ_utime,
	OBJ_tztime,
	OBJ_totaldown,
	OBJ_totalup,
	OBJ_updates,
	OBJ_upspeed,
	OBJ_upspeedf,
#ifdef X11
	OBJ_upspeedgraph,
#endif /* X11 */
	OBJ_uptime,
	OBJ_uptime_short,
	OBJ_user_names,
	OBJ_user_terms,
	OBJ_user_times,
	OBJ_user_time,
	OBJ_user_number,
	OBJ_imap_messages,
	OBJ_imap_unseen,
	OBJ_pop3_unseen,
	OBJ_pop3_used,
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
		|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
	OBJ_apm_adapter,
	OBJ_apm_battery_time,
	OBJ_apm_battery_life,
#endif /* __FreeBSD__ __OpenBSD__ */
#ifdef __OpenBSD__
	OBJ_obsd_sensors_temp,
	OBJ_obsd_sensors_fan,
	OBJ_obsd_sensors_volt,
	OBJ_obsd_vendor,
	OBJ_obsd_product,
#endif /* __OpenBSD__ */
#ifdef MPD
	OBJ_mpd_title,
	OBJ_mpd_artist,
	OBJ_mpd_albumartist,
	OBJ_mpd_album,
	OBJ_mpd_date,
	OBJ_mpd_random,
	OBJ_mpd_repeat,
	OBJ_mpd_vol,
	OBJ_mpd_bitrate,
	OBJ_mpd_status,
	OBJ_mpd_bar,
	OBJ_mpd_elapsed,
	OBJ_mpd_length,
	OBJ_mpd_track,
	OBJ_mpd_name,
	OBJ_mpd_file,
	OBJ_mpd_percent,
	OBJ_mpd_smart,
	OBJ_if_mpd_playing,
#endif /* MPD */
#ifdef MOC
	OBJ_moc_state,
	OBJ_moc_file,
	OBJ_moc_title,
	OBJ_moc_artist,
	OBJ_moc_song,
	OBJ_moc_album,
	OBJ_moc_totaltime,
	OBJ_moc_timeleft,
	OBJ_moc_curtime,
	OBJ_moc_bitrate,
	OBJ_moc_rate,
#endif /* MOC */
#ifdef XMMS2
	OBJ_xmms2_artist,
	OBJ_xmms2_album,
	OBJ_xmms2_title,
	OBJ_xmms2_genre,
	OBJ_xmms2_comment,
	OBJ_xmms2_url,
	OBJ_xmms2_date,
	OBJ_xmms2_tracknr,
	OBJ_xmms2_bitrate,
	OBJ_xmms2_id,
	OBJ_xmms2_duration,
	OBJ_xmms2_elapsed,
	OBJ_xmms2_size,
	OBJ_xmms2_percent,
	OBJ_xmms2_status,
#ifdef X11
	OBJ_xmms2_bar,
#endif /* X11 */
	OBJ_xmms2_smart,
	OBJ_xmms2_playlist,
	OBJ_xmms2_timesplayed,
	OBJ_if_xmms2_connected,
#endif /* XMMS2 */
#ifdef AUDACIOUS
	OBJ_audacious_status,
	OBJ_audacious_title,
	OBJ_audacious_length,
	OBJ_audacious_length_seconds,
	OBJ_audacious_position,
	OBJ_audacious_position_seconds,
	OBJ_audacious_bitrate,
	OBJ_audacious_frequency,
	OBJ_audacious_channels,
	OBJ_audacious_filename,
	OBJ_audacious_playlist_length,
	OBJ_audacious_playlist_position,
	OBJ_audacious_main_volume,
#ifdef X11
	OBJ_audacious_bar,
#endif /* X11 */
#endif /* AUDACIOUS */
#ifdef BMPX
	OBJ_bmpx_title,
	OBJ_bmpx_artist,
	OBJ_bmpx_album,
	OBJ_bmpx_track,
	OBJ_bmpx_uri,
	OBJ_bmpx_bitrate,
#endif /* BMPX */
#ifdef EVE
	OBJ_eve,
#endif /* EVE */
#ifdef HAVE_CURL
	OBJ_curl,
#endif /* HAVE_CURL */
#ifdef RSS
	OBJ_rss,
#endif /* RSS */
#ifdef WEATHER
	OBJ_weather,
#endif /* WEATHER */
#ifdef XOAP
	OBJ_weather_forecast,
#endif /* XOAP */
#ifdef HAVE_LUA
	OBJ_lua,
	OBJ_lua_parse,
	OBJ_lua_bar,
	OBJ_lua_gauge,
#ifdef X11
	OBJ_lua_graph,
#endif /* X11 */
#endif /* HAVE_LUA */
#ifdef TCP_PORT_MONITOR
	OBJ_tcp_portmon,
#endif /* TCP_PORT_MONITOR */
#ifdef HAVE_ICONV
	OBJ_iconv_start,
	OBJ_iconv_stop,
#endif /* HAVE_ICONV */
#ifdef HDDTEMP
	OBJ_hddtemp,
#endif /* HDDTEMP */
	OBJ_include,
	OBJ_blink,
	OBJ_to_bytes,
	OBJ_scroll,
	OBJ_combine,
	OBJ_entropy_avail,
	OBJ_entropy_perc,
	OBJ_entropy_poolsize,
	OBJ_entropy_bar,
#ifdef APCUPSD
	OBJ_apcupsd,
	OBJ_apcupsd_name,
	OBJ_apcupsd_model,
	OBJ_apcupsd_upsmode,
	OBJ_apcupsd_cable,
	OBJ_apcupsd_status,
	OBJ_apcupsd_linev,
	OBJ_apcupsd_load,
	OBJ_apcupsd_loadbar,
	OBJ_apcupsd_loadgauge,
#ifdef X11
	OBJ_apcupsd_loadgraph,
#endif /* X11 */
	OBJ_apcupsd_charge,
	OBJ_apcupsd_timeleft,
	OBJ_apcupsd_temp,
	OBJ_apcupsd_lastxfer,
#endif /* APCUPSD */
#ifdef JACK
	OBJ_if_jack_active,
	OBJ_jack_cpu_load,
	OBJ_jack_buffer_size,
	OBJ_jack_sample_rate,
	OBJ_jack_xruns,
	/* JACK Transport */
	OBJ_if_jack_rolling,
	OBJ_jack_frame,
	OBJ_jack_hour,
	OBJ_jack_min,
	OBJ_jack_sec,
	OBJ_if_jack_bbt,
	OBJ_jack_beat_type,
	OBJ_jack_beats_per_bar,
	OBJ_jack_bpm,
	OBJ_jack_bar,
	OBJ_jack_beat,
	OBJ_jack_tick,
#endif /* JACK */
};

struct text_object {
	struct text_object *next, *prev;	/* doubly linked list of text objects */
	struct text_object *sub;		/* for objects parsing text into objects */
	struct text_object *ifblock_next;	/* jump target for ifblock objects */
	union {
		void *opaque;		/* new style generic per object data */
		char *s;		/* some string */
		int i;			/* some integer */
		long l;			/* some long integer */
	} data;

	void *special_data;
	int type;
	long line;
};

/* text object list helpers */
int append_object(struct text_object *root, struct text_object *obj);

/* ifblock helpers
 *
 * Opaque is a pointer to the address of the ifblock stack's top object.
 * Calling clients should pass the address of a defined void pointer which
 * was initialised to NULL (empty stack).
 * */
int obj_be_ifblock_if(void **opaque, struct text_object *);
int obj_be_ifblock_else(void **opaque, struct text_object *);
int obj_be_ifblock_endif(void **opaque, struct text_object *);
int ifblock_stack_empty(void **opaque);

#endif /* _TEXT_OBJECT_H */
