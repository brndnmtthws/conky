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
const long colour = 0x996633; // brown
const long colour_hue = 30;
const long colour_value = 0x99;

const long full_scale = conky::gradient_factory::SCALE360;

TEST_CASE("gradient_factory::convert_from_rgb returns correct value") {
  state = nullptr;
  SECTION("rgb_gradient_factory") {
    auto factory = new conky::rgb_gradient_factory(width, colour, colour);
    long result[3];

    factory->convert_from_rgb(colour, result);

    SECTION("red") {
      REQUIRE(result[0] == 0x99 * full_scale);
    }
    SECTION("green") {
      REQUIRE(result[1] == 0x66 * full_scale);
    }
    SECTION("blue") {
      REQUIRE(result[2] == 0x33 * full_scale);
    }

    delete factory;
  }

  SECTION("hsv_gradient_factory") {
    auto factory = new conky::hsv_gradient_factory(width, colour, colour);
    long result[3];

    factory->convert_from_rgb(colour, result);

    SECTION("hue") {
      REQUIRE(result[0] == colour_hue * conky::gradient_factory::SCALE);
    }
    SECTION("saturation") {
      REQUIRE(result[1] == conky::gradient_factory::SCALE * 240L);
    }
    SECTION("value") {
      REQUIRE(result[2] == colour_value * full_scale);
    }

    delete factory;
  }

  SECTION("hcl_gradient_factory") {
    auto factory = new conky::hcl_gradient_factory(width, colour, colour);
    long result[3];

    factory->convert_from_rgb(colour, result);

    SECTION("hue") {
      REQUIRE(result[0] == colour_hue * conky::gradient_factory::SCALE);
    }
    SECTION("chroma") {
      REQUIRE(result[1] == 0x66 * full_scale);
    }
    SECTION("luma") {
      REQUIRE(result[2] == 20712665L);
    }

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
