/*
 *
 * audacious.c:  conky support for audacious music player
 *
 * Copyright (C) 2005-2007 Philip Kovacs pkovacs@users.sourceforge.net
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 */

#include <config.h>

#include <cmath>

#include <mutex>
#include "audacious.h"
#include "conky.h"
#include "logging.h"
#include "update-cb.hh"

#include <glib.h>
#ifdef NEW_AUDACIOUS_FOUND
#include <audacious/audctrl.h>
#include <audacious/dbus.h>
#include <glib-object.h>
#else /* NEW_AUDACIOUS_FOUND */
#include <audacious/beepctrl.h>
#define audacious_remote_is_running(x) xmms_remote_is_running(x)
#define audacious_remote_is_paused(x) xmms_remote_is_paused(x)
#define audacious_remote_is_playing(x) xmms_remote_is_playing(x)
#define audacious_remote_get_playlist_pos(x) xmms_remote_get_playlist_pos(x)
#define audacious_remote_get_playlist_title(x, y) \
  xmms_remote_get_playlist_title(x, y)
#define audacious_remote_get_playlist_time(x, y) \
  xmms_remote_get_playlist_time(x, y)
#define audacious_remote_get_output_time(x) xmms_remote_get_output_time(x)
#define audacious_remote_get_info(w, x, y, z) xmms_remote_get_info(w, x, y, z)
#define audacious_remote_get_playlist_file(x, y) \
  xmms_remote_get_playlist_file(x, y)
#define audacious_remote_get_playlist_length(x) \
  xmms_remote_get_playlist_length(x)
#endif /* NEW_AUDACIOUS_FOUND */

namespace {

enum aud_status { AS_NOT_RUNNING, AS_PAUSED, AS_PLAYING, AS_STOPPED };
const char *const as_message[] = {"Not running", "Paused", "Playing",
                                  "Stopped"};

struct aud_result {
  std::string title;
  std::string filename;
  int length;    // in ms
  int position;  // in ms
  int bitrate;
  int frequency;
  int channels;
  int playlist_length;
  int playlist_position;
  int main_volume;
  aud_status status;

  aud_result()
      : length(0),
        position(0),
        bitrate(0),
        frequency(0),
        channels(0),
        playlist_length(0),
        playlist_position(0),
        main_volume(0),
        status(AS_NOT_RUNNING) {}
};

class audacious_cb : public conky::callback<aud_result> {
  typedef conky::callback<aud_result> Base;

#ifdef NEW_AUDACIOUS_FOUND
  DBusGProxy *session;
#else
  gint session;
#endif

 protected:
  virtual void work();

 public:
  audacious_cb(uint32_t period) : Base(period, false, Tuple()) {
#ifdef NEW_AUDACIOUS_FOUND
    DBusGConnection *connection = dbus_g_bus_get(DBUS_BUS_SESSION, nullptr);
    if (!connection)
      throw std::runtime_error("unable to establish dbus connection");

    session = dbus_g_proxy_new_for_name(connection, AUDACIOUS_DBUS_SERVICE,
                                        AUDACIOUS_DBUS_PATH,
                                        AUDACIOUS_DBUS_INTERFACE);
    if (!session) throw std::runtime_error("unable to create dbus proxy");
#else
    session = 0;
#endif /* NEW_AUDACIOUS_FOUND */
  }

#ifdef NEW_AUDACIOUS_FOUND
  ~audacious_cb() {
    /* release reference to dbus proxy */
    g_object_unref(session);
  }
#endif
};

/* ---------------------------------------------------
 * Worker thread function for audacious data sampling.
 * --------------------------------------------------- */
void audacious_cb::work() {
  aud_result tmp;
  gchar *psong, *pfilename;
  psong = nullptr;
  pfilename = nullptr;

  do {
    if (!audacious_remote_is_running(session)) {
      tmp.status = AS_NOT_RUNNING;
      break;
    }

    /* Player status */
    if (audacious_remote_is_paused(session)) {
      tmp.status = AS_PAUSED;
    } else if (audacious_remote_is_playing(session)) {
      tmp.status = AS_PLAYING;
    } else {
      tmp.status = AS_STOPPED;
    }

    /* Current song title */
    tmp.playlist_position = audacious_remote_get_playlist_pos(session);
    psong = audacious_remote_get_playlist_title(session, tmp.playlist_position);
    if (psong) {
      tmp.title = psong;
      g_free(psong);
    }

    /* Current song length */
    tmp.length =
        audacious_remote_get_playlist_time(session, tmp.playlist_position);

    /* Current song position */
    tmp.position = audacious_remote_get_output_time(session);

    /* Current song bitrate, frequency, channels */
    audacious_remote_get_info(session, &tmp.bitrate, &tmp.frequency,
                              &tmp.channels);

    /* Current song filename */
    pfilename =
        audacious_remote_get_playlist_file(session, tmp.playlist_position);
    if (pfilename) {
      tmp.filename = pfilename;
      g_free(pfilename);
    }

    /* Length of the Playlist (number of songs) */
    tmp.playlist_length = audacious_remote_get_playlist_length(session);

    /* Main volume */
    tmp.main_volume = audacious_remote_get_main_volume(session);
  } while (0);
  {
    /* Deliver the refreshed items array to audacious_items. */
    std::lock_guard<std::mutex> lock(result_mutex);
    result = tmp;
  }
}

aud_result get_res() {
  uint32_t period = std::max(
      lround(music_player_interval.get(*state) / active_update_interval()), 1l);
  return conky::register_cb<audacious_cb>(period)->get_result_copy();
}
}  // namespace

void print_audacious_status(struct text_object *, char *p,
                            unsigned int p_max_size) {
  const aud_result &res = get_res();
  snprintf(p, p_max_size, "%s", as_message[res.status]);
}

void print_audacious_title(struct text_object *obj, char *p,
                           unsigned int p_max_size) {
  snprintf(p, std::min((unsigned int)obj->data.i, p_max_size), "%s",
           get_res().title.c_str());
}

void print_audacious_filename(struct text_object *obj, char *p,
                              unsigned int p_max_size) {
  snprintf(p, std::min((unsigned int)obj->data.i, p_max_size), "%s",
           get_res().filename.c_str());
}

double audacious_barval(struct text_object *) {
  const aud_result &res = get_res();
  return (double)res.position / res.length;
}

void print_audacious_length(struct text_object *, char *p,
                            unsigned int p_max_size) {
  const aud_result &res = get_res();
  int sec = res.length / 1000;
  snprintf(p, p_max_size, "%d:%.2d", sec / 60, sec % 60);
}

void print_audacious_length_seconds(struct text_object *, char *p,
                                    unsigned int p_max_size) {
  snprintf(p, p_max_size, "%d", get_res().length);
}

void print_audacious_position(struct text_object *, char *p,
                              unsigned int p_max_size) {
  const aud_result &res = get_res();
  int sec = res.position / 1000;
  snprintf(p, p_max_size, "%d:%.2d", sec / 60, sec % 60);
}

void print_audacious_position_seconds(struct text_object *, char *p,
                                      unsigned int p_max_size) {
  snprintf(p, p_max_size, "%d", get_res().position);
}

void print_audacious_bitrate(struct text_object *, char *p,
                             unsigned int p_max_size) {
  snprintf(p, p_max_size, "%d", get_res().bitrate);
}

void print_audacious_frequency(struct text_object *, char *p,
                               unsigned int p_max_size) {
  snprintf(p, p_max_size, "%d", get_res().frequency);
}

void print_audacious_channels(struct text_object *, char *p,
                              unsigned int p_max_size) {
  snprintf(p, p_max_size, "%d", get_res().channels);
}

void print_audacious_playlist_length(struct text_object *, char *p,
                                     unsigned int p_max_size) {
  snprintf(p, p_max_size, "%d", get_res().playlist_length);
}

void print_audacious_playlist_position(struct text_object *, char *p,
                                       unsigned int p_max_size) {
  snprintf(p, p_max_size, "%d", get_res().playlist_position + 1);
}

void print_audacious_main_volume(struct text_object *, char *p,
                                 unsigned int p_max_size) {
  snprintf(p, p_max_size, "%d", get_res().main_volume);
}
