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
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
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

#include <string.h>
#include <systemd/sd-journal.h>
#include <time.h>
#include <unistd.h>
#include <memory>
#include "../../buffer.hh"
#include "config.h"
#include "../../conky.h"
#include "../../logging.h"
#include "../../content/text_object.h"

#define MAX_JOURNAL_LINES 200

class journal {
 public:
  int wantedlines;
  int flags;

  journal() : wantedlines(0), flags(SD_JOURNAL_LOCAL_ONLY) {}
};

void free_journal(struct text_object *obj) {
  journal *j = (journal *)obj->data.opaque;
  obj->data.opaque = nullptr;
  delete j;
}

void init_journal(const char *type, const char *arg, struct text_object *obj,
                  void *free_at_crash) {
  unsigned int args;
  journal *j = new journal;

  std::unique_ptr<char[]> tmp(new char[DEFAULT_TEXT_BUFFER_SIZE]);
  memset(tmp.get(), 0, DEFAULT_TEXT_BUFFER_SIZE);

  args = sscanf(arg, "%d %6s", &j->wantedlines, tmp.get());
  if (args < 1 || args > 2) {
    free_journal(obj);
    free(obj);
    free(free_at_crash);
    COMMAND_ARG_ERR(type,
        "{} needs a number of lines as 1st argument and optionally a journal "
        "type as 2nd argument",
        type);
  }
  if (j->wantedlines > 0 && j->wantedlines <= MAX_JOURNAL_LINES) {
    if (args > 1) {
      if (strcmp(tmp.get(), "system") == 0) {
        j->flags |= SD_JOURNAL_SYSTEM;
      } else if (strcmp(tmp.get(), "user") == 0) {
        j->flags |= SD_JOURNAL_CURRENT_USER;
      } else {
        free_journal(obj);
        free(obj);
        free(free_at_crash);
        COMMAND_ARG_ERR(type, "invalid arg for {}, type must be 'system' or 'user'",
                 type);
      }
    } else {
      LOG_WARNING("you should specify 'user' or 'system' as an argument");
    }

  } else {
    free_journal(obj);
    free(obj);
    free(free_at_crash);
    COMMAND_ARG_ERR(type,
        "invalid arg for {}, number of lines must be between 1 and {}", type,
        MAX_JOURNAL_LINES);
  }
  obj->data.opaque = j;
}

static int print_field(sd_journal *jh, const char *field, char spacer,
                       conky::buffer_writer &out) {
  const void *get;
  size_t length;
  size_t fieldlen = strlen(field) + 1;

  int ret = sd_journal_get_data(jh, field, &get, &length);
  if (ret == -ENOENT) goto out;

  if (ret < 0 || length - fieldlen > out.remaining()) return -1;

  out.append((const char *)get + fieldlen, length - fieldlen);

out:
  if (spacer && !out.append(spacer)) return -1;
  return length ? length - fieldlen : 0;
}

bool read_log(sd_journal *jh, conky::buffer_writer &out) {
  struct tm tm;
  uint64_t timestamp;
  time_t time;
  size_t length;

  if (sd_journal_get_realtime_usec(jh, &timestamp) < 0) return false;
  time = timestamp / 1000000;
  localtime_r(&time, &tm);

  if ((length = strftime(out.cursor(), out.remaining(), "%b %d %H:%M:%S",
                         &tm)) <= 0)
    return false;
  out.advance(length);

  if (!out.append(' ')) return false;

  if (print_field(jh, "_HOSTNAME", ' ', out) < 0) return false;
  if (print_field(jh, "SYSLOG_IDENTIFIER", '[', out) < 0) return false;
  if (print_field(jh, "_PID", ']', out) < 0) return false;
  if (!out.append(':')) return false;
  if (!out.append(' ')) return false;
  if (print_field(jh, "MESSAGE", '\n', out) < 0) return false;
  return true;
}

void print_journal(struct text_object *obj, char *p, unsigned int p_max_size) {
  journal *j = (journal *)obj->data.opaque;
  sd_journal *jh = nullptr;
  conky::buffer_writer out(p_max_size, p);

  if (sd_journal_open(&jh, j->flags) != 0) {
    LOG_ERROR("unable to open journal");
    goto done;
  }

  if (sd_journal_seek_tail(jh) < 0) {
    LOG_ERROR("unable to seek to end of journal");
    goto done;
  }
  if (sd_journal_previous_skip(jh, j->wantedlines) < 0) {
    LOG_ERROR("unable to seek back {} lines", j->wantedlines);
    goto done;
  }

  while (read_log(jh, out) && sd_journal_next(jh))
    ;

done:
  if (jh) sd_journal_close(jh);
  out.terminate();
}
