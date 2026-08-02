/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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

#include "catch2/catch.hpp"

#include <config.h>

#ifdef BUILD_HTTP
#include <string>
#include <utility>
#include <vector>

#include <conky.h>
#include <lua/lua-config.hh>
#include <lua/llua.h>

// lua_L is defined in llua.cc but never declared in a header; it's the
// separate Lua VM that llua_do_call() (and therefore llua_http_response_hook)
// actually calls into, distinct from `state`, which only parses the config
// file itself.
extern lua_State *lua_L;

namespace {
// Sets conky.config.lua_http_response_hook in `state`, which is what
// simple_config_setting::get() reads from.
void set_hook_setting(const std::string &value) {
  state->loadstring(
      ("conky.config.lua_http_response_hook = '" + value + "'").c_str());
  state->call(0, 0);
}
}  // namespace

TEST_CASE("llua_http_response_hook", "[http][lua]") {
  state = std::make_unique<lua::state>();
  conky::export_symbols(*state);
  llua_init();

  std::string body;
  int status = 200;
  std::vector<std::pair<std::string, std::string>> headers;

  SECTION("returns false when hook is not configured") {
    set_hook_setting("");

    bool result = llua_http_response_hook(&body, &status, &headers);

    REQUIRE_FALSE(result);
    REQUIRE(body.empty());
    REQUIRE(status == 200);
    REQUIRE(headers.empty());
  }

  SECTION("fills body/status/headers from a full table") {
    set_hook_setting("test_full");
    REQUIRE(luaL_dostring(lua_L,
                          "function conky_test_full()\n"
                          "  return {\n"
                          "    body = 'hello',\n"
                          "    status = 201,\n"
                          "    headers = { ['X-Test'] = 'yes' }\n"
                          "  }\n"
                          "end") == 0);

    bool result = llua_http_response_hook(&body, &status, &headers);

    REQUIRE(result);
    REQUIRE(body == "hello");
    REQUIRE(status == 201);
    REQUIRE(headers.size() == 1);
    REQUIRE(headers[0].first == "X-Test");
    REQUIRE(headers[0].second == "yes");
  }

  SECTION("defaults status and headers when only body is returned") {
    set_hook_setting("test_body_only");
    REQUIRE(luaL_dostring(lua_L,
                          "function conky_test_body_only()\n"
                          "  return { body = 'just the body' }\n"
                          "end") == 0);

    bool result = llua_http_response_hook(&body, &status, &headers);

    REQUIRE(result);
    REQUIRE(body == "just the body");
    REQUIRE(status == 200);
    REQUIRE(headers.empty());
  }

  SECTION("returns false when hook returns a non-table value") {
    set_hook_setting("test_not_table");
    REQUIRE(luaL_dostring(lua_L,
                          "function conky_test_not_table()\n"
                          "  return 'not a table'\n"
                          "end") == 0);

    bool result = llua_http_response_hook(&body, &status, &headers);

    REQUIRE_FALSE(result);
    REQUIRE(body.empty());
    REQUIRE(status == 200);
    REQUIRE(headers.empty());
  }

  SECTION("warns and continues when body field is missing") {
    set_hook_setting("test_no_body");
    REQUIRE(luaL_dostring(lua_L,
                          "function conky_test_no_body()\n"
                          "  return { status = 204 }\n"
                          "end") == 0);

    bool result = llua_http_response_hook(&body, &status, &headers);

    REQUIRE(result);
    REQUIRE(body.empty());
    REQUIRE(status == 204);
    REQUIRE(headers.empty());
  }

  SECTION("ignores non-string entries in the headers table") {
    set_hook_setting("test_mixed_headers");
    REQUIRE(luaL_dostring(lua_L,
                          "function conky_test_mixed_headers()\n"
                          "  return {\n"
                          "    body = 'ok',\n"
                          "    headers = { ['Good'] = 'yes', [42] = 'skip me' "
                          "}\n"
                          "  }\n"
                          "end") == 0);

    bool result = llua_http_response_hook(&body, &status, &headers);

    REQUIRE(result);
    REQUIRE(headers.size() == 1);
    REQUIRE(headers[0].first == "Good");
  }
}
#endif
