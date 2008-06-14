/* smapi.c:  conky support for IBM Thinkpad smapi
 *
 * Copyright (C) 2007 Phil Sutter <Phil@nwl.cc>
 * 
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 * $Id$
 *
 */

#include "smapi.h"

#define SYS_SMAPI_PATH "/sys/devices/platform/smapi"

int smapi_bat_installed(int idx)
{
	char path[128];
	struct stat sb;
	int ret = 0;

	snprintf(path, 127, SYS_SMAPI_PATH "/BAT%i", idx);
	if (!stat(path, &sb) && (sb.st_mode & S_IFMT) == S_IFDIR) {
		snprintf(path, 127, SYS_SMAPI_PATH "/BAT%i/installed", idx);
		ret = (smapi_read_int(path) == 1) ? 1 : 0;
	}
	return ret;

}

char *smapi_read_str(const char *path)
{
	FILE *fp;
	char str[256] = "failed";
	if ((fp = fopen(path, "r")) != NULL) {
		fscanf(fp, "%255s\n", str);
		fclose(fp);
	}
	return strndup(str, text_buffer_size);
}

int smapi_read_int(const char *path)
{
	FILE *fp;
	int i = 0;
	if ((fp = fopen(path, "r")) != NULL) {
		fscanf(fp, "%i\n", &i);
		fclose(fp);
	}
	return i;
}

char *smapi_get_str(const char *fname)
{
	char path[128];
	if(snprintf(path, 127, SYS_SMAPI_PATH "/%s", fname) < 0)
		return NULL;

	return smapi_read_str(path);
}

char *smapi_get_bat_str(int idx, const char *fname)
{
	char path[128];
	if(snprintf(path, 127, SYS_SMAPI_PATH "/BAT%i/%s", idx, fname) < 0)
		return NULL;
	return smapi_read_str(path);
}

int smapi_get_bat_int(int idx, const char *fname)
{
	char path[128];
	if(snprintf(path, 127, SYS_SMAPI_PATH "/BAT%i/%s", idx, fname) < 0)
		return 0;
	return smapi_read_int(path);
}

char *smapi_get_bat_val(const char *args)
{
	char fname[128];
	int idx, cnt;

	if(sscanf(args, "%i %n", &idx, &cnt) <= 0 ||
	   snprintf(fname, 127, "%s", (args + cnt)) < 0) {
		ERR("smapi: wrong arguments, should be 'bat,<int>,<str>'");
		return NULL;
	}

	if(!smapi_bat_installed(idx))
		return NULL;

	return smapi_get_bat_str(idx, fname);
}

char *smapi_get_val(const char *args)
{
	char str[128];

	if(!args || sscanf(args, "%127s", str) <= 0)
		return NULL;

	if(!strcmp(str, "bat"))
		return smapi_get_bat_val(args + 4);

	return smapi_get_str(str);
}
