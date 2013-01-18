/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef JACK_H_
#define JACK_H_

#include <jack/jack.h>

enum {	JACK_IS_ACTIVE =	0x01,
		JACK_IS_ROLLING = 	0x02,
		JACK_IS_BBT = 		0x04	};

struct jack_s {
	int				state; /* |'d JACK_IS_* */
	float			cpu_load;
	jack_nframes_t	buffer_size;
	jack_nframes_t	sample_rate;
	int				xruns;
	/* transport data: */
	int32_t			frame;
	int				hour;
	int				min;
	int				sec;
	float			beat_type;
	float			beats_per_bar;
	double			bpm;
	int32_t			bar;
	int32_t			beat;
	int32_t			tick;
};

void	init_jack(void);
int		update_jack(void);
void	jack_close(void);

#endif /*JACK_H_*/
