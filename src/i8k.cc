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
 * Copyright (c) 2007 Toni Spets
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "conky.h"
#include "logging.h"
#include "temphelper.h"
#include "text_object.h"

struct _i8k {
  char *version;
  char *bios;
  char *serial;
  char *cpu_temp;
  char *left_fan_status;
  char *right_fan_status;
  char *left_fan_rpm;
  char *right_fan_rpm;
  char *ac_status;
  char *buttons_status;
} i8k;

/* FIXME: there should be an ioctl interface to request specific data */
#define PROC_I8K "/proc/i8k"
#define I8K_DELIM " "
static char *i8k_procbuf = nullptr;
int update_i8k(void) {
  FILE *fp;

  if (!i8k_procbuf) {
    i8k_procbuf = (char *)malloc(128 * sizeof(char));
  }
  if ((fp = fopen(PROC_I8K, "r")) == nullptr) {
    free_and_zero(i8k_procbuf);
    /*THREAD_CRIT_ERR(nullptr, NULL, "/proc/i8k doesn't exist! use insmod to make
      sure the kernel " "driver is loaded...");*/
    NORM_ERR(
        "/proc/i8k doesn't exist! use insmod to make sure the kernel driver is "
        "loaded...");
    clean_up_without_threads(nullptr, NULL);
    return 1;
  }

  memset(&i8k_procbuf[0], 0, 128);
  if (fread(&i8k_procbuf[0], sizeof(char), 128, fp) == 0) {
    NORM_ERR("something wrong with /proc/i8k...");
  }

  fclose(fp);

  DBGP("read `%s' from /proc/i8k\n", i8k_procbuf);

  i8k.version = strtok(&i8k_procbuf[0], I8K_DELIM);
  i8k.bios = strtok(nullptr, I8K_DELIM);
  i8k.serial = strtok(nullptr, I8K_DELIM);
  i8k.cpu_temp = strtok(nullptr, I8K_DELIM);
  i8k.left_fan_status = strtok(nullptr, I8K_DELIM);
  i8k.right_fan_status = strtok(nullptr, I8K_DELIM);
  i8k.left_fan_rpm = strtok(nullptr, I8K_DELIM);
  i8k.right_fan_rpm = strtok(nullptr, I8K_DELIM);
  i8k.ac_status = strtok(nullptr, I8K_DELIM);
  i8k.buttons_status = strtok(nullptr, I8K_DELIM);
  return 0;
}

static void print_i8k_fan_status(char *p, int p_max_size, const char *status) {
  static const char *status_arr[] = {"off", "low", "high", "error"};

  int i = status ? atoi(status) : 3;
  if (i < 0 || i > 3) i = 3;

  snprintf(p, p_max_size, "%s", status_arr[i]);
}

void print_i8k_left_fan_status(struct text_object *obj, char *p,
                               unsigned int p_max_size) {
  (void)obj;
  print_i8k_fan_status(p, p_max_size, i8k.left_fan_status);
}

void print_i8k_cpu_temp(struct text_object *obj, char *p, unsigned int p_max_size) {
  int cpu_temp;

  (void)obj;

  sscanf(i8k.cpu_temp, "%d", &cpu_temp);
  temp_print(p, p_max_size, (double)cpu_temp, TEMP_CELSIUS, 1);
}

void print_i8k_right_fan_status(struct text_object *obj, char *p,
                                unsigned int p_max_size) {
  (void)obj;
  print_i8k_fan_status(p, p_max_size, i8k.right_fan_status);
}

void print_i8k_ac_status(struct text_object *obj, char *p, unsigned int p_max_size) {
  int ac_status;

  (void)obj;

  sscanf(i8k.ac_status, "%d", &ac_status);
  if (ac_status == -1) {
    snprintf(p, p_max_size, "%s", "disabled (read i8k docs)");
  }
  if (ac_status == 0) {
    snprintf(p, p_max_size, "%s", "off");
  }
  if (ac_status == 1) {
    snprintf(p, p_max_size, "%s", "on");
  }
}

#define I8K_PRINT_GENERATOR(name)                                           \
  void print_i8k_##name(struct text_object *obj, char *p,                   \
		                                 unsigned int p_max_size) { \
    (void)obj;                                                              \
    snprintf(p, p_max_size, "%s", i8k.name);                                \
  }

I8K_PRINT_GENERATOR(version)
I8K_PRINT_GENERATOR(bios)
I8K_PRINT_GENERATOR(serial)
I8K_PRINT_GENERATOR(left_fan_rpm)
I8K_PRINT_GENERATOR(right_fan_rpm)
I8K_PRINT_GENERATOR(buttons_status)

#undef I8K_PRINT_GENERATOR
