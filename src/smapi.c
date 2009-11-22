/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
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
#define _GNU_SOURCE
#include "conky.h"	/* text_buffer_size, PACKAGE_NAME, maybe more */
#include "smapi.h"
#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SYS_SMAPI_PATH "/sys/devices/platform/smapi"

int smapi_bat_installed(int idx)
{
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

char *smapi_read_str(const char *path)
{
	FILE *fp;
	char str[256] = "failed";
	if ((fp = fopen(path, "r")) != NULL) {
		fscanf(fp, "%255s\n", str);
		fclose(fp);
	}
	return strndup(str, text_buffer_size);
}

int smapi_read_int(const char *path)
{
	FILE *fp;
	int i = 0;
	if ((fp = fopen(path, "r")) != NULL) {
		fscanf(fp, "%i\n", &i);
		fclose(fp);
	}
	return i;
}

char *smapi_get_str(const char *fname)
{
	char path[128];
	if(snprintf(path, 127, SYS_SMAPI_PATH "/%s", fname) < 0)
		return NULL;

	return smapi_read_str(path);
}

char *smapi_get_bat_str(int idx, const char *fname)
{
	char path[128];
	if(snprintf(path, 127, SYS_SMAPI_PATH "/BAT%i/%s", idx, fname) < 0)
		return NULL;
	return smapi_read_str(path);
}

int smapi_get_bat_int(int idx, const char *fname)
{
	char path[128];
	if(snprintf(path, 127, SYS_SMAPI_PATH "/BAT%i/%s", idx, fname) < 0)
		return 0;
	return smapi_read_int(path);
}

char *smapi_get_bat_val(const char *args)
{
	char fname[128];
	int idx, cnt;

	if(sscanf(args, "%i %n", &idx, &cnt) <= 0 ||
	   snprintf(fname, 127, "%s", (args + cnt)) < 0) {
		NORM_ERR("smapi: wrong arguments, should be 'bat,<int>,<str>'");
		return NULL;
	}

	if(!smapi_bat_installed(idx))
		return NULL;

	return smapi_get_bat_str(idx, fname);
}

char *smapi_get_val(const char *args)
{
	char str[128];

	if(!args || sscanf(args, "%127s", str) <= 0)
		return NULL;

	if(!strcmp(str, "bat"))
		return smapi_get_bat_val(args + 4);

	return smapi_get_str(str);
}

void print_smapi(struct text_object *obj, char *p, int p_max_size)
{
	char *s;

	if (!obj->data.s)
		return;

	s = smapi_get_val(obj->data.s);
	snprintf(p, p_max_size, "%s", s);
	free(s);
}

void print_smapi_bat_perc(struct text_object *obj, char *p, int p_max_size)
{
	int idx, val;
	if (obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
		val = smapi_bat_installed(idx) ?
			smapi_get_bat_int(idx, "remaining_percent") : 0;
		percent_print(p, p_max_size, val);
	} else
		NORM_ERR("argument to smapi_bat_perc must be an integer");
}

void print_smapi_bat_temp(struct text_object *obj, char *p, int p_max_size)
{
	int idx, val;
	if (obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
		val = smapi_bat_installed(idx) ?
			smapi_get_bat_int(idx, "temperature") : 0;
		/* temperature is in milli degree celsius */
		temp_print(p, p_max_size, val / 1000, TEMP_CELSIUS);
	} else
		NORM_ERR("argument to smapi_bat_temp must be an integer");
}

void print_smapi_bat_power(struct text_object *obj, char *p, int p_max_size)
{
	int idx, val;
	if (obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
		val = smapi_bat_installed(idx) ?
			smapi_get_bat_int(idx, "power_now") : 0;
		/* power_now is in mW, set to W with one digit precision */
		snprintf(p, p_max_size, "%.1f", ((double)val / 1000));
	} else
		NORM_ERR("argument to smapi_bat_power must be an integer");
}

void print_smapi_bat_bar(struct text_object *obj, char *p, int p_max_size)
{
	if (!p_max_size)
		return;

	if (obj->data.i >= 0 && smapi_bat_installed(obj->data.i))
		new_bar(obj, p, p_max_size, (int)
				(255 * smapi_get_bat_int(obj->data.i, "remaining_percent") / 100));
	else
		new_bar(obj, p, p_max_size, 0);
}
