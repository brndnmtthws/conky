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
 * Copyright (c) 2005-2019 Brenden Matthews, Philip Kovacs, et. al.
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
#include <machine/apm_bios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "config.h"
#include "conky.h"
#include "text_object.h"

#define APMDEV "/dev/apm"
#define APM_UNKNOWN 255

#ifndef APM_AC_OFF
#define APM_AC_OFF 0
#endif

#ifndef APM_AC_ON
#define APM_AC_ON 1
#endif

#ifndef APM_BATT_CHARGING
#define APM_BATT_CHARGING 3
#endif

static int apm_getinfo(int fd, apm_info_t aip) {
#ifdef __OpenBSD__
  if (ioctl(fd, APM_IOC_GETPOWER, aip) == -1) {
#else
  if (ioctl(fd, APMIO_GETINFO, aip) == -1) {
#endif
    return -1;
  }

  return 0;
}

void print_apm_adapter(struct text_object *obj, char *p,
                       unsigned int p_max_size) {
  int fd;
  const char *out;
#ifdef __OpenBSD__
  struct apm_power_info a_info;
#else
  struct apm_info a_info;
#endif

  (void)obj;

  fd = open(APMDEV, O_RDONLY);
  if (fd < 0) {
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }

  if (apm_getinfo(fd, &a_info) != 0) {
    close(fd);
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }
  close(fd);

#ifdef __OpenBSD__
#define ai_acline ac_state
#endif
  switch (a_info.ai_acline) {
    case APM_AC_OFF:
      out = "off-line";
      break;
    case APM_AC_ON:
#ifdef __OpenBSD__
#define ai_batt_stat battery_state
#endif
      if (a_info.ai_batt_stat == APM_BATT_CHARGING) {
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
  u_int batt_life;
  const char *out;
#ifdef __OpenBSD__
  struct apm_power_info a_info;
#else
  struct apm_info a_info;
#endif

  (void)obj;

  fd = open(APMDEV, O_RDONLY);
  if (fd < 0) {
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }

  if (apm_getinfo(fd, &a_info) != 0) {
    close(fd);
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }
  close(fd);

#ifdef __OpenBSD__
#define ai_batt_life battery_life
#endif
  batt_life = a_info.ai_batt_life;
  if (batt_life == APM_UNKNOWN) {
    out = "unknown";
  } else if (batt_life <= 100) {
    snprintf(p, p_max_size, "%d%%", batt_life);
    return;
  } else {
    out = "ERR";
  }

  snprintf(p, p_max_size, "%s", out);
}

void print_apm_battery_time(struct text_object *obj, char *p,
                            unsigned int p_max_size) {
  int fd;
  int batt_time;
#ifdef __OpenBSD__
  int h, m;
  struct apm_power_info a_info;
#else
  int h, m, s;
  struct apm_info a_info;
#endif

  (void)obj;

  fd = open(APMDEV, O_RDONLY);
  if (fd < 0) {
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }

  if (apm_getinfo(fd, &a_info) != 0) {
    close(fd);
    snprintf(p, p_max_size, "%s", "ERR");
    return;
  }
  close(fd);

#ifdef __OpenBSD__
#define ai_batt_time minutes_left
#endif
  batt_time = a_info.ai_batt_time;

  if (batt_time == -1) {
    snprintf(p, p_max_size, "%s", "unknown");
  } else
#ifdef __OpenBSD__
  {
    h = batt_time / 60;
    m = batt_time % 60;
    snprintf(p, p_max_size, "%2d:%02d", h, m);
  }
#else
  {
    h = batt_time;
    s = h % 60;
    h /= 60;
    m = h % 60;
    h /= 60;
    snprintf(p, p_max_size, "%2d:%02d:%02d", h, m, s);
  }
#endif
}
