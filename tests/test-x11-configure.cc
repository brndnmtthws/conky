#include "catch2/catch.hpp"

#include "config.h"

#if defined(BUILD_X11) && defined(OWN_WINDOW)
#include <X11/Xlib.h>

#include <cstdlib>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "conky.h"
#include "lua/lua-config.hh"
#include "output/display-x11.hh"
#include "output/x11.h"

extern int fixed_size, fixed_pos;
extern conky::vec2i text_size;

namespace conky {
bool handle_event(display_output_x11 *surface, Display *display,
                  XConfigureEvent ev, x11::event *propagated);
}

namespace {
struct configure_output : conky::display_output_x11 {
  int clears = 0;

  void clear_text(int) override { ++clears; }
  float get_dpi_scale() override { return 1.0f; }
};

// Use a private connection and a plain window: no running WM, fonts or drawing
// buffers are needed to exercise the real ConfigureNotify handler.
struct configure_test_context {
  Display *saved_display = display;
  int saved_screen = screen;
  conky_x11_window saved_window = window;
  std::unique_ptr<lua::state> saved_state = std::move(state);
  int saved_fixed_size = fixed_size;
  int saved_fixed_pos = fixed_pos;
  conky::vec2i saved_text_size = text_size;
  conky::display_outputs_t saved_outputs = conky::registered_outputs();
  std::vector<conky::display_output_base *> saved_active =
      conky::active_display_outputs;
  std::vector<conky::display_output_base *> saved_current =
      conky::current_display_outputs;
  configure_output output;

  explicit configure_test_context(const char *test_display) {
    display = XOpenDisplay(test_display);
    window = conky_x11_window{};
    if (display) { screen = DefaultScreen(display); }
    state = std::make_unique<lua::state>();
    conky::export_symbols(*state);
    state->loadstring(
        "conky.config = {own_window = true, own_window_type = 'desktop', "
        "alignment = 'top_left', border_inner_margin = 2, "
        "border_outer_margin = 2, border_width = 1, maximum_width = 200}");
    state->call(0, 0);
    fixed_size = fixed_pos = 0;
    text_size = conky::vec2i(200, 65);
    conky::active_display_outputs = {&output};
    conky::current_display_outputs.clear();
  }

  ~configure_test_context() {
    if (display) {
      if (window.window != None) { XDestroyWindow(display, window.window); }
      XCloseDisplay(display);
    }
    display = saved_display;
    screen = saved_screen;
    window = saved_window;
    state = std::move(saved_state);
    fixed_size = saved_fixed_size;
    fixed_pos = saved_fixed_pos;
    text_size = saved_text_size;
    conky::registered_outputs().swap(saved_outputs);
    conky::active_display_outputs.swap(saved_active);
    conky::current_display_outputs.swap(saved_current);
  }

  void create() {
    REQUIRE(display != nullptr);
    window.root = RootWindow(display, screen);
    window.window =
        XCreateSimpleWindow(display, window.root, 17, 23, 210, 75, 0, 0, 0);
    REQUIRE(window.window != None);
    // Keep real test resizes deterministic even if the isolated display has
    // a WM. Delayed WM application is modeled by differing logical size.
    XSetWindowAttributes attrs{};
    attrs.override_redirect = True;
    XChangeWindowAttributes(display, window.window, CWOverrideRedirect, &attrs);
    window.owned = true;
    window.geometry.set_size(210, 75);
    window.geometry.set_pos(99, 99);
    window.pending_resize_request = 100;
    XSync(display, False);
  }

  void resize_server(int width, int height) {
    XResizeWindow(display, window.window, width, height);
    XSync(display, False);
  }

  void notify(unsigned long serial, int width, int height,
              Window target = None) {
    XConfigureEvent ev{};
    ev.type = ConfigureNotify;
    ev.display = display;
    ev.event = ev.window = target == None ? window.window : target;
    ev.serial = serial;
    ev.width = width;
    ev.height = height;
    conky::x11::event propagated;
    // Calling directly preserves the supplied serial. XSendEvent would let
    // the server replace it, so it cannot replay an old request serial.
    REQUIRE(conky::handle_event(&output, display, ev, &propagated));
  }

  void check_automatic() {
    CHECK(fixed_size == 0);
    CHECK(window.geometry.size() == conky::vec2i(210, 75));
    CHECK(text_size == conky::vec2i(200, 65));
    CHECK(output.clears == 0);
  }

  void check_external_resize() {
    CHECK(fixed_size == 1);
    CHECK_FALSE(window.pending_resize_request.has_value());
    CHECK(window.geometry.size() == conky::vec2i(260, 90));
    // Both borders are removed and maximum_width still caps text width.
    CHECK(text_size == conky::vec2i(200, 80));
    CHECK(output.clears == 1);
  }
};
}  // namespace

TEST_CASE("x11_configure_size", "[x11][integration][x11_configure]") {
  const char *test_display = std::getenv("CONKY_TEST_X11_DISPLAY");
  if (!test_display || !*test_display) {
    SKIP("set CONKY_TEST_X11_DISPLAY to an isolated test X server");
  }
  configure_test_context context(test_display);
  context.create();

  SECTION("stale_notification_after_requested_resize_completed") {
    context.notify(99, 5, 5);
    context.check_automatic();
    CHECK(window.geometry.pos() == conky::vec2i(17, 23));
  }

  SECTION("stale_notification_while_wm_resize_is_pending") {
    context.resize_server(210, 23);
    context.notify(99, 210, 23);
    context.check_automatic();
    CHECK(window.pending_resize_request == 100);
    CHECK(window.geometry.pos() == conky::vec2i(17, 23));

    context.resize_server(210, 75);
    context.notify(100, 210, 75);
    context.check_automatic();
    CHECK_FALSE(window.pending_resize_request.has_value());
  }

  SECTION("acknowledgement_then_old_payload_then_external_resize") {
    context.notify(100, 210, 75);
    CHECK_FALSE(window.pending_resize_request.has_value());
    context.notify(99, 5, 5);
    context.check_automatic();
    context.resize_server(260, 90);
    context.notify(101, 260, 90);
    context.check_external_resize();
  }

  SECTION("current_attributes_override_outdated_event_dimensions") {
    context.notify(101, 5, 5);
    context.check_automatic();
  }

  SECTION("matching_notification_does_not_disable_content_sizing") {
    context.notify(100, 210, 75);
    context.check_automatic();
  }

  SECTION("fresh_external_resize_keeps_fixed_size_behavior") {
    context.resize_server(260, 90);
    context.notify(101, 260, 90);
    context.check_external_resize();
  }

  SECTION("equal_request_serial_is_not_stale") {
    context.resize_server(260, 90);
    context.notify(100, 260, 90);
    context.check_external_resize();
  }

  SECTION("matching_event_dimensions_do_not_hide_current_external_size") {
    context.resize_server(260, 90);
    context.notify(101, 210, 75);
    context.check_external_resize();
  }

  SECTION("external_resize_before_any_content_resize") {
    window.pending_resize_request.reset();
    context.resize_server(260, 90);
    context.notify(0, 260, 90);
    context.check_external_resize();
  }

  SECTION("foreign_window_notification_cannot_change_our_geometry") {
    context.resize_server(260, 90);
    context.notify(101, 260, 90, window.root);
    context.check_automatic();
    CHECK(window.geometry.pos() == conky::vec2i(99, 99));
  }

  SECTION("existing_fixed_size_is_not_cleared") {
    state->loadstring("conky.config.alignment = 'none'");
    state->call(0, 0);
    fixed_size = 1;
    context.notify(101, 5, 5);
    CHECK(fixed_size == 1);
    CHECK(window.geometry.size() == conky::vec2i(210, 75));
    CHECK(text_size == conky::vec2i(200, 65));
    CHECK(context.output.clears == 0);
  }

  SECTION("older_serial_across_wraparound_is_ignored") {
    window.pending_resize_request = 1;
    context.resize_server(210, 23);
    context.notify(std::numeric_limits<unsigned long>::max(), 210, 23);
    context.check_automatic();
  }

  SECTION("newer_serial_across_wraparound_is_accepted") {
    window.pending_resize_request = std::numeric_limits<unsigned long>::max();
    context.resize_server(260, 90);
    context.notify(1, 260, 90);
    context.check_external_resize();
  }

  SECTION("zero_is_a_valid_resize_request_serial") {
    window.pending_resize_request = 0;
    context.resize_server(210, 23);
    context.notify(std::numeric_limits<unsigned long>::max(), 210, 23);
    context.check_automatic();
  }
}
#endif  // BUILD_X11 && OWN_WINDOW
