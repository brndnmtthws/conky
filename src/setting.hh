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

#include "logging.h"
#include "luamm.hh"

namespace conky {

	// performs the assignment without any error checking
	void simple_lua_setter(lua::state *l, bool init);

	// converts standard lua types to C types
	template<typename T,
		bool integral = std::is_integral<T>::value,
		bool floating_point = std::is_floating_point<T>::value>
	struct simple_getter {
		// integral is here to force the compiler to evaluate the assert at instantiation time
		static_assert(integral && false,
				"Only specializations for string, integral and floating point types are available");
	};

	// Specialization for integral type.
	template<typename T>
	struct simple_getter<T, true, false> {
		static T do_it_def(lua::state *l, T def)
		{
			if(l->isnil(-1))
				return def;

			T t = l->tointeger(-1);
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
			virtual ~config_setting_base() {}

			/*
			 * Set the setting manually.
			 * stack on entry: | ... new_value |
			 * stack on exit:  | ... |
			 */
			void lua_set(lua::state &l);
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
	 * The lua_setter function is called when someone tries to set the value.
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

		l.getglobal("conky");
		l.getfield(-1, "config");
		l.replace(-2);

		l.getfield(-1, name.c_str());
		l.replace(-2);

		return getter(&l);
	}

	template<typename T>
	class enum_config_setting: public config_setting<T> {
		// In theory, this class may be useful for other types too. If you think think you have a
		// use for that, remove this assert.
		static_assert(std::is_enum<T>::value, "Only enum types allowed");

		typedef config_setting<T> Base;

		std::pair<T, bool> convert(lua::state *l, int index);
		T enum_getter(lua::state *l);
		void enum_lua_setter(lua::state *l, bool init);

	public:
		typedef std::initializer_list<std::pair<std::string, T>> Map;
		enum_config_setting(const std::string &name_, Map map_, bool modifiable_, T default_value_)
			: Base(name_,
					std::bind(&enum_config_setting::enum_getter, this, std::placeholders::_1),
					std::bind(&enum_config_setting::enum_lua_setter, this, std::placeholders::_1,
									std::placeholders::_2)),
			  map(map_), modifiable(modifiable_), default_value(default_value_)
		{}
	
	private:
		Map map;
		bool modifiable;
		T default_value;
	};

	template<typename T>
	std::pair<T, bool> enum_config_setting<T>::convert(lua::state *l, int index)
	{
		if(l->isnil(index))
			return {default_value, true};

		std::string val = l->tostring(index);

		for(auto i = map.begin(); i != map.end(); ++i) {
			if(i->first == val)
				return {i->second, true};
		}

		std::string msg = "Invalid value '" + val + "' for setting '"
			+ Base::name + "'. Valid values are: ";
		for(auto i = map.begin(); i != map.end(); ++i) {
			if(i != map.begin())
				msg += ", ";
			msg += "'" + i->first + "'";
		}
		msg += ".";
		NORM_ERR("%s", msg.c_str());
		
		return {default_value, false};
	}

	template<typename T>
	T enum_config_setting<T>::enum_getter(lua::state *l)
	{
		lua::stack_sentry s(*l, -1);
		auto ret = convert(l, -1);
		l->pop();

		// setter function should make sure the value is valid
		assert(ret.second);

		return ret.first;
	}

	template<typename T>
	void enum_config_setting<T>::enum_lua_setter(lua::state *l, bool init)
	{
		lua::stack_sentry s(*l, -2);

		if(!init && !modifiable) {
			NORM_ERR("Setting '%s' is not modifiable", Base::name.c_str());
			l->replace(-2);
		} else {
			auto ret = convert(l, -2);
			if(ret.second)
				l->pop();
			else
				l->replace(-2);
		}
		++s;
	}

	void check_config_settings(lua::state &l);

/////////// example settings, remove after real settings are available ///////
	extern config_setting<std::string> asdf;
}

#endif /* SETTING_HH */
