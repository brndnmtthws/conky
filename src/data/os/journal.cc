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

#include <systemd/sd-journal.h>
#include <chrono>
#include <cstring>
#include <ctime>
#include <memory>
#include "../../buffer.hh"
#include "../../conky.h"
#include "../../content/text_object.h"
#include "../../logging.h"

struct journal {
  int wanted_lines;
  int flags;

  journal() : wanted_lines(1), flags(SD_JOURNAL_LOCAL_ONLY) {}
};

static void free_journal(struct text_object *obj) {
  delete static_cast<journal *>(obj->data.opaque);
}

void init_journal(const char *type, const char *arg, struct text_object *obj) {
  unsigned int argc;
  auto options = std::make_unique<journal>();

  char type_arg[8] = {};
  char ignored[2] = {};
  argc = sscanf(arg, "%d %7s %1s", &options->wanted_lines, type_arg, ignored);
  if (argc > 2) {
    COMMAND_ARG_ERR(
        type,
        "too many arguments provided; expected: [line_count=1] [type='local']");
  }

  if (options->wanted_lines < 0) {
    LOG_WARNING("invalid line count {}; clamping to 1", options->wanted_lines);
    options->wanted_lines = 1;
  }

  if (argc >= 2) {
    if (strcmp(type_arg, "local") == 0) {
      options->flags |= SD_JOURNAL_LOCAL_ONLY; /* no-op */
    } else if (strcmp(type_arg, "runtime") == 0) {
      options->flags |= SD_JOURNAL_RUNTIME_ONLY;
    } else if (strcmp(type_arg, "system") == 0) {
      options->flags |= SD_JOURNAL_SYSTEM;
    } else if (strcmp(type_arg, "user") == 0) {
      options->flags |= SD_JOURNAL_CURRENT_USER;
    } else {
      COMMAND_ARG_ERR(
          type, "type must be one of: 'local', 'runtime', 'system', 'user'");
    }
  }

  obj->data.opaque = options.release();
  obj->callbacks.free = &free_journal;
}

static bool print_field(sd_journal *handle, const char *field, char spacer,
                        conky::buffer_writer &out) {
  const void *data;
  size_t length;
  size_t fieldlen = strlen(field) + 1;

  int ret = sd_journal_get_data(handle, field, &data, &length);
  if (ret >= 0) {
    if (length - fieldlen > out.remaining()) return false;
    out.append(static_cast<const char *>(data) + fieldlen, length - fieldlen);
  } else if (ret != -ENOENT) {
    return false;
  }

  return !spacer || out.append(spacer);
}

bool read_log(sd_journal *handle, conky::buffer_writer &out) {
  uint64_t usec;
  if (sd_journal_get_realtime_usec(handle, &usec) < 0) return false;

  auto tp =
      std::chrono::system_clock::time_point{std::chrono::microseconds{usec}};
  std::time_t epoch = std::chrono::system_clock::to_time_t(tp);
  std::tm local_tm{};
  localtime_r(&epoch, &local_tm);

  size_t length =
      strftime(out.cursor(), out.remaining(), "%b %d %H:%M:%S", &local_tm);
  if (length == 0) return false;
  out.advance(length);

  if (!out.append(' ')) return false;
  if (!print_field(handle, "_HOSTNAME", ' ', out)) return false;
  if (!print_field(handle, "SYSLOG_IDENTIFIER", '[', out)) return false;
  if (!print_field(handle, "_PID", ']', out)) return false;
  if (!out.append(':')) return false;
  if (!out.append(' ')) return false;
  if (!print_field(handle, "MESSAGE", '\n', out)) return false;
  return true;
}

void print_journal(struct text_object *obj, char *p, unsigned int p_max_size) {
  journal *conf = static_cast<journal *>(obj->data.opaque);
  conky::buffer_writer out(p_max_size, p);
  out.terminate();

  sd_journal *raw = nullptr;
  if (sd_journal_open(&raw, conf->flags) != 0) {
    LOG_ERROR("unable to open journal");
    return;
  }
  auto handle = std::unique_ptr<sd_journal, decltype(&sd_journal_close)>(
      raw, sd_journal_close);

  if (conf->wanted_lines == 0) {
    if (sd_journal_seek_head(handle.get()) < 0) {
      LOG_ERROR("unable to seek to start of journal");
      return;
    }
    if (sd_journal_next(handle.get()) <= 0) { return; }
  } else {
    if (sd_journal_seek_tail(handle.get()) < 0) {
      LOG_ERROR("unable to seek to end of journal");
      return;
    }
    if (sd_journal_previous_skip(handle.get(), conf->wanted_lines) < 0) {
      LOG_ERROR("unable to seek back {} lines", conf->wanted_lines);
      return;
    }
  }

  while (read_log(handle.get(), out) && sd_journal_next(handle.get()));
  out.terminate();
}
