/* -------------------------------------------------------------------------
 * audacious.h:  conky support for Audacious audio player
 * 
 * http://audacious-media-player.org
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

#ifndef AUDACIOUS_H
#define AUDACIOUS_H

/* 11 keys comprise the audacious information. */
enum _audacious_keys {
	AUDACIOUS_STATUS,
	AUDACIOUS_SONG,
	AUDACIOUS_SONG_LENGTH,
	AUDACIOUS_SONG_LENGTH_SECONDS,
	AUDACIOUS_SONG_LENGTH_FRAMES,
	AUDACIOUS_SONG_OUTPUT_LENGTH,
	AUDACIOUS_SONG_OUTPUT_LENGTH_SECONDS,
	AUDACIOUS_SONG_OUTPUT_LENGTH_FRAMES,
	AUDACIOUS_SONG_BITRATE,
	AUDACIOUS_SONG_FREQUENCY,
	AUDACIOUS_SONG_CHANNELS
};
		
/* 11 slots for the audacious values */
typedef char audacious_t[11][128];

/* Service routine for the conky main thread */
void update_audacious(void);

/* Thread function */
void *audacious_thread_func(void *);

#endif
