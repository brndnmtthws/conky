#include "catch2/catch.hpp"

#include "config.h"

#if defined(BUILD_X11) && defined(BUILD_XSHAPE) && defined(OWN_WINDOW)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "conky.h"
#include "lua/lua-config.hh"
#include "output/gui.h"
#include "output/x11.h"

extern int fixed_size, fixed_pos;

namespace {
std::vector<XErrorEvent> x11_errors;

int record_x11_error(Display *, XErrorEvent *error) {
  x11_errors.push_back(*error);
  return 0;
}

void configure(const std::string &settings) {
  state->loadstring(settings.c_str());
  state->call(0, 0);
}

// Keep every test on its own connection and restore globals even when a
// REQUIRE or SKIP exits a section. Closing through deinit_x11 also resets the
// backend's server-capability cache.
struct alpha_test_context {
  Display *saved_display = display;
  int saved_screen = screen;
  conky_x11_window saved_window = window;
  std::unique_ptr<lua::state> saved_state = std::move(state);
  conky::absolute_rect<int> saved_workarea = workarea;
  int saved_fixed_size = fixed_size;
  int saved_fixed_pos = fixed_pos;
  bool saved_reloading = g_is_reloading.load();
  XErrorHandler saved_error_handler = nullptr;
  Window compositor_owner = None;
  XVisualInfo argb_visual{};

  explicit alpha_test_context(const char *test_display) {
    display = XOpenDisplay(test_display);
    window = conky_x11_window{};
    if (display) { screen = DefaultScreen(display); }
    state = std::make_unique<lua::state>();
    conky::export_symbols(*state);
    configure(
        "conky.config = {own_window = true, double_buffer = true, "
        "own_window_input_from_alpha = true, own_window_type = 'normal', "
        "own_window_color = '#00000000'}");
    g_is_reloading = false;
    x11_errors.clear();
    saved_error_handler = XSetErrorHandler(record_x11_error);
  }

  ~alpha_test_context() {
    if (display) {
      destroy_window();
      if (compositor_owner != None) {
        XDestroyWindow(display, compositor_owner);
      }
      deinit_x11();
    }
    XSetErrorHandler(saved_error_handler);
    display = saved_display;
    screen = saved_screen;
    window = saved_window;
    state = std::move(saved_state);
    workarea = saved_workarea;
    fixed_size = saved_fixed_size;
    fixed_pos = saved_fixed_pos;
    g_is_reloading = saved_reloading;
  }

  void require_support() {
    if (!display) { SKIP("could not open the isolated test X server"); }
    int major = 0, minor = 0, event_base = 0, error_base = 0;
    if (!XShapeQueryExtension(display, &event_base, &error_base) ||
        !XShapeQueryVersion(display, &major, &minor) || major < 1 ||
        (major == 1 && minor < 1)) {
      SKIP("test requires SHAPE 1.1 input regions");
    }
    if (DefaultDepth(display, screen) != 24 ||
        !XMatchVisualInfo(display, screen, argb8888_color_depth, TrueColor,
                          &argb_visual) ||
        argb_visual.red_mask != 0xff0000 ||
        argb_visual.green_mask != 0x00ff00 ||
        argb_visual.blue_mask != 0x0000ff) {
      SKIP("test requires a 24-bit root and 32-bit ARGB visual");
    }
#ifdef BUILD_XDBE
    if (!XdbeQueryExtension(display, &major, &minor)) {
      SKIP("test requires the configured XDBE back buffer");
    }
    Drawable root = RootWindow(display, screen);
    int count = 1;
    auto *info = XdbeGetVisualInfo(display, &root, &count);
    bool argb_supported = false, rgb_supported = false;
    if (info && count == 1) {
      for (int i = 0; i < info[0].count; ++i) {
        argb_supported |= info[0].visinfo[i].visual == argb_visual.visualid;
        rgb_supported |= info[0].visinfo[i].visual ==
                         XVisualIDFromVisual(DefaultVisual(display, screen));
      }
    }
    if (info) { XdbeFreeVisualInfo(info); }
    if (!argb_supported || !rgb_supported) {
      SKIP("test requires XDBE support for both RGB and ARGB visuals");
    }
#endif
    // Conky deliberately requires a compositor selection before choosing
    // ARGB. Claim it only when unowned; actual compositing is unnecessary for
    // inspecting back-buffer pixels and server-side input regions.
    std::string selection = "_NET_WM_CM_S" + std::to_string(screen);
    Atom atom = XInternAtom(display, selection.c_str(), False);
    if (XGetSelectionOwner(display, atom) == None) {
      compositor_owner = XCreateSimpleWindow(
          display, RootWindow(display, screen), 0, 0, 1, 1, 0, 0, 0);
      XSetSelectionOwner(display, atom, compositor_owner, CurrentTime);
      REQUIRE(XGetSelectionOwner(display, atom) == compositor_owner);
    }
  }

  void check_errors() {
    XSync(display, False);
    for (const auto &error : x11_errors) {
      CAPTURE(static_cast<int>(error.error_code),
              static_cast<int>(error.request_code),
              static_cast<int>(error.minor_code));
      CHECK(error.error_code == 0);
    }
    REQUIRE(x11_errors.empty());
  }

  void resize(int width, int height, bool buffered = true) {
    XResizeWindow(display, window.window, width, height);
    window.geometry.set_size(width, height);
    if (!window.gc) {
      window.gc = XCreateGC(display, window.window, 0, nullptr);
      REQUIRE(window.gc != nullptr);
    }
    if (buffered) { REQUIRE(x11_set_up_double_buffer(*state)); }
    check_errors();
  }

  void create(int width = 12, int height = 12, bool argb = true,
              bool buffered = true) {
    if (window.window != None) { destroy_window(); }
    configure(
        std::string("conky.config.own_window_color = '") +
        (argb ? "#00000000" : "#ff000000") +
        "'; conky.config.double_buffer = " + (buffered ? "true" : "false"));
    x11_init_window(*state);
    REQUIRE(window.owned);
    REQUIRE(window.window != None);
    resize(width, height, buffered);
    XWindowAttributes attributes{};
    REQUIRE(XGetWindowAttributes(display, window.window, &attributes));
    REQUIRE(attributes.depth == (argb ? argb8888_color_depth : 24));
  }

  void clear(unsigned long pixel = 0) {
    XSetForeground(display, window.gc, pixel);
    XFillRectangle(display, window.drawable, window.gc, 0, 0,
                   window.geometry.width(), window.geometry.height());
  }

  void pixel(int x, int y, unsigned long value) {
    XSetForeground(display, window.gc, value);
    XDrawPoint(display, window.drawable, window.gc, x, y);
  }

  void swap() {
    swap_x11_buffers();
    check_errors();
  }
};

template <typename Predicate>
void check_input_region(Predicate expected) {
  int count = 0, ordering = 0;
  auto *rectangles = XShapeGetRectangles(display, window.window, ShapeInput,
                                         &count, &ordering);
  REQUIRE((rectangles != nullptr || count == 0));
  auto free_rectangles = [](XRectangle *value) { XFree(value); };
  std::unique_ptr<XRectangle, decltype(free_rectangles)> owned_rectangles(
      rectangles, free_rectangles);
  auto free_region = [](Region value) { XDestroyRegion(value); };
  std::unique_ptr<std::remove_pointer_t<Region>, decltype(free_region)> region(
      XCreateRegion(), free_region);
  REQUIRE(region != nullptr);
  for (int i = 0; i < count; ++i) {
    XUnionRectWithRegion(&rectangles[i], region.get(), region.get());
  }
  // Check the entire effective window area, independent of the server's
  // rectangle ordering/coalescing. Fail at the first incorrect pixel.
  for (int y = 0; y < window.geometry.height(); ++y) {
    for (int x = 0; x < window.geometry.width(); ++x) {
      bool actual = XPointInRegion(region.get(), x, y);
      if (actual != expected(x, y)) {
        CAPTURE(x, y, count);
        REQUIRE(actual == expected(x, y));
      }
    }
  }
}

void check_uniform_region(bool input) {
  check_input_region([input](int, int) { return input; });
}

void draw_corner(alpha_test_context &context) {
  context.clear();
  context.pixel(0, 0, 0xff000000);
}

struct window_style {
  const char *type;
  const char *hints;
  bool click_through;
};

const window_style styles[] = {
    {"normal", "", false},   {"normal", "undecorated", true},
#ifdef BUILD_XFIXES
    {"utility", "", true},
#else
    {"utility", "", false},
#endif
    {"override", "", false}, {"override", "undecorated", false},
};

void configure_style(const window_style &style, bool alpha) {
  configure(std::string("conky.config.own_window_type = '") + style.type +
            "'; conky.config.own_window_hints = '" + style.hints +
            "'; conky.config.own_window_input_from_alpha = " +
            (alpha ? "true" : "false"));
}
}  // namespace

TEST_CASE("x11_input_alpha", "[x11][integration][x11_input]") {
  const char *test_display = std::getenv("CONKY_TEST_X11_DISPLAY");
  if (!test_display || !*test_display) {
    SKIP("set CONKY_TEST_X11_DISPLAY to an isolated test X server");
  }
  alpha_test_context context(test_display);
  context.require_support();

  SECTION("rendered_alpha_cells") {
    context.create(11, 9);
    context.clear(0xff000000);
    context.swap();
    check_uniform_region(true);
    // Nonzero RGB with zero alpha must not intercept input.
    context.clear(0x00ffffff);
    context.swap();
    check_uniform_region(false);

    for (int frame = 0; frame < 2; ++frame) {
      context.clear(0x00ffffff);
      context.pixel(1, 1,
                    0x01000000);  // Even alpha 1 makes its cell interactive.
      context.pixel(6, 4, 0xff123456);
      context.pixel(9, 8,
                    0x80123456);  // Partial cells at the right/bottom edge.
      context.swap();
      check_input_region([](int x, int y) {
        return (x < 4 && y < 4) || (x >= 4 && x < 8 && y >= 4 && y < 8) ||
               (x >= 8 && y >= 8);
      });
    }
    context.clear();
    context.swap();
    check_uniform_region(false);
  }

  SECTION("disabled_alpha_restores_creation_baseline") {
    for (const auto &style : styles) {
      CAPTURE(style.type, style.hints);
      configure_style(style, false);
      context.create();
      check_uniform_region(!style.click_through);
      configure_style(style, true);
      draw_corner(context);
      context.swap();
      check_input_region([](int x, int y) { return x < 4 && y < 4; });
      configure_style(style, false);
      context.swap();
      check_uniform_region(!style.click_through);
    }
  }

  SECTION("unsupported_initial_configuration_keeps_creation_baseline") {
    for (const auto &style : styles) {
      for (bool argb : {false, true}) {
        CAPTURE(style.type, style.hints, argb);
        configure_style(style, true);
        // Test an RGB double buffer and an ARGB window without a buffer.
        context.create(12, 12, argb, !argb);
        check_uniform_region(!style.click_through);
        context.clear();
        context.swap();
        check_uniform_region(!style.click_through);
      }
    }
  }

  SECTION("losing_double_buffer_restores_creation_baseline") {
    for (const auto &style : styles) {
      CAPTURE(style.type, style.hints);
      configure_style(style, true);
      context.create();
      draw_corner(context);
      context.swap();
      check_input_region([](int x, int y) { return x < 4 && y < 4; });
      configure("conky.config.double_buffer = false");
      context.swap();
      check_uniform_region(!style.click_through);
    }
  }

  SECTION("resize_and_new_window_invalidate_cached_mask") {
    context.create(9, 9);
    auto draw_edge = [&] {
      context.clear();
      context.pixel(8, 8, 0xff000000);
      context.swap();
      check_input_region([](int x, int y) { return x >= 8 && y >= 8; });
    };
    draw_edge();
    // The cell mask still has 3x3 entries, but the last cell grew.
    context.resize(12, 12);
    draw_edge();
    Window first = window.window;
    context.create(12, 12);
    REQUIRE(window.window != first);
    draw_edge();
  }

  SECTION("fragmented_mask_spans_multiple_shape_requests") {
    // 130x130 alternating cells produce 8450 rectangles, exceeding the
    // implementation's 8192-rectangle request cap without a huge drawable.
    context.create(520, 520);
    context.clear();
    XSetForeground(display, window.gc, 0xff000000);
    for (int y = 0; y < 130; ++y) {
      for (int x = 0; x < 130; ++x) {
        if ((x + y) % 2 == 0) {
          XFillRectangle(display, window.drawable, window.gc, x * 4, y * 4, 4,
                         4);
        }
      }
    }
    context.swap();
    check_input_region([](int x, int y) { return (x / 4 + y / 4) % 2 == 0; });
    context.clear();
    context.swap();
    check_uniform_region(false);
  }

  SECTION("missing_compositor_preserves_opaque_window_on_reload") {
    if (context.compositor_owner == None) {
      SKIP("test requires an isolated server without a real compositor");
    }
    XDestroyWindow(display, context.compositor_owner);
    context.compositor_owner = None;
    context.check_errors();
    context.create(12, 12, false);
    Window opaque = window.window;
    configure("conky.config.own_window_color = '#00000000'");
    for (int reload = 0; reload < 2; ++reload) {
      x11_init_window(*state);
      REQUIRE(window.window == opaque);
      REQUIRE(window.color_depth != argb8888_color_depth);
      context.resize(12, 12);
      context.clear();
      context.swap();
      check_uniform_region(true);
    }
  }

  SECTION("opaque_window_can_reload_to_argb_and_retain_argb_depth") {
    configure("conky.config.own_window_input_from_alpha = false");
    context.create(12, 12, false);
    Window opaque = window.window;
    configure(
        "conky.config.own_window_input_from_alpha = true; "
        "conky.config.own_window_color = '#00000000'");
    x11_init_window(*state);
    REQUIRE(window.window != opaque);
    REQUIRE(window.color_depth == argb8888_color_depth);
    context.resize(12, 12);
    draw_corner(context);
    context.swap();
    check_input_region([](int x, int y) { return x < 4 && y < 4; });

    Window argb = window.window;
    configure("conky.config.own_window_color = '#ff000000'");
    x11_init_window(*state);
    REQUIRE(window.window == argb);
    REQUIRE(window.color_depth == argb8888_color_depth);
    REQUIRE(window.opacity == 255);
    context.resize(12, 12);
    draw_corner(context);
    context.swap();
    check_input_region([](int x, int y) { return x < 4 && y < 4; });
  }
  destroy_window();
  context.check_errors();
}
#endif  // BUILD_X11 && BUILD_XSHAPE && OWN_WINDOW
