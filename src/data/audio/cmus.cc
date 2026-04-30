/*
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

#include "../../conky.h"
#include "../../logging.h"
#include "content/specials.h"
#include "../../content/text_object.h"
#include "parse/variables.hh"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <mutex>

#include "../../update-cb.hh"

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

class cmus_cb : public conky::callback<cmus_result> {
  typedef conky::callback<cmus_result> Base;

 protected:
  virtual void work();

 public:
  explicit cmus_cb(uint32_t period) : Base(period, false, Tuple()) {}
};

void cmus_cb::work() {
  cmus_result cmus;
  FILE *fp;

  fp = popen("cmus-remote -Q 2>/dev/null", "r");
  if (!fp) {
    LOG_ERROR("failed to run 'cmus-remote -Q'");
    cmus.state = "Can't run 'cmus-remote -Q'";
  } else {
    while (1) {
      char line[255];
      char *p;

      /* Read a line from the pipe and strip the possible '\n'. */
      if (!fgets(line, 255, fp)) break;
      if ((p = strrchr(line, '\n'))) *p = '\0';

      /* Parse infos. */
      if (strncmp(line, "status ", 7) == 0) {
        cmus.state = line + 7;

      } else if (strncmp(line, "file ", 5) == 0) {
        cmus.file = line + 5;

      } else if (strncmp(line, "tag artist ", 11) == 0) {
        cmus.artist = line + 11;

      } else if (strncmp(line, "tag title ", 10) == 0) {
        cmus.title = line + 10;

      } else if (strncmp(line, "tag album ", 10) == 0) {
        cmus.album = line + 10;

      } else if (strncmp(line, "duration ", 9) == 0) {
        cmus.totaltime = line + 9;

      } else if (strncmp(line, "position ", 9) == 0) {
        cmus.curtime = line + 9;
        cmus.timeleft = strtol(cmus.totaltime.c_str(), nullptr, 10) -
                        strtol(cmus.curtime.c_str(), nullptr, 10);
        if (cmus.curtime.size() > 0) {
          cmus.progress =
              static_cast<float>(strtol(cmus.curtime.c_str(), nullptr, 10)) /
              strtol(cmus.totaltime.c_str(), nullptr, 10);
        } else {
          cmus.progress = 0;
        }
      }

      else if (strncmp(line, "set shuffle ", 12) == 0) {
        cmus.random = (strncmp(line + 12, "true", 4) == 0 ? "on" : "off");

      } else if (strncmp(line, "set repeat ", 11) == 0) {
        cmus.repeat = (strncmp((line + 11), "true", 4) == 0 ? "all" : "off");

      } else if (strncmp(line, "set repeat_current ", 19) == 0) {
        cmus.repeat =
            (strncmp((line + 19), "true", 4) == 0 ? "song" : cmus.repeat);
      } else if (strncmp(line, "set aaa_mode ", 13) == 0) {
        cmus.aaa = line + 13;

      } else if (strncmp(line, "tag tracknumber ", 16) == 0) {
        cmus.track = line + 16;
      } else if (strncmp(line, "tag genre ", 10) == 0) {
        cmus.genre = line + 10;
      } else if (strncmp(line, "tag date ", 9) == 0) {
        cmus.date = line + 9;
      }
    }
  }

  pclose(fp);

  std::lock_guard<std::mutex> l(result_mutex);
  result = cmus;
}
}  // namespace

enum class cmus_field { state, file, title, artist, album, random, repeat, aaa, track, genre, date };

struct cmus_field_info {
  std::string cmus_result::*member;
  const char *fallback;
};

static constexpr cmus_field_info cmus_fields[] = {
    {&cmus_result::state, "Off"},
    {&cmus_result::file, "no file"},
    {&cmus_result::title, "no title"},
    {&cmus_result::artist, "no artist"},
    {&cmus_result::album, "no album"},
    {&cmus_result::random, ""},
    {&cmus_result::repeat, ""},
    {&cmus_result::aaa, "all"},
    {&cmus_result::track, "no track"},
    {&cmus_result::genre, ""},
    {&cmus_result::date, ""},
};

using namespace conky::text_object;

template <cmus_field F>
variable_definition cmus_var(const char *name) {
  return {name, [](text_object *obj, const construct_context &) {
    obj->callbacks.print = [](text_object *, char *p, unsigned int s) {
      constexpr auto &info = cmus_fields[static_cast<int>(F)];
      uint32_t period = std::max(
          lround(music_player_interval.get(*state) / active_update_interval()), 1l);
      const auto cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
      const auto &val = cmus.*info.member;
      snprintf(p, s, "%s", val.length() ? val.c_str() : info.fallback);
    };
  }};
}

// clang-format off
CONKY_REGISTER_VARIABLES(
    cmus_var<cmus_field::state>("cmus_state"),
    cmus_var<cmus_field::file>("cmus_file"),
    cmus_var<cmus_field::title>("cmus_title"),
    cmus_var<cmus_field::artist>("cmus_artist"),
    cmus_var<cmus_field::album>("cmus_album"),
    {"cmus_totaltime", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = [](text_object *, char *p, unsigned int s) {
        uint32_t period = std::max(
            lround(music_player_interval.get(*state) / active_update_interval()), 1l);
        const auto cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
        format_seconds_short(p, s, strtol(cmus.totaltime.c_str(), nullptr, 10));
      };
    }},
    {"cmus_timeleft", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = [](text_object *, char *p, unsigned int s) {
        uint32_t period = std::max(
            lround(music_player_interval.get(*state) / active_update_interval()), 1l);
        const auto cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
        format_seconds_short(p, s, static_cast<long>(cmus.timeleft));
      };
    }},
    {"cmus_curtime", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = [](text_object *, char *p, unsigned int s) {
        uint32_t period = std::max(
            lround(music_player_interval.get(*state) / active_update_interval()), 1l);
        const auto cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
        format_seconds_short(p, s, strtol(cmus.curtime.c_str(), nullptr, 10));
      };
    }},
    cmus_var<cmus_field::random>("cmus_random"),
    cmus_var<cmus_field::repeat>("cmus_repeat"),
    cmus_var<cmus_field::aaa>("cmus_aaa"),
    cmus_var<cmus_field::track>("cmus_track"),
    cmus_var<cmus_field::genre>("cmus_genre"),
    cmus_var<cmus_field::date>("cmus_date"),
    {"cmus_progress", [](text_object *obj, const construct_context &ctx) {
      scan_bar(obj, ctx.arg, 1);
      obj->callbacks.barval = [](text_object *) -> double {
        uint32_t period = std::max(
            lround(music_player_interval.get(*state) / active_update_interval()), 1l);
        const auto cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
        return static_cast<double>(cmus.progress);
      };
    }},
    {"cmus_percent", [](text_object *obj, const construct_context &) {
      obj->callbacks.percentage = [](text_object *) -> uint8_t {
        uint32_t period = std::max(
            lround(music_player_interval.get(*state) / active_update_interval()), 1l);
        const auto cmus = conky::register_cb<cmus_cb>(period)->get_result_copy();
        return static_cast<uint8_t>(round(cmus.progress * 100.0f));
      };
    }},
)
// clang-format on
