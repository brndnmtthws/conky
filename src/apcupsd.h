/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
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
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 */

#ifndef APCUPSD_H_
#define APCUPSD_H_

enum _apcupsd_items {
	APCUPSD_NAME,
	APCUPSD_MODEL,
	APCUPSD_UPSMODE,
	APCUPSD_CABLE,
	APCUPSD_STATUS,
	APCUPSD_LINEV,
	APCUPSD_LOAD,
	APCUPSD_CHARGE,
	APCUPSD_TIMELEFT,
	APCUPSD_TEMP,
	APCUPSD_LASTXFER,
	_APCUPSD_COUNT,
};

/* type for data exchange with main thread */
#define APCUPSD_MAXSTR 32
typedef struct apcupsd_s {
	char items[_APCUPSD_COUNT][APCUPSD_MAXSTR+1];	/* e.g. items[APCUPSD_STATUS] */
	char host[64];
	int  port;
} APCUPSD_S, *PAPCUPSD_S;

/* Service routine for the conky main thread */
void update_apcupsd(void);

/* fill in the default values */
void init_apcupsd(void);

#endif /*APCUPSD_H_*/
