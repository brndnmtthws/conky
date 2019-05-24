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

std::string get_valid_environment_variable_name() {
  if (getenv("HOME") != nullptr) { return "HOME"; }

  // If HOME is not a valid environment variable name, try to get a valid one.
  char *env_var = *environ;

  for (int i = 1; env_var; i++) {
    std::string variable_name(env_var);
    int variable_name_length = variable_name.find('=');
    variable_name = variable_name.substr(0, variable_name_length);

    if (getenv(variable_name.c_str()) != nullptr) { return variable_name; }

    env_var = *(environ + i);
  }

  return "";
}

std::string get_invalid_environment_variable_name() {
  std::string variable_name = "INVALIDVARIABLENAME";

  while (getenv(variable_name.c_str()) != nullptr) {
    variable_name += std::to_string(variable_name.length());
  }

  return variable_name;
}

TEST_CASE("to_real_path becomes homedir", "[to_real_path]") {
  REQUIRE(to_real_path("~/test") == std::string(getenv("HOME")) + "/test");
}

TEST_CASE("environment variables are substituted correctly",
          "[variable_substitute]") {
  std::string valid_name = get_valid_environment_variable_name();
  std::string valid_value = getenv(valid_name.c_str());
  std::string invalid_name = get_invalid_environment_variable_name();

  SECTION("an empty string input returns an empty string") {
    REQUIRE(variable_substitute("") == "");
  }

  SECTION("string in with no $ returns same string") {
    std::string string_alpha = "abcdefghijklmnopqrstuvwxyz";
    std::string string_numbers = "1234567890";
    std::string string_special = "`~!@#$%^&*()-=_+[]{}\\|;:'\",<.>/?";
    std::string string_valid_name = valid_name;

    REQUIRE(variable_substitute(string_alpha) == string_alpha);
    REQUIRE(variable_substitute(string_numbers) == string_numbers);
    REQUIRE(variable_substitute(string_special) == string_special);
    REQUIRE(variable_substitute(string_valid_name) == string_valid_name);
  }

  SECTION("invalid variables are removed from return string") {
    std::string string_in_1 = "a$" + invalid_name + " z";
    std::string string_in_2 = "a${" + invalid_name + "} z";
    std::string string_in_3 = "a${" + invalid_name + " " + valid_name + "} z";
    std::string string_in_4 = "a${ " + valid_name + "} z";
    std::string string_in_5 = "a${" + valid_name + " } z";
    std::string string_in_6 = "a$" + valid_name + "z z";
    std::string string_in_7 = "a$" + invalid_name + "# z";

    REQUIRE(variable_substitute(string_in_1) == "a z");
    REQUIRE(variable_substitute(string_in_2) == "a z");
    REQUIRE(variable_substitute(string_in_3) == "a z");
    REQUIRE(variable_substitute(string_in_4) == "a z");
    REQUIRE(variable_substitute(string_in_5) == "a z");
    REQUIRE(variable_substitute(string_in_6) == "a z");
    REQUIRE(variable_substitute(string_in_7) == "a# z");
  }

  SECTION("valid variable gets replaced in the return string") {
    std::string string_in_1 = "a$" + valid_name + " z";
    std::string string_in_2 = "a${" + valid_name + "} z";
    std::string string_in_3 = "a$" + valid_name + "# z";

    std::string string_var_replaced_1 = "a" + valid_value + " z";
    std::string string_var_replaced_2 = "a" + valid_value + " z";
    std::string string_var_replaced_3 = "a" + valid_value + "# z";

    REQUIRE(variable_substitute(string_in_1) == string_var_replaced_1);
    REQUIRE(variable_substitute(string_in_2) == string_var_replaced_2);
    REQUIRE(variable_substitute(string_in_3) == string_var_replaced_3);
  }

  SECTION("$ without variable is ignored") {
    std::string string_in_1 = "a$#z";
    std::string string_in_2 = "a$2z";

    REQUIRE(variable_substitute(string_in_1) == string_in_1);
    REQUIRE(variable_substitute(string_in_2) == string_in_2);
  }

  SECTION("double $ gets converted to single $ and is passed over") {
    std::string string_in_1 = "a$$sz";
    std::string string_in_2 = "a$$" + valid_name + "z";
    std::string string_out_1 = "a$sz";
    std::string string_out_2 = "a$" + valid_name + "z";

    REQUIRE(variable_substitute(string_in_1) == string_out_1);
    REQUIRE(variable_substitute(string_in_2) == string_out_2);
  }

  SECTION("incomplete variable does not get replaced in return string") {
    std::string string_in = "a${" + valid_name + " z";

    REQUIRE(variable_substitute(string_in) == string_in);
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
