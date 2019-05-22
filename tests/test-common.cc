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

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do
                           // this in one cpp file

#include "catch2/catch.hpp"

#include <common.h>
#include <conky.h>

TEST_CASE("to_real_path becomes homedir", "[to_real_path]") {
  REQUIRE(to_real_path("~/test") == std::string(getenv("HOME")) + "/test");
}

TEST_CASE("variables are substituted correctly", "[variable_substitute]") {
  SECTION("an empty string input returns an empty string") {
    REQUIRE(variable_substitute("") == "");
  }

  SECTION("string in with no $ returns same string") {
    std::string string_alpha = "abcdefghijklmnopqrstuvwxyz";
    std::string string_numbers = "1234567890";
    std::string string_special = "`~!@#$%^&*()-=_+[]{}\\|;:'\",<.>/?";

    REQUIRE(variable_substitute(string_alpha) == string_alpha);
    REQUIRE(variable_substitute(string_numbers) == string_numbers);
    REQUIRE(variable_substitute(string_special) == string_special);
  }
}

TEST_CASE("cpu_percentage and cpu_barval return correct values") {
  struct text_object obj0;
  obj0.data.i = 0;
  struct text_object obj1;
  obj1.data.i = 1;
  struct text_object obj2;
  obj2.data.i = 2;
  info.cpu_count = 1;

  SECTION("for non-existent cpu") {
    info.cpu_usage = new float[2];
    info.cpu_usage[0] = 0.253;
    info.cpu_usage[1] = 0.507;

    REQUIRE(cpu_barval(&obj2) == 0);

    // This does not exist in Catch2, but would be nice to have since that's
    // what happens in this case.
    // REQUIRE_EXIT(cpu_percentage(&obj2));

    delete[] info.cpu_usage;
  }

  SECTION("for cpu_usage == nullptr") {
    info.cpu_usage = nullptr;

    REQUIRE(cpu_percentage(&obj0) == 0);
    REQUIRE(cpu_barval(&obj0) == 0);
    REQUIRE(cpu_percentage(&obj1) == 0);
    REQUIRE(cpu_barval(&obj1) == 0);
  }

  SECTION("for cpu_usage has data") {
    info.cpu_usage = new float[2];
    info.cpu_usage[0] = 0.253;
    info.cpu_usage[1] = 0.507;

    REQUIRE(cpu_percentage(&obj0) == 25);
    REQUIRE(cpu_barval(&obj0) == Approx(0.253));
    REQUIRE(cpu_percentage(&obj1) == 51);
    REQUIRE(cpu_barval(&obj1) == Approx(0.507));

    delete[] info.cpu_usage;
  }
}

TEST_CASE("mem_with_buffers_barval returns correct value") {
  info.memwithbuffers = 6;

  SECTION("for memmax == 0") {
    info.memmax = 0;
    REQUIRE(mem_with_buffers_barval(nullptr) == 0);
  }

  SECTION("for memmax > 0") {
    info.memmax = 24;
    REQUIRE(mem_with_buffers_barval(nullptr) == Approx(0.25));
  }
}

TEST_CASE("swap_percentage and swap_barval return correct values") {
  info.swap = 6;

  SECTION("for swapmax == 0") {
    info.swapmax = 0;

    REQUIRE(swap_percentage(nullptr) == 0);
    REQUIRE(swap_barval(nullptr) == 0);
  }

  SECTION("for swapmax > 0") {
    info.swapmax = 24;

    REQUIRE(swap_percentage(nullptr) == 25);
    REQUIRE(swap_barval(nullptr) == Approx(0.25));
  }
}
