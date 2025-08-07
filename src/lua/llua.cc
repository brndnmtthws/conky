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
#include "config.h"

#include <cstring>
#include <filesystem>
#include <sstream>

#include "../conky.h"
#include "../geometry.h"
#include "../logging.h"
#include "build.h"
#include "llua.h"

#ifdef BUILD_GUI
#include "../output/gui.h"

#ifdef BUILD_X11
#include "../output/x11.h"
#include "x11-settings.h"
#endif /* BUILD_X11 */

#ifdef BUILD_MOUSE_EVENTS
#include "../mouse-events.h"
#endif /* BUILD_MOUSE_EVENTS */
#endif /* BUILD_GUI */

extern "C" {
#include <tolua++.h>
}

#ifdef HAVE_SYS_INOTIFY_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#include <sys/inotify.h>
#pragma clang diagnostic pop

void llua_append_notify(const char *name);
void llua_rm_notifies(void);
static int llua_block_notify = 0;
#endif /* HAVE_SYS_INOTIFY_H */

// POSIX compliant
#include <sys/stat.h>

static void llua_load(const char *script);

lua_State *lua_L = nullptr;

namespace {
class lua_load_setting : public conky::simple_config_setting<std::string> {
  using Base = conky::simple_config_setting<std::string>;

 protected:
  void lua_setter(lua::state &l, bool init) override {
    lua::stack_sentry s(l, -2);

    Base::lua_setter(l, init);

    if (init) {
      std::string files = do_convert(l, -1).first;

      // Split file names into separate `\0` strings
      if (files.find(';') != std::string::npos) {
        for (auto &ch : files) {
          if (ch == ';') { ch = '\0'; }
        }
      } else {
        // TODO: Remove space-delimited file name handling in 3 years (2028.)
        for (auto &ch : files) {
          if (ch == ' ') { ch = '\0'; }
        }
      }

      const char *start = files.c_str();
      const char *end = start + files.size();
      while (start < end) {
        if (*start != '\0') {  // Skip empty strings
          llua_load(start);
        }
        start += strlen(start) + 1;
      }
    }

    ++s;
  }

  void cleanup(lua::state &l) override {
    lua::stack_sentry s(l, -1);

#ifdef HAVE_SYS_INOTIFY_H
    llua_rm_notifies();
#endif /* HAVE_SYS_INOTIFY_H */
    lua_close(lua_L);
    lua_L = nullptr;
  }

 public:
  lua_load_setting() : Base("lua_load", std::string(), false) {}
};

lua_load_setting lua_load;

conky::simple_config_setting<std::string> lua_startup_hook("lua_startup_hook",
                                                           std::string(), true);
conky::simple_config_setting<std::string> lua_shutdown_hook("lua_shutdown_hook",
                                                            std::string(),
                                                            true);
#ifdef BUILD_GUI
conky::simple_config_setting<std::string> lua_draw_hook_pre("lua_draw_hook_pre",
                                                            std::string(),
                                                            true);
conky::simple_config_setting<std::string> lua_draw_hook_post(
    "lua_draw_hook_post", std::string(), true);

#ifdef BUILD_MOUSE_EVENTS
conky::simple_config_setting<std::string> lua_mouse_hook("lua_mouse_hook",
                                                         std::string(), true);
#endif /* BUILD_MOUSE_EVENTS */

#endif
}  // namespace

static int llua_conky_parse(lua_State *L) {
  int n = lua_gettop(L); /* number of arguments */
  char *str;
  auto *buf = static_cast<char *>(calloc(1, max_user_text.get(*state)));
  if (n != 1) {
    lua_pushstring(
        L, "incorrect arguments, conky_parse(string) takes exactly 1 argument");
    lua_error(L);
  }
  if (lua_isstring(L, 1) == 0) {
    lua_pushstring(L, "incorrect argument (expecting a string)");
    lua_error(L);
  }
  str = strdup(lua_tostring(L, 1));
  evaluate(str, buf, max_user_text.get(*state));
  lua_pushstring(L, buf);
  free(str);
  free(buf);
  return 1; /* number of results */
}

static int llua_conky_set_update_interval(lua_State *L) {
  int n = lua_gettop(L); /* number of arguments */
  if (n != 1) {
    lua_pushstring(L,
                   "incorrect arguments, conky_set_update_interval(number) "
                   "takes exactly 1 argument");
    lua_error(L);
  }
  if (lua_isnumber(L, 1) == 0) {
    lua_pushstring(L, "incorrect argument (expecting a number)");
    lua_error(L);
  }
  state->pushnumber(lua_tonumber(L, 1));
  update_interval.lua_set(*state);
  return 0; /* number of results */
}

void llua_init() {
  std::string libs(PACKAGE_LIBDIR "/lib?.so;");
  std::string old_path, new_path;
  if (lua_L != nullptr) { return; }
  lua_L = luaL_newstate();

  /* add our library path to the lua package.cpath global var */
  luaL_openlibs(lua_L);
  lua_getglobal(lua_L, "package");   // stack: package
  lua_getfield(lua_L, -1, "cpath");  // stack: package.cpath, package

  old_path = std::string(lua_tostring(lua_L, -1));
  new_path = libs + old_path;

  lua_pushstring(lua_L,
                 new_path.c_str());  // stack: new_path, package.cpath, package
  lua_setfield(lua_L, -3, "cpath");  // stack: package.cpath, package
  lua_pop(lua_L, 1);                 // stack: package

  /* Add config file and XDG paths to package.path so scripts can load other
   * scripts from relative paths */
  {
    struct stat file_stat{};

    std::string path_ext;

    // add XDG directory to lua path
    auto xdg_path =
        std::filesystem::path(to_real_path(XDG_CONFIG_FILE)).parent_path();
    if (stat(xdg_path.c_str(), &file_stat) == 0) {
      path_ext.append(xdg_path);
      path_ext.append("/?.lua");
      path_ext.push_back(';');
    }

    auto parent_path = current_config.parent_path();
    if (xdg_path != parent_path && stat(parent_path.c_str(), &file_stat) == 0) {
      path_ext.append(parent_path);
      path_ext.append("/?.lua");
      path_ext.push_back(';');
    }

    lua_getfield(lua_L, -1, "path");  // stack: package.path, package
    old_path = std::string(lua_tostring(lua_L, -1));
    new_path = path_ext + old_path;

    lua_pushstring(lua_L,
                   new_path.c_str());  // stack: new_path, package.path, package
    lua_setfield(lua_L, -3, "path");   // stack: package.path, package
    lua_pop(lua_L, 1);                 // stack: package
  }
  lua_pop(lua_L, 1);  // stack is empty

  lua_pushstring(lua_L, PACKAGE_NAME " " VERSION " compiled for " BUILD_ARCH);
  lua_setglobal(lua_L, "conky_build_info");

  lua_pushstring(lua_L, VERSION);
  lua_setglobal(lua_L, "conky_version");

  lua_pushstring(lua_L, BUILD_ARCH);
  lua_setglobal(lua_L, "conky_build_arch");

  lua_pushstring(lua_L, current_config.c_str());
  lua_setglobal(lua_L, "conky_config");

  lua_pushcfunction(lua_L, &llua_conky_parse);
  lua_setglobal(lua_L, "conky_parse");

  lua_pushcfunction(lua_L, &llua_conky_set_update_interval);
  lua_setglobal(lua_L, "conky_set_update_interval");

#if defined(BUILD_X11)
  /* register tolua++ user types */
  tolua_open(lua_L);
  tolua_usertype(lua_L, "Drawable");
  tolua_usertype(lua_L, "Visual");
  tolua_usertype(lua_L, "Display");
#endif /* BUILD_X11 */
}

inline bool file_exists(const char *path) {
  struct stat buffer;
  return (stat(path, &buffer) == 0);
}

void llua_load(const char *script) {
  int error;

  std::filesystem::path path;
  std::filesystem::path script_path(script);

  path = to_real_path(script); // handles ~/some/path.lua
  if (!file_exists(path.c_str())) {
    if (!script_path.is_absolute()) {
      auto cfg_path = std::filesystem::path(to_real_path(XDG_CONFIG_FILE));
      auto cfg_dir  = cfg_path.parent_path();
  
      // prepend the config directory to the script path
      auto full = cfg_dir / script_path;
      path = to_real_path(full.c_str());
    }
    else {
      // Already an absolute path
      path = to_real_path(script);
    }
  }

  if (!file_exists(path.c_str())) {
    bool found_alternative = false;

    // Try resolving file name by using files in lua path:
    lua_getglobal(lua_L, "package");  // stack: package
    lua_getfield(lua_L, -1, "path");  // stack: package.path, package
    auto lua_path = lua_tostring(lua_L, -1);
    lua_pop(lua_L, 2);  // stack is empty

    std::stringstream path_stream(lua_path);
    std::string current;
    while (std::getline(path_stream, current, ';')) {
      // lua_load conky variable accepts full file names, so replace "?.lua"
      // with "?" to ensure file names don't get the unexpected .lua suffix; but
      // modules with init.lua will still work.
      size_t substitute_pos = current.find("?.lua");
      if (substitute_pos != std::string::npos) {
        current.replace(substitute_pos, 5, "?");
      }

      substitute_pos = current.find('?');
      if (substitute_pos == std::string::npos) { continue; }
      current.replace(substitute_pos, 1, script);
      path = to_real_path(current);

      if (file_exists(path.c_str())) {
        found_alternative = true;
        break;
      }
    }

    if (!found_alternative) {
      NORM_ERR("llua_load: specified script file '%s' doesn't exist", script);
      // return without initializing lua_L because other parts of the code rely
      // on it being null if the script is not loaded
      return;
    }
  }

  error = luaL_dofile(lua_L, path.c_str());
  if (error != 0) {
    NORM_ERR("llua_load: %s", lua_tostring(lua_L, -1));
    lua_pop(lua_L, 1);
#ifdef HAVE_SYS_INOTIFY_H
  } else if (!llua_block_notify && inotify_fd != -1) {
    llua_append_notify(path.c_str());
#endif /* HAVE_SYS_INOTIFY_H */
  }
}

/*
 * Returns the first space-delimited token of the string starting at position
 * *len. On return *len contains the length of the token. Spaces inside brackets
 * are ignored, so that eg. '${foo bar}' is treated as a single token. Sets *len
 * to zero and *str points to the end of the string when there are no more
 * tokens.
 */
static const char *tokenize(const char *str, size_t *len) {
  str += *len;
  *len = 0;
  while ((str != nullptr) && (isspace(static_cast<unsigned char>(*str)) != 0)) {
    ++str;
  }

  size_t level = 0;
  while ((str != nullptr) && (str[*len] != 0) &&
         (level > 0 || (isspace(static_cast<unsigned char>(str[*len])) == 0))) {
    switch (str[*len]) {
      case '{':
        ++level;
        break;
      case '}':
        --level;
        break;
    }
    ++*len;
  }

  if (str != nullptr && (str[*len] == 0) && level > 0) {
    NORM_ERR("tokenize: improperly nested token: %s", str);
  }

  return str;
}

/*
   llua_do_call does a flexible call to any Lua function
string: <function> [par1] [par2...]
retc: the number of return values expected
 */
static char *llua_do_call(const char *string, int retc) {
  static char func[64];
  int argc = 0;

  size_t len = 0;

  const char *ptr = tokenize(string, &len);

  /* proceed only if the function name is present */
  if (len == 0U) { return nullptr; }

  /* call only conky_ prefixed functions */
  if (strncmp(ptr, LUAPREFIX, strlen(LUAPREFIX)) != 0) {
    snprintf(func, sizeof func, "%s", LUAPREFIX);
  } else {
    *func = 0;
  }
  strncat(func, ptr, std::min(len, sizeof(func) - strlen(func) - 1));

  /* push the function name to stack */
  lua_getglobal(lua_L, func);

  /* parse all function parameters from args and push them to the stack */
  while (ptr = tokenize(ptr, &len), len != 0u) {
    lua_pushlstring(lua_L, ptr, len);
    argc++;
  }

  if (lua_pcall(lua_L, argc, retc, 0) != 0) {
    NORM_ERR("llua_do_call: function %s execution failed: %s", func,
             lua_tostring(lua_L, -1));
    lua_pop(lua_L, -1);
    return nullptr;
  }

  return func;
}

#if 0
/*
 * same as llua_do_call() except passes everything after func as one arg.
 */
static char *llua_do_read_call(const char *function, const char *arg, int retc)
{
	static char func[64];
	snprintf(func, 64, "conky_%s", function);

	/* push the function name to stack */
	lua_getglobal(lua_L, func);

	/* push function parameter to the stack */
	lua_pushstring(lua_L, arg);

	if (lua_pcall(lua_L, 1, retc, 0) != 0) {
		NORM_ERR("llua_do_call: function %s execution failed: %s", func, lua_tostring(lua_L, -1));
		lua_pop(lua_L, -1);
		return nullptr;
	}

	return func;
}
#endif

/* call a function with args, and return a string from it (must be free'd) */
static char *llua_getstring(const char *args) {
  char *func;
  char *ret = nullptr;

  func = llua_do_call(args, 1);
  if (func != nullptr) {
    if (lua_isstring(lua_L, -1) == 0) {
      NORM_ERR(
          "llua_getstring: function %s didn't return a string, result "
          "discarded",
          func);
    } else {
      ret = strdup(lua_tostring(lua_L, -1));
      lua_pop(lua_L, 1);
    }
  }

  return ret;
}

#if 0
/* call a function with args, and return a string from it (must be free'd) */
static char *llua_getstring_read(const char *function, const char *arg)
{
	char *func;
	char *ret = nullptr;

	if(!lua_L) return nullptr;

	func = llua_do_read_call(function, arg, 1);
	if (func) {
		if(!lua_isstring(lua_L, -1)) {
			NORM_ERR("llua_getstring_read: function %s didn't return a string, result discarded", func);
		} else {
			ret = strdup(lua_tostring(lua_L, -1));
			lua_pop(lua_L, 1);
		}
	}

	return ret;
}
#endif

/* call a function with args, and put the result in ret */
static int llua_getnumber(const char *args, double *ret) {
  char *func;

  func = llua_do_call(args, 1);
  if (func != nullptr) {
    if (lua_isnumber(lua_L, -1) == 0) {
      NORM_ERR(
          "llua_getnumber: function %s didn't return a number, result "
          "discarded",
          func);
    } else {
      *ret = lua_tonumber(lua_L, -1);
      lua_pop(lua_L, 1);
      return 1;
    }
  }
  return 0;
}

#ifdef HAVE_SYS_INOTIFY_H
struct _lua_notify_s {
  int wd;
  char name[DEFAULT_TEXT_BUFFER_SIZE];
  struct _lua_notify_s *next;
};
static struct _lua_notify_s *lua_notifies = 0;

static struct _lua_notify_s *llua_notify_list_do_alloc(const char *name) {
  struct _lua_notify_s *ret =
      (struct _lua_notify_s *)malloc(sizeof(struct _lua_notify_s));
  memset(ret, 0, sizeof(struct _lua_notify_s));
  strncpy(ret->name, name, DEFAULT_TEXT_BUFFER_SIZE);
  return ret;
}

void llua_append_notify(const char *name) {
  /* do it */
  struct _lua_notify_s *new_tail = 0;
  if (!lua_notifies) {
    /* empty, fresh new digs */
    new_tail = lua_notifies = llua_notify_list_do_alloc(name);
  } else {
    struct _lua_notify_s *tail = lua_notifies;
    while (tail->next) { tail = tail->next; }
    // should be @ the end now
    new_tail = llua_notify_list_do_alloc(name);
    tail->next = new_tail;
  }
  new_tail->wd = inotify_add_watch(inotify_fd, new_tail->name, IN_MODIFY);
}

void llua_rm_notifies(void) {
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

void llua_inotify_query(int wd, int mask) {
  struct _lua_notify_s *head = lua_notifies;
  if (mask & IN_MODIFY || mask & IN_IGNORED) {
    /* for whatever reason, i keep getting IN_IGNORED when the file is
     * modified */
    while (head) {
      if (head->wd == wd) {
        llua_block_notify = 1;
        llua_load(head->name);
        llua_block_notify = 0;
        NORM_ERR("Lua script '%s' reloaded", head->name);
        if (mask & IN_IGNORED) {
          /* for some reason we get IN_IGNORED here
           * sometimes, so we need to re-add the watch */
          head->wd = inotify_add_watch(inotify_fd, head->name, IN_MODIFY);
        }
        return;
      }
      head = head->next;
    }
  }
}
#endif /* HAVE_SYS_INOTIFY_H */

void llua_set_number(const char *key, double value) {
  lua_pushnumber(lua_L, value);
  lua_setfield(lua_L, -2, key);
}

void llua_startup_hook() {
  if (lua_startup_hook.get(*state).empty()) { return; }
  llua_do_call(lua_startup_hook.get(*state).c_str(), 0);
}

void llua_shutdown_hook() {
  if (lua_shutdown_hook.get(*state).empty()) { return; }
  llua_do_call(lua_shutdown_hook.get(*state).c_str(), 0);
}

#ifdef BUILD_GUI
void llua_draw_pre_hook() {
  if (lua_draw_hook_pre.get(*state).empty()) { return; }
  llua_do_call(lua_draw_hook_pre.get(*state).c_str(), 0);
}

void llua_draw_post_hook() {
  if (lua_draw_hook_post.get(*state).empty()) { return; }
  llua_do_call(lua_draw_hook_post.get(*state).c_str(), 0);
}

#ifdef BUILD_MOUSE_EVENTS
template <typename EventT>
bool llua_mouse_hook(const EventT &ev) {
  if (lua_mouse_hook.get(*state).empty()) { return false; }
  const std::string raw_hook_name = lua_mouse_hook.get(*state);
  std::string hook_name;
  if (raw_hook_name.rfind("conky_", 0) == 0) {
    hook_name = raw_hook_name;
  } else {
    hook_name = "conky_" + raw_hook_name;
  }

  int ty = lua_getglobal(lua_L, hook_name.c_str());
  if (ty == LUA_TNIL) {
    int ty_raw = lua_getglobal(lua_L, raw_hook_name.c_str());
    if (ty_raw == LUA_TFUNCTION) {
      // TODO: (1.22.0) Force conky_ prefix on use_mouse_hook like llua_do_call
      // does
      // - keep only else case, remove ty_raw and make hook_name const.
      NORM_ERR(
          "llua_mouse_hook: hook %s declaration is missing 'conky_' prefix",
          raw_hook_name.c_str());
      hook_name = raw_hook_name;
      ty = ty_raw;
      lua_insert(lua_L, -2);
      lua_pop(lua_L, 1);
    } else {
      NORM_ERR("llua_mouse_hook: hook %s is not defined", hook_name.c_str());
      lua_pop(lua_L, 2);
      return false;
    }
  } else if (ty != LUA_TFUNCTION) {
    NORM_ERR("llua_mouse_hook: hook %s is not a function", hook_name.c_str());
    lua_pop(lua_L, 1);
    return false;
  }

  ev.push_lua_table(lua_L);

  bool result = false;
  if (lua_pcall(lua_L, 1, 1, 0) != LUA_OK) {
    NORM_ERR("llua_mouse_hook: hook %s execution failed: %s", hook_name.c_str(),
             lua_tostring(lua_L, -1));
    lua_pop(lua_L, 1);
  } else {
    result = lua_toboolean(lua_L, -1);
    lua_pop(lua_L, 1);
  }

  return result;
}

template bool llua_mouse_hook<conky::mouse_scroll_event>(
    const conky::mouse_scroll_event &ev);
template bool llua_mouse_hook<conky::mouse_button_event>(
    const conky::mouse_button_event &ev);
template bool llua_mouse_hook<conky::mouse_move_event>(
    const conky::mouse_move_event &ev);
template bool llua_mouse_hook<conky::mouse_crossing_event>(
    const conky::mouse_crossing_event &ev);
#endif /* BUILD_MOUSE_EVENTS */

void llua_set_userdata(const char *key, const char *type, void *value) {
  tolua_pushusertype(lua_L, value, type);
  lua_setfield(lua_L, -2, key);
}

void llua_setup_window_table(conky::rect<int> text_rect) {
  lua_newtable(lua_L);

#ifdef BUILD_X11
  if (out_to_x.get(*state)) {
    llua_set_userdata("drawable", "Drawable", (void *)&window.drawable);
    llua_set_userdata("visual", "Visual", window.visual);
    llua_set_userdata("display", "Display", display);
  }
#endif /*BUILD_X11*/

#ifdef BUILD_GUI
  if (out_to_gui(*state)) {
#ifdef BUILD_X11
    llua_set_number("width", window.geometry.width());
    llua_set_number("height", window.geometry.height());
#endif /*BUILD_X11*/
    llua_set_number("border_inner_margin", border_inner_margin.get(*state));
    llua_set_number("border_outer_margin", border_outer_margin.get(*state));
    llua_set_number("border_width", border_width.get(*state));

    llua_set_number("text_start_x", text_rect.x());
    llua_set_number("text_start_y", text_rect.y());
    llua_set_number("text_width", text_rect.width());
    llua_set_number("text_height", text_rect.height());

    lua_setglobal(lua_L, "conky_window");
  }
#endif /*BUILD_GUI*/
}

void llua_update_window_table(conky::rect<int> text_rect) {
  lua_getglobal(lua_L, "conky_window");
  if (lua_isnil(lua_L, -1)) {
    /* window table isn't populated yet */
    lua_pop(lua_L, 1);
    return;
  }

#ifdef BUILD_X11
  llua_set_number("width", window.geometry.width());
  llua_set_number("height", window.geometry.height());
#endif /*BUILD_X11*/

  llua_set_number("text_start_x", text_rect.x());
  llua_set_number("text_start_y", text_rect.y());
  llua_set_number("text_width", text_rect.width());
  llua_set_number("text_height", text_rect.height());

  lua_setglobal(lua_L, "conky_window");
}
#endif /* BUILD_GUI */

void llua_setup_info(struct information *i, double u_interval) {
  lua_newtable(lua_L);

  llua_set_number("update_interval", u_interval);
  llua_set_number("cpu_count", i->cpu_count);

  lua_setglobal(lua_L, "conky_info");
}

void llua_update_info(struct information *i, double u_interval) {
  lua_getglobal(lua_L, "conky_info");
  if (lua_isnil(lua_L, -1)) {
    /* window table isn't populated yet */
    lua_pop(lua_L, 1);
    return;
  }

  llua_set_number("update_interval", u_interval);
  (void)i;

  lua_setglobal(lua_L, "conky_info");
}

void print_lua(struct text_object *obj, char *p, unsigned int p_max_size) {
  char *str = llua_getstring(obj->data.s);
  if (str != nullptr) {
    snprintf(p, p_max_size, "%s", str);
    free(str);
  }
}

void print_lua_parse(struct text_object *obj, char *p,
                     unsigned int p_max_size) {
  char *str = llua_getstring(obj->data.s);
  if (str != nullptr) {
    evaluate(str, p, p_max_size);
    free(str);
  }
}

double lua_barval(struct text_object *obj) {
  double per;
  if (llua_getnumber(obj->data.s, &per) != 0) { return per; }
  return 0;
}
