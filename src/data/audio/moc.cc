/*
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

#include "../../conky.h"
#include "../../content/specials.h"
#include "../../content/text_object.h"
#include "../../logging.h"
#include "../../parse/variables.hh"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

#include "../../update-cb.hh"

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
  std::string totalsec;
  std::string curtime;
  std::string cursec;
  std::string bitrate;
  std::string avgbitrate;
  std::string rate;
};

enum class moc_field {
  state, file, title, artist, song, album, totaltime, timeleft,
  totalsec, curtime, cursec, bitrate, avgbitrate, rate
};

struct moc_field_info {
  const char *key;                  // "State:" prefix from mocp -i output
  std::string moc_result::*member;
  const char *fallback;
};

static constexpr moc_field_info moc_fields[] = {
    {"State:", &moc_result::state, "??"},
    {"File:", &moc_result::file, "no file"},
    {"Title:", &moc_result::title, "no title"},
    {"Artist:", &moc_result::artist, "no artist"},
    {"SongTitle:", &moc_result::song, "no song"},
    {"Album:", &moc_result::album, "no album"},
    {"TotalTime:", &moc_result::totaltime, "0:00"},
    {"TimeLeft:", &moc_result::timeleft, "0:00"},
    {"TotalSec:", &moc_result::totalsec, "0"},
    {"CurrentTime:", &moc_result::curtime, "0:00"},
    {"CurrentSec:", &moc_result::cursec, "0"},
    {"Bitrate:", &moc_result::bitrate, "0kbps"},
    {"AvgBitrate:", &moc_result::avgbitrate, "0kbps"},
    {"Rate:", &moc_result::rate, "0kHz"},
};

class moc_cb : public conky::callback<moc_result> {
  using Base = conky::callback<moc_result>;

 protected:
  void work() override;

 public:
  explicit moc_cb(uint32_t period) : Base(period, false, Tuple()) {}
};

void moc_cb::work() {
  moc_result moc;
  FILE *fp;

  fp = popen("mocp -i", "r");
  if (fp == nullptr) {
    LOG_ERROR("failed to run 'mocp -i'");
    moc.state = "Can't run 'mocp -i'";
  } else {
    while (1) {
      char line[100];
      char *p;

      /* Read a line from the pipe and strip the possible '\n'. */
      if (fgets(line, 100, fp) == nullptr) { break; }
      if ((p = strrchr(line, '\n')) != nullptr) { *p = '\0'; }

      /**
       * Parse infos. Example output of mocp -i:
       * State: PLAY
       * File: /home/user/music/Korn - Falling Away From Me.ogg
       * Title: 2 Korn - Falling Away From Me (Issues)
       * Artist: Korn
       * SongTitle: Falling Away From Me
       * Album: Issues
       * TotalTime: 04:31
       * TimeLeft: 02:44
       * TotalSec: 271
       * CurrentTime: 01:47
       * CurrentSec: 107
       * Bitrate: 54kbps
       * AvgBitrate: 233kbps
       * Rate: 44kHz
       **/
      for (const auto &f : moc_fields) {
        auto len = strlen(f.key);
        if (strncmp(line, f.key, len) == 0) {
          moc.*f.member = line + len + 1;
          break;
        }
      }
    }
  }

  pclose(fp);

  std::lock_guard<std::mutex> l(result_mutex);
  result = moc;
}
}  // namespace

using namespace conky::text_object;

template <moc_field F>
variable_definition moc_var(const char *name) {
  return {name, [](text_object *obj, const construct_context &) {
    obj->callbacks.print = [](text_object *, char *p, unsigned int s) {
      constexpr auto &info = moc_fields[static_cast<int>(F)];
      uint32_t period = std::max(
          lround(music_player_interval.get(*state) / active_update_interval()), 1l);
      const auto moc = conky::register_cb<moc_cb>(period)->get_result_copy();
      const auto &val = moc.*info.member;
      snprintf(p, s, "%s", val.length() ? val.c_str() : info.fallback);
    };
  }};
}

// clang-format off
CONKY_REGISTER_VARIABLES(
    moc_var<moc_field::state>("moc_state"),
    moc_var<moc_field::file>("moc_file"),
    moc_var<moc_field::title>("moc_title"),
    moc_var<moc_field::artist>("moc_artist"),
    moc_var<moc_field::song>("moc_song"),
    moc_var<moc_field::album>("moc_album"),
    moc_var<moc_field::totaltime>("moc_totaltime"),
    moc_var<moc_field::timeleft>("moc_timeleft"),
    moc_var<moc_field::totalsec>("moc_totalsec"),
    moc_var<moc_field::curtime>("moc_curtime"),
    moc_var<moc_field::cursec>("moc_cursec"),
    moc_var<moc_field::bitrate>("moc_bitrate"),
    moc_var<moc_field::avgbitrate>("moc_avgbitrate"),
    moc_var<moc_field::rate>("moc_rate"),
    {"moc_percent", [](text_object *obj, const construct_context &) {
      obj->callbacks.percentage = [](text_object *) -> uint8_t {
        uint32_t period = std::max(
            lround(music_player_interval.get(*state) / active_update_interval()), 1l);
        const auto moc = conky::register_cb<moc_cb>(period)->get_result_copy();
        int totalsec = atoi(moc.totalsec.c_str());
        int cursec = atoi(moc.cursec.c_str());
        return totalsec == 0 ? 0 : round_to_positive_int(100.0f * cursec / totalsec);
      };
    }},
    {"moc_bar", [](text_object *obj, const construct_context &ctx) {
      scan_bar(obj, ctx.arg, 1);
      obj->callbacks.barval = [](text_object *) -> double {
        uint32_t period = std::max(
            lround(music_player_interval.get(*state) / active_update_interval()), 1l);
        const auto moc = conky::register_cb<moc_cb>(period)->get_result_copy();
        int totalsec = atoi(moc.totalsec.c_str());
        int cursec = atoi(moc.cursec.c_str());
        return totalsec == 0 ? 0.0 : static_cast<double>(cursec) / totalsec;
      };
    }},
)
// clang-format on
