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

#ifndef _LIBRSVG_HELPER_H_
#define _LIBRSVG_HELPER_H_

#include <glib.h>
#include <stdlib.h>
#include <librsvg/rsvg.h>

RsvgDimensionData *
rsvgDimensionDataCreate(void)
{
  return (RsvgDimensionData *)calloc(1, sizeof(RsvgDimensionData));
}

void
rsvgDimensionDataGet(RsvgDimensionData * dd,
                     int * width, int * height, double * em, double * ex)
{
  if (dd) {
    *width = dd->width;
    *height = dd->height;
    *em = dd->em;
    *ex = dd->ex;
  }
}

RsvgPositionData *
rsvgPositionDataCreate(void)
{
  return (RsvgPositionData *)calloc(1, sizeof(RsvgPositionData));
}

void
rsvgPositionDataGet(RsvgPositionData * pd, int * x, int * y)
{
  if (pd) {
    *x = pd->x;
    *y = pd->y;
  }
}

RsvgHandle *
rsvg_create_handle_from_file(const char * filename)
{
  GError * error = NULL;
  RsvgHandle * handle = rsvg_handle_new_from_file(filename, &error);

  if (error) {
    g_object_unref(error);
    if (handle)
      g_object_unref(handle);
    handle = NULL;
  }

  return handle;
}

int
rsvg_destroy_handle(RsvgHandle * handle)
{
  int status = 0;

  if (handle) {
    GError * error = NULL;
    status = rsvg_handle_close(handle, &error);

    if (status)
      g_object_unref(handle);

    if (error)
      g_object_unref(error);
  }

  return status;
}

#endif /* _LIBRSVG_HELPER_H_ */
