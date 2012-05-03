/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 *
 * Conky, a system monitor, based on torsmo
 *
 * Copyright (c) 2009 Toni Spets
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

#ifndef LUA_H_
#define LUA_H_

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "config.h"

#ifdef X11
#include "x11.h"
#endif /* X11 */

#define LUAPREFIX "conky_"

/* load a lua script */
void llua_load(const char *script);
/* close lua stuff */
void llua_close(void);
#ifdef HAVE_SYS_INOTIFY_H
/* check our lua inotify status */
void llua_inotify_query(int wd, int mask);
#endif /* HAVE_SYS_INOTIFY_H */

void llua_set_startup_hook(const char *args);
void llua_set_shutdown_hook(const char *args);

void llua_startup_hook(void);
void llua_shutdown_hook(void);

#ifdef X11
void llua_draw_pre_hook(void);
void llua_draw_post_hook(void);

void llua_set_draw_pre_hook(const char *args);
void llua_set_draw_post_hook(const char *args);

void llua_setup_window_table(int text_start_x, int text_start_y, int text_width, int text_height);
void llua_update_window_table(int text_start_x, int text_start_y, int text_width, int text_height);
#endif /* X11 */

void llua_setup_info(struct information *i, double u_interval);
void llua_update_info(struct information *i, double u_interval);

void print_lua(struct text_object *, char *, int);
void print_lua_parse(struct text_object *, char *, int);
void print_lua_bar(struct text_object *, char *, int);
#ifdef X11
void print_lua_graph(struct text_object *, char *, int);
#endif /* X11 */
void print_lua_gauge(struct text_object *, char *, int);

#endif /* LUA_H_*/
