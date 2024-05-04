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

#ifndef _CONKY_LOGGER_H
#define _CONKY_LOGGER_H

#include "config.h"

#include "i18n.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cinttypes>  // correct formatting for int types
#include <cstdio>
#include <cstring>
#include <ctime>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#if __has_include(<syslog.h>)
#define HAS_SYSLOG
#endif

#ifdef HAS_SYSLOG
extern "C" {
#include <syslog.h>
// hide syslog level definitions
#undef LOG_EMERG
#undef LOG_ALERT
#undef LOG_CRIT
#undef LOG_ERR
#undef LOG_WARNING
#undef LOG_NOTICE
#undef LOG_INFO
#undef LOG_DEBUG
}
#endif /* HAS_SYSLOG */

namespace conky::log {
/// @brief Logging levels.
///
/// Values match syslog ones, with addition of `TRACE` which is local.
/// `Emergency` (0) and `Alert` (1) are not used as they're not warranted from a
/// userspace application.
enum class level {
  OFF = 1,
  CRITICAL = 2,
  ERROR = 3,
  WARNING = 4,
  NOTICE = 5,
  INFO = 6,
  /// @brief Debug information.
  ///
  /// Previously DBGP.
  DEBUG = 7,
  /// @brief Very detailed information useful for debugging purposes.
  ///
  /// Previously DBGP2.
  TRACE = 8,
};

const auto level_names = std::array<const char *, 7>{
    "CRITICAL", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG", "TRACE",
};
const size_t MAX_LEVEL_NAME_LEN = 8;

constexpr inline const char *log_level_to_cstr(level log_level) {
  return level_names[static_cast<size_t>(log_level) - 2];
}

using clock = std::chrono::system_clock;
using instant = clock::time_point;

struct source_details {
  const char *file = nullptr;
  size_t line;
};

struct log_details {
  std::optional<source_details> source;
  std::optional<instant> time;
};

static const size_t TIME_LEN = 24;

namespace _priv {
inline size_t format_log_time(char *out, size_t max_len, instant time) {
  std::time_t current_time = std::chrono::system_clock::to_time_t(time);
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
      time.time_since_epoch());
  struct tm local_time;
  localtime_r(&current_time, &local_time);
  size_t time_len = 0;
  time_len += std::strftime(out, max_len, "%F %T", &local_time);
  time_len += snprintf(&out[time_len], max_len - time_len, ".%03d",
                       static_cast<int>(millis.count() % 1000));

  return time_len;
}

inline size_t format_log_time(FILE *out, size_t max_len, instant time) {
  char buffer[TIME_LEN];
  size_t result = format_log_time(&buffer[0], max_len, time);
  fprintf(out, "%s", buffer);
  return result;
}

#define VAL_AND_LEN(STR) (std::make_pair("][" STR "]", sizeof(STR) + 3))
const std::array<std::pair<const char *, size_t>, 7> _FMT_DATA{
    VAL_AND_LEN("CRITICAL"), VAL_AND_LEN("ERROR"), VAL_AND_LEN("WARNING"),
    VAL_AND_LEN("NOTICE"),   VAL_AND_LEN("INFO"),  VAL_AND_LEN("DEBUG"),
    VAL_AND_LEN("TRACE"),
};
#undef VAL_AND_LEN
constexpr inline const char *_log_level_fmt(level log_level) {
  return _FMT_DATA[static_cast<size_t>(log_level) - 2].first;
}
constexpr inline size_t _log_level_fmt_len(level log_level) {
  return _FMT_DATA[static_cast<size_t>(log_level) - 2].second;
}

/// @brief Given some `base` path string view, returns last part of the string
/// that contains (starts with) `/src/`. If no such path is found, `begin` of
/// `base` is returned instead. First `/` character is excluded from result.
///
/// @note Provided `base` should (but doesn't have to) be a null terminated
/// string.
///
/// @param base string to trim
/// @return start of `base` or ending that starts with `/src/`
constexpr const char *relative_source_path(const std::string_view &base) {
  // leave space to lookup previous "/src/" characters
  auto last = base.begin() + 5;
  for (auto it = base.end(); it != last; it--) {
    if (*it == '/' && *(it - 1) == 'c' && *(it - 2) == 'r' &&
        *(it - 3) == 's' && *(it - 4) == '/') {
      return it - 3;
    }
  }
  return base.begin();
}

template <typename... Args>
inline void _impl_syslog(level log_level, const char *format, Args &&...args) {
  syslog(static_cast<int>(log_level), format, args...);
}
inline void _impl_syslog(level log_level, const char *format) {
  syslog(static_cast<int>(log_level), "%s", format);
}

template <typename... Args>
inline void _impl_fprintf(FILE *out, const char *format, Args &&...args) {
  fprintf(out, format, args...);
}
inline void _impl_fprintf(FILE *out, const char *format) {
  fprintf(out, "%s", format);
}
}  // namespace _priv

const size_t MAX_LOG_TARGETS = 5;
const level DEFAULT_LOG_LEVEL = level::NOTICE;
struct logger {
  struct log_target {
    FILE *stream = nullptr;
    level log_level = DEFAULT_LOG_LEVEL;

   public:
    bool is_enabled(level log_level) const {
      return static_cast<int>(this->log_level) >= static_cast<int>(log_level);
    }
    void set_log_level(level log_level) { this->log_level = log_level; }
    void log_more() {
      this->log_level =
          static_cast<level>(std::min(static_cast<int>(this->log_level) + 1,
                                      static_cast<int>(level::TRACE)));
    }
    void log_less() {
      this->log_level = static_cast<level>(std::max(
          static_cast<int>(this->log_level) - 1, static_cast<int>(level::OFF)));
    }
  };

 private:
  const char *name;
  std::array<log_target, MAX_LOG_TARGETS> entries;

  size_t check_available() const {
    for (size_t i = 0; i < MAX_LOG_TARGETS; i++) {
      if (entries[i].stream == nullptr) return MAX_LOG_TARGETS - i;
    }
    return 0;
  }

  bool is_any_enabled(level log_level) const {
    for (size_t i = 0; i < MAX_LOG_TARGETS; i++) {
      if (this->entries.at(i).is_enabled(log_level)) return true;
    }
    return false;
  }

 public:
  logger(const char *name) : name(name) {
    this->entries[0] = log_target{
        stderr,
        level::NOTICE,
    };

#ifdef HAS_SYSLOG
    openlog(PACKAGE_NAME, LOG_PID, LOG_USER);
#else
    use_log_file();
#endif
  }
  ~logger() {
    for (auto &entry : this->entries) {
      if (entry.stream != nullptr) {
        fclose(entry.stream);
        entry.stream = nullptr;
      }
    }

#ifdef HAS_SYSLOG
    closelog();
#endif
  }

  const log_target *add_log_file(const std::string &path) {
    int available = check_available();
    if (available < 0) { return nullptr; }
    this->entries[available].stream = fopen(path.c_str(), "a+");
    this->entries[available].log_level = DEFAULT_LOG_LEVEL;
    return &this->entries[available];
  }

  log_target *get_stream_target(FILE *stream) {
    for (size_t i = 0; i < MAX_LOG_TARGETS; i++) {
      if (this->entries[i].stream == stream) { return &this->entries[i]; }
    }
    return nullptr;
  }

  template <level log_level, typename... Args>
  inline void log_print_fmt(log_details &&details, const char *format,
                            Args &&...args) {
#ifdef HAS_SYSLOG
    if constexpr (static_cast<int>(log_level) >=
                      static_cast<int>(level::CRITICAL) &&
                  static_cast<int>(log_level) <
                      static_cast<int>(level::TRACE)) {
      _priv::_impl_syslog(log_level, format, args...);
    }
#endif

    if (!is_any_enabled(log_level)) return;

    static const size_t MAX_FILE_LEN = 32;
    static const size_t MAX_LOCATION_LEN = MAX_FILE_LEN + 1 + 5;  // name:line
    static const size_t PREAMBLE_LENGTH =
        2 + TIME_LEN + 2 + MAX_LEVEL_NAME_LEN + 2 + MAX_LOCATION_LEN;
    char preamble[PREAMBLE_LENGTH + 1] = "[";
    size_t offset = 1;
    // append time
    offset += _priv::format_log_time(&preamble[offset], TIME_LEN,
                                     details.time.value_or(clock::now()));
    // append log level
    std::strncat(&preamble[offset], _priv::_log_level_fmt(log_level),
                 _priv::_log_level_fmt_len(log_level));
    offset += _priv::_log_level_fmt_len(log_level) - 1;
    // append source information
    if (details.source.has_value()) {
      auto source = details.source.value();
      offset += snprintf(&preamble[offset], PREAMBLE_LENGTH - offset,
                         "[%s:%ld]", source.file, source.line);
    }

    // localized output to console
    for (const auto &target : this->entries) {
      if (!target.is_enabled(log_level)) continue;
      fprintf(target.stream, "%s: ", preamble);
      _priv::_impl_fprintf(target.stream, _(format), args...);
      fputs("\n", target.stream);
    }
  }

  template <level log_level, typename... Args>
  void log_location(const char *file, size_t line, const char *format,
                    Args &&...args) {
    this->log_print_fmt<log_level>(
        log_details{
            std::optional(source_details{file, line}),
        },
        format, args...);
  }

  template <level log_level, typename... Args>
  void log(const char *format, Args &&...args) {
    this->log_print_fmt<log_level>(log_details{}, format, args...);
  }
};

}  // namespace conky::log

#endif /* _CONKY_LOGGER_H */
