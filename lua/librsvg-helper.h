/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2021 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef _LIBRSVG_HELPER_H_
#define _LIBRSVG_HELPER_H_

#include <glib.h>
#include <librsvg/rsvg.h>
#include <stdlib.h>

RsvgDimensionData *rsvg_dimension_data_create(void) {
  return (RsvgDimensionData *)calloc(1, sizeof(RsvgDimensionData));
}

void rsvg_dimension_data_destroy(RsvgDimensionData *pointer) {
  if (pointer) { free(pointer); }
}

void rsvg_dimension_data_get(RsvgDimensionData *dd, int *width, int *height,
                             double *em, double *ex) {
  if (dd) {
    *width = dd->width;
    *height = dd->height;
    *em = dd->em;
    *ex = dd->ex;
  }
}

void rsvg_dimension_data_set(RsvgDimensionData *dd, int width, int height,
                             double em, double ex) {
  if (dd) {
    dd->width = width;
    dd->height = height;
    dd->em = em;
    dd->ex = ex;
  }
}

RsvgPositionData *rsvgPositionDataCreate(void) {
  return (RsvgPositionData *)calloc(1, sizeof(RsvgPositionData));
}

void rsvgPositionDataGet(RsvgPositionData *pd, int *x, int *y) {
  if (pd) {
    *x = pd->x;
    *y = pd->y;
  }
}

RsvgHandle *rsvg_create_handle_from_file(const char *filename) {
  GFile *gfile = g_file_new_for_path(filename);

  GError *error = NULL;
  RsvgHandle *handle = rsvg_handle_new_from_gfile_sync(
      gfile, RSVG_HANDLE_FLAGS_NONE, NULL, &error);

  if (error) {
    g_object_unref(error);
    if (handle) g_object_unref(handle);
    handle = NULL;
  }

  g_object_unref(gfile);

  return handle;
}

int rsvg_destroy_handle(RsvgHandle *handle) {
  if (handle) { g_object_unref(handle); }

  return 0;
}

RsvgRectangle *rsvg_rectangle_create(void) {
  return calloc(1, sizeof(RsvgRectangle));
}

void rsvg_rectangle_destroy(RsvgRectangle *rect) { free(rect); }

void rsvg_rectangle_set(RsvgRectangle *rect, double x, double y, double width,
                        double height) {
  if (rect) {
    rect->x = x;
    rect->y = y;
    rect->width = width;
    rect->height = height;
  }
}

void rsvg_rectangle_get(RsvgRectangle *rect, double *x, double *y,
                        double *width, double *height) {
  if (rect) {
    *x = rect->x;
    *y = rect->y;
    *width = rect->width;
    *height = rect->height;
  }
}

#endif /* _LIBRSVG_HELPER_H_ */
