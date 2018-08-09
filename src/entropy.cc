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

#include "config.h"
#include "conky.h"
#include "text_object.h"
#include <inttypes.h>
#include <time.h>

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "freebsd.h"
#elif defined(__DragonFly__)
#include "dragonfly.h"
#elif defined(__OpenBSD__)
#include "openbsd.h"
#elif defined(__sun)
#include "solaris.h"
#elif defined(__HAIKU__)
#include "haiku.h"
#elif defined(__APPLE__) && defined(__MACH__)
#include "darwin.h"
#endif

struct _entropy {
  _entropy() = default;
  unsigned int avail{0};
  unsigned int poolsize{0};
};

static _entropy entropy;

int update_entropy() {
  get_entropy_avail(&entropy.avail);
  get_entropy_poolsize(&entropy.poolsize);
  return 0;
}

void print_entropy_avail(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  snprintf(p, p_max_size, "%u", entropy.avail);
}

uint8_t entropy_percentage(struct text_object *obj) {
  (void)obj;
  return round_to_int(static_cast<double>(entropy.avail) * 100.0 /
                      static_cast<double>(entropy.poolsize));
}

void print_entropy_poolsize(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  snprintf(p, p_max_size, "%u", entropy.poolsize);
}

double entropy_barval(struct text_object *obj) {
  (void)obj;

  return static_cast<double>(entropy.avail) / entropy.poolsize;
}

void print_password(struct text_object *obj, char *p, unsigned int p_max_size) {
  time_t t;
  static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789~!@#$%^&*()_";
  static const int len = (int)sizeof(letters) - 1;
  uintmax_t x = strtoumax(obj->data.s, (char **)NULL, 10);
  uintmax_t z = 0;

  if (-1 == (t = time(NULL))) {
    return;
  }
  srandom((unsigned int)t);

  for (; z < x && p_max_size-1 > z; z++) {
    *p++ = letters[random() % len];
  }
  *p = '\0';
}
