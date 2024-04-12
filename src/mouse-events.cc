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
#include <ctime>
#include <string>
#include <type_traits>

#include "logging.h"

#ifdef BUILD_XINPUT
#include <cstring>
#endif

extern "C" {
#include <lua.h>

#ifdef BUILD_XINPUT
#include <X11/extensions/XInput2.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
}

namespace conky {

/* Lua helper functions */
void push_table_value(lua_State *L, std::string key, std::string value) {
  lua_pushstring(L, key.c_str());
  lua_pushstring(L, value.c_str());
  lua_settable(L, -3);
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value>::type push_table_value(
    lua_State *L, std::string key, T value) {
  lua_pushstring(L, key.c_str());
  lua_pushinteger(L, value);
  lua_settable(L, -3);
}

template <typename T>
typename std::enable_if<std::is_floating_point<T>::value>::type
push_table_value(lua_State *L, std::string key, T value) {
  lua_pushstring(L, key.c_str());
  lua_pushnumber(L, value);
  lua_settable(L, -3);
}

void push_table_value(lua_State *L, std::string key, bool value) {
  lua_pushstring(L, key.c_str());
  lua_pushboolean(L, value);
  lua_settable(L, -3);
}

template <std::size_t N>
void push_bitset(lua_State *L, std::bitset<N> it,
                 std::array<std::string, N> labels) {
  lua_newtable(L);
  for (std::size_t i = 0; i < N; i++)
    push_table_value(L, labels[i], it.test(i));
}

const std::array<std::string, 6> mod_names = {{
    "shift",
    "control",
    "alt",
    "super",
    "caps_lock",
    "num_lock",
}};

void push_mods(lua_State *L, modifier_state_t mods) {
  lua_pushstring(L, "mods");
  push_bitset(L, mods, mod_names);
  lua_settable(L, -3);
}

// Returns ms since Epoch.
inline std::size_t current_time_ms() {
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return static_cast<std::size_t>(static_cast<std::uint64_t>(spec.tv_sec) *
                                      1'000 +
                                  spec.tv_nsec / 1'000'000);
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

void push_table_value(lua_State *L, std::string key,
                      scroll_direction_t direction) {
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
    case BUTTON_BACK:
      lua_pushstring(L, "back");
      break;
    case BUTTON_FORWARD:
      lua_pushstring(L, "forward");
      break;
    default:
      lua_pushnil(L);
      break;
  }
  lua_settable(L, -3);
}

/* Class methods */
mouse_event::mouse_event(mouse_event_t type)
    : type(type), time(current_time_ms()){};

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

void mouse_move_event::push_lua_data(lua_State *L) const {
  mouse_positioned_event::push_lua_data(L);
  push_mods(L, this->mods);
}

void mouse_scroll_event::push_lua_data(lua_State *L) const {
  mouse_positioned_event::push_lua_data(L);
  push_table_value(L, "direction", this->direction);
  push_mods(L, this->mods);
}

void mouse_button_event::push_lua_data(lua_State *L) const {
  mouse_positioned_event::push_lua_data(L);
  push_table_value(L, "button_code", static_cast<std::uint32_t>(this->button));
  push_table_value(L, "button", this->button);
  push_mods(L, this->mods);
}

#ifdef BUILD_XINPUT
XIDeviceInfoMap xi_device_info_cache{};

conky_device_info *conky_device_info::from_xi_id(Display *display,
                                                 int device_id) {
  if (xi_device_info_cache.count(device_id)) {
    return &xi_device_info_cache[device_id];
  }

  int num_devices;
  XIDeviceInfo *device = XIQueryDevice(display, device_id, &num_devices);
  if (num_devices == 0) { return nullptr; }

  std::map<std::string, xi_valuator_info> valuators;
  for (int i = 0; i < device->num_classes; i++) {
    if (device->classes[i]->type != XIValuatorClass) continue;

    XIValuatorClassInfo *class_info = (XIValuatorClassInfo *)device->classes[i];
    char *label = XGetAtomName(display, class_info->label);
    if (label == nullptr) {
      XFree(label);
      continue;
    }

    valuators[std::string(label)] =
        xi_valuator_info{.index = static_cast<size_t>(class_info->number),
                         .min = class_info->min,
                         .max = class_info->max};
    DBGP2("%s - %f %f %f", label, class_info->value, class_info->min,
          class_info->max);
    XFree(label);
  }

  xi_device_info_cache[device_id] =
      conky_device_info{.device_id = device_id,
                        .name = std::string(device->name),
                        .valuators = valuators};
  XIFreeDeviceInfo(device);

  return &xi_device_info_cache[device_id];
}

bool xi_event_data::test_valuator(size_t index) const {
  return this->valuators.count(index) > 0;
}

std::optional<double> xi_event_data::valuator_value(size_t index) const {
  if (this->valuators.count(index) == 0) return std::nullopt;
  return std::optional(this->valuators.at(index));
}

xi_event_data *xi_event_data::read_cookie(Display *display,
                                          XGenericEventCookie *cookie) {
  if (!XGetEventData(display, cookie)) {
    // already consumed
    return nullptr;
  }
  auto *source = reinterpret_cast<XIDeviceEvent *>(cookie->data);

  uint32_t buttons = 0;
  for (size_t bi = 1; bi <= source->buttons.mask_len; bi++) {
    buttons |= source->buttons.mask[bi] << (source->buttons.mask_len - bi) * 8;
  }

  std::map<size_t, double> valuators{};
  size_t valuator_index = 0;
  for (size_t vi = 0; vi < source->valuators.mask_len * 8; vi++) {
    if (XIMaskIsSet(source->valuators.mask, vi)) {
      valuators[vi] = source->valuators.values[valuator_index++];
    }
  }

  auto result = new xi_event_data{
      .evtype = static_cast<xi_event_type>(source->evtype),
      .serial = source->serial,
      .send_event = source->send_event,
      .display = source->display,
      .extension = source->extension,
      .time = source->time,
      .deviceid = source->deviceid,
      .sourceid = source->sourceid,
      .detail = source->detail,
      .root = source->root,
      .event = source->event,
      .child = source->child,
      .root_x = source->root_x,
      .root_y = source->root_y,
      .event_x = source->event_x,
      .event_y = source->event_y,
      .flags = source->flags,
      .buttons = std::bitset<32>(buttons),
      .valuators = valuators,
      .mods = source->mods,
      .group = source->group,
  };
  XFreeEventData(display, cookie);

  return result;
}

std::vector<std::tuple<int, XEvent *>> xi_event_data::generate_events(
    Window target, Window child, double target_x, double target_y) const {
  std::vector<std::tuple<int, XEvent *>> result{};

  if (this->evtype == XI_Motion) {
    auto device_info = conky_device_info::from_xi_id(display, this->deviceid);

    // Note that these are absolute (not relative) values in some cases
    int hor_move_v = device_info->valuators["Rel X"].index;   // Almost always 0
    int vert_move_v = device_info->valuators["Rel Y"].index;  // Almost always 1
    int hor_scroll_v =
        device_info->valuators["Rel Horiz Scroll"].index;  // Almost always 2
    int vert_scroll_v =
        device_info->valuators["Rel Vert Scroll"].index;  // Almost always 3

    bool is_move =
        this->test_valuator(hor_move_v) || this->test_valuator(vert_move_v);
    bool is_scroll =
        this->test_valuator(hor_scroll_v) || this->test_valuator(vert_scroll_v);

    if (is_move) {
      XEvent *produced = new XEvent;
      std::memset(produced, 0, sizeof(XEvent));

      XMotionEvent *e = &produced->xmotion;
      e->type = MotionNotify;
      e->display = this->display;
      e->root = this->root;
      e->window = target;
      e->subwindow = child;
      e->time = CurrentTime;
      e->x = static_cast<int>(target_x);
      e->y = static_cast<int>(target_y);
      e->x_root = static_cast<int>(this->root_x);
      e->y_root = static_cast<int>(this->root_y);
      e->state = this->mods.effective;
      e->is_hint = NotifyNormal;
      e->same_screen = True;
      result.emplace_back(std::make_tuple(PointerMotionMask, produced));
    }
    if (is_scroll) {
      XEvent *produced = new XEvent;
      std::memset(produced, 0, sizeof(XEvent));

      uint scroll_direction = 4;
      auto vertical = this->valuator_value(vert_scroll_v);

      // FIXME: Turn into relative values so direction works
      if (vertical.value_or(0.0) != 0.0) {
        scroll_direction = vertical.value() < 0.0 ? Button4 : Button5;
      } else {
        auto horizontal = this->valuator_value(hor_scroll_v);
        if (horizontal.value_or(0.0) != 0.0) {
          scroll_direction = horizontal.value() < 0.0 ? 6 : 7;
        }
      }

      XButtonEvent *e = &produced->xbutton;
      e->display = display;
      e->root = this->root;
      e->window = target;
      e->subwindow = child;
      e->time = CurrentTime;
      e->x = static_cast<int>(target_x);
      e->y = static_cast<int>(target_y);
      e->x_root = static_cast<int>(this->root_x);
      e->y_root = static_cast<int>(this->root_y);
      e->state = this->mods.effective;
      e->button = scroll_direction;
      e->same_screen = True;

      XEvent *press = new XEvent;
      e->type = ButtonPress;
      std::memcpy(press, produced, sizeof(XEvent));
      result.emplace_back(std::make_tuple(ButtonPressMask, press));

      e->type = ButtonRelease;
      result.emplace_back(std::make_tuple(ButtonReleaseMask, produced));
    }
  } else {
    XEvent *produced = new XEvent;
    std::memset(produced, 0, sizeof(XEvent));

    XButtonEvent *e = &produced->xbutton;
    e->display = display;
    e->root = this->root;
    e->window = target;
    e->subwindow = child;
    e->time = CurrentTime;
    e->x = static_cast<int>(target_x);
    e->y = static_cast<int>(target_y);
    e->x_root = static_cast<int>(this->root_x);
    e->y_root = static_cast<int>(this->root_y);
    e->state = this->mods.effective;
    e->button = this->detail;
    e->same_screen = True;

    long event_mask = NoEventMask;
    switch (this->evtype) {
      case XI_ButtonPress:
        e->type = ButtonPress;
        event_mask = ButtonPressMask;
        break;
      case XI_ButtonRelease:
        e->type = ButtonRelease;
        event_mask = ButtonReleaseMask;
        switch (this->detail) {
          case 1:
            event_mask |= Button1MotionMask;
            break;
          case 2:
            event_mask |= Button2MotionMask;
            break;
          case 3:
            event_mask |= Button3MotionMask;
            break;
          case 4:
            event_mask |= Button4MotionMask;
            break;
          case 5:
            event_mask |= Button5MotionMask;
            break;
        }
        break;
    }

    result.emplace_back(std::make_tuple(event_mask, produced));
  }

  return result;
}
#endif /* BUILD_XINPUT */

}  // namespace conky