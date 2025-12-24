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

#include "catch2/catch.hpp"

#ifdef __linux__
#include <conky.h>
#include <content/text_object.h>
#include <data/proc.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <lua/lua-config.hh>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

using namespace Catch::Matchers;

namespace {
void ensure_lua_state() {
  if (state) { return; }
  state = std::make_unique<lua::state>();
  conky::export_symbols(*state);
}

struct sub_text_object {
  struct text_object root {};
  struct text_object obj {};

  explicit sub_text_object(const char *text) {
    obj_be_plain_text(&obj, text);
    append_object(&root, &obj);
  }

  ~sub_text_object() { free(obj.data.s); }
};

struct thread_group {
  std::atomic<bool> running{true};
  std::vector<std::thread> threads;

  explicit thread_group(size_t count) {
    threads.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      threads.emplace_back([this]() {
        while (running.load()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
      });
    }
  }

  ~thread_group() {
    running = false;
    for (auto &thread : threads) {
      if (thread.joinable()) { thread.join(); }
    }
  }
};

struct proc_name_guard {
  char original[16] = {};
  bool ok = false;

  proc_name_guard() { ok = (prctl(PR_GET_NAME, original) == 0); }

  bool set(const char *name) const {
    return ok && (prctl(PR_SET_NAME, name) == 0);
  }

  ~proc_name_guard() {
    if (ok) { prctl(PR_SET_NAME, original); }
  }
};

std::string read_cmdline() {
  std::ifstream input("/proc/self/cmdline", std::ios::binary);
  std::string raw((std::istreambuf_iterator<char>(input)),
                  std::istreambuf_iterator<char>());
  for (char &ch : raw) {
    if (ch == '\0') { ch = ' '; }
  }
  while (!raw.empty() && raw.back() == ' ') { raw.pop_back(); }
  return raw;
}

bool parse_proc_stat_times(const std::string &stat, unsigned long int *utime,
                           unsigned long int *stime) {
  if (utime == nullptr || stime == nullptr) { return false; }
  size_t close = stat.rfind(')');
  if (close == std::string::npos) { return false; }
  size_t pos = close + 1;
  while (pos < stat.size() && stat[pos] == ' ') { ++pos; }
  const char *after = stat.c_str() + pos;
  int parsed =
      sscanf(after, "%*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
             utime, stime);
  return parsed == 2;
}
}  // namespace

TEST_CASE("cmdline_to_pid finds the current process",
          "[proc][cmdline_to_pid]") {
  std::string cmdline = read_cmdline();
  REQUIRE_FALSE(cmdline.empty());

  struct text_object obj {};
  obj.data.s = strdup(cmdline.c_str());

  char buf[32]{};
  print_cmdline_to_pid(&obj, buf, sizeof(buf));

  free(obj.data.s);

  REQUIRE(std::string(buf) == std::to_string(getpid()));
}

TEST_CASE("pid_time handles comm with spaces", "[proc][pid_time]") {
  ensure_lua_state();

  proc_name_guard name_guard;
  REQUIRE(name_guard.ok);
  REQUIRE(name_guard.set("conky test"));

  std::ifstream input("/proc/self/stat", std::ios::binary);
  std::string stat((std::istreambuf_iterator<char>(input)),
                   std::istreambuf_iterator<char>());
  REQUIRE_FALSE(stat.empty());

  unsigned long int utime = 0;
  unsigned long int stime = 0;
  REQUIRE(parse_proc_stat_times(stat, &utime, &stime));

  double expected = static_cast<double>(utime + stime) / 100.0;

  std::string pid_str = std::to_string(getpid());
  sub_text_object sub(pid_str.c_str());
  struct text_object obj {};
  obj.sub = &sub.root;

  char buf[64]{};
  print_pid_time(&obj, buf, sizeof(buf));

  double actual = std::stod(buf);
  REQUIRE_THAT(actual, WithinAbs(expected, 0.01));
}

TEST_CASE("pid_thread_list does not overflow small buffers",
          "[proc][pid_thread_list]") {
  ensure_lua_state();

  thread_group group(4);

  std::string pid_str = std::to_string(getpid());
  sub_text_object sub(pid_str.c_str());
  struct text_object obj {};
  obj.sub = &sub.root;

  constexpr size_t k_buf_size = 8;
  constexpr char k_sentinel = 'Z';
  std::array<char, k_buf_size + 4> buffer{};
  buffer.fill('X');
  for (size_t i = k_buf_size; i < buffer.size(); ++i) {
    buffer[i] = k_sentinel;
  }

  print_pid_thread_list(&obj, buffer.data(), k_buf_size);

  REQUIRE(memchr(buffer.data(), '\0', k_buf_size) != nullptr);
  for (size_t i = k_buf_size; i < buffer.size(); ++i) {
    REQUIRE(buffer[i] == k_sentinel);
  }
}
#endif
