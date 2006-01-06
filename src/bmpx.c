/** bmpx.c
 * BMPx client
 *
 * $Id$
 */

#include <dbus/dbus-glib.h>
#include <bmpx/dbus.h>
#include <stdio.h>
#include <string.h>

#include "conky.h"

#define DBUS_TYPE_G_STRING_VALUE_HASHTABLE (dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))

static DBusGConnection *bus;
static DBusGProxy *remote_object;
static int connected = 0;
static char *unknown = "unknown";

void update_bmpx()
{
	GError *error = NULL;
	struct information *current_info = &info;
	gchar *uri;
	GHashTable *metadata;
	
	if (connected == 0) {
		g_type_init();
		dbus_g_type_specialized_init();
		
		bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
		if (bus == NULL) {
			ERR("BMPx error 1: %s\n", error->message);
			goto fail;
		}
		
		remote_object = dbus_g_proxy_new_for_name(bus,
			BMP_DBUS_SERVICE,
			BMP_DBUS_PATH_SYSTEMCONTROL,
			BMP_DBUS_INTERFACE);
		if (!remote_object) {
			ERR("BMPx error 2: %s\n", error->message);
			goto fail;
		} 

		connected = 1;
	} 
	
	if (connected == 1) {
		if (dbus_g_proxy_call(remote_object, "GetCurrentUri", &error,
					G_TYPE_INVALID,
					G_TYPE_STRING, &uri, G_TYPE_INVALID)) {
			current_info->bmpx.uri = uri;
		} else {
			ERR("BMPx error 3: %s\n", error->message);
			goto fail;
		}
	
		if (dbus_g_proxy_call(remote_object, "GetMetadataForUri", &error,
				G_TYPE_STRING,
				uri,
				G_TYPE_INVALID,
				DBUS_TYPE_G_STRING_VALUE_HASHTABLE,
				&metadata,
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
			current_info->bmpx.title = g_value_dup_string(g_hash_table_lookup(metadata, "title"));
			current_info->bmpx.artist = g_value_dup_string(g_hash_table_lookup(metadata, "artist"));
			current_info->bmpx.album = g_value_dup_string(g_hash_table_lookup(metadata, "album"));
			current_info->bmpx.bitrate = g_value_get_int(g_hash_table_lookup(metadata, "bitrate"));
			current_info->bmpx.track = g_value_get_int(g_hash_table_lookup(metadata, "track-number"));
		} else {
			ERR("BMPx error 4: %s\n", error->message);
			goto fail;
		}
		
		if (uri)
			free(uri);
		g_hash_table_destroy(metadata);
	} else {
fail: 
		if (error)
			g_error_free(error);
		current_info->bmpx.title = unknown;
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
		current_info->bmpx.artist = unknown;
		current_info->bmpx.album = unknown;
		current_info->bmpx.bitrate = 0;
		current_info->bmpx.track = 0;
	}
}
