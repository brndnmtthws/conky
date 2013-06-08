/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef _LIBCAIRO_IMAGE_HELPER_H_
#define _LIBCAIRO_IMAGE_HELPER_H_

#include <cairo.h>
#include <Imlib2.h>

void
cairo_draw_image(const char * file, cairo_surface_t * cs, int x, int y,
                 double scale_x, double scale_y,
                 double * return_scale_w, double * return_scale_h)
{
  Imlib_Image * image = imlib_load_image(file);
  if (! image) { return; }

  imlib_context_set_image(image);
  int w = imlib_image_get_width(), h = imlib_image_get_height();

  double scaled_w = *return_scale_w = scale_x * (double)w
       , scaled_h = *return_scale_h = scale_y * (double)h;

  /* create temporary image */
  Imlib_Image premul = imlib_create_image(scaled_w, scaled_h);
  /* FIXME: add error handling */

  /* fill with opaque black */
  imlib_context_set_image(premul);
  imlib_context_set_color(0, 0, 0, 255);
  imlib_image_fill_rectangle(0, 0, scaled_w, scaled_h);

  /* blend source image on top -
   * in effect this multiplies the rgb values by alpha */
  imlib_blend_image_onto_image(image, 0, 0, 0, w, h, 0, 0, scaled_w, scaled_h);

  /* and use the alpha channel of the source image */
  imlib_image_copy_alpha_to_image(image, 0, 0);

  /* now pass the result to cairo */
  cairo_surface_t * result = cairo_image_surface_create_for_data(
    (void *) imlib_image_get_data_for_reading_only(),
    CAIRO_FORMAT_ARGB32, scaled_w, scaled_h, sizeof(DATA32) * scaled_w);

  cairo_t * cr = cairo_create(cs);
  cairo_set_source_surface(cr, result, x, y);
  cairo_paint(cr);

  imlib_context_set_image(image);
  imlib_free_image();
  imlib_context_set_image(premul);
  imlib_free_image();

  cairo_destroy(cr);
  cairo_surface_destroy(result);
}

#endif /* _LIBCAIRO_IMAGE_HELPER_H_ */
