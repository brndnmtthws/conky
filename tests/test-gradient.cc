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

#include <colours.h>
#include <conky.h>
#include <gradient.h>

#include <iomanip>
#include <iostream>

const int width = 4;
const Colour colour = Colour::from_argb32(0xff996633);  // brown
const long expected_hue = 256;
const long expected_value = 0x99;   // max(0x99, 0x66, 0x33)
const long expected_chroma = 0x66;  // (0x99 - 0x33)
const long expected_luma = 20712665L;
const long expected_saturation = 122880L;
const long expected_red = 0x99;
const long expected_green = 0x66;
const long expected_blue = 0x33;

const long full_scale = conky::gradient_factory::SCALE360;

std::ostream& operator<<(std::ostream& s, const Colour& c) {
  s << '#';
  s << std::setfill('0');
  s << std::setw(2);
  s << std::setbase(16);
  s << (int)c.alpha << (int)c.red << (int)c.green << (int)c.blue;
  return s;
}

TEST_CASE("gradient_factory::convert_from_rgb returns correct value") {
#ifdef BUILD_X11
  state = nullptr;
#endif
  SECTION("rgb_gradient_factory") {
    auto factory = new conky::rgb_gradient_factory(width, colour, colour);
    long result[3];

    factory->convert_from_rgb(colour, result);

    SECTION("red") { REQUIRE(result[0] == expected_red * full_scale); }
    SECTION("green") { REQUIRE(result[1] == expected_green * full_scale); }
    SECTION("blue") { REQUIRE(result[2] == expected_blue * full_scale); }

    delete factory;
  }

  SECTION("hsv_gradient_factory") {
    auto factory = new conky::hsv_gradient_factory(width, colour, colour);
    long result[3];

    factory->convert_from_rgb(colour, result);

    SECTION("hue") { REQUIRE(result[0] == expected_hue * 60); }
    SECTION("saturation") { REQUIRE(result[1] == expected_saturation); }
    SECTION("value") { REQUIRE(result[2] == expected_value * full_scale); }

    delete factory;
  }

  SECTION("hcl_gradient_factory") {
    auto factory = new conky::hcl_gradient_factory(width, colour, colour);
    long result[3];

    factory->convert_from_rgb(colour, result);

    SECTION("hue") { REQUIRE(result[0] == expected_hue * 60); }
    SECTION("chroma") { REQUIRE(result[1] == expected_chroma * full_scale); }
    SECTION("luma") { REQUIRE(result[2] == expected_luma); }

    delete factory;
  }
}

TEST_CASE(
    "gradient_factory should convert to and from rgb "
    "and get the initial value") {
  SECTION("rgb_gradient_factory") {
    long tmp[3];
    auto factory = new conky::rgb_gradient_factory(width, colour, colour);
    factory->convert_from_rgb(colour, tmp);
    auto result = factory->convert_to_rgb(tmp);

    REQUIRE(result == colour);

    delete factory;
  }

  SECTION("hsv_gradient_factory") {
    long tmp[3];
    auto factory = new conky::hsv_gradient_factory(width, colour, colour);
    factory->convert_from_rgb(colour, tmp);
    auto result = factory->convert_to_rgb(tmp);

    REQUIRE(result == colour);

    delete factory;
  }

  SECTION("hcl_gradient_factory") {
    long tmp[3];
    auto factory = new conky::hcl_gradient_factory(width, colour, colour);
    factory->convert_from_rgb(colour, tmp);
    auto result = factory->convert_to_rgb(tmp);

    REQUIRE(result == colour);

    delete factory;
  }
}
