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
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <lua/lua-config.hh>

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "test_variable.hh"

using namespace Catch::Matchers;

namespace {
void ensure_lua_state() {
  if (state) { return; }
  state = std::make_unique<lua::state>();
  conky::export_symbols(*state);
}

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

std::string read_status_value_for_pid(pid_t pid, const std::string &key) {
  std::string path = "/proc/" + std::to_string(pid) + "/status";
  std::ifstream input(path);
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

std::string read_status_value(const std::string &key) {
  return read_status_value_for_pid(getpid(), key);
}

pid_t spawn_stopped_child() {
  pid_t pid = fork();
  if (pid == 0) {
    raise(SIGSTOP);
    _exit(0);
  }
  return pid;
}

struct child_guard {
  pid_t pid = -1;

  ~child_guard() {
    if (pid > 0) {
      kill(pid, SIGKILL);
      int status = 0;
      waitpid(pid, &status, 0);
    }
  }
};

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
  ensure_lua_state();

  std::string cmdline = read_cmdline();
  REQUIRE_FALSE(cmdline.empty());

  test_variable var("cmdline_to_pid", cmdline.c_str());
  REQUIRE(var);

  REQUIRE(var.print() == std::to_string(getpid()));
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
  test_variable var("pid_time", pid_str.c_str());
  REQUIRE(var);

  double actual = std::stod(var.print());
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
  test_variable var("pid_time_kernelmode", pid_str.c_str());
  REQUIRE(var);

  double actual = std::stod(var.print());
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
  test_variable var("pid_time_usermode", pid_str.c_str());
  REQUIRE(var);

  double actual = std::stod(var.print());
  REQUIRE_THAT(actual, WithinAbs(expected, 0.01));
}

TEST_CASE("pid_thread_list does not overflow small buffers",
          "[proc][pid_thread_list]") {
  ensure_lua_state();

  thread_group group(4);

  std::string pid_str = std::to_string(getpid());
  test_variable var("pid_thread_list", pid_str.c_str());
  REQUIRE(var);

  constexpr unsigned int k_buf_size = 8;
  constexpr char k_sentinel = 'Z';
  std::array<char, k_buf_size + 4> buffer{};
  buffer.fill('X');
  for (size_t i = k_buf_size; i < buffer.size(); ++i) {
    buffer[i] = k_sentinel;
  }

  var.obj->callbacks.print(var.obj, buffer.data(), k_buf_size);

  REQUIRE(memchr(buffer.data(), '\0', k_buf_size) != nullptr);
  for (size_t i = k_buf_size; i < buffer.size(); ++i) {
    REQUIRE(buffer[i] == k_sentinel);
  }
}

TEST_CASE("pid_environ reads values from /proc environ",
          "[proc][pid_environ]") {
  ensure_lua_state();

  const char *expected = getenv("PATH");
  REQUIRE(expected != nullptr);

  std::string pid_str = std::to_string(getpid());
  std::string arg = pid_str + " PATH";
  test_variable var("pid_environ", arg.c_str());
  REQUIRE(var);

  std::string result = var.print(strlen(expected) + 1);
  REQUIRE(result == expected);
}

TEST_CASE("pid_state_short returns the short state",
          "[proc][pid_state_short]") {
  ensure_lua_state();

  std::string pstate = read_status_value("State:");
  REQUIRE_FALSE(pstate.empty());

  std::string pid_str = std::to_string(getpid());
  test_variable var("pid_state_short", pid_str.c_str());
  REQUIRE(var);

  std::string result = var.print();
  REQUIRE(result[0] == pstate[0]);
}

TEST_CASE("pid_vm values map to correct status entries", "[proc][pid_vm]") {
  ensure_lua_state();

  pid_t child = spawn_stopped_child();
  REQUIRE(child > 0);
  int status = 0;
  REQUIRE(waitpid(child, &status, WUNTRACED) == child);
  REQUIRE(WIFSTOPPED(status));

  child_guard guard;
  guard.pid = child;

  std::string vmrss = read_status_value_for_pid(child, "VmRSS:");
  std::string vmstk = read_status_value_for_pid(child, "VmStk:");
  std::string vmexe = read_status_value_for_pid(child, "VmExe:");
  REQUIRE_FALSE(vmrss.empty());
  REQUIRE_FALSE(vmstk.empty());
  REQUIRE_FALSE(vmexe.empty());

  std::string pid_str = std::to_string(child);

  test_variable var_rss("pid_vmrss", pid_str.c_str());
  REQUIRE(var_rss);
  REQUIRE(var_rss.print() == vmrss);

  test_variable var_stk("pid_vmstk", pid_str.c_str());
  REQUIRE(var_stk);
  REQUIRE(var_stk.print() == vmstk);

  test_variable var_exe("pid_vmexe", pid_str.c_str());
  REQUIRE(var_exe);
  REQUIRE(var_exe.print() == vmexe);
}
#endif
