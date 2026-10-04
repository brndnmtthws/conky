/* Exercise GC drawing on real X11 pixmaps; use only an isolated test server. */
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cstdio>
#include <cstdlib>
#include <utility>

#include "output/display-x11.hh"
#include "output/x11.h"

int main() {
  const char *test_display = std::getenv("CONKY_TEST_X11_DISPLAY");
  if (!test_display) {
    std::puts("SKIP: set CONKY_TEST_X11_DISPLAY to an isolated test X server");
    return 77;
  }
  display = XOpenDisplay(test_display);
  if (!display) { return 1; }
  screen = DefaultScreen(display);
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
    std::puts("SKIP: test requires a 24-bit root and 32-bit TrueColor visual");
    XCloseDisplay(display);
    return 77;
  }

  conky::display_output_x11 output;
  int failures = 0;
  // Include an opaque background on a retained ARGB drawable (e.g. a reload),
  // as well as non-ARGB fallback. Background opacity must not affect the GC.
  for (int depth : {DefaultDepth(display, screen), argb8888_color_depth}) {
    window.color_depth = depth;
    window.drawable =
        XCreatePixmap(display, RootWindow(display, screen), 16, 16, depth);
    window.gc = XCreateGC(display, window.drawable, 0, nullptr);
    output.set_line_style(1, true);
    for (int background_alpha : {0, 128, 255}) {
      window.opacity = background_alpha;
      for (int foreground_alpha : {0, 128, 255}) {
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
        auto image = XGetImage(display, window.drawable, 0, 0, 16, 16,
                               AllPlanes, ZPixmap);
        if (!image) { return 1; }
        for (auto point : {std::pair{1, 1}, std::pair{8, 1}, std::pair{8, 8}}) {
          const auto actual = XGetPixel(image, point.first, point.second);
          if (actual != expected) {
            std::fprintf(stderr,
                         "depth=%d background_alpha=%d foreground_alpha=%d "
                         "pixel=(%d,%d): expected %#lx, got %#lx\n",
                         depth, background_alpha, foreground_alpha, point.first,
                         point.second, expected, actual);
            ++failures;
          }
        }
        XDestroyImage(image);
      }
    }
    XFreeGC(display, window.gc);
    XFreePixmap(display, window.drawable);
  }
  window = conky_x11_window{};
  XCloseDisplay(display);
  display = nullptr;
  if (!failures) { std::puts("All 54 foreground pixel checks passed"); }
  return failures ? 1 : 0;
}
