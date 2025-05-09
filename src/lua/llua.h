/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Copyright (c) 2009 Toni Spets
 * Copyright (c) 2005-2024 Brenden Matthews, Philip Kovacs, et. al.
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

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include <config.h>
#include "../geometry.h"

#ifdef BUILD_MOUSE_EVENTS
#include "../mouse-events.h"
#endif /* BUILD_MOUSE_EVENTS */

#define LUAPREFIX "conky_"

#ifdef HAVE_SYS_INOTIFY_H
/* check our lua inotify status */
void llua_inotify_query(int wd, int mask);
#endif /* HAVE_SYS_INOTIFY_H */

void llua_init();
void llua_startup_hook(void);
void llua_shutdown_hook(void);

#ifdef BUILD_GUI
void llua_draw_pre_hook(void);
void llua_draw_post_hook(void);

#ifdef BUILD_MOUSE_EVENTS
/**
  Takes a mouse_event as argument.
  Returns true if event was properly consumed, false otherwise.
 */
template <typename EventT>
bool llua_mouse_hook(const EventT &ev);
#endif /* BUILD_MOUSE_EVENTS */

void llua_setup_window_table(conky::rect<int> text_rect);
void llua_update_window_table(conky::rect<int> text_rect);
#endif /* BUILD_GUI */

void llua_setup_info(struct information *i, double u_interval);
void llua_update_info(struct information *i, double u_interval);

void print_lua(struct text_object *, char *, unsigned int);
void print_lua_parse(struct text_object *, char *, unsigned int);
double lua_barval(struct text_object *);

#endif /* LUA_H_*/
