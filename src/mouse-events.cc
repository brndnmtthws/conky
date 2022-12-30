/*
 * mouse_events.cc: conky support for mouse events
 *
 * Copyright (C) 2020 Tin Svagelj tin.svagelj@live.com
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "mouse-events.h"

#include <array>
#include <string>
#include "X11/Xlib.h"

std::string event_type_to_str(int type) {
  switch (type) {
    case MOUSE_DOWN:
      return "button_down";
    case MOUSE_UP:
      return "button_up";
    case MOUSE_SCROLL:
      return "mouse_scroll";
    case MOUSE_MOVE:
      return "mouse_move";
    case AREA_ENTER:
      return "mouse_enter";
    case AREA_LEAVE:
      return "mouse_leave";
    default:
      return "err";
  }
}

/* Lua helper functions */
template <typename T>
void push_table_value(lua_State *L, std::string key, T value);

void push_table_value(lua_State *L, std::string key, std::string value) {
  lua_pushstring(L, key.c_str());
  lua_pushstring(L, value.c_str());
  lua_settable(L, -3);
}

void push_table_value(lua_State *L, std::string key, int value) {
  lua_pushstring(L, key.c_str());
  lua_pushinteger(L, value);
  lua_settable(L, -3);
}

void push_table_value(lua_State *L, std::string key, uint value) {
  lua_pushstring(L, key.c_str());
  lua_pushinteger(L, value);
  lua_settable(L, -3);
}

void push_table_value(lua_State *L, std::string key, uint64_t value) {
  lua_pushstring(L, key.c_str());
  lua_pushinteger(L, value);
  lua_settable(L, -3);
}

void push_table_value(lua_State *L, std::string key, bool value) {
  lua_pushstring(L, key.c_str());
  lua_pushboolean(L, value);
  lua_settable(L, -3);
}

void push_table_value(lua_State *L, std::string key, float value) {
  lua_pushstring(L, key.c_str());
  lua_pushnumber(L, value);
  lua_settable(L, -3);
}

void push_table_value(lua_State *L, std::string key, double value) {
  lua_pushstring(L, key.c_str());
  lua_pushnumber(L, value);
  lua_settable(L, -3);
}

template <size_t N>
void push_bitset(lua_State *L, std::bitset<N> it,
                 std::array<std::string, N> labels) {
  lua_newtable(L);
  for (size_t i = 0; i < N; i++) push_table_value(L, labels[i], it.test(i));
}

const std::array<std::string, 13> mod_names = {
    {"shift", "lock", "control", "mod1", "num_lock", "mod3", "mod4", "mod5",
     "mouse_left", "mouse_right", "mouse_middle", "scroll_up", "scroll_down"}};

void push_mods(lua_State *L, std::bitset<13> mods) {
  lua_pushstring(L, "mods");
  push_bitset(L, mods, mod_names);
  lua_settable(L, -3);
}

/* Class methods */

void mouse_event::push_lua_table(lua_State *L) const {
  lua_newtable(L);
  push_table_value(L, "type", event_type_to_str(this->type));
  push_lua_data(L);
}

void mouse_positioned_event::push_lua_data(lua_State *L) const {
  push_table_value(L, "x", this->x);
  push_table_value(L, "y", this->y);
  push_table_value(L, "x_abs", this->x_abs);
  push_table_value(L, "y_abs", this->y_abs);
  push_table_value(L, "time", this->time);
}

mouse_move_event::mouse_move_event(XMotionEvent *ev) {
  this->type = MOUSE_MOVE;
  this->x = ev->x;
  this->y = ev->y;
  this->x_abs = ev->x_root;
  this->y_abs = ev->y_root;
  this->time = ev->time;
}

void mouse_move_event::push_lua_data(lua_State *L) const {
  mouse_positioned_event::push_lua_data(L);
  push_mods(L, this->mods);
}

mouse_scroll_event::mouse_scroll_event(XButtonEvent *ev) {
  this->type = MOUSE_SCROLL;
  this->x = ev->x;
  this->y = ev->y;
  this->x_abs = ev->x_root;
  this->y_abs = ev->y_root;
  this->time = ev->time;
  this->mods = ev->state;
  this->up = ev->button == 4;
}

void mouse_scroll_event::push_lua_data(lua_State *L) const {
  mouse_positioned_event::push_lua_data(L);
  push_table_value(L, "direction", std::string(this->up ? "up" : "down"));
  push_mods(L, this->mods);
}

mouse_button_event::mouse_button_event(XButtonEvent *ev) {
  this->type = ev->type == ButtonPress ? MOUSE_DOWN : MOUSE_UP;
  this->x = ev->x;
  this->y = ev->y;
  this->x_abs = ev->x_root;
  this->y_abs = ev->y_root;
  this->time = ev->time;
  this->mods = ev->state;
  this->button = ev->button;
}

void mouse_button_event::push_lua_data(lua_State *L) const {
  mouse_positioned_event::push_lua_data(L);
  push_table_value(L, "button", this->button);
  push_mods(L, this->mods);
}

mouse_crossing_event::mouse_crossing_event(XCrossingEvent *ev) {
  this->type = ev->type == EnterNotify ? AREA_ENTER : AREA_LEAVE;
  this->x = ev->x;
  this->y = ev->y;
  this->x_abs = ev->x_root;
  this->y_abs = ev->y_root;
  this->time = ev->time;
}
