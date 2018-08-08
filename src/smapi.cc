/*
 *
 * smapi.c:  conky support for IBM Thinkpad smapi
 *
 * Copyright (C) 2007 Phil Sutter <Phil@nwl.cc>
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "conky.h" /* text_buffer_size, PACKAGE_NAME, maybe more */
#include "logging.h"
#include "temphelper.h"

#define SYS_SMAPI_PATH "/sys/devices/platform/smapi"

static int smapi_read_int(const char *path) {
  FILE *fp;
  int i = 0;
  if ((fp = fopen(path, "r"))) {
    if (fscanf(fp, "%i\n", &i) < 0) perror("fscanf()");
    fclose(fp);
  }
  return i;
}

static int smapi_bat_installed_internal(int idx) {
  char path[128];
  struct stat sb;
  int ret = 0;

  snprintf(path, 127, SYS_SMAPI_PATH "/BAT%i", idx);
  if (!stat(path, &sb) && (sb.st_mode & S_IFMT) == S_IFDIR) {
    snprintf(path, 127, SYS_SMAPI_PATH "/BAT%i/installed", idx);
    ret = (smapi_read_int(path) == 1) ? 1 : 0;
  }
  return ret;
}

static char *smapi_read_str(const char *path) {
  FILE *fp;
  char str[256] = "failed";
  if ((fp = fopen(path, "r")) != nullptr) {
    if (fscanf(fp, "%255s\n", str) < 0) perror("fscanf()");
    fclose(fp);
  }
  return strndup(str, text_buffer_size.get(*state));
}

static char *smapi_get_str(const char *fname) {
  char path[128];
  if (snprintf(path, 127, SYS_SMAPI_PATH "/%s", fname) < 0) return nullptr;

  return smapi_read_str(path);
}

static char *smapi_get_bat_str(int idx, const char *fname) {
  char path[128];
  if (snprintf(path, 127, SYS_SMAPI_PATH "/BAT%i/%s", idx, fname) < 0)
    return nullptr;
  return smapi_read_str(path);
}

static int smapi_get_bat_int(int idx, const char *fname) {
  char path[128];
  if (snprintf(path, 127, SYS_SMAPI_PATH "/BAT%i/%s", idx, fname) < 0) return 0;
  return smapi_read_int(path);
}

static char *smapi_get_bat_val(const char *args) {
  char fname[128];
  int idx, cnt;

  if (sscanf(args, "%i %n", &idx, &cnt) <= 0 ||
      snprintf(fname, 127, "%s", (args + cnt)) < 0) {
    NORM_ERR("smapi: wrong arguments, should be 'bat,<int>,<str>'");
    return nullptr;
  }

  if (!smapi_bat_installed_internal(idx)) return nullptr;

  return smapi_get_bat_str(idx, fname);
}

static char *smapi_get_val(const char *args) {
  char str[128];

  if (!args || sscanf(args, "%127s", str) <= 0) return nullptr;

  if (!strcmp(str, "bat")) return smapi_get_bat_val(args + 4);

  return smapi_get_str(str);
}

void print_smapi(struct text_object *obj, char *p, unsigned int p_max_size) {
  char *s;

  if (!obj->data.s) return;

  s = smapi_get_val(obj->data.s);
  snprintf(p, p_max_size, "%s", s);
  free(s);
}

uint8_t smapi_bat_percentage(struct text_object *obj) {
  int idx, val = 0;
  if (obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
    val = smapi_bat_installed_internal(idx)
              ? smapi_get_bat_int(idx, "remaining_percent")
              : 0;
  } else
    NORM_ERR("argument to smapi_bat_perc must be an integer");

  return val;
}

void print_smapi_bat_temp(struct text_object *obj, char *p, unsigned int p_max_size) {
  int idx, val;
  if (obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
    val = smapi_bat_installed_internal(idx)
              ? smapi_get_bat_int(idx, "temperature")
              : 0;
    /* temperature is in milli degree celsius */
    temp_print(p, p_max_size, val / 1000, TEMP_CELSIUS, 1);
  } else
    NORM_ERR("argument to smapi_bat_temp must be an integer");
}

void print_smapi_bat_power(struct text_object *obj, char *p, unsigned int p_max_size) {
  int idx, val;
  if (obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
    val = smapi_bat_installed_internal(idx)
              ? smapi_get_bat_int(idx, "power_now")
              : 0;
    /* power_now is in mW, set to W with one digit precision */
    snprintf(p, p_max_size, "%.1f", ((double)val / 1000));
  } else
    NORM_ERR("argument to smapi_bat_power must be an integer");
}

double smapi_bat_barval(struct text_object *obj) {
  if (obj->data.i >= 0 && smapi_bat_installed_internal(obj->data.i))
    return smapi_get_bat_int(obj->data.i, "remaining_percent");
  return 0;
}

int smapi_bat_installed(struct text_object *obj) {
  int idx;
  if (obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
    if (!smapi_bat_installed_internal(idx)) {
      return 0;
    }
  } else
    NORM_ERR("argument to if_smapi_bat_installed must be an integer");
  return 1;
}
