/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 * apcupsd.h:  conky module for APC UPS daemon monitoring
 *
 * Copyright (C) 2009 Jaromir Smrcek <jaromir.smrcek@zoner.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 */

#ifndef APCUPSD_H_
#define APCUPSD_H_

#ifdef __cplusplus
extern "C" {
#endif

int apcupsd_scan_arg(const char *);

/* Service routine for the conky main thread */
void update_apcupsd(void);

double apcupsd_loadbarval(struct text_object *);

void print_apcupsd_name(struct text_object *, char *, int);
void print_apcupsd_model(struct text_object *, char *, int);
void print_apcupsd_upsmode(struct text_object *, char *, int);
void print_apcupsd_cable(struct text_object *, char *, int);
void print_apcupsd_status(struct text_object *, char *, int);
void print_apcupsd_linev(struct text_object *, char *, int);
void print_apcupsd_load(struct text_object *, char *, int);
void print_apcupsd_charge(struct text_object *, char *, int);
void print_apcupsd_timeleft(struct text_object *, char *, int);
void print_apcupsd_temp(struct text_object *, char *, int);
void print_apcupsd_lastxfer(struct text_object *, char *, int);

#ifdef __cplusplus
}
#endif

#endif /*APCUPSD_H_*/
