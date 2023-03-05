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
 * Copyright (c) 2005-2021 Brenden Matthews, Philip Kovacs, et. al.
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

#include <tuple>
#include "catch2/catch.hpp"
#include "conky.h"
#include "lua-config.hh"
#include "specials.h"

#ifdef BUILD_GUI

#define SF_SHOWLOG (1 << 1)

// Specific value doesn't matter, but we check if the same one comes back
constexpr double default_scale = M_PI;

constexpr double default_width = 0;
constexpr double default_height = 25;

struct graph {
  int id;
  char flags;
  int width, height;
  bool colours_set;
  Colour first_colour, last_colour;
  double scale;
  char tempgrad;
};

static std::pair<struct graph, char *> test_parse(const char *s) {
  struct text_object obj;
  char *command = scan_graph(&obj, s, default_scale);
  auto g = static_cast<struct graph *>(obj.special_data);
  struct graph graph = *g;
  free(g);
  return {graph, command};
}

std::string unquote(const std::string &s) {
  auto out = s;
  out.erase(remove(out.begin(), out.end(), '\"'), out.end());
  return out;
}

TEST_CASE("scan_graph correctly parses input strings") {
  state = std::make_unique<lua::state>();
  conky::export_symbols(*state);

  SECTION("Trivial parse") {
    auto [g, command] = test_parse("");

    REQUIRE(g.width == default_width);
    REQUIRE(g.height == default_height);
  }

  /* test parsing combinations of options */
  const char *command_options[] = {"\"foo bar\"", "\"ls -t\"", "foo-test", ""};
  const char *size_options[][2] = {{"30", "500"}, {"80", ""}, {"", ""}};
  const char *color_options[][2] = {{"orange", "blue"},
                                    {"#deadff", "#392014"},
                                    {"000000", "000000"},
                                    {"", ""}};
  const char *scale_options[] = {"0.5", ""};

  SECTION(
      "subset of [command] [height,width] [color1 color2] [scale] [-t] [-l]") {
    for (auto command : command_options) {
      for (auto size : size_options) {
        for (auto colors : color_options) {
          for (auto scale : scale_options) {
            bool ends_at_first_size = false;

            /* build an argument string by combining the selected options */
            std::string s;
            s += command;
            s += " ";
            if (*size[0] != '\0') {
              s += size[0];
              s += ",";
              if (*size[1] == '\0') {
                /* if the size is just a height, it has to be the end of the
                 * argument string */
                ends_at_first_size = true;
              } else {
                s += size[1];
              }
              s += " ";
            }
            if (!ends_at_first_size) {
              s += colors[0];
              s += " ";
              s += colors[1];
              s += " ";
              s += scale;
            }

            /* parse the argument string */
            auto [g, parsed_command] = test_parse(s.c_str());

            printf("command: %s\n", s.c_str());

            /* validate parsing of each component */
            if (*command == '\0') {
              REQUIRE(parsed_command == nullptr);
            } else {
              REQUIRE(parsed_command != nullptr);
              REQUIRE(std::string(parsed_command) == unquote(command));
            }

            if (*size[0] == '\0') {
              REQUIRE(g.width == default_width);
              REQUIRE(g.height == default_height);
            } else {
              REQUIRE(g.height == atoi(size[0]));
              REQUIRE(g.width == atoi(size[1]));
            }

            /* if second half of size is empty, no subsequent values should be
             * set
             */
            if (ends_at_first_size) {
              REQUIRE(g.colours_set == false);
              continue;
            }

            if (*colors[0] == '\0') {
              REQUIRE(g.colours_set == false);
            } else {
              REQUIRE(g.colours_set == true);
              REQUIRE(g.first_colour == parse_color(colors[0]));
              REQUIRE(g.last_colour == parse_color(colors[1]));
            }

            if (*scale == '\0') {
              REQUIRE(g.scale == default_scale);
            } else {
              REQUIRE(g.scale == 0.5);
            }

            REQUIRE(g.flags == 0);
            REQUIRE(g.tempgrad == 0);
          }
        }
      }
    }
  }

  SECTION("[height,width] [color1 color2] [scale] [-t] [-l]") {
    auto [g, command] = test_parse("21,340 orange blue 0.5 -t -l");

    REQUIRE(command == nullptr);
    REQUIRE(g.width == 340);
    REQUIRE(g.height == 21);
    REQUIRE(g.colours_set == true);
    REQUIRE(g.scale == 0.5);
    REQUIRE(g.flags == SF_SHOWLOG);
    REQUIRE(g.tempgrad == true);
  }

  SECTION("-t location") {
    {
      auto [g, command] = test_parse("\"ls -t\" 21,340 red blue 0.5");
      REQUIRE(g.tempgrad == false);
    }
    {
      auto [g, command] = test_parse("foo-test 21,340 red blue 0.5");
      REQUIRE(g.tempgrad == false);
    }
    {
      auto [g, command] = test_parse("foo-test -t 21,340 red blue 0.5");
      REQUIRE(g.tempgrad == true);
    }
    {
      auto [g, command] = test_parse("21,340 -t red blue 0.5");
      REQUIRE(g.tempgrad == true);
    }
  }
}

TEST_CASE("scan_command correctly parses input strings") {
  SECTION("parse commands") {
    const char *command_options[][2] = {{"\"foo bar\"", "foo bar"},
                                        {"\"foo bar\"\tbaz", "foo bar"},
                                        {"\"foo bar\"\nbaz", "foo bar"},
                                        {"\"foo bar\" baz", "foo bar"},
                                        {"one two", "one"},
                                        {"\"ls -t\"", "ls -t"},
                                        {"\"ls -t\" 4309", "ls -t"},
                                        {"foo-test", "foo-test"},
                                        {"foo-test a b c", "foo-test"},
                                        {"", ""}};
    for (auto [command, expected_parsed] : command_options) {
      auto [parsed, len] = scan_command(command);
      REQUIRE(std::string(parsed) == expected_parsed);
      if (command[0] == '"') {
        REQUIRE(len == strlen(expected_parsed) + 2);
      } else {
        REQUIRE(len == strlen(expected_parsed));
      }
    }
  }
}

#endif /* BUILD_GUI */
