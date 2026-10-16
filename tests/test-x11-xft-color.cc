#include "catch2/catch.hpp"

#include "config.h"

#if defined(BUILD_X11) && defined(BUILD_XFT)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "conky.h"
#include "lua/fonts.h"
#include "lua/lua-config.hh"
#include "output/display-x11.hh"
#include "output/x11.h"

extern Colour current_color;

namespace {
constexpr int image_width = 96;
constexpr int image_height = 64;
constexpr const char *test_font = "monospace:size=24:antialias=false";
std::vector<XErrorEvent> x11_errors;

int record_x11_error(Display *, XErrorEvent *error) {
  x11_errors.push_back(*error);
  return 0;
}

void configure(const std::string &settings) {
  state->loadstring(settings.c_str());
  state->call(0, 0);
}

// Own a separate display connection and restore all globals even when a
// REQUIRE or SKIP exits early. The backend's private font cache is managed
// only through its public loading/freeing methods.
struct xft_test_context {
  Display *saved_display = display;
  int saved_screen = screen;
  conky_x11_window saved_window = window;
  Colour saved_color = current_color;
  std::unique_ptr<lua::state> saved_state;
  std::vector<font_list> saved_fonts = std::move(fonts);
  unsigned int saved_selected_font = selected_font;
  conky::display_outputs_t saved_outputs = conky::registered_outputs();
  conky::display_output_x11 output;
  XErrorHandler saved_error_handler = nullptr;
  bool restore_font_cache = false;
  bool loaded_fonts = false;
  XVisualInfo argb_visual{};

  explicit xft_test_context(const char *test_display) {
    if (display && state && !saved_fonts.empty()) {
      // Preserve an existing Xft drawing context while releasing its fonts
      // before switching connections; reload those fonts when restoring it.
      window.xftdraw = nullptr;
      output.free_fonts(utf8_mode.get(*state));
      restore_font_cache = true;
    }
    saved_state = std::move(state);
    display = XOpenDisplay(test_display);
    window = conky_x11_window{};
    if (display) { screen = DefaultScreen(display); }
    state = std::make_unique<lua::state>();
    conky::export_symbols(*state);
    configure("conky.config = {use_xft = true, text_alpha = 1}");
    selected_font = 0;
    x11_errors.clear();
    saved_error_handler = XSetErrorHandler(record_x11_error);
  }

  ~xft_test_context() {
    if (display) {
      if (loaded_fonts) { output.free_fonts(utf8_mode.get(*state)); }
      if (window.gc) { XFreeGC(display, window.gc); }
      if (window.drawable) { XFreePixmap(display, window.drawable); }
      if (window.colourmap) { XFreeColormap(display, window.colourmap); }
      XCloseDisplay(display);
    }
    XSetErrorHandler(saved_error_handler);
    display = saved_display;
    screen = saved_screen;
    window = saved_window;
    current_color = saved_color;
    state = std::move(saved_state);
    fonts.swap(saved_fonts);
    selected_font = saved_selected_font;
    conky::registered_outputs().swap(saved_outputs);
    if (restore_font_cache) { output.load_fonts(utf8_mode.get(*state)); }
  }

  void require_support() {
    if (!display) { SKIP("could not open the isolated test X server"); }
    int event_base = 0, error_base = 0;
    if (!XRenderQueryExtension(display, &event_base, &error_base)) {
      SKIP("test requires the X Render extension");
    }
    auto *root_visual = DefaultVisual(display, screen);
    if (DefaultDepth(display, screen) != 24 ||
        root_visual->c_class != TrueColor ||
        root_visual->red_mask != 0xff0000 ||
        root_visual->green_mask != 0x00ff00 ||
        root_visual->blue_mask != 0x0000ff ||
        !XMatchVisualInfo(display, screen, argb8888_color_depth, TrueColor,
                          &argb_visual) ||
        argb_visual.red_mask != 0xff0000 ||
        argb_visual.green_mask != 0x00ff00 ||
        argb_visual.blue_mask != 0x0000ff) {
      SKIP("test requires a 24-bit RGB root and 32-bit ARGB visual");
    }
    auto *format = XRenderFindVisualFormat(display, argb_visual.visual);
    if (!format || format->direct.alphaMask != 0xff ||
        format->direct.alpha != 24) {
      SKIP("test requires an ARGB8888 X Render format");
    }
    auto *font = XftFontOpenName(display, screen, test_font);
    if (!font) { SKIP("test requires an installed Xft font"); }
    bool has_glyph = XftCharExists(display, font, 'M');
    XftFontClose(display, font);
    if (!has_glyph) { SKIP("test requires an Xft font containing M"); }
  }

  void check_errors() {
    XSync(display, False);
    for (const auto &error : x11_errors) {
      CAPTURE(static_cast<int>(error.error_code),
              static_cast<int>(error.request_code),
              static_cast<int>(error.minor_code), error.resourceid);
      CHECK(error.error_code == 0);
    }
    CHECK(x11_errors.empty());
    x11_errors.clear();
  }

  void create(int depth, bool utf8) {
    configure(std::string("conky.config.override_utf8_locale = ") +
              (utf8 ? "true" : "false"));
    REQUIRE(utf8_mode.get(*state) == utf8);
    window.color_depth = depth;
    window.visual = depth == argb8888_color_depth
                        ? argb_visual.visual
                        : DefaultVisual(display, screen);
    window.colourmap = XCreateColormap(display, RootWindow(display, screen),
                                       window.visual, AllocNone);
    REQUIRE(window.colourmap != None);
    window.drawable = XCreatePixmap(display, RootWindow(display, screen),
                                    image_width, image_height, depth);
    REQUIRE(window.drawable != None);
    window.gc = XCreateGC(display, window.drawable, 0, nullptr);
    REQUIRE(window.gc != nullptr);
    fonts.resize(1);
    fonts[0].name = test_font;
    loaded_fonts = true;
    output.load_fonts(utf8);
    output.setup_fonts();
    output.set_font(selected_font);
    REQUIRE(window.xftdraw != nullptr);
    REQUIRE(output.font_height(0) < image_height - 8);
    check_errors();
  }

  void check_glyph(int background_alpha, uint8_t component) {
    CAPTURE(static_cast<int>(window.color_depth), background_alpha,
            static_cast<int>(component), utf8_mode.get(*state));
    window.opacity = background_alpha;
    const unsigned long alpha =
        window.color_depth == argb8888_color_depth ? 0xff000000UL : 0;
    const unsigned long background = alpha | 0x13579b;
    const unsigned long expected = alpha | (component * 0x010101UL);
    XSetForeground(display, window.gc, background);
    XFillRectangle(display, window.drawable, window.gc, 0, 0, image_width,
                   image_height);
    output.set_foreground_color(Colour{component, component, component, 255});
    // Use the real Xft text path. Monochrome rasterization makes every glyph
    // pixel exact without relying on a particular font's glyph coordinates.
    output.draw_string_at(8, 8 + output.font_ascent(0), "M", 1);
    check_errors();
    auto destroy_image = [](XImage *image) { XDestroyImage(image); };
    std::unique_ptr<XImage, decltype(destroy_image)> image(
        XGetImage(display, window.drawable, 0, 0, image_width, image_height,
                  AllPlanes, ZPixmap),
        destroy_image);
    REQUIRE(image != nullptr);
    int glyph_pixels = 0, unexpected_pixels = 0;
    for (int y = 0; y < image_height; ++y) {
      for (int x = 0; x < image_width; ++x) {
        unsigned long pixel = XGetPixel(image.get(), x, y);
        if (pixel == expected) {
          ++glyph_pixels;
        } else if (pixel != background) {
          ++unexpected_pixels;
        }
      }
    }
    CHECK(glyph_pixels > 0);
    CHECK(unexpected_pixels == 0);
    check_errors();
  }
};
}  // namespace

TEST_CASE("x11_xft_color", "[x11][integration][x11_xft]") {
  const char *test_display = std::getenv("CONKY_TEST_X11_DISPLAY");
  if (!test_display || !*test_display) {
    SKIP("set CONKY_TEST_X11_DISPLAY to an isolated test X server");
  }
  xft_test_context context(test_display);
  context.require_support();
  // Inspect drawable pixels only: no compositor or compositor blending is
  // needed. ARGB with opacity 255 also covers a retained drawable on reload.
  int depth = GENERATE(24, argb8888_color_depth);
  bool utf8 = GENERATE(false, true);
  context.create(depth, utf8);
  for (int background_alpha : {0, 128, 255}) {
    for (uint8_t component : {255, 128, 0}) {
      context.check_glyph(background_alpha, component);
    }
  }
}
TEST_CASE("x11_xft_reload", "[x11][integration][x11_xft_reload]") {
  const char *test_display = std::getenv("CONKY_TEST_X11_DISPLAY");
  if (!test_display || !*test_display) {
    SKIP("set CONKY_TEST_X11_DISPLAY to an isolated test X server");
  }
  SECTION("reconnect_after_drawing_cached_glyphs") {
    for (int cycle = 0; cycle < 8; ++cycle) {
      CAPTURE(cycle);
      {
        xft_test_context context(test_display);
        REQUIRE(display != nullptr);
        int opcode, event_base, error_base;
        // Query through core Xlib: calling XRenderQueryExtension here would
        // initialize Render before the backend and hide the lifecycle bug.
        if (!XQueryExtension(display, "RENDER", &opcode, &event_base,
                             &error_base)) {
          SKIP("test requires the X Render extension");
        }
        if (DefaultDepth(display, screen) != 24) {
          SKIP("test requires a 24-bit root visual");
        }
        context.create(24, true);
        // Opening a font is insufficient: draw to populate Xft's glyph cache,
        // whose cleanup calls Render from the display-close hook.
        context.check_glyph(255, 255);
      }
      CHECK(x11_errors.empty());
    }
  }
}
#endif  // BUILD_X11 && BUILD_XFT
