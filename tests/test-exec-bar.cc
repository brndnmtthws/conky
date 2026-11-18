/*
 * Conky, a system monitor, based on torsmo
 * This file is licensed under GPLv3. See COPYING for details.
 */

#include "config.h"

#include <cstdlib>
#include <string>
#include <utility>

#include "catch2/catch.hpp"
#include "conky.h"
#include "content/specials.h"
#include "content/text_object.h"
#include "data/exec.h"
#include "lua/lua-config.hh"
#include "output/display-output.hh"
#ifdef BUILD_GUI
#include "geometry.h"
#include "lua/fonts.h"

extern conky::vec2i text_size;
int draw_each_line_inner(char *, int, int);
#endif

namespace {
struct bar_context {
  std::unique_ptr<lua::state> saved_state = std::move(state);
  conky::display_outputs_t saved_registered = conky::registered_outputs();
  std::vector<conky::display_output_base *> saved_active =
      conky::active_display_outputs;
  std::vector<conky::display_output_base *> saved_current =
      conky::current_display_outputs;
  temporary_specials isolated_specials;
#ifdef BUILD_GUI
  std::vector<font_list> saved_fonts = std::move(fonts);
  unsigned int saved_selected_font = selected_font;
  conky::vec2i saved_text_size = text_size;
#endif

  bar_context() {
    state = std::make_unique<lua::state>();
    conky::export_symbols(*state);
    conky::active_display_outputs.clear();
    conky::current_display_outputs.clear();
#ifdef BUILD_GUI
    fonts.resize(1);
    selected_font = 0;
    text_size = conky::vec2i(102, 20);
#endif
  }
  ~bar_context() {
    conky::registered_outputs().swap(saved_registered);
    conky::active_display_outputs.swap(saved_active);
    conky::current_display_outputs.swap(saved_current);
#ifdef BUILD_GUI
    fonts.swap(saved_fonts);
    selected_font = saved_selected_font;
    text_size = saved_text_size;
#endif
    state = std::move(saved_state);
  }
};

struct parsed_bar {
  text_object obj{};
  parsed_bar(const std::string &arg, bool interval) {
    scan_exec_arg(
        &obj, arg.c_str(),
        interval ? exec_flag::bar | exec_flag::interval : exec_flag::bar);
  }
  ~parsed_bar() {
    free_exec(&obj);
    free(obj.special_data);
  }
};

#ifdef BUILD_GUI
struct recording_output : conky::display_output_base {
  int outline_x = 0, outline_width = 0, fill_x = 0, fill_width = 0;
  recording_output() : display_output_base("bar test", conky::output_t::X11) {
    is_graphical = true;
  }
  void draw_rect(int x, int, int width, int) override {
    outline_x = x;
    outline_width = width;
  }
  void fill_rect(int x, int, int width, int) override {
    fill_x = x;
    fill_width = width;
  }
};
#endif
}  // namespace

TEST_CASE("exec bars reverse text fill only when requested", "[exec_bar]") {
  bar_context context;
  state->pushboolean(true);
  out_to_stdout.lua_set(*state);
  const bool interval = GENERATE(false, true);
  const bool reverse = GENERATE(false, true);
  const int value = GENERATE(0, 25, 100, -25, 125);
  parsed_bar bar((interval ? "2 " : "") + std::string(reverse ? "-x " : "") +
                     "6,8 echo 25",
                 interval);
  char buffer[64]{};
  new_bar(&bar.obj, buffer, sizeof(buffer),
          get_barnum(std::to_string(value).c_str()));
  const int filled = value >= 0 && value <= 100 ? value * 8 / 100 : 0;
  const std::string fill(filled, '#'), empty(8 - filled, '.');
  CHECK(buffer == (reverse ? empty + fill : fill + empty));
}

TEST_CASE("exec bar options leave shell arguments intact", "[exec_bar]") {
  bar_context context;
  const bool interval = GENERATE(false, true);
  const std::string prefix = interval ? "2 " : "";
  const std::string options = GENERATE("", "-x ", "-x\t", "-x 6,8 ");
  parsed_bar bar(prefix + options + "printf '%s' 'command -x argument'",
                 interval);
  register_exec(&bar.obj);
  conky::run_all_callbacks();
  REQUIRE(bar.obj.exec_handle != nullptr);
  CHECK((*bar.obj.exec_handle)->get_result_copy() == "command -x argument");
}

TEST_CASE("exec bar direction option requires a separate leading token",
          "[exec_bar]") {
  bar_context context;
  const bool interval = GENERATE(false, true);
  const std::string prefix = interval ? "2 " : "";
  const std::string command = GENERATE("-xyz", "-x=1", "echo -x", "\"-x\"");
  parsed_bar bar(prefix + command, interval);
  register_exec(&bar.obj);
  auto expected = conky::register_cb<exec_cb>(1, true, command);
  REQUIRE(bar.obj.exec_handle != nullptr);
  CHECK(&**bar.obj.exec_handle == &*expected);
}

#ifdef BUILD_GUI
TEST_CASE("exec bars anchor graphical fill at the selected edge",
          "[exec_bar]") {
  bar_context context;
  recording_output output;
  conky::active_display_outputs = {&output};
  const bool interval = GENERATE(false, true);
  const bool reverse = GENERATE(false, true);
  const int value = GENERATE(0, 25, 100, -25);
  const std::string dimensions = GENERATE("6,101 ", "6,0 ", "");
  parsed_bar bar((interval ? "2 " : "") + std::string(reverse ? "-x " : "") +
                     dimensions + "echo 25",
                 interval);
  char buffer[64]{};
  new_bar(&bar.obj, buffer, sizeof(buffer),
          get_barnum(std::to_string(value).c_str()));
  REQUIRE(specials != nullptr);
  REQUIRE(specials->type == text_node_t::BAR);
  draw_each_line_inner(buffer, 0, -1);
  CHECK(output.outline_width == 101);
  CHECK(output.fill_width == (value < 0 ? 0 : value * 101 / 100));
  CHECK(output.fill_x - output.outline_x ==
        (reverse ? output.outline_width - output.fill_width : 0));
}
#endif
