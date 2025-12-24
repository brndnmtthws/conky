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

#ifndef CONKY_DARWIN_TOP_HELPERS_H
#define CONKY_DARWIN_TOP_HELPERS_H

#include <climits>
#include <cstddef>
#include <cstdint>

/*
 * useful info about the cpu used by functions such as update_cpu_usage() and
 * get_top_info()
 */
struct cpusample {
  uint64_t totalUserTime;   /* ticks of CPU in userspace */
  uint64_t totalSystemTime; /* ticks of CPU in kernelspace */
  uint64_t totalIdleTime;   /* ticks in idleness */

  uint64_t total;          /* delta of current and previous */
  uint64_t current_total;  /* total CPU ticks of current iteration */
  uint64_t previous_total; /* total CPU tick of previous iteration */
};

void reset_cpu_sample_overall(struct cpusample *sample);
void sum_cpu_sample_overall(struct cpusample *sample, size_t processorCount);
uint64_t cpu_total_delta(uint64_t current_total,
                         unsigned long *previous_total_cpu_time);
uint64_t mach_ticks_to_centis(uint64_t ticks, uint32_t numer, uint32_t denom);
uint64_t mach_ticks_to_centis_system(uint64_t ticks);

#endif /* CONKY_DARWIN_TOP_HELPERS_H */
