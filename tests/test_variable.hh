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

#ifndef CONKY_TEST_VARIABLE_HH
#define CONKY_TEST_VARIABLE_HH

#include <cstring>
#include <string>

#include <content/text_object.h>
#include <parse/variables.hh>

/// RAII wrapper that looks up a registered variable by name, constructs a
/// text_object for it, and exposes the callbacks. Intended for tests that
/// exercise formatting through the registry rather than calling internal
/// print functions directly.
struct test_variable {
  struct text_object *obj = nullptr;

  test_variable(const char *name, const char *arg = nullptr) {
    using namespace conky::text_object;
    const auto *def = find_variable(name);
    if (def == nullptr) { return; }
    obj = static_cast<struct text_object *>(malloc(sizeof(struct text_object)));
    memset(obj, 0, sizeof(struct text_object));
    create_status status = create_status::success;
    construct_context ctx{arg, nullptr, nullptr, &status};
    def->construct(obj, ctx);
    if (status != create_status::success) {
      free(obj);
      obj = nullptr;
    }
  }

  ~test_variable() {
    if (obj == nullptr) { return; }
    if (obj->callbacks.free != nullptr) { obj->callbacks.free(obj); }
    if (obj->sub != nullptr) {
      if (obj->sub->data.s != nullptr) { free(obj->sub->data.s); }
      free(obj->sub);
    }
    free(obj);
  }

  test_variable(const test_variable &) = delete;
  test_variable &operator=(const test_variable &) = delete;

  explicit operator bool() const { return obj != nullptr; }

  std::string print(unsigned int buf_size = 256) const {
    if (obj == nullptr || obj->callbacks.print == nullptr) { return {}; }
    std::string buf(buf_size, '\0');
    obj->callbacks.print(obj, buf.data(), buf_size);
    buf.resize(strlen(buf.c_str()));
    return buf;
  }

  uint8_t percentage() const {
    if (obj == nullptr || obj->callbacks.percentage == nullptr) { return 0; }
    return obj->callbacks.percentage(obj);
  }

  double barval() const {
    if (obj == nullptr || obj->callbacks.barval == nullptr) { return 0; }
    return obj->callbacks.barval(obj);
  }

  int iftest() const {
    if (obj == nullptr || obj->callbacks.iftest == nullptr) { return 0; }
    return obj->callbacks.iftest(obj);
  }
};

#endif  // CONKY_TEST_VARIABLE_HH
