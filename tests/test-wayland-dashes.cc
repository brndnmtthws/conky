#include "catch2/catch.hpp"
#include "config.h"

#ifdef BUILD_WAYLAND
#include <cairo.h>

#include <memory>

#include "output/wayland-dashes.hh"

namespace {
struct dash_context {
  std::unique_ptr<cairo_surface_t, decltype(&cairo_surface_destroy)> surface{
      cairo_image_surface_create(CAIRO_FORMAT_A8, 32, 16),
      cairo_surface_destroy};
  std::unique_ptr<cairo_t, decltype(&cairo_destroy)> cr{
      cairo_create(surface.get()), cairo_destroy};

  unsigned char pixel(int x, int y) {
    cairo_surface_flush(surface.get());
    return cairo_image_surface_get_data(
        surface.get())[y * cairo_image_surface_get_stride(surface.get()) + x];
  }
};
}  // namespace

TEST_CASE("wayland_dash_lengths", "[wayland][dashes]") {
  dash_context ctx;
  // Heap allocation makes a string scan past the two-byte input observable
  // under AddressSanitizer, even if the adjacent stack happens to contain zero.
  auto pattern = std::make_unique<char[]>(2);
  for (unsigned int length : {1u, 4u, 127u, 128u, 255u}) {
    pattern[0] = static_cast<char>(length);
    pattern[1] = static_cast<char>(length);
    conky::set_cairo_dashes(ctx.cr.get(), pattern.get());
    REQUIRE(cairo_status(ctx.cr.get()) == CAIRO_STATUS_SUCCESS);
    REQUIRE(cairo_get_dash_count(ctx.cr.get()) == 2);
    double dashes[2], offset;
    cairo_get_dash(ctx.cr.get(), dashes, &offset);
    CHECK(dashes[0] == length);
    CHECK(dashes[1] == length);
    CHECK(offset == 0);
  }

  SECTION("dash and gap lengths are independent") {
    pattern[0] = 3;
    pattern[1] = 7;
    conky::set_cairo_dashes(ctx.cr.get(), pattern.get());
    REQUIRE(cairo_status(ctx.cr.get()) == CAIRO_STATUS_SUCCESS);
    REQUIRE(cairo_get_dash_count(ctx.cr.get()) == 2);
    double dashes[2], offset;
    cairo_get_dash(ctx.cr.get(), dashes, &offset);
    CHECK(dashes[0] == 3);
    CHECK(dashes[1] == 7);
  }
  SECTION("zero bytes are lengths, not string terminators") {
    pattern[0] = 0;
    pattern[1] = 4;
    conky::set_cairo_dashes(ctx.cr.get(), pattern.get());
    REQUIRE(cairo_status(ctx.cr.get()) == CAIRO_STATUS_SUCCESS);
    REQUIRE(cairo_get_dash_count(ctx.cr.get()) == 2);
    double dashes[2], offset;
    cairo_get_dash(ctx.cr.get(), dashes, &offset);
    CHECK(dashes[0] == 0);
    CHECK(dashes[1] == 4);
  }
  SECTION("all-zero input leaves a usable solid context") {
    pattern[0] = pattern[1] = 0;
    conky::set_cairo_dashes(ctx.cr.get(), pattern.get());
    CHECK(cairo_status(ctx.cr.get()) == CAIRO_STATUS_SUCCESS);
    CHECK(cairo_get_dash_count(ctx.cr.get()) == 0);
  }
}

TEST_CASE("wayland_dash_raster", "[wayland][dashes]") {
  dash_context ctx;
  char pattern[2] = {4, 4};
  conky::set_cairo_dashes(ctx.cr.get(), pattern);
  cairo_set_source_rgba(ctx.cr.get(), 1, 1, 1, 1);
  cairo_set_line_width(ctx.cr.get(), 1);
  SECTION("stippled horizontal rule") {
    cairo_move_to(ctx.cr.get(), 0, 2.5);
    cairo_line_to(ctx.cr.get(), 32, 2.5);
  }
  SECTION("stippled border") {
    cairo_rectangle(ctx.cr.get(), 0.5, 2.5, 31, 12);
  }
  cairo_stroke(ctx.cr.get());
  REQUIRE(cairo_status(ctx.cr.get()) == CAIRO_STATUS_SUCCESS);
  CHECK(ctx.pixel(2, 2) == 255);
  CHECK(ctx.pixel(6, 2) == 0);
  CHECK(ctx.pixel(10, 2) == 255);
  CHECK(ctx.pixel(14, 2) == 0);

  // Subsequent solid drawing must still work on the same Cairo context.
  cairo_set_dash(ctx.cr.get(), nullptr, 0, 0);
  cairo_move_to(ctx.cr.get(), 0, 8.5);
  cairo_line_to(ctx.cr.get(), 32, 8.5);
  cairo_stroke(ctx.cr.get());
  CHECK(ctx.pixel(6, 8) == 255);
}
#endif
