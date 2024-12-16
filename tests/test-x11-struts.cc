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
x11_strut expect_strut() {
  mock::x11_change_property change =
      EXPECT_NEXT_CHANGE(mock::x11_change_property);
  REQUIRE(change.property_name() == "_NET_WM_STRUT");
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
x11_strut_partial expect_strut_partial() {
  mock::x11_change_property change =
      EXPECT_NEXT_CHANGE(mock::x11_change_property);
  REQUIRE(change.property_name() == "_NET_WM_STRUT_PARTIAL");
  REQUIRE(change.element_count() == 12);
  auto result = EXPECT_X11_ARRAY(change.data(), change.type(), change.format(),
                                 change.element_count(), std::uint32_t, 12);
  return x11_strut_partial{
      result[0], result[1], result[2], result[3], result[4],  result[5],
      result[6], result[7], result[8], result[9], result[10], result[11],
  };
}

// from conky.cc
extern conky::vec2i text_size;
extern void apply_window_alignment(conky::vec2i &xy, alignment align);

TEST_CASE("x11 set_struts sets correct struts") {
  // Temporarily initialize used globals
  workarea = absolute_rect<int>{vec2i(0, 0), vec2i(600, 800)};
  const auto half_width = workarea.width() / 2;
  const auto half_height = workarea.height() / 2;
  window.geometry = rect<int>{vec2i(0, 0), vec2i(200, 600)};
  auto &xy = *reinterpret_cast<vec2i *>(&window.geometry);

  SECTION("for TOP_LEFT alignment") {
    set_struts(alignment::TOP_LEFT);
    apply_window_alignment(xy, alignment::TOP_LEFT);
    auto strut = expect_strut();
    CHECK(strut.left == 0);
    CHECK(strut.right == 0);
    CHECK(strut.top == window.geometry.end_y());
    CHECK(strut.bottom == 0);

    auto strut_partial = expect_strut_partial();
    CHECK(strut_partial.left == 0);
    CHECK(strut_partial.right == 0);
    CHECK(strut_partial.top == window.geometry.end_y());
    CHECK(strut_partial.bottom == 0);
    CHECK(strut_partial.left_start_y == 0);
    CHECK(strut_partial.left_end_y == 0);
    CHECK(strut_partial.right_start_y == 0);
    CHECK(strut_partial.right_end_y == 0);
    CHECK(strut_partial.top_start_x == window.geometry.x());
    CHECK(strut_partial.top_end_x == window.geometry.end_x());
    CHECK(strut_partial.bottom_start_x == 0);
    CHECK(strut_partial.bottom_end_x == 0);
    EXPECT_NO_MORE_CHANGES();
  }

  SECTION("for TOP_MIDDLE alignment") {
    set_struts(alignment::TOP_MIDDLE);
    apply_window_alignment(xy, alignment::TOP_MIDDLE);
    auto strut = expect_strut();
    CHECK(strut.left == 0);
    CHECK(strut.right == 0);
    CHECK(strut.top == window.geometry.end_y());
    CHECK(strut.bottom == 0);

    auto strut_partial = expect_strut_partial();
    CHECK(strut_partial.left == 0);
    CHECK(strut_partial.right == 0);
    CHECK(strut_partial.top == window.geometry.end_y());
    CHECK(strut_partial.bottom == 0);
    CHECK(strut_partial.left_start_y == 0);
    CHECK(strut_partial.left_end_y == 0);
    CHECK(strut_partial.right_start_y == 0);
    CHECK(strut_partial.right_end_y == 0);
    CHECK(strut_partial.top_start_x == window.geometry.x());
    CHECK(strut_partial.top_end_x == window.geometry.end_x());
    CHECK(strut_partial.bottom_start_x == 0);
    CHECK(strut_partial.bottom_end_x == 0);
    EXPECT_NO_MORE_CHANGES();
  }

  // Reset globals
  window.geometry = rect<int>{};
  workarea = conky::absolute_rect<int>{};
}
