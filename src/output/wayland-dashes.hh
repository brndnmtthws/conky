// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef CONKY_WAYLAND_DASHES_HH
#define CONKY_WAYLAND_DASHES_HH

#include <cairo.h>

namespace conky {

inline void set_cairo_dashes(cairo_t *cr, const char *pattern) {
  // Match XSetDashes: callers supply exactly two unsigned byte lengths.
  const double dashes[] = {
      static_cast<double>(static_cast<unsigned char>(pattern[0])),
      static_cast<double>(static_cast<unsigned char>(pattern[1]))};
  // A zero pattern can result from byte truncation after DPI scaling. Cairo
  // errors are sticky, so do not poison the context with an all-zero pattern.
  cairo_set_dash(cr, dashes, dashes[0] == 0 && dashes[1] == 0 ? 0 : 2, 0);
}

}  // namespace conky

#endif
