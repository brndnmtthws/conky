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

#ifndef DATA_SOURCE_HH
#define DATA_SOURCE_HH

#include <limits>
#include <string>
#include <type_traits>

#include "luamm.hh"

namespace conky {

/*
 * A base class for all data sources.
 * API consists of two functions:
 * - get_number should return numeric representation of the data (if available).
 * This can then be used when drawing graphs, bars, ... The default
 * implementation returns NaN.
 * - get_text should return textual representation of the data. This is used
 * when simple displaying the value of the data source. The default
 * implementation converts get_number() to a string, but you can override to
 * return anything (e.g. add units)
 */
class data_source_base {
 public:
  const std::string name;

  explicit data_source_base(const std::string &name_) : name(name_) {}

  virtual ~data_source_base() {}
  virtual double get_number() const;
  virtual std::string get_text() const;
};

/*
 * A simple data source that returns the value of some variable. It ignores the
 * lua table. The source variable can be specified as a fixed parameter to the
 * register_data_source constructor, or one can create a subclass and then set
 * the source from the subclass constructor.
 */
template <typename T>
class simple_numeric_source : public data_source_base {
  static_assert(std::is_convertible<T, double>::value,
                "T must be convertible to double");

  const T *source;

 public:
  simple_numeric_source(lua::state *, const std::string &name_,
                        const T *source_)
      : data_source_base(name_), source(source_) {}

  virtual double get_number() const { return *source; }
};

/*
 * This is a part of the implementation, but it cannot be in .cc file because
 * the template functions below call it
 */
namespace priv {
const char data_source_metatable[] = "conky::data_source_metatable";

void do_register_data_source(const std::string &name,
                             const lua::cpp_function &fn);

class disabled_data_source : public simple_numeric_source<float> {
 public:
  disabled_data_source(lua::state *l, const std::string &name,
                       const std::string &setting);
};

}  // namespace priv

/*
 * Declaring an object of this type at global scope will register a data source
 * with the given name. Any additional parameters are passed on to the data
 * source constructor.
 */
template <typename T>
class register_data_source {
  template <typename... Args>
  static int factory(lua::state *l, const std::string &name,
                     const Args &...args) {
    T *t = static_cast<T *>(l->newuserdata(sizeof(T)));
    l->insert(1);
    new (t) T(l, name, args...);
    l->settop(1);
    l->rawgetfield(lua::REGISTRYINDEX, priv::data_source_metatable);
    l->setmetatable(-2);
    return 1;
  }

 public:
  template <typename... Args>
  explicit register_data_source(const std::string &name, Args &&...args) {
    priv::do_register_data_source(
        name,
        std::bind(&factory<Args...>, std::placeholders::_1, name, args...));
  }
};

/*
 * Use this to declare a data source that has been disabled during compilation.
 * We can then print a nice error message telling the used which setting to
 * enable.
 */
class register_disabled_data_source
    : public register_data_source<priv::disabled_data_source> {
 public:
  register_disabled_data_source(const std::string &name,
                                const std::string &setting);
};

/*
 * It expects to have a table at the top of lua stack. It then exports all the
 * data sources into that table (actually, into a "variables" subtable).
 */
void export_data_sources(lua::state &l);
}  // namespace conky

#endif /* DATA_SOURCE_HH */
