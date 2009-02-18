/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2008 Markus Meissner
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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


#ifndef NVIDIA_CONKY_H
#define NVIDIA_CONKY_H

#include <X11/Xlib.h>
#include <NVCtrl/NVCtrlLib.h>

typedef enum _QUERY_ID {
	NV_TEMP,
	NV_TEMP_THRESHOLD,
	NV_TEMP_AMBIENT,
	NV_GPU_FREQ,
	NV_MEM_FREQ,
	NV_IMAGE_QUALITY
} QUERY_ID;

struct nvidia_s {
	int interval;
	int print_as_float;
	QUERY_ID type;
};

int get_nvidia_value(QUERY_ID qid, Display *dpy);
int set_nvidia_type(struct nvidia_s *, const char *);

#endif
