/*
 *
 * Conky, a system monitor, based on torsmo.
 *
 * Any original torsmo code is licensed under the BSD license.
 * All code written since the fork of torsmo is licensed under the GPL.
 * Please see COPYING for details.
 *
 * Copyright (c) 2021 Rogier Reerink
 *  (See AUTHORS)
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "intel_backlight.h"
#include "logging.h"

#define FS_BRIGHTNESS_MAX "/sys/class/backlight/intel_backlight/max_brightness"
#define FS_BRIGHTNESS_CURRENT "/sys/class/backlight/intel_backlight/brightness"

struct backlight {
  FILE *fp_max;
  unsigned max;
  FILE *fp_current;
  unsigned current;
};

void open_backlight(struct backlight *bl) {
  bl->fp_max = fopen(FS_BRIGHTNESS_MAX, "r");
  if (bl->fp_max == NULL) {
    NORM_ERR("Failed to open file: '" FS_BRIGHTNESS_MAX "'.");
  }
  bl->fp_current = fopen(FS_BRIGHTNESS_CURRENT, "r");
  if (bl->fp_current == NULL) {
    NORM_ERR("Failed to open file: '" FS_BRIGHTNESS_CURRENT "'.");
  }
}

void read_backlight(struct backlight *bl) {
  FILE *fp_max, *fp_current;
  fp_max = bl->fp_max;
  if (fp_max != NULL) {
    rewind(fp_max);
    fflush(fp_max);
    if (fscanf(fp_max, "%u", &(bl->max)) < 0) {
      NORM_ERR("Failed to read maximum brightness.");
    }
  } else {
    bl->max = 0;
  }
  fp_current = bl->fp_current;
  if (fp_current != NULL) {
    rewind(fp_current);
    fflush(fp_current);
    if (fscanf(fp_current, "%u", &(bl->current)) < 0) {
      NORM_ERR("Failed to read current brightness.");
    }
  } else {
    bl->current = 0;
  }
}

unsigned get_backlight_percent(struct backlight *bl) {
  read_backlight(bl);
  if (bl->max == 0) {
    return 0;
  } else {
    return bl->current * 100.0 / bl->max + 0.5;
  }
}

void close_backlight(struct backlight *bl) {
  if (bl->fp_max != NULL) { fclose(bl->fp_max); }
  if (bl->fp_current != NULL) { fclose(bl->fp_current); }
}

void init_intel_backlight(struct text_object *obj) {
  struct backlight *bl = (struct backlight *)malloc(sizeof(struct backlight));
  open_backlight(bl);
  obj->data.opaque = bl;
}

void free_intel_backlight(struct text_object *obj) {
  struct backlight *bl = (struct backlight *)obj->data.opaque;
  close_backlight(bl);
  free(bl);
}

void print_intel_backlight(struct text_object *obj, char *p,
                           unsigned int p_max_size) {
  struct backlight *bl = (struct backlight *)obj->data.opaque;
  unsigned percent = get_backlight_percent(bl);
  snprintf(p, p_max_size, "%d", percent);
}