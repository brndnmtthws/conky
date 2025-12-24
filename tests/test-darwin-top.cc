/*
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
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

#include "data/os/darwin_top_helpers.h"

#include <climits>

#include "catch2/catch_amalgamated.hpp"

TEST_CASE("darwin cpu sample overall totals are reset before summing",
          "[darwin][top]") {
  struct cpusample sample[3] = {};

  sample[0].totalUserTime = 100;
  sample[0].totalSystemTime = 200;
  sample[0].totalIdleTime = 300;

  sample[1].totalUserTime = 7;
  sample[1].totalSystemTime = 11;
  sample[1].totalIdleTime = 13;

  sample[2].totalUserTime = 17;
  sample[2].totalSystemTime = 19;
  sample[2].totalIdleTime = 23;

  sum_cpu_sample_overall(sample, 2);

  REQUIRE(sample[0].totalUserTime == 24);
  REQUIRE(sample[0].totalSystemTime == 30);
  REQUIRE(sample[0].totalIdleTime == 36);
}

TEST_CASE("darwin cpu total delta initializes safely", "[darwin][top]") {
  unsigned long previous = ULONG_MAX;

  uint64_t delta = cpu_total_delta(1000, &previous);
  REQUIRE(delta == 0);
  REQUIRE(previous == 1000);

  delta = cpu_total_delta(1600, &previous);
  REQUIRE(delta == 600);
  REQUIRE(previous == 1600);
}

TEST_CASE("darwin mach ticks convert to centiseconds", "[darwin][top]") {
  REQUIRE(mach_ticks_to_centis(0, 1, 1) == 0);
  REQUIRE(mach_ticks_to_centis(10000000, 1, 1) == 1);
  REQUIRE(mach_ticks_to_centis(20000000, 1, 1) == 2);

  REQUIRE(mach_ticks_to_centis(240000, 125, 3) == 1);
  REQUIRE(mach_ticks_to_centis(480000, 125, 3) == 2);
}
