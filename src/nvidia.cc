/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
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


/*
 * 
 * Author:
 * Fonic <fonic.maxxim@live.com>
 * 
 * TODO:
 * - Add third argument to module to allow querying multiple GPUs/fans etc.,
 *   e.g. ${nvidia gputemp 2}, ${nvidia fanlevel 1}
 * - Implement static flag; static values are only queried once to save CPU time,
 *   e.g. min/max values, temp threshold etc.
 * - Move decoding of GPU/MEM freqs to print_nvidia_value() using QUERY_SPECIAL
 *   so that all quirks are located there
 * - Implement nvs->print_type to allow control over how the value is printed
 *   (int, float, temperature...)
 * - Rename set_nvidia_type() to set_nvidia_query (requires changes in core.cc)
 * 
 * Showcase (conky.conf):
 * --==| NVIDIA | ==--
 * GPU   ${nvidia gpufreq}MHz (${nvidia gpufreqmin}-${nvidia gpufreqmax}MHz)
 * MEM   ${nvidia memfreq}MHz (${nvidia memfreqmin}-${nvidia memfreqmax}MHz)
 * MTR   ${nvidia mtrfreq}MHz (${nvidia mtrfreqmin}-${nvidia mtrfreqmax}MHz)
 * PERF  Level ${nvidia perflevel} (${nvidia perflevelmin}-${nvidia perflevelmax}), Mode: ${nvidia perfmode}
 * VRAM  ${nvidia memutil}% (${nvidia memused}MB/${nvidia memtotal}MB)
 * LOAD  GPU ${nvidia gpuutil}%, RAM ${nvidia membwutil}%, VIDEO ${nvidia videoutil}%, PCIe ${nvidia pcieutil}%
 * TEMP  GPU ${nvidia gputemp}°C (${nvidia gputempthreshold}°C max.), SYS ${nvidia ambienttemp}°C
 * FAN   ${nvidia fanspeed}% RPM (${nvidia fanlevel}%)
 * 
 * --==| NVIDIA Bars |==--
 * LOAD ${nvidiabar gpuutil}
 * VRAM ${nvidiabar memutil}
 * RAM ${nvidiabar membwutil}
 * VIDEO ${nvidiabar videoutil}
 * PCIe ${nvidiabar pcieutil}
 * Fan ${nvidiabar fanlevel}
 * TEMP ${nvidiabar gputemp}
 * 
 */


#include "conky.h"
#include "logging.h"
#include "nvidia.h"
#include "temphelper.h"
#include "x11.h"
#include <NVCtrl/NVCtrlLib.h>


// Separators for nvidia string parsing
// (sample: "perf=0, nvclock=324, nvclockmin=324, nvclockmax=324 ; perf=1, nvclock=549, nvclockmin=549, nvclockmax=549")
#define NV_KVPAIR_SEPARATORS ", ;"
#define NV_KEYVAL_SEPARATORS "="


// Module arguments
const char* translate_module_argument[] = {
						"temp",			// Temperatures
						"gputemp",
						"threshold",
						"gputempthreshold",
						"ambient",
						"ambienttemp",
						
						"gpufreq",		// GPU frequency
						"gpufreqcur",
						"gpufreqmin",
						"gpufreqmax",
						
						"memfreq",		// Memory frequency
						"memfreqcur",
						"memfreqmin",
						"memfreqmax",
						
						"mtrfreq",		// Memory transfer rate frequency
						"mtrfreqcur",
						"mtrfreqmin",
						"mtrfreqmax",

						"perflevel",		// Performance levels
						"perflevelcur",
						"perflevelmin",
						"perflevelmax",
						"perfmode",

						"gpuutil",		// Load/utilization
						"membwutil",		// NOTE: this is the memory _bandwidth_ utilization, not the percentage of used/available memory!
						"videoutil",
						"pcieutil",
						
						"mem",			// RAM statistics
						"memused",
						"memfree",
						"memavail",
						"memmax",
						"memtotal",
						"memutil",
						"memperc",
						
						"fanspeed",		// Fan/cooler
						"fanlevel",
						
						"imagequality"		// Miscellaneous
};

// Enum for module arguments
typedef enum _ARG_ID {
	ARG_TEMP,
	ARG_GPU_TEMP,
	ARG_THRESHOLD,
	ARG_GPU_TEMP_THRESHOLD,
	ARG_AMBIENT,
	ARG_AMBIENT_TEMP,
	
	ARG_GPU_FREQ,
	ARG_GPU_FREQ_CUR,
	ARG_GPU_FREQ_MIN,
	ARG_GPU_FREQ_MAX,
	
	ARG_MEM_FREQ,
	ARG_MEM_FREQ_CUR,
	ARG_MEM_FREQ_MIN,
	ARG_MEM_FREQ_MAX,

	ARG_MTR_FREQ,
	ARG_MTR_FREQ_CUR,
	ARG_MTR_FREQ_MIN,
	ARG_MTR_FREQ_MAX,

	ARG_PERF_LEVEL,
	ARG_PERF_LEVEL_CUR,
	ARG_PERF_LEVEL_MIN,
	ARG_PERF_LEVEL_MAX,
	ARG_PERF_MODE,

	ARG_GPU_UTIL,
	ARG_MEM_BW_UTIL,
	ARG_VIDEO_UTIL,
	ARG_PCIE_UTIL,
	
	ARG_MEM,
	ARG_MEM_USED,
	ARG_MEM_FREE,
	ARG_MEM_AVAIL,
	ARG_MEM_MAX,
	ARG_MEM_TOTAL,
	ARG_MEM_UTIL,
	ARG_MEM_PERC,

	ARG_FAN_SPEED,
	ARG_FAN_LEVEL,
	
	ARG_IMAGEQUALITY,

	ARG_UNKNOWN
} ARG_ID;


// Nvidia query targets
const int translate_nvidia_target[] = {
					NV_CTRL_TARGET_TYPE_X_SCREEN,
					NV_CTRL_TARGET_TYPE_GPU,
					NV_CTRL_TARGET_TYPE_FRAMELOCK,
					NV_CTRL_TARGET_TYPE_VCSC,
					NV_CTRL_TARGET_TYPE_GVI,
					NV_CTRL_TARGET_TYPE_COOLER,
					NV_CTRL_TARGET_TYPE_THERMAL_SENSOR,
					NV_CTRL_TARGET_TYPE_3D_VISION_PRO_TRANSCEIVER,
					NV_CTRL_TARGET_TYPE_DISPLAY,
};

// Enum for nvidia query targets
typedef enum _TARGET_ID {
	TARGET_SCREEN,
	TARGET_GPU,
	TARGET_FRAMELOCK,
	TARGET_VCSC,
	TARGET_GVI,
	TARGET_COOLER,
	TARGET_THERMAL,
	TARGET_3DVISION,
	TARGET_DISPLAY
} TARGET_ID;


// Nvidia query attributes
const int translate_nvidia_attribute[] = {
					NV_CTRL_GPU_CORE_TEMPERATURE,
					NV_CTRL_GPU_CORE_THRESHOLD,
					NV_CTRL_AMBIENT_TEMPERATURE,
					
					NV_CTRL_GPU_CURRENT_CLOCK_FREQS,
					NV_CTRL_GPU_CURRENT_CLOCK_FREQS,
					NV_CTRL_STRING_PERFORMANCE_MODES,
					NV_CTRL_STRING_GPU_CURRENT_CLOCK_FREQS,
					NV_CTRL_GPU_POWER_MIZER_MODE,

					NV_CTRL_STRING_GPU_UTILIZATION,

					NV_CTRL_USED_DEDICATED_GPU_MEMORY,
					0,
					NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY,		// NOTE: NV_CTRL_TOTAL_GPU_MEMORY would be better, but returns KB instead of MB
					0,
					
					NV_CTRL_THERMAL_COOLER_SPEED,
					NV_CTRL_THERMAL_COOLER_LEVEL,

					NV_CTRL_GPU_CURRENT_PERFORMANCE_LEVEL,
					NV_CTRL_IMAGE_SETTINGS,
};

// Enum for nvidia query attributes
typedef enum _ATTR_ID {
	ATTR_GPU_TEMP,
	ATTR_GPU_TEMP_THRESHOLD,
	ATTR_AMBIENT_TEMP,
	
	ATTR_GPU_FREQ,
	ATTR_MEM_FREQ,
	ATTR_PERFMODES_STRING,
	ATTR_FREQS_STRING,
	ATTR_PERF_MODE,
	
	ATTR_UTILS_STRING,

	ATTR_MEM_USED,
	ATTR_MEM_FREE,
	ATTR_MEM_TOTAL,
	ATTR_MEM_UTIL,

	ATTR_FAN_SPEED,
	ATTR_FAN_LEVEL,

	ATTR_PERF_LEVEL,
	ATTR_IMAGE_QUALITY,
} ATTR_ID;


// Enum for query type
typedef enum _QUERY_ID {
	QUERY_VALUE,
	QUERY_STRING,
	QUERY_STRING_VALUE,
	QUERY_SPECIAL
} QUERY_ID;


// Enum for string token search mode
typedef enum _SEARCH_ID {
	SEARCH_FIRST,
	SEARCH_LAST,
	SEARCH_MIN,
	SEARCH_MAX
} SEARCH_ID;


// Global struct to keep track of queries
struct nvidia_s {
	QUERY_ID query;
	TARGET_ID target;
	ATTR_ID attribute;
	char *token;
	SEARCH_ID search;
};

static Display *nvdisplay;


namespace {
	class nvidia_display_setting: public conky::simple_config_setting<std::string> {
		typedef conky::simple_config_setting<std::string> Base;
	
	protected:
		virtual void lua_setter(lua::state &l, bool init);
		virtual void cleanup(lua::state &l);

	public:
		nvidia_display_setting()
			: Base("nvidia_display", std::string(), false)
		{}
	};

	void nvidia_display_setting::lua_setter(lua::state &l, bool init)
	{
		lua::stack_sentry s(l, -2);

		Base::lua_setter(l, init);

		std::string str = do_convert(l, -1).first;
		if(str.size()) {
			if ((nvdisplay = XOpenDisplay(str.c_str())) == NULL) {
				CRIT_ERR(NULL, NULL, "can't open nvidia display: %s", XDisplayName(str.c_str()));
			}
		}	

		++s;
	}

	void nvidia_display_setting::cleanup(lua::state &l)
	{
		lua::stack_sentry s(l, -1);

		if(nvdisplay) {
			XCloseDisplay(nvdisplay);
			nvdisplay = NULL;
		}

		l.pop();
	}

	nvidia_display_setting nvidia_display;
}

// Extract arguments for nvidiabar, etc, and run set_nvidia_type
int scan_nvidia_args (struct text_object *obj, const char *args, unsigned int special_t) {
	const char *arg = args;

	switch (special_t) {
		case BAR:
			arg = scan_bar(obj, arg, 100);
			break;
		case GRAPH:
			arg = scan_graph(obj, arg, 100);
			break;
		case GAUGE:
			arg = scan_gauge(obj, arg, 100);
			break;
		default:
			return 1;
	}

	// Return error if no argument
	// (sometimes scan_graph gets excited and eats the whole string!
	if (!arg) return 1;

	return set_nvidia_type(obj, arg);
}


// Evaluate module parameters and prepare query
int set_nvidia_type(struct text_object *obj, const char *arg)
{
	struct nvidia_s *nvs;
	int aid;

	// Initialize global struct
	obj->data.opaque = malloc(sizeof(struct nvidia_s));
	nvs = static_cast<nvidia_s *>(obj->data.opaque);
	memset(nvs, 0, sizeof(struct nvidia_s));
	
	// Translate parameter to id
	for (aid=0; aid < ARG_UNKNOWN; aid++) {
		if (strcmp(arg, translate_module_argument[aid]) == 0)
			break;
	}
	//fprintf(stderr, "parameter: %s -> aid: %d\n", arg, aid);
	
	// Evaluate parameter
	switch(aid) {
		
		case ARG_TEMP:						// GPU temperature
		case ARG_GPU_TEMP:
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_GPU_TEMP;
			break;
		case ARG_THRESHOLD:					// GPU temperature threshold
		case ARG_GPU_TEMP_THRESHOLD:
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_GPU_TEMP_THRESHOLD;
			break;
		case ARG_AMBIENT:					// Ambient temperature
		case ARG_AMBIENT_TEMP:
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_AMBIENT_TEMP;
			break;
			
		case ARG_GPU_FREQ:					// Current GPU clock
		case ARG_GPU_FREQ_CUR:
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_GPU_FREQ;
			break;
		case ARG_GPU_FREQ_MIN:					// Minimum GPU clock
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_PERFMODES_STRING;
			nvs->token = (char*) "nvclockmin";
			nvs->search = SEARCH_MIN;
			break;
		case ARG_GPU_FREQ_MAX:					// Maximum GPU clock
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_PERFMODES_STRING;
			nvs->token = (char*) "nvclockmax";
			nvs->search = SEARCH_MAX;
			break;
			
		case ARG_MEM_FREQ:					// Current memory clock
		case ARG_MEM_FREQ_CUR:
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_MEM_FREQ;
			break;
		case ARG_MEM_FREQ_MIN:					// Minimum memory clock
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_PERFMODES_STRING;
			nvs->token = (char*) "memclockmin";
			nvs->search = SEARCH_MIN;
			break;
		case ARG_MEM_FREQ_MAX:					// Maximum memory clock
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_PERFMODES_STRING;
			nvs->token = (char*) "memclockmax";
			nvs->search = SEARCH_MAX;
			break;

		case ARG_MTR_FREQ:					// Current memory transfer rate clock
		case ARG_MTR_FREQ_CUR:
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_FREQS_STRING;
			nvs->token = (char*) "memTransferRate";
			nvs->search = SEARCH_FIRST;
			break;
		case ARG_MTR_FREQ_MIN:					// Minimum memory transfer rate clock
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_PERFMODES_STRING;
			nvs->token = (char*) "memTransferRatemin";
			nvs->search = SEARCH_MIN;
			break;
		case ARG_MTR_FREQ_MAX:					// Maximum memory transfer rate clock
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_PERFMODES_STRING;
			nvs->token = (char*) "memTransferRatemax";
			nvs->search = SEARCH_MAX;
			break;

		case ARG_PERF_LEVEL:					// Current performance level
		case ARG_PERF_LEVEL_CUR:
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_PERF_LEVEL;
			break;
		case ARG_PERF_LEVEL_MIN:				// Lowest performance level
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_PERFMODES_STRING;
			nvs->token = (char*) "perf";
			nvs->search = SEARCH_MIN;
			break;
		case ARG_PERF_LEVEL_MAX:				// Highest performance level
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_PERFMODES_STRING;
			nvs->token = (char*) "perf";
			nvs->search = SEARCH_MAX;
			break;
		case ARG_PERF_MODE:					// Performance mode
			nvs->query = QUERY_SPECIAL;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_PERF_MODE;
			break;
			
		case ARG_GPU_UTIL:					// GPU utilization %
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_UTILS_STRING;
			nvs->token = (char*) "graphics";
			nvs->search = SEARCH_FIRST;
			break;
		case ARG_MEM_BW_UTIL:					// Memory bandwidth utilization %
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_UTILS_STRING;
			nvs->token = (char*) "memory";
			nvs->search = SEARCH_FIRST;
			break;
		case ARG_VIDEO_UTIL:					// Video engine utilization %
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_UTILS_STRING;
			nvs->token = (char*) "video";
			nvs->search = SEARCH_FIRST;
			break;
		case ARG_PCIE_UTIL:					// PCIe bandwidth utilization %
			nvs->query = QUERY_STRING_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_UTILS_STRING;
			nvs->token = (char*) "PCIe";
			nvs->search = SEARCH_FIRST;
			break;
			
		case ARG_MEM:						// Amount of used memory
		case ARG_MEM_USED:
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_MEM_USED;
			break;
		case ARG_MEM_FREE:					// Amount of free memory
		case ARG_MEM_AVAIL:
			nvs->query = QUERY_SPECIAL;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_MEM_FREE;
			break;
		case ARG_MEM_MAX:					// Total amount of memory
		case ARG_MEM_TOTAL:
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_MEM_TOTAL;
			break;
		case ARG_MEM_UTIL:					// Memory utilization %
		case ARG_MEM_PERC:
			nvs->query = QUERY_SPECIAL;
			nvs->target = TARGET_GPU;
			nvs->attribute = ATTR_MEM_UTIL;
			break;
			
		case ARG_FAN_SPEED:					// Fan speed
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_COOLER;
			nvs->attribute = ATTR_FAN_SPEED;
			break;
		case ARG_FAN_LEVEL:					// Fan level %
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_COOLER;
			nvs->attribute = ATTR_FAN_LEVEL;
			break;
		
		case ARG_IMAGEQUALITY:					// Image quality
			nvs->query = QUERY_VALUE;
			nvs->target = TARGET_SCREEN;
			nvs->attribute = ATTR_IMAGE_QUALITY;
			break;
			
		default:						// Unknown/invalid argument
			return 1;
	}
	return 0;
}


// Retrieve attribute value via nvidia interface
static int get_nvidia_value(TARGET_ID tid, ATTR_ID aid)
{
	Display *dpy = nvdisplay ? nvdisplay : display;
	int value;

	// Query nvidia interface
	if(!dpy || !XNVCTRLQueryTargetAttribute(dpy, translate_nvidia_target[tid], 0, 0, translate_nvidia_attribute[aid], &value)){
		return -1;
	}
	
	// Unpack clock values (see NVCtrl.h for details)
	if (aid == ATTR_GPU_FREQ)
		return value >> 16;
	if (aid == ATTR_MEM_FREQ)
		return value & 0xFFFF;
	
	// Return value
	return value;
}


// Retrieve attribute string via nvidia interface
static char* get_nvidia_string(TARGET_ID tid, ATTR_ID aid)
{
	Display *dpy = nvdisplay ? nvdisplay : display;
	char *str;
	
	// Query nvidia interface
	if (!dpy || !XNVCTRLQueryTargetStringAttribute(dpy, translate_nvidia_target[tid], 0, 0, translate_nvidia_attribute[aid], &str)) {
		return NULL;
	}
	//fprintf(stderr, "%s", str);
	return str;
}


// Retrieve token value from nvidia string
static int get_nvidia_string_value(TARGET_ID tid, ATTR_ID aid, char *token, SEARCH_ID search)
{
	char *str;
	char *kvp, *key, *val;
	char *saveptr1, *saveptr2;
	int value, temp;
	
	// Get string via nvidia interface
	str = get_nvidia_string(tid, aid);

	// Split string into 'key=value' substrings, split substring
	// into key and value, from value, check if token was found,
	// convert value to int, evaluate value according to specified
	// token search mode
	value = -1;
	kvp = strtok_r(str, NV_KVPAIR_SEPARATORS, &saveptr1);
	while (kvp) {
		key = strtok_r(kvp, NV_KEYVAL_SEPARATORS, &saveptr2);
		val = strtok_r(NULL, NV_KEYVAL_SEPARATORS, &saveptr2);
		if (key && val && (strcmp(token, key) == 0)) {
			temp = (int) strtol(val, NULL, 0);
			if (search == SEARCH_FIRST) {
				value = temp;
				break;
			} else if (search == SEARCH_LAST) {
				value = temp;
			} else if (search == SEARCH_MIN) {
				if ((value == -1) || (temp < value))
					value = temp;
			} else if (search == SEARCH_MAX) {
				if (temp > value)
					value = temp;
			} else {
				value = -1;
				break;
			}
		}
		kvp = strtok_r(NULL, NV_KVPAIR_SEPARATORS, &saveptr1);
	}
	
	// TESTING - print raw string if token was not found;
	// string has to be queried again due to strtok_r()
	/*if (value == -1) {
		free(str);
		str = get_nvidia_string(tid, aid);
		fprintf(stderr, "%s", str);
	}*/
	
	// Free string, return value
	free(str);
	return value;
}


// Perform query and print result
void print_nvidia_value(struct text_object *obj, char *p, int p_max_size)
{
	struct nvidia_s *nvs = static_cast<nvidia_s *>(obj->data.opaque);
	int value, temp1, temp2;
	char* str;

	// Assume failure
	value = -1;
	str = NULL;

	// Perform query
	if (nvs != NULL) {
		switch (nvs->query) {
			case QUERY_VALUE:
				value = get_nvidia_value(nvs->target, nvs->attribute);
				break;
			case QUERY_STRING:
				str = get_nvidia_string(nvs->target, nvs->attribute);
				break;
			case QUERY_STRING_VALUE:
				value = get_nvidia_string_value(nvs->target, nvs->attribute, nvs->token, nvs->search);
				break;
			case QUERY_SPECIAL:
				switch (nvs->attribute) {
					case ATTR_PERF_MODE:
						temp1 = get_nvidia_value(nvs->target, nvs->attribute);
						switch (temp1) {
							case NV_CTRL_GPU_POWER_MIZER_MODE_ADAPTIVE:
								temp2 = asprintf(&str, "Adaptive");
								break;
							case NV_CTRL_GPU_POWER_MIZER_MODE_PREFER_MAXIMUM_PERFORMANCE:
								temp2 = asprintf(&str, "Max. Perf.");
								break;
							case NV_CTRL_GPU_POWER_MIZER_MODE_AUTO:
								temp2 = asprintf(&str, "Auto");
								break;
							case NV_CTRL_GPU_POWER_MIZER_MODE_PREFER_CONSISTENT_PERFORMANCE:
								temp2 = asprintf(&str, "Consistent");
								break;
							default:
								temp2 = asprintf(&str, "Unknown (%d)", value);
						}
						break;
					case ATTR_MEM_FREE:
						temp1 = get_nvidia_value(nvs->target, ATTR_MEM_USED);
						temp2 = get_nvidia_value(nvs->target, ATTR_MEM_TOTAL);
						value = temp2 - temp1;
						break;
					case ATTR_MEM_UTIL:
						temp1 = get_nvidia_value(nvs->target, ATTR_MEM_USED);
						temp2 = get_nvidia_value(nvs->target, ATTR_MEM_TOTAL);
						value = ((float)temp1 * 100 / (float)temp2) + 0.5;
						break;
				}
				break;
		}
	}
	
	// Print result
	if (value != -1) {
		snprintf(p, p_max_size, "%d", value);
	} else if (str != NULL) {
		snprintf(p, p_max_size, "%s", str);
		free(str);
	} else {
		snprintf(p, p_max_size, "N/A");
	}
	
}

double get_nvidia_barval(struct text_object *obj) {
	struct nvidia_s *nvs = static_cast<nvidia_s *>(obj->data.opaque);
	int temp1, temp2;
	double value;
	
	// Assume failure
	value = 0;
	
	// Convert query_result to a percentage
	if (nvs != NULL) {
		switch (nvs->attribute) {
			case ATTR_UTILS_STRING: // one of the percentage utils (gpuutil, membwutil, videoutil and pcieutil)
				value = get_nvidia_string_value(nvs->target, ATTR_UTILS_STRING, nvs->token, nvs->search);
				break;
			case ATTR_MEM_UTIL: // memutil
				temp1 = get_nvidia_value(nvs->target, ATTR_MEM_USED);
				temp2 = get_nvidia_value(nvs->target, ATTR_MEM_TOTAL);
				value = ((float)temp1 * 100 / (float)temp2) + 0.5;
				break;
			case ATTR_FAN_LEVEL: // fanlevel
			case ATTR_FAN_SPEED: // TODO warn user to use fanlevel if they use fanspeed
				value = get_nvidia_value(nvs->target, ATTR_FAN_LEVEL);
				break;
			case ATTR_GPU_TEMP: // gputemp (calculate out of gputempthreshold)
				temp1 = get_nvidia_value(nvs->target, ATTR_GPU_TEMP);
				temp2 = get_nvidia_value(nvs->target, ATTR_GPU_TEMP_THRESHOLD);
				value = ((float)temp1 * 100 / (float)temp2) + 0.5;
				break;
			// TODO: calculate gpufreq, memfreq, etc
			// can use (val-min)÷(max-min)×100. Perhaps a helper function or macro
			
			// TODO: throw errors if unsupported args are used
		}
	}
	
	// Return the percentage
	return value;
}


// Cleanup
void free_nvidia(struct text_object *obj)
{
	free_and_zero(obj->data.opaque);
}
