#ifndef CONKY_SESSION_DETAILS_H
#define CONKY_SESSION_DETAILS_H

#include <cstddef>
#include <string_view>

#include <spdlog/fmt/fmt.h>

namespace conky::info {

// Don't guard enum values with #ifdef *_BUILD features; it will only make code
// harder to maintain - size_t won't change in size.

enum class display_session : std::size_t { unknown, x11, wayland };
enum class window_manager : std::size_t {
  unknown,

  // X11
  awesome,
  bspwm,
  compiz,
  dde,  // Deepin
  dwm,
  enlightenment,
  fluxbox,
  herbstluftwm,
  i3,
  kwin,
  marco,
  metacity,
  mutter,
  openbox,
  qtile,
  xfwm,
  windowmaker,

  // Wayland (only)
  hyprland,
  river,
  sway,
  wayfire,

  // Remember to update `populate_system_details` when adding new ones!
};

struct system {
  display_session session;

  window_manager wm;
  const char *wm_name;
};

}  // namespace conky::info

extern conky::info::system *user_system();  // defined in conky.cc

template <>
struct fmt::formatter<conky::info::display_session>
    : fmt::formatter<std::string_view> {
  auto format(conky::info::display_session session,
              fmt::format_context &ctx) const {
    std::string_view name = "unknown";
    switch (session) {
      case conky::info::display_session::x11:
        name = "X11";
        break;
      case conky::info::display_session::wayland:
        name = "Wayland";
        break;
      case conky::info::display_session::unknown:
        name = "unknown";
        break;
    }
    return fmt::formatter<std::string_view>::format(name, ctx);
  }
};

#endif /* CONKY_SESSION_DETAILS_H */