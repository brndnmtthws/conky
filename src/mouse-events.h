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
#include <string>
#include <cstdint>

#include "config.h"
#include "logging.h"

#ifdef BUILD_X11
#include <X11/X.h>
#endif /* BUILD_X11 */

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
#define BTN_LEFT     0x110
// Right mouse button event code
#define BTN_RIGHT    0x111
// Middle mouse button event code
#define BTN_MIDDLE   0x112

// Back mouse button event code
#define BTN_BACK     0x116
// Forward mouse button event code
#define BTN_FORWARD  0x115
#endif

enum mouse_button_t: uint32_t {
  BUTTON_LEFT = BTN_LEFT,
  BUTTON_RIGHT = BTN_RIGHT,
  BUTTON_MIDDLE = BTN_MIDDLE,
  BUTTON_BACK = BTN_BACK,
  BUTTON_FORWARD = BTN_FORWARD,
};

#ifdef BUILD_X11
inline mouse_button_t x11_mouse_button_code(unsigned int x11_mouse_button) {
  mouse_button_t button;
  switch (x11_mouse_button) {
    case Button1:
      button = BUTTON_LEFT;
      break;
    case Button2:
      button = BUTTON_MIDDLE;
      break;
    case Button3:
      button = BUTTON_RIGHT;
      break;
    case 8:
      button = BUTTON_BACK;
      break;
    case 9:
      button = BUTTON_FORWARD;
      break;
    default:
      DBGP("X11 button %d is not mapped", x11_mouse_button);
      break;
  }
  return button;
}
#endif /* BUILD_X11 */

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

typedef std::bitset<6> modifier_state_t;
enum modifier_key: uint32_t {
  MOD_SHIFT = 0,
  MOD_CONTROL = 1,
  MOD_ALT = 2,
  // Windows/MacOS key on most keyboards
  MOD_SUPER = 3,
  MOD_CAPS_LOCK = 4,
  MOD_NUM_LOCK = 5,
};
std::string modifier_name(modifier_key key);

#ifdef BUILD_X11
inline modifier_state_t x11_modifier_state(unsigned int mods) {
    modifier_state_t result;
    result[MOD_SHIFT] = mods & ShiftMask;
    result[MOD_CONTROL] = mods & ControlMask;
    result[MOD_ALT] = mods & Mod1Mask;
    result[MOD_SUPER] = mods & Mod4Mask;
    result[MOD_CAPS_LOCK] = mods & LockMask;
    result[MOD_NUM_LOCK] = mods & Mod2Mask;
    return result;
}
#endif /* BUILD_X11 */

struct mouse_move_event : public mouse_positioned_event {
  modifier_state_t mods;  // held buttons and modifiers (ctrl, shift, ...)

  mouse_move_event(size_t x, size_t y, size_t x_abs, size_t y_abs, modifier_state_t mods = 0): mouse_positioned_event{mouse_event_t::MOUSE_MOVE, x, y, x_abs, y_abs}, mods(mods) {};

  void push_lua_data(lua_State *L) const;
};

enum scroll_direction_t: uint8_t {
  SCROLL_UNKNOWN = 0,
  SCROLL_UP,
  SCROLL_DOWN,
  SCROLL_LEFT,
  SCROLL_RIGHT,
};

#ifdef BUILD_X11
inline scroll_direction_t x11_scroll_direction(unsigned int x11_mouse_button) {
  scroll_direction_t direction = SCROLL_UNKNOWN;
  switch (x11_mouse_button) {
    case Button4:
      direction = SCROLL_UP;
      break;
    case Button5:
      direction = SCROLL_DOWN;
      break;
    case 6:
      direction = SCROLL_LEFT;
      break;
    case 7:
      direction = SCROLL_RIGHT;
      break;
  }
  return direction;
}
#endif /* BUILD_X11 */

struct mouse_scroll_event : public mouse_positioned_event {
  modifier_state_t mods;  // held buttons and modifiers (ctrl, shift, ...)
  scroll_direction_t direction;

  mouse_scroll_event(size_t x, size_t y, size_t x_abs, size_t y_abs, scroll_direction_t direction, modifier_state_t mods = 0): mouse_positioned_event{mouse_event_t::MOUSE_SCROLL, x, y, x_abs, y_abs}, direction(direction), mods(mods) {};

  void push_lua_data(lua_State *L) const;
};

struct mouse_button_event : public mouse_positioned_event {
  modifier_state_t mods;  // held buttons and modifiers (ctrl, shift, ...)
  mouse_button_t button;

  mouse_button_event(mouse_event_t type, size_t x, size_t y, size_t x_abs, size_t y_abs, mouse_button_t button, modifier_state_t mods = 0): mouse_positioned_event{type, x, y, x_abs, y_abs}, button(button), mods(mods) {};

  void push_lua_data(lua_State *L) const;
};

struct mouse_crossing_event : public mouse_positioned_event {
  mouse_crossing_event(mouse_event_t type, size_t x, size_t y, size_t x_abs, size_t y_abs): mouse_positioned_event{type, x, y, x_abs, y_abs} {};
};

#endif /* MOUSE_EVENTS_H */
