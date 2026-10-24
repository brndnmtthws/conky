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

#include <memory>
#include <string>
#include <utility>

#include <data/fs.h>
#include <lua/llua.h>
#include <lua/lua-config.hh>
#include <lua/setting.hh>

TEST_CASE("fs_free_percentage returns correct value") {
  struct text_object obj;

  SECTION("for fs size == 0") {
    fs_stat *fs = new fs_stat;
    fs->size = 0;
    fs->avail = 17;
    fs->free = 97;

    obj.data.opaque = fs;

    REQUIRE(fs_free_percentage(&obj) == 0);

    delete fs;
  }

  SECTION("for valid data") {
    fs_stat *fs = new fs_stat;
    fs->size = 68;
    fs->avail = 17;
    fs->free = 97;

    obj.data.opaque = fs;

    REQUIRE(fs_free_percentage(&obj) == 25);

    delete fs;
  }
}

extern lua_State *lua_L;

namespace {
struct fs_test_context {
  std::unique_ptr<lua::state> saved_state = std::move(state);
  double saved_update_time = current_update_time;
  lua_State *saved_script_state = lua_L;
  bool settings_initialized = false;

  fs_test_context() {
    clear_fs_stats();
    current_update_time = 0.0;
    state = std::make_unique<lua::state>();
    conky::export_symbols(*state);
  }

  ~fs_test_context() {
    clear_fs_stats();
    if (settings_initialized) {
      conky::cleanup_config_settings(*state);
      lua_L = saved_script_state;
    }
    state = std::move(saved_state);
    current_update_time = saved_update_time;
  }

  void configure(const std::string &interval, bool validate = false) {
    // Disable graphical output so validation never requires a display server.
    std::string config = "conky.config = {output_backend = 'console'";
    if (!interval.empty()) { config += ", fs_update_interval = " + interval; }
    config += "}";
    state->loadstring(config.c_str());
    state->call(0, 0);
    if (validate) {
      // Full config cleanup closes the separate Lua scripting state. Give
      // this fixture its own, leaving any previous test's state untouched.
      lua_L = nullptr;
      llua_init();
      conky::set_config_settings(*state);
      settings_initialized = true;
    }
  }

  void update(double time) {
    current_update_time = time;
    REQUIRE(update_fs_stats() == 0);
  }
};

// Mark only the cached size, not the filesystem itself. A real statfs refresh
// replaces this impossible size regardless of concurrent filesystem activity.
void mark_stale(fs_stat *fs) { fs->size = -1; }
}  // namespace

TEST_CASE("filesystem stats initialize immediately and share cached paths",
          "[fs]") {
  fs_test_context context;
  auto *fs = prepare_fs_stat("/");
  REQUIRE(fs != nullptr);
  REQUIRE(fs->size >= 0);
  REQUIRE(fs->errored == 0);

  mark_stale(fs);
  REQUIRE(prepare_fs_stat("/") == fs);
  REQUIRE(fs->size == -1);
  // A different path still receives its initial reading immediately.
  auto *other = prepare_fs_stat("/.");
  REQUIRE(other != nullptr);
  REQUIRE(other != fs);
  REQUIRE(other->size >= 0);
}

TEST_CASE("filesystem stats retain the default thirteen second interval",
          "[fs]") {
  fs_test_context context;
  auto *fs = prepare_fs_stat("/");
  REQUIRE(fs != nullptr);
  mark_stale(fs);
  context.update(0.0);
  REQUIRE(fs->size >= 0);

  mark_stale(fs);
  context.update(12.75);
  REQUIRE(fs->size == -1);
  context.update(13.0);
  REQUIRE(fs->size >= 0);
  mark_stale(fs);
  context.update(25.75);
  REQUIRE(fs->size == -1);
  context.update(26.0);
  REQUIRE(fs->size >= 0);
}

TEST_CASE("filesystem stats honor fractional refresh intervals", "[fs]") {
  fs_test_context context;
  context.configure("0.5");
  auto *fs = prepare_fs_stat("/");
  REQUIRE(fs != nullptr);
  context.update(10.0);
  mark_stale(fs);
  context.update(10.25);
  REQUIRE(fs->size == -1);
  context.update(10.5);
  REQUIRE(fs->size >= 0);
}

TEST_CASE("zero filesystem interval refreshes every update cycle", "[fs]") {
  fs_test_context context;
  context.configure("0");
  auto *fs = prepare_fs_stat("/");
  auto *other = prepare_fs_stat("/.");
  REQUIRE(fs != nullptr);
  REQUIRE(other != nullptr);
  context.update(0.0);
  for (double time : {0.0, 0.25, 1.0}) {
    mark_stale(fs);
    mark_stale(other);
    context.update(time);
    REQUIRE(fs->size >= 0);
    REQUIRE(other->size >= 0);
  }
}

TEST_CASE("filesystem refresh waits for the first eligible update cycle",
          "[fs]") {
  fs_test_context context;
  context.configure("5");
  auto *fs = prepare_fs_stat("/");
  REQUIRE(fs != nullptr);
  context.update(0.0);
  mark_stale(fs);
  context.update(3.0);
  REQUIRE(fs->size == -1);
  context.update(6.0);
  REQUIRE(fs->size >= 0);
  mark_stale(fs);
  context.update(9.0);
  REQUIRE(fs->size == -1);
  context.update(12.0);
  REQUIRE(fs->size >= 0);
}

TEST_CASE("clearing filesystem stats resets the refresh deadline", "[fs]") {
  fs_test_context context;
  auto *fs = prepare_fs_stat("/");
  REQUIRE(fs != nullptr);
  context.update(100.0);
  clear_fs_stats();
  fs = prepare_fs_stat("/");
  REQUIRE(fs != nullptr);
  REQUIRE(fs->size >= 0);
  mark_stale(fs);
  context.update(100.25);
  REQUIRE(fs->size >= 0);
}

TEST_CASE("invalid filesystem intervals fall back to the default", "[fs]") {
  fs_test_context context;
  const char *interval = GENERATE("-1", "1/0", "-1/0", "0/0", "'fast'");
  INFO("fs_update_interval = " << interval);
  context.configure(interval, true);
  auto *fs = prepare_fs_stat("/");
  REQUIRE(fs != nullptr);
  context.update(0.0);
  mark_stale(fs);
  context.update(12.75);
  REQUIRE(fs->size == -1);
  context.update(13.0);
  REQUIRE(fs->size >= 0);
}

TEST_CASE("filesystem refresh preserves error recovery", "[fs]") {
  fs_test_context context;
  context.configure("0");
  auto *fs = prepare_fs_stat("/");
  REQUIRE(fs != nullptr);
  // An empty pathname fails deterministically without modifying the disk.
  fs->path[0] = '\0';
  context.update(0.0);
  REQUIRE(fs->errored == 1);
  REQUIRE(fs->size == 0);
  REQUIRE(fs->avail == 0);
  REQUIRE(fs->free == 0);
  REQUIRE(std::string(fs->type) == "unknown");
  context.update(1.0);
  REQUIRE(fs->errored == 1);
  fs->path[0] = '/';
  fs->path[1] = '\0';
  context.update(2.0);
  REQUIRE(fs->size >= 0);
#ifndef __sun
  REQUIRE(fs->errored == 0);
#endif
}
