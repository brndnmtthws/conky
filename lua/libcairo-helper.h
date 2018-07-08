/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef _LIBCAIRO_HELPER_H_
#define _LIBCAIRO_HELPER_H_

#include <cairo.h>

cairo_text_extents_t *create_cairo_text_extents_t(void) {
  return calloc(1, sizeof(cairo_text_extents_t));
}

cairo_font_extents_t *create_cairo_font_extents_t(void) {
  return calloc(1, sizeof(cairo_font_extents_t));
}

cairo_matrix_t *create_cairo_matrix_t(void) {
  return calloc(1, sizeof(cairo_matrix_t));
}

void destroy_cairo_text_extents_t(cairo_text_extents_t *pointer) {
  free(pointer);
}

void destroy_cairo_font_extents_t(cairo_font_extents_t *pointer) {
  free(pointer);
}

void destroy_cairo_matrix_t(cairo_matrix_t *pointer) {
  free(pointer);
}


#endif /* _LIBCAIRO_HELPER_H_ */
