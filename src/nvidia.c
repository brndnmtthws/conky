/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2008 Markus Meissner
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

#include "conky.h"
#include "logging.h"
#include "nvidia.h"
#include "temphelper.h"
#include "x11.h"
#include <NVCtrl/NVCtrlLib.h>

const int nvidia_query_to_attr[] = {NV_CTRL_GPU_CORE_TEMPERATURE,
				    NV_CTRL_GPU_CORE_THRESHOLD,
				    NV_CTRL_AMBIENT_TEMPERATURE,
				    NV_CTRL_GPU_CURRENT_CLOCK_FREQS,
				    NV_CTRL_GPU_CURRENT_CLOCK_FREQS,
				    NV_CTRL_IMAGE_SETTINGS};

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

static Display *nvdisplay;

static int get_nvidia_value(QUERY_ID qid){
	int tmp;
	Display *dpy = nvdisplay ? nvdisplay : display;
	if(!dpy || !XNVCTRLQueryAttribute(dpy, 0, 0, nvidia_query_to_attr[qid], &tmp)){
		return -1;
	}
	/* FIXME: when are the low 2 bytes of NV_GPU_FREQ needed? */
	if (qid == NV_GPU_FREQ)
		return tmp >> 16;
	if (qid == NV_MEM_FREQ)
		return tmp & 0xFFFF;
	return tmp;
}

int set_nvidia_type(struct text_object *obj, const char *arg)
{
	struct nvidia_s *nvs;

	nvs = obj->data.opaque = malloc(sizeof(struct nvidia_s));
	memset(nvs, 0, sizeof(struct nvidia_s));

	switch(arg[0]) {
		case 't':                              // temp or threshold
			nvs->print_as_float = 1;
			if (arg[1] == 'e')
				nvs->type = NV_TEMP;
			else if (arg[1] == 'h')
				nvs->type = NV_TEMP_THRESHOLD;
			else
				return 1;
			break;
		case 'a':                              // ambient temp
			nvs->print_as_float = 1;
			nvs->type = NV_TEMP_AMBIENT;
			break;
		case 'g':                              // gpufreq
			nvs->type = NV_GPU_FREQ;
			break;
		case 'm':                              // memfreq
			nvs->type = NV_MEM_FREQ;
			break;
		case 'i':                              // imagequality
			nvs->type = NV_IMAGE_QUALITY;
			break;
		default:
			return 1;
	}
	return 0;
}

void print_nvidia_value(struct text_object *obj, char *p, int p_max_size)
{
	int value;
	struct nvidia_s *nvs = obj->data.opaque;

	if (!nvs ||
	    (value = get_nvidia_value(nvs->type)) == -1) {
		snprintf(p, p_max_size, "N/A");
		return;
	}
	if (nvs->type == NV_TEMP)
		temp_print(p, p_max_size, (double)value, TEMP_CELSIUS);
	else if (nvs->print_as_float &&
			value > 0 && value < 100)
		snprintf(p, p_max_size, "%.1f", (float)value);
	else
		snprintf(p, p_max_size, "%d", value);
}

void free_nvidia(struct text_object *obj)
{
	if (obj->data.opaque) {
		free(obj->data.opaque);
		obj->data.opaque = NULL;
	}
}

void set_nvidia_display(const char *disp)
{
	if(nvdisplay) {
		XCloseDisplay(nvdisplay);
		nvdisplay = NULL;
	}
	if(disp) {
		if ((nvdisplay = XOpenDisplay(disp)) == NULL) {
			CRIT_ERR(NULL, NULL, "can't open nvidia display: %s", XDisplayName(disp));
		}
	}
}

