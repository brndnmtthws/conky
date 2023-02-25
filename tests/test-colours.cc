#include "catch2/catch.hpp"

#include <colours.h>
#include <config.h>

TEST_CASE("parse_color correctly parses colours", "[colours][parse_color]") {
  SECTION("parsing hex red") {
    auto colour = parse_color("#ff0000");
    REQUIRE(colour.alpha == 255);
    REQUIRE(colour.red == 255);
    REQUIRE(colour.green == 0);
    REQUIRE(colour.blue == 0);
  }

  SECTION("parsing hex green") {
    auto colour = parse_color("#00ff00");
    REQUIRE(colour.alpha == 255);
    REQUIRE(colour.red == 0);
    REQUIRE(colour.green == 255);
    REQUIRE(colour.blue == 0);
  }

  SECTION("parsing hex blue") {
    auto colour = parse_color("#0000ff");
    REQUIRE(colour.alpha == 255);
    REQUIRE(colour.red == 0);
    REQUIRE(colour.green == 0);
    REQUIRE(colour.blue == 255);
  }

  SECTION("parsing red") {
    auto colour = parse_color("ff0000");
    REQUIRE(colour.alpha == 255);
    REQUIRE(colour.red == 255);
    REQUIRE(colour.green == 0);
    REQUIRE(colour.blue == 0);
  }

  SECTION("parsing green") {
    auto colour = parse_color("00ff00");
    REQUIRE(colour.alpha == 255);
    REQUIRE(colour.red == 0);
    REQUIRE(colour.green == 255);
    REQUIRE(colour.blue == 0);
  }

  SECTION("parsing blue") {
    auto colour = parse_color("0000ff");
    REQUIRE(colour.alpha == 255);
    REQUIRE(colour.red == 0);
    REQUIRE(colour.green == 0);
    REQUIRE(colour.blue == 255);
  }

  SECTION("argb values produce the expected result") {
    auto colour = Colour::from_argb32(0x11223344);
    REQUIRE(colour.alpha == 0x11);
    REQUIRE(colour.red == 0x22);
    REQUIRE(colour.green == 0x33);
    REQUIRE(colour.blue == 0x44);
  }

  SECTION("it parses the colour 'red'") {
    auto colour = parse_color("red");
    REQUIRE(colour.alpha == 255);
    REQUIRE(colour.red == 255);
    REQUIRE(colour.green == 0);
    REQUIRE(colour.blue == 0);
  }

  SECTION("it parses the colour 'green'") {
    auto colour = parse_color("green");
    REQUIRE(colour.alpha == 255);
    REQUIRE(colour.red == 0);
    REQUIRE(colour.green == 255);
    REQUIRE(colour.blue == 0);
  }

  SECTION("it parses the colour 'blue'") {
    auto colour = parse_color("blue");
    REQUIRE(colour.alpha == 255);
    REQUIRE(colour.red == 0);
    REQUIRE(colour.green == 0);
    REQUIRE(colour.blue == 255);
  }

  SECTION("two identical colours should be equal") {
    auto c = GENERATE(take(100, random((uint32_t)0, (uint32_t)0xffffffff)));
    auto colour1 = Colour::from_argb32(c);
    auto colour2 = Colour::from_argb32(c);
    REQUIRE(colour1 == colour2);
  }
}
