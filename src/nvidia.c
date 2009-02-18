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

#include "nvidia.h"

const int nvidia_query_to_attr[] = {NV_CTRL_GPU_CORE_TEMPERATURE,
				    NV_CTRL_GPU_CORE_THRESHOLD,
				    NV_CTRL_AMBIENT_TEMPERATURE,
				    NV_CTRL_GPU_CURRENT_CLOCK_FREQS,
				    NV_CTRL_GPU_CURRENT_CLOCK_FREQS,
				    NV_CTRL_IMAGE_SETTINGS};

int get_nvidia_value(QUERY_ID qid, Display *dpy){
	int tmp;
	if(!XNVCTRLQueryAttribute(dpy, 0, 0, nvidia_query_to_attr[qid], &tmp)){
		return -1;
	}
	/* FIXME: when are the low 2 bytes of NV_GPU_FREQ needed? */
	if (qid == NV_GPU_FREQ)
		return tmp >> 16;
	if (qid == NV_MEM_FREQ)
		return tmp & 0xFFFF;
	return tmp;
}

int set_nvidia_type(struct nvidia_s *nvidia, const char *arg)
{
	if (!arg || !arg[0] || !arg[1])
		return 1;

	nvidia->print_as_float = 0;
	switch(arg[0]) {
		case 't':                              // temp or threshold
			nvidia->print_as_float = 1;
			if (arg[1] == 'e')
				nvidia->type = NV_TEMP;
			else if (arg[1] == 'h')
				nvidia->type = NV_TEMP_THRESHOLD;
			else
				return 1;
			break;
		case 'a':                              // ambient temp
			nvidia->print_as_float = 1;
			nvidia->type = NV_TEMP_AMBIENT;
			break;
		case 'g':                              // gpufreq
			nvidia->type = NV_GPU_FREQ;
			break;
		case 'm':                              // memfreq
			nvidia->type = NV_MEM_FREQ;
			break;
		case 'i':                              // imagequality
			nvidia->type = NV_IMAGE_QUALITY;
			break;
		default:
			return 1;
	}
	return 0;
}
