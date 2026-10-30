#include "catch2/catch.hpp"

#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

#include "lua/llua.h"

// The scripting VM is separate from the configuration VM.
extern lua_State *lua_L;

namespace {
struct scoped_environment {
  const char *name;
  std::optional<std::string> saved;

  scoped_environment(const char *key, const char *value) : name(key) {
    if (const char *old = std::getenv(name)) { saved = old; }
    REQUIRE((value != nullptr ? setenv(name, value, 1) : unsetenv(name)) == 0);
  }

  ~scoped_environment() {
    if (saved) {
      setenv(name, saved->c_str(), 1);
    } else {
      unsetenv(name);
    }
  }
};

struct script_state_context {
  lua_State *saved = lua_L;

  script_state_context() { lua_L = nullptr; }

  ~script_state_context() {
    if (lua_L != nullptr) { lua_close(lua_L); }
    lua_L = saved;
  }
};

std::string get_cpath(lua_State *l) {
  lua_getglobal(l, "package");
  lua_getfield(l, -1, "cpath");
  REQUIRE(lua_isstring(l, -1));
  std::string path = lua_tostring(l, -1);
  lua_pop(l, 2);
  return path;
}
}  // namespace

TEST_CASE("llua_cpath", "[lua][appimage]") {
  // Exercise both default and user-supplied paths, including Lua's
  // version-specific variable, without depending on the host's environment.
  const auto source = GENERATE(0, 1, 2);
  scoped_environment cpath("LUA_CPATH",
                           source == 1 ? "/custom/lib?.so;;" : nullptr);
  scoped_environment versioned_cpath(
      "LUA_CPATH_" LUA_VERSION_MAJOR "_" LUA_VERSION_MINOR,
      source == 2 ? "/versioned/lib?.so;;" : nullptr);
  std::unique_ptr<lua_State, decltype(&lua_close)> baseline(luaL_newstate(),
                                                            &lua_close);
  REQUIRE(baseline != nullptr);
  luaL_openlibs(baseline.get());
  const std::string native_path =
      std::string(PACKAGE_LIBDIR "/lib?.so;") + get_cpath(baseline.get());
  script_state_context context;

  SECTION("unset_appdir_preserves_native_paths") {
    scoped_environment appdir("APPDIR", nullptr);
    llua_init();
    CHECK(get_cpath(lua_L) == native_path);
    CHECK(lua_gettop(lua_L) == 0);
  }

  SECTION("empty_appdir_preserves_native_paths") {
    scoped_environment appdir("APPDIR", "");
    llua_init();
    CHECK(get_cpath(lua_L) == native_path);
    CHECK(lua_gettop(lua_L) == 0);
  }

  SECTION("appimage_path_precedes_native_paths") {
    scoped_environment appdir("APPDIR", "/tmp/conky AppDir");
    llua_init();
    const std::string expected =
        "/tmp/conky AppDir/usr/lib/conky/lib?.so;" + native_path;
    CHECK(get_cpath(lua_L) == expected);
    CHECK(lua_gettop(lua_L) == 0);

    // Reinitializing must not duplicate paths or replace the scripting VM.
    lua_State *initialized = lua_L;
    scoped_environment changed_appdir("APPDIR", "/another/AppDir");
    llua_init();
    CHECK(lua_L == initialized);
    CHECK(get_cpath(lua_L) == expected);
    CHECK(lua_gettop(lua_L) == 0);
  }
}
