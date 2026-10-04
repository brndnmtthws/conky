#include "catch2/catch.hpp"

#include "config.h"

#ifdef BUILD_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cstdlib>
#include <memory>
#include <utility>

#include "output/display-x11.hh"
#include "output/x11.h"

extern Colour current_color;

namespace {
// Restore backend globals and release resources even if REQUIRE or SKIP exits
// early, so this test can safely run with the rest of the Catch2 suite.
struct x11_test_context {
  Display *saved_display = display;
  int saved_screen = screen;
  conky_x11_window saved_window = window;
  Colour saved_color = current_color;
  conky::display_outputs_t saved_outputs = conky::registered_outputs();

  explicit x11_test_context(const char *test_display) {
    display = XOpenDisplay(test_display);
    window = conky_x11_window{};
    if (display) { screen = DefaultScreen(display); }
  }

  ~x11_test_context() {
    if (display) {
      if (window.gc) { XFreeGC(display, window.gc); }
      if (window.drawable) { XFreePixmap(display, window.drawable); }
      XCloseDisplay(display);
    }
    display = saved_display;
    screen = saved_screen;
    window = saved_window;
    current_color = saved_color;
    conky::registered_outputs().swap(saved_outputs);
  }
};

void check_foreground_pixels(int depth) {
  conky::display_output_x11 output;
  window.color_depth = depth;
  window.drawable =
      XCreatePixmap(display, RootWindow(display, screen), 16, 16, depth);
  REQUIRE(window.drawable != None);
  window.gc = XCreateGC(display, window.drawable, 0, nullptr);
  REQUIRE(window.gc != nullptr);
  output.set_line_style(1, true);
  // Include an opaque background on a retained ARGB drawable (e.g. a reload).
  // Background opacity must not affect the foreground GC.
  for (int background_alpha : {0, 128, 255}) {
    window.opacity = background_alpha;
    for (int foreground_alpha : {0, 128, 255}) {
      CAPTURE(depth, background_alpha, foreground_alpha);
      Colour color{0x99, 0xcc, 0xff, static_cast<uint8_t>(foreground_alpha)};
      unsigned long expected = 0x99ccff;
      if (depth == argb8888_color_depth) {
        expected = (static_cast<unsigned long>(foreground_alpha) << 24) |
                   ((0x99 * foreground_alpha / 255) << 16) |
                   ((0xcc * foreground_alpha / 255) << 8) |
                   (0xff * foreground_alpha / 255);
      }
      // A contrasting initial value ensures each primitive really draws.
      XSetForeground(display, window.gc, expected ^ 0x00ffffff);
      XFillRectangle(display, window.drawable, window.gc, 0, 0, 16, 16);
      output.set_foreground_color(color);
      output.draw_rect(1, 1, 4, 4);   // window border
      output.draw_line(7, 1, 12, 1);  // $hr
      output.fill_rect(7, 7, 4, 4);   // bars/graphs
      auto destroy_image = [](XImage *image) { XDestroyImage(image); };
      std::unique_ptr<XImage, decltype(destroy_image)> image(
          XGetImage(display, window.drawable, 0, 0, 16, 16, AllPlanes, ZPixmap),
          destroy_image);
      REQUIRE(image != nullptr);
      for (auto point : {std::pair{1, 1}, std::pair{8, 1}, std::pair{8, 8}}) {
        CAPTURE(point.first, point.second);
        CHECK(XGetPixel(image.get(), point.first, point.second) == expected);
      }
    }
  }
}
}  // namespace

TEST_CASE("x11_foreground_alpha", "[x11][integration][x11_foreground]") {
  const char *test_display = std::getenv("CONKY_TEST_X11_DISPLAY");
  if (!test_display || !*test_display) {
    SKIP("set CONKY_TEST_X11_DISPLAY to an isolated test X server");
  }
  x11_test_context context(test_display);
  REQUIRE(display != nullptr);
  XVisualInfo visual{};
  auto root_visual = DefaultVisual(display, screen);
  if (DefaultDepth(display, screen) != 24 ||
      root_visual->red_mask != 0xff0000 ||
      root_visual->green_mask != 0x00ff00 ||
      root_visual->blue_mask != 0x0000ff ||
      !XMatchVisualInfo(display, screen, argb8888_color_depth, TrueColor,
                        &visual) ||
      visual.red_mask != 0xff0000 || visual.green_mask != 0x00ff00 ||
      visual.blue_mask != 0x0000ff) {
    SKIP("test requires a 24-bit root and 32-bit TrueColor visual");
  }

  SECTION("rgb_drawable") { check_foreground_pixels(24); }
  SECTION("argb_drawable") { check_foreground_pixels(argb8888_color_depth); }
}
#endif  // BUILD_X11
