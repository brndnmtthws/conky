#include "catch2/catch.hpp"

#include <algebra.h>
#include <config.h>

TEST_CASE("GetMatchTypeTest - ValidOperators") {
  REQUIRE(get_match_type("a==b") == OP_EQ);
  REQUIRE(get_match_type("a!=b") == OP_NEQ);
  REQUIRE(get_match_type("a>b") == OP_GT);
  REQUIRE(get_match_type("a>=b") == OP_GEQ);
  REQUIRE(get_match_type("a<b") == OP_LT);
  REQUIRE(get_match_type("a<=b") == OP_LEQ);
}

TEST_CASE("GetMatchTypeTest - InvalidOperators") {
  REQUIRE(get_match_type("a=b") == -1);
  REQUIRE(get_match_type("a!b") == -1);
  REQUIRE(get_match_type("a=b") == -1);
}

TEST_CASE("GetMatchTypeTest - NoOperators") {
  REQUIRE(get_match_type("abc") == -1);
  REQUIRE(get_match_type("123") == -1);
  REQUIRE(get_match_type("") == -1);
}

TEST_CASE("find_match_op identifies operators correctly", "[find_match_op]") {
  SECTION("Single character operators") {
    REQUIRE(find_match_op("a < b") == 2);
    REQUIRE(find_match_op("a > b") == 2);
  }

  SECTION("Double character operators") {
    REQUIRE(find_match_op("a == b") == 2);
    REQUIRE(find_match_op("a != b") == 2);
    REQUIRE(find_match_op("a >= b") == 2);
    REQUIRE(find_match_op("a <= b") == 2);
  }

  SECTION("No operator present") { REQUIRE(find_match_op("a b") == -1); }

  SECTION("Invalid operators") {
    REQUIRE(find_match_op("a = b") == -1);
    REQUIRE(find_match_op("a ! b") == -1);
  }

  SECTION("Operators at different positions") {
    REQUIRE(find_match_op("x == y") == 2);
    REQUIRE(find_match_op("x != y") == 2);
    REQUIRE(find_match_op("x >= y") == 2);
    REQUIRE(find_match_op("x <= y") == 2);
    REQUIRE(find_match_op("x < y") == 2);
    REQUIRE(find_match_op("x > y") == 2);
  }

  SECTION("Empty string") { REQUIRE(find_match_op("") == -1); }
}

TEST_CASE("GetMatchTypeTest - More test cases") {
  REQUIRE(get_match_type("\"a!b\" == \"ab\"") == -1);     // "a!b" == "ab"
  REQUIRE(get_match_type("\"a!/b\" == \"a!/b\"") == -1);  // "a!/b" == "a!/b"
  REQUIRE(get_match_type("\"a!=/a==b\" == \"a!=/a==b\"") ==
          6);  // "a!=/a==b" == "a!=/a==b"
  REQUIRE(get_match_type("\"a!=/b==b\" == \"a!==/a==b\"") ==
          6);  // "a!=/b==b" == "a!==/a==b"
  REQUIRE(get_match_type("\"a!======b\" == \"!==/==\"") ==
          6);  // "a!======b" == "!==/=="
  REQUIRE(get_match_type("\" !=<>==\" == \" !=<>==\"") ==
          6);  // " !=<>==" == " !=<>=="
  REQUIRE(get_match_type("\"a!=><==\" < \"b!=><==\"") ==
          6);  // "a!=><==" < "b!=><=="
  REQUIRE(get_match_type("\"!=<>==\" == \"!=<>==\"") ==
          6);                                       // "!=<>==" == "!=<>=="
  REQUIRE(get_match_type("\"=\" == \"=\"") == -1);  // "=" == "="
  REQUIRE(get_match_type("\"FRITZ!Box 7520 HI\" == \"off/any\"") ==
          -1);  // "FRITZ!Box 7520 HI" == "off/any"
}

TEST_CASE("CompareTest - ValidOperators") {
  REQUIRE(compare("\"a\"==\"b\"") == 0);
  REQUIRE(compare("\"a\"!=\"b\"") == 1);
  REQUIRE(compare("\"a\">\"b\"") == 0);
  REQUIRE(compare("\"a\">=\"b\"") == 0);
  REQUIRE(compare("\"a\"<\"b\"") == 1);
  REQUIRE(compare("\"a\"<=\"b\"") == 1);
}
TEST_CASE("CompareTest - ValidOperators with Integers") {
  REQUIRE(compare("1==1") == 1);
  REQUIRE(compare("1!=2") == 1);
  REQUIRE(compare("2>1") == 1);
  REQUIRE(compare("2>=1") == 1);
  REQUIRE(compare("1<2") == 1);
  REQUIRE(compare("1<=2") == 1);
}

TEST_CASE("CompareTest - ValidOperators with Doubles") {
  REQUIRE(compare("1.0==1.0") == 1);
  REQUIRE(compare("1.0!=2.0") == 1);
  REQUIRE(compare("2.0>1.0") == 1);
  REQUIRE(compare("2.0>=1.0") == 1);
  REQUIRE(compare("1.0<2.0") == 1);
  REQUIRE(compare("1.0<=2.0") == 1);
}

TEST_CASE("CompareTest - InvalidOperators") {
  REQUIRE(compare("\"a\"=\"b\"") == -2);
  REQUIRE(compare("\"a\"!\"b\"") == -2);
  REQUIRE(compare("\"a\"=\"b\"") == -2);
}

TEST_CASE("CompareTest - NoOperators") {
  REQUIRE(compare("abc") == -2);
  REQUIRE(compare("123") == -2);
  REQUIRE(compare("") == -2);
}

TEST_CASE("CompareTest - More test cases") {
  REQUIRE(compare("\"a!b\" == \"ab\"") == -2);     // "a!b" == "ab"
  REQUIRE(compare("\"a!/b\" == \"a!/b\"") == -2);  // "a!/b" == "a!/b"
  REQUIRE(compare("\"a!=/a==b\" == \"a!=/a==b\"") ==
          -2);  // "a!=/a==b" == "a!=/a==b"
  REQUIRE(compare("\"a!=/b==b\" == \"a!==/a==b\"") ==
          -2);  // "a!=/b==b" == "a!==/a==b"
  REQUIRE(compare("\"a!======b\" == \"!==/==\"") ==
          -2);  // "a!======b" == "!==/=="
  REQUIRE(compare("\" !=<>==\" == \" !=<>==\"") ==
          -2);  // " !=<>==" == " !=<>=="
  REQUIRE(compare("\"a!=><==\" < \"b!=><==\"") == -2);  // "a!=><==" < "b!=><=="
  REQUIRE(compare("\"!=<>==\" == \"!=<>==\"") == -2);   // "!=<>==" == "!=<>=="
  REQUIRE(compare("\"=\" == \"=\"") == -2);             // "=" == "="
  REQUIRE(compare("\"FRITZ!Box 7520 HI\" == \"off/any\"") ==
          -2);  // "FRITZ!Box 7520 HI" == "off/any"
}
