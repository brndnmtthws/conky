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

#include <ctype.h>
#include <string.h>
#include <systemd/sd-journal.h>
#include <time.h>
#include <unistd.h>
#include <memory>
#include "common.h"
#include "config.h"
#include "conky.h"
#include "logging.h"
#include "text_object.h"

#define MAX_JOURNAL_LINES 200
#ifndef SD_JOURNAL_SYSTEM
// SD_JOURNAL_SYSTEM added and SD_JOURNAL_SYSTEM_ONLY deprecated in systemd-205
#define SD_JOURNAL_SYSTEM SD_JOURNAL_SYSTEM_ONLY
#endif /* SD_JOURNAL_SYSTEM */

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
    CRIT_ERR(obj, free_at_crash,
             "%s a number of lines as 1st argument and optionally a journal "
             "type as 2nd argument",
             type);
  }
  if (j->wantedlines > 0 && j->wantedlines <= MAX_JOURNAL_LINES) {
    if (args > 1) {
      if (strcmp(tmp.get(), "system") == 0) {
        j->flags |= SD_JOURNAL_SYSTEM;
#ifdef SD_JOURNAL_CURRENT_USER  // not present in older version of systemd
      } else if (strcmp(tmp.get(), "user") == 0) {
        j->flags |= SD_JOURNAL_CURRENT_USER;
#endif /* SD_JOURNAL_CURRENT_USER */
      } else {
        free_journal(obj);
        CRIT_ERR(obj, free_at_crash,
                 "invalid arg for %s, type must be 'system' or 'user'", type);
      }
    } else {
      NORM_ERR("You should type a 'user' or 'system' as an argument");
    }

  } else {
    free_journal(obj);
    CRIT_ERR(obj, free_at_crash,
             "invalid arg for %s, number of lines must be between 1 and %d",
             type, MAX_JOURNAL_LINES);
  }
  obj->data.opaque = j;
}

static int print_field(sd_journal *jh, const char *field, char spacer,
                       size_t *read, char *p, unsigned int p_max_size) {
  const void *get;
  size_t length;
  size_t fieldlen = strlen(field) + 1;

  int ret = sd_journal_get_data(jh, field, &get, &length);
  if (ret == -ENOENT) goto out;

  if (ret < 0 || length + *read > p_max_size) return -1;

  memcpy(p + *read, (const char *)get + fieldlen, length - fieldlen);
  *read += length - fieldlen;

out:
  if (spacer) {
    if (p_max_size < *read) {
      *read = p_max_size - 1;
    } else {
      p[(*read)++] = spacer;
    }
  }
  return length ? length - fieldlen : 0;
}

bool read_log(size_t *read, size_t *length, time_t *time, uint64_t *timestamp,
              sd_journal *jh, char *p, unsigned int p_max_size) {
  struct tm tm;
  if (sd_journal_get_realtime_usec(jh, timestamp) < 0) return false;
  *time = *timestamp / 1000000;
  localtime_r(time, &tm);

  if ((*length =
           strftime(p + *read, p_max_size - *read, "%b %d %H:%M:%S", &tm)) <= 0)
    return false;
  *read += *length;

  if (p_max_size < *read) {
    *read = p_max_size - 1;
    return false;
  }
  p[*read++] = ' ';

  if (print_field(jh, "_HOSTNAME", ' ', read, p, p_max_size) < 0) return false;

  if (print_field(jh, "SYSLOG_IDENTIFIER", '[', read, p, p_max_size) < 0)
    return false;

  if (print_field(jh, "_PID", ']', read, p, p_max_size) < 0) return false;

  if (p_max_size < *read) {
    *read = p_max_size - 1;
    return false;
  }
  p[*read++] = ':';

  if (p_max_size < *read) {
    *read = p_max_size - 1;
    return false;
  }
  p[*read++] = ' ';

  if (print_field(jh, "MESSAGE", '\n', read, p, p_max_size) < 0) return false;
  return true;
}

void print_journal(struct text_object *obj, char *p, unsigned int p_max_size) {
  struct journal *j = (struct journal *)obj->data.opaque;
  sd_journal *jh = nullptr;
  size_t read = 0;
  size_t length;
  time_t time;
  uint64_t timestamp;

  if (sd_journal_open(&jh, j->flags) != 0) {
    NORM_ERR("unable to open journal");
    goto out;
  }

  if (sd_journal_seek_tail(jh) < 0) {
    NORM_ERR("unable to seek to end of journal");
    goto out;
  }
  if (sd_journal_previous_skip(jh, j->wantedlines) < 0) {
    NORM_ERR("unable to seek back %d lines", j->wantedlines);
    goto out;
  }

  while (read_log(&read, &length, &time, &timestamp, jh, p, p_max_size) &&
         sd_journal_next(jh))
    ;

out:
  if (jh) sd_journal_close(jh);
  p[read] = '\0';
}
