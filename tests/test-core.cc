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

#include <core.h>

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
