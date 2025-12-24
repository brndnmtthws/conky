/*
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
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

#include "darwin_top_helpers.h"

#include <mach/mach_time.h>

void reset_cpu_sample_overall(struct cpusample *sample) {
  if (sample == nullptr) { return; }

  sample[0].totalUserTime = 0;
  sample[0].totalSystemTime = 0;
  sample[0].totalIdleTime = 0;
}

void sum_cpu_sample_overall(struct cpusample *sample, size_t processorCount) {
  if (sample == nullptr) { return; }

  reset_cpu_sample_overall(sample);

  for (size_t i = 1; i < processorCount + 1; i++) {
    sample[0].totalSystemTime += sample[i].totalSystemTime;
    sample[0].totalUserTime += sample[i].totalUserTime;
    sample[0].totalIdleTime += sample[i].totalIdleTime;
  }
}

uint64_t cpu_total_delta(uint64_t current_total,
                         unsigned long *previous_total_cpu_time) {
  if (previous_total_cpu_time == nullptr) { return 0; }

  if (*previous_total_cpu_time == ULONG_MAX) {
    *previous_total_cpu_time = current_total;
    return 0;
  }

  uint64_t delta = current_total - *previous_total_cpu_time;
  *previous_total_cpu_time = current_total;
  return delta;
}

uint64_t mach_ticks_to_centis(uint64_t ticks, uint32_t numer, uint32_t denom) {
  if (ticks == 0 || denom == 0) { return 0; }

  unsigned __int128 ns = static_cast<unsigned __int128>(ticks) * numer;
  ns /= denom;
  return static_cast<uint64_t>(ns / 10000000ULL);
}

uint64_t mach_ticks_to_centis_system(uint64_t ticks) {
  static bool initialized = false;
  static mach_timebase_info_data_t timebase{};

  if (!initialized) {
    (void)mach_timebase_info(&timebase);
    initialized = true;
  }

  return mach_ticks_to_centis(ticks, timebase.numer, timebase.denom);
}
