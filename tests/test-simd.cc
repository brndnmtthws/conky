
#include "catch2/catch.hpp"

#include "simd.h"

#include <array>

#ifndef USE_SSE
#error SSE not enabled
#endif USE_SSE
#ifndef USE_SSE2
#error SSE2 not enabled
#endif USE_SSE2

TEST_CASE("SIMD operations work as intended") {
  SECTION("float opeations") {
    using traits_f32x2 = conky::simd::simd_traits<float, 2>;
    auto a = traits_f32x2::from_array(std::array{2.0, 3.0});
    auto b = traits_f32x2::from_array(std::array{4.0, 5.0});

    auto add_result = traits_f32x2::to_array(traits_f32x2::add(a, b));

    REQUIRE(add_result[0] == 8.0);
    REQUIRE(add_result[1] == 15.0);
  }
  SECTION("uint32_t opeations") {}
}
