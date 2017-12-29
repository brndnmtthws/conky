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

#include "conky.h"
#include "logging.h"
#include "specials.h"
#include "text_object.h"
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>


#ifdef HAVE_LINUX_SOUNDCARD_H
#include <linux/soundcard.h>
#else
#ifdef __OpenBSD__
#include <soundcard.h>
#elif defined __APPLE__
#include "darwin_soundcard.h"
#else
#include <sys/soundcard.h>
#endif /* __OpenBSD__ */
#endif /* HAVE_LINUX_SOUNDCARD_H */

#define MIXER_DEV "/dev/mixer"

static int mixer_fd;
static const char *devs[] = SOUND_DEVICE_NAMES;

int mixer_init(const char *name)
{
	unsigned int i;

	if (name == 0 || name[0] == '\0') {
		name = "vol";
	}

	/* open mixer */
	if (mixer_fd <= 0) {
		mixer_fd = open(MIXER_DEV, O_RDONLY);
		if (mixer_fd == -1) {
			NORM_ERR("can't open %s: %s", MIXER_DEV, strerror(errno));
			return -1;
		}
	}

	for (i = 0; i < sizeof(devs) / sizeof(const char *); i++) {
		if (strcasecmp(devs[i], name) == 0) {
			return i;
		}
	}

	return -1;
}

static int mixer_get(int i)
{
	static char rep = 0;
	int val = -1;

	if (ioctl(mixer_fd, MIXER_READ(i), &val) == -1) {
		if (!rep) {
			NORM_ERR("mixer ioctl: %s", strerror(errno));
		}
		rep = 1;
		return 0;
	}
	rep = 0;

	return val;
}

static int mixer_get_avg(int i)
{
	int v = mixer_get(i);

	return ((v >> 8) + (v & 0xFF)) / 2;
}

static int mixer_get_left(int i)
{
	return mixer_get(i) >> 8;
}

static int mixer_get_right(int i)
{
	return mixer_get(i) & 0xFF;
}
int mixer_is_mute(int i)
{
	return !mixer_get(i);
}

#define mixer_to_255(i, x) x

void parse_mixer_arg(struct text_object *obj, const char *arg)
{
	obj->data.l = mixer_init(arg);
}

uint8_t mixer_percentage(struct text_object *obj)
{
	return mixer_get_avg(obj->data.l);
}

uint8_t mixerl_percentage(struct text_object *obj)
{
	return mixer_get_left(obj->data.l);
}

uint8_t mixerr_percentage(struct text_object *obj)
{
	return mixer_get_right(obj->data.l);
}

int check_mixer_muted(struct text_object *obj)
{
	if (!mixer_is_mute(obj->data.l))
		return 0;
	return 1;
}

void scan_mixer_bar(struct text_object *obj, const char *arg)
{
	char buf1[64];
	int n;

	if (arg && sscanf(arg, "%63s %n", buf1, &n) >= 1) {
		obj->data.i = mixer_init(buf1);
		scan_bar(obj, arg + n, 100);
	} else {
		obj->data.i = mixer_init(NULL);
		scan_bar(obj, arg, 100);
	}
}

double mixer_barval(struct text_object *obj)
{
	return mixer_to_255(obj->data.i, mixer_get_avg(obj->data.i));
}

double mixerl_barval(struct text_object *obj)
{
	return mixer_to_255(obj->data.i, mixer_get_left(obj->data.i));
}

double mixerr_barval(struct text_object *obj)
{
	return mixer_to_255(obj->data.i, mixer_get_right(obj->data.i));
}
