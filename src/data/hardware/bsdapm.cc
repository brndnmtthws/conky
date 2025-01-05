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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "config.h"
#include "../../conky.h"
#include "../../content/text_object.h"

#if defined(__OpenBSD__)
#include <machine/apmvar.h>
#else
#include <machine/apm_bios.h>
#endif

const char *APMDEV = "/dev/apm";
const u_int APM_UNKNOWN = 255;

#ifndef APM_AC_OFF
#define APM_AC_OFF 0
#endif

#ifndef APM_AC_ON
#define APM_AC_ON 1
#endif

#ifndef APM_BATT_CHARGING
#define APM_BATT_CHARGING 3
#endif

#ifdef __OpenBSD__
const u_long GET_APM_INFO = APM_IOC_GETPOWER;
using apm_info = apm_power_info;
#define seconds_left minutes_left * 1000
#else
const u_long GET_APM_INFO = APMIO_GETINFO;
#define ac_state ai_acline
#define battery_state ai_batt_stat
#define battery_life ai_batt_life
#define seconds_left ai_batt_time
#endif

static bool apm_getinfo(int fd, apm_info *aip) {
  return ioctl(fd, GET_APM_INFO, aip) == -1;
}

void print_apm_adapter(struct text_object *obj, char *p,
                       unsigned int p_max_size) {
  int fd;
  apm_info info;

  (void)obj;

  fd = open(APMDEV, O_RDONLY);
  if (fd < 0) {
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }

  if (!apm_getinfo(fd, &info)) {
    close(fd);
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }
  close(fd);

  const char *out;
  switch (info.ac_state) {
    case APM_AC_OFF:
      out = "off-line";
      break;
    case APM_AC_ON:
      if (info.battery_state == APM_BATT_CHARGING) {
        out = "charging";
      } else {
        out = "on-line";
      }
      break;
    default:
      out = "unknown";
      break;
  }
  snprintf(p, p_max_size, "%s", out);
}

void print_apm_battery_life(struct text_object *obj, char *p,
                            unsigned int p_max_size) {
  int fd;
  apm_info info;

  (void)obj;

  fd = open(APMDEV, O_RDONLY);
  if (fd < 0) {
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }

  if (!apm_getinfo(fd, &info)) {
    close(fd);
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }
  close(fd);

  if (info.battery_life <= 100) {
    snprintf(p, p_max_size, "%d%%", info.battery_life);
    return;
  }

  const char *out;
  if (info.battery_life == APM_UNKNOWN) {
    out = "unknown";
  } else {
    out = "ERR";
  }
  
  snprintf(p, p_max_size, "%s", out);
}

void print_apm_battery_time(struct text_object *obj, char *p,
                            unsigned int p_max_size) {
  int fd;
  int h, m, s;
  apm_info info;

  (void)obj;

  fd = open(APMDEV, O_RDONLY);
  if (fd < 0) {
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }

  if (!apm_getinfo(fd, &info)) {
    close(fd);
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }
  close(fd);

  int batt_time = info.seconds_left;
  if (batt_time == -1) {
    snprintf(p, p_max_size, "%s", "unknown");
  } else {
    h = batt_time;
    s = h % 60;
    h /= 60;
    m = h % 60;
    h /= 60;
    snprintf(p, p_max_size, "%2d:%02d:%02d", h, m, s);
  }
}
