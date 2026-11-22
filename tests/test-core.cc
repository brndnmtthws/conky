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

#include <conky.h>
#include <content/specials.h>
#include <core.h>
#include <lua/lua-config.hh>

namespace {
struct memory_config {
  std::unique_ptr<lua::state> saved_state = std::move(state);
  information saved_info = info;

  memory_config() {
    state = std::make_unique<lua::state>();
    conky::export_symbols(*state);
    info.memmax = 102400;
    info.mem = 26000;  // 25.39%, keeping the legacy truncation
    info.memwithbuffers = 76800;
    info.memeasyfree = 51200;
    info.legacymem = 40960;
    info.memfree = 20480;
    info.memdirty = 10240;
    info.memavail = 61440;
  }
  ~memory_config() {
    info = saved_info;
    state = std::move(saved_state);
  }
};

struct parsed_memory_text {
  text_object root{};

  explicit parsed_memory_text(const char *text) {
    extract_variable_text_internal(&root, text);
  }
  ~parsed_memory_text() { free_text_objects(&root); }

  std::string render() const {
    char output[256]{};
    generate_text_internal(output, sizeof(output), root);
    return output;
  }
};
}  // namespace

TEST_CASE("remove_comments returns correct value") {
  SECTION("for no comments") {
    char text[] = "test text\n";

    size_t removed_chars = remove_comments(text);

    REQUIRE(removed_chars == 0);
  }

  SECTION("for no comments but with backslashes") {
    char text[] = "te\\st t\\ext\n";

    size_t removed_chars = remove_comments(text);

    REQUIRE(removed_chars == 0);
  }

  SECTION("for single line of comment") {
    char text_with_newline[] = "#test text\n";
    char text_no_newline[] = "#test text";

    size_t removed_chars_with_newline = remove_comments(text_with_newline);
    size_t removed_chars_no_newline = remove_comments(text_no_newline);

    REQUIRE(removed_chars_with_newline == 11);
    REQUIRE(removed_chars_no_newline == 10);
  }

  SECTION("for comment starting in middle of line") {
    char text[] = "test #text\n";

    size_t removed_chars = remove_comments(text);

    REQUIRE(removed_chars == 6);
  }
}

TEST_CASE("memory objects select metrics independently", "[memory]") {
  memory_config config;
  const parsed_memory_text percents(
      "$memperc|${memperc -b}|${memperc --metric free}|"
      "${memperc --metric easyfree}|${memperc --metric used}|$memperc");
  CHECK(percents.render() == "25|75|20|50|25|25");

  const parsed_memory_text buffered_si("${mem --metric buffered si}");
  const parsed_memory_text legacy_si("${memwithbuffers si}");
  const parsed_memory_text decimal("${mem si}");
  const parsed_memory_text binary("$mem");
  const parsed_memory_text free_mem("${mem --metric free}");
  const parsed_memory_text legacy_free("$memfree");
  CHECK(buffered_si.render() == legacy_si.render());
  CHECK(decimal.render() != binary.render());
  CHECK(free_mem.render() == legacy_free.render());
}

TEST_CASE("memory percentages handle malformed and out of range values",
          "[memory]") {
  memory_config config;
  const parsed_memory_text text(
      "${memperc --metric unknown}|${memperc --metric}|${memperc -z}|"
      "${memperc -b extra}|${memperc --metric free}|${memperc -b   }");
  CHECK(text.render() == "25|25|25|25|20|75");
  info.memfree = info.memmax + 1;
  CHECK(text.render() == "25|25|25|25|100|75");
  info.memmax = 0;
  CHECK(text.render() == "0|0|0|0|0|0");
}

TEST_CASE("memory selector preserves bar dimensions and buffered alias",
          "[memory]") {
  memory_config config;
  const temporary_specials isolated_specials;
  state->pushboolean(true);
  out_to_stdout.lua_set(*state);
  const parsed_memory_text text("${membar -b 7,20}");
  const parsed_memory_text alias("${memwithbuffersbar 7,20}");
  auto *bar = text.root.next;
  auto *legacy = alias.root.next;
  REQUIRE(bar != nullptr);
  REQUIRE(legacy != nullptr);
  CHECK(bar->callbacks.barval(bar) == 0.75);
  CHECK(bar->callbacks.barval(bar) == legacy->callbacks.barval(legacy));
  CHECK(text.render() == std::string(15, '#') + std::string(5, '.'));
  CHECK(text.render() == alias.render());
}

#ifdef __linux__
TEST_CASE("Linux memory counters can all be selected", "[memory]") {
  memory_config config;
  const parsed_memory_text text(
      "${memperc --metric legacy}|${memperc --metric dirty}|"
      "${memperc --metric available}");
  CHECK(text.render() == "40|10|60");
}
#endif

#ifdef BUILD_GUI
TEST_CASE("memory gauge uses the selected value", "[memory]") {
  memory_config config;
  const parsed_memory_text gauge("${memgauge --metric free 7,20}");
  auto *obj = gauge.root.next;
  REQUIRE(obj != nullptr);
  REQUIRE(obj->callbacks.gaugeval != nullptr);
  CHECK(obj->callbacks.gaugeval(obj) == Catch::Approx(0.2));
}

TEST_CASE("memory selectors preserve graph options and isolate history",
          "[memory]") {
  memory_config config;
  const temporary_specials isolated_specials;
  const parsed_memory_text free_graph("${memgraph --metric free}");
  const parsed_memory_text buffered("${memgraph -b 7,20 -l}");
  const parsed_memory_text legacy("${memwithbuffersgraph 7,20 -l}");
  const parsed_memory_text used("${memgraph}");
  const auto plot = [](text_object *obj) {
    char output[16]{};
    new_graph(obj, output, sizeof(output), obj->callbacks.graphval(obj));
  };

  plot(buffered.root.next);
  REQUIRE(specials != nullptr);
  CHECK(specials->height == 7);
  CHECK(specials->width == 20);
#ifdef BUILD_MATH
  CHECK(specials->scale_log != 0);
#endif
  const auto buffered_key = specials->data_hash;

  special_count = 0;
  plot(legacy.root.next);
  CHECK(specials->data_hash == buffered_key);

  special_count = 0;
  plot(free_graph.root.next);
  CHECK(specials->data_hash != buffered_key);
  const auto free_key = specials->data_hash;

  special_count = 0;
  plot(used.root.next);
  CHECK(specials->data_hash != free_key);
  CHECK(specials->data_hash != buffered_key);
}
#endif
