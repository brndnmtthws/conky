#include "catch2/catch.hpp"

#include "config.h"

#if defined(BUILD_X11) && defined(OWN_WINDOW)
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <array>
#include <cstdlib>
#include <limits>
#include <vector>

#include "conky.h"
#include "output/x11.h"

namespace {
conky::rect<int> rectangle(int x, int y, int width, int height) {
  return {{x, y}, {width, height}};
}
}  // namespace

TEST_CASE("x11_monitor_strut_area", "[x11][x11_struts]") {
  const auto right_monitor = rectangle(2560, 0, 2560, 1440);
  auto check = [&](const conky::rect<int> &panel,
                   std::array<long, 4> expected) {
    const auto area = x11_monitor_strut_area(panel, right_monitor);
    REQUIRE(area.has_value());
    CHECK(*area == expected);
  };

  SECTION("internal_left_edge_from_issue_2530") {
    check(rectangle(2560, 0, 300, 1322), {2560, 0, 300, 1322});
  }
  SECTION("gaps_are_included_on_all_four_edges") {
    check(rectangle(2570, 20, 300, 1300), {2560, 20, 310, 1300});
    check(rectangle(4780, 50, 300, 1000), {4780, 50, 340, 1000});
    check(rectangle(2600, 10, 2000, 50), {2600, 0, 2000, 60});
    check(rectangle(2600, 1350, 2000, 50), {2600, 1350, 2000, 90});
  }
  SECTION("stacked_and_offset_monitors_keep_root_coordinates") {
    CHECK(x11_monitor_strut_area(rectangle(20, 1450, 2400, 40),
                                 rectangle(0, 1440, 2560, 1440)) ==
          std::array<long, 4>{20, 1440, 2400, 50});
    CHECK(x11_monitor_strut_area(rectangle(2560, 350, 100, 900),
                                 rectangle(2560, 300, 1920, 1080)) ==
          std::array<long, 4>{2560, 350, 100, 900});
  }
  SECTION("panels_outside_or_spanning_monitors_have_no_area") {
    for (auto panel :
         {rectangle(2550, 0, 300, 1322), rectangle(5000, 0, 300, 1322),
          rectangle(2560, -1, 300, 1322), rectangle(2560, 200, 300, 1322),
          rectangle(2560, 0, 0, 1322), rectangle(2560, 0, 300, -1)}) {
      CHECK_FALSE(x11_monitor_strut_area(panel, right_monitor));
    }
  }
  SECTION("large_gaps_must_not_change_the_inferred_edge") {
    CHECK_FALSE(
        x11_monitor_strut_area(rectangle(3560, 0, 100, 500), right_monitor));
    CHECK_FALSE(
        x11_monitor_strut_area(rectangle(2560, 500, 400, 50), right_monitor));
  }
  SECTION("coordinate_sums_do_not_overflow") {
    const int limit = std::numeric_limits<int>::max();
    CHECK_FALSE(
        x11_monitor_strut_area(rectangle(limit, 0, limit, 100), right_monitor));
    CHECK_FALSE(
        x11_monitor_strut_area(rectangle(0, limit, 100, limit), right_monitor));
    CHECK_FALSE(x11_monitor_strut_area(rectangle(-10, 0, 100, 1000),
                                       rectangle(-10, 0, 2560, 1440)));
  }
}

namespace {
struct strut_test_context {
  Display *saved_display = display;
  int saved_screen = screen;
  conky_x11_window saved_window = window;
  conky::info::system saved_system = info.system;

  explicit strut_test_context(const char *test_display) {
    display = XOpenDisplay(test_display);
    window = conky_x11_window{};
    if (display) { screen = DefaultScreen(display); }
    info.system.wm = conky::info::window_manager::marco;
  }
  ~strut_test_context() {
    if (display) {
      if (window.window != None) { XDestroyWindow(display, window.window); }
      XCloseDisplay(display);
    }
    display = saved_display;
    screen = saved_screen;
    window = saved_window;
    info.system = saved_system;
  }
  void create() {
    REQUIRE(display != nullptr);
    window.root = RootWindow(display, screen);
    window.window =
        XCreateSimpleWindow(display, window.root, 0, 0, 10, 10, 0, 0, 0);
    REQUIRE(window.window != None);
    window.owned = true;
  }
  std::vector<long> property(const char *name) {
    Atom actual_type;
    int format;
    unsigned long count, remaining;
    unsigned char *bytes = nullptr;
    REQUIRE(XGetWindowProperty(display, window.window,
                               XInternAtom(display, name, False), 0, 12, False,
                               XA_CARDINAL, &actual_type, &format, &count,
                               &remaining, &bytes) == Success);
    std::vector<long> result;
    if (actual_type != None) {
      CHECK(actual_type == XA_CARDINAL);
      CHECK(format == 32);
      CHECK(remaining == 0);
      auto values = reinterpret_cast<long *>(bytes);
      result.assign(values, values + count);
    }
    if (bytes) { XFree(bytes); }
    return result;
  }
};
}  // namespace

TEST_CASE("x11_strut_properties", "[x11][integration][x11_struts]") {
  const char *test_display = std::getenv("CONKY_TEST_X11_DISPLAY");
  if (!test_display || !*test_display) {
    SKIP("set CONKY_TEST_X11_DISPLAY to an isolated single-monitor X server");
  }
  strut_test_context context(test_display);
  context.create();
  const int width = DisplayWidth(display, screen);
  const int height = DisplayHeight(display, screen);
  REQUIRE(width >= 200);
  REQUIRE(height >= 200);
  window.geometry = rectangle(10, 20, 20, 100);
  set_struts();
  CHECK(context.property("_GNOME_WM_STRUT_AREA") ==
        std::vector<long>{0, 20, 30, 100});
  CHECK(context.property("_NET_WM_STRUT") == std::vector<long>{30, 0, 0, 0});
  CHECK(context.property("_NET_WM_STRUT_PARTIAL") ==
        std::vector<long>{30, 0, 0, 0, 20, 120, 0, 0, 0, 0, 0, 0});

  // Resizing and moving refresh the property rather than keeping a stale area.
  window.geometry = rectangle(width - 30, 40, 20, 150);
  set_struts();
  CHECK(context.property("_GNOME_WM_STRUT_AREA") ==
        std::vector<long>{width - 30, 40, 30, 150});
  window.geometry = rectangle(width - 5, 40, 20, 150);
  set_struts();
  CHECK(context.property("_GNOME_WM_STRUT_AREA").empty());

  window.geometry = rectangle(10, 20, 20, 100);
  set_struts();
  REQUIRE_FALSE(context.property("_GNOME_WM_STRUT_AREA").empty());
  clear_struts();
  for (const char *name :
       {"_NET_WM_STRUT", "_NET_WM_STRUT_PARTIAL", "_GNOME_WM_STRUT_AREA"}) {
    CHECK(context.property(name).empty());
  }
}
#endif  // BUILD_X11 && OWN_WINDOW
