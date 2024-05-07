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

#include "logger.hh"

#include <string>

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

extern conky::log::logger DEFAULT_LOGGER;

#define LOG(Level, ...)                                              \
  DEFAULT_LOGGER.log_location<::conky::log::level::Level>(           \
      ::conky::log::_priv::relative_source_path(__FILE__), __LINE__, \
      __VA_ARGS__)

#define LOG_CRITICAL(...) LOG(CRITICAL, __VA_ARGS__)
#define LOG_ERROR(...) LOG(ERROR, __VA_ARGS__)
#define LOG_WARNING(...) LOG(WARNING, __VA_ARGS__)
#define LOG_NOTICE(...) LOG(NOTICE, __VA_ARGS__)
#define LOG_INFO(...) LOG(INFO, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(DEBUG, __VA_ARGS__)
#define LOG_TRACE(...) LOG(TRACE, __VA_ARGS__)

#define LOG_CONTEXT(Key, Value)                                   \
  volatile ::conky::log::logger::context_guard _log_guard_##Key = \
      DEFAULT_LOGGER.add_context(#Key, Value)

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

namespace conky {
namespace _priv_error_print {
template <typename... Args>
inline std::string alloc_printf(const char *format, Args &&...args) {
  auto size = std::snprintf(nullptr, 0, format, args...);
  char output[size + 1];
  std::snprintf(output, sizeof(output), format, args...);
  return std::string(&output[0]);
}
inline std::string alloc_printf(const char *format) {
  return std::string(format);
}
}  // namespace _priv_error_print

struct error : public std::runtime_error {
  /// @brief Construct a new error.
  ///
  /// @param msg already localized error message.
  error(const std::string &msg) : std::runtime_error(msg) {}
  /// @brief Construct a new error.
  ///
  /// @param msg unlocalized error message.
  error(const char *msg) : std::runtime_error(_(msg)) {}
  /// @brief Construct a new error.
  ///
  /// @param format unlocalized format of error message.
  /// @param args format arguments.
  template <typename... Args>
  error(const char *format, Args &&...args)
      : error(_priv_error_print::alloc_printf(_(format), args...)) {}
};

struct bad_command_arguments_error : public std::runtime_error {
  std::string command;

  /// @brief Construct a new error.
  ///
  /// @param command command with invalid arguments.
  /// @param msg already localized error message.
  bad_command_arguments_error(const char *command, const std::string &msg)
      : std::runtime_error(msg), command(command) {}
  /// @brief Construct a new error.
  ///
  /// @param command command with invalid arguments.
  /// @param msg unlocalized error message.
  bad_command_arguments_error(const char *command, const char *msg)
      : std::runtime_error(_(msg)), command(std::string(command)) {}
  /// @brief Construct a new error.
  ///
  /// @param command command with invalid arguments.
  /// @param format unlocalized format of error message.
  /// @param args format arguments.
  template <typename... Args>
  bad_command_arguments_error(const char *command, const char *format,
                              Args &&...args)
      : bad_command_arguments_error(
            _priv_error_print::alloc_printf(_(format), args...)),
        command(std::string(command)) {}
};
}  // namespace conky

/// @brief Error that warrants termination of the program, but is caused by user
/// error (e.g. bad input) and as such a core dump isn't useful.
///
/// @param Format printf style format string.
/// @param Args printf style arguments.
#define USER_ERR(...)     \
  LOG_ERROR(__VA_ARGS__); \
  throw conky::error(__VA_ARGS__)

/// @brief Error caused by system not supporting some required conky feature.
///
/// We assume the fault is with user for not properly configuring conky so we
/// don't produce a core dump.
///
/// @param Format printf style format string.
/// @param Args printf style arguments.
#define SYSTEM_ERR(...)   \
  LOG_ERROR(__VA_ARGS__); \
  throw conky::error(__VA_ARGS__)

/// @brief Invalid command arguments error.
///
/// @param Command command with invalid arguments.
/// @param Format printf style format string.
/// @param Args printf style arguments.
#define COMMAND_ARG_ERR(Command, ...) \
  LOG_ERROR(__VA_ARGS__);             \
  throw conky::bad_command_arguments_error(Command, __VA_ARGS__)

/* critical error with additional cleanup */
#define CRIT_ERR_FREE(memtofree1, memtofree2, ...) \
  free(memtofree1);                                \
  free(memtofree2);                                \
  SYSTEM_ERR(__VA_ARGS__);

#endif /* _LOGGING_H */
