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

#ifndef _LIBCAIRO_IMAGE_HELPER_H_
#define _LIBCAIRO_IMAGE_HELPER_H_

#include <Imlib2.h>
#include <cairo.h>

#include "logging.h"

void cairo_place_image(const char *file, cairo_t *cr, int x, int y,
                       int width, int height, double alpha) {
  int w, h, stride;
  Imlib_Image alpha_image, image, premul;
  cairo_surface_t *result;

  if (!file) {
    NORM_ERR("cairoimagehelper: File is NULL\n");
    return;
  }

  if (!cr) {
    NORM_ERR("cairoimagehelper: cairo_t is NULL\n");
    return;
  }

  image = imlib_load_image(file);
  if (!image) {
    NORM_ERR("cairoimagehelper: Couldn't load %s\n", file);
    return;
  }

  imlib_context_set_image(image);
  w = imlib_image_get_width();
  h = imlib_image_get_height();

  if ((w <= 0) && (h <= 0)) {
    NORM_ERR("cairoimagehelper: %s has 0 size\n", file);
    return;
  }

  /* create scaled version of image to later extract the alpha channel */
  alpha_image = imlib_create_cropped_scaled_image   (0, 0, w, h, width, height);

  /* create temporary image */
  premul = imlib_create_image(width, height);
  if (!premul) {
    NORM_ERR("cairoimagehelper: Couldn't create premul image for %s\n", file);
    return;
  }

  /* fill with opaque black */
  imlib_context_set_image(premul);
  imlib_context_set_color(0, 0, 0, 255);
  imlib_image_fill_rectangle(0, 0, width, height);

  /* blend source image on top -
   * in effect this multiplies the rgb values by alpha */
  imlib_blend_image_onto_image(image, 0, 0, 0, w, h, 0, 0, width, height);

  /* and use the alpha channel of the source image */
  imlib_image_copy_alpha_to_image(alpha_image, 0, 0);

  stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, width);

  /* now pass the result to cairo */
  result = cairo_image_surface_create_for_data(
      (void *)imlib_image_get_data_for_reading_only(), CAIRO_FORMAT_ARGB32,
      width, height, stride);

  cairo_set_source_surface(cr, result, x, y);
  cairo_paint_with_alpha(cr, alpha);

  imlib_context_set_image(image);
  imlib_free_image();
  imlib_context_set_image(premul);
  imlib_free_image();

  cairo_surface_destroy(result);

}

void cairo_draw_image(const char *file, cairo_surface_t *cs, int x, int y,
                      double scale_x, double scale_y, double *return_scale_w,
                      double *return_scale_h) {
  cairo_t *cr;
  int w, h;
  double scaled_w, scaled_h;

  if (!file) {
    NORM_ERR("cairoimagehelper: File is NULL\n");
    return;
  }

  if (!cs) {
    NORM_ERR("cairoimagehelper: Surface is NULL\n");
    return;
  }

  if ((scale_x <= 0.0) && (scale_y <= 0.0)) {
    NORM_ERR("cairoimagehelper: Image Scale is 0, %s\n", file);
    return;
  }

  Imlib_Image *image = imlib_load_image(file);
  if (!image) {
    NORM_ERR("cairoimagehelper: Couldn't load %s\n", file);
    return;
  }

  imlib_context_set_image(image);
  w = imlib_image_get_width();
  h = imlib_image_get_height();

  if ((w <= 0) && (h <= 0)) {
    NORM_ERR("cairoimagehelper: %s has 0 size\n", file);
    return;
  }

  scaled_w = *return_scale_w = scale_x * (double)w;
  scaled_h = *return_scale_h = scale_y * (double)h;

  if ((scaled_w <= 0.0) && (scaled_h <= 0.0)) {
    NORM_ERR("cairoimagehelper: %s scaled image has 0 size\n", file);
    return;
  }

  cr = cairo_create(cs);
  cairo_place_image(file, cr, x, y, scaled_w, scaled_h, 1.0);

  cairo_destroy(cr);
}

#endif /* _LIBCAIRO_IMAGE_HELPER_H_ */
