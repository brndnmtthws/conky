/* -------------------------------------------------------------------------
 * infopipe.h: conky support for infopipe plugin
 * 
 * Copyright (C) 2005  Philip Kovacs kovacsp3@comcast.net
 *
 * $Id $
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

#ifndef INFOPIPE_H
#define INFOPIPE_H

enum _infopipe_items {
	INFOPIPE_STATUS=0,
	INFOPIPE_TITLE,
	INFOPIPE_LENGTH,
	INFOPIPE_LENGTH_SECONDS,
	INFOPIPE_POSITION,
	INFOPIPE_POSITION_SECONDS,
	INFOPIPE_BITRATE,
	INFOPIPE_FREQUENCY,
	INFOPIPE_CHANNELS,
	INFOPIPE_FILENAME,
	INFOPIPE_PLAYLIST_LENGTH,
	INFOPIPE_PLAYLIST_POSITION,
};

/* 12 slots for the infopipe values */
typedef char infopipe_t[12][128];

/* create a worker thread for infopipe media player status */
int create_infopipe_thread(void);

/* destroy infopipe media player worker thread */
int destroy_infopipe_thread(void);

/* Service routine for the conky main thread */
void update_infopipe(void);

/* Thread functions */
void *infopipe_thread_func(void *);

#endif
