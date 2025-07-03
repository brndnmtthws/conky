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

/// This file contains functions that are useful to most target platforms and
/// behave similarly across them.
/// It also has some non-specific functions that extend C++ standard library.

#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "content/text_object.h"
#include "logging.h"
#include "lua/setting.hh"

char *readfile(const char *filename, int *total_read, char showerror);

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

/// @brief Iterator that handles interation over `D` delimited segments of a
/// string.
///
/// Iterated sequence can be any type that can be turned into std::string_view.
/// `D` can be one of many delimiter elements - a char or a substring.
///
/// If `Yield` is `std::string_view` string will not be copied, but
/// `std::string_view` isn't null terminated. If `Yield` is `std::string` then
/// the string will be copied and will be null terminated (compatible with C
/// APIs).
template <typename D, typename Yield = std::string_view>
class SplitIterator {
  std::string_view base;
  D delimiter;
  std::string_view::size_type delimiter_length;
  std::string_view::size_type pos;
  std::string_view item;

 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = Yield;

  SplitIterator(std::string_view str, D delimiter, size_t start = 0)
      : base(str), delimiter(delimiter), pos(start) {
    // Precompute delimiter length based on `D` type.
    if constexpr (std::is_same_v<D, char>) {
      delimiter_length = 1;
    } else {
      delimiter_length =
          static_cast<std::string_view::size_type>(delimiter.size());
      if (delimiter_length == 0) {
        CRIT_ERR("SplitIterator provided with an empty delimiter for string %s",
                 base);
      }
    }
    ++(*this);  // Load first entry
  }

  Yield operator*() const {
    if constexpr (std::is_same_v<Yield, std::string_view>) {
      return item;
    } else {
      return Yield(item);
    }
  }

  inline SplitIterator &operator++() { return next(); }
  inline SplitIterator &operator++(int) { return next(); }

  bool operator==(const SplitIterator &other) const {
    return base == other.base && pos == other.pos;
  }

  bool operator!=(const SplitIterator &other) const {
    return base != other.base || pos != other.pos;
  }

 private:
  SplitIterator &next() {
    if (pos == std::string_view::npos) { return *this; }

    size_t next = base.find(delimiter, pos);
    if (next == std::string::npos) {
      item = base.substr(pos);
      pos = std::string_view::npos;
    } else {
      item = base.substr(pos, next - pos);
      pos = next + delimiter_length;
    }
    return *this;
  }
};

/// SplitIterator helper class, see SplitIterator for details.
template <typename D, typename Yield = std::string_view>
class SplitIterable {
  std::string_view base;
  D delimiter;

 public:
  SplitIterable(std::string_view base, D delim)
      : base(base), delimiter(delim) {}

  auto begin() const { return SplitIterator<D, Yield>(base, delimiter); }
  auto end() const {
    return SplitIterator<D, Yield>(base, delimiter, std::string_view::npos);
  }
};

/// Splits a string like type `S` with a delimiter `D`, producing entries of
/// type `Yield` (`std::string_view` by default).
template <typename S, typename D, typename Yield = std::string_view>
inline SplitIterable<D, Yield> split(const S &string, const D &delimiter) {
  return SplitIterable<D, Yield>(std::string_view(string), delimiter);
}

/// Splits a string like type `S` with a delimiter `D`, allocating a
/// `std::vector` with entries of type `Yield`.
template <typename S, typename D, typename Yield = std::string_view>
inline std::vector<Yield> split_to_vec(const S &string, const D &delimiter) {
  std::vector<Yield> result;
  for (const auto yield : split<S, D, Yield>(string, delimiter)) {
    result.push_back(yield);
  }
  return result;
}

/// @brief Handles environment variable expansion in paths and canonicalization.
///
/// This is a simplified reimplementation of `wordexp` function that's cross
/// platform.
/// Path expansion is done in following stages:
/// - Tilde expansion (e.g. `~/file` -> `/home/user/file`)
/// - Variable substitution (e.g. `$XDG_CONFIG_DIRS/file` -> `/etc/xdg/file`)
/// - Command substitution (e.g. `path/$(echo "part")` -> `path/part`)
/// - Wildcard expansion (e.g. `/some/**/path` -> `/some/deeply/nested/path`)
///
/// In case wildcard expansion produces multiple matches, first one will be
/// returned, but ordering depends on directory listing order (of filesystem
//  and/or OS).
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
/// @brief Performs variable substitution on a string and returns the result.
///
/// Example:
/// - `$HOME/.config/conky` -> `/home/user/.config/conky`
std::string variable_substitute(std::string s);
/// @brief Performs command substitution on a string and returns the result.
///
/// Examples:
/// - `/some/$(echo "path")` -> `/some/path`
/// - `/nested/$(echo $((2 + 3)))` -> `/nested/5`
std::string command_substitute(std::string s);
/// @brief Performs wildcard expansion of a path.
///
/// This function will
///
/// Following characters are supported:
/// - ? (question mark) - a single character
/// - * (asterisk) - zero or more characters
/// - ** (double asterisk) - zero or more subpaths
std::vector<std::filesystem::path> wildcard_expand_path(
    const std::filesystem::path &path);

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

void scan_no_update(struct text_object *, const char *);
void print_no_update(struct text_object *, char *, unsigned int);
void free_no_update(struct text_object *);

void scan_loadavg_arg(struct text_object *, const char *);
void print_loadavg(struct text_object *, char *, unsigned int);
#ifdef BUILD_GUI
void scan_loadgraph_arg(struct text_object *, const char *);
double loadgraphval(struct text_object *);
#endif /* BUILD_GUI */

uint8_t cpu_percentage(struct text_object *);
double cpu_barval(struct text_object *);

void print_mem(struct text_object *, char *, unsigned int);
void print_memwithbuffers(struct text_object *, char *, unsigned int);
void print_memeasyfree(struct text_object *, char *, unsigned int);
void print_legacymem(struct text_object *, char *, unsigned int);
void print_memfree(struct text_object *, char *, unsigned int);
void print_memmax(struct text_object *, char *, unsigned int);
void print_memactive(struct text_object *, char *, unsigned int);
void print_meminactive(struct text_object *, char *, unsigned int);
void print_memwired(struct text_object *, char *, unsigned int);
void print_memlaundry(struct text_object *, char *, unsigned int);
void print_memdirty(struct text_object *, char *, unsigned int);
void print_shmem(struct text_object *, char *, unsigned int);
void print_memavail(struct text_object *, char *, unsigned int);
void print_swap(struct text_object *, char *, unsigned int);
void print_swapfree(struct text_object *, char *, unsigned int);
void print_swapmax(struct text_object *, char *, unsigned int);
uint8_t mem_percentage(struct text_object *);
double mem_barval(struct text_object *);
double mem_with_buffers_barval(struct text_object *);
uint8_t swap_percentage(struct text_object *);
double swap_barval(struct text_object *);

void print_kernel(struct text_object *, char *, unsigned int);
void print_machine(struct text_object *, char *, unsigned int);
void print_nodename(struct text_object *, char *, unsigned int);
void print_nodename_short(struct text_object *, char *, unsigned int);
void print_sysname(struct text_object *, char *, unsigned int);

#if defined(__DragonFly__)
void print_version(struct text_object *obj, char *p, unsigned int p_max_size);
#endif

void print_uptime(struct text_object *, char *, unsigned int);
void print_uptime_short(struct text_object *, char *, unsigned int);

void print_processes(struct text_object *, char *, unsigned int);
void print_running_processes(struct text_object *, char *, unsigned int);
void print_running_threads(struct text_object *, char *, unsigned int);
void print_threads(struct text_object *, char *, unsigned int);

void print_buffers(struct text_object *, char *, unsigned int);
void print_cached(struct text_object *, char *, unsigned int);
void print_free_bufcache(struct text_object *, char *, unsigned int);
void print_free_cached(struct text_object *, char *, unsigned int);

void print_evaluate(struct text_object *, char *, unsigned int);

int if_empty_iftest(struct text_object *);
int if_existing_iftest(struct text_object *);
int if_running_iftest(struct text_object *);

#ifndef __OpenBSD__
void print_acpitemp(struct text_object *, char *, unsigned int);
void free_acpitemp(struct text_object *);
#endif /* !__OpenBSD__ */

void print_freq(struct text_object *, char *, unsigned int);
void print_freq_g(struct text_object *, char *, unsigned int);

#ifndef __OpenBSD__
void print_acpifan(struct text_object *, char *, unsigned int);
void print_acpiacadapter(struct text_object *, char *, unsigned int);
void print_battery(struct text_object *, char *, unsigned int);
void print_battery_time(struct text_object *, char *, unsigned int);
uint8_t battery_percentage(struct text_object *);
void battery_power_draw(struct text_object *, char *, unsigned int);
void print_battery_short(struct text_object *, char *, unsigned int);
void print_battery_status(struct text_object *, char *, unsigned int);
#endif /* !__OpenBSD__ */

void free_cpu(struct text_object *);

void print_blink(struct text_object *, char *, unsigned int);
void print_include(struct text_object *, char *, unsigned int);

void print_updates(struct text_object *, char *, unsigned int);
int updatenr_iftest(struct text_object *);

#ifdef BUILD_CURL
void print_github(struct text_object *, char *, unsigned int);
void print_stock(struct text_object *, char *, unsigned int);
void free_stock(struct text_object *);
#endif /* BUILD_CURL */

#endif /* _COMMON_H */
