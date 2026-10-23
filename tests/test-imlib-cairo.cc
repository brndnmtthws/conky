#include "catch2/catch.hpp"

#include "config.h"

#if defined(BUILD_WAYLAND) && defined(BUILD_IMLIB2)
#include <Imlib2.h>
#include <cairo.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "conky-imlib2.h"
#include "conky.h"
#include "lua/lua-config.hh"
#include "output/display-output.hh"

namespace {
struct cairo_image_context {
  std::unique_ptr<lua::state> saved_state = std::move(state);
  std::vector<conky::display_output_base *> saved_active =
      std::move(conky::active_display_outputs);
  std::vector<conky::display_output_base *> saved_current =
      std::move(conky::current_display_outputs);
  std::unique_ptr<cairo_surface_t, decltype(&cairo_surface_destroy)> surface{
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 16, 16),
      cairo_surface_destroy};
  std::unique_ptr<cairo_t, decltype(&cairo_destroy)> cr{
      cairo_create(surface.get()), cairo_destroy};

  cairo_image_context() {
    conky::active_display_outputs.clear();
    conky::current_display_outputs.clear();
    state = std::make_unique<lua::state>();
    conky::export_symbols(*state);
    state->loadstring("conky.config = {imlib_cache_size = 1048576}");
    state->call(0, 0);
    cimlib_init_cpu();
    REQUIRE(cairo_status(cr.get()) == CAIRO_STATUS_SUCCESS);
  }

  ~cairo_image_context() {
    cimlib_deinit();
    state = std::move(saved_state);
    conky::active_display_outputs = std::move(saved_active);
    conky::current_display_outputs = std::move(saved_current);
  }

  void clear() {
    cairo_save(cr.get());
    cairo_set_operator(cr.get(), CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr.get());
    cairo_restore(cr.get());
  }

  uint32_t pixel(int x, int y) {
    cairo_surface_flush(surface.get());
    uint32_t result;
    std::memcpy(&result,
                cairo_image_surface_get_data(surface.get()) +
                    y * cairo_image_surface_get_stride(surface.get()) + x * 4,
                sizeof(result));
    return result;
  }

  void render(bool blend = true, int x = 0, int y = 0) {
    cimlib_render_cairo(cr.get(), x, y, 0, blend);
    REQUIRE(cairo_status(cr.get()) == CAIRO_STATUS_SUCCESS);
  }
};

// Generate tiny fixtures at runtime rather than committing binary assets.
struct test_png {
  char directory[32] = "/tmp/conky-imlib-cairo-XXXXXX";
  std::string path;

  explicit test_png(const std::vector<uint32_t> &pixels,
                    bool has_alpha = true) {
    REQUIRE(pixels.size() == 16);
    REQUIRE(mkdtemp(directory) != nullptr);
    path = std::string(directory) + "/image.png";
    Imlib_Image image = imlib_create_image(4, 4);
    REQUIRE(image != nullptr);
    imlib_context_set_image(image);
    imlib_image_set_has_alpha(has_alpha);
    DATA32 *data = imlib_image_get_data();
    REQUIRE(data != nullptr);
    std::memcpy(data, pixels.data(), pixels.size() * sizeof(uint32_t));
    imlib_image_put_back_data(data);
    imlib_image_set_format("png");
    imlib_save_image(path.c_str());
    imlib_free_image_and_decache();
    REQUIRE(access(path.c_str(), F_OK) == 0);
  }

  ~test_png() {
    std::remove(path.c_str());
    rmdir(directory);
  }

  void queue(const std::string &options = "") {
    cimlib_add_image((path + options).c_str());
  }
};
}  // namespace

TEST_CASE("imlib_cairo_pixels", "[imlib][cairo]") {
  cairo_image_context context;
  std::vector<uint32_t> pixels;
  for (int row = 0; row < 4; ++row) {
    pixels.insert(pixels.end(),
                  {0xffff0000, 0x8000ff00, 0x000000ff, 0xff123456});
  }
  test_png image(pixels);
  image.queue(" -p 2,3");

  SECTION("premultiplied_alpha_and_position") {
    context.render(true, 1, 2);
    CHECK(context.pixel(3, 5) == 0xffff0000);
    CHECK(context.pixel(4, 5) == 0x80008000);
    CHECK(context.pixel(5, 5) == 0x00000000);
    CHECK(context.pixel(6, 5) == 0xff123456);
    CHECK(context.pixel(2, 5) == 0);
    CHECK(context.pixel(3, 4) == 0);
  }

  SECTION("draw_blended_false_is_opaque") {
    context.render(false);
    CHECK(context.pixel(2, 3) == 0xffff0000);
    CHECK(context.pixel(3, 3) == 0xff00ff00);
    CHECK(context.pixel(4, 3) == 0xff0000ff);
  }

  SECTION("cairo_state_is_preserved") {
    cairo_set_source_rgb(context.cr.get(), 0, 0, 1);
    cairo_set_operator(context.cr.get(), CAIRO_OPERATOR_SOURCE);
    context.render();
    CHECK(cairo_get_operator(context.cr.get()) == CAIRO_OPERATOR_SOURCE);
    double r, g, b, a;
    REQUIRE(cairo_pattern_get_rgba(cairo_get_source(context.cr.get()), &r, &g,
                                   &b, &a) == CAIRO_STATUS_SUCCESS);
    CHECK(r == 0);
    CHECK(g == 0);
    CHECK(b == 1);
    CHECK(a == 1);
  }
}

TEST_CASE("imlib_cairo_rgb", "[imlib][cairo]") {
  cairo_image_context context;
  test_png image(std::vector<uint32_t>(16, 0x00123456), false);
  Imlib_Image loaded = imlib_load_image(image.path.c_str());
  REQUIRE(loaded != nullptr);
  imlib_context_set_image(loaded);
  REQUIRE(imlib_image_has_alpha() == 0);
  imlib_free_image();
  image.queue();
  context.render();
  CHECK(context.pixel(1, 1) == 0xff123456);
}

TEST_CASE("imlib_cairo_geometry", "[imlib][cairo]") {
  cairo_image_context context;
  test_png image(std::vector<uint32_t>(16, 0xffff0000));

  SECTION("scaled_size") {
    image.queue(" -p 2,2 -s 8x8");
    context.render();
    for (int y = 2; y < 10; ++y) {
      for (int x = 2; x < 10; ++x) { CHECK(context.pixel(x, y) == 0xffff0000); }
    }
    CHECK(context.pixel(1, 2) == 0);
    CHECK(context.pixel(10, 9) == 0);
    CHECK(context.pixel(2, 1) == 0);
    CHECK(context.pixel(9, 10) == 0);
    CHECK(context.pixel(7, 7) == 0xffff0000);
    CHECK(context.pixel(0, 0) == 0);
    CHECK(context.pixel(12, 12) == 0);
  }

  SECTION("negative_position_is_clipped") {
    image.queue(" -p -2,-1");
    context.render();
    CHECK(context.pixel(0, 0) == 0xffff0000);
    CHECK(context.pixel(1, 2) == 0xffff0000);
    CHECK(context.pixel(2, 0) == 0);
    CHECK(context.pixel(0, 3) == 0);
  }

  SECTION("list_order_composites_over_previous_images") {
    test_png overlay(std::vector<uint32_t>(16, 0x8000ff00));
    image.queue();
    overlay.queue();
    context.render();
    CHECK(context.pixel(1, 1) == 0xff7f8000);
  }
}

TEST_CASE("imlib_cairo_cache", "[imlib][cairo]") {
  cairo_image_context context;
  test_png image(std::vector<uint32_t>(16, 0xffff0000));
  bool cached = false;
  SECTION("default_cache_preserves_decode") {
    image.queue();
    cached = true;
  }
  SECTION("no_cache_evicts_decode") { image.queue(" -n"); }
  SECTION("per_image_flush_evicts_decode") { image.queue(" -f 1"); }

  context.render();
  REQUIRE(context.pixel(1, 1) == 0xffff0000);
  REQUIRE(std::remove(image.path.c_str()) == 0);
  context.clear();
  // A read-only pixel accessor keeps the decoded image available after its
  // file disappears; requesting writable data incorrectly invalidates it.
  context.render();
  CHECK(context.pixel(1, 1) == (cached ? 0xffff0000U : 0U));
}

TEST_CASE("imlib_cairo_lifecycle", "[imlib][cairo]") {
  cairo_image_context context;
  test_png image(std::vector<uint32_t>(16, 0xffff0000));
  image.queue();
  cimlib_init_cpu();  // A repeated init must not discard queued images.
  context.render();
  REQUIRE(context.pixel(1, 1) == 0xffff0000);
  cimlib_deinit();
  cimlib_deinit();
  context.clear();
  cimlib_init_cpu();
  context.render();
  CHECK(context.pixel(1, 1) == 0);
  image.queue();
  context.render();
  CHECK(context.pixel(1, 1) == 0xffff0000);
}
#endif
