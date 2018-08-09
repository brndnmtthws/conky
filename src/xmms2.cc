/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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

#include "conky.h"

xmmsc_connection_t *xmms2_conn;

#define CONN_INIT 0
#define CONN_OK 1
#define CONN_NO 2

static void xmms_alloc(struct information *ptr) {
  if (ptr->xmms2.artist == nullptr) {
    ptr->xmms2.artist = (char *)malloc(text_buffer_size.get(*state));
  }

  if (ptr->xmms2.album == nullptr) {
    ptr->xmms2.album = (char *)malloc(text_buffer_size.get(*state));
  }

  if (ptr->xmms2.title == nullptr) {
    ptr->xmms2.title = (char *)malloc(text_buffer_size.get(*state));
  }

  if (ptr->xmms2.genre == nullptr) {
    ptr->xmms2.genre = (char *)malloc(text_buffer_size.get(*state));
  }

  if (ptr->xmms2.comment == nullptr) {
    ptr->xmms2.comment = (char *)malloc(text_buffer_size.get(*state));
  }

  if (ptr->xmms2.url == nullptr) {
    ptr->xmms2.url = (char *)malloc(text_buffer_size.get(*state));
  }

  if (ptr->xmms2.date == nullptr) {
    ptr->xmms2.date = (char *)malloc(text_buffer_size.get(*state));
  }

  ptr->xmms2.artist[0] = '\0';
  ptr->xmms2.album[0] = '\0';
  ptr->xmms2.title[0] = '\0';
  ptr->xmms2.genre[0] = '\0';
  ptr->xmms2.comment[0] = '\0';
  ptr->xmms2.url[0] = '\0';
  ptr->xmms2.date[0] = '\0';

  ptr->xmms2.tracknr = 0;
  ptr->xmms2.id = 0;
  ptr->xmms2.bitrate = 0;
  ptr->xmms2.duration = 0;
  ptr->xmms2.elapsed = 0;
  ptr->xmms2.size = 0;
  ptr->xmms2.progress = 0;
  ptr->xmms2.timesplayed = -1;
}

void free_xmms2(struct text_object *obj) {
  (void)obj;

  free_and_zero(info.xmms2.artist);
  free_and_zero(info.xmms2.album);
  free_and_zero(info.xmms2.title);
  free_and_zero(info.xmms2.genre);
  free_and_zero(info.xmms2.comment);
  free_and_zero(info.xmms2.url);
  free_and_zero(info.xmms2.date);
  free_and_zero(info.xmms2.status);
  free_and_zero(info.xmms2.playlist);
}

void connection_lost(void *p) {
  struct information *ptr = (struct information *)p;
  ptr->xmms2.conn_state = CONN_NO;

  fprintf(stderr, "XMMS2 connection failed. %s\n",
          xmmsc_get_last_error(xmms2_conn));

  xmms_alloc(ptr);
  strncpy(ptr->xmms2.status, "Disconnected", text_buffer_size.get(*state) - 1);
  ptr->xmms2.playlist[0] = '\0';
  ptr->xmms2.id = 0;
}

int handle_curent_id(xmmsv_t *value, void *p) {
  struct information *ptr = (struct information *)p;
  xmmsv_t *val, *infos, *dict_entry;
  xmmsc_result_t *res;
  const char *errbuf;
  int current_id;

  const char *charval;
  int intval;

  if (xmmsv_get_error(value, &errbuf)) {
    fprintf(stderr, "XMMS2 server error. %s\n", errbuf);
    return TRUE;
  }

  if (xmmsv_get_int(value, &current_id) && current_id > 0) {
    res = xmmsc_medialib_get_info(xmms2_conn, current_id);
    xmmsc_result_wait(res);
    val = xmmsc_result_get_value(res);

    if (xmmsv_get_error(val, &errbuf)) {
      fprintf(stderr, "XMMS2 server error. %s\n", errbuf);
      return TRUE;
    }

    xmms_alloc(ptr);

    ptr->xmms2.id = current_id;

    infos = xmmsv_propdict_to_dict(val, nullptr);

    if (xmmsv_dict_get(infos, "artist", &dict_entry) &&
        xmmsv_get_string(dict_entry, &charval))
      strncpy(ptr->xmms2.artist, charval, text_buffer_size.get(*state) - 1);

    if (xmmsv_dict_get(infos, "title", &dict_entry) &&
        xmmsv_get_string(dict_entry, &charval))
      strncpy(ptr->xmms2.title, charval, text_buffer_size.get(*state) - 1);

    if (xmmsv_dict_get(infos, "album", &dict_entry) &&
        xmmsv_get_string(dict_entry, &charval))
      strncpy(ptr->xmms2.album, charval, text_buffer_size.get(*state) - 1);

    if (xmmsv_dict_get(infos, "genre", &dict_entry) &&
        xmmsv_get_string(dict_entry, &charval))
      strncpy(ptr->xmms2.genre, charval, text_buffer_size.get(*state) - 1);

    if (xmmsv_dict_get(infos, "comment", &dict_entry) &&
        xmmsv_get_string(dict_entry, &charval))
      strncpy(ptr->xmms2.comment, charval, text_buffer_size.get(*state) - 1);

    if (xmmsv_dict_get(infos, "url", &dict_entry) &&
        xmmsv_get_string(dict_entry, &charval))
      strncpy(ptr->xmms2.url, charval, text_buffer_size.get(*state) - 1);

    if (xmmsv_dict_get(infos, "date", &dict_entry) &&
        xmmsv_get_string(dict_entry, &charval))
      strncpy(ptr->xmms2.date, charval, text_buffer_size.get(*state) - 1);

    if (xmmsv_dict_get(infos, "tracknr", &dict_entry) &&
        xmmsv_get_int(dict_entry, &intval))
      ptr->xmms2.tracknr = intval;

    if (xmmsv_dict_get(infos, "duration", &dict_entry) &&
        xmmsv_get_int(dict_entry, &intval))
      ptr->xmms2.duration = intval;

    if (xmmsv_dict_get(infos, "bitrate", &dict_entry) &&
        xmmsv_get_int(dict_entry, &intval))
      ptr->xmms2.bitrate = intval / 1000;

    if (xmmsv_dict_get(infos, "size", &dict_entry) &&
        xmmsv_get_int(dict_entry, &intval))
      ptr->xmms2.size = (float)intval / 1048576;

    if (xmmsv_dict_get(infos, "timesplayed", &dict_entry) &&
        xmmsv_get_int(dict_entry, &intval))
      ptr->xmms2.timesplayed = intval;

    xmmsv_unref(infos);
    xmmsc_result_unref(res);
  }
  return TRUE;
}

int handle_playtime(xmmsv_t *value, void *p) {
  struct information *ptr = (struct information *)p;
  int play_time;
  const char *errbuf;

  if (xmmsv_get_error(value, &errbuf)) {
    fprintf(stderr, "XMMS2 server error. %s\n", errbuf);
    return TRUE;
  }

  if (xmmsv_get_int(value, &play_time)) {
    ptr->xmms2.elapsed = play_time;
    ptr->xmms2.progress = (float)play_time / ptr->xmms2.duration;
    ptr->xmms2.percent = (int)(ptr->xmms2.progress * 100);
  }

  return TRUE;
}

int handle_playback_state_change(xmmsv_t *value, void *p) {
  struct information *ptr = (struct information *)p;
  int pb_state = 0;
  const char *errbuf;

  if (xmmsv_get_error(value, &errbuf)) {
    fprintf(stderr, "XMMS2 server error. %s\n", errbuf);
    return TRUE;
  }

  if (ptr->xmms2.status == nullptr) {
    ptr->xmms2.status = (char *)malloc(text_buffer_size.get(*state));
    ptr->xmms2.status[0] = '\0';
  }

  if (xmmsv_get_int(value, &pb_state)) {
    switch (pb_state) {
      case XMMS_PLAYBACK_STATUS_PLAY:
        strncpy(ptr->xmms2.status, "Playing", text_buffer_size.get(*state) - 1);
        break;
      case XMMS_PLAYBACK_STATUS_PAUSE:
        strncpy(ptr->xmms2.status, "Paused", text_buffer_size.get(*state) - 1);
        break;
      case XMMS_PLAYBACK_STATUS_STOP:
        strncpy(ptr->xmms2.status, "Stopped", text_buffer_size.get(*state) - 1);
        ptr->xmms2.elapsed = ptr->xmms2.progress = ptr->xmms2.percent = 0;
        break;
      default:
        strncpy(ptr->xmms2.status, "Unknown", text_buffer_size.get(*state) - 1);
    }
  }
  return TRUE;
}

int handle_playlist_loaded(xmmsv_t *value, void *p) {
  struct information *ptr = (struct information *)p;
  const char *c, *errbuf;

  if (xmmsv_get_error(value, &errbuf)) {
    fprintf(stderr, "XMMS2 server error. %s\n", errbuf);
    return TRUE;
  }

  if (ptr->xmms2.playlist == nullptr) {
    ptr->xmms2.playlist = (char *)malloc(text_buffer_size.get(*state));
    ptr->xmms2.playlist[0] = '\0';
  }

  if (xmmsv_get_string(value, &c)) {
    strncpy(ptr->xmms2.playlist, c, text_buffer_size.get(*state) - 1);
  }
  return TRUE;
}

int handle_medialib_changed(xmmsv_t *value, void *p) {
  struct information *ptr = (struct information *)p;
  const char *errbuf;
  int current_id;

  if (xmmsv_get_error(value, &errbuf)) {
    fprintf(stderr, "XMMS2 server error. %s\n", errbuf);
    return TRUE;
  }

  if (xmmsv_get_int(value, &current_id) && current_id > 0 &&
      ptr->xmms2.id == (unsigned int)current_id) {
    return handle_curent_id(value, ptr);
  }

  return TRUE;
}

int update_xmms2(void) {
  struct information *current_info = &info;

  /* initialize connection */
  if (current_info->xmms2.conn_state == CONN_INIT) {
    if (xmms2_conn == nullptr) {
      xmms2_conn = xmmsc_init(PACKAGE_NAME);
    }

    /* did init fail? */
    if (xmms2_conn == nullptr) {
      fprintf(stderr, "XMMS2 init failed. %s\n",
              xmmsc_get_last_error(xmms2_conn));
      return 0;
    }

    /* init ok but not connected yet.. */
    current_info->xmms2.conn_state = CONN_NO;

    /* clear all values */
    xmms_alloc(current_info);
  }

  /* connect */
  if (current_info->xmms2.conn_state == CONN_NO) {
    char *path = getenv("XMMS_PATH");

    if (!xmmsc_connect(xmms2_conn, path)) {
      fprintf(stderr, "XMMS2 connection failed. %s\n",
              xmmsc_get_last_error(xmms2_conn));
      current_info->xmms2.conn_state = CONN_NO;
      return 0;
    }

    /* set callbacks */
    xmmsc_disconnect_callback_set(xmms2_conn, connection_lost, current_info);
    XMMS_CALLBACK_SET(xmms2_conn, xmmsc_broadcast_playback_current_id,
                      handle_curent_id, current_info);
    XMMS_CALLBACK_SET(xmms2_conn, xmmsc_signal_playback_playtime,
                      handle_playtime, current_info);
    XMMS_CALLBACK_SET(xmms2_conn, xmmsc_broadcast_playback_status,
                      handle_playback_state_change, current_info);
    XMMS_CALLBACK_SET(xmms2_conn, xmmsc_broadcast_playlist_loaded,
                      handle_playlist_loaded, current_info);
    XMMS_CALLBACK_SET(xmms2_conn, xmmsc_broadcast_medialib_entry_changed,
                      handle_medialib_changed, current_info);

    /* get playback status, current id and active playlist */
    XMMS_CALLBACK_SET(xmms2_conn, xmmsc_playback_current_id, handle_curent_id,
                      current_info);
    XMMS_CALLBACK_SET(xmms2_conn, xmmsc_playback_status,
                      handle_playback_state_change, current_info);
    XMMS_CALLBACK_SET(xmms2_conn, xmmsc_playlist_current_active,
                      handle_playlist_loaded, current_info);

    /* everything seems to be ok */
    current_info->xmms2.conn_state = CONN_OK;
  }

  /* handle callbacks */
  if (current_info->xmms2.conn_state == CONN_OK) {
    xmmsc_io_in_handle(xmms2_conn);
    if (xmmsc_io_want_out(xmms2_conn)) xmmsc_io_out_handle(xmms2_conn);
  }
  return 0;
}

void print_xmms2_tracknr(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  if (info.xmms2.tracknr != -1) {
    snprintf(p, p_max_size, "%i", info.xmms2.tracknr);
  }
}

void print_xmms2_elapsed(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  snprintf(p, p_max_size, "%02d:%02d", info.xmms2.elapsed / 60000,
           (info.xmms2.elapsed / 1000) % 60);
}

void print_xmms2_duration(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  snprintf(p, p_max_size, "%02d:%02d", info.xmms2.duration / 60000,
           (info.xmms2.duration / 1000) % 60);
}

double xmms2_barval(struct text_object *obj) {
  (void)obj;

  return info.xmms2.progress;
}

void print_xmms2_smart(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  int artist_len = strlen(info.xmms2.artist);
  int title_len = strlen(info.xmms2.title);
  if (artist_len < 2 && title_len < 2) {
    snprintf(p, p_max_size, "%s", info.xmms2.url);
  } else if (artist_len < 1) {
    snprintf(p, p_max_size, "%s", info.xmms2.title);
  } else {
    snprintf(p, p_max_size, "%s - %s", info.xmms2.artist, info.xmms2.title);
  }
}

#define XMMS2_PRINT_GENERATOR(name, fmt)                                      \
  void print_xmms2_##name(struct text_object *obj, char *p,                   \
		                                   unsigned int p_max_size) { \
    (void)obj;                                                                \
    snprintf(p, p_max_size, fmt, info.xmms2.name);                            \
  }

XMMS2_PRINT_GENERATOR(artist, "%s")
XMMS2_PRINT_GENERATOR(album, "%s")
XMMS2_PRINT_GENERATOR(title, "%s")
XMMS2_PRINT_GENERATOR(genre, "%s")
XMMS2_PRINT_GENERATOR(comment, "%s")
XMMS2_PRINT_GENERATOR(url, "%s")
XMMS2_PRINT_GENERATOR(status, "%s")
XMMS2_PRINT_GENERATOR(date, "%s")
XMMS2_PRINT_GENERATOR(bitrate, "%i")
XMMS2_PRINT_GENERATOR(id, "%u")
XMMS2_PRINT_GENERATOR(size, "%2.1f")
XMMS2_PRINT_GENERATOR(playlist, "%s")
XMMS2_PRINT_GENERATOR(timesplayed, "%i")
XMMS2_PRINT_GENERATOR(percent, "%i")

#undef XMMS2_PRINT_GENERATOR

int if_xmms2_connected(struct text_object *obj) {
  (void)obj;

  return info.xmms2.conn_state == CONN_OK;
}
