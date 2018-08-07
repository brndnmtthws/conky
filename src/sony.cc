/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
 * Copyright (c) 2009 Yeon-Hyeong Yang <lbird94@gmail.com>
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
/* conky support for information from sony_laptop kernel module
 *   information from sony_laptop kernel module
 *   /sys/devices/platform/sony-laptop
 *   I mimicked the methods from ibm.c
 * Yeon-Hyeong Yang <lbird94@gmail.com> */

#include "sony.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "conky.h"
#include "logging.h"
#include "text_object.h"

#define SONY_LAPTOP_DIR "/sys/devices/platform/sony-laptop"

/* fanspeed in SONY_LAPTOP_DIR contains an integer value for fanspeed (0~255).
 * I don't know the exact measurement unit, though. I may assume that 0 for
 * 'fan stopped' and 255 for 'maximum fan speed'. */
void get_sony_fanspeed(struct text_object *obj, char *p_client_buffer,
                       unsigned int client_buffer_size) {
  FILE *fp;
  unsigned int speed = 0;
  char fan[128];

  (void)obj;

  if (!p_client_buffer || client_buffer_size <= 0) {
    return;
  }

  snprintf(fan, 127, "%s/fanspeed", SONY_LAPTOP_DIR);

  fp = fopen(fan, "r");
  if (fp != nullptr) {
    while (!feof(fp)) {
      char line[256];

      if (fgets(line, 255, fp) == nullptr) {
        break;
      }
      if (sscanf(line, "%u", &speed)) {
        break;
      }
    }
  } else {
    CRIT_ERR(nullptr, NULL,
             "can't open '%s': %s\nEnable sony support or remove "
             "sony* from your " PACKAGE_NAME " config file.",
             fan, strerror(errno));
  }

  fclose(fp);
  snprintf(p_client_buffer, client_buffer_size, "%d", speed);
}
