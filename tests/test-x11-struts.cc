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

#include <X11/Xlib.h>
#include <conky.h>
#include <diskio.h>
#include <cstdio>
#include "catch2/catch_amalgamated.hpp"
#include "config.h"
#include "geometry.h"
#include "gui.h"
#include "mock/mock.hh"
#include "mock/x11-mock.hh"
#include "x11.h"

using namespace conky;

TEST_CASE("x11 set_struts sets correct struts") {
  // Temporarily initialize used globals
  workarea = absolute_rect<int>{vec2i(0, 0), vec2i(600, 800)};
  window.geometry = rect<int>{vec2i(0, 0), vec2i(200, 400)};

  SECTION("for TOP_LEFT alignment") {
    set_struts(alignment::TOP_LEFT);
    mock::x11_change_property full =
        EXPECT_NEXT_CHANGE(mock::x11_change_property);
    REQUIRE(full.property == "_NET_WM_STRUT");

    mock::x11_change_property partial =
        EXPECT_NEXT_CHANGE(mock::x11_change_property);
    REQUIRE(partial.property == "_NET_WM_STRUT_PARTIAL");
  }

  // Reset globals
  window.geometry = rect<int>{};
  workarea = conky::absolute_rect<int>{};
}
