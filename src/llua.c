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
#include "build.h"

#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>

void llua_append_notify(const char *name);
void llua_rm_notifies(void);
static int llua_block_notify = 0;
#endif /* HAVE_SYS_INOTIFY_H */

static char *draw_pre = 0;
static char *draw_post = 0;

lua_State *lua_L = NULL;

static int llua_conky_parse(lua_State *L)
{
	int n = lua_gettop(L);    /* number of arguments */
	char *str;
	char *buf = calloc(1, max_user_text);
	if (n != 1) {
		lua_pushstring(L, "incorrect arguments, conky_parse(string) takes exactly 1 argument");
		lua_error(L);
	}
	if (!lua_isstring(L, 1)) {
		lua_pushstring(L, "incorrect argument (expecting a string)");
		lua_error(L);
	}
	str = strdup(lua_tostring(L, 1));
	evaluate(str, buf);
	lua_pushstring(L, buf);
	free(str);
	free(buf);
	return 1;                 /* number of results */
}

void llua_init(void)
{
	const char *libs = PACKAGE_LIBDIR"/lib?.so;";
	char *old_path, *new_path;
	if (lua_L) return;
	lua_L = lua_open();

	/* add our library path to the lua package.cpath global var */
	luaL_openlibs(lua_L);
	lua_getglobal(lua_L, "package");
	lua_getfield(lua_L, -1, "cpath");
	old_path = strdup(lua_tostring(lua_L, -1));
	new_path = malloc(strlen(old_path) + strlen(libs) + 1);
	strcpy(new_path, libs);
	strcat(new_path, old_path);
	lua_pushstring(lua_L, new_path);
	lua_setfield(lua_L, -3, "cpath");
	lua_pop(lua_L, 2);
	free(old_path);
	free(new_path);

	lua_pushstring(lua_L, PACKAGE_NAME" "VERSION" compiled "BUILD_DATE" for "BUILD_ARCH);
	lua_setglobal(lua_L, "conky_build_info");

	lua_pushstring(lua_L, VERSION);
	lua_setglobal(lua_L, "conky_version");

	lua_pushstring(lua_L, BUILD_DATE);
	lua_setglobal(lua_L, "conky_build_date");

	lua_pushstring(lua_L, BUILD_ARCH);
	lua_setglobal(lua_L, "conky_build_arch");

	lua_pushstring(lua_L, current_config);
	lua_setglobal(lua_L, "conky_config");

	lua_pushcfunction(lua_L, &llua_conky_parse);
	lua_setglobal(lua_L, "conky_parse");
}

void llua_load(const char *script)
{
	int error;
	char path[DEFAULT_TEXT_BUFFER_SIZE];

	llua_init();

	to_real_path(path, script);
	error = luaL_dofile(lua_L, path);
	if (error) {
		ERR("llua_load: %s", lua_tostring(lua_L, -1));
		lua_pop(lua_L, 1);
#ifdef HAVE_SYS_INOTIFY_H
	} else if (!llua_block_notify && inotify_fd != -1) {
		llua_append_notify(path);
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
	if (!ptr) {
		free(tmp);
		return NULL;
	}

	/* call only conky_ prefixed functions */
	if(strncmp(ptr, LUAPREFIX, strlen(LUAPREFIX)) == 0) {
		snprintf(func, 64, "%s", ptr);
	}else{
		snprintf(func, 64, "%s%s", LUAPREFIX, ptr);
	}

	/* push the function name to stack */
	lua_getglobal(lua_L, func);

	/* parse all function parameters from args and push them to the stack */
	ptr = strtok(NULL, " ");
	while (ptr) {
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
	if (func) {
		if (!lua_isstring(lua_L, -1)) {
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
	if (draw_pre) {
		free(draw_pre);
		draw_pre = 0;
	}
	if (draw_post) {
		free(draw_post);
		draw_post = 0;
	}
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

#ifdef X11
void llua_draw_pre_hook(void)
{
	if (!lua_L || !draw_pre) return;
	llua_do_call(draw_pre, 0);
}

void llua_draw_post_hook(void)
{
	if (!lua_L || !draw_post) return;
	llua_do_call(draw_post, 0);
}

void llua_set_draw_pre_hook(const char *args)
{
	draw_pre = strdup(args);
}

void llua_set_draw_post_hook(const char *args)
{
	draw_post = strdup(args);
}

void llua_set_long(const char *key, long value)
{
	lua_pushnumber(lua_L, value);
	lua_setfield(lua_L, -2, key);
}

/* this function mostly copied from tolua++ source so that we could play nice
 * with tolua++ libs.  tolua++ is provided 'as is'
 */
void llua_set_userdata(const char *key, const char *type, void *value)
{
	if (value == NULL) {
		lua_pushnil(lua_L);
	} else {
		luaL_getmetatable(lua_L, type);
		lua_pushstring(lua_L,"tolua_ubox");
		lua_rawget(lua_L,-2);        /* stack: mt ubox */
		if (lua_isnil(lua_L, -1)) {
			lua_pop(lua_L, 1);
			lua_pushstring(lua_L, "tolua_ubox");
			lua_rawget(lua_L, LUA_REGISTRYINDEX);
		}
		lua_pushlightuserdata(lua_L,value);
		lua_rawget(lua_L,-2);                       /* stack: mt ubox ubox[u] */
		if (lua_isnil(lua_L,-1)) {
			lua_pop(lua_L,1);                          /* stack: mt ubox */
			lua_pushlightuserdata(lua_L,value);
			*(void**)lua_newuserdata(lua_L,sizeof(void *)) = value;   /* stack: mt ubox u newud */
			lua_pushvalue(lua_L,-1);                   /* stack: mt ubox u newud newud */
			lua_insert(lua_L,-4);                      /* stack: mt newud ubox u newud */
			lua_rawset(lua_L,-3);                      /* stack: mt newud ubox */
			lua_pop(lua_L,1);                          /* stack: mt newud */
			/*luaL_getmetatable(lua_L,type);*/
			lua_pushvalue(lua_L, -2);			/* stack: mt newud mt */
			lua_setmetatable(lua_L,-2);			/* stack: mt newud */

		} else {
			/* check the need of updating the metatable to a more specialized class */
			lua_insert(lua_L,-2);                       /* stack: mt ubox[u] ubox */
			lua_pop(lua_L,1);                           /* stack: mt ubox[u] */
			lua_pushstring(lua_L,"tolua_super");
			lua_rawget(lua_L,LUA_REGISTRYINDEX);        /* stack: mt ubox[u] super */
			lua_getmetatable(lua_L,-2);                 /* stack: mt ubox[u] super mt */
			lua_rawget(lua_L,-2);                       /* stack: mt ubox[u] super super[mt] */
			if (lua_istable(lua_L,-1)) {
				lua_pushstring(lua_L,type);                 /* stack: mt ubox[u] super super[mt] type */
				lua_rawget(lua_L,-2);                       /* stack: mt ubox[u] super super[mt] flag */
				if (lua_toboolean(lua_L,-1) == 1) {
					/* if true */
					lua_pop(lua_L,3);	/* mt ubox[u]*/
					lua_remove(lua_L, -2);
					return;
				}
			}
			/* type represents a more specilized type */
			/*luaL_getmetatable(lua_L,type);             // stack: mt ubox[u] super super[mt] flag mt */
			lua_pushvalue(lua_L, -5);					/* stack: mt ubox[u] super super[mt] flag mt */
			lua_setmetatable(lua_L,-5);                /* stack: mt ubox[u] super super[mt] flag */
			lua_pop(lua_L,3);                          /* stack: mt ubox[u] */
		}
		lua_remove(lua_L, -2);	/* stack: ubox[u]*/
	}
	lua_setfield(lua_L, -2, key);
}

void llua_setup_window_table(int text_start_x, int text_start_y, int text_width, int text_height)
{
	if (!lua_L) return;
	lua_newtable(lua_L);
	
	if (output_methods & TO_X) {
		llua_set_userdata("drawable", "Drawable", (void*)&window.drawable);
		llua_set_userdata("visual", "Visual", window.visual);
		llua_set_userdata("display", "Display", display);


		llua_set_long("width", window.width);
		llua_set_long("height", window.height);
		llua_set_long("border_inner_margin", window.border_inner_margin);
		llua_set_long("border_outer_margin", window.border_outer_margin);
		llua_set_long("border_width", window.border_width);

		llua_set_long("text_start_x", text_start_x);
		llua_set_long("text_start_y", text_start_y);
		llua_set_long("text_width", text_width);
		llua_set_long("text_height", text_height);

		lua_setglobal(lua_L, "conky_window");
	}
}

void llua_update_window_table(int text_start_x, int text_start_y, int text_width, int text_height)
{
	if (!lua_L) return;

	lua_getglobal(lua_L, "conky_window");
	if (lua_isnil(lua_L, -1)) {
		/* window table isn't populated yet */
		return;
	}

	llua_set_long("width", window.width);
	llua_set_long("height", window.height);

	llua_set_long("text_start_x", text_start_x);
	llua_set_long("text_start_y", text_start_y);
	llua_set_long("text_width", text_width);
	llua_set_long("text_height", text_height);

	lua_setglobal(lua_L, "conky_window");
}
#endif /* X11 */

