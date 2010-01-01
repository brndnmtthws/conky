/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=c
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
 * Copyright (c) 2005-2010 Brenden Matthews, Philip Kovacs, et. al.
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


#ifdef MIXER_IS_ALSA
#include <alsa/asoundlib.h>
#else
#ifdef HAVE_LINUX_SOUNDCARD_H
#include <linux/soundcard.h>
#else
#ifdef __OpenBSD__
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif /* __OpenBSD__ */
#endif /* HAVE_LINUX_SOUNDCARD_H */
#endif /* MIXER_IS_ALSA */

#define MIXER_DEV "/dev/mixer"

#ifdef MIXER_IS_ALSA
#define MAX_MIXERS 8
struct mixer_control {
	char name[64];
	snd_mixer_t *mixer;
	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;
	long vol_min, vol_max;
};

static struct mixer_control mixer_data[MAX_MIXERS];
int num_mixers = 0;
static char soundcard[64] = "default";
#else
static int mixer_fd;
static const char *devs[] = SOUND_DEVICE_NAMES;
#endif

#ifdef MIXER_IS_ALSA
static int parse_simple_id(const char *str, snd_mixer_selem_id_t *sid)
{
	int c, size;
	char buf[128];
	char *ptr = buf;

	while (*str == ' ' || *str == '\t')
		str++;
	if (!(*str))
		return -EINVAL;
	size = 1;	/* for '\0' */
	if (*str != '"' && *str != '\'') {
		while (*str && *str != ',') {
			if (size < (int)sizeof(buf)) {
				*ptr++ = *str;
				size++;
			}
			str++;
		}
	} else {
		c = *str++;
		while (*str && *str != c) {
			if (size < (int)sizeof(buf)) {
				*ptr++ = *str;
				size++;
			}
			str++;
		}
		if (*str == c)
			str++;
	}
	if (*str == '\0') {
		snd_mixer_selem_id_set_index(sid, 0);
		*ptr = 0;
		goto _set;
	}
	if (*str != ',')
		return -EINVAL;
	*ptr = 0;	/* terminate the string */
	str++;
	if (!isdigit(*str))
		return -EINVAL;
	snd_mixer_selem_id_set_index(sid, atoi(str));
       _set:
	snd_mixer_selem_id_set_name(sid, buf);
	return 0;
}

int mixer_init (const char *name)
{
	/* from amixer.c, replaced -EINVAL with -1 */
	int i, err;
	if (!name)
		name = "Master";

	for (i = 0; i < num_mixers; i++) {
		if (!strcasecmp (mixer_data[i].name, name)) {
			return i;
		}
	}
	if (i == MAX_MIXERS) {
		fprintf (stderr, "max mixers (%d) reached\n", MAX_MIXERS);
		return -1;
	};

	num_mixers++;
#define data mixer_data[i]

	strncpy (mixer_data[i].name, name, 63);
	mixer_data[i].name[63] = '\0';
	snd_mixer_selem_id_alloca (&data.sid);
	data.mixer = NULL;
	if (parse_simple_id (name, data.sid) < 0) {
		fprintf (stderr, "Wrong mixer identifier: %s\n", name);
		return -1;
	}
	if ((err = snd_mixer_open (&data.mixer, 0)) < 0) {
		fprintf (stderr, "snd_mixer_open: %s\n", snd_strerror (err));
		return -1;
	}
	if ((err = snd_mixer_attach (data.mixer, soundcard)) < 0) {
		fprintf (stderr, "snd_mixer_attach: %s\n", snd_strerror (err));
		return -1;
	}
	if ((err = snd_mixer_selem_register (data.mixer, NULL, NULL)) < 0) {
		fprintf (stderr, "snd_mixer_selem_register: %s\n",
			 snd_strerror (err));
		return -1;
	}
	if ((err = snd_mixer_load (data.mixer)) < 0) {
		fprintf (stderr, "snd_mixer_load: %s\n", snd_strerror (err));
		return -1;
	}
	if (!(data.elem = snd_mixer_find_selem (data.mixer, data.sid))) {
		fprintf (stderr, "snd_mixer_find_selem (\"%s\", %i)\n",
			 snd_mixer_selem_id_get_name (data.sid),
			 snd_mixer_selem_id_get_index (data.sid));
		return -1;
	}
	snd_mixer_selem_get_playback_volume_range(data.elem, &data.vol_min, &data.vol_max);
	return i;
}
static int mixer_get_avg (int i)
{
  long val;

  snd_mixer_handle_events (data.mixer);
  snd_mixer_selem_get_playback_volume (data.elem, 0, &val);
  return (int) val;
}
static int mixer_get_left (int i)
{
  /* stub */
  return mixer_get_avg (i);
}
static int mixer_get_right (int i)
{
  /* stub */
  return mixer_get_avg (i);
}
int mixer_to_255(int i, int x)
{
  return (x-data.vol_min)*255/(data.vol_max-data.vol_min);
}
int mixer_is_mute(int i)
{
	snd_mixer_handle_events (data.mixer);
	if (snd_mixer_selem_has_playback_switch (data.elem)) {
		int val, err;
		if ((err = snd_mixer_selem_get_playback_switch(data.elem, 0, &val)) < 0)
			fprintf (stderr, "playback_switch: %s\n", snd_strerror (err));
		return !val;
	} else {
		return !mixer_get_avg(i);
	}
}
#undef data

#else /* MIXER_IS_ALSA */
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
#endif /* MIXER_IS_ALSA */

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
		scan_bar(obj, arg + n, 255);
	} else {
		obj->data.i = mixer_init(NULL);
		scan_bar(obj, arg, 255);
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
