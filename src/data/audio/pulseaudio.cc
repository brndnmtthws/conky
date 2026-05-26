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
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
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

#include <math.h>
#include <pulse/pulseaudio.h>
#include <unistd.h>
#include <string>

#include "common.h"
#include "conky.h"
#include "content/specials.h"
#include "content/text_object.h"
#include "logging.h"
#include "parse/variables.hh"

using namespace conky::text_object;

struct pulseaudio_default_results {
  std::string sink_name;
  std::string sink_description;
  std::string sink_active_port_name;
  std::string sink_active_port_description;
  uint32_t sink_card;
  int sink_mute;
  uint32_t sink_index;
  unsigned int sink_volume;

  std::string source_name;
  pa_source_state source_state;
  int source_mute;

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
                0, 0, 0, std::string(), PA_SOURCE_SUSPENDED, 0, std::string(),
                std::string(), 0}){};
};

const struct pulseaudio_default_results pulseaudio_result0 = {
    std::string(),
    std::string(),
    std::string(),
    std::string(),
    0,
    0,
    0,
    0,
    std::string(),
    PA_SOURCE_SUSPENDED,
    0,
    std::string(),
    std::string(),
    0};
pulseaudio_c *pulseaudio = nullptr;

void pa_sink_info_callback(pa_context *c, const pa_sink_info *i, int eol,
                           void *data) {
  if (i != nullptr && data) {
    struct pulseaudio_default_results *pdr =
        (struct pulseaudio_default_results *)data;
    pdr->sink_description.assign(i->description);
    pdr->sink_mute = i->mute;
    pdr->sink_card = i->card;
    pdr->sink_index = i->index;
    if (i->active_port != nullptr) {
      pdr->sink_active_port_name.assign(i->active_port->name);
      pdr->sink_active_port_description.assign(i->active_port->description);
    } else {
      pdr->sink_active_port_name.erase();
      pdr->sink_active_port_description.erase();
    }
    pdr->sink_volume = round_to_positive_int(
        100.0f * (float)pa_cvolume_avg(&(i->volume)) / (float)PA_VOLUME_NORM);
    pa_threaded_mainloop_signal(pulseaudio->mainloop, 0);
  }
  (void)c;
  ++eol;
}

void pa_source_info_callback(pa_context *c, const pa_source_info *i, int eol,
                             void *data) {
  if (i != nullptr && data) {
    struct pulseaudio_default_results *pdr =
        (struct pulseaudio_default_results *)data;
    pdr->source_state = i->state;
    pdr->source_mute = i->mute;
    pa_threaded_mainloop_signal(pulseaudio->mainloop, 0);
  }
  (void)c;
  ++eol;
}

void pa_server_info_callback(pa_context *c, const pa_server_info *i,
                             void *userdata) {
  if (i != nullptr) {
    struct pulseaudio_default_results *pdr =
        (struct pulseaudio_default_results *)userdata;
    pdr->sink_name.assign(i->default_sink_name);
    pdr->source_name.assign(i->default_source_name);
    pa_threaded_mainloop_signal(pulseaudio->mainloop, 0);
  }
  (void)c;
}

void pa_server_sink_info_callback(pa_context *c, const pa_server_info *i,
                                  void *userdata) {
  if (i != nullptr) {
    struct pulseaudio_default_results *pdr =
        (struct pulseaudio_default_results *)userdata;
    pdr->sink_name.assign(i->default_sink_name);
    if (pdr->sink_name.empty()) return;
    pa_operation *op;
    if (!(op = pa_context_get_sink_info_by_name(c, pdr->sink_name.c_str(),
                                                pa_sink_info_callback, pdr))) {
      LOG_ERROR("failed to query pulseaudio sink info");
      return;
    }
    pa_operation_unref(op);
  }
  (void)c;
}

void pa_card_info_callback(pa_context *c, const pa_card_info *card, int eol,
                           void *userdata) {
  if (card) {
    struct pulseaudio_default_results *pdr =
        (struct pulseaudio_default_results *)userdata;
    pdr->card_name.assign(card->name);
    pdr->card_index = card->index;
    pdr->card_active_profile_description.assign(
        card->active_profile->description);
    pa_threaded_mainloop_signal(pulseaudio->mainloop, 0);
  }
  (void)c;
  eol++;
}

void context_state_cb(pa_context *c, void *userdata) {
  pulseaudio_c *puau_int = static_cast<pulseaudio_c *>(userdata);
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
  if (!(op = command)) {                  \
    LOG_ERROR(error_msg);                 \
    return;                               \
  }                                       \
  pa_operation_unref(op);

void subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t index,
                  void *userdata) {
  struct pulseaudio_default_results *res =
      (struct pulseaudio_default_results *)userdata;

  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
    case PA_SUBSCRIPTION_EVENT_SINK: {
      if (res->sink_name.empty()) return;
      pa_operation *op;
      PULSEAUDIO_OP(pa_context_get_sink_info_by_name(
                        c, res->sink_name.c_str(), pa_sink_info_callback, res),
                    "pa_context_get_sink_info_by_name failed");
    } break;

    case PA_SUBSCRIPTION_EVENT_SOURCE: {
      if (res->source_name.empty()) return;
      pa_operation *op;
      PULSEAUDIO_OP(
          pa_context_get_source_info_by_name(c, res->source_name.c_str(),
                                             pa_source_info_callback, res),
          "pa_context_get_source_info_by_name failed");
    } break;

    case PA_SUBSCRIPTION_EVENT_CARD:
      if (index == res->card_index && res->card_index != (uint32_t)-1) {
        pa_operation *op;
        PULSEAUDIO_OP(pa_context_get_card_info_by_index(
                          c, index, pa_card_info_callback, res),
                      "failed to query pulseaudio card info")
      }
      break;

    case PA_SUBSCRIPTION_EVENT_SERVER: {
      pa_operation *op;
      PULSEAUDIO_OP(
          pa_context_get_server_info(c, pa_server_sink_info_callback, res),
          "failed to query pulseaudio server info");
    } break;
  }
}

#define PULSEAUDIO_WAIT(COMMAND)                                 \
  {                                                              \
    op = COMMAND;                                                \
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) { \
      pa_threaded_mainloop_wait(pulseaudio->mainloop);           \
    }                                                            \
    pa_operation_unref(op);                                      \
  }

void init_pulseaudio(struct text_object *obj) {
  // already initialized
  (void)obj;
  if (pulseaudio != nullptr && pulseaudio->cstate == PULSE_CONTEXT_READY) {
    pulseaudio->ninits++;
    obj->data.opaque = (void *)pulseaudio;
    return;
  }
  pulseaudio = new pulseaudio_c();
  obj->data.opaque = (void *)pulseaudio;
  pulseaudio->ninits++;

  // Create a mainloop API and connection to the default server
  pulseaudio->mainloop = pa_threaded_mainloop_new();
  if (!pulseaudio->mainloop) LOG_ERROR("cannot create pulseaudio mainloop");

  pulseaudio->mainloop_api = pa_threaded_mainloop_get_api(pulseaudio->mainloop);

  if (!pulseaudio->mainloop_api) LOG_ERROR("cannot get mainloop api");

  pulseaudio->context = pa_context_new(pulseaudio->mainloop_api, "Conky Infos");

  // This function defines a callback so the server will tell us its state.
  pa_context_set_state_callback(pulseaudio->context, context_state_cb,
                                pulseaudio);

  // This function connects to the pulse server
  if (pa_context_connect(pulseaudio->context, nullptr, (pa_context_flags_t)0,
                         nullptr) < 0) {
    SYSTEM_ERR("cannot connect to pulseaudio server");
    return;
  }
  pa_threaded_mainloop_start(pulseaudio->mainloop);

  while (pulseaudio->cstate != PULSE_CONTEXT_READY) {
    struct timespec req;
    struct timespec rem;
    req.tv_sec = 1;
    req.tv_nsec = 200000;

    nanosleep(&req, &rem);
  }

  // Initial parameters update

  pa_operation *op;
  PULSEAUDIO_WAIT(pa_context_get_server_info(
      pulseaudio->context, pa_server_info_callback, &pulseaudio->result));

  if (pulseaudio->result.sink_name.empty()) return;

  PULSEAUDIO_WAIT(pa_context_get_sink_info_by_name(
      pulseaudio->context, pulseaudio->result.sink_name.c_str(),
      pa_sink_info_callback, &pulseaudio->result));

  if (pulseaudio->result.sink_name.empty()) {
    LOG_ERROR("incorrect pulseaudio sink information");
    return;
  }

  if (pulseaudio->result.source_name.empty()) return;

  PULSEAUDIO_WAIT(pa_context_get_source_info_by_name(
      pulseaudio->context, pulseaudio->result.source_name.c_str(),
      pa_source_info_callback, &pulseaudio->result));

  if (pulseaudio->result.source_name.empty()) {
    LOG_ERROR("incorrect pulseaudio source information");
    return;
  }
  if (pulseaudio->result.sink_card != (uint32_t)-1)
    PULSEAUDIO_WAIT(pa_context_get_card_info_by_index(
        pulseaudio->context, pulseaudio->result.sink_card,
        pa_card_info_callback, &pulseaudio->result));

  // get notification when something changes in PA
  pa_context_set_subscribe_callback(pulseaudio->context, subscribe_cb,
                                    &pulseaudio->result);

  if (!(op = pa_context_subscribe(
            pulseaudio->context,
            (pa_subscription_mask_t)(PA_SUBSCRIPTION_MASK_SINK |
                                     PA_SUBSCRIPTION_MASK_SOURCE |
                                     PA_SUBSCRIPTION_MASK_SERVER |
                                     PA_SUBSCRIPTION_MASK_CARD),
            nullptr, NULL))) {
    LOG_ERROR("failed to subscribe to pulseaudio events");
    return;
  }
  pa_operation_unref(op);
}

void free_pulseaudio(struct text_object *obj) {
  pulseaudio_c *puau_int = static_cast<pulseaudio_c *>(obj->data.opaque);

  if (!puau_int) return;

  if (--puau_int->ninits > 0) {
    obj->data.opaque = nullptr;
    return;
  }

  puau_int->cstate = PULSE_CONTEXT_FINISHED;

  if (puau_int->context) {
    pa_context_set_state_callback(puau_int->context, nullptr, NULL);
    pa_context_disconnect(puau_int->context);
    pa_context_unref(puau_int->context);
  }
  if (puau_int->mainloop) {
    pa_threaded_mainloop_stop(puau_int->mainloop);
    pa_threaded_mainloop_free(puau_int->mainloop);
  }
  delete puau_int;
  puau_int = nullptr;
}

struct pulseaudio_default_results get_pulseaudio(struct text_object *obj) {
  pulseaudio_c *puau_int = static_cast<pulseaudio_c *>(obj->data.opaque);
  if (puau_int && puau_int->cstate == PULSE_CONTEXT_READY)
    return puau_int->result;
  return pulseaudio_result0;
}

template <auto Member>
variable_definition puau_print_var(const char *name) {
  return {name, [](text_object *obj, const construct_context &) {
    init_pulseaudio(obj);
    obj->callbacks.print = [](text_object *obj, char *p, unsigned int s) {
      snprintf(p, s, "%s", (get_pulseaudio(obj).*Member).c_str());
    };
    obj->callbacks.free = &free_pulseaudio;
  }};
}

// clang-format off
CONKY_REGISTER_VARIABLES(
    puau_print_var<&pulseaudio_default_results::sink_description>("pa_sink_description"),
    puau_print_var<&pulseaudio_default_results::sink_active_port_name>("pa_sink_active_port_name"),
    puau_print_var<&pulseaudio_default_results::sink_active_port_description>("pa_sink_active_port_description"),
    puau_print_var<&pulseaudio_default_results::card_active_profile_description>("pa_card_active_profile"),
    puau_print_var<&pulseaudio_default_results::card_name>("pa_card_name"),
    {"pa_sink_volume", [](text_object *obj, const construct_context &) {
      init_pulseaudio(obj);
      obj->callbacks.percentage = [](text_object *obj) -> uint8_t {
        return get_pulseaudio(obj).sink_volume;
      };
      obj->callbacks.free = &free_pulseaudio;
    }},
    {"pa_sink_volumebar", [](text_object *obj, const construct_context &ctx) {
      scan_bar(obj, ctx.arg, 1);
      init_pulseaudio(obj);
      obj->callbacks.barval = [](text_object *obj) -> double {
        return get_pulseaudio(obj).sink_volume / 100.0;
      };
      obj->callbacks.free = &free_pulseaudio;
    }},
    {"if_pa_sink_muted", [](text_object *obj, const construct_context &) {
      init_pulseaudio(obj);
      obj->callbacks.iftest = [](text_object *obj) -> int {
        return get_pulseaudio(obj).sink_mute;
      };
      obj->callbacks.free = &free_pulseaudio;
    }, nullptr, {}, obj_flags::cond},
    {"if_pa_source_running", [](text_object *obj, const construct_context &) {
      init_pulseaudio(obj);
      obj->callbacks.iftest = [](text_object *obj) -> int {
        return get_pulseaudio(obj).source_state == PA_SOURCE_RUNNING;
      };
      obj->callbacks.free = &free_pulseaudio;
    }, nullptr, {}, obj_flags::cond},
    {"if_pa_source_muted", [](text_object *obj, const construct_context &) {
      init_pulseaudio(obj);
      obj->callbacks.iftest = [](text_object *obj) -> int {
        return get_pulseaudio(obj).source_mute;
      };
      obj->callbacks.free = &free_pulseaudio;
    }, nullptr, {}, obj_flags::cond},
)
// clang-format on
