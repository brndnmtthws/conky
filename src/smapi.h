/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * smapi.h:  conky support for IBM Thinkpad smapi
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

#ifndef _SMAPI_H
#define _SMAPI_H

int smapi_bat_installed(int);

char *smapi_read_str(const char *);
int smapi_read_int(const char *);

char *smapi_get_str(const char *);
char *smapi_get_val(const char *);

char *smapi_get_bat_str(int, const char *);
int smapi_get_bat_int(int, const char *);
char *smapi_get_bat_val(const char *);

void print_smapi(struct text_object *, char *, int);
uint8_t smapi_bat_percentage(struct text_object *);
void print_smapi_bat_temp(struct text_object *, char *, int);
void print_smapi_bat_power(struct text_object *, char *, int);
double smapi_bat_barval(struct text_object *);
int smapi_bat_installed(struct text_object *obj)

#endif /* _SMAPI_H */
