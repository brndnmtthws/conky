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

#include "catch2/catch.hpp"

#include <conky.h>
#include <gradient.h>

const int width = 4;
#ifdef BUILD_X11               // 24-bit color depth
const long colour = 0x996633;  // brown
const long expected_hue = 256;
const long expected_value = 0x99;   // max(0x99, 0x66, 0x33)
const long expected_chroma = 0x66;  // (0x99 - 0x33)
const long expected_luma = 20712665L;
const long expected_saturation = 122880L;
const long expected_red = 0x99;
const long expected_green = 0x66;
const long expected_blue = 0x33;
#else  // 16-bit color depth
const long colour = 0x99A6;  // brown
const long expected_hue = 275;
const long expected_value = 0x13;   // max(0x13, 0x0d, 0x06)
const long expected_chroma = 0x0d;  // (0x1a - 0x06)
const long expected_luma = 2610173L;
const long expected_saturation = 126113L;
const long expected_red = 0x13;
const long expected_green = 0x0d;
const long expected_blue = 0x06;
#endif

const long full_scale = conky::gradient_factory::SCALE360;

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

/*
 * Due to lack of precision, the HSV and HCL functions are not reversible
 * if color depth is less than 24-bit
 */
#ifdef BUILD_X11
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
#endif
}
