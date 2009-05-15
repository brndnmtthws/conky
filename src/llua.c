/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2007 Toni Spets
 * Copyright (c) 2005-2009 Brenden Matthews, Philip Kovacs, et. al.
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

lua_State *lua_L = NULL;

void llua_init()
{
	if(lua_L) return;
	lua_L = lua_open();
	luaL_openlibs(lua_L);
}

void llua_load(const char *script)
{
	int error;
	error = luaL_loadfile(lua_L, script);
	if(error) {
		fprintf(stderr, "llua_load: %s\n", lua_tostring(lua_L, -1));
		lua_pop(lua_L, 1);
	} else {
		lua_pcall(lua_L, 0, 0, 0);
	}
}

char *llua_getstring(const char *args)
{
	char *ret = NULL;
	char *tmp = strdup(args);
	char func[64];
	int parcount = 0;

	if(!lua_L) return NULL;

	char *ptr = strtok(tmp, " ");
	if(!ptr) return NULL; /* function name missing */
	snprintf(func, 64, "conky_%s", ptr);

	lua_getglobal(lua_L, func);

	ptr = strtok(NULL, " ");
	while(ptr) {
		lua_pushstring(lua_L, ptr);
		ptr = strtok(NULL, " ");
		parcount++;
	}

	if(lua_pcall(lua_L, parcount, 1, 0) != 0) {
		fprintf(stderr, "llua: function %s execution failed: %s\n", func, lua_tostring(lua_L, -1));
		lua_pop(lua_L, -1);
	} else {
		if(!lua_isstring(lua_L, -1)) {
			fprintf(stderr, "llua: function %s didn't return a string, result discarded\n", func);
		} else {
			ret = strdup((char *)lua_tostring(lua_L, -1));
			lua_pop(lua_L, 1);
		}
	}

	free(tmp);

	return ret;
}

int llua_getpercent(const char *args, int *per)
{
	char func[64];
	char *tmp = strdup(args);
	int parcount = 0;

	if(!lua_L) return 0;

	char *ptr = strtok(tmp, " ");
	if(!ptr) return 0; /* function name missing */
	snprintf(func, 64, "conky_%s", ptr);

	lua_getglobal(lua_L, func);

	ptr = strtok(NULL, " ");
	while(ptr) {
		lua_pushstring(lua_L, ptr);
		ptr = strtok(NULL, " ");
		parcount++;
	}
	free(tmp);

	if(lua_pcall(lua_L, parcount, 1, 0) != 0) {
		fprintf(stderr, "llua: function %s execution failed: %s\n", func, lua_tostring(lua_L, -1));
		lua_pop(lua_L, -1);
	} else {
		if(!lua_isnumber(lua_L, -1)) {
			fprintf(stderr, "llua: function %s didn't return a number (percent), result discarded\n", func);
		} else {
			*per = lua_tonumber(lua_L, -1);
			lua_pop(lua_L, 1);
			return 1;
		}
	}
	return 0;
}

void llua_close()
{
	if(!lua_L) return;
	lua_close(lua_L);
	lua_L = NULL;
}
