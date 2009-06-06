/* Conky, a system monitor, based on torsmo
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

#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>

void llua_append_notify(const char *name);
void llua_rm_notifies(void);
static int llua_block_notify = 0;
#endif /* HAVE_SYS_INOTIFY_H */

lua_State *lua_L = NULL;

void llua_init(void)
{
	if(lua_L) return;
	lua_L = lua_open();
	luaL_openlibs(lua_L);
}

void llua_load(const char *script)
{
	int error;
	char path[DEFAULT_TEXT_BUFFER_SIZE];

	if(!lua_L) return;

	to_real_path(path, script);
	error = luaL_dofile(lua_L, path);
	if (error) {
		ERR("llua_load: %s", lua_tostring(lua_L, -1));
		lua_pop(lua_L, 1);
#ifdef HAVE_SYS_INOTIFY_H
	} else if (!llua_block_notify && inotify_fd) {
		llua_append_notify(script);
#endif /* HAVE_SYS_INOTIFY_H */
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

/*
 * same as llua_do_call() except passes everything after func as one arg.
 */
char *llua_do_read_call(const char *function, const char *arg, int retc)
{
	static char func[64];
	snprintf(func, 64, "conky_%s", function);
	
	/* push the function name to stack */
	lua_getglobal(lua_L, func);

	/* push function parameter to the stack */
	lua_pushstring(lua_L, arg);

	if (lua_pcall(lua_L, 1, retc, 0) != 0) {
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
			ret = strdup(lua_tostring(lua_L, -1));
			lua_pop(lua_L, 1);
		}
	}

	return ret;
}

char *llua_getstring_read(const char *function, const char *arg)
{
	char *func;
	char *ret = NULL;

	if(!lua_L) return NULL;

	func = llua_do_read_call(function, arg, 1);
	if (func) {
		if(!lua_isstring(lua_L, -1)) {
			ERR("llua_getstring_read: function %s didn't return a string, result discarded", func);
		} else {
			ret = strdup(lua_tostring(lua_L, -1));
			lua_pop(lua_L, 1);
		}
	}

	return ret;
}

int llua_getnumber(const char *args, double *ret)
{
	char *func;

	if(!lua_L) return 0;

	func = llua_do_call(args, 1);
	if(func) {
		if(!lua_isnumber(lua_L, -1)) {
			ERR("llua_getnumber: function %s didn't return a number, result discarded", func);
		} else {
			*ret = lua_tonumber(lua_L, -1);
			lua_pop(lua_L, 1);
			return 1;
		}
	}
	return 0;
}

void llua_close(void)
{
#ifdef HAVE_SYS_INOTIFY_H
	llua_rm_notifies();
#endif /* HAVE_SYS_INOTIFY_H */
	if(!lua_L) return;
	lua_close(lua_L);
	lua_L = NULL;
}

#ifdef HAVE_SYS_INOTIFY_H
struct _lua_notify_s {
	int wd;
	char name[DEFAULT_TEXT_BUFFER_SIZE];
	struct _lua_notify_s *next;
};
static struct _lua_notify_s *lua_notifies = 0;

static struct _lua_notify_s *llua_notify_list_do_alloc(const char *name)
{
	struct _lua_notify_s *ret = malloc(sizeof(struct _lua_notify_s));
	memset(ret, 0, sizeof(struct _lua_notify_s));
	strncpy(ret->name, name, DEFAULT_TEXT_BUFFER_SIZE);
	return ret;
}

void llua_append_notify(const char *name)
{
	/* do it */
	struct _lua_notify_s *new_tail = 0;
	if (!lua_notifies) {
		/* empty, fresh new digs */
		new_tail = lua_notifies = llua_notify_list_do_alloc(name);
	} else {
		struct _lua_notify_s *tail = lua_notifies;
		while (tail->next) {
			tail = tail->next;
		}
		// should be @ the end now
		new_tail = llua_notify_list_do_alloc(name);
		tail->next = new_tail;
	}
	new_tail->wd = inotify_add_watch(inotify_fd,
			new_tail->name,
			IN_MODIFY);
}

void llua_rm_notifies(void)
{
	/* git 'er done */
	struct _lua_notify_s *head = lua_notifies;
	struct _lua_notify_s *next = 0;
	if (!lua_notifies) return;
	inotify_rm_watch(inotify_fd, head->wd);
	if (head->next) next = head->next;
	free(head);
	while (next) {
		head = next;
		next = head->next;
		inotify_rm_watch(inotify_fd, head->wd);
		free(head);
	}
	lua_notifies = 0;
}

void llua_inotify_query(int wd, int mask)
{
	struct _lua_notify_s *head = lua_notifies;
	if (mask & IN_MODIFY || mask & IN_IGNORED) {
		/* for whatever reason, i keep getting IN_IGNORED when the file is
		 * modified */
		while (head) {
			if (head->wd == wd) {
				llua_block_notify = 1;
				llua_load(head->name);
				llua_block_notify = 0;
				ERR("Lua script '%s' reloaded", head->name);
				if (mask & IN_IGNORED) {
					/* for some reason we get IN_IGNORED here
					 * sometimes, so we need to re-add the watch */
					head->wd = inotify_add_watch(inotify_fd,
							head->name,
							IN_MODIFY);
				}
				return;
			}
			head = head->next;
		}
	}
}
#endif /* HAVE_SYS_INOTIFY_H */

