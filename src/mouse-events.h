/*
 * mouse_events.h: conky support for mouse events
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

#ifndef MOUSE_EVENTS_H
#define MOUSE_EVENTS_H

#include <bitset>
#include <cstdint>

extern "C" {
#ifdef BUILD_X11
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#include <X11/Xlib.h>
#pragma GCC diagnostic pop
#endif /* BUILD_X11 */
#include <lua.h>
}

enum mouse_event_type {
  MOUSE_DOWN = 0,
  MOUSE_UP = 1,
  MOUSE_SCROLL = 2,
  MOUSE_MOVE = 3,
  AREA_ENTER = 4,
  AREA_LEAVE = 5,
  MOUSE_EVENT_COUNT = 6,
};

struct mouse_event {
  mouse_event_type type;
  uint64_t time = 0L;  // event time

  mouse_event(mouse_event_type type, uint64_t time): type(type), time(time) {};

  void push_lua_table(lua_State *L) const;

  virtual void push_lua_data(lua_State *L) const = 0;
};

struct mouse_positioned_event : public mouse_event {
  size_t x = 0, y = 0;          // positions relative to window
  size_t x_abs = 0, y_abs = 0;  // positions relative to root

  mouse_positioned_event(mouse_event_type type, uint64_t time, size_t x, size_t y, size_t x_abs, size_t y_abs): mouse_event{type, time}, x(x), y(y), x_abs(x_abs), y_abs() {};
  
  void push_lua_data(lua_State *L) const;
};

struct mouse_move_event : public mouse_positioned_event {
  std::bitset<13> mods;  // held buttons and modifiers (ctrl, shift, ...)

  explicit mouse_move_event(mouse_event_type type, uint64_t time, size_t x, size_t y, size_t x_abs, size_t y_abs, std::bitset<13> mods = 0): mouse_positioned_event{type, time, x, y, x_abs, y_abs}, mods(mods) {};
  #ifdef BUILD_X11
  explicit mouse_move_event(XMotionEvent *ev);
  #endif /* BUILD_X11 */

  void push_lua_data(lua_State *L) const;
};

struct mouse_scroll_event : public mouse_positioned_event {
  std::bitset<13> mods;  // held buttons and modifiers (ctrl, shift, ...)
  bool up = false;

  #ifdef BUILD_X11
  explicit mouse_scroll_event(XButtonEvent *ev);
  #endif /* BUILD_X11 */

  void push_lua_data(lua_State *L) const;
};

struct mouse_button_event : public mouse_positioned_event {
  std::bitset<13> mods;  // held buttons and modifiers (ctrl, shift, ...)
  uint button = 0;

  #ifdef BUILD_X11
  explicit mouse_button_event(XButtonEvent *ev);
  #endif /* BUILD_X11 */

  void push_lua_data(lua_State *L) const;
};

typedef struct mouse_button_event mouse_press_event;
typedef struct mouse_button_event mouse_release_event;

struct mouse_crossing_event : public mouse_positioned_event {
  #ifdef BUILD_X11
  explicit mouse_crossing_event(XCrossingEvent *ev);
  #endif /* BUILD_X11 */
};

typedef struct mouse_crossing_event mouse_enter_event;
typedef struct mouse_crossing_event mouse_leave_event;

#endif /* MOUSE_EVENTS_H */
