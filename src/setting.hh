/* -*- mode: c++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
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

#ifndef SETTING_HH
#define SETTING_HH

#include <string>
#include <type_traits>
#include <unordered_map>

#include "luamm.hh"

namespace conky {

	// performs the assignment without any error checking
	void simple_lua_setter(lua::state *l, bool init);

	// converts standard lua types to C types
	template<typename T,
		bool integral_or_enum = std::is_integral<T>::value or std::is_enum<T>::value,
		bool floating_point = std::is_floating_point<T>::value>
	struct simple_getter {
		// integral_or_enum is here to force the compiler to evaluate the assert at instantiation
		// time
		static_assert(integral_or_enum && false,
				"Only specializations for string, integral, enum"
				"and floating point types are available" );
	};

	// Specialization for integral type and enums. In case of enums, one should provide a setter
	// function that makes sure the user sets a sane value
	template<typename T>
	struct simple_getter<T, true, false> {
		static T do_it_def(lua::state *l, T def)
		{
			if(l->isnil(-1))
				return def;

			// for enums we need to force a conversion
			T t = static_cast<T>(l->tointeger(-1));
			l->pop();
			return t;
		}

		static T do_it(lua::state *l)
		{ return do_it_def(l, static_cast<T>(0)); }
	};

	// specialization for floating point types
	template<typename T>
	struct simple_getter<T, false, true> {
		static T do_it_def(lua::state *l, T def)
		{
			if(l->isnil(-1))
				return def;

			T t = l->tonumber(-1);
			l->pop();
			return t;
		}

		static T do_it(lua::state *l)
		{ return do_it_def(l, T(0)); }
	};

	// specialization for std::string
	template<>
	struct simple_getter<std::string, false, false> {
		static std::string do_it_def(lua::state *l, const std::string &def)
		{
			if(l->isnil(-1))
				return def;

			std::string t = l->tostring(-1);
			l->pop();
			return t;
		}

		static std::string do_it(lua::state *l)
		{ return do_it_def(l, std::string()); }
	};

	namespace priv {
		class config_setting_base {
		public:
			typedef std::function<void (lua::state *l, bool init)> lua_setter_t;

			const std::string name;
			const lua_setter_t lua_setter;

			config_setting_base(const std::string &name_, const lua_setter_t &lua_setter_);
		};

		typedef std::unordered_map<std::string, config_setting_base *> config_settings_t;

		extern config_settings_t *config_settings;
	}

	/*
	 * Declares a setting <name> in the conky.config table.
	 * Getter function is used to translate the lua value into C++. It recieves the value on the
	 * lua stack. It should pop it and return the C++ value. In case the value is nil, it should
	 * return a predefined default value. Translation into basic types is provided with the
	 * default simple_getter::do_it functions.
	 * The lua_setter function is called when the user tries to set the value it the lua script.
	 * It recieves the new and the old value on the stack (old one is on top). It should return
	 * the new value for the setting. It doesn't have to be the value the user set, if e.g. the
	 * value doesn't make sense. The second parameter is true if the assignment occurs during the
	 * initial parsing of the config file, and false afterwards. Some settings obviously cannot
	 * be changed (easily?) when conky is running, but some (e.g. x/y position of the window)
	 * can.
	 */
	template<typename T>
	class config_setting: public priv::config_setting_base {
	public:
		typedef std::function<T (lua::state *l)> getter_t;

		config_setting(const std::string &name_,
				const getter_t &getter_ = &simple_getter<T>::do_it,
				const lua_setter_t &lua_setter_ = &simple_lua_setter)
			: config_setting_base(name_, lua_setter_), getter(getter_)
		{}

		T get(lua::state &l);
	private:
		getter_t getter;
	};

	template<typename T>
	T config_setting<T>::get(lua::state &l)
	{
		lua::stack_sentry s(l);
		l.checkstack(2);

		l.getglobal("conky"); ++s;
		l.getfield(-1, "config"); ++s;
		--s; l.replace(-2);
		l.getfield(-1, name.c_str()); ++s;
		--s; l.replace(-2);
		--s; return getter(&l);
	}

	void check_config_settings(lua::state &l);

/////////// example settings, remove after real settings are available ///////
	enum foo { bar, baz };
	extern config_setting<std::string> asdf;
	extern config_setting<foo> aasf;
}

#endif /* SETTING_HH */
