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
#include "pulseaudio.h"
#include "specials.h"
#include "text_object.h"
#include <math.h>
#include <unistd.h>

struct pulseaudio_default_results get_result_copy();


const struct pulseaudio_default_results pulseaudio_result0 =
             { std::string(), std::string(), 0, 0, 0, 0, std::string(), std::string(), 0 };
pulseaudio_c *pulseaudio = NULL;

void pa_sink_info_callback(pa_context *c, const pa_sink_info *i, int eol, void *data) {
    if (i != NULL && data) {
        struct pulseaudio_default_results *pdr = (struct pulseaudio_default_results *)data;
        pdr->sink_description.assign(i->description);
        pdr->sink_mute = i->mute;
        pdr->sink_card = i->card;
        pdr->sink_index = i->index;
        pdr->sink_volume = round_to_int(100.0f * (float)pa_cvolume_avg(&(i->volume)) / (float)PA_VOLUME_NORM);
        pa_threaded_mainloop_signal(pulseaudio->mainloop, 0);
    }
    (void)c;
    ++eol;
}

void pa_server_info_callback(pa_context *c, const pa_server_info *i, void *userdata) {
    if (i != NULL) {
        struct pulseaudio_default_results *pdr = (struct pulseaudio_default_results *)userdata;
        pdr->sink_name.assign(i->default_sink_name);
        pa_threaded_mainloop_signal(pulseaudio->mainloop, 0);
    }
    (void)c;
}

void pa_server_sink_info_callback(pa_context *c, const pa_server_info *i, void *userdata) {
    if (i != NULL) {
        struct pulseaudio_default_results *pdr = (struct pulseaudio_default_results *)userdata;
        pdr->sink_name.assign(i->default_sink_name);
        if(pdr->sink_name.empty()) return;
        pa_operation *op;
        if (!(op = pa_context_get_sink_info_by_name(c, pdr->sink_name.c_str(), pa_sink_info_callback, pdr))) {
            NORM_ERR("pa_context_get_sink_info_by_index() failed");
            return;
        }
        pa_operation_unref(op);
    }
    (void)c;
}

void pa_card_info_callback(pa_context *c, const pa_card_info *card,
                           int eol, void *userdata) {
    if (card) {
        struct pulseaudio_default_results *pdr = (struct pulseaudio_default_results *)userdata;
        pdr->card_name.assign(card->name);
        pdr->card_index = card->index;
        pdr->card_active_profile_description.assign(card->active_profile->description);
        pa_threaded_mainloop_signal(pulseaudio->mainloop, 0);
    }
    (void)c;
    eol++;
}

void context_state_cb(pa_context *c, void *userdata) {
    pulseaudio_c *puau_int = static_cast<pulseaudio_c *>( userdata);
    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY: {
            puau_int->cstate = PULSE_CONTEXT_READY;
            break;
        }
        case PA_CONTEXT_FAILED:
        case PA_CONTEXT_TERMINATED: {
            puau_int->cstate = PULSE_CONTEXT_FINISHED;
            break;
        }
        default:
            return;
    }
}

#define PULSEAUDIO_OP(command, error_msg) \
    if (!(op = command)) {                \
        NORM_ERR(error_msg);              \
        return;                           \
    }                                     \
    pa_operation_unref(op);


void subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *userdata) {
    struct pulseaudio_default_results *res = (struct pulseaudio_default_results *)userdata;

    switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK: {
            if(res->sink_name.empty()) return;
            pa_operation *op;
            PULSEAUDIO_OP(pa_context_get_sink_info_by_name(c, res->sink_name.c_str(),
                                                           pa_sink_info_callback, res),
                          "pa_context_get_sink_info_by_name failed");
        } break;

        case PA_SUBSCRIPTION_EVENT_CARD:
            if (index == res->card_index && res->card_index != (uint32_t)-1) {
                pa_operation *op;
                PULSEAUDIO_OP(pa_context_get_card_info_by_index(c,
                                                                index, pa_card_info_callback, res),
                              "pa_context_get_card_info_by_index() failed")
            }
            break;

        case PA_SUBSCRIPTION_EVENT_SERVER: {
            pa_operation *op;
            PULSEAUDIO_OP(pa_context_get_server_info(c,
                                                     pa_server_sink_info_callback, res),
                          "pa_context_get_server_info() failed");
        } break;
    }
}

#define PULSEAUDIO_WAIT(COMMAND)                                 \
    op = COMMAND;                                                \
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) { \
        pa_threaded_mainloop_wait(pulseaudio->mainloop);         \
    }                                                            \
    pa_operation_unref(op);


void init_pulseaudio(struct text_object *obj) {
    // already initialized
    (void)obj;
    if(pulseaudio != NULL && pulseaudio->cstate == PULSE_CONTEXT_READY) {
        pulseaudio->ninits++;
        obj->data.opaque = (void *)pulseaudio;
        return ;
    }
    pulseaudio = new pulseaudio_c();
    obj->data.opaque = (void *)pulseaudio;
    pulseaudio->ninits++;

    // Create a mainloop API and connection to the default server
    pulseaudio->mainloop = pa_threaded_mainloop_new();
    if (!pulseaudio->mainloop)
        NORM_ERR("Cannot create pulseaudio mainloop");

    pulseaudio->mainloop_api = pa_threaded_mainloop_get_api(pulseaudio->mainloop);

    if (!pulseaudio->mainloop_api)
        NORM_ERR("Cannot get mainloop api");

    pulseaudio->context = pa_context_new(pulseaudio->mainloop_api, "Conky Infos");

    // This function defines a callback so the server will tell us its state.
    pa_context_set_state_callback(pulseaudio->context,
                                  context_state_cb, pulseaudio);

    // This function connects to the pulse server
    if (pa_context_connect(pulseaudio->context, NULL, (pa_context_flags_t)0, NULL) < 0) {
        CRIT_ERR(NULL, NULL, "Cannot connect to pulseaudio");
        return;
    }
    pa_threaded_mainloop_start(pulseaudio->mainloop);

    while (pulseaudio->cstate != PULSE_CONTEXT_READY) {
        usleep(200);
    }

    //Initial parameters update

    pa_operation *op;
    PULSEAUDIO_WAIT(pa_context_get_server_info(pulseaudio->context,
                                               pa_server_info_callback, &pulseaudio->result));

    if (pulseaudio->result.sink_name.empty())
        return;

    PULSEAUDIO_WAIT(pa_context_get_sink_info_by_name(pulseaudio->context, pulseaudio->result.sink_name.c_str(),
                                                     pa_sink_info_callback, &pulseaudio->result));

    if (pulseaudio->result.sink_name.empty()){
        NORM_ERR("Incorrect pulseaudio sink information.");
        return;
    }

    if(pulseaudio->result.sink_card!=(uint32_t)-1)
        PULSEAUDIO_WAIT(pa_context_get_card_info_by_index(pulseaudio->context, pulseaudio->result.sink_card,
                                                          pa_card_info_callback, &pulseaudio->result));

    // get notification when something changes in PA
    pa_context_set_subscribe_callback(pulseaudio->context, subscribe_cb, &pulseaudio->result);

    if (!(op = pa_context_subscribe(pulseaudio->context, (pa_subscription_mask_t)(PA_SUBSCRIPTION_MASK_SINK |
                                                                                 PA_SUBSCRIPTION_MASK_SERVER |
                                                                                 PA_SUBSCRIPTION_MASK_CARD),
                                    NULL, NULL))) {
        NORM_ERR("pa_context_subscribe() failed");
        return;
    }
    pa_operation_unref(op);

}

void free_pulseaudio(struct text_object *obj) {
    pulseaudio_c *puau_int = static_cast<pulseaudio_c *>( obj->data.opaque);

    if(!puau_int) return;

    if(--puau_int->ninits > 0) {
        obj->data.opaque=NULL;
        return;
    }

    puau_int->cstate = PULSE_CONTEXT_FINISHED;

    if (puau_int->context) {
        pa_context_set_state_callback(puau_int->context, NULL, NULL);
        pa_context_disconnect(puau_int->context);
        pa_context_unref(puau_int->context);
    }
    if (puau_int->mainloop) {
        pa_threaded_mainloop_stop(puau_int->mainloop);
        pa_threaded_mainloop_free(puau_int->mainloop);
    }
    delete puau_int;
    puau_int = NULL;
}

struct pulseaudio_default_results get_pulseaudio(struct text_object *obj) {
    pulseaudio_c *puau_int = static_cast<pulseaudio_c *>( obj->data.opaque);
    if(puau_int && puau_int->cstate == PULSE_CONTEXT_READY)
        return puau_int->result;
    return pulseaudio_result0;
}

uint8_t puau_vol(struct text_object *obj) {
    return get_pulseaudio(obj).sink_volume;
}

int puau_muted(struct text_object *obj) {
    return get_pulseaudio(obj).sink_mute;
}

void print_puau_sink_description(struct text_object *obj, char *p, int p_max_size) {
    snprintf(p, p_max_size, "%s", get_pulseaudio(obj).sink_description.c_str());
}

void print_puau_card_active_profile(struct text_object *obj, char *p, int p_max_size) {
    snprintf(p, p_max_size, "%s", get_pulseaudio(obj).card_active_profile_description.c_str());
}

void print_puau_card_name(struct text_object *obj, char *p, int p_max_size) {
    snprintf(p, p_max_size, "%s", get_pulseaudio(obj).card_name.c_str());
}

double puau_volumebarval(struct text_object *obj) {
    return get_pulseaudio(obj).sink_volume / 100.0f;
}
