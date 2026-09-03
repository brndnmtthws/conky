/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018-2021 François Revol et al.
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "display-wayland.hh"

#include <spdlog/details/console_globals.h>
#include <wayland-client.h>
// #include "wayland.h"
#include <cairo.h>
#include <fontconfig/fontconfig.h>
#include <pango/pangocairo.h>
#include <pango/pangofc-fontmap.h>

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <fractional-scale-client-protocol.h>
#include <viewporter-client-protocol.h>
#include <wayland-client-protocol.h>
#include <wlr-layer-shell-client-protocol.h>
#include <xdg-shell-client-protocol.h>

#include <cerrno>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <memory>

#include "../conky.h"
#include "../geometry.h"
#include "../logging.h"
#include "../lua/llua.h"
#include "display-output.hh"
#include "gui.h"
#include "wl-shell.h"

#include "../lua/fonts.h"

#ifdef BUILD_MOUSE_EVENTS
#include <array>
#include <map>
#include "../mouse-events.h"
#endif

#pragma GCC diagnostic ignored "-Wunused-parameter"

static int set_cloexec_or_close(int fd) {
  long flags;

  if (fd == -1) return -1;

  flags = fcntl(fd, F_GETFD);
  if (flags == -1) goto err;

  if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1) goto err;

  return fd;

err:
  close(fd);
  return -1;
}

static int create_tmpfile_cloexec(char *tmpname) {
  int fd;

#ifdef HAVE_MKOSTEMP
  fd = mkostemp(tmpname, O_CLOEXEC);
  if (fd >= 0) unlink(tmpname);
#else
  fd = mkstemp(tmpname);
  if (fd >= 0) {
    fd = set_cloexec_or_close(fd);
    unlink(tmpname);
  }
#endif

  return fd;
}

/*
 * Create a new, unique, anonymous file of the given size, and
 * return the file descriptor for it. The file descriptor is set
 * CLOEXEC. The file is immediately suitable for mmap()'ing
 * the given size at offset zero.
 *
 * The file should not have a permanent backing store like a disk,
 * but may have if XDG_RUNTIME_DIR is not properly implemented in OS.
 *
 * The file name is deleted from the file system.
 *
 * The file is suitable for buffer sharing between processes by
 * transmitting the file descriptor over Unix sockets using the
 * SCM_RIGHTS methods.
 *
 * If the C library implements posix_fallocate(), it is used to
 * guarantee that disk space is available for the file at the
 * given size. If disk space is insufficent, errno is set to ENOSPC.
 * If posix_fallocate() is not supported, program may receive
 * SIGBUS on accessing mmap()'ed file contents instead.
 */
static int os_create_anonymous_file(off_t size) {
  static const char templ[] = "/weston-shared-XXXXXX";
  const char *path;
  char *name;
  int fd;
  int ret;

  path = getenv("XDG_RUNTIME_DIR");
  if (!path) {
    errno = ENOENT;
    return -1;
  }

  name = static_cast<char *>(malloc(strlen(path) + sizeof(templ)));
  if (!name) return -1;

  strcpy(name, path);
  strcat(name, templ);

  fd = create_tmpfile_cloexec(name);

  free(name);

  if (fd < 0) return -1;
  ret = posix_fallocate(fd, 0, size);
  if (ret != 0) {
    close(fd);
    errno = ret;
    return -1;
  }
  return fd;
}

// TODO: cleanup externs (move to conky.h ?)
#ifdef OWN_WINDOW
extern int fixed_size, fixed_pos;
#endif
extern conky::vec2i text_start;  /* text start position in window */
extern conky::vec2i text_offset; /* offset for start position */
extern conky::vec2i
    text_size; /* initially 1 so no zero-sized window is created */
extern double current_update_time, next_update_time, last_update_time;
void update_text();
extern int need_to_update;
int get_border_total();
extern conky::range_config_setting<int> maximum_width;
extern Colour current_color;

/* set from the main loop's signal handler; used to request a clean shutdown
 * when the compositor closes our shell surface. */
extern volatile sig_atomic_t g_sigterm_pending;

/* for pango_fonts */
struct pango_font {
  PangoFontDescription *desc;

  struct {
    uint32_t ascent;
    uint32_t descent;
  } metrics;
  int font_alpha;

  pango_font() : desc(nullptr), metrics({0, 0}), font_alpha(0xffff) {}
};

static std::vector<pango_font> pango_fonts; /* indexed by selected_font */

namespace {
class textalpha_setting : public conky::simple_config_setting<float> {
  using Base = conky::simple_config_setting<float>;

 protected:
  void lua_setter(lua::state &l, bool init) override {
    lua::stack_sentry s(l, -2);

    Base::lua_setter(l, init);

    if (init) {
      pango_fonts.resize(std::max(1, static_cast<int>(fonts.size())));
      pango_fonts[0].desc = nullptr;
      pango_fonts[0].font_alpha = do_convert(l, -1).first * 0xffff;
    }

    ++s;
  }

 public:
  textalpha_setting() : Base("textalpha", 1.0, false) {}
};

textalpha_setting textalpha;
}  // namespace

static void wayland_create_window();

static void wayland_create_window() {
  setup_fonts();
  load_fonts(utf8_mode.get(*state));
  update_text_area(); /* to position text/window on screen */

  selected_font = 0;
  update_text_area(); /* to get initial size of the window */
}

namespace conky {
namespace {
conky::display_output_wayland wayland_output;
}  // namespace

template <>
void register_output<output_t::WAYLAND>(display_outputs_t &outputs) {
  outputs.push_back(&wayland_output);
}

display_output_wayland::display_output_wayland()
    : display_output_base("wayland") {
  is_graphical = true;
}

bool display_output_wayland::detect() {
  if (out_to_wayland.get(*state)) {
    LOG_DEBUG("wayland display output '{}' enabled in config", name);
    return true;
  }
  return false;
}

static int epoll_fd;
static epoll_event ep[1];

struct window;
static window *global_window;
static wl_display *global_display;

struct window {
  rect<size_t> rectangle;
  wl_shm *shm;
  wl_surface *surface;
  /// @brief Shell role (layer-shell or xdg-shell) bound to @ref surface.
  std::unique_ptr<conky::shell_surface> shell;
  /// @brief Surface viewport mapping the device-pixel buffer onto the logical
  /// surface size; null when the compositor lacks wp_viewporter.
  wp_viewport *viewport = nullptr;
  /// @brief Object reporting the compositor's preferred fractional scale; null
  /// when the compositor lacks wp_fractional_scale_v1.
  wp_fractional_scale_v1 *fractional_scale = nullptr;
  /// @brief Logical-to-device scale factor currently applied to the buffers.
  ///
  /// Fractional when the compositor supports wp_fractional_scale_v1, otherwise
  /// the integer wl_output.scale.
  float scale = 1.0f;
  /// @brief Scale most recently advertised by the compositor.
  ///
  /// Updated asynchronously by the scale listeners and copied into @ref scale
  /// when the window is reallocated (see window_resize).
  float pending_scale = 1.0f;
  int current_buffer;
  std::shared_ptr<cairo_surface_t> shm_surface[2];
  std::unique_ptr<uint8_t[]> private_buffer;
  std::shared_ptr<cairo_surface_t> cairo_surface;
  std::shared_ptr<cairo_t> cr;
  PangoLayout *layout;
  PangoContext *pango_context;
};

struct {
  wl_registry *registry;
  wl_compositor *compositor;
  wl_shm *shm;
  wl_surface *surface;
  wl_seat *seat;
  wl_pointer *pointer;
  wl_output *output;
  xdg_wm_base *shell;
  zwlr_layer_shell_v1 *layer_shell;
  wp_viewporter *viewporter;
  wp_fractional_scale_manager_v1 *fractional_scale_manager;
} wl_globals;

static void xdg_wm_base_ping(void *data, xdg_wm_base *shell, uint32_t serial) {
  xdg_wm_base_pong(shell, serial);
}

static const xdg_wm_base_listener xdg_wm_base_listener = {
    /*.ping =*/&xdg_wm_base_ping,
};

static void output_geometry(void *data, wl_output *wl_output, int32_t x,
                            int32_t y, int32_t physical_width,
                            int32_t physical_height, int32_t subpixel,
                            const char *make, const char *model,
                            int32_t transform) {
  // TODO: Add support for proper output management through:
  // - xdg-output-unstable-v1
  // Maybe also support (if XDG protocol not reported):
  // - kde-output-management(-v2)
  // - wlr-output-management-unstable-v1
  workarea = absolute_rect<int>(
      vec2i(x, y),
      vec2i(x + physical_width,
            y + physical_height));  // TODO: use xdg_output.logical_position
}

static void output_mode(void *data, wl_output *wl_output, uint32_t flags,
                        int32_t width, int32_t height, int32_t refresh) {}

#ifdef WL_OUTPUT_DONE_SINCE_VERSION
static void output_done(void *data, wl_output *wl_output) {}
#endif

#ifdef WL_OUTPUT_SCALE_SINCE_VERSION
void output_scale(void *data, wl_output *wl_output, int32_t factor) {
  /* For now, assume we have one output and adopt its scale unconditionally. */
  /* We should also re-render immediately when scale changes. */
  // wl_output.scale only carries integer scales. When the compositor supports
  // wp_fractional_scale_v1 we get a more precise value from that instead, so
  // ignore the legacy event to avoid clobbering the fractional scale.
  if (wl_globals.fractional_scale_manager != nullptr) { return; }
  global_window->pending_scale = factor;
  LOG_TRACE_WITH(({"window->scale", global_window->scale}),
                 "received output scale event: {}", factor);
}
#endif

#ifdef WL_OUTPUT_NAME_SINCE_VERSION
static void output_name(void *data, wl_output *wl_output, const char *name) {}
#endif

#ifdef WL_OUTPUT_DESCRIPTION_SINCE_VERSION
static void output_description(void *data, wl_output *wl_output,
                               const char *description) {}
#endif

static const wl_output_listener output_listener = {
    /*.geometry =*/output_geometry,
    /*.mode =*/output_mode,
#ifdef WL_OUTPUT_DONE_SINCE_VERSION
    /*.done =*/output_done,
#endif
#ifdef WL_OUTPUT_SCALE_SINCE_VERSION
    /*.scale =*/&output_scale,
#endif
#ifdef WL_OUTPUT_NAME_SINCE_VERSION
    /*.name =*/&output_name,
#endif
#ifdef WL_OUTPUT_DESCRIPTION_SINCE_VERSION
    /*.description =*/&output_description,
#endif
};

static void fractional_scale_preferred(void *data,
                                       wp_fractional_scale_v1 *fractional_scale,
                                       uint32_t scale) {
  // scale is expressed in 1/120ths of the logical pixel size.
  global_window->pending_scale = static_cast<float>(scale) / 120.0f;
  LOG_TRACE_WITH(({"window->scale", global_window->scale}),
                 "received fractional scale event: {}/120", scale);
}

static const wp_fractional_scale_v1_listener fractional_scale_listener = {
    /*.preferred_scale =*/&fractional_scale_preferred};

void registry_handle_global(void *data, wl_registry *registry, uint32_t name,
                            const char *interface, uint32_t version) {
  if (strcmp(interface, "wl_compositor") == 0) {
    wl_globals.compositor = static_cast<wl_compositor *>(
        wl_registry_bind(registry, name, &wl_compositor_interface, 3));
  } else if (strcmp(interface, "wl_shm") == 0) {
    wl_globals.shm = static_cast<wl_shm *>(
        wl_registry_bind(registry, name, &wl_shm_interface, 1));
  } else if (strcmp(interface, "wl_seat") == 0) {
    wl_globals.seat = static_cast<wl_seat *>(
        wl_registry_bind(registry, name, &wl_seat_interface, 1));
  } else if (strcmp(interface, "wl_output") == 0) {
    wl_globals.output = static_cast<wl_output *>(
        wl_registry_bind(registry, name, &wl_output_interface, 2));
    wl_output_add_listener(wl_globals.output, &output_listener, nullptr);
  } else if (strcmp(interface, "xdg_wm_base") == 0) {
    wl_globals.shell = static_cast<xdg_wm_base *>(
        wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
    xdg_wm_base_add_listener(wl_globals.shell, &xdg_wm_base_listener, nullptr);
  } else if (strcmp(interface, "zwlr_layer_shell_v1") == 0) {
    wl_globals.layer_shell = static_cast<zwlr_layer_shell_v1 *>(
        wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1));
  } else if (strcmp(interface, wp_viewporter_interface.name) == 0) {
    wl_globals.viewporter = static_cast<wp_viewporter *>(
        wl_registry_bind(registry, name, &wp_viewporter_interface, 1));
  } else if (strcmp(interface, wp_fractional_scale_manager_v1_interface.name) ==
             0) {
    wl_globals.fractional_scale_manager =
        static_cast<wp_fractional_scale_manager_v1 *>(wl_registry_bind(
            registry, name, &wp_fractional_scale_manager_v1_interface, 1));
  }
}

void registry_handle_global_remove(void *data, wl_registry *registry,
                                   uint32_t name) {}

static const wl_registry_listener registry_listener = {
    registry_handle_global, registry_handle_global_remove};

window *window_create(wl_surface *surface, wl_shm *shm, int width, int height);

void window_resize(window *window, int width, int height);

void window_allocate_buffer(window *window);

void window_destroy(window *window);

void window_commit_buffer(window *window);

void window_get_width_height(window *window, int *w, int *h);

/// @brief Whether `own_window_hints` request behaviour that only the layer
/// shell can provide.
///
/// xdg-shell toplevels are always compositor-managed: they stack among regular
/// windows and are bound to a single workspace. Hints asking for the opposite
/// (`below`/`above` stacking or `sticky` across workspaces) can therefore only
/// be honoured by mounting the surface on a wlr-layer-shell layer. (Layer
/// surfaces are also intrinsically skipped by taskbars/pagers, so those hints
/// need no special handling here.)
static bool hints_require_layer_shell() {
  uint16_t hints = own_window_hints.get(*state);
  return TEST_HINT(hints, window_hints::BELOW) ||
         TEST_HINT(hints, window_hints::ABOVE) ||
         TEST_HINT(hints, window_hints::STICKY);
}

/// @brief Maps the configured `own_window_type` and `own_window_hints` onto a
/// wlr-layer-shell layer.
///
/// Explicit `above`/`below` hints take precedence and pick the top/bottom
/// layer. Otherwise the window type decides: desktop widgets sit on the
/// background, docks below regular windows, and panels above them; everything
/// else keeps the historical bottom layer.
static zwlr_layer_shell_v1_layer layer_for_window() {
  uint16_t hints = own_window_hints.get(*state);
  if (TEST_HINT(hints, window_hints::ABOVE)) {
    return ZWLR_LAYER_SHELL_V1_LAYER_TOP;
  }
  if (TEST_HINT(hints, window_hints::BELOW)) {
    return ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
  }
  switch (own_window_type.get(*state)) {
    case window_type::DESKTOP:
      return ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
    case window_type::PANEL:
      return ZWLR_LAYER_SHELL_V1_LAYER_TOP;
    case window_type::DOCK:
    case window_type::NORMAL:
    case window_type::UTILITY:
    default:
      return ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
  }
}

/// @brief Updates the shell surface anchoring and reserved space (struts).
///
/// Docks and panels reserve a strip along a screen edge so other surfaces
/// (e.g. maximized windows) don't overlap them; other window types anchor
/// where they're aligned and reserve nothing.
///
/// @param window window whose shell surface should be updated
/// @param width current window width in surface-local pixels
/// @param height current window height in surface-local pixels
static void window_update_struts(window *window, int width, int height) {
  if (!window->shell->supports_struts()) { return; }

  LOG_DEBUG("defining struts");

  alignment text_align = text_alignment.get(*state);
  axis_align valign = vertical_alignment(text_align);
  axis_align halign = horizontal_alignment(text_align);

  window_type type = own_window_type.get(*state);
  bool reserve_space = type == window_type::DOCK || type == window_type::PANEL;

  conky::screen_edge edge = conky::screen_edge::NONE;
  int exclusive_zone = 0;

  if (reserve_space) {
    // Reservation only works against a single edge, so collapse the alignment
    // to one dominant edge and reserve the perpendicular size (plus margin).
    if (valign == axis_align::START) {
      edge = conky::screen_edge::TOP;
      exclusive_zone = height + gap_y.get(*state);
    } else if (valign == axis_align::END) {
      edge = conky::screen_edge::BOTTOM;
      exclusive_zone = height + gap_y.get(*state);
    } else if (halign == axis_align::START) {
      edge = conky::screen_edge::LEFT;
      exclusive_zone = width + gap_x.get(*state);
    } else if (halign == axis_align::END) {
      edge = conky::screen_edge::RIGHT;
      exclusive_zone = width + gap_x.get(*state);
    }
    // A fully centered (mm) dock/panel has no edge to reserve against, so the
    // exclusive zone stays zero.
  } else {
    // Normal windows anchor exactly where they're aligned.
    edge = static_cast<conky::screen_edge>(*text_align);
  }

  window->shell->reserve_space(edge, exclusive_zone, gap_x.get(*state),
                               gap_y.get(*state));
}

#ifdef BUILD_MOUSE_EVENTS
static std::map<wl_pointer *, vec2<size_t>> last_known_positions{};

static void on_pointer_enter(void *data, wl_pointer *pointer,
                             std::uint32_t serial, wl_surface *surface,
                             wl_fixed_t surface_x, wl_fixed_t surface_y) {
  auto w = reinterpret_cast<window *>(data);

  auto pos =
      vec2d(wl_fixed_to_double(surface_x), wl_fixed_to_double(surface_y));
  last_known_positions[pointer] = pos;
  auto pos_abs = w->rectangle.pos() + pos;

  mouse_crossing_event event{mouse_event_t::AREA_ENTER, pos, pos_abs};
  llua_mouse_hook(event);
}

static void on_pointer_leave(void *data, wl_pointer *pointer,
                             std::uint32_t serial, wl_surface *surface) {
  auto w = reinterpret_cast<window *>(data);

  auto pos = last_known_positions[pointer];
  auto pos_abs = w->rectangle.pos() + pos;

  mouse_crossing_event event{mouse_event_t::AREA_LEAVE, pos, pos_abs};
  llua_mouse_hook(event);
}

static void on_pointer_motion(void *data, wl_pointer *pointer,
                              std::uint32_t _time, wl_fixed_t surface_x,
                              wl_fixed_t surface_y) {
  auto w = reinterpret_cast<window *>(data);

  auto pos =
      vec2d(wl_fixed_to_double(surface_x), wl_fixed_to_double(surface_y));
  last_known_positions[pointer] = pos;
  auto pos_abs = w->rectangle.pos() + pos;

  mouse_move_event event{pos, pos_abs};
  llua_mouse_hook(event);
}

static void on_pointer_button(void *data, wl_pointer *pointer,
                              std::uint32_t serial, std::uint32_t time,
                              std::uint32_t button, std::uint32_t state) {
  auto w = reinterpret_cast<window *>(data);

  auto pos = last_known_positions[pointer];
  auto pos_abs = w->rectangle.pos() + pos;

  mouse_button_event event{
      mouse_event_t::RELEASE,
      pos,
      pos_abs,
      static_cast<mouse_button_t>(button),
  };

  switch (static_cast<wl_pointer_button_state>(state)) {
    case WL_POINTER_BUTTON_STATE_RELEASED:
      // pass; default is MOUSE_RELEASE
      break;
    case WL_POINTER_BUTTON_STATE_PRESSED:
      event.type = mouse_event_t::PRESS;
      break;
    default:
      return;
  }
  llua_mouse_hook(event);
}

void on_pointer_axis(void *data, wl_pointer *pointer, std::uint32_t time,
                     std::uint32_t axis, wl_fixed_t value) {
  if (value == 0) return;

  auto w = reinterpret_cast<window *>(data);

  auto pos = last_known_positions[pointer];
  auto pos_abs = w->rectangle.pos() + pos;

  mouse_scroll_event event{
      pos,
      pos_abs,
      scroll_direction_t::UP,
  };

  switch (static_cast<wl_pointer_axis>(axis)) {
    case WL_POINTER_AXIS_VERTICAL_SCROLL:
      event.direction =
          value > 0 ? scroll_direction_t::DOWN : scroll_direction_t::UP;
      break;
    case WL_POINTER_AXIS_HORIZONTAL_SCROLL:
      event.direction =
          value > 0 ? scroll_direction_t::RIGHT : scroll_direction_t::LEFT;
      break;
    default:
      return;
  }
  llua_mouse_hook(event);
}

static const wl_pointer_listener pointer_listener = {
    .enter = on_pointer_enter,
    .leave = on_pointer_leave,
    .motion = on_pointer_motion,
    .button = on_pointer_button,
    .axis = on_pointer_axis,
};

static void seat_capability_listener(void *data, wl_seat *seat,
                                     uint32_t capability_int) {
  wl_seat_capability capabilities =
      static_cast<wl_seat_capability>(capability_int);
  if (wl_globals.seat == seat) {
    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) > 0) {
      wl_globals.pointer = wl_seat_get_pointer(seat);

      wl_pointer_add_listener(wl_globals.pointer, &pointer_listener, data);
    }
  }
}
static void seat_name_listener(void *data, wl_seat *wl_seat, const char *name) {
}

static const wl_seat_listener seat_listener = {
    .capabilities = seat_capability_listener,
    .name = seat_name_listener,
};
#endif /* BUILD_MOUSE_EVENTS */

bool display_output_wayland::initialize() {
  epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    LOG_ERROR("epoll_create failed: {}", strerror(errno));
    return false;
  }
  global_display = wl_display_connect(NULL);
  if (!global_display) {
    LOG_ERROR("wl_display_connect failed: {}", strerror(errno));
    return false;
  }

  wl_globals.registry = wl_display_get_registry(global_display);
  wl_registry_add_listener(wl_globals.registry, &registry_listener, NULL);

  wl_display_roundtrip(global_display);

  wl_surface *surface = wl_compositor_create_surface(wl_globals.compositor);
  global_window = window_create(surface, wl_globals.shm, 1, 1);
  window_allocate_buffer(global_window);

  // An own_window of normal/utility type becomes a regular compositor-managed
  // toplevel; everything else (desktop/dock/panel, or no own_window) mounts as
  // a layer surface, falling back to an xdg toplevel when the compositor lacks
  // wlr-layer-shell. own_window_hints that only a layer surface can satisfy
  // (below/above/sticky) also force the layer path.
  window_type type = own_window_type.get(*state);
  bool hints_layer_shell =
      own_window.get(*state) &&
      (type == window_type::NORMAL || type == window_type::UTILITY) &&
      !hints_require_layer_shell();
  auto on_close = []() { g_sigterm_pending = 1; };

  // KWin classifies the layer-shell surface from its namespace. Keep the
  // classic namespace unless the user opts into another via
  // own_window_namespace, so existing configs are unaffected.
  std::string namespace_str = own_window_namespace.get(*state);

  if (!hints_layer_shell && wl_globals.layer_shell != nullptr) {
    global_window->shell =
        conky::create_shell_surface<conky::layer_shell_surface>({
            on_close,
            global_window->surface,
            wl_globals.layer_shell,
            static_cast<uint32_t>(layer_for_window()),
            namespace_str.c_str(),
        });
  } else {
    if (!hints_layer_shell) {
      LOG_WARNING(
          "compositor lacks wlr-layer-shell; falling back to an xdg-shell "
          "toplevel (desktop/dock/panel space reservation unavailable)");
    }
    if (wl_globals.shell == nullptr) {
      SYSTEM_ERR("compositor supports neither wlr-layer-shell nor xdg-shell");
      return false;
    }
    global_window->shell =
        conky::create_shell_surface<conky::xdg_shell_surface>({
            on_close,
            global_window->surface,
            wl_globals.shell,
            own_window_title.get(*state),
            own_window_class.get(*state),
        });
  }
  global_window->shell->set_size(global_window->rectangle.width(),
                                 global_window->rectangle.height());

#ifdef BUILD_MOUSE_EVENTS
  wl_seat_add_listener(wl_globals.seat, &seat_listener, global_window);
#endif /* BUILD_MOUSE_EVENTS */

  wl_surface_commit(global_window->surface);
  wl_display_roundtrip(global_display);

  wayland_create_window();
  return true;
}

typedef void (*display_global_handler_t)(wl_display *display, uint32_t name,
                                         const char *interface,
                                         uint32_t version, void *data);
typedef void (*display_output_handler_t)(wl_output *output, void *data);

bool display_output_wayland::shutdown() { return false; }

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

bool display_output_wayland::main_loop_wait(double t) {
  errno = 0;
  while (wl_display_prepare_read(global_display) != 0) {
    if (wl_display_dispatch_pending(global_display) == -1) {
      SYSTEM_ERR("wayland dispatch error: {}", strerror(errno));
    }
  }

  errno = 0;
  if (wl_display_flush(global_display) < 0 && errno != EAGAIN) {
    wl_display_cancel_read(global_display);
    SYSTEM_ERR("wayland flush error: {}", strerror(errno));
  }

  if (t < 0.0) { t = 0.0; }
  int ms = t * 1000;

  /* add fd to epoll set the first time around */
  static bool configured_epoll = false;
  if (!configured_epoll) {
    ep[0].events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
    ep[0].data.ptr = nullptr;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, wl_display_get_fd(global_display),
                  &ep[0]) == -1) {
      SYSTEM_ERR("unable to setup epoll for wayland display fd");
      return false;
    }
    configured_epoll = true;
  }

  /* wait for Wayland event or timeout */
  int ep_count = epoll_wait(epoll_fd, ep, ARRAY_LENGTH(ep), ms);

  if (ep_count > 0) {
    if (ep[0].events & (EPOLLERR | EPOLLHUP)) {
      SYSTEM_ERR("wayland output closed unexpectedly");
    }
  }

  int read_status = 0;
  if (ep_count > 0) {
    read_status = wl_display_read_events(global_display);
  } else {
    wl_display_cancel_read(global_display);
  }

  if (read_status == 0) {
    int num = wl_display_dispatch_pending(global_display);
    (void)num;
    LOG_TRACE("dispatched {} Wayland events", num);
  }

  wl_display_flush(global_display);

  /* timeout */
  if (ep_count == 0) { update_text(); }

  if (need_to_update != 0) {
    need_to_update = 0;
    selected_font = 0;
    update_text_area();

    bool bounds_changed = false;
    int border_total = get_border_total();

    int width, height;
    window_get_width_height(global_window, &width, &height);

    int fixed_size = 0;

    // Scales are quantised to 1/120 (fractional-scale) or whole integers, so a
    // genuine change is always >= 1/120; half a step is comfortably above any
    // float-representation noise and below the smallest real change.
    constexpr float scale_change_threshold = 1.0f / 240.0f;
    bool scale_changed =
        std::abs(global_window->scale - global_window->pending_scale) >
        scale_change_threshold;
    if (scale_changed)
      LOG_TRACE("scale changed from {} to {}", global_window->scale,
                global_window->pending_scale);

    /* resize window if it isn't right size */
    if ((fixed_size == 0) &&
        (text_size.x() + 2 * border_total != width ||
         text_size.y() + 2 * border_total != height || scale_changed)) {
      /* clamp text_width to configured maximum; maximum_width is in logical
       * pixels like text_size, the compositor scale is applied later at the
       * cairo/viewport level. */
      int mw = maximum_width.get(*state);
      if (mw > 0) { text_size.set_x(std::min(mw, text_size.x())); }

      /* pending scale will be applied by resizing the window */
      global_window->scale = global_window->pending_scale;

      width = text_size.x() + 2 * border_total;
      height = text_size.y() + 2 * border_total;
      window_resize(global_window, width, height); /* resize window */

      bounds_changed |= true;
    }

    /* update struts */
    if (bounds_changed) { window_update_struts(global_window, width, height); }

    /* update lua window globals */
    llua_update_window_table(conky::vec2i(width, height),
                             conky::rect<int>(text_start, text_size));

    clear_text(1);
    draw_stuff();
  }
  wl_display_flush(global_display);

  return true;
}

void display_output_wayland::sigterm_cleanup() {}

void display_output_wayland::cleanup() {
  if (global_window != nullptr) {
    window_destroy(global_window);
    global_window = nullptr;
  }
  free_fonts(utf8_mode.get(*state));
}

void display_output_wayland::set_foreground_color(Colour c) {
  current_color = c;
  if (global_window->cr) {
    cairo_set_source_rgba(global_window->cr.get(), current_color.red / 255.0,
                          current_color.green / 255.0,
                          current_color.blue / 255.0,
                          current_color.alpha / 255.0);
  }
}

int display_output_wayland::calc_text_width(const char *s) {
  window *window = global_window;
  size_t slen = strlen(s);
  pango_layout_set_text(window->layout, s, slen);
  PangoRectangle margin_rect;
  pango_layout_set_font_description(window->layout,
                                    pango_fonts[selected_font].desc);
  pango_layout_get_pixel_extents(window->layout, nullptr, &margin_rect);
  return margin_rect.width;
}

static void adjust_coords(int &x, int &y) {
  x -= text_start.x();
  y -= text_start.y();
  int border = get_border_total();
  x += border;
  y += border;
}

void display_output_wayland::draw_string_at(int x, int y, const char *s,
                                            int w) {
  window *window = global_window;
  auto cr = window->cr.get();
  y -= pango_fonts[selected_font].metrics.ascent;
  adjust_coords(x, y);
  pango_layout_set_text(window->layout, s, strlen(s));
  cairo_save(cr);
  uint8_t r = current_color.red;
  uint8_t g = current_color.green;
  uint8_t b = current_color.blue;
  unsigned int a = pango_fonts[selected_font].font_alpha;
  cairo_set_source_rgba(cr, r / 255.0, g / 255.0, b / 255.0, a / 65535.);
  cairo_move_to(cr, x, y);
  pango_cairo_show_layout(cr, window->layout);
  cairo_restore(cr);
}

void display_output_wayland::set_line_style(int w, bool solid) {
  window *window = global_window;
  auto cr = window->cr.get();
  static double dashes[2] = {1.0, 1.0};
  if (solid)
    cairo_set_dash(cr, nullptr, 0, 0);
  else
    cairo_set_dash(cr, dashes, 2, 0);
  cairo_set_line_width(cr, w);
}

void display_output_wayland::set_dashes(char *s) {
  window *window = global_window;
  auto cr = window->cr.get();
  size_t len = strlen(s);
  double *dashes = new double[len];
  for (size_t i = 0; i < len; i++) { dashes[i] = s[i]; }
  cairo_set_dash(cr, dashes, len, 0);
  delete[] dashes;
}

void display_output_wayland::draw_line(int x1, int y1, int x2, int y2) {
  window *window = global_window;
  auto cr = window->cr.get();
  adjust_coords(x1, y1);
  adjust_coords(x2, y2);
  cairo_save(cr);
  cairo_move_to(cr, x1 - 0.5, y1 - 0.5);
  cairo_line_to(cr, x2 - 0.5, y2 - 0.5);
  cairo_stroke(cr);
  cairo_restore(cr);
}

std::weak_ptr<conky::draw_surface> display_output_wayland::drawing_surface() {
  if (!global_window) { return {}; }
  return global_window->cairo_surface;
}

template <bool Fill>
inline void do_rect(cairo_t *cr, int x, int y, int w, int h) {
  adjust_coords(x, y);

  cairo_save(cr);
  if constexpr (Fill) {
    /* Note that cairo interprets fill and stroke coordinates differently,
    so here we don't add 0.5 to move between centers and corners of pixels. */
    cairo_rectangle(cr, x, y, w - 1, h - 1);
    cairo_fill(cr);
  } else {
    cairo_rectangle(cr, x - 0.5, y - 0.5, w, h);
    cairo_stroke(cr);
  }
  cairo_restore(cr);
}

void display_output_wayland::draw_rect(int x, int y, int w, int h) {
  auto cr = global_window->cr.get();
  do_rect<false>(cr, x, y, w, h);
}

void display_output_wayland::fill_rect(int x, int y, int w, int h) {
  auto cr = global_window->cr.get();
  do_rect<true>(cr, x, y, w, h);
}

void display_output_wayland::draw_arc(int x, int y, int w, int h, int a1,
                                      int a2) {
  window *window = global_window;
  auto cr = window->cr.get();
  adjust_coords(x, y);
  cairo_save(cr);
  cairo_translate(cr, x + w / 2. - 0.5, y + h / 2. - 0.5);
  cairo_scale(cr, w / 2., h / 2.);
  cairo_set_line_width(cr, 2. / (w + h));
  double mult = M_PI / (180. * 64.);
  cairo_arc_negative(cr, 0., 0., 1., a1 * mult, a2 * mult);
  cairo_stroke(cr);
  cairo_restore(cr);
}

void display_output_wayland::move_win(int x, int y) {
  // window.x = x;
  // window.y = y;
  // TODO
}

void display_output_wayland::end_draw_stuff() {
  window_commit_buffer(global_window);
}

void display_output_wayland::clear_text(int exposures) {
  window *window = global_window;
  auto cr = window->cr.get();
  cairo_save(cr);

  Colour color = get_background_colour_preference(*state);

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_set_source_rgba(cr, color.red / 255.0, color.green / 255.0,
                        color.blue / 255.0, color.alpha / 255.0);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_rectangle(cr, 0, 0, window->rectangle.width(),
                  window->rectangle.height());
  cairo_fill(cr);
  cairo_restore(cr);
}

int display_output_wayland::font_height(unsigned int f) {
  if (pango_fonts.size() == 0) { return 2; }
  assert(f < pango_fonts.size());
  return pango_fonts[f].metrics.ascent + pango_fonts[f].metrics.descent;
}

int display_output_wayland::font_ascent(unsigned int f) {
  if (pango_fonts.size() == 0) { return 1; }
  assert(f < pango_fonts.size());
  return pango_fonts[f].metrics.ascent;
}

int display_output_wayland::font_descent(unsigned int f) {
  if (pango_fonts.size() == 0) { return 1; }
  assert(f < pango_fonts.size());
  return pango_fonts[f].metrics.descent;
}

void display_output_wayland::setup_fonts(void) { /* Nothing to do here */ }

void display_output_wayland::set_font(unsigned int f) {
  assert(f < pango_fonts.size());
  if (pango_fonts.size() > f && pango_fonts[f].desc != nullptr) {
    pango_layout_set_font_description(global_window->layout,
                                      pango_fonts[f].desc);
  }
}

void display_output_wayland::free_fonts(bool utf8) {
  for (auto &font : pango_fonts) {
    if (font.desc != nullptr) {
      pango_font_description_free(font.desc);
      font.desc = nullptr;
    }
  }
  pango_fonts.clear();
}

void display_output_wayland::load_fonts(bool utf8) {
  free_fonts(utf8);
  pango_fonts.resize(fonts.size());
  for (unsigned int i = 0; i < fonts.size(); i++) {
    auto &font = fonts[i];
    auto &pango_font_entry = pango_fonts[i];
    FcPattern *fc_pattern =
        FcNameParse(reinterpret_cast<const unsigned char *>(font.name.c_str()));
    // pango_fc_font_description_from_pattern requires a FAMILY to be set,
    // so set an empty one if none is present.
    FcValue dummy;
    if (FcPatternGet(fc_pattern, FC_FAMILY, 0, &dummy) != FcResultMatch) {
      FcPatternAddString(fc_pattern, FC_FAMILY, (FcChar8 *)"");
    }
    pango_font_entry.desc =
        pango_fc_font_description_from_pattern(fc_pattern, true);

    // Handle pixel size ourselves because
    // pango_fc_font_description_from_pattern does not
    double pixel_size = -1;
    if (FcPatternGetDouble(fc_pattern, FC_PIXEL_SIZE, 0, &pixel_size) ==
        FcResultMatch) {
      pango_font_description_set_absolute_size(pango_font_entry.desc,
                                               pixel_size * PANGO_SCALE);
    }
    FcPatternDestroy(fc_pattern);

    PangoFont *pango_font = pango_context_load_font(
        global_window->pango_context, pango_font_entry.desc);
    PangoFontMetrics *font_metrics =
        pango_font_get_metrics(pango_font, nullptr);
    auto ascent = pango_font_metrics_get_ascent(font_metrics) / PANGO_SCALE;
    auto descent = pango_font_metrics_get_descent(font_metrics) / PANGO_SCALE;
    pango_font_metrics_unref(font_metrics);
    g_object_unref(pango_font);

    pango_font_entry.metrics.ascent = ascent;
    pango_font_entry.metrics.descent = descent;
  }
}

struct shm_pool {
  wl_shm_pool *pool;
  size_t size;
  size_t used;
  void *data;
};

struct shm_surface_data {
  wl_buffer *buffer;
  shm_pool *pool;
  bool busy;
};

static const cairo_user_data_key_t shm_surface_data_key = {0};

static void buffer_release(void *data, wl_buffer *wl_buffer) {
  auto *surface_data = static_cast<shm_surface_data *>(data);
  surface_data->busy = false;
}

static const wl_buffer_listener buffer_listener = {buffer_release};

wl_buffer *get_buffer_from_cairo_surface(cairo_surface_t *surface) {
  shm_surface_data *data;

  data = static_cast<shm_surface_data *>(
      cairo_surface_get_user_data(surface, &shm_surface_data_key));

  return data->buffer;
}

static void shm_pool_destroy(shm_pool *pool);

static void shm_surface_data_destroy(void *p) {
  shm_surface_data *data = static_cast<shm_surface_data *>(p);
  wl_buffer_destroy(data->buffer);

  if (data->pool) { shm_pool_destroy(data->pool); }

  // Make sure the wayland server knows about the buffer destroy and the pool
  // destroy.
  wl_display_roundtrip(global_display);

  delete data;
}

static wl_shm_pool *make_shm_pool(wl_shm *shm, int size, void **data) {
  wl_shm_pool *pool;
  int fd;

  fd = os_create_anonymous_file(size);
  if (fd < 0) {
    LOG_ERROR("creating a buffer file for {}B failed: {}", size,
              strerror(errno));
    return NULL;
  }

  *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (*data == MAP_FAILED) {
    LOG_ERROR("mmap failed for {}B buffer: {}", size, strerror(errno));
    close(fd);
    return NULL;
  }

  pool = wl_shm_create_pool(shm, fd, size);

  close(fd);

  return pool;
}

static shm_pool *shm_pool_create(wl_shm *shm, size_t size) {
  shm_pool *pool = new shm_pool;

  if (!pool) return NULL;

  pool->pool = make_shm_pool(shm, size, &pool->data);
  if (!pool->pool) {
    delete pool;
    return NULL;
  }

  pool->size = size;
  pool->used = 0;

  return pool;
}

static void *shm_pool_allocate(shm_pool *pool, size_t size, int *offset) {
  if (pool->used + size > pool->size) return NULL;

  *offset = pool->used;
  pool->used += size;

  return (char *)pool->data + *offset;
}

/* destroy the pool. this does not unmap the memory though */
static void shm_pool_destroy(shm_pool *pool) {
  munmap(pool->data, pool->size);
  wl_shm_pool_destroy(pool->pool);
  delete pool;
}

/// @brief Physical (device-pixel) dimensions of the buffer backing a logical
/// rectangle.
///
/// The scale may be fractional, so the result is rounded up to guarantee the
/// buffer is large enough to cover the logical area.
static vec2i scaled_size(rect<size_t> *rect, float scale) {
  return conky::ceil(rect->size().cast<float>() * scale).cast<int>();
}

static int stride_for_shm_surface(rect<size_t> *rect, float scale) {
  return cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
                                       scaled_size(rect, scale).x());
}

static int data_length_for_shm_surface(rect<size_t> *rect, float scale) {
  return stride_for_shm_surface(rect, scale) * scaled_size(rect, scale).y();
}

static std::shared_ptr<conky::draw_surface> create_shm_surface_from_pool(
    void *none, rect<size_t> *rectangle, shm_pool *pool, float scale) {
  shm_surface_data *data;
  uint32_t format;
  cairo_surface_t *surface;
  cairo_format_t cairo_format;
  int stride, length, offset;
  void *map;

  data = new shm_surface_data;
  if (data == NULL) return NULL;

  cairo_format = CAIRO_FORMAT_ARGB32; /*or CAIRO_FORMAT_RGB16_565 who knows??*/

  stride = stride_for_shm_surface(rectangle, scale);
  length = data_length_for_shm_surface(rectangle, scale);
  data->pool = NULL;
  map = shm_pool_allocate(pool, length, &offset);

  if (!map) {
    delete data;
    return NULL;
  }

  auto scaled = scaled_size(rectangle, scale);
  surface = cairo_image_surface_create_for_data(
      static_cast<unsigned char *>(map), cairo_format, scaled.x(), scaled.y(),
      stride);

  cairo_surface_set_user_data(surface, &shm_surface_data_key, data,
                              shm_surface_data_destroy);

  format = WL_SHM_FORMAT_ARGB8888; /*or WL_SHM_FORMAT_RGB565*/

  data->buffer = wl_shm_pool_create_buffer(pool->pool, offset, scaled.x(),
                                           scaled.y(), stride, format);
  data->busy = false;
  wl_buffer_add_listener(data->buffer, &buffer_listener, data);

  return std::shared_ptr<conky::draw_surface>(surface, [](auto it) {
    if (it) cairo_surface_destroy(it);
  });
}

void window_allocate_buffer(window *window) {
  assert(window->shm != nullptr);

  float scale = window->scale;
  shm_pool *pool;
  pool = shm_pool_create(
      window->shm, data_length_for_shm_surface(&window->rectangle, scale) * 2);
  if (!pool) {
    LOG_ERROR("could not allocate shm pool for {}x{} window",
              window->rectangle.width(), window->rectangle.height());
    return;
  }
  for (int i = 0; i < 2; ++i) {
    window->shm_surface[i] = create_shm_surface_from_pool(
        window->shm, &window->rectangle, pool, scale);

    if (!window->shm_surface[i]) {
      if (i == 1) { window->shm_surface[0] = nullptr; }
      shm_pool_destroy(pool);
      return;
    }

    auto cs = window->shm_surface[i].get();
    cairo_surface_set_device_scale(cs, scale, scale);

    /* make sure we destroy the pool when the surface is destroyed */
    auto data = static_cast<shm_surface_data *>(
        cairo_surface_get_user_data(cs, &shm_surface_data_key));
    data->pool = (i == 1) ? pool : nullptr;
  }
  window->current_buffer = 0;

  int stride = stride_for_shm_surface(&window->rectangle, scale);
  int length = data_length_for_shm_surface(&window->rectangle, scale);

  window->private_buffer = std::make_unique<uint8_t[]>(length);
  auto scaled = scaled_size(&window->rectangle, scale);

  window->cairo_surface = std::shared_ptr<conky::draw_surface>(
      cairo_image_surface_create_for_data(window->private_buffer.get(),
                                          CAIRO_FORMAT_ARGB32, scaled.x(),
                                          scaled.y(), stride),
      [](auto it) {
        if (it) cairo_surface_destroy(it);
      });

  cairo_surface_set_device_scale(window->cairo_surface.get(), scale, scale);

  window->cr = std::shared_ptr<cairo_t>(
      cairo_create(window->cairo_surface.get()), [](auto it) {
        if (it) cairo_destroy(it);
      });
  window->layout = pango_cairo_create_layout(window->cr.get());
  window->pango_context = pango_cairo_create_context(window->cr.get());
}

window *window_create(wl_surface *surface, wl_shm *shm, int width, int height) {
  window *result;
  result = new window();

  result->rectangle.set_pos(vec2<size_t>::Zero());
  result->rectangle.set_size(width, height);
  result->scale = 1.0f;
  result->pending_scale = 1.0f;

  result->surface = surface;
  result->shm = shm;

  // When the compositor supports viewporter we scale via a wp_viewport
  // destination rather than wl_surface.set_buffer_scale, which lets us honour
  // fractional scales reported through wp_fractional_scale_v1.
  if (wl_globals.viewporter != nullptr) {
    result->viewport =
        wp_viewporter_get_viewport(wl_globals.viewporter, surface);
  }
  if (wl_globals.fractional_scale_manager != nullptr) {
    result->fractional_scale =
        wp_fractional_scale_manager_v1_get_fractional_scale(
            wl_globals.fractional_scale_manager, surface);
    wp_fractional_scale_v1_add_listener(result->fractional_scale,
                                        &fractional_scale_listener, nullptr);
  }

  return result;
}

void window_free_buffer(window *window) {
  for (int i = 0; i < 2; ++i) { window->shm_surface[i].reset(); }
  window->cr = nullptr;
  window->cairo_surface = nullptr;
  if (window->layout) g_object_unref(window->layout);
  if (window->pango_context) g_object_unref(window->pango_context);
  window->layout = nullptr;
  window->pango_context = nullptr;
  window->private_buffer.reset();
}

void window_destroy(window *window) {
  window_free_buffer(window);
  if (window->fractional_scale) {
    wp_fractional_scale_v1_destroy(window->fractional_scale);
  }
  if (window->viewport) { wp_viewport_destroy(window->viewport); }
  // Destroy the shell role before the wl_surface it is bound to.
  window->shell.reset();
  wl_surface_attach(window->surface, nullptr, 0, 0);
  wl_surface_commit(window->surface);
  wl_display_roundtrip(global_display);
  wl_surface_destroy(window->surface);
  wl_shm_destroy(window->shm);
  delete window;
}

void window_resize(window *window, int width, int height) {
  LOG_TRACE("resizing conky display ({}) to {}x{}", window->rectangle, width,
            height);
  window_free_buffer(window);
  window->rectangle.set_size(width, height);
  window_allocate_buffer(window);
  window->shell->set_size(window->rectangle.width(),
                          window->rectangle.height());
}

void window_commit_buffer(window *window) {
  assert(window->shm_surface[window->current_buffer] != nullptr);

  cairo_surface_flush(window->cairo_surface.get());

  float scale = window->scale;
  int length = data_length_for_shm_surface(&window->rectangle, scale);

  auto shm_surf = window->shm_surface[window->current_buffer].get();
  unsigned char *shm_data = cairo_image_surface_get_data(shm_surf);

  std::memcpy(shm_data, window->private_buffer.get(), length);

  if (window->viewport != nullptr) {
    // The buffer is rendered at device resolution; the viewport maps it back
    // down to the logical surface size, which is what carries the (possibly
    // fractional) scale to the compositor.
    wp_viewport_set_destination(window->viewport, window->rectangle.width(),
                                window->rectangle.height());
  } else {
    // No viewporter: fall back to integer buffer scaling.
    wl_surface_set_buffer_scale(window->surface,
                                static_cast<int>(std::lround(scale)));
  }
  wl_surface_attach(window->surface, get_buffer_from_cairo_surface(shm_surf), 0,
                    0);
  /* repaint all the pixels in the surface, change size to only repaint changed
   * area*/
  wl_surface_damage(window->surface, window->rectangle.x(),
                    window->rectangle.y(), window->rectangle.width(),
                    window->rectangle.height());
  wl_surface_commit(window->surface);
  shm_surface_data *data = static_cast<shm_surface_data *>(
      cairo_surface_get_user_data(shm_surf, &shm_surface_data_key));
  data->busy = true;
  window->current_buffer = 1 - window->current_buffer;
  auto next_surf = window->shm_surface[window->current_buffer].get();
  shm_surface_data *next_data = static_cast<shm_surface_data *>(
      cairo_surface_get_user_data(next_surf, &shm_surface_data_key));
  while (next_data->busy) { wl_display_dispatch(global_display); }
}

void window_get_width_height(window *window, int *w, int *h) {
  *w = window->rectangle.width();
  *h = window->rectangle.height();
}

}  // namespace conky
