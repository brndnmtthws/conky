/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2010 Pavel Labath et al.
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

#include <config.h>

#include "setting.hh"

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace conky {

namespace {
typedef std::unordered_map<std::string, priv::config_setting_base *>
    settings_map;
using settings_vector = std::vector<priv::config_setting_base *>;

/*
 * We cannot construct this object statically, because order of object
 * construction in different modules is not defined, so config_setting_base
 * could be called before this object is constructed. Therefore, we create it on
 * the first call to config_setting_base constructor.
 */
settings_map *settings;

/// Settings that have been removed. Maps old name to an explanation message.
static const std::unordered_map<std::string, std::string> removed_settings = {
    {"own_window_argb_visual",
     "ARGB is now always enabled when available. Control opacity with "
     "`own_window_color` (e.g. '#8000')."},
    {"store_graph_data_explicitly",
     "Graph data is now always stored directly in the node; this setting has "
     "no effect."},
};

/// A setting that was renamed: the new canonical name plus whether using the
/// old name should emit a warning (some aliases are silent compatibility
/// shims, others nudge the user toward the new name).
struct setting_alias {
  std::string target;
  bool warn;
};

/// Settings that have been renamed. Maps an old (alias) name to its
/// replacement. When the user assigns one of these, the value is applied to
/// the target setting as if they had used the new name.
static const std::unordered_map<std::string, setting_alias> aliased_settings = {
    // `xftalpha` (X11) and `textalpha` (Wayland) controlled the same thing on
    // different backends; both now map to the unified `text_alpha`.
    {"xftalpha", {"text_alpha", true}},
    {"textalpha", {"text_alpha", true}},

    // old GB name, renamed for consistency
    {"own_window_colour", {"own_window_color", true}},
};

/*
 * Returns the setting record corresponding to the value at the specified index.
 * If the value is not valid, returns nullptr and prints an error.
 *
 * Doesn't affect the stack.
 */
priv::config_setting_base *get_setting(lua::state &l, int index) {
  lua::Type type = l.type(index);
  if (type != lua::TSTRING) {
    LOG_ERROR("invalid setting of type '{}'", l.type_name(type));
    return nullptr;
  }

  const std::string &name = l.tostring(index);
  auto iter = settings->find(name);
  if (iter != settings->end()) { return iter->second; }

  auto removed = removed_settings.find(name);
  if (removed != removed_settings.end()) {
    LOG_WARNING("setting '{}' has been removed: {}", name, removed->second);
  } else {
    LOG_ERROR("unknown setting '{}'", name);
  }
  return nullptr;
}

/*
 * Rewrites renamed (alias) settings in the user's config table to their
 * canonical names, so all later processing - the ordered apply loop and the
 * unknown-setting sweep - only ever sees current names. For each alias that the
 * user set: warns (when flagged), moves its value to the canonical key unless
 * the canonical name was also given (an explicit current name wins), and drops
 * the alias key so it is deduped and not later flagged as unknown. Aliases
 * whose target isn't registered (e.g. its backend wasn't built in) are left
 * in place so the unknown-setting sweep can report them.
 * stack on entry: | ... config_table |
 * stack on exit:  | ... config_table |
 */
void normalize_aliased_settings(lua::state &l) {
  lua::stack_sentry s(l);
  l.checkstack(3);

  for (const auto &[alias, target] : aliased_settings) {
    l.rawgetfield(-1, alias.c_str());
    if (l.isnil(-1) || settings->count(target.target) == 0) {
      l.pop();
      continue;
    }

    if (target.warn) {
      LOG_WARNING(
          "setting '{}' has been renamed to '{}'; update your config to use "
          "the new name",
          alias, target.target);
    }

    l.rawgetfield(-2, target.target.c_str());
    bool target_set = !l.isnil(-1);
    l.pop();
    if (target_set) {
      l.pop();  // explicit canonical value wins; drop the alias value
    } else {
      l.rawsetfield(-2, target.target.c_str());  // move value to canonical key
    }

    l.pushnil();
    l.rawsetfield(-2, alias.c_str());  // drop the alias key
  }
}

const std::vector<std::string> settings_ordering{
    "display",
    "out_to_x",
    // out_to_wayland must precede own_window: own_window's setter checks the
    // active GUI output (out_to_gui) and disables itself when none is set yet.
    "out_to_wayland",
    "use_xft",
    "font",
    "font0",
    "font1",
    "font2",
    "font3",
    "font4",
    "font5",
    "font6",
    "font7",
    "font8",
    "font9",
    "color0",
    "color1",
    "color2",
    "color3",
    "color4",
    "color5",
    "color6",
    "color7",
    "color8",
    "color9",
    "default_color",
    "default_shade_color",
    "default_outline_color",
    "border_inner_margin",
    "border_outer_margin",
    "border_width",
    "alignment",
    "xinerama_head",
    "lua_mouse_hook",
    "own_window_transparent",
    "own_window_class",
    "own_window_title",
    "own_window_type",
    "own_window_hints",
    "own_window_color",
    "own_window",
    "double_buffer",
    "imlib_cache_size",
};

// returns a vector of all settings, sorted in order of registration
settings_vector make_settings_vector() {
  settings_vector ret;
  ret.reserve(settings->size());

  // for _some_ settings, the order matters, for others it does not. first we
  // fill the vec with the settings which are ordered, then we add the remainder
  // in.
  for (auto &name : settings_ordering) {
    if (settings->count(name) > 0) {
      auto setting = settings->at(name);
      ret.push_back(setting);
    }
  }
  for (auto &setting : *settings) {
    if (std::find(settings_ordering.begin(), settings_ordering.end(),
                  setting.second->name) == settings_ordering.end()) {
      ret.push_back(setting.second);
    }
  }
  auto start = ret.begin();
  std::advance(start, settings_ordering.size());
  sort(start, ret.end(), &priv::config_setting_base::seq_compare);

  return ret;
}

/*
 * Returns the seq_no for the new setting object. Also constructs settings
 * object if needed.
 */
size_t get_next_seq_no() {
  struct settings_constructor {
    settings_constructor() { settings = new settings_map; }
    ~settings_constructor() {
      delete settings;
      settings = nullptr;
    }
  };
  static settings_constructor constructor;

  return settings->size();
}
}  // namespace

namespace priv {

config_setting_base::config_setting_base(std::string name_)
    : name(std::move(name_)), seq_no(get_next_seq_no()) {
  bool inserted = settings->insert({name, this}).second;
  if (!inserted) { CRIT_ERR("setting '{}' already registered", name); }
}

config_setting_base::config_setting_base(config_setting_base &&other) noexcept
    : name(std::move(other.name)),
      seq_no(other.seq_no),
      deprecation_msg(std::move(other.deprecation_msg)) {
  (*settings)[name] = this;
}

void config_setting_base::lua_set(lua::state &l) {
  std::lock_guard<lua::state> guard(l);
  lua::stack_sentry s(l, -1);
  l.checkstack(2);

  l.getglobal("conky");
  l.rawgetfield(-1, "config");
  l.replace(-2);
  l.insert(-2);

  l.setfield(-2, name.c_str());
  l.pop();
}

bool config_setting_base::is_set(lua::state &l) {
  std::lock_guard<lua::state> guard(l);
  lua::stack_sentry s(l);
  l.checkstack(2);

  l.getglobal("conky");
  l.getfield(-1, "config");
  l.replace(-2);

  l.getfield(-1, name.c_str());
  bool set = !l.isnil(-1);
  l.pop(2);

  return set;
}

/*
 * Performs the actual assignment of settings. Calls the setting-specific setter
 * after some sanity-checking.
 * stack on entry: | ..., new_config_table, key, value, old_value |
 * stack on exit:  | ..., new_config_table |
 */
void config_setting_base::process_setting(lua::state &l,
                                          config_setting_base *init) {
  lua::stack_sentry s(l, -3);

  config_setting_base *ptr = init != nullptr ? init : get_setting(l, -3);
  if (ptr == nullptr) { return; }

  if (init && ptr->deprecation_msg.has_value() && !l.isnil(-2)) {
    LOG_WARNING(
        "'{}' is deprecated and will be removed in a future "
        "release: {}",
        ptr->name, *ptr->deprecation_msg);
  }

  ptr->lua_setter(l, init != nullptr);
  l.pushvalue(-2);
  l.insert(-2);
  l.rawset(-4);
}

/*
 * Called when user sets a new value for a setting
 * stack on entry: | config_table, key, value |
 * stack on exit:  | |
 */
int config_setting_base::config__newindex(lua::state *l) {
  lua::stack_sentry s(*l, -3);
  l->checkstack(1);

  l->getmetatable(-3);
  l->replace(-4);

  l->pushvalue(-2);
  l->rawget(-4);
  process_setting(*l);

  return 0;
}

/*
 * conky.config will not be a table, but a userdata with some metamethods we do
 * this because we want to control access to the settings we use the metatable
 * for storing the settings, that means having a setting whose name starts with
 * "__" is a bad idea stack on entry: | ... | stack on exit:  | ...
 * new_config_table |
 */
void config_setting_base::make_conky_config(lua::state &l) {
  lua::stack_sentry s(l);
  l.checkstack(3);

  l.newuserdata(1);

  l.newtable();
  {
    l.pushboolean(false);
    l.rawsetfield(-2, "__metatable");

    l.pushvalue(-1);
    l.rawsetfield(-2, "__index");

    l.pushfunction(&priv::config_setting_base::config__newindex);
    l.rawsetfield(-2, "__newindex");
  }
  l.setmetatable(-2);

  ++s;
}
}  // namespace priv

void set_config_settings(lua::state &l) {
  lua::stack_sentry s(l);
  l.checkstack(6);

  // Force creation of settings map. In the off chance we have no settings.
  get_next_seq_no();

  l.getglobal("conky");
  {
    if (l.type(-1) != lua::TTABLE) {
      USER_ERR("'conky' must be a table in config");
    }

    l.rawgetfield(-1, "config");
    {
      if (l.type(-1) != lua::TTABLE) {
        USER_ERR("'conky.config' must be a table in config");
      }

      // Rewrite renamed settings to their canonical names up front, so the
      // ordered apply loop and the unknown-setting sweep below only deal with
      // current names.
      normalize_aliased_settings(l);

      priv::config_setting_base::make_conky_config(l);
      l.rawsetfield(-3, "config");

      l.rawgetfield(-2, "config");
      l.getmetatable(-1);
      l.replace(-2);
      {
        settings_vector all_settings = make_settings_vector();

        for (auto it : all_settings) {
          l.pushstring(it->name);
          l.rawgetfield(-3, it->name.c_str());
          l.pushnil();
          priv::config_setting_base::process_setting(l, it);
        }
      }
      l.pop();

      // print error messages for unknown settings
      l.pushnil();
      while (l.next(-2)) {
        l.pop();
        get_setting(l, -1);
      }
    }
    l.pop();
  }
  l.pop();
}

void cleanup_config_settings(lua::state &l) {
  lua::stack_sentry s(l);
  l.checkstack(2);

  l.getglobal("conky");
  l.rawgetfield(-1, "config");
  l.replace(-2);

  const settings_vector &v = make_settings_vector();
  for (size_t i = v.size(); i > 0; --i) {
    l.getfield(-1, v[i - 1]->name.c_str());
    v[i - 1]->cleanup(l);
  }

  l.pop();
}

}  // namespace conky
