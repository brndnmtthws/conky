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

#ifdef __linux__
#include <common.h>
#include <content/text_object.h>
#include <sys/resource.h>

#include <cstring>

namespace {
struct file_limit_guard {
  struct rlimit original {};

  ~file_limit_guard() { setrlimit(RLIMIT_NOFILE, &original); }
};

void check_frequency_recovery(void (*print)(struct text_object *, char *,
                                            unsigned int)) {
  struct text_object first {};
  first.data.i = 1;
  struct text_object second {};
  second.data.i = 2;
  char output[64] = {};

  // Establish that the frequency source is accessible before inducing failure.
  print(&first, output, sizeof(output));
  REQUIRE(std::strlen(output) > 0);

  struct rlimit original {};
  REQUIRE(getrlimit(RLIMIT_NOFILE, &original) == 0);
  file_limit_guard guard{original};
  struct rlimit exhausted = original;
  exhausted.rlim_cur = 0;

  // Repeat the failure/recovery cycle to catch state retained across updates.
  for (int cycle = 0; cycle < 2; ++cycle) {
    const int limited = setrlimit(RLIMIT_NOFILE, &exhausted);
    output[0] = '\0';
    if (limited == 0) { print(&first, output, sizeof(output)); }
    // Restore the limit before assertions or further test framework work.
    const int restored = setrlimit(RLIMIT_NOFILE, &original);
    REQUIRE(restored == 0);
    REQUIRE(limited == 0);
    CHECK(output[0] == '\0');

    // A failure in one object must not disable another CPU's output.
    print(&second, output, sizeof(output));
    CHECK(std::strlen(output) > 0);
    output[0] = '\0';
    print(&first, output, sizeof(output));
    CHECK(std::strlen(output) > 0);
  }
}
}  // namespace

TEST_CASE("frequency output recovers after file descriptor exhaustion",
          "[linux][frequency]") {
  SECTION("MHz") { check_frequency_recovery(print_freq); }
  SECTION("GHz") { check_frequency_recovery(print_freq_g); }
}
#endif
