/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Copyright (c) 2009 Toni Spets
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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

#include "llua.h"
#include <config.h>
#include "build.h"
#include "conky.h"
#include "logging.h"

#ifdef BUILD_LUA_EXTRAS
extern "C" {
#include <tolua++.h>
}
#endif /* BUILD_LUA_EXTRAS */

#ifdef HAVE_SYS_INOTIFY_H
#include <sys/inotify.h>

void llua_append_notify(const char *name);
void llua_rm_notifies(void);
static int llua_block_notify = 0;
#endif /* HAVE_SYS_INOTIFY_H */

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
      while (not files.empty()) {
        std::string::size_type pos = files.find(' ');
        if (pos > 0) {
          std::string file(files, 0, pos);
          llua_load(file.c_str());
        }
        files.erase(0, pos == std::string::npos ? pos : pos + 1);
      }
    }

    ++s;
  }

  void cleanup(lua::state &l) override {
    lua::stack_sentry s(l, -1);

#ifdef HAVE_SYS_INOTIFY_H
    llua_rm_notifies();
#endif /* HAVE_SYS_INOTIFY_H */
    if (lua_L == nullptr) {
      return;
    }
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
#ifdef BUILD_X11
conky::simple_config_setting<std::string> lua_draw_hook_pre("lua_draw_hook_pre",
                                                            std::string(),
                                                            true);
conky::simple_config_setting<std::string> lua_draw_hook_post(
    "lua_draw_hook_post", std::string(), true);
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
  const char *libs = PACKAGE_LIBDIR "/lib?.so;";
  char *old_path, *new_path;
  if (lua_L != nullptr) {
    return;
  }
  lua_L = luaL_newstate();

  /* add our library path to the lua package.cpath global var */
  luaL_openlibs(lua_L);
  lua_getglobal(lua_L, "package");
  lua_getfield(lua_L, -1, "cpath");
  old_path = strdup(lua_tostring(lua_L, -1));
  new_path = static_cast<char *>(malloc(strlen(old_path) + strlen(libs) + 1));
  strcpy(new_path, libs);
  strcat(new_path, old_path);
  lua_pushstring(lua_L, new_path);
  lua_setfield(lua_L, -3, "cpath");
  lua_pop(lua_L, 2);
  free(old_path);
  free(new_path);

  lua_pushstring(lua_L, PACKAGE_NAME " " VERSION " compiled " BUILD_DATE
                                     " for " BUILD_ARCH);
  lua_setglobal(lua_L, "conky_build_info");

  lua_pushstring(lua_L, VERSION);
  lua_setglobal(lua_L, "conky_version");

  lua_pushstring(lua_L, BUILD_DATE);
  lua_setglobal(lua_L, "conky_build_date");

  lua_pushstring(lua_L, BUILD_ARCH);
  lua_setglobal(lua_L, "conky_build_arch");

  lua_pushstring(lua_L, current_config.c_str());
  lua_setglobal(lua_L, "conky_config");

  lua_pushcfunction(lua_L, &llua_conky_parse);
  lua_setglobal(lua_L, "conky_parse");

  lua_pushcfunction(lua_L, &llua_conky_set_update_interval);
  lua_setglobal(lua_L, "conky_set_update_interval");

#if defined(BUILD_X11) && defined(BUILD_LUA_EXTRAS)
  /* register tolua++ user types */
  tolua_open(lua_L);
  tolua_usertype(lua_L, "Drawable");
  tolua_usertype(lua_L, "Visual");
  tolua_usertype(lua_L, "Display");
#endif /* BUILD_X11 */
}

void llua_load(const char *script) {
  int error;

  llua_init();

  std::string path = to_real_path(script);
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
  while ((str != nullptr) && (isspace(*str) != 0)) {
    ++str;
  }

  size_t level = 0;
  while ((str[*len] != 0) && (level > 0 || (isspace(str[*len]) == 0))) {
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

  if ((str[*len] == 0) && level > 0) {
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
  if (len == 0u) {
    return nullptr;
  }

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

  if (lua_L == nullptr) {
    return nullptr;
  }

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

  if (lua_L == nullptr) {
    return 0;
  }

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
    while (tail->next) {
      tail = tail->next;
    }
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
  if ((lua_L == nullptr) || lua_startup_hook.get(*state).empty()) {
    return;
  }
  llua_do_call(lua_startup_hook.get(*state).c_str(), 0);
}

void llua_shutdown_hook() {
  if ((lua_L == nullptr) || lua_shutdown_hook.get(*state).empty()) {
    return;
  }
  llua_do_call(lua_shutdown_hook.get(*state).c_str(), 0);
}

#ifdef BUILD_X11
void llua_draw_pre_hook() {
  if ((lua_L == nullptr) || lua_draw_hook_pre.get(*state).empty()) {
    return;
  }
  llua_do_call(lua_draw_hook_pre.get(*state).c_str(), 0);
}

void llua_draw_post_hook() {
  if ((lua_L == nullptr) || lua_draw_hook_post.get(*state).empty()) {
    return;
  }
  llua_do_call(lua_draw_hook_post.get(*state).c_str(), 0);
}

#ifdef BUILD_LUA_EXTRAS
void llua_set_userdata(const char *key, const char *type, void *value) {
  tolua_pushusertype(lua_L, value, type);
  lua_setfield(lua_L, -2, key);
}
#endif /* BUILD_LUA_EXTRAS */

void llua_setup_window_table(int text_start_x, int text_start_y, int text_width,
                             int text_height) {
  if (lua_L == nullptr) {
    return;
  }
  lua_newtable(lua_L);

  if (out_to_x.get(*state)) {
#ifdef BUILD_LUA_EXTRAS
    llua_set_userdata("drawable", "Drawable", (void *)&window.drawable);
    llua_set_userdata("visual", "Visual", window.visual);
    llua_set_userdata("display", "Display", display);
#endif /* BUILD_LUA_EXTRAS */

    llua_set_number("width", window.width);
    llua_set_number("height", window.height);
    llua_set_number("border_inner_margin", border_inner_margin.get(*state));
    llua_set_number("border_outer_margin", border_outer_margin.get(*state));
    llua_set_number("border_width", border_width.get(*state));

    llua_set_number("text_start_x", text_start_x);
    llua_set_number("text_start_y", text_start_y);
    llua_set_number("text_width", text_width);
    llua_set_number("text_height", text_height);

    lua_setglobal(lua_L, "conky_window");
  }
}

void llua_update_window_table(int text_start_x, int text_start_y,
                              int text_width, int text_height) {
  if (lua_L == nullptr) {
    return;
  }

  lua_getglobal(lua_L, "conky_window");
  if (lua_isnil(lua_L, -1)) {
    /* window table isn't populated yet */
    lua_pop(lua_L, 1);
    return;
  }

  llua_set_number("width", window.width);
  llua_set_number("height", window.height);

  llua_set_number("text_start_x", text_start_x);
  llua_set_number("text_start_y", text_start_y);
  llua_set_number("text_width", text_width);
  llua_set_number("text_height", text_height);

  lua_setglobal(lua_L, "conky_window");
}
#endif /* BUILD_X11 */

void llua_setup_info(struct information *i, double u_interval) {
  if (lua_L == nullptr) {
    return;
  }
  lua_newtable(lua_L);

  llua_set_number("update_interval", u_interval);
  llua_set_number("uptime", i->uptime);

  lua_setglobal(lua_L, "conky_info");
}

void llua_update_info(struct information *i, double u_interval) {
  if (lua_L == nullptr) {
    return;
  }

  lua_getglobal(lua_L, "conky_info");
  if (lua_isnil(lua_L, -1)) {
    /* window table isn't populated yet */
    lua_pop(lua_L, 1);
    return;
  }

  llua_set_number("update_interval", u_interval);
  llua_set_number("uptime", i->uptime);

  lua_setglobal(lua_L, "conky_info");
}

void print_lua(struct text_object *obj, char *p, int p_max_size) {
  char *str = llua_getstring(obj->data.s);
  if (str != nullptr) {
    snprintf(p, p_max_size, "%s", str);
    free(str);
  }
}

void print_lua_parse(struct text_object *obj, char *p, int p_max_size) {
  char *str = llua_getstring(obj->data.s);
  if (str != nullptr) {
    evaluate(str, p, p_max_size);
    free(str);
  }
}

double lua_barval(struct text_object *obj) {
  double per;
  if (llua_getnumber(obj->data.s, &per) != 0) {
    return per;
  }
  return 0;
}
