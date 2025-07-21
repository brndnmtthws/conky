/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
 *  (see AUTHORS)
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

#include "config.h"

#ifdef BUILD_I18N
#include <libintl.h>
#else
#define gettext
#endif

// TODO: inject reference to conky logger
// Lua allows modifying .so loading, so for each loaded library check if it has
// some hardcoded set_logger function symbol, and call it to set per-library
// reference to the global logger.
#define PRINT_ERROR(Format, ...)                   \
  fputs(stderr, "cairoimagehelper: ");             \
  fprintf(stderr, gettext(Format), ##__VA_ARGS__); \
  fputs(stderr, "\n")

void cairo_place_image(const char *file, cairo_t *cr, int x, int y, int width,
                       int height, double alpha) {
  int w, h, stride;
  Imlib_Image alpha_image, image, premul;
  cairo_surface_t *result;

  if (!file) {
    PRINT_ERROR("File is nil");
    return;
  }

  if (!cr) {
    PRINT_ERROR("cairo_t is nil");
    return;
  }

  image = (Imlib_Image *)imlib_load_image(file);
  if (!image) {
    PRINT_ERROR("can't load %s", file);
    return;
  }

  imlib_context_set_image(image);
  w = imlib_image_get_width();
  h = imlib_image_get_height();

  if ((w <= 0) && (h <= 0)) {
    PRINT_ERROR("%s has 0 size", file);
    return;
  }

  alpha_image = imlib_create_cropped_scaled_image(0, 0, w, h, width, height);

  /* create temporary image */
  premul = imlib_create_image(width, height);
  if (!premul) {
    PRINT_ERROR("can't create premul image for %s", file);
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

  stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

  /* now pass the result to cairo */
  result = cairo_image_surface_create_for_data(
      (unsigned char *)imlib_image_get_data_for_reading_only(),
      CAIRO_FORMAT_ARGB32, width, height, stride);

  cairo_set_source_surface(cr, result, x, y);
  cairo_paint_with_alpha(cr, alpha);

  imlib_context_set_image(alpha_image);
  imlib_free_image();
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
    PRINT_ERROR("File is nil");
    return;
  }

  if (!cs) {
    PRINT_ERROR("Surface is nil");
    return;
  }

  if ((scale_x <= 0.0) && (scale_y <= 0.0)) {
    PRINT_ERROR("Image Scale is 0, %s", file);
    return;
  }

  Imlib_Image *image = (Imlib_Image *)imlib_load_image(file);
  if (!image) {
    PRINT_ERROR("Couldn't load %s", file);
    return;
  }

  imlib_context_set_image(image);
  w = imlib_image_get_width();
  h = imlib_image_get_height();

  if ((w <= 0) && (h <= 0)) {
    PRINT_ERROR("%s has 0 size", file);
    return;
  }

  scaled_w = *return_scale_w = scale_x * (double)w;
  scaled_h = *return_scale_h = scale_y * (double)h;

  if ((scaled_w <= 0.0) && (scaled_h <= 0.0)) {
    PRINT_ERROR("%s scaled image has 0 size", file);
    return;
  }

  cr = cairo_create(cs);
  cairo_place_image(file, cr, x, y, scaled_w, scaled_h, 1.0);
  imlib_context_set_image(image);
  imlib_free_image_and_decache();

  cairo_destroy(cr);
}

#endif /* _LIBCAIRO_IMAGE_HELPER_H_ */
