/*
 *
 * Conky, a system monitor, based on torsmo
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

#include "wl-shell.h"

#include <wayland-client.h>

#include <wlr-layer-shell-client-protocol.h>
#include <xdg-shell-client-protocol.h>

namespace conky {
namespace {

uint32_t to_layer_anchor(screen_edge edge) {
  auto align = static_cast<alignment>(static_cast<uint8_t>(edge));
  uint32_t anchor = 0;
  switch (vertical_alignment(align)) {
    case axis_align::START:
      anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
      break;
    case axis_align::END:
      anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
      break;
    default:
      break;
  }
  switch (horizontal_alignment(align)) {
    case axis_align::START:
      anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
      break;
    case axis_align::END:
      anchor |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
      break;
    default:
      break;
  }
  return anchor;
}

}  // namespace

// --- layer_shell_surface ---------------------------------------------------

const zwlr_layer_surface_v1_listener layer_shell_surface::listener = {
    .configure = &layer_shell_surface::handle_configure,
    .closed = &layer_shell_surface::handle_closed,
};

layer_shell_surface::layer_shell_surface(const params &p)
    : m_on_close(p.on_close) {
  m_layer_surface = zwlr_layer_shell_v1_get_layer_surface(
      p.layer_shell, p.surface, p.output, p.layer, p.m_namespace);
  zwlr_layer_surface_v1_add_listener(m_layer_surface, &listener, this);
}

layer_shell_surface::~layer_shell_surface() {
  if (m_layer_surface != nullptr) {
    zwlr_layer_surface_v1_destroy(m_layer_surface);
  }
}

void layer_shell_surface::set_size(uint32_t width, uint32_t height) {
  zwlr_layer_surface_v1_set_size(m_layer_surface, width, height);
}

void layer_shell_surface::reserve_space(screen_edge edge, int exclusive_zone,
                                        int margin_x, int margin_y) {
  zwlr_layer_surface_v1_set_anchor(m_layer_surface, to_layer_anchor(edge));
  zwlr_layer_surface_v1_set_margin(m_layer_surface, margin_y, margin_x,
                                   margin_y, margin_x);
  zwlr_layer_surface_v1_set_exclusive_zone(m_layer_surface, exclusive_zone);
}

void layer_shell_surface::handle_configure(void *data,
                                           zwlr_layer_surface_v1 *surface,
                                           uint32_t serial, uint32_t width,
                                           uint32_t height) {
  (void)data;
  (void)width;
  (void)height;
  zwlr_layer_surface_v1_ack_configure(surface, serial);
}

void layer_shell_surface::handle_closed(void *data,
                                        zwlr_layer_surface_v1 *surface) {
  (void)surface;
  auto *self = static_cast<layer_shell_surface *>(data);
  if (self->m_on_close) { self->m_on_close(); }
}

// --- xdg_shell_surface -----------------------------------------------------

const xdg_surface_listener xdg_shell_surface::surface_listener = {
    .configure = &xdg_shell_surface::handle_surface_configure,
};
const xdg_toplevel_listener xdg_shell_surface::toplevel_listener = {
    .configure = &xdg_shell_surface::handle_toplevel_configure,
    .close = &xdg_shell_surface::handle_toplevel_close,
};

xdg_shell_surface::xdg_shell_surface(const params &p) : m_on_close(p.on_close) {
  m_xdg_surface = xdg_wm_base_get_xdg_surface(p.xdg_shell, p.surface);
  xdg_surface_add_listener(m_xdg_surface, &surface_listener, this);
  m_xdg_toplevel = xdg_surface_get_toplevel(m_xdg_surface);
  xdg_toplevel_add_listener(m_xdg_toplevel, &toplevel_listener, this);
  if (!p.title.empty()) {
    xdg_toplevel_set_title(m_xdg_toplevel, p.title.c_str());
  }
  if (!p.app_id.empty()) {
    xdg_toplevel_set_app_id(m_xdg_toplevel, p.app_id.c_str());
  }
}

xdg_shell_surface::~xdg_shell_surface() {
  if (m_xdg_toplevel != nullptr) { xdg_toplevel_destroy(m_xdg_toplevel); }
  if (m_xdg_surface != nullptr) { xdg_surface_destroy(m_xdg_surface); }
}

void xdg_shell_surface::set_size(uint32_t width, uint32_t height) {
  // Fixed-size window: pin min == max to the requested logical size.
  xdg_toplevel_set_min_size(m_xdg_toplevel, width, height);
  xdg_toplevel_set_max_size(m_xdg_toplevel, width, height);
  xdg_surface_set_window_geometry(m_xdg_surface, 0, 0, width, height);
}

void xdg_shell_surface::handle_surface_configure(void *data,
                                                 xdg_surface *surface,
                                                 uint32_t serial) {
  (void)data;
  xdg_surface_ack_configure(surface, serial);
}

void xdg_shell_surface::handle_toplevel_configure(void *data,
                                                  xdg_toplevel *toplevel,
                                                  int32_t width, int32_t height,
                                                  wl_array *states) {
  // Fixed-size window: ignore compositor-suggested dimensions and states.
  (void)data;
  (void)toplevel;
  (void)width;
  (void)height;
  (void)states;
}

void xdg_shell_surface::handle_toplevel_close(void *data,
                                              xdg_toplevel *toplevel) {
  (void)toplevel;
  auto *self = static_cast<xdg_shell_surface *>(data);
  if (self->m_on_close) { self->m_on_close(); }
}

}  // namespace conky
