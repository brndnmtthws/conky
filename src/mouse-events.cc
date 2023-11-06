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
#include <lua.h>
#include <time.h>

#ifdef BUILD_X11
#include "X11/Xlib.h"
#endif /* BUILD_X11 */

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

// Returns ms since Epoch.
inline size_t current_time_ms() {
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return static_cast<size_t>(static_cast<uint64_t>(spec.tv_sec)*1'000 + spec.tv_nsec/1'000'000);
}

void push_table_value(lua_State *L, std::string key, mouse_event_t type) {
  lua_pushstring(L, key.c_str());
  switch (type) {
    case MOUSE_PRESS:
      lua_pushstring(L, "button_down");
      break;
    case MOUSE_RELEASE:
      lua_pushstring(L, "button_up");
      break;
    case MOUSE_SCROLL:
      lua_pushstring(L, "mouse_scroll");
      break;
    case MOUSE_MOVE:
      lua_pushstring(L, "mouse_move");
      break;
    case AREA_ENTER:
      lua_pushstring(L, "mouse_enter");
      break;
    case AREA_LEAVE:
      lua_pushstring(L, "mouse_leave");
      break;
    default:
      lua_pushnil(L);
      break;
  }
  lua_settable(L, -3);
}

void push_table_value(lua_State *L, std::string key, scroll_direction_t direction) {
  lua_pushstring(L, key.c_str());
  switch (direction) {
    case SCROLL_DOWN:
      lua_pushstring(L, "down");
      break;
    case SCROLL_UP:
      lua_pushstring(L, "up");
      break;
    case SCROLL_LEFT:
      lua_pushstring(L, "left");
      break;
    case SCROLL_RIGHT:
      lua_pushstring(L, "right");
      break;
    default:
      lua_pushnil(L);
      break;
  }
  lua_settable(L, -3);
}


void push_table_value(lua_State *L, std::string key, mouse_button_t button) {
  lua_pushstring(L, key.c_str());
  switch (button) {
    case BUTTON_LEFT:
      lua_pushstring(L, "left");
      break;
    case BUTTON_RIGHT:
      lua_pushstring(L, "right");
      break;
    case BUTTON_MIDDLE:
      lua_pushstring(L, "middle");
      break;
    default:
      lua_pushnil(L);
      break;
  }
  lua_settable(L, -3);
}

/* Class methods */
mouse_event::mouse_event(mouse_event_t type): type(type), time(current_time_ms()) {};

void mouse_event::push_lua_table(lua_State *L) const {
  lua_newtable(L);
  push_table_value(L, "type", this->type);
  push_table_value(L, "time", this->time);
  push_lua_data(L);
}

void mouse_positioned_event::push_lua_data(lua_State *L) const {
  push_table_value(L, "x", this->x);
  push_table_value(L, "y", this->y);
  push_table_value(L, "x_abs", this->x_abs);
  push_table_value(L, "y_abs", this->y_abs);
}

#ifdef BUILD_X11
mouse_move_event::mouse_move_event(XMotionEvent *ev) {
  this->type = MOUSE_MOVE;
  this->x = ev->x;
  this->y = ev->y;
  this->x_abs = ev->x_root;
  this->y_abs = ev->y_root;
}
#endif /* BUILD_X11 */

void mouse_move_event::push_lua_data(lua_State *L) const {
  mouse_positioned_event::push_lua_data(L);
  push_mods(L, this->mods);
}

#ifdef BUILD_X11
mouse_scroll_event::mouse_scroll_event(XButtonEvent *ev) {
  this->type = MOUSE_SCROLL;
  this->x = ev->x;
  this->y = ev->y;
  this->x_abs = ev->x_root;
  this->y_abs = ev->y_root;
  this->mods = ev->state;
  if (ev->button == 4) {
    this->direction = scroll_direction_t::UP;
  } else if (ev->button == 5) {
    this->direction = scroll_direction_t::DOWN;
  }
}
#endif /* BUILD_X11 */

void mouse_scroll_event::push_lua_data(lua_State *L) const {
  mouse_positioned_event::push_lua_data(L);
  push_table_value(L, "direction", this->direction);
  push_mods(L, this->mods);
}

#ifdef BUILD_X11
mouse_button_event::mouse_button_event(XButtonEvent *ev) {
  this->type = ev->type == ButtonPress ? MOUSE_PRESS : MOUSE_RELEASE;
  this->x = ev->x;
  this->y = ev->y;
  this->x_abs = ev->x_root;
  this->y_abs = ev->y_root;
  this->mods = ev->state;
  switch (ev->button) {
    case Button1:
      this->button = BUTTON_LEFT;
      break;
    case Button2:
      this->button = BUTTON_RIGHT;
      break;
    case Button3:
      this->button = BUTTON_MIDDLE;
      break;
  }
}
#endif /* BUILD_X11 */

void mouse_button_event::push_lua_data(lua_State *L) const {
  mouse_positioned_event::push_lua_data(L);
  push_table_value(L, "button_code", static_cast<uint32_t>(this->button));
  push_table_value(L, "button", this->button);
  push_mods(L, this->mods);
}

#ifdef BUILD_X11
mouse_crossing_event::mouse_crossing_event(XCrossingEvent *ev) {
  this->type = ev->type == EnterNotify ? AREA_ENTER : AREA_LEAVE;
  this->x = ev->x;
  this->y = ev->y;
  this->x_abs = ev->x_root;
  this->y_abs = ev->y_root;
}
#endif /* BUILD_X11 */
