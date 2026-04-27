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
}  // namespace conky::log

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
