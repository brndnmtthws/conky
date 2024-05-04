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

#ifndef _LOGGING_H
#define _LOGGING_H

#include "config.h"

#include "i18n.h"

#include <array>
#include <chrono>
#include <cinttypes>  // correct formatting for int types
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
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

class fork_throw : public std::runtime_error {
 public:
  fork_throw() : std::runtime_error("Fork happened") {}
  fork_throw(const std::string &msg) : std::runtime_error(msg) {}
};

class unknown_arg_throw : public std::runtime_error {
 public:
  unknown_arg_throw() : std::runtime_error("Unknown argumunt given") {}
  unknown_arg_throw(const std::string &msg) : std::runtime_error(msg) {}
};

class combine_needs_2_args_error : public std::runtime_error {
 public:
  combine_needs_2_args_error()
      : std::runtime_error("combine needs arguments: <text1> <text2>") {}
  combine_needs_2_args_error(const std::string &msg)
      : std::runtime_error(msg) {}
};

class obj_create_error : public std::runtime_error {
 public:
  obj_create_error() : std::runtime_error("Failed to create object") {}
  obj_create_error(const std::string &msg) : std::runtime_error(msg) {}
};

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

bool is_enabled(level log_level);
void set_log_level(level log_level);
void log_more();
void log_less();

/// Implementation detail:
/// 0 - console output
/// 1 - file output
FILE **_log_streams();
void use_log_file(const char *path = nullptr);
void init_system_logging();
void terminate_logging();

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

template <level log_level, typename... Args>
inline void log_print_fmt(log_details &&details, const char *format,
                          Args &&...args) {
#ifdef HAS_SYSLOG
  if constexpr (static_cast<int>(log_level) >=
                    static_cast<int>(level::CRITICAL) &&
                static_cast<int>(log_level) < static_cast<int>(level::TRACE)) {
    _priv::_impl_syslog(log_level, format, args...);
  }
#endif

  auto streams = _log_streams();
  if (!is_enabled(log_level) && streams[1] != nullptr) return;

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
    offset += snprintf(&preamble[offset], PREAMBLE_LENGTH - offset, "[%s:%ld]",
                       source.file, source.line);
  }

  // localized output to console
  if (is_enabled(log_level)) {
    fprintf(streams[0], "%s: ", preamble);
    _priv::_impl_fprintf(streams[0], _(format), args...);
    fputs("\n", streams[0]);
  }
  // unlocalized output to file
  if (streams[1] != nullptr) {
    fprintf(streams[1], "%s: ", preamble);
    _priv::_impl_fprintf(streams[1], format, args...);
    fputs("\n", streams[1]);
  }
}

template <level log_level, typename... Args>
void log_location(const char *file, size_t line, const char *format,
                  Args &&...args) {
  log_print_fmt<log_level>(
      log_details{
          std::optional(source_details{file, line}),
      },
      format, args...);
}

template <level log_level, typename... Args>
void log(const char *format, Args &&...args) {
  log_print_fmt<log_level>(log_details{}, format, args...);
}

#define LOG(Level, ...)                                              \
  ::conky::log::log_location<::conky::log::level::Level>(            \
      ::conky::log::_priv::relative_source_path(__FILE__), __LINE__, \
      __VA_ARGS__)

#define LOG_CRITICAL(...) LOG(CRITICAL, __VA_ARGS__)
#define LOG_ERROR(...) LOG(ERROR, __VA_ARGS__)
#define LOG_WARNING(...) LOG(WARNING, __VA_ARGS__)
#define LOG_NOTICE(...) LOG(NOTICE, __VA_ARGS__)
#define LOG_INFO(...) LOG(INFO, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(DEBUG, __VA_ARGS__)
#define LOG_TRACE(...) LOG(TRACE, __VA_ARGS__)

}  // namespace conky::log

// backwards compatibility aliases
#define NORM_ERR(...) LOG_INFO(__VA_ARGS__)
#define DBGP(...) LOG_DEBUG(__VA_ARGS__)
#define DBGP2(...) LOG_TRACE(__VA_ARGS__)

/// @brief Error that warrants termination of the program, and is caused by
/// developer so it should produce a core dump for reporting.
///
/// @param Format printf style format string.
/// @param Args printf style arguments.
#define CRIT_ERR(...)        \
  LOG_CRITICAL(__VA_ARGS__); \
  std::terminate()

extern void clean_up();

/// @brief Error that warrants termination of the program, but is caused by user
/// error (e.g. bad input) and as such a core dump isn't useful.
///
/// @param Format printf style format string.
/// @param Args printf style arguments.
#define USER_ERR(...)     \
  LOG_ERROR(__VA_ARGS__); \
  clean_up();             \
  std::exit(EXIT_FAILURE)

/// @brief Error caused by system not supporting some required conky feature.
///
/// We assume the fault is with user for not properly configuring conky so we
/// don't produce a core dump.
///
/// @param Format printf style format string.
/// @param Args printf style arguments.
#define SYSTEM_ERR(...)   \
  LOG_ERROR(__VA_ARGS__); \
  clean_up();             \
  std::exit(EXIT_FAILURE)

/* critical error with additional cleanup */
#define CRIT_ERR_FREE(memtofree1, memtofree2, ...) \
  free(memtofree1);                                \
  free(memtofree2);                                \
  SYSTEM_ERR(__VA_ARGS__);

namespace conky {
class error : public std::runtime_error {
 public:
  error(const std::string &msg) : std::runtime_error(msg) {}
};
}  // namespace conky

#endif /* _LOGGING_H */
