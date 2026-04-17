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

// TODO: guard behind BUILD_LEGACY_HARDWARE. The /proc/i8k interface is
// deprecated. Modern Dell laptops use dell-smm-hwmon via standard hwmon/sysfs.
// Shouldn't be removed as keeping support is trivial, but it shouldn't be
// included in builds by default at this point.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "conky.h"
#include "logging.h"
#include "content/temphelper.h"
#include "content/text_object.h"
#include "parse/variables.hh"

using namespace conky::text_object;

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
constexpr const char *proc_i8k = "/proc/i8k"; // TODO: char * should be std::filesystem::path
constexpr const char *i8k_delim = " ";

static char *i8k_procbuf = nullptr;
int update_i8k(void) {
  FILE *fp;

  if ((fp = fopen(proc_i8k, "r")) == nullptr) {
    LOG_ERROR(
        "failed to open /proc/i8k; ensure the i8k kernel module is loaded "
        "(modprobe dell-smm-hwmon or insmod i8k)");
    return 1;
  }

  if (!i8k_procbuf) { i8k_procbuf = (char *)malloc(128 * sizeof(char)); }
  memset(&i8k_procbuf[0], 0, 128);
  if (fread(&i8k_procbuf[0], sizeof(char), 128, fp) == 0) {
    LOG_ERROR("failed to read from /proc/i8k: fread returned 0 bytes");
  }

  fclose(fp);

  LOG_DEBUG("read '{}' from /proc/i8k", i8k_procbuf);

  // FIXME: replace strtok chain with a string_view tokenizer that terminates
  // gracefully on short input. Fields should be std::string (empty on missing)
  // instead of char* (nullptr on missing) to avoid null dereference in print
  // functions like cpu_temp/ac_status/fan_status.
  i8k.version = strtok(&i8k_procbuf[0], i8k_delim);
  i8k.bios = strtok(nullptr, i8k_delim);
  i8k.serial = strtok(nullptr, i8k_delim);
  i8k.cpu_temp = strtok(nullptr, i8k_delim);
  i8k.left_fan_status = strtok(nullptr, i8k_delim);
  i8k.right_fan_status = strtok(nullptr, i8k_delim);
  i8k.left_fan_rpm = strtok(nullptr, i8k_delim);
  i8k.right_fan_rpm = strtok(nullptr, i8k_delim);
  i8k.ac_status = strtok(nullptr, i8k_delim);
  i8k.buttons_status = strtok(nullptr, i8k_delim);
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

void print_i8k_cpu_temp(struct text_object *obj, char *p,
                        unsigned int p_max_size) {
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

void print_i8k_ac_status(struct text_object *obj, char *p,
                         unsigned int p_max_size) {
  int ac_status;

  (void)obj;

  sscanf(i8k.ac_status, "%d", &ac_status);
  if (ac_status == -1) {
    snprintf(p, p_max_size, "%s", "disabled (read i8k docs)");
  }
  if (ac_status == 0) { snprintf(p, p_max_size, "%s", "off"); }
  if (ac_status == 1) { snprintf(p, p_max_size, "%s", "on"); }
}

// clang-format off
CONKY_REGISTER_VARIABLES(
    print_variable("i8k_version", +[] { return i8k.version; }, &update_i8k),
    print_variable("i8k_bios", +[] { return i8k.bios; }, &update_i8k),
    print_variable("i8k_serial", +[] { return i8k.serial; }, &update_i8k),
    {"i8k_cpu_temp", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = &print_i8k_cpu_temp;
    }, &update_i8k},
    {"i8k_left_fan_status", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = &print_i8k_left_fan_status;
    }, &update_i8k},
    {"i8k_right_fan_status", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = &print_i8k_right_fan_status;
    }, &update_i8k},
    print_variable("i8k_left_fan_rpm", +[] { return i8k.left_fan_rpm; }, &update_i8k),
    print_variable("i8k_right_fan_rpm", +[] { return i8k.right_fan_rpm; }, &update_i8k),
    {"i8k_ac_status", [](text_object *obj, const construct_context &) {
      obj->callbacks.print = &print_i8k_ac_status;
    }, &update_i8k},
    print_variable("i8k_buttons_status", +[] { return i8k.buttons_status; }, &update_i8k),
)
// clang-format on
