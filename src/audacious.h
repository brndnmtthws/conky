/* audacious.h:  conky support for audacious music player
 *
 * Copyright (C) 2005-2007 Philip Kovacs pkovacs@users.sourceforge.net
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
 * USA. */

#ifndef AUDACIOUS_H
#define AUDACIOUS_H

#include "timed_thread.h"

enum _audacious_items {
	AUDACIOUS_STATUS = 0,
	AUDACIOUS_TITLE,
	AUDACIOUS_LENGTH,
	AUDACIOUS_LENGTH_SECONDS,
	AUDACIOUS_POSITION,
	AUDACIOUS_POSITION_SECONDS,
	AUDACIOUS_BITRATE,
	AUDACIOUS_FREQUENCY,
	AUDACIOUS_CHANNELS,
	AUDACIOUS_FILENAME,
	AUDACIOUS_PLAYLIST_LENGTH,
	AUDACIOUS_PLAYLIST_POSITION,
	AUDACIOUS_MAIN_VOLUME,
};

/* 12 slots for the audacious values */
typedef char audacious_t[13][128];

/* type for data exchange with main thread */
typedef struct audacious_s {
  audacious_t items;  /* e.g. items[AUDACIOUS_STATUS] */
  int max_title_len;  /* e.g. ${audacious_title 50} */
  timed_thread *p_timed_thread;
} AUDACIOUS_S;

/* create a worker thread for audacious media player status */
int create_audacious_thread(void);

/* destroy audacious media player worker thread */
int destroy_audacious_thread(void);

/* Service routine for the conky main thread */
void update_audacious(void);

/* Thread functions */
void *audacious_thread_func(void *);

#endif
