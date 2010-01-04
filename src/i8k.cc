/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
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
 * Copyright (c) 2005-2010 Brenden Matthews, Philip Kovacs, et. al.
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
static char *i8k_procbuf = NULL;
void update_i8k(void)
{
	FILE *fp;

	if (!i8k_procbuf) {
		i8k_procbuf = (char *) malloc(128 * sizeof(char));
	}
	if ((fp = fopen(PROC_I8K, "r")) == NULL) {
		CRIT_ERR(NULL, NULL, "/proc/i8k doesn't exist! use insmod to make sure the kernel "
			"driver is loaded...");
	}

	memset(&i8k_procbuf[0], 0, 128);
	if (fread(&i8k_procbuf[0], sizeof(char), 128, fp) == 0) {
		NORM_ERR("something wrong with /proc/i8k...");
	}

	fclose(fp);

	i8k.version = strtok(&i8k_procbuf[0], I8K_DELIM);
	i8k.bios = strtok(NULL, I8K_DELIM);
	i8k.serial = strtok(NULL, I8K_DELIM);
	i8k.cpu_temp = strtok(NULL, I8K_DELIM);
	i8k.left_fan_status = strtok(NULL, I8K_DELIM);
	i8k.right_fan_status = strtok(NULL, I8K_DELIM);
	i8k.left_fan_rpm = strtok(NULL, I8K_DELIM);
	i8k.right_fan_rpm = strtok(NULL, I8K_DELIM);
	i8k.ac_status = strtok(NULL, I8K_DELIM);
	i8k.buttons_status = strtok(NULL, I8K_DELIM);
}

static const char *fan_status_to_string(int status)
{
	switch(status) {
		case 0: return "off";
		case 1: return "low";
		case 2: return "high";
	}
	return "error";
}

void print_i8k_left_fan_status(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%s",
	         fan_status_to_string(atoi(i8k.left_fan_status)));
}

void print_i8k_cpu_temp(struct text_object *obj, char *p, int p_max_size)
{
	int cpu_temp;

	(void)obj;

	sscanf(i8k.cpu_temp, "%d", &cpu_temp);
	temp_print(p, p_max_size, (double)cpu_temp, TEMP_CELSIUS);
}

void print_i8k_right_fan_status(struct text_object *obj, char *p, int p_max_size)
{
	(void)obj;
	snprintf(p, p_max_size, "%s",
	         fan_status_to_string(atoi(i8k.right_fan_status)));
}

void print_i8k_ac_status(struct text_object *obj, char *p, int p_max_size)
{
	int ac_status;

	(void)obj;

	sscanf(i8k.ac_status, "%d", &ac_status);
	if (ac_status == -1) {
		snprintf(p, p_max_size, "disabled (read i8k docs)");
	}
	if (ac_status == 0) {
		snprintf(p, p_max_size, "off");
	}
	if (ac_status == 1) {
		snprintf(p, p_max_size, "on");
	}
}

#define I8K_PRINT_GENERATOR(name) \
void print_i8k_##name(struct text_object *obj, char *p, int p_max_size) \
{ \
	(void)obj; \
	snprintf(p, p_max_size, "%s", i8k.name); \
}

I8K_PRINT_GENERATOR(version)
I8K_PRINT_GENERATOR(bios)
I8K_PRINT_GENERATOR(serial)
I8K_PRINT_GENERATOR(left_fan_rpm)
I8K_PRINT_GENERATOR(right_fan_rpm)
I8K_PRINT_GENERATOR(buttons_status)

#undef I8K_PRINT_GENERATOR
