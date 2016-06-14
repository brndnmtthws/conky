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
*     (see AUTHORS)
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

#include "common.h"
#include "config.h"
#include "conky.h"
#include "core.h"
#include "logging.h"
#include "specials.h"
#include "text_object.h"
#include "pulseaudio.h"
#include <string.h>
#include <math.h>

void pa_sink_info_callback(pa_context *c, const pa_sink_info *i, int eol, void *data) {
    if (i != NULL && data) {
        struct pulseaudio_default_results *dsi = (struct pulseaudio_default_results *)data;
        dsi->sink_name.assign( i->name);
        dsi->sink_description.assign( i->description);
        dsi->sink_mute = i->mute;
        dsi->sink_card = i->card;
        dsi->sink_volume = round_to_int(100.0f * (float)pa_cvolume_avg(&(i->volume)) / (float)PA_VOLUME_NORM);
    }
    (void) c;
    ++eol;
}

void pa_server_info_callback(pa_context *c, const pa_server_info *i, void *userdata) {
    if (i != NULL) {
        char **o = (char **) userdata;
        *o = strdup( i->default_sink_name);
    }
    (void )c;
}

void pa_card_info_callback(pa_context *c, const pa_card_info *card,
                           int eol, void *userdata) {
    if (card) {
        struct pulseaudio_default_results *pdr = (struct pulseaudio_default_results *)userdata;
        pdr->card_name.assign( card->name);
        pdr->card_active_profile_description.assign( card->active_profile->description);
    }
    (void) c;
    eol++;
}

void pulseaudio_context_state_cb(pa_context *c, void *userdata) {
    enum pulseaudio_state *pulseaudio_context_state = (enum pulseaudio_state *)userdata;
    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY: {
            *pulseaudio_context_state = PULSE_CONTEXT_READY;
            break;
        }
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED: {
            *pulseaudio_context_state = PULSE_CONTEXT_FINISHED;
            break;
        }
        default:
            return;
    }
}

void pulseaudio_cb::init_pulseaudio() {

    // already initialized
    if(pulseaudio_context_state == PULSE_CONTEXT_READY)
        return;

    // Create a mainloop API and connection to the default server
    pulseaudio_ml = pa_mainloop_new();
    if (!pulseaudio_ml)
        NORM_ERR("Cannot create pulseaudio mainloop");

    pulseaudio_mlapi = pa_mainloop_get_api(pulseaudio_ml);

    if (!pulseaudio_mlapi)
        NORM_ERR("Cannot get mainloop api");

    pulseaudio_context = pa_context_new(pulseaudio_mlapi, "Conky Infos");

    // This function defines a callback so the server will tell us its state.
    pa_context_set_state_callback(pulseaudio_context,
                                  pulseaudio_context_state_cb, &pulseaudio_context_state);

    // This function connects to the pulse server
    if (pa_context_connect(pulseaudio_context, NULL, (pa_context_flags_t)0, NULL) < 0) {
        NORM_ERR("Cannot connect to pulseaudio");
    }

    while (pulseaudio_context_state != PULSE_CONTEXT_READY)
        if(pa_mainloop_iterate(pulseaudio_ml, 0, NULL) <0 ){
            NORM_ERR("pulseaudio: error in mainloop iterate (init)");
            return;
    }
}

#define PULSEAUDIO_ITERATE(COMMAND)                             \
    op = COMMAND;                                               \
    while ((pa_operation_get_state(op) != PA_OPERATION_DONE))   \
        if(pa_mainloop_iterate(pulseaudio_ml, 0, NULL) < 0) {   \
            NORM_ERR("pulseaudio: error in mainloop iterate");  \
            pa_operation_unref(op);                             \
            return;                                             \
    }                                                           \
    pa_operation_unref(op);

void pulseaudio_cb::work(){
    printf("working\n");
    char *default_sink_name =NULL ;
    struct pulseaudio_default_results res =
        { std::string(), std::string(), 0, 0 , 0, std::string(), std::string()};

    if (pulseaudio_context_state != PULSE_CONTEXT_READY) {
        init_pulseaudio();
        return;
    }

    pa_operation *op;
    PULSEAUDIO_ITERATE(pa_context_get_server_info(pulseaudio_context,
                                                  pa_server_info_callback, &default_sink_name));

    if (default_sink_name == NULL)
        return;

    PULSEAUDIO_ITERATE(pa_context_get_sink_info_by_name(pulseaudio_context, default_sink_name,
                                                        pa_sink_info_callback, &res));

    free(default_sink_name);

    if (res.sink_name.empty())
        return;

    PULSEAUDIO_ITERATE(pa_context_get_card_info_by_index(pulseaudio_context,
                                                         res.sink_card, pa_card_info_callback, &res));

    std::lock_guard<std::mutex> l(result_mutex);
    result = res;
}

pulseaudio_cb::~pulseaudio_cb() {
    pulseaudio_context_state = PULSE_CONTEXT_FINISHED;
    if (pulseaudio_context) {
        pa_context_set_state_callback(pulseaudio_context, NULL, NULL);
        pa_context_disconnect(pulseaudio_context);
        pa_context_unref(pulseaudio_context);
    }
    if (pulseaudio_ml) {
        pa_mainloop_quit(pulseaudio_ml, 0);
        pa_mainloop_free(pulseaudio_ml);
    }
}

struct pulseaudio_default_results get_pulseaudio(struct text_object *obj){
    (void)obj;
    uint32_t period = std::max(
        lround(music_player_interval.get(*state)/active_update_interval()), 1l);

    return (conky::register_cb<pulseaudio_cb>(period ))->get_result_copy();
}

uint8_t puau_vol(struct text_object *obj){
    return get_pulseaudio(obj).sink_volume;
}

int puau_muted(struct text_object *obj){
    return get_pulseaudio(obj).sink_mute;
}

void print_puau_sink_description(struct text_object *obj, char *p, int p_max_size){
    snprintf(p, p_max_size, "%s", get_pulseaudio(obj).sink_description.c_str());
}

void print_puau_card_active_profile(struct text_object *obj, char *p, int p_max_size){
    snprintf(p, p_max_size, "%s", get_pulseaudio(obj).card_active_profile_description.c_str());
}

void print_puau_card_name(struct text_object *obj, char *p, int p_max_size){
    snprintf(p, p_max_size, "%s", get_pulseaudio(obj).card_name.c_str());
}

double puau_volumebarval(struct text_object *obj){
    return get_pulseaudio(obj).sink_volume/100.0f;
}
