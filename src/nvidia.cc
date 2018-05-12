/*
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
 * - Move decoding of GPU/MEM freqs to print_nvidia_value() using QUERY_SPECIAL
 *   so that all quirks are located there
 * - Implement nvs->print_type to allow control over how the value is printed
 *   (int, float, temperature...)
 *
 * Showcase (conky.conf):
 * --==| NVIDIA | ==--
 * GPU    ${nvidia gpufreq [gpu_id]}MHz (${nvidia gpufreqmin [gpu_id]}-${nvidia
 * gpufreqmax [gpu_id]}MHz) MEM    ${nvidia memfreq [gpu_id]}MHz (${nvidia
 * memfreqmin [gpu_id]}-${nvidia memfreqmax [gpu_id]}MHz) MTR    ${nvidia
 * mtrfreq [gpu_id]}MHz (${nvidia mtrfreqmin [gpu_id]}-${nvidia mtrfreqmax
 * [gpu_id]}MHz) PERF   Level ${nvidia perflevel [gpu_id]} (${nvidia
 * perflevelmin [gpu_id]}-${nvidia perflevelmax [gpu_id]}), Mode: ${nvidia
 * perfmode [gpu_id]} VRAM   ${nvidia memutil [gpu_id]}% (${nvidia memused
 * [gpu_id]}MB/${nvidia memtotal [gpu_id]}MB) LOAD   GPU ${nvidia gpuutil
 * [gpu_id]}%, RAM ${nvidia membwutil [gpu_id]}%, VIDEO ${nvidia videoutil
 * [gpu_id]}%, PCIe ${nvidia pcieutil [gpu_id]}% TEMP   GPU ${nvidia gputemp
 * [gpu_id]}°C (${nvidia gputempthreshold [gpu_id]}°C max.), SYS ${nvidia
 * ambienttemp [gpu_id]}°C FAN    ${nvidia fanspeed [gpu_id]} RPM (${nvidia
 * fanlevel [gpu_id]}%)
 *
 * miscelaneas:
 * OPENGL ${nvidia imagequality [gpu_id]}
 * GPU    ${nvidia modelname [gpu_id]}
 *
 * --==| NVIDIA Bars |==--
 * LOAD  ${nvidiabar [height][,width] gpuutil [gpu_id]}
 * VRAM  ${nvidiabar [height][,width] memutil [gpu_id]}
 * RAM   ${nvidiabar [height][,width] membwutil [gpu_id]}
 * VIDEO ${nvidiabar [height][,width] videoutil [gpu_id]}
 * PCIe  ${nvidiabar [height][,width] pcieutil [gpu_id]}
 * Fan   ${nvidiabar [height][,width] fanlevel [gpu_id]}
 * TEMP  ${nvidiabar [height][,width] gputemp [gpu_id]}
 *
 * --==| NVIDIA Gauge |==--
 * LOAD  ${nvidiagauge [height][,width] gpuutil [gpu_id]}
 * VRAM  ${nvidiagauge [height][,width] memutil [gpu_id]}
 * RAM   ${nvidiagauge [height][,width] membwutil [gpu_id]}
 * VIDEO ${nvidiagauge [height][,width] videoutil [gpu_id]}
 * PCIe  ${nvidiagauge [height][,width] pcieutil [gpu_id]}
 * Fan   ${nvidiagauge [height][,width] fanlevel [gpu_id]}
 * TEMP  ${nvidiagauge [height][,width] gputemp [gpu_id]}
 *
 * --==| NVIDIA Graph |==-- (gpu_id is not optional in this case)
 * LOAD  ${nvidiagraph gpuutil [height][,width] [gradient color 1] [gradient
 * color 2] [scale] [-t] [-l] gpu_id} VRAM  ${nvidiagraph memutil
 * [height][,width] [gradient color 1] [gradient color 2] [scale] [-t] [-l]
 * gpu_id} RAM   ${nvidiagraph membwutil [height][,width] [gradient color 1]
 * [gradient color 2] [scale] [-t] [-l] gpu_id} VIDEO ${nvidiagraph videoutil
 * [height][,width] [gradient color 1] [gradient color 2] [scale] [-t] [-l]
 * gpu_id} PCIe  ${nvidiagraph pcieutil [height][,width] [gradient color 1]
 * [gradient color 2] [scale] [-t] [-l] gpu_id} Fan   ${nvidiagraph fanlevel
 * [height][,width] [gradient color 1] [gradient color 2] [scale] [-t] [-l]
 * gpu_id} TEMP  ${nvidiagraph gputemp [height][,width] [gradient color 1]
 * [gradient color 2] [scale] [-t] [-l] gpu_id}
 */

#include "nvidia.h"
#include <NVCtrl/NVCtrlLib.h>
#include "conky.h"
#include "logging.h"
#include "temphelper.h"
#include "x11.h"

// Separators for nvidia string parsing
// (sample: "perf=0, nvclock=324, nvclockmin=324, nvclockmax=324 ; perf=1,
// nvclock=549, nvclockmin=549, nvclockmax=549")
#define NV_KVPAIR_SEPARATORS ", ;"
#define NV_KEYVAL_SEPARATORS "="

// Module arguments
const char *translate_module_argument[] = {
    "temp",  // Temperatures
    "gputemp", "threshold", "gputempthreshold", "ambient", "ambienttemp",

    "gpufreq",  // GPU frequency
    "gpufreqcur", "gpufreqmin", "gpufreqmax",

    "memfreq",  // Memory frequency
    "memfreqcur", "memfreqmin", "memfreqmax",

    "mtrfreq",  // Memory transfer rate frequency
    "mtrfreqcur", "mtrfreqmin", "mtrfreqmax",

    "perflevel",  // Performance levels
    "perflevelcur", "perflevelmin", "perflevelmax", "perfmode",

    "gpuutil",    // Load/utilization
    "membwutil",  // NOTE: this is the memory _bandwidth_ utilization, not the
                  // percentage of used/available memory!
    "videoutil", "pcieutil",

    "mem",  // RAM statistics
    "memused", "memfree", "memavail", "memmax", "memtotal", "memutil",
    "memperc",

    "fanspeed",  // Fan/cooler
    "fanlevel",

    "imagequality",  // Miscellaneous
    "modelname"};

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

  ARG_MODEL_NAME,

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
    NV_CTRL_TOTAL_DEDICATED_GPU_MEMORY,  // NOTE: NV_CTRL_TOTAL_GPU_MEMORY would
                                         // be better, but returns KB instead of
                                         // MB
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

  ATTR_MODEL_NAME,

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

// Translate special_type into command string
const char *translate_nvidia_special_type[] = {
    "nvidia",       // NONSPECIAL
    "",             // HORIZONTAL_LINE
    "",             // STIPPLED_HR
    "nvidiabar",    // BAR
    "",             // FG
    "",             // BG
    "",             // OUTLINE
    "",             // ALIGNR
    "",             // ALIGNC
    "nvidiagague",  // GAUGE
    "nvidiagraph",  // GRAPH
    "",             // OFFSET
    "",             // VOFFSET
    "",             // FONT
    "",             // GOTO
    ""              // TAB
};

// Global struct to keep track of queries
struct nvidia_s {
  const char *command;
  const char *arg;
  QUERY_ID query;
  TARGET_ID target;
  ATTR_ID attribute;
  char *token;
  SEARCH_ID search;
  //  added new field for GPU id
  int gpu_id;
};

// Cache by value
struct nvidia_c_value {
  int memtotal = -1;
  int gputempthreshold = -1;
};

// Cache by string
struct nvidia_c_string {
  int nvclockmin = -1;
  int nvclockmax = -1;
  int memclockmin = -1;
  int memclockmax = -1;
  int memTransferRatemin = -1;
  int memTransferRatemax = -1;
  int perfmin = -1;
  int perfmax = -1;
};

static Display *nvdisplay;

// Maximum number of GPU connected:
// For cache default value: choosed a model of direct access to array instead of
// list for speed improvement value based on the incoming quad Naples tech
// having 256 PCIe lanes available
const int MAXNUMGPU = 64;

namespace {
class nvidia_display_setting
    : public conky::simple_config_setting<std::string> {
  typedef conky::simple_config_setting<std::string> Base;

 protected:
  virtual void lua_setter(lua::state &l, bool init);
  virtual void cleanup(lua::state &l);

 public:
  nvidia_display_setting() : Base("nvidia_display", std::string(), false) {}
};

void nvidia_display_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  std::string str = do_convert(l, -1).first;
  if (str.size()) {
    if ((nvdisplay = XOpenDisplay(str.c_str())) == NULL) {
      CRIT_ERR(NULL, NULL, "can't open nvidia display: %s",
               XDisplayName(str.c_str()));
    }
  }

  ++s;
}

void nvidia_display_setting::cleanup(lua::state &l) {
  lua::stack_sentry s(l, -1);

  if (nvdisplay) {
    XCloseDisplay(nvdisplay);
    nvdisplay = NULL;
  }

  l.pop();
}

nvidia_display_setting nvidia_display;
}  // namespace

// Evaluate module parameters and prepare query
int set_nvidia_query(struct text_object *obj, const char *arg,
                     unsigned int special_type) {
  struct nvidia_s *nvs;
  int aid;
  int ilen;

  // Initialize global struct
  obj->data.opaque = malloc(sizeof(struct nvidia_s));
  nvs = static_cast<nvidia_s *>(obj->data.opaque);
  memset(nvs, 0, sizeof(struct nvidia_s));

  // Added new parameter parsing GPU_ID as 0,1,2,..
  // if no GPU_ID parameter then default to 0
  nvs->gpu_id = 0;
  char *strbuf = strdup(arg);
  char *p = strrchr(strbuf, ' ');
  if (p && *(p + 1)) {
    nvs->gpu_id = atoi(p + 1);
    if ((nvs->gpu_id > 0) || !strcmp(p + 1, "0")) {
      ilen = strlen(strbuf);
      ilen = ilen - strlen(p);
      strbuf[ilen] = 0;
      arg = strbuf;
    }
  }

  // If the value is negative it is set to 0
  if (nvs->gpu_id < 0) nvs->gpu_id = 0;

  // Extract arguments for nvidiabar, etc, and run set_nvidia_query
  switch (special_type) {
    case BAR:
      arg = scan_bar(obj, arg, 100);
      break;
    case GRAPH:
      arg = scan_graph(obj, arg, 100);
      break;
    case GAUGE:
      arg = scan_gauge(obj, arg, 100);
      break;
  }

  // Return error if no argument
  // (sometimes scan_graph gets excited and eats the whole string!
  if (!arg) return 1;

  // Translate parameter to id
  for (aid = 0; aid < ARG_UNKNOWN; aid++) {
    if (strcmp(arg, translate_module_argument[aid]) == 0) break;
  }

  // free the string buffer after arg is not anymore needed
  if (strbuf != NULL) free(strbuf);

  // Save pointers to the arg and command strings for debugging and printing
  nvs->arg = translate_module_argument[aid];
  nvs->command = translate_nvidia_special_type[special_type];

  // Evaluate parameter
  switch (aid) {
    case ARG_TEMP:  // GPU temperature
    case ARG_GPU_TEMP:
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_GPU_TEMP;
      break;
    case ARG_THRESHOLD:  // GPU temperature threshold
    case ARG_GPU_TEMP_THRESHOLD:
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_GPU_TEMP_THRESHOLD;
      break;
    case ARG_AMBIENT:  // Ambient temperature
    case ARG_AMBIENT_TEMP:
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_AMBIENT_TEMP;
      break;

    case ARG_GPU_FREQ:  // Current GPU clock
    case ARG_GPU_FREQ_CUR:
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_GPU_FREQ;
      break;
    case ARG_GPU_FREQ_MIN:  // Minimum GPU clock
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_PERFMODES_STRING;
      nvs->token = (char *)"nvclockmin";
      nvs->search = SEARCH_MIN;
      break;
    case ARG_GPU_FREQ_MAX:  // Maximum GPU clock
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_PERFMODES_STRING;
      nvs->token = (char *)"nvclockmax";
      nvs->search = SEARCH_MAX;
      break;

    case ARG_MEM_FREQ:  // Current memory clock
    case ARG_MEM_FREQ_CUR:
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_MEM_FREQ;
      break;
    case ARG_MEM_FREQ_MIN:  // Minimum memory clock
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_PERFMODES_STRING;
      nvs->token = (char *)"memclockmin";
      nvs->search = SEARCH_MIN;
      break;
    case ARG_MEM_FREQ_MAX:  // Maximum memory clock
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_PERFMODES_STRING;
      nvs->token = (char *)"memclockmax";
      nvs->search = SEARCH_MAX;
      break;

    case ARG_MTR_FREQ:  // Current memory transfer rate clock
    case ARG_MTR_FREQ_CUR:
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_FREQS_STRING;
      nvs->token = (char *)"memTransferRate";
      nvs->search = SEARCH_FIRST;
      break;
    case ARG_MTR_FREQ_MIN:  // Minimum memory transfer rate clock
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_PERFMODES_STRING;
      nvs->token = (char *)"memTransferRatemin";
      nvs->search = SEARCH_MIN;
      break;
    case ARG_MTR_FREQ_MAX:  // Maximum memory transfer rate clock
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_PERFMODES_STRING;
      nvs->token = (char *)"memTransferRatemax";
      nvs->search = SEARCH_MAX;
      break;

    case ARG_PERF_LEVEL:  // Current performance level
    case ARG_PERF_LEVEL_CUR:
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_PERF_LEVEL;
      break;
    case ARG_PERF_LEVEL_MIN:  // Lowest performance level
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_PERFMODES_STRING;
      nvs->token = (char *)"perf";
      nvs->search = SEARCH_MIN;
      break;
    case ARG_PERF_LEVEL_MAX:  // Highest performance level
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_PERFMODES_STRING;
      nvs->token = (char *)"perf";
      nvs->search = SEARCH_MAX;
      break;
    case ARG_PERF_MODE:  // Performance mode
      nvs->query = QUERY_SPECIAL;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_PERF_MODE;
      break;

    case ARG_GPU_UTIL:  // GPU utilization %
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_UTILS_STRING;
      nvs->token = (char *)"graphics";
      nvs->search = SEARCH_FIRST;
      break;
    case ARG_MEM_BW_UTIL:  // Memory bandwidth utilization %
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_UTILS_STRING;
      nvs->token = (char *)"memory";
      nvs->search = SEARCH_FIRST;
      break;
    case ARG_VIDEO_UTIL:  // Video engine utilization %
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_UTILS_STRING;
      nvs->token = (char *)"video";
      nvs->search = SEARCH_FIRST;
      break;
    case ARG_PCIE_UTIL:  // PCIe bandwidth utilization %
      nvs->query = QUERY_STRING_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_UTILS_STRING;
      nvs->token = (char *)"PCIe";
      nvs->search = SEARCH_FIRST;
      break;

    case ARG_MEM:  // Amount of used memory
    case ARG_MEM_USED:
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_MEM_USED;
      break;
    case ARG_MEM_FREE:  // Amount of free memory
    case ARG_MEM_AVAIL:
      nvs->query = QUERY_SPECIAL;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_MEM_FREE;
      break;
    case ARG_MEM_MAX:  // Total amount of memory
    case ARG_MEM_TOTAL:
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_MEM_TOTAL;
      break;
    case ARG_MEM_UTIL:  // Memory utilization %
    case ARG_MEM_PERC:
      nvs->query = QUERY_SPECIAL;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_MEM_UTIL;
      break;

    case ARG_FAN_SPEED:  // Fan speed
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_COOLER;
      nvs->attribute = ATTR_FAN_SPEED;
      break;
    case ARG_FAN_LEVEL:  // Fan level %
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_COOLER;
      nvs->attribute = ATTR_FAN_LEVEL;
      break;

    case ARG_IMAGEQUALITY:  // Image quality
      nvs->query = QUERY_VALUE;
      nvs->target = TARGET_SCREEN;
      nvs->attribute = ATTR_IMAGE_QUALITY;
      break;

    case ARG_MODEL_NAME:
      nvs->query = QUERY_STRING;
      nvs->target = TARGET_GPU;
      nvs->attribute = ATTR_MODEL_NAME;
      break;

    default:  // Unknown/invalid argument
      // Error printed by core.cc
      return 1;
  }
  return 0;
}

// Return the amount of targets present or raise error)
static inline int get_nvidia_target_count(Display *dpy, TARGET_ID tid) {
  int num_tgts;
  if (!XNVCTRLQueryTargetCount(dpy, translate_nvidia_target[tid], &num_tgts)) {
    num_tgts = -1;
  }

  if (num_tgts < 1) {
    // Print error and exit if there's no NVIDIA's GPU
    CRIT_ERR(NULL, NULL,
             "%s:"
             "\n          Trying to query Nvidia target failed (using the "
             "propietary drivers)."
             "\n          Are you sure they are installed correctly and a "
             "Nvidia GPU is in use?"
             "\n          (display: %d,Nvidia target_count: %d)",
             __func__, dpy, num_tgts);
  }

  return num_tgts;
}

static int cache_nvidia_value(TARGET_ID tid, ATTR_ID aid, Display *dpy,
                              int *value, int gid) {
  static nvidia_c_value ac_value[MAXNUMGPU];

  if (aid == ATTR_MEM_TOTAL) {
    if (ac_value[gid].memtotal < 0) {
      if (!dpy || !XNVCTRLQueryTargetAttribute(
                      dpy, translate_nvidia_target[tid], gid, 0,
                      translate_nvidia_attribute[aid], value)) {
        NORM_ERR(
            "%s: Something went wrong running nvidia query (tid: %d, aid: %d)",
            __func__, tid, aid);
        return -1;
      }
      ac_value[gid].memtotal = *value;
    } else {
      *value = ac_value[gid].memtotal;
    }
  } else if (aid == ATTR_GPU_TEMP_THRESHOLD) {
    if (ac_value[gid].gputempthreshold < 0) {
      if (!dpy || !XNVCTRLQueryTargetAttribute(
                      dpy, translate_nvidia_target[tid], gid, 0,
                      translate_nvidia_attribute[aid], value)) {
        NORM_ERR(
            "%s: Something went wrong running nvidia query (tid: %d, aid: %d)",
            __func__, tid, aid);
        return -1;
      }
      ac_value[gid].gputempthreshold = *value;
    } else {
      *value = ac_value[gid].gputempthreshold;
    }
  }

  return 0;
}

// Retrieve attribute value via nvidia interface
static int get_nvidia_value(TARGET_ID tid, ATTR_ID aid, int gid) {
  Display *dpy = nvdisplay ? nvdisplay : display;
  int value;

  // Check if the aid is cacheable
  if (aid == ATTR_MEM_TOTAL || aid == ATTR_GPU_TEMP_THRESHOLD) {
    if (cache_nvidia_value(tid, aid, dpy, &value, gid)) {
      return -1;
    }
    // If not, then query it
  } else {
    if (!dpy ||
        !XNVCTRLQueryTargetAttribute(dpy, translate_nvidia_target[tid], gid, 0,
                                     translate_nvidia_attribute[aid], &value)) {
      NORM_ERR(
          "%s: Something went wrong running nvidia query (tid: %d, aid: %d)",
          __func__, tid, aid);
      return -1;
    }
  }

  // Unpack clock values (see NVCtrl.h for details)
  if (aid == ATTR_GPU_FREQ) return value >> 16;
  if (aid == ATTR_MEM_FREQ) return value & 0xFFFF;

  // Return value
  return value;
}

// Retrieve attribute string via nvidia interface
static char *get_nvidia_string(TARGET_ID tid, ATTR_ID aid, int gid) {
  Display *dpy = nvdisplay ? nvdisplay : display;
  char *str;

  // Query nvidia interface
  if (!dpy || !XNVCTRLQueryTargetStringAttribute(
                  dpy, translate_nvidia_target[tid], gid, 0,
                  translate_nvidia_attribute[aid], &str)) {
    NORM_ERR(
        "%s: Something went wrong running nvidia string query (tid: %d, aid: "
        "%d, GPU %d)",
        __func__, tid, aid, gid);
    return NULL;
  }
  // fprintf(stderr, "checking get_nvidia_string-> '%s'", str);
  return str;
}

static int cache_nvidia_string_value(TARGET_ID tid, ATTR_ID aid, char *token,
                                     SEARCH_ID search, int *value, int update,
                                     int gid) {
  static nvidia_c_string ac_string[MAXNUMGPU];

  if (update) {
    if (strcmp(token, (char *)"nvclockmin") == 0 &&
        ac_string[gid].nvclockmin < 0) {
      ac_string[gid].nvclockmin = *value;
    } else if (strcmp(token, (char *)"nvclockmax") == 0 &&
               ac_string[gid].nvclockmax < 0) {
      ac_string[gid].nvclockmax = *value;
    } else if (strcmp(token, (char *)"memclockmin") == 0 &&
               ac_string[gid].memclockmin < 0) {
      ac_string[gid].memclockmin = *value;
    } else if (strcmp(token, (char *)"memclockmax") == 0 &&
               ac_string[gid].memclockmax < 0) {
      ac_string[gid].memclockmax = *value;
    } else if (strcmp(token, (char *)"memTransferRatemin") == 0 &&
               ac_string[gid].memTransferRatemin < 0) {
      ac_string[gid].memTransferRatemin = *value;
    } else if (strcmp(token, (char *)"memTransferRatemax") == 0 &&
               ac_string[gid].memTransferRatemax < 0) {
      ac_string[gid].memTransferRatemax = *value;

    } else if (strcmp(token, (char *)"perf") == 0 &&
               ac_string[gid].memTransferRatemax < 0) {
      if (search == SEARCH_MIN) {
        ac_string[gid].perfmin = *value;
      } else if (search == SEARCH_MAX) {
        ac_string[gid].perfmax = *value;
      }
    }

  } else {
    if (strcmp(token, (char *)"nvclockmin") == 0) {
      *value = ac_string[gid].nvclockmin;
    } else if (strcmp(token, (char *)"nvclockmax") == 0) {
      *value = ac_string[gid].nvclockmax;
    } else if (strcmp(token, (char *)"memclockmin") == 0) {
      *value = ac_string[gid].memclockmin;
    } else if (strcmp(token, (char *)"memclockmax") == 0) {
      *value = ac_string[gid].memclockmax;
    } else if (strcmp(token, (char *)"memTransferRatemin") == 0) {
      *value = ac_string[gid].memTransferRatemin;
    } else if (strcmp(token, (char *)"memTransferRatemax") == 0) {
      *value = ac_string[gid].memTransferRatemax;

    } else if (strcmp(token, (char *)"perf") == 0) {
      if (search == SEARCH_MIN) {
        *value = ac_string[gid].perfmin;
      } else if (search == SEARCH_MAX) {
        *value = ac_string[gid].perfmax;
      }
    }
  }

  return 0;
}

// Retrieve token value from nvidia string
static int get_nvidia_string_value(TARGET_ID tid, ATTR_ID aid, char *token,
                                   SEARCH_ID search, int gid) {
  char *str;
  char *kvp, *key, *val;
  char *saveptr1, *saveptr2;
  int value, temp;

  value = -1;

  // Checks if the value is cacheable and is already loaded
  cache_nvidia_string_value(tid, aid, token, search, &value, 0, gid);
  if (value != -1) {
    return value;
  }

  // Get string via nvidia interface
  str = get_nvidia_string(tid, aid, gid);

  // Split string into 'key=value' substrings, split substring
  // into key and value, from value, check if token was found,
  // convert value to int, evaluate value according to specified
  // token search mode
  kvp = strtok_r(str, NV_KVPAIR_SEPARATORS, &saveptr1);
  while (kvp) {
    key = strtok_r(kvp, NV_KEYVAL_SEPARATORS, &saveptr2);
    val = strtok_r(NULL, NV_KEYVAL_SEPARATORS, &saveptr2);
    if (key && val && (strcmp(token, key) == 0)) {
      temp = (int)strtol(val, NULL, 0);
      if (search == SEARCH_FIRST) {
        value = temp;
        break;
      } else if (search == SEARCH_LAST) {
        value = temp;
      } else if (search == SEARCH_MIN) {
        if ((value == -1) || (temp < value)) value = temp;
      } else if (search == SEARCH_MAX) {
        if (temp > value) value = temp;
      } else {
        value = -1;
        break;
      }
    }
    kvp = strtok_r(NULL, NV_KVPAIR_SEPARATORS, &saveptr1);
  }

  // This call updated the cache for the cacheable values;
  cache_nvidia_string_value(tid, aid, token, search, &value, 1, gid);

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
void print_nvidia_value(struct text_object *obj, char *p, int p_max_size) {
  struct nvidia_s *nvs = static_cast<nvidia_s *>(obj->data.opaque);
  int value, temp1, temp2;
  char *str;

  Display *dpy = nvdisplay ? nvdisplay : display;

  // num_GPU calculated only once based on the physical target
  static int num_GPU = get_nvidia_target_count(dpy, TARGET_GPU) - 1;

  // Assume failure
  value = -1;
  str = NULL;

  // Perform query
  if (nvs != NULL) {
    // Reduce overcommitted GPU number to last GPU
    if (nvs->gpu_id > num_GPU) nvs->gpu_id = num_GPU;

    // Execute switch by query type
    switch (nvs->query) {
      case QUERY_VALUE:
        value = get_nvidia_value(nvs->target, nvs->attribute, nvs->gpu_id);
        break;
      case QUERY_STRING:
        str = get_nvidia_string(nvs->target, nvs->attribute, nvs->gpu_id);
        break;
      case QUERY_STRING_VALUE:
        value = get_nvidia_string_value(nvs->target, nvs->attribute, nvs->token,
                                        nvs->search, nvs->gpu_id);
        break;
      case QUERY_SPECIAL:
        switch (nvs->attribute) {
          case ATTR_PERF_MODE:
            temp1 = get_nvidia_value(nvs->target, nvs->attribute, nvs->gpu_id);
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
            temp1 = get_nvidia_value(nvs->target, ATTR_MEM_USED, nvs->gpu_id);
            temp2 = get_nvidia_value(nvs->target, ATTR_MEM_TOTAL, nvs->gpu_id);
            value = temp2 - temp1;
            break;
          case ATTR_MEM_UTIL:
            temp1 = get_nvidia_value(nvs->target, ATTR_MEM_USED, nvs->gpu_id);
            temp2 = get_nvidia_value(nvs->target, ATTR_MEM_TOTAL, nvs->gpu_id);
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

  // Convert query_result to a percentage using ((val-min)÷(max-min)×100)+0.5 if
  // needed.
  if (nvs != NULL) {
    switch (nvs->attribute) {
      case ATTR_UTILS_STRING:  // one of the percentage utils (gpuutil,
                               // membwutil, videoutil and pcieutil)
        value = get_nvidia_string_value(nvs->target, ATTR_UTILS_STRING,
                                        nvs->token, nvs->search, nvs->gpu_id);
        break;
      case ATTR_MEM_UTIL:  // memutil
      case ATTR_MEM_USED:
        temp1 = get_nvidia_value(nvs->target, ATTR_MEM_USED, nvs->gpu_id);
        temp2 = get_nvidia_value(nvs->target, ATTR_MEM_TOTAL, nvs->gpu_id);
        value = ((float)temp1 * 100 / (float)temp2) + 0.5;
        break;
      case ATTR_MEM_FREE:  // memfree
        temp1 = get_nvidia_value(nvs->target, ATTR_MEM_USED, nvs->gpu_id);
        temp2 = get_nvidia_value(nvs->target, ATTR_MEM_TOTAL, nvs->gpu_id);
        value = temp2 - temp1;
        break;
      case ATTR_FAN_SPEED:  // fanspeed: Warn user we are using fanlevel
        NORM_ERR(
            "%s: invalid argument specified: '%s' (using 'fanlevel' instead).",
            nvs->command, nvs->arg);
        // No break, continue into fanlevel
      case ATTR_FAN_LEVEL:  // fanlevel
        value = get_nvidia_value(nvs->target, ATTR_FAN_LEVEL, nvs->gpu_id);
        break;
      case ATTR_GPU_TEMP:  // gputemp (calculate out of gputempthreshold)
        temp1 = get_nvidia_value(nvs->target, ATTR_GPU_TEMP, nvs->gpu_id);
        temp2 =
            get_nvidia_value(nvs->target, ATTR_GPU_TEMP_THRESHOLD, nvs->gpu_id);
        value = ((float)temp1 * 100 / (float)temp2) + 0.5;
        break;
      case ATTR_AMBIENT_TEMP:  // ambienttemp (calculate out of gputempthreshold
                               // for consistency)
        temp1 = get_nvidia_value(nvs->target, ATTR_AMBIENT_TEMP, nvs->gpu_id);
        temp2 =
            get_nvidia_value(nvs->target, ATTR_GPU_TEMP_THRESHOLD, nvs->gpu_id);
        value = ((float)temp1 * 100 / (float)temp2) + 0.5;
        break;
      case ATTR_GPU_FREQ:  // gpufreq (calculate out of gpufreqmax)
        temp1 = get_nvidia_value(nvs->target, ATTR_GPU_FREQ, nvs->gpu_id);
        temp2 = get_nvidia_string_value(nvs->target, ATTR_PERFMODES_STRING,
                                        (char *)"nvclockmax", SEARCH_MAX,
                                        nvs->gpu_id);
        value = ((float)temp1 * 100 / (float)temp2) + 0.5;
        break;
      case ATTR_MEM_FREQ:  // memfreq (calculate out of memfreqmax)
        temp1 = get_nvidia_value(nvs->target, ATTR_MEM_FREQ, nvs->gpu_id);
        temp2 = get_nvidia_string_value(nvs->target, ATTR_PERFMODES_STRING,
                                        (char *)"memclockmax", SEARCH_MAX,
                                        nvs->gpu_id);
        value = ((float)temp1 * 100 / (float)temp2) + 0.5;
        break;
      case ATTR_FREQS_STRING:  // mtrfreq (calculate out of memfreqmax)
        if (nvs->token == "memTransferRate") {
          // Just in case error for silly devs
          CRIT_ERR(NULL, NULL,
                   "%s: attribute is 'ATTR_FREQS_STRING' but token is not "
                   "\"memTransferRate\" (arg: '%s')",
                   nvs->command, nvs->arg);
          return 0;
        }
        temp1 = get_nvidia_string_value(nvs->target, ATTR_FREQS_STRING,
                                        nvs->token, SEARCH_MAX, nvs->gpu_id);
        temp2 = get_nvidia_string_value(nvs->target, ATTR_PERFMODES_STRING,
                                        (char *)"memTransferRatemax",
                                        SEARCH_MAX, nvs->gpu_id);
        if (temp2 > temp1) temp1 = temp2;  // extra safe here
        value = ((float)temp1 * 100 / (float)temp2) + 0.5;
        break;
      case ATTR_IMAGE_QUALITY:  // imagequality
        value = get_nvidia_value(nvs->target, ATTR_IMAGE_QUALITY, nvs->gpu_id);
        break;

      default:  // Throw error if unsupported args are used
        CRIT_ERR(NULL, NULL, "%s: invalid argument specified: '%s'",
                 nvs->command, nvs->arg);
    }
  }

  // Return the percentage
  return value;
}

// Cleanup
void free_nvidia(struct text_object *obj) { free_and_zero(obj->data.opaque); }
