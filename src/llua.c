/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2009 Toni Spets
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
#include "logging.h"

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
	if(!lua_L) return;
	error = luaL_loadfile(lua_L, script);
	if(error) {
		ERR("llua_load: %s", lua_tostring(lua_L, -1));
		lua_pop(lua_L, 1);
	} else {
		lua_pcall(lua_L, 0, 0, 0);
	}
}

/*
	llua_do_call does a flexible call to any Lua function
	string: <function> [par1] [par2...]
	retc: the number of return values expected
*/
char *llua_do_call(const char *string, int retc)
{
	static char func[64];
	int argc = 0;

	char *tmp = strdup(string);
	char *ptr = strtok(tmp, " ");

	/* proceed only if the function name is present */
	if(!ptr) {
		free(tmp);
		return NULL;
	}

	/* call only conky_ prefixed functions */
	snprintf(func, 64, "conky_%s", ptr);

	/* push the function name to stack */
	lua_getglobal(lua_L, func);

	/* parse all function parameters from args and push them to the stack */
	ptr = strtok(NULL, " ");
	while(ptr) {
		lua_pushstring(lua_L, ptr);
		ptr = strtok(NULL, " ");
		argc++;
	}

	free(tmp);

	if(lua_pcall(lua_L, argc, retc, 0) != 0) {
		ERR("llua_do_call: function %s execution failed: %s", func, lua_tostring(lua_L, -1));
		lua_pop(lua_L, -1);
		return NULL;
	}

	return func;
}

char *llua_getstring(const char *args)
{
	char *func;
	char *ret = NULL;

	if(!lua_L) return NULL;

	func = llua_do_call(args, 1);
	if(func) {
		if(!lua_isstring(lua_L, -1)) {
			ERR("llua_getstring: function %s didn't return a string, result discarded", func);
		} else {
			ret = strdup((char *)lua_tostring(lua_L, -1));
			lua_pop(lua_L, 1);
		}
	}

	return ret;
}

int llua_getpercent(const char *args, int *per)
{
	char *func;

	if(!lua_L) return 0;

	func = llua_do_call(args, 1);
	if(func) {
		if(!lua_isnumber(lua_L, -1)) {
			ERR("llua_getpercent: function %s didn't return a number (percent), result discarded", func);
		} else {
			*per = lua_tointeger(lua_L, -1);
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
