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

#include "config.h"
#include "conky.h"
#include "logging.h"

#include <bmp/dbus.hh>
#include <dbus/dbus-glib.h>

#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE \
	(dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))

static DBusGConnection *bus;
static DBusGProxy *remote_object;
static int connected = 0;
static char *unknown = "unknown";

void fail(GError *error, struct information *);

void update_bmpx()
{
	GError *error = NULL;
	struct information *current_info = &info;
	gint current_track;
	GHashTable *metadata;

	if (connected == 0) {
		g_type_init();
		dbus_g_type_specialized_init();

		bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
		if (bus == NULL) {
			NORM_ERR("BMPx error 1: %s\n", error->message);
			fail(error, current_info);
			return;
		}

		remote_object = dbus_g_proxy_new_for_name(bus, BMP_DBUS_SERVICE,
				BMP_DBUS_PATH__BMP, BMP_DBUS_INTERFACE__BMP);
		if (!remote_object) {
			NORM_ERR("BMPx error 2: %s\n", error->message);
			fail(error, current_info);
			return;
		}

		connected = 1;
	}

	if (connected == 1) {
		if (dbus_g_proxy_call(remote_object, "GetCurrentTrack", &error,
					G_TYPE_INVALID, G_TYPE_INT, &current_track, G_TYPE_INVALID)) {
		} else {
			NORM_ERR("BMPx error 3: %s\n", error->message);
			fail(error, current_info);
			return;
		}

		if (dbus_g_proxy_call(remote_object, "GetMetadataForListItem", &error,
					G_TYPE_INT, current_track, G_TYPE_INVALID,
					DBUS_TYPE_G_STRING_VALUE_HASHTABLE, &metadata,
					G_TYPE_INVALID)) {
			if (current_info->bmpx.title) {
				free(current_info->bmpx.title);
				current_info->bmpx.title = 0;
			}
			if (current_info->bmpx.artist) {
				free(current_info->bmpx.artist);
				current_info->bmpx.artist = 0;
			}
			if (current_info->bmpx.album) {
				free(current_info->bmpx.album);
				current_info->bmpx.album = 0;
			}
			current_info->bmpx.title =
				g_value_dup_string(g_hash_table_lookup(metadata, "title"));
			current_info->bmpx.artist =
				g_value_dup_string(g_hash_table_lookup(metadata, "artist"));
			current_info->bmpx.album =
				g_value_dup_string(g_hash_table_lookup(metadata, "album"));
			current_info->bmpx.bitrate =
				g_value_get_int(g_hash_table_lookup(metadata, "bitrate"));
			current_info->bmpx.track =
				g_value_get_int(g_hash_table_lookup(metadata, "track-number"));
			current_info->bmpx.uri =
				g_value_get_string(g_hash_table_lookup(metadata, "location"));
		} else {
			NORM_ERR("BMPx error 4: %s\n", error->message);
			fail(error, current_info);
			return;
		}

		g_hash_table_destroy(metadata);
	} else {
		fail(error, current_info);
	}
}

void fail(GError *error, struct information *current_info)
{
	if (error) {
		g_error_free(error);
	}
	if (current_info->bmpx.title) {
		g_free(current_info->bmpx.title);
		current_info->bmpx.title = 0;
	}
	if (current_info->bmpx.artist) {
		g_free(current_info->bmpx.artist);
		current_info->bmpx.artist = 0;
	}
	if (current_info->bmpx.album) {
		g_free(current_info->bmpx.album);
		current_info->bmpx.album = 0;
	}
	current_info->bmpx.title = unknown;
	current_info->bmpx.artist = unknown;
	current_info->bmpx.album = unknown;
	current_info->bmpx.bitrate = 0;
	current_info->bmpx.track = 0;
}
