/*
 * Conky, a system monitor, based on torsmo
 *
 * This file is licensed under GPLv3. See COPYING for details.
 */

#include "config.h"

#include "catch2/catch.hpp"
#include "data/exec.h"

TEST_CASE("exec callbacks distinguish wait modes", "[exec][update_cb]") {
  SECTION("threaded command registered first") {
    auto threaded =
        conky::register_cb<exec_cb>(5, false, "printf threaded-first");
    auto blocking =
        conky::register_cb<exec_cb>(1, true, "printf threaded-first");

    REQUIRE(&*threaded != &*blocking);
    REQUIRE(&*threaded ==
            &*conky::register_cb<exec_cb>(2, false, "printf threaded-first"));
    REQUIRE(&*blocking ==
            &*conky::register_cb<exec_cb>(3, true, "printf threaded-first"));
  }

  SECTION("blocking command registered first") {
    auto blocking =
        conky::register_cb<exec_cb>(1, true, "printf blocking-first");
    auto threaded =
        conky::register_cb<exec_cb>(5, false, "printf blocking-first");

    REQUIRE(&*threaded != &*blocking);
    REQUIRE(&*blocking ==
            &*conky::register_cb<exec_cb>(3, true, "printf blocking-first"));
    REQUIRE(&*threaded ==
            &*conky::register_cb<exec_cb>(2, false, "printf blocking-first"));
  }
}

TEST_CASE("exec callbacks still share identical commands and wait modes",
          "[exec][update_cb]") {
  const bool wait = GENERATE(false, true);
  auto slow = conky::register_cb<exec_cb>(10, wait, "printf shared");
  auto fast = conky::register_cb<exec_cb>(1, wait, "printf shared");
  auto other = conky::register_cb<exec_cb>(1, wait, "printf different");

  REQUIRE(&*slow == &*fast);
  REQUIRE(&*slow != &*other);
}
