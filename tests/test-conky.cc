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
 * Copyright (c) 2005-2019 Brenden Matthews, Philip Kovacs, et. al.
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
#include "conky.h"
#include "lua-config.hh"

TEST_CASE("Expressions can be evaluated", "[evaluate]") {
  state = std::make_unique<lua::state>();
  conky::export_symbols(*state);

  SECTION("Simple expressions without substitutions can be evaluated") {
    constexpr int kMaxSize = 10;
    const char input[kMaxSize] = "text";
    char result[kMaxSize]{'\0'};

    evaluate(input, result, kMaxSize);
    REQUIRE(strncmp(input, result, kMaxSize) == 0);
  }

  SECTION("execs can be evaluated") {
    constexpr int kMaxSize = 50;
    const char input[kMaxSize] = "${exec echo text}";
    char result[kMaxSize]{'\0'};

    evaluate(input, result, kMaxSize);
    REQUIRE(strncmp("text", result, kMaxSize) == 0);
  }

  SECTION("execp echo without other substitutions can be evaluated") {
    constexpr int kMaxSize = 50;
    const char input[kMaxSize] = "${execp echo text}";
    char result[kMaxSize]{'\0'};

    evaluate(input, result, kMaxSize);
    REQUIRE(strncmp("text", result, kMaxSize) == 0);
  }
}
