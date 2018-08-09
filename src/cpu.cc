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
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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

#include "config.h"
#include "conky.h"
#include "text_object.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#ifdef __x86_64__
#define CPU_FEATURE(x, z) __asm__ __volatile__("cpuid": "=a" (z) : "a" (x))
#define CPU_REGS(x, y, z) __asm__ __volatile__ ( \
  "cpuid": \
  "=a" (z), \
  "=b" (y) \
  : "a" (x) \
)
#define CPU_STR2(regizter, a, b, c, d) __asm__ __volatile__ ( \
  "cpuid": \
    "=a" (a), \
    "=b" (b), \
    "=c" (c), \
    "=d" (d) \
    : "a" (regizter) \
)

#define AmD    0x68747541
#define InteL  0x756e6547
#define FMT_UINT "%" PRIuMAX

#if defined(__FreeBSD__)
# define TICKZ 100L
#else
# define TICKZ sysconf(_SC_CLK_TCK)
#endif /* __FreeBSD__ */

uint8_t has_tsc_reg(void) {
  uint_fast16_t vend = 0;
  uint_fast16_t leafs = 0;
  uint_fast16_t eax = 0;
  uint_fast16_t ecx = 0;
  uint_fast16_t edx = 0;
  uint_fast16_t ebx = 0;

  CPU_REGS(0x00000000, vend, leafs);
  if (0x00000001 > leafs) {
    return 1U;
  }
  if (vend != AmD && vend != InteL) {
    return 1U;
  }

  CPU_STR2(0x00000001, eax, ebx, ecx, edx);
  if (0U == (edx & (1U << 4U))) {
    return 1U;
  }
  return 0U;
}

uintmax_t rdtsc(void) {
  unsigned int tickhi = 0;
  unsigned int ticklo = 0;
  uint_fast16_t eax = 0;
  uint_fast16_t ecx = 0;
  uint_fast16_t edx = 0;
  uint_fast16_t ebx = 0;
  uint_fast16_t regz = 0;
  uint_fast16_t x = 0;

  if (0U != (has_tsc_reg())) {
    goto seeya;
  }
  __asm__ __volatile__ (
    "cpuid\n\t"
    "rdtsc\n\t"
    : "=a"(ticklo), "=d"(tickhi)
    :: "%rbx", "%rcx"
  );

  CPU_FEATURE(0x80000000, regz);
  if (0x80000001 > regz) {
    goto seeya;
  }
  CPU_STR2(0x80000001, eax, ebx, ecx, edx);

  if (0U != (edx & (1U << 27U))) {
    for (x = 0; x < 6U; x++) {
      __asm__ __volatile__ (
        "rdtscp\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        "cpuid\n\t"
        : "=r"(tickhi), "=r"(ticklo)
        :: "%rax", "%rbx", "%rcx", "%rdx"
      );
    }
  }

seeya:
  return (((uintmax_t)tickhi << 32) | (uintmax_t)ticklo);
}

void
get_cpu_clock_speed(char *str1, unsigned int p_max_size) {
  uintmax_t x = 0;
  uintmax_t z = 0;
  struct timespec tc = {0L, 0L};

  tc.tv_nsec = TICKZ * 1000000L;

  x = rdtsc();
  if (-1 == (nanosleep(&tc, NULL))) {
    return;
  }
  z = rdtsc();

  snprintf(str1, p_max_size, FMT_UINT " MHz", ((z - x) / 100000U));
}

void print_freq2(struct text_object *obj, char *p, unsigned int p_max_size) {
  (void)obj;
  get_cpu_clock_speed(p, p_max_size);
}

#else
char *l337;
#endif /* __x86_64__ */
