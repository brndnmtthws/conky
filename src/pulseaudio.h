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
 * Copyright (c) 2005-2019 Brenden Matthews, Philip Kovacs, et. al.
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

#ifndef _PULSEAUDIO_H
#define _PULSEAUDIO_H

#include <pulse/pulseaudio.h>
#include "text_object.h"

void init_pulseaudio(struct text_object *obj);
void free_pulseaudio(struct text_object *obj);
uint8_t puau_vol(struct text_object *);  // preserve pa_* for libpulse
void print_puau_sink_description(struct text_object *obj, char *p,
                                 unsigned int p_max_size);
void print_puau_sink_active_port_name(struct text_object *obj, char *p,
                                      unsigned int p_max_size);
void print_puau_sink_active_port_description(struct text_object *obj, char *p,
                                             unsigned int p_max_size);
void print_puau_card_name(struct text_object *obj, char *p,
                          unsigned int p_max_size);
void print_puau_card_active_profile(struct text_object *obj, char *p,
                                    unsigned int p_max_size);
double puau_volumebarval(struct text_object *obj);
int puau_muted(struct text_object *obj);

struct pulseaudio_default_results {
  // default sink
  std::string sink_name;
  std::string sink_description;
  std::string sink_active_port_name;
  std::string sink_active_port_description;
  uint32_t sink_card;
  int sink_mute;
  uint32_t sink_index;
  unsigned int sink_volume;  // percentage

  // default card
  std::string card_active_profile_description;
  std::string card_name;
  uint32_t card_index;
};

enum pulseaudio_state {
  PULSE_CONTEXT_INITIALIZING,
  PULSE_CONTEXT_READY,
  PULSE_CONTEXT_FINISHED
};

class pulseaudio_c {
 public:
  pa_threaded_mainloop *mainloop;
  pa_mainloop_api *mainloop_api;
  pa_context *context;
  volatile enum pulseaudio_state cstate;
  int ninits;
  struct pulseaudio_default_results result;
  pulseaudio_c()
      : mainloop(nullptr),
        mainloop_api(nullptr),
        context(nullptr),
        cstate(PULSE_CONTEXT_INITIALIZING),
        ninits(0),
        result({std::string(), std::string(), std::string(), std::string(), 0,
                0, 0, 0, std::string(), std::string(), 0}){};
};

#endif /* _PULSEAUDIO_H */
