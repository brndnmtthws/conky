/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */

#include <sys/ioctl.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#include "conky.h"

#ifdef HAVE_LINUX_SOUNDCARD_H
#include <linux/soundcard.h>
#else
#ifdef __OpenBSD__
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif				/* __OpenBSD__ */
#endif				/* HAVE_LINUX_SOUNDCARD_H */

#define MIXER_DEV "/dev/mixer"

static int mixer_fd;
static const char *devs[] = SOUND_DEVICE_NAMES;

int mixer_init(const char *name)
{
	unsigned int i;

	if (name == 0 || name[0] == '\0')
		name = "vol";

	/* open mixer */
	if (mixer_fd <= 0) {
		mixer_fd = open(MIXER_DEV, O_RDONLY);
		if (mixer_fd == -1) {
			ERR("can't open %s: %s", MIXER_DEV,
			    strerror(errno));
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
		if (!rep)
			ERR("mixer ioctl: %s", strerror(errno));
		rep = 1;
		return 0;
	}
	rep = 0;

	return val;
}

int mixer_get_avg(int i)
{
	int v = mixer_get(i);
	return ((v >> 8) + (v & 0xFF)) / 2;
}

int mixer_get_left(int i)
{
	return mixer_get(i) >> 8;
}

int mixer_get_right(int i)
{
	return mixer_get(i) & 0xFF;
}
