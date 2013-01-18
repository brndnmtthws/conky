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

#include <jack/jack.h>

#if HAVE_SEMAPHORE_H
#include <semaphore.h>
static sem_t zombified;
#else
static int zzz = 0;
static int* zombified = 0;
#endif

static jack_client_t* client = 0;

void jack_shutdown_cb(void* arg)
{
	/* this is called from JACK's thread */
	struct jack_s* jackdata = (struct jack_s*)arg;

	#if HAVE_SEMAPHORE_H
	int z = 0;
	if (sem_getvalue(&zombified, &z) == 0 && z == 0) {
		sem_post (&zombified);
	}
	#else
	zombified = &zzz;
	#endif
}

int	jack_buffer_size_cb(jack_nframes_t nframes, void* arg)
{
	struct jack_s* jackdata = (struct jack_s*)arg;
	/* XXX fixme: does this need to be made thread safe or not? */
	jackdata->buffer_size = nframes;
	return 0;
}

int jack_sample_rate_cb(jack_nframes_t nframes, void* arg)
{
	struct jack_s* jackdata = (struct jack_s*)arg;
	/* XXX fixme: does this need to be made thread safe or not? */
	jackdata->sample_rate = nframes;
	return 0;
}

int	jack_xrun_cb(void* arg)
{
	struct jack_s* jackdata = (struct jack_s*)arg;
	/* XXX fixme: does this need to be made thread safe or not? */
	jackdata->xruns++;
	return 0;
}

static int connect_jack(struct jack_s* jackdata)
{
	jack_status_t status;

	if ((client = jack_client_open(PACKAGE_NAME, JackNoStartServer, &status)) == 0) {
		return -1;
	}

	printf("Registered JACK client '%s'\n", jack_get_client_name(client));

	#if HAVE_SEMAPHORE_H
	sem_init(&zombified, 0, 0);
	#else
	zombified = 0;
	#endif
	jackdata->buffer_size = jack_get_buffer_size(client);
	jackdata->sample_rate = jack_get_sample_rate(client);
	jack_on_shutdown(client, jack_shutdown_cb, jackdata);
	jack_set_buffer_size_callback(client, jack_buffer_size_cb, jackdata);
	jack_set_sample_rate_callback(client, jack_sample_rate_cb, jackdata);
	jack_set_xrun_callback(client, jack_xrun_cb, jackdata);
}

void init_jack(void)
{
	struct information *current_info = &info;
	struct jack_s* jackdata = &current_info->jack;
	jackdata->active = 0;
	printf("jack initialized ;-)\n");
}

int update_jack(void)
{
	struct information *current_info = &info;
	struct jack_s* jackdata = &current_info->jack;

	if (!jackdata->active) {
		jackdata->active = 0;
		jackdata->cpu_load = 0;
		jackdata->buffer_size = 0;
		jackdata->sample_rate = 0;
		jackdata->xruns = 0;

		if (connect_jack(jackdata) == 0) {
			if (jack_activate(client) == 0) {
				printf("activated jack client\n");
				jackdata->active = 1;
			}
		}
	}

	if (jackdata->active) {
		#if HAVE_SEMAPHORE_H
		int z = 0;
		#endif
		jackdata->cpu_load = jack_cpu_load(client);

		#if HAVE_SEMAPHORE_H
		if (sem_getvalue (&zombified, &z) == 0 && z > 0) {
			sem_destroy(&zombified);
		#else
		if (zombified == zzz) {
		#endif
			jackdata->active = 0;
			client = 0;
		}
	}
	return 0;
}

void jack_close(void)
{
	if (client) {
		if (jack_client_close(client) == 0) {
		}
		else {
		}
		client = 0;
	}
}

