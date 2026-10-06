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

#include <cstdlib>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "catch2/catch.hpp"

#include "common.h"
#include "conky.h"
#include "content/specials.h"
#include "content/text_object.h"
#include "core.h"
#include "data/exec.h"
#include "lua/lua-config.hh"
#include "output/display-output.hh"

#ifdef BUILD_GUI
#include "data/network/net_stat.h"
#include "lua/fonts.h"
#endif

extern double maxspeedval;

namespace conky {
extern std::set<output_t> resolved_outputs;
}

// Exercise the same output-parsing entry point as print_exec, without starting
// a shell or relying on an asynchronous command callback.
void fill_p(const char *, text_object *, char *, unsigned int);

namespace {
void clear_test_specials() {
  while (specials != nullptr) {
    auto *next = specials->next;
    delete specials;
    specials = next;
  }
  special_count = 0;
}

struct no_update_context {
  std::unique_ptr<lua::state> saved_state = std::move(state);
  special_node *saved_specials = specials;
  int saved_special_count = special_count;
  float saved_load = info.loadavg[0];
  Colour saved_color = get_current_text_color();
  double saved_max_speed = maxspeedval;
  conky::display_outputs_t saved_registered = conky::registered_outputs();
  std::set<conky::output_t> saved_resolved = conky::resolved_outputs;
  std::vector<conky::display_output_base *> saved_active =
      conky::active_display_outputs;
  std::vector<conky::display_output_base *> saved_current =
      conky::current_display_outputs;
#ifdef BUILD_GUI
  std::vector<font_list> saved_fonts = std::move(fonts);
  unsigned int saved_selected_font = selected_font;
#endif
  // The base backend's drawing/font methods are no-ops: no display is needed.
  conky::display_output_base output{"no_update test", conky::output_t::X11};

  no_update_context() {
    state = std::make_unique<lua::state>();
    conky::export_symbols(*state);
    specials = nullptr;
    special_count = 0;
    output.is_graphical = true;
    conky::resolved_outputs = {conky::output_t::X11};
    conky::active_display_outputs = {&output};
    conky::current_display_outputs.clear();
#ifdef BUILD_GUI
    fonts.resize(1);
    selected_font = 0;
#endif
  }

  ~no_update_context() {
    clear_test_specials();
    specials = saved_specials;
    special_count = saved_special_count;
    info.loadavg[0] = saved_load;
    set_current_text_color(saved_color);
    maxspeedval = saved_max_speed;
    conky::registered_outputs().swap(saved_registered);
    conky::resolved_outputs.swap(saved_resolved);
    conky::active_display_outputs.swap(saved_active);
    conky::current_display_outputs.swap(saved_current);
#ifdef BUILD_GUI
    fonts.swap(saved_fonts);
    selected_font = saved_selected_font;
#endif
    state = std::move(saved_state);
  }
};

struct cached_text {
  text_object object{};

  explicit cached_text(const char *text) { scan_no_update(&object, text); }
  ~cached_text() { free_no_update(&object); }

  std::string print() {
    char buffer[256]{};
    print_no_update(&object, buffer, sizeof(buffer));
    return buffer;
  }
};

struct parsed_text {
  text_object root{};

  explicit parsed_text(const char *text) {
    extract_variable_text_internal(&root, text);
  }
  ~parsed_text() { free_text_objects(&root); }

  std::string generate() {
    char buffer[256]{};
    generate_text_internal(buffer, sizeof(buffer), root);
    return buffer;
  }
};

struct parsed_exec_output {
  text_object object{};

  parsed_exec_output() {
    scan_exec_arg(&object, "");
    object.parse = true;
  }
  ~parsed_exec_output() {
    free_exec(&object);
    free_text_objects(object.sub);
    free(object.sub);
  }

  std::string print(const char *text) {
    char buffer[256]{};
    fill_p(text, &object, buffer, sizeof(buffer));
    return buffer;
  }
};

std::string evaluate_text(const char *text) {
  char buffer[256]{};
  evaluate(text, buffer, sizeof(buffer));
  return buffer;
}

void check_color(const special_node *node, const char *color) {
  REQUIRE(node != nullptr);
  CHECK(node->type == text_node_t::FG);
  CHECK(node->arg == parse_color(color).to_argb32());
}

const std::string marker(1, SPECIAL_CHAR);
}  // namespace

TEST_CASE("no_update caches evaluated plain text", "[no_update]") {
  no_update_context context;
  info.loadavg[0] = 1.25f;
  cached_text text("load=${loadavg 1}");
  REQUIRE(text.print() == "load=1.25");

  info.loadavg[0] = 2.5f;
  CHECK(text.print() == "load=1.25");
  CHECK(text.print() == "load=1.25");
  CHECK(special_count == 0);
  CHECK(specials == nullptr);

  char truncated[5]{};
  print_no_update(&text.object, truncated, sizeof(truncated));
  CHECK(std::string(truncated) == "load");

  cached_text empty("");
  CHECK(empty.print().empty());
}

TEST_CASE("no_update strips special markers from cached text", "[no_update]") {
  no_update_context context;
  std::vector<const char *> inputs = {"left${color red}right",
                                      "left${offset 3}right"};
#ifdef BUILD_GUI
  inputs.insert(inputs.end(),
                {"left${hr 2}right", "left${font monospace}right",
                 "left${membar 2,4}right", "left${loadgraph 2,4}right"});
#endif

  for (const char *input : inputs) {
    CAPTURE(input);
    // Prove this backend really emits a marker for the object under test.
    REQUIRE(evaluate_text(input) == "left" + marker + "right");
    REQUIRE(special_count == 1);
    clear_test_specials();

    cached_text text(input);
    CHECK(std::string(text.object.data.s) == "leftright");
    CHECK(text.print() == "leftright");
    CHECK(text.print() == "leftright");
    CHECK(special_count == 0);
    CHECK(specials == nullptr);
  }
}

TEST_CASE("nested no_update stays aligned across draw cycles", "[no_update]") {
  no_update_context context;
  parsed_text text(
      "${color blue}A${no_update B${color red}"
      "${no_update ${offset 3}C}D}${color green}E");
  REQUIRE(special_count == 0);
  REQUIRE(specials == nullptr);

  for (int frame = 0; frame < 3; ++frame) {
    CAPTURE(frame);
    special_count = 0;
    CHECK(text.generate() == marker + "ABCD" + marker + "E");
    REQUIRE(special_count == 2);
    check_color(specials, "blue");
    check_color(specials->next, "green");
    CHECK(specials->next->next == nullptr);
  }
}

TEST_CASE("runtime eval no_update preserves surrounding specials",
          "[no_update][evaluate]") {
  no_update_context context;
  const char *input = "${eval ${no_update ${color red}X}}${color green}Y";
  std::string expected = "X" + marker + "Y";

  SECTION("no_previous_special") {}
  SECTION("already_generated_special") {
    input = "${color blue}B${eval ${no_update ${color red}X}}${color green}Y";
    expected = marker + "B" + expected;
  }

  for (int frame = 0; frame < 3; ++frame) {
    CAPTURE(frame);
    special_count = 0;
    CHECK(evaluate_text(input) == expected);
    if (expected[0] == SPECIAL_CHAR) {
      REQUIRE(special_count == 2);
      check_color(specials, "blue");
      check_color(specials->next, "green");
      CHECK(specials->next->next == nullptr);
    } else {
      REQUIRE(special_count == 1);
      check_color(specials, "green");
      CHECK(specials->next == nullptr);
    }
  }
}

TEST_CASE("execp no_update preserves specials when output changes",
          "[no_update][execp]") {
  no_update_context context;
  parsed_exec_output output;
  // The unchanged second result reuses execp's parsed subtree; the third
  // result forces a new scan during generation with a live special before it.
  for (const char *cached : {"X", "X", "changed"}) {
    CAPTURE(cached);
    special_count = 0;
    REQUIRE(evaluate_text("${color blue}B") == marker + "B");
    special_node *previous = specials;
    std::string input =
        std::string("${no_update ${color red}") + cached + "}${color green}Y";
    CHECK(output.print(input.c_str()) == cached + marker + "Y");
    REQUIRE(special_count == 2);
    REQUIRE(specials == previous);
    check_color(specials, "blue");
    check_color(specials->next, "green");
    CHECK(specials->next->next == nullptr);
  }
}

TEST_CASE("temporary specials restore nested state after exceptions",
          "[no_update]") {
  no_update_context context;
  REQUIRE(evaluate_text("${color blue}") == marker);
  special_node *original = specials;
  maxspeedval = 16.0;
#ifdef BUILD_GUI
  const auto font_count = fonts.size();
#endif

  auto outer_failure = [&] {
    temporary_specials outer;
    REQUIRE(specials == nullptr);
    REQUIRE(special_count == 0);
    char buffer[2]{};
    special_node *temporary = new_special(buffer, text_node_t::GRAPH);
    temporary->graph_data = {1.0, 2.0};
    maxspeedval = 32.0;

    auto inner_failure = [&] {
      temporary_specials inner;
      REQUIRE(specials == nullptr);
      REQUIRE(special_count == 0);
      new_special(buffer, text_node_t::GRAPH)->graph_data = {3.0, 4.0};
      maxspeedval = 64.0;
      throw std::runtime_error("inner evaluation failed");
    };
    REQUIRE_THROWS_AS(inner_failure(), std::runtime_error);
    REQUIRE(specials == temporary);
    CHECK(special_count == 1);
    CHECK(temporary->graph_data == std::vector<double>{1.0, 2.0});
    CHECK(temporary->next == nullptr);
    CHECK(maxspeedval == 32.0);
#ifdef BUILD_GUI
    // Unwinding the inner guard must keep the outer guard's suppression on.
    CHECK(evaluate_text("${font discarded}") == marker);
    CHECK(fonts.size() == font_count);
#endif
    throw std::runtime_error("outer evaluation failed");
  };

  REQUIRE_THROWS_AS(outer_failure(), std::runtime_error);
  REQUIRE(specials == original);
  CHECK(special_count == 1);
  check_color(specials, "blue");
  CHECK(specials->next == nullptr);
  CHECK(maxspeedval == 16.0);
#ifdef BUILD_GUI
  // Unwinding the outer guard must enable ordinary font registration again.
  CHECK(evaluate_text("${font serif}") == marker);
  REQUIRE(fonts.size() == font_count + 1);
  REQUIRE(special_count == 2);
  REQUIRE(specials->next != nullptr);
  REQUIRE(specials->next->font_added > 0);
  REQUIRE(static_cast<size_t>(specials->next->font_added) < fonts.size());
  CHECK(fonts[specials->next->font_added].name == "serif");
#endif
}

#ifdef BUILD_GUI
TEST_CASE("startup no_update keeps the following horizontal line aligned",
          "[no_update]") {
  no_update_context context;
  parsed_text text("${no_update ${color green}cached}$hr");
  REQUIRE(special_count == 0);
  REQUIRE(specials == nullptr);

  for (int frame = 0; frame < 3; ++frame) {
    CAPTURE(frame);
    special_count = 0;
    CHECK(text.generate() == "cached" + marker);
    REQUIRE(special_count == 1);
    REQUIRE(specials != nullptr);
    CHECK(specials->type == text_node_t::HORIZONTAL_LINE);
    CHECK(specials->next == nullptr);
  }
}

TEST_CASE("runtime no_update does not register discarded fonts",
          "[no_update][font]") {
  no_update_context context;
  const auto font_count = fonts.size();
  parsed_text text(
      "${font serif}A${eval ${no_update ${font monospace}X}}"
      "${font sans}Y");

  for (int frame = 0; frame < 3; ++frame) {
    CAPTURE(frame);
    special_count = 0;
    CHECK(text.generate() == marker + "AX" + marker + "Y");
    REQUIRE(special_count == 2);
    REQUIRE(fonts.size() == font_count + 2);
    REQUIRE(specials != nullptr);
    CHECK(specials->type == text_node_t::FONT);
    REQUIRE(specials->font_added > 0);
    REQUIRE(static_cast<size_t>(specials->font_added) < fonts.size());
    CHECK(fonts[specials->font_added].name == "serif");
    REQUIRE(specials->next != nullptr);
    CHECK(specials->next->type == text_node_t::FONT);
    REQUIRE(specials->next->font_added > 0);
    REQUIRE(static_cast<size_t>(specials->next->font_added) < fonts.size());
    CHECK(fonts[specials->next->font_added].name == "sans");
    CHECK(specials->next->next == nullptr);
  }
}

TEST_CASE("no_update preserves retained graph history", "[no_update][graph]") {
  no_update_context context;
  info.loadavg[0] = 1.25f;
  parsed_text live_graph("${loadgraph 2,4}");
  REQUIRE(live_graph.generate() == marker);
  REQUIRE(special_count == 1);
  special_node *retained = specials;
  const auto history = retained->graph_data;
  const auto data_hash = retained->data_hash;
  REQUIRE(history.size() == 4);
  REQUIRE(history[0] == 1.25);

  SECTION("graph_already_active_this_frame") {}
  SECTION("graph_slot_retained_from_previous_frame") { special_count = 0; }
  const int count_before_scan = special_count;
  info.loadavg[0] = 8.0f;
  cached_text cached("${loadgraph 2,4}${color red}cached");
  CHECK(cached.print() == "cached");
  CHECK(special_count == count_before_scan);
  REQUIRE(specials == retained);
  CHECK(retained->type == text_node_t::GRAPH);
  CHECK(retained->data_hash == data_hash);
  CHECK(retained->graph_data == history);
  CHECK(retained->next == nullptr);

  special_count = 0;
  info.loadavg[0] = 2.5f;
  CHECK(live_graph.generate() == marker);
  REQUIRE(special_count == 1);
  REQUIRE(specials == retained);
  CHECK(retained->graph_data[0] == 2.5);
  CHECK(retained->graph_data[1] == 1.25);
}
TEST_CASE("no_update preserves shared speedgraph scale", "[no_update][graph]") {
  no_update_context context;
  struct network_sample {
    net_stat saved = netstats[0];

    network_sample() {
      netstats[0] = net_stat{};
      netstats[0].dev = strdup("noupdatetest");
      netstats[0].recv_speed = 4096.0;
    }
    ~network_sample() {
      free(netstats[0].dev);
      netstats[0] = saved;
    }
  } sample;

  constexpr const char *graph = "${downspeedgraph noupdatetest 2,4}";
  maxspeedval = 16.0;
  REQUIRE(evaluate_text(graph) == marker);
  REQUIRE(maxspeedval == 4096.0);
  clear_test_specials();

  maxspeedval = 16.0;
  cached_text cached(graph);
  CHECK(cached.print().empty());
  CHECK(special_count == 0);
  CHECK(specials == nullptr);
  CHECK(maxspeedval == 16.0);
}
#endif
