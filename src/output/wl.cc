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

#include <config.h>
#include <cstring>

#include "../conky.h"
#include "output-setting.hh"

#ifdef BUILD_WAYLAND

static const char NOT_IN_WAYLAND[] = "Not running in Wayland";

__attribute__((weak)) void print_monitor(struct text_object *obj, char *p,
                                         unsigned int p_max_size) {
  (void)obj;

  if (!conky::output_enabled(conky::output_t::WAYLAND)) {
    strncpy(p, NOT_IN_WAYLAND, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", -1);
}

__attribute__((weak)) void print_monitor_number(struct text_object *obj,
                                                char *p,
                                                unsigned int p_max_size) {
  (void)obj;

  if (!conky::output_enabled(conky::output_t::WAYLAND)) {
    strncpy(p, NOT_IN_WAYLAND, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", -1);
}

__attribute__((weak)) void print_desktop(struct text_object *obj, char *p,
                                         unsigned int p_max_size) {
  (void)obj;

  if (!conky::output_enabled(conky::output_t::WAYLAND)) {
    strncpy(p, NOT_IN_WAYLAND, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", -1);
}

__attribute__((weak)) void print_desktop_number(struct text_object *obj,
                                                char *p,
                                                unsigned int p_max_size) {
  (void)obj;

  if (!conky::output_enabled(conky::output_t::WAYLAND)) {
    strncpy(p, NOT_IN_WAYLAND, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", -1);
}

__attribute__((weak)) void print_desktop_name(struct text_object *obj, char *p,
                                              unsigned int p_max_size) {
  (void)obj;

  if (!conky::output_enabled(conky::output_t::WAYLAND)) {
    strncpy(p, NOT_IN_WAYLAND, p_max_size);
  } else {
    strncpy(p, "NYI", p_max_size);
  }
}
#endif
