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

#include <stdexcept>
#include "config.h"
#include "i18n.h"

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <filesystem>

template <>
struct fmt::formatter<std::filesystem::path> : fmt::formatter<std::string> {
  auto format(const std::filesystem::path &p, fmt::format_context &ctx) const {
    return fmt::formatter<std::string>::format(p.string(), ctx);
  }
};

namespace conky {

class error : public std::runtime_error {
 public:
  error(const std::string &msg) : std::runtime_error(msg) {}
};

struct bad_command_arguments_error : public std::runtime_error {
  std::string command;

  bad_command_arguments_error(const char *command, const std::string &msg)
      : std::runtime_error(msg), command(command) {}
};
}  // namespace conky

namespace conky::log {
void init_logger();
void log_more();
void log_less();
void set_quiet();

struct attribute {
  std::string key;
  std::string value;

  template <typename T,
            typename = std::enable_if_t<fmt::is_formattable<T>::value>>
  attribute(std::string_view k, const T &v)
      : key(k), value(fmt::format("{}", v)) {}
};

using attribute_list = std::vector<attribute>;

class span {
  std::string m_name;

 public:
  explicit span(std::string name) : m_name(std::move(name)) {}

  const std::string &name() const { return m_name; }
};

class span_guard {
  bool m_active = false;

 public:
  span_guard() = default;
  span_guard(span_guard &&other) noexcept : m_active(other.m_active) {
    other.m_active = false;
  }
  span_guard(const span_guard &) = delete;
  span_guard &operator=(const span_guard &) = delete;
  span_guard &operator=(span_guard &&) = delete;
  ~span_guard();

  /// Activate this span guard, pushing a span onto the thread-local stack.
  void open(spdlog::source_loc loc, std::string name,
            std::initializer_list<attribute> attrs = {});

  /// Explicitly end this span before scope exit.
  void drop();
};

/// Returns the current span context formatted for log output.
std::string current_span_context();

/// Captures the current thread's span stack for cross-thread propagation.
std::vector<span> capture_context();

/// Installs a captured span stack into the current thread.
void install_context(const std::vector<span> &ctx);

/// Push per-message attributes (accumulated, cleared after log call).
void push_msg_attrs(std::initializer_list<attribute> attrs);
/// Clear all accumulated per-message attributes
void clear_msg_attrs();

}  // namespace conky::log

#define LOG_SCOPE(name, ...)                                            \
  ([&]() -> conky::log::span_guard {                                    \
    conky::log::span_guard _guard;                                      \
    if (spdlog::default_logger()->should_log(spdlog::level::debug))     \
      _guard.open(                                                      \
          spdlog::source_loc{__FILE__, __LINE__, __func__},             \
          name, ##__VA_ARGS__);                                         \
    return _guard;                                                      \
  }())

// syslog.h defines LOG_DEBUG, LOG_INFO, LOG_WARNING etc. as integers
#undef LOG_TRACE
#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_WARNING
#undef LOG_ERROR
#undef LOG_CRITICAL

#define LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARNING(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

#define _LOG_STRIP_PARENS(...) __VA_ARGS__
#define _LOG_MESSAGE_LEVEL_WITH_ATTRIBUTES_IMPLEMENTATION(spdlog_macro, attrs, ...) \
  do {                                                  \
    if (spdlog::default_logger()->should_log(           \
            spdlog::level::trace)) {                    \
      conky::log::push_msg_attrs({_LOG_STRIP_PARENS attrs}); \
      spdlog_macro(__VA_ARGS__);                        \
      conky::log::clear_msg_attrs();                    \
    } else {                                            \
      spdlog_macro(__VA_ARGS__);                        \
    }                                                   \
  } while (0)

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_TRACE_WITH(attrs, ...) _LOG_MESSAGE_LEVEL_WITH_ATTRIBUTES_IMPLEMENTATION(SPDLOG_TRACE, attrs, __VA_ARGS__)
#else
#define LOG_TRACE_WITH(attrs, ...) ((void)0)
#endif
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_DEBUG_WITH(attrs, ...) _LOG_MESSAGE_LEVEL_WITH_ATTRIBUTES_IMPLEMENTATION(SPDLOG_DEBUG, attrs, __VA_ARGS__)
#else
#define LOG_DEBUG_WITH(attrs, ...) ((void)0)
#endif
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_INFO_WITH(attrs, ...) _LOG_MESSAGE_LEVEL_WITH_ATTRIBUTES_IMPLEMENTATION(SPDLOG_INFO, attrs, __VA_ARGS__)
#else
#define LOG_INFO_WITH(attrs, ...) ((void)0)
#endif
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_WARNING_WITH(attrs, ...) _LOG_MESSAGE_LEVEL_WITH_ATTRIBUTES_IMPLEMENTATION(SPDLOG_WARN, attrs, __VA_ARGS__)
#else
#define LOG_WARNING_WITH(attrs, ...) ((void)0)
#endif
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_ERROR_WITH(attrs, ...) _LOG_MESSAGE_LEVEL_WITH_ATTRIBUTES_IMPLEMENTATION(SPDLOG_ERROR, attrs, __VA_ARGS__)
#else
#define LOG_ERROR_WITH(attrs, ...) ((void)0)
#endif
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define LOG_CRITICAL_WITH(attrs, ...) _LOG_MESSAGE_LEVEL_WITH_ATTRIBUTES_IMPLEMENTATION(SPDLOG_CRITICAL, attrs, __VA_ARGS__)
#else
#define LOG_CRITICAL_WITH(attrs, ...) ((void)0)
#endif

/// Critical error (developer fault) - logs and terminates with core dump.
#define CRIT_ERR(...)          \
  do {                         \
    LOG_CRITICAL(__VA_ARGS__); \
    std::terminate();          \
  } while (0)

/// User error (bad input/config) - logs and throws.
#define USER_ERR(...)                                    \
  do {                                                   \
    LOG_ERROR(__VA_ARGS__);                              \
    throw conky::error(fmt::format(__VA_ARGS__));        \
  } while (0)

/// System error (missing feature/support) - logs and throws.
#define SYSTEM_ERR(...)                                  \
  do {                                                   \
    LOG_ERROR(__VA_ARGS__);                              \
    throw conky::error(fmt::format(__VA_ARGS__));        \
  } while (0)

/// Invalid command arguments - logs and throws.
#define COMMAND_ARG_ERR(Command, ...)                                    \
  do {                                                                   \
    LOG_ERROR(__VA_ARGS__);                                              \
    throw conky::bad_command_arguments_error(Command,                    \
                                             fmt::format(__VA_ARGS__));  \
  } while (0)

#endif /* _LOGGING_H */
