/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
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

#include "catch2/catch.hpp"

#include <config.h>

#ifdef BUILD_WAYLAND

#include <output/display-wayland.hh>

// Exercises the null-guard branches of the Lua-facing Wayland accessors
// added alongside PR (#1844)'s build-system groundwork. The accessors
// read a namespace-scoped static `global_window` maintained by
// display-wayland.cc; in a test binary that never initializes the
// Wayland backend, that static is zero-initialized (nullptr), so the
// accessors must return nullptr / zeros rather than dereferencing the
// null pointer.

TEST_CASE(
    "get_wayland_cairo_surface returns nullptr when backend is not initialized",
    "[wayland][lua]") {
  // No Wayland compositor is running under `ctest`; global_window is
  // zero-initialized and the surface is therefore unavailable. The
  // accessor must fail closed, not dereference.
  cairo_surface_t *surface = conky::get_wayland_cairo_surface();
  REQUIRE(surface == nullptr);
}

TEST_CASE(
    "get_wayland_window_size zeros its outputs when backend is not initialized",
    "[wayland][lua]") {
  // Seed the outputs with non-zero sentinels so we can prove the
  // accessor actually wrote to them rather than leaving stale data.
  int w = 0xdead;
  int h = 0xbeef;
  conky::get_wayland_window_size(&w, &h);
  REQUIRE(w == 0);
  REQUIRE(h == 0);
}

#endif /* BUILD_WAYLAND */
