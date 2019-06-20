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

#include <config.h>
#include <diskio.h>
#include <conky.h>

#if BUILD_X11
TEST_CASE("diskiographval returns correct value") {
  struct text_object obj;

  SECTION("for missing data") {
    REQUIRE(diskiographval(&obj) == 0);
  }

  SECTION("for valid data") {
    diskio_stat *stat = new diskio_stat;
    stat->current = 2.5;
    
    obj.data.opaque = stat;

    REQUIRE(diskiographval(&obj) == Approx(2.5));
  }
}
#endif
