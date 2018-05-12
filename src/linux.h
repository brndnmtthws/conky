/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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

#ifndef _LINUX_H
#define _LINUX_H

#include "common.h"

void print_disk_protect_queue(struct text_object *, char *, int);

void print_ioscheduler(struct text_object *, char *, int);
void print_laptop_mode(struct text_object *, char *, int);

int update_gateway_info(void);
void free_gateway_info(struct text_object *obj);
int gateway_exists(struct text_object *);
void print_gateway_iface(struct text_object *, char *, int);
void print_gateway_ip(struct text_object *, char *, int);

enum { PB_BATT_STATUS, PB_BATT_PERCENT, PB_BATT_TIME };
void get_powerbook_batt_info(struct text_object *, char *, int);

void parse_i2c_sensor(struct text_object *, const char *);
void parse_hwmon_sensor(struct text_object *, const char *);
void parse_platform_sensor(struct text_object *, const char *);
void print_sysfs_sensor(struct text_object *, char *, int);
void free_sysfs_sensor(struct text_object *);

int get_entropy_avail(unsigned int *);
int get_entropy_poolsize(unsigned int *);

int update_stat(void);

void print_distribution(struct text_object *, char *, int);

void determine_longstat_file(void);
#endif /* _LINUX_H */
