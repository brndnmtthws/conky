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
#include "config.h"

#if defined(__linux__) && defined(BUILD_IPV6)
#include <arpa/inet.h>
#endif

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

#include <conky.h>
#include <content/specials.h>
#include <content/text_object.h>
#include <core.h>
#include <data/exec.h>
#include <data/network/net_stat.h>
#include <lua/lua-config.hh>

namespace {

// Mirrors the layout of `struct bar` in specials.cc so the parsed dimensions
// can be read back out of obj.special_data.
struct bar {
  char flags;
  int width, height;
  double scale;
};

std::pair<int, int> parse_bar_dimensions(const char *args) {
  struct text_object obj {};
  parse_net_stat_bar_arg(&obj, args, nullptr);
  auto *b = static_cast<struct bar *>(obj.special_data);
  std::pair<int, int> dims{b->height, b->width};
  free(obj.special_data);
  return dims;
}

}  // namespace

TEST_CASE("parse_net_stat_bar_arg parses dimensions regardless of arg order",
          "[net]") {
  state = std::make_unique<lua::state>();
  conky::export_symbols(*state);

  SECTION("interface before dimensions (#2349)") {
    auto [height, width] = parse_bar_dimensions("wlp3s0 3,260");
    REQUIRE(height == 3);
    REQUIRE(width == 260);
  }

  SECTION("dimensions before interface") {
    auto [height, width] = parse_bar_dimensions("3,260 wlp3s0");
    REQUIRE(height == 3);
    REQUIRE(width == 260);
  }

  SECTION("interface only falls back to default bar height") {
    auto [height, width] = parse_bar_dimensions("wlp3s0");
    REQUIRE(height == 6);  // default_bar_height
    REQUIRE(width == 0);   // default_bar_width
  }
}

#if defined(__linux__) && defined(BUILD_IPV6)
TEST_CASE("v6addrs filters and formats each object independently",
          "[net][ipv6]") {
  struct network_sample {
    std::unique_ptr<lua::state> saved_state = std::move(state);
    net_stat saved_netstat = netstats[0];
    char device[16] = "v6addrs_test";
    v6addr link{{}, 64, 'L', nullptr};
    v6addr global{{}, 64, 'G', &link};

    network_sample() {
      state = std::make_unique<lua::state>();
      conky::export_symbols(*state);
      netstats[0] = net_stat{};
      netstats[0].dev = device;
      netstats[0].v6addrs = &global;
    }

    ~network_sample() {
      netstats[0] = saved_netstat;
      state = std::move(saved_state);
    }
  } sample;

  REQUIRE(inet_pton(AF_INET6, "2001:db8::1", &sample.global.addr) == 1);
  REQUIRE(inet_pton(AF_INET6, "fe80::1", &sample.link.addr) == 1);

  struct parsed_text {
    text_object root{};

    explicit parsed_text(const char *text) {
      extract_variable_text_internal(&root, text);
    }

    ~parsed_text() { free_text_objects(&root); }

    std::string render() const {
      char output[256]{};
      generate_text_internal(output, sizeof(output), root);
      return output;
    }
  } text(
      "${v6addrs -n v6addrs_test global}|"
      "${v6addrs link -s v6addrs_test}|"
      "${v6addrs v6addrs_test}");

  CHECK(text.render() == "2001:db8::1/64|fe80::1(L)|2001:db8::1, fe80::1");
  netstats[0].v6addrs = &sample.link;
  CHECK(text.render() == "No Address|fe80::1(L)|fe80::1");
}
#endif  // defined(__linux__) && defined(BUILD_IPV6)

TEST_CASE("get_barnum returns 0.0 silently for empty or null input", "[exec]") {
  SECTION("empty string returns 0.0") { REQUIRE(get_barnum("") == 0.0); }

  SECTION("nullptr returns 0.0") { REQUIRE(get_barnum(nullptr) == 0.0); }

  SECTION("valid integer returns parsed value") {
    REQUIRE(get_barnum("42") == 42.0);
  }

  SECTION("valid float returns parsed value") {
    REQUIRE(get_barnum("73.5") == 73.5);
  }

  SECTION("value at 100 returns 100") { REQUIRE(get_barnum("100") == 100.0); }

  SECTION("value at 0 returns 0") { REQUIRE(get_barnum("0") == 0.0); }
}
