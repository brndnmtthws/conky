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
#include <array>
#include <cstdint>
#include <cstdio>
#include "geometry.h"
#include "gui.h"
#include "mock/mock.hh"
#include "mock/x11-mock.hh"
#include "x11.h"

using namespace conky;

struct x11_strut {
  std::uint32_t left;
  std::uint32_t right;
  std::uint32_t top;
  std::uint32_t bottom;
};
x11_strut expect_strut(mock::x11_change_property &change) {
  REQUIRE(change.element_count() == 4);
  auto result = EXPECT_X11_ARRAY(change.data(), change.type(), change.format(),
                                 change.element_count(), std::uint32_t, 4);
  return x11_strut{result[0], result[1], result[2], result[3]};
}

struct x11_strut_partial {
  std::uint32_t left;
  std::uint32_t right;
  std::uint32_t top;
  std::uint32_t bottom;
  std::uint32_t left_start_y;
  std::uint32_t left_end_y;
  std::uint32_t right_start_y;
  std::uint32_t right_end_y;
  std::uint32_t top_start_x;
  std::uint32_t top_end_x;
  std::uint32_t bottom_start_x;
  std::uint32_t bottom_end_x;
};
x11_strut_partial expect_strut_partial(mock::x11_change_property &change) {
  REQUIRE(change.element_count() == 12);
  auto result = EXPECT_X11_ARRAY(change.data(), change.type(), change.format(),
                                 change.element_count(), std::uint32_t, 12);
  return x11_strut_partial{
      result[0], result[1], result[2], result[3], result[4],  result[5],
      result[6], result[7], result[8], result[9], result[10], result[11],
  };
}

TEST_CASE("x11 set_struts sets correct struts") {
  // Temporarily initialize used globals
  workarea = absolute_rect<int>{vec2i(0, 0), vec2i(600, 800)};
  window.geometry = rect<int>{vec2i(0, 0), vec2i(200, 600)};

  SECTION("for TOP_LEFT alignment") {
    set_struts(alignment::TOP_LEFT);
    mock::x11_change_property full =
        EXPECT_NEXT_CHANGE(mock::x11_change_property);
    REQUIRE(full.property_name() == "_NET_WM_STRUT");
    REQUIRE(full.type() == mock::x11_property_type::CARDINAL);

    auto strut_bounds = expect_strut(full);
    // CHECK(strut_bounds.left == 0);
    // CHECK(strut_bounds.right == workarea.width() - window.geometry.width());
    // CHECK(strut_bounds.top == 0);
    // CHECK(strut_bounds.bottom == workarea.height() - window.geometry.height());

    mock::x11_change_property partial =
        EXPECT_NEXT_CHANGE(mock::x11_change_property);
    REQUIRE(partial.property_name() == "_NET_WM_STRUT_PARTIAL");
    REQUIRE(partial.type() == mock::x11_property_type::CARDINAL);
    auto strut_partial_bounds = expect_strut_partial(partial);
    // CHECK(strut_partial_bounds.left == 0);
    // CHECK(strut_partial_bounds.right ==
    //       workarea.width() - window.geometry.width());
    // CHECK(strut_partial_bounds.top == 0);
    // CHECK(strut_partial_bounds.bottom ==
    //       workarea.height() - window.geometry.height());
  }

  // Reset globals
  window.geometry = rect<int>{};
  workarea = conky::absolute_rect<int>{};
}
