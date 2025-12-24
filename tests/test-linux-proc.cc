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

std::string read_status_value(const std::string &key) {
  std::ifstream input("/proc/self/status");
  std::string line;
  while (std::getline(input, line)) {
    if (line.rfind(key, 0) == 0) {
      std::string value = line.substr(key.size());
      while (!value.empty() &&
             (value.front() == '\t' || value.front() == ' ')) {
        value.erase(value.begin());
      }
      return value;
    }
  }
  return {};
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

TEST_CASE("pid_time_kernelmode uses system time",
          "[proc][pid_time_kernelmode]") {
  ensure_lua_state();

  std::ifstream input("/proc/self/stat", std::ios::binary);
  std::string stat((std::istreambuf_iterator<char>(input)),
                   std::istreambuf_iterator<char>());
  REQUIRE_FALSE(stat.empty());

  unsigned long int utime = 0;
  unsigned long int stime = 0;
  REQUIRE(parse_proc_stat_times(stat, &utime, &stime));

  double expected = static_cast<double>(stime) / 100.0;

  std::string pid_str = std::to_string(getpid());
  sub_text_object sub(pid_str.c_str());
  struct text_object obj {};
  obj.sub = &sub.root;

  char buf[64]{};
  print_pid_time_kernelmode(&obj, buf, sizeof(buf));

  double actual = std::stod(buf);
  REQUIRE_THAT(actual, WithinAbs(expected, 0.01));
}

TEST_CASE("pid_time_usermode uses user time", "[proc][pid_time_usermode]") {
  ensure_lua_state();

  std::ifstream input("/proc/self/stat", std::ios::binary);
  std::string stat((std::istreambuf_iterator<char>(input)),
                   std::istreambuf_iterator<char>());
  REQUIRE_FALSE(stat.empty());

  unsigned long int utime = 0;
  unsigned long int stime = 0;
  REQUIRE(parse_proc_stat_times(stat, &utime, &stime));

  double expected = static_cast<double>(utime) / 100.0;

  std::string pid_str = std::to_string(getpid());
  sub_text_object sub(pid_str.c_str());
  struct text_object obj {};
  obj.sub = &sub.root;

  char buf[64]{};
  print_pid_time_usermode(&obj, buf, sizeof(buf));

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

TEST_CASE("pid_environ reads values from /proc environ",
          "[proc][pid_environ]") {
  ensure_lua_state();

  setenv("CONKY_PROC_TEST", "ok", 1);

  std::string pid_str = std::to_string(getpid());
  std::string arg = pid_str + " conky_proc_test";
  sub_text_object sub(arg.c_str());
  struct text_object obj {};
  obj.sub = &sub.root;
  obj.data.s = strdup("conky_proc_test");

  char buf[64]{};
  print_pid_environ(&obj, buf, sizeof(buf));

  free(obj.data.s);

  REQUIRE(std::string(buf) == "ok");
}

TEST_CASE("pid_state_short returns the short state",
          "[proc][pid_state_short]") {
  ensure_lua_state();

  std::string state = read_status_value("State:");
  REQUIRE_FALSE(state.empty());

  std::string pid_str = std::to_string(getpid());
  sub_text_object sub(pid_str.c_str());
  struct text_object obj {};
  obj.sub = &sub.root;

  char buf[8]{};
  print_pid_state_short(&obj, buf, sizeof(buf));

  REQUIRE(buf[0] == state[0]);
}

TEST_CASE("pid_vm values map to correct status entries", "[proc][pid_vm]") {
  ensure_lua_state();

  std::string vmrss = read_status_value("VmRSS:");
  std::string vmstk = read_status_value("VmStk:");
  std::string vmexe = read_status_value("VmExe:");
  REQUIRE_FALSE(vmrss.empty());
  REQUIRE_FALSE(vmstk.empty());
  REQUIRE_FALSE(vmexe.empty());

  std::string pid_str = std::to_string(getpid());
  sub_text_object sub(pid_str.c_str());
  struct text_object obj {};
  obj.sub = &sub.root;

  char buf[64]{};
  print_pid_vmrss(&obj, buf, sizeof(buf));
  REQUIRE(std::string(buf) == vmrss);

  memset(buf, 0, sizeof(buf));
  print_pid_vmstk(&obj, buf, sizeof(buf));
  REQUIRE(std::string(buf) == vmstk);

  memset(buf, 0, sizeof(buf));
  print_pid_vmexe(&obj, buf, sizeof(buf));
  REQUIRE(std::string(buf) == vmexe);
}
#endif
