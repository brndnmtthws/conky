/* -------------------------------------------------------------------------
 * xmms.h:  conky support for XMMS-related projects
 * 
 * Copyright (C) 2005  Philip Kovacs kovacsp3@comcast.net
 *
 * $Id$
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * --------------------------------------------------------------------------- */

#ifndef XMMS_H
#define XMMS_H

enum _xmms_items {
	XMMS_STATUS=0,
	XMMS_TITLE,
	XMMS_LENGTH,
	XMMS_LENGTH_SECONDS,
	XMMS_POSITION,
	XMMS_POSITION_SECONDS,
	XMMS_BITRATE,
	XMMS_FREQUENCY,
	XMMS_CHANNELS,
	XMMS_FILENAME,
	XMMS_PLAYLIST_LENGTH,
	XMMS_PLAYLIST_POSITION,
};

enum _xmms_projects {
	PROJECT_NONE = 0,
	PROJECT_XMMS = 1,
	PROJECT_BMP = 2,
	PROJECT_AUDACIOUS = 3,
	PROJECT_INFOPIPE = 4
};

#define SET_XMMS_PROJECT_AVAILABLE(mask,project) 	(mask |= (1<<project))
#define TEST_XMMS_PROJECT_AVAILABLE(mask,project)	(mask & (1<<project))

/* 12 slots for the xmms values */
typedef char xmms_t[12][128];

/* create a worker thread for xmms media player status */
int create_xmms_thread(void);

/* destroy xmms media player worker thread */
int destroy_xmms_thread(void);

/* Service routine for the conky main thread */
void update_xmms(void);

/* Thread functions */
void *xmms_thread_func_dynamic(void *);
void *xmms_thread_func_infopipe(void *);

#endif
