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

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "content/specials.h"
#include "content/text_object.h"
#include "parse/variables.hh"

using namespace conky::text_object;

#ifdef HAVE_SOUNDCARD_H
#if defined(__linux__)
#include <linux/soundcard.h>
#elif defined(__OpenBSD__)
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif
#endif /* HAVE_SOUNDCARD_H */

#if defined(__sun)
#include <stropts.h>
#include <unistd.h>
#endif

#define MIXER_DEV "/dev/mixer"

static int mixer_fd;
static const char *devs[] = SOUND_DEVICE_NAMES;

int mixer_init(const char *name) {
  unsigned int i;

  if (name == 0 || name[0] == '\0') { name = "vol"; }

  /* open mixer */
  if (mixer_fd <= 0) {
    mixer_fd = open(MIXER_DEV, O_RDONLY);
    if (mixer_fd == -1) {
      LOG_ERROR("can't open {}: {}", MIXER_DEV, strerror(errno));
      return -1;
    }
  }

  for (i = 0; i < sizeof(devs) / sizeof(const char *); i++) {
    if (strcasecmp(devs[i], name) == 0) { return i; }
  }

  return -1;
}

static int mixer_get(int i) {
  static char rep = 0;
  int val = -1;

  if (ioctl(mixer_fd, MIXER_READ(i), &val) == -1) {
    if (!rep) { LOG_ERROR("mixer ioctl: {}", strerror(errno)); }
    rep = 1;
    return 0;
  }
  rep = 0;

  return val;
}

static int mixer_get_avg(int i) {
  int v = mixer_get(i);

  return ((v >> 8) + (v & 0xFF)) / 2;
}

static int mixer_get_left(int i) { return mixer_get(i) >> 8; }

static int mixer_get_right(int i) { return mixer_get(i) & 0xFF; }
int mixer_is_mute(int i) { return !mixer_get(i); }

using namespace conky::text_object;

using mixer_channel_fn = int (*)(int);

template <mixer_channel_fn channel>
variable_definition mixer_perc_var(const char *name) {
  return {name, [](text_object *obj, const construct_context &ctx) {
    obj->data.l = mixer_init(ctx.arg);
    obj->callbacks.percentage = [](text_object *obj) -> uint8_t {
      return channel(obj->data.l);
    };
  }};
}

template <mixer_channel_fn channel>
variable_definition mixer_bar_var(const char *name) {
  return {name, [](text_object *obj, const construct_context &ctx) {
    char buf1[64];
    int n;
    if (ctx.arg && sscanf(ctx.arg, "%63s %n", buf1, &n) >= 1) {
      obj->data.i = mixer_init(buf1);
      scan_bar(obj, ctx.arg + n, 100);
    } else {
      obj->data.i = mixer_init(nullptr);
      scan_bar(obj, ctx.arg, 100);
    }
    obj->callbacks.barval = [](text_object *obj) -> double {
      return channel(obj->data.i);
    };
  }};
}

// clang-format off
CONKY_REGISTER_VARIABLES(
    mixer_perc_var<mixer_get_avg>("mixer"),
    mixer_perc_var<mixer_get_left>("mixerl"),
    mixer_perc_var<mixer_get_right>("mixerr"),
    mixer_bar_var<mixer_get_avg>("mixerbar"),
    mixer_bar_var<mixer_get_left>("mixerlbar"),
    mixer_bar_var<mixer_get_right>("mixerrbar"),
    {"if_mixer_mute", [](text_object *obj, const construct_context &ctx) {
      obj->data.l = mixer_init(ctx.arg);
      obj->callbacks.iftest = [](text_object *obj) -> int {
        return mixer_is_mute(obj->data.l) ? 1 : 0;
      };
    }, nullptr, {}, obj_flags::cond},
)
// clang-format on
