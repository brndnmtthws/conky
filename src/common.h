/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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

#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <filesystem>
#include <optional>
#include <string>

#include "content/text_object.h"
#include "lua/setting.hh"

/// Resolve a device path and strip the "/dev/" prefix.
inline std::string dev_name(const char *path) {
  if (path == nullptr) { return {}; }
  char resolved[PATH_MAX];
  const char *p = realpath(path, resolved) != nullptr ? resolved : path;
  constexpr std::string_view prefix = "/dev/";
  if (std::string_view(p).substr(0, prefix.size()) == prefix) {
    return p + prefix.size();
  }
  return p;
}

void print_to_bytes(struct text_object *, char *, unsigned int);

void strfold(char *start, int count);
int check_mount(struct text_object *);
void prepare_update(void);
int update_uptime(void);
int update_meminfo(void);
int update_net_stats(void);
int update_cpu_usage(void);
int update_total_processes(void);
int update_uname(void);
int update_threads(void);
int update_running_processes(void);
void update_stuff(void);
char get_freq(char *, size_t, const char *, int, unsigned int);
void print_voltage_mv(struct text_object *, char *, unsigned int);
void print_voltage_v(struct text_object *, char *, unsigned int);
int update_load_average(void);
void free_all_processes(void);
struct process *get_first_process(void);
void get_cpu_count(void);
double get_time(void);

/// @brief Handles environment variable expansion in paths and canonicalization.
///
/// Examples:
/// - `~/conky` -> `/home/conky_user/conky`
/// - `$HOME/conky` -> `/home/conky_user/conky`
/// - `$HOME/a/b/../c/../../conky` -> `/home/conky_user/conky`
std::filesystem::path to_real_path(const std::string &source);
FILE *open_file(const char *file, int *reported);
int open_fifo(const char *file, int *reported);

/// Returns current working directory of conky.
std::filesystem::path get_cwd();
/// Returns the username of user/account that started conky.
std::string current_username();
/// Returns home directory of user with `username`.
std::optional<std::filesystem::path> user_home(const std::string &username);
/// Returns home directory of user with `current_username`.
std::optional<std::filesystem::path> user_home();
/// Performs tilde expansion on a path-like string and returns the result.
std::string tilde_expand(const std::string &unexpanded);
/// Performs variable substitution on a string and returns the result.
std::string variable_substitute(std::string s);

void format_seconds(char *buf, unsigned int n, long seconds);
void format_seconds_short(char *buf, unsigned int n, long seconds);

int round_to_int_temp(float);

unsigned int round_to_positive_int(float);

extern conky::simple_config_setting<bool> no_buffers;
extern conky::simple_config_setting<std::string> bar_fill;
extern conky::simple_config_setting<std::string> bar_unfill;
extern conky::simple_config_setting<std::string> github_token;

int open_acpi_temperature(const char *name);
double get_acpi_temperature(int fd);
void get_acpi_ac_adapter(char *p_client_buffer, size_t client_buffer_size,
                         const char *adapter);
void get_acpi_fan(char *, size_t);
void get_battery_stuff(char *buf, unsigned int n, const char *bat, int item);
int get_battery_perct(const char *bat);
void get_battery_power_draw(char *buffer, unsigned int n, const char *bat);
double get_battery_perct_bar(struct text_object *);
void get_battery_short_status(char *buf, unsigned int n, const char *bat);

uint8_t cpu_percentage(struct text_object *);
double cpu_barval(struct text_object *);

/// Memory/swap state queries — used by variable lambdas and tests.
uint8_t mem_percentage();
double mem_barval();
double mem_with_buffers_barval();
uint8_t swap_percentage();
double swap_barval();

void free_cpu(struct text_object *);

#ifdef BUILD_CURL
std::string github_notifications_url();
std::string github_authorization_header(const std::string &token);
#endif /* BUILD_CURL */

#endif /* _COMMON_H */
