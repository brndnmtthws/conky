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
#include <lua.h>
}

enum mouse_event_t {
  MOUSE_PRESS = 0,
  MOUSE_RELEASE = 1,
  MOUSE_SCROLL = 2,
  MOUSE_MOVE = 3,
  AREA_ENTER = 4,
  AREA_LEAVE = 5,
  MOUSE_EVENT_COUNT = 6,
};

#ifdef __linux
#include <linux/input-event-codes.h>
#elif __FreeBSD__
#include <dev/evdev/input-event-codes.h>
#elif __DragonFly__
#include <dev/misc/evdev/input-event-codes.h>
#else
// Probably incorrect for some platforms, feel free to add your platform to the
// above list if it has other event codes or a standard file containing them.

// Left mouse button event code
#define BTN_LEFT    0x110
// Right mouse button event code
#define BTN_RIGHT   0x111
// Middle mouse button event code
#define BTN_MIDDLE  0x112
#endif

enum mouse_button_t: uint32_t {
  BUTTON_LEFT = BTN_LEFT,
  BUTTON_RIGHT = BTN_RIGHT,
  BUTTON_MIDDLE = BTN_MIDDLE,
};

struct mouse_event {
  mouse_event_t type; // type of event
  size_t time; // ms since epoch when the event happened

  explicit mouse_event(mouse_event_t type);

  void push_lua_table(lua_State *L) const;

  virtual void push_lua_data(lua_State *L) const = 0;
};

struct mouse_positioned_event : public mouse_event {
  size_t x = 0, y = 0;          // position relative to window
  size_t x_abs = 0, y_abs = 0;  // position relative to root

  mouse_positioned_event(mouse_event_t type, size_t x, size_t y, size_t x_abs, size_t y_abs): mouse_event(type), x(x), y(y), x_abs(x_abs), y_abs() {};
  
  void push_lua_data(lua_State *L) const;
};

struct mouse_move_event : public mouse_positioned_event {
  std::bitset<13> mods;  // held buttons and modifiers (ctrl, shift, ...)

  mouse_move_event(size_t x, size_t y, size_t x_abs, size_t y_abs, std::bitset<13> mods = 0): mouse_positioned_event{mouse_event_t::MOUSE_MOVE, x, y, x_abs, y_abs}, mods(mods) {};

  void push_lua_data(lua_State *L) const;
};

enum scroll_direction_t: uint8_t {
  SCROLL_UP = 0,
  SCROLL_DOWN,
  SCROLL_LEFT,
  SCROLL_RIGHT,
};

struct mouse_scroll_event : public mouse_positioned_event {
  std::bitset<13> mods;  // held buttons and modifiers (ctrl, shift, ...)
  scroll_direction_t direction;

  mouse_scroll_event(size_t x, size_t y, size_t x_abs, size_t y_abs, scroll_direction_t direction, std::bitset<13> mods = 0): mouse_positioned_event{mouse_event_t::MOUSE_SCROLL, x, y, x_abs, y_abs}, direction(direction), mods(mods) {};

  void push_lua_data(lua_State *L) const;
};

struct mouse_button_event : public mouse_positioned_event {
  std::bitset<13> mods;  // held buttons and modifiers (ctrl, shift, ...)
  mouse_button_t button;

  mouse_button_event(mouse_event_t type, size_t x, size_t y, size_t x_abs, size_t y_abs, mouse_button_t button, std::bitset<13> mods = 0): mouse_positioned_event{type, x, y, x_abs, y_abs}, button(button), mods(mods) {};

  void push_lua_data(lua_State *L) const;
};

struct mouse_crossing_event : public mouse_positioned_event {
  mouse_crossing_event(mouse_event_t type, size_t x, size_t y, size_t x_abs, size_t y_abs): mouse_positioned_event{type, x, y, x_abs, y_abs} {};
};

#endif /* MOUSE_EVENTS_H */
