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

#include <hsv_gradient.h>
#include <conky.h>
#include <lua-config.hh>


int testColor(long *rgb, int scale) {
  long hsv[3];
  long rgb1[3];
  long rgb2[3];
  long rgb3[3];

  rgb1[0] = to_decimal_scale(rgb[0], scale);
  rgb1[1] = to_decimal_scale(rgb[1], scale);
  rgb1[2] = to_decimal_scale(rgb[2], scale);

  scaled_rgb_to_scaled_hsv(rgb1, hsv);
  scaled_hsv_to_scaled_rgb(hsv, rgb2);

  rgb3[0] = from_decimal_scale(rgb2[0], scale);
  rgb3[1] = from_decimal_scale(rgb2[1], scale);
  rgb3[2] = from_decimal_scale(rgb2[2], scale);

  return (rgb[0] != rgb3[0] || rgb[1] != rgb3[1] || rgb[2] != rgb3[2]);
}

TEST_CASE("hsv gradient tests") {
  SECTION("rgb -> hsv -> rgb should returns original value") {
    int failedCount = 0;
    long rgb1[3];

    for (int i = 0; i < 256 && failedCount < 10; i++) {
      for (int j = 0; j < 256 && failedCount < 10; j++) {
        for (int k = 0; k < 256 && failedCount < 10; k++) {
          rgb1[0] = i;
          rgb1[1] = j;
          rgb1[2] = k;
          failedCount += testColor(rgb1, 255);
        }
      }
    }
    REQUIRE(failedCount == 0);

  }
}
