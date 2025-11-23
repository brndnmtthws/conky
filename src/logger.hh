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

#include "str_buffer.hh"

#ifdef BUILD_I18N
#include <libintl.h>
#else
#define gettext(string) (string)
#endif
#define _(string) gettext(string)

#include <algorithm>
#include <array>
#include <chrono>
#include <cinttypes>  // correct formatting for int types
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_set>

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

#define ENABLE_LOG_COLORS  // FIXME: Make compile option after debugging

#ifdef ENABLE_LOG_COLORS

extern "C" {
#include <unistd.h>
}

namespace fs = std::filesystem;

namespace conky::log {
/// Control Sequence Introducer
#define CSI "\033["
#define NORMAL "0;"
#define BOLD "1;"

struct colors {
  struct color {
    const char *value;
    size_t length;
  };

#define DECL_COLOR(Name, Value) \
  static inline color Name = color { #Value, sizeof(#Value) }

  // Should work on most posix compliant systems (or be hidden): Linux, FreeBSD,
  // MacOS, and (even) Windows 10. Feel free to add custom definitions if ANSI
  // colors don't work for you
  DECL_COLOR(BLACK, CSI NORMAL "30m");
  DECL_COLOR(RED, CSI NORMAL "31m");
  DECL_COLOR(GREEN, CSI NORMAL "32m");
  DECL_COLOR(YELLOW, CSI NORMAL "33m");
  DECL_COLOR(BLUE, CSI NORMAL "34m");
  DECL_COLOR(MAGENTA, CSI NORMAL "35m");
  DECL_COLOR(CYAN, CSI NORMAL "36m");
  DECL_COLOR(WHITE, CSI NORMAL "37m");
  DECL_COLOR(BLACK_BOLD, CSI BOLD "30m");
  DECL_COLOR(RED_BOLD, CSI BOLD "31m");
  DECL_COLOR(GREEN_BOLD, CSI BOLD "32m");
  DECL_COLOR(YELLOW_BOLD, CSI BOLD "33m");
  DECL_COLOR(BLUE_BOLD, CSI BOLD "34m");
  DECL_COLOR(MAGENTA_BOLD, CSI BOLD "35m");
  DECL_COLOR(CYAN_BOLD, CSI BOLD "36m");
  DECL_COLOR(WHITE_BOLD, CSI BOLD "37m");
  DECL_COLOR(RESET, CSI "0m");

#undef DECL_COLOR
};

#undef BOLD
#undef NORMAL
#undef CSI
}  // namespace conky::log

#endif /* ENABLE_LOG_COLORS */

namespace conky::log {
/// @brief Logging levels.
///
/// Values match syslog ones, with addition of `TRACE` which is local.
/// `Emergency` (0) and `Alert` (1) are not used as they're not warranted from a
/// userspace application.
enum class log_level : uint8_t {
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

constexpr inline const char *log_level_to_cstr(log_level level) {
  return level_names[static_cast<size_t>(level) - 2];
}

using clock = std::chrono::system_clock;
using instant = clock::time_point;

struct source_details {
  const char *file = nullptr;
  size_t line;
};

struct msg_details {
  log_level level;
  std::optional<source_details> source;
  std::optional<instant> time;
};

enum class detail {
  LEVEL,
  SOURCE,
  TIME,
  CONTEXT,
};

static const size_t TIME_LEN = 24;

namespace _priv {
constexpr inline const char *_log_level_fmt(log_level level) {
  return _FMT_DATA[static_cast<size_t>(level) - 2].first;
}
constexpr inline size_t _log_level_fmt_len(log_level level) {
  return _FMT_DATA[static_cast<size_t>(level) - 2].second;
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
inline void _impl_fprintf(FILE *out, const char *format, Args &&...args) {
  fprintf(out, format, args...);
}
inline void _impl_fprintf(FILE *out, const char *format) {
  fprintf(out, "%s", format);
}

inline bool check_color_support(FILE *stream) {
#ifdef ENABLE_LOG_COLORS
  if (stream == stdout) {
    return isatty(1);
  } else if (stream == stderr) {
    return isatty(2);
  }
  return false;
#else
  return false;
#endif /* ENABLE_LOG_COLORS */
}

class ::conky::log::logger;
void format_detail(str_buffer &buffer, const ::conky::log::logger &logger_state,
                   const ::conky::log::msg_details &entry_state, detail detail);
}  // namespace _priv

const log_level DEFAULT_LOG_LEVEL = log_level::NOTICE;
class logger {
  const char *name;
  std::map<const char *, std::string> context;
  size_t context_length;

 public:
  struct sink {
    log_level level = DEFAULT_LOG_LEVEL;
    bool colorize = false;
    std::unordered_set<detail> details;

    sink(log_level level, bool colorize)
        : log_level(level), colorize(colorize) {}

    bool is_enabled(log_level level) const {
      return static_cast<int>(this->level) >= static_cast<int>(level);
    }
    void set_log_level(log_level level) { this->level = level; }
    void log_more() {
      this->level =
          static_cast<log_level>(std::min(static_cast<int>(this->level) + 1,
                                          static_cast<int>(log_level::TRACE)));
    }
    void log_less() {
      this->level =
          static_cast<log_level>(std::max(static_cast<int>(this->log_level) - 1,
                                          static_cast<int>(log_level::OFF)));
    }

    virtual FILE *get_stream() { return nullptr; }

    virtual void format_detail(const char *out, size_t max_length,
                               const msg_details &info, detail detail) {
      std::string formatted = _priv::format_detail(str_buffer(out, max_length),
                                                   *this, info, detail);
    }

    virtual void format_preamble() {}

    virtual void format_output() {}

    virtual void write(const msg_details &info, const char *message) = 0;
  };

#ifdef HAS_SYSLOG
  class syslog_sink : public sink {
   public:
    syslog_sink() : sink(log_level, false) {
      openlog(PACKAGE_NAME, LOG_PID, LOG_USER);
    }
    ~syslog_sink() { closelog(); }

    void write(const msg_details &info, const char *message) override {
      if (static_cast<int>(info.level) >=
              static_cast<int>(log_level::CRITICAL) &&
          static_cast<int>(info.level) < static_cast<int>(log_level::TRACE)) {
        syslog(static_cast<int>(info.level), "%s", message);
      }
    }
  };
#endif /* HAS_SYSLOG */

  class console_sink : public sink {
    FILE *stream;

   public:
    console_sink(FILE *stream, log_level level, bool colorize)
        : sink(level, colorize), stream(stream) {}

    FILE *get_stream() override { return stream; }

    void write(const msg_details &info, const char *message) override {
      fputs(message, stream);
    }
  };

  struct file_sink : public sink {
    fs::path file;
    FILE *stream;

   public:
    file_sink(fs::path file, log_level log_level) : sink(level, false) {
      this->stream = fopen(file.c_str(), "a+");
    }
    ~file_sink() { fclose(stream); }

    const fs::path &get_path() const { return this->file; }
    FILE *get_stream() override { return stream; }

    void write(const msg_details &info, const char *message) override {
      fputs(message, stream);
    }
  };

 private:
  std::vector<std::shared_ptr<sink>> sinks;

 public:
  struct context_guard {
    logger *parent;
    const char *key;

    context_guard(logger *parent, const char *key) : parent(parent), key(key) {
      if (parent->context_length > 0)
        parent->context_length++;  // delimiter (';')
      parent->context_length +=
          strlen(key) + 1 + parent->context.at(key).length();  // key=value
    }
    ~context_guard() {
      parent->context_length -=
          strlen(key) + 1 + parent->context.at(key).length();  // key=value
      if (parent->context_length > 0)
        parent->context_length--;  // delimiter (';')
      parent->context.erase(key);
    }

    std::string &value() const { return parent->context.at(key); }
  };

 public:
  logger(const char *name) : name(name) {
    this->sinks.emplace_back(console_sink{
        stderr,
        log_level::NOTICE,
        _priv::check_color_support(stderr),
    });

#ifdef HAS_SYSLOG
    this->sinks.emplace_back(syslog_sink());
#else
    this->sinks.emplace_back(file_sink{
        "/tmp/conky.log",
        level::DEBUG,
    });
#endif
  }

  std::shared_ptr<sink> add_log_file(const fs::path &path,
                                     log_level level = DEFAULT_LOG_LEVEL) {
    auto s = std::shared_ptr<sink>(new file_sink{path, level});
    this->sinks.emplace_back(s);
    return s;
  }

  std::shared_ptr<sink> get_stream_sink(FILE *stream) {
    for (const auto &s : this->sinks) {
      if (s->get_stream() == stream) { return s; }
    }
    return nullptr;
  }

  template <typename T>
  [[nodiscard]] volatile context_guard add_context(const char *key, T value) {
    static_assert(std::is_convertible_v<T, std::string>,
                  "can't convert context value to std::string");
    context.emplace(std::make_pair(key, std::string(value)));
    return context_guard(this, key);
  }

  template <log_level log_level, typename... Args>
  inline void log_print_fmt(msg_details &&details, const char *format,
                            Args &&...args) {
    // skip formatting if no targets will log
    if (!is_any_enabled(log_level)) return;

    static const size_t MAX_FILE_LEN = 32;
    static const size_t MAX_LOCATION_LEN = MAX_FILE_LEN + 1 + 5;  // name:line

#ifdef ENABLE_LOG_COLORS
    static const size_t MAX_COLOR_LEN = 16;
#else
    static const size_t MAX_COLOR_LEN = 0;
#endif

    static const size_t BASE_PREAMBLE_LENGTH =
        2 + TIME_LEN + 2 + MAX_LEVEL_NAME_LEN + 2 + MAX_LOCATION_LEN +
        3 * MAX_COLOR_LEN;

    size_t preamble_length = BASE_PREAMBLE_LENGTH;
    if (context_length > 0) {
      preamble_length += 2 + context_length;  // [context]
#ifdef ENABLE_LOG_COLORS
      preamble_length += COLOR_LEN;
#endif
    }

    // using a string here would require several intermediate buffers so it's
    // not really worth the convenience
    char preamble[preamble_length + 1];
    size_t offset = 0;

    template <typename F>
    const auto print_tag = inline[&](
        F formatter, std::optional<colors::color> color = std::nullopt) {
      preamble[offset] = '[';
      offset++;

#ifdef ENABLE_LOG_COLORS
      if (color.has_value()) {
        std::strncat(&preamble[offset], color.value, color.length);
        offset += color.length;
      }
#endif

      offset += formatter(offset);

#ifdef ENABLE_LOG_COLORS
      if (color.has_value()) {
        std::strncat(&preamble[offset], colors::RESET.value,
                     colors::RESET.length);
        offset += colors::RESET.length;
      }
#endif

      preamble[offset] = ']';
      offset++;
      preamble[offset] = '\0';
    };

    preamble[0] = '[';

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
      offset += snprintf(&preamble[offset], preamble_length - offset,
                         "[%s:%ld]", source.file, source.line);
    }

    if (context_length > 0) {
      std::string buffer;
      buffer.reserve(context_length);
      for (const auto &[key, value] : context) {
        if (!buffer.empty()) buffer += ";";
        buffer += key;
        buffer += '=';
        buffer += value;
      }
      offset += snprintf(&preamble[offset], preamble_length - offset, "[%s]",
                         buffer.c_str());
    }

    // localized output to console
    for (const auto &target : this->entries) {
      if (!target.is_enabled(log_level)) continue;
      fprintf(target.stream, "%s: ", preamble);
      _priv::_impl_fprintf(target.stream, _(format), args...);
      fputs("\n", target.stream);
    }
  }

  template <log_level level, typename... Args>
  void log_location(const char *file, size_t line, const char *format,
                    Args &&...args) {
    this->log_print_fmt<log_level>(
        msg_details{
            .level = level,
            .source = std::optional(source_details{file, line}),
        },
        format, args...);
  }

  template <log_level level, typename... Args>
  void log(const char *format, Args &&...args) {
    this->log_print_fmt<log_level>(
        msg_details{
            .level = level,
        },
        format, args...);
  }
};

}  // namespace conky::log

#endif /* _CONKY_LOGGER_H */
