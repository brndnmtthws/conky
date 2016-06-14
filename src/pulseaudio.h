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
* Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
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

#ifndef _PULSEAUDIO_H
#define _PULSEAUDIO_H

#include "text_object.h"
#include <pulse/pulseaudio.h>

uint8_t puau_vol(struct text_object *); // preserve pa_* for libpulse
void  print_puau_sink_description(struct text_object *obj, char *p, int p_max_size);
void  print_puau_card_name(struct text_object *obj, char *p, int p_max_size);
void  print_puau_card_active_profile(struct text_object *obj, char *p, int p_max_size);
double puau_volumebarval(struct text_object *obj);
int puau_muted(struct text_object *obj);

struct pulseaudio_default_results {
    // default sink
    std::string sink_name;
    std::string sink_description;
    uint32_t sink_card;
    int sink_mute;
    unsigned int sink_volume; // percentage

    // default card
    std::string card_active_profile_description;
    std::string card_name;
};

enum pulseaudio_state {
	PULSE_CONTEXT_INITIALIZING,
	PULSE_CONTEXT_READY,
	PULSE_CONTEXT_FINISHED
};

class pulseaudio_cb: public conky::callback<pulseaudio_default_results> {
	typedef conky::callback< pulseaudio_default_results> Base;
	pa_mainloop *pulseaudio_ml = NULL;
	pa_mainloop_api *pulseaudio_mlapi = NULL;

	pa_context *pulseaudio_context = NULL;

	int ret;

	enum pulseaudio_state pulseaudio_context_state = PULSE_CONTEXT_INITIALIZING;

	protected:
		virtual void work();

	public:
		pulseaudio_cb(uint32_t period)
			: Base(period, false, Base::Tuple())
		{
			init_pulseaudio();
			result={ std::string(), std::string(), 0, 0 , 0, std::string(), std::string()};
		}
    	~pulseaudio_cb();

	    void init_pulseaudio();
};


#endif /* _PULSEAUDIO_H */
