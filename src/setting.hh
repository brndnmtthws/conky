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

#include <limits>
#include <string>
#include <type_traits>
#include <unordered_map>

#include "logging.h"
#include "luamm.hh"

namespace conky {

	void check_config_settings(lua::state &l);

	template<typename T,
		bool is_integral = std::is_integral<T>::value,
		bool floating_point = std::is_floating_point<T>::value,
		bool is_enum = std::is_enum<T>::value>
	struct lua_traits
	{
		// integral is here to force the compiler to evaluate the assert at instantiation time
		static_assert(is_integral && false,
			"Only specializations for enum, string, integral and floating point types are available");
	};

	// specialization for integral types
	template<typename T>
	struct lua_traits<T, true, false, false> {
		static const lua::Type type = lua::TNUMBER;

		static std::pair<T, bool> convert(lua::state *l, int index, const std::string &)
		{ return {l->tointeger(index), true}; }
	};

	// specialization for floating point types
	template<typename T>
	struct lua_traits<T, false, true, false> {
		static const lua::Type type = lua::TNUMBER;

		static std::pair<T, bool> convert(lua::state *l, int index, const std::string &)
		{ return {l->tonumber(index), true}; }
	};

	// specialization for std::string
	template<>
	struct lua_traits<std::string, false, false, false> {
		static const lua::Type type = lua::TSTRING;

		static std::pair<std::string, bool> convert(lua::state *l, int index, const std::string &)
		{ return {l->tostring(index), true}; }
	};

	// specialization for bool
	template<>
	struct lua_traits<bool, true, false, false> {
		static const lua::Type type = lua::TBOOLEAN;

		static std::pair<bool, bool> convert(lua::state *l, int index, const std::string &)
		{ return {l->toboolean(index), true}; }
	};

	// specialization for enums
	// to use this, one first has to declare string<->value map
	template<typename T>
	struct lua_traits<T, false, false, true> {
		static const lua::Type type = lua::TSTRING;

		typedef std::initializer_list<std::pair<std::string, T>> Map;
		static Map map;

		static std::pair<T, bool> convert(lua::state *l, int index, const std::string &name)
		{
			std::string val = l->tostring(index);

			for(auto i = map.begin(); i != map.end(); ++i) {
				if(i->first == val)
					return {i->second, true};
			}

			std::string msg = "Invalid value '" + val + "' for setting '"
				+ name + "'. Valid values are: ";
			for(auto i = map.begin(); i != map.end(); ++i) {
				if(i != map.begin())
					msg += ", ";
				msg += "'" + i->first + "'";
			}
			msg += ".";
			NORM_ERR("%s", msg.c_str());
			
			return {T(), false};
		}
	};


	// standard getters and setters for basic types. They try to do The Right Thing(tm) (accept
	// only values of correct type and print an error message otherwise). For something more
	// elaborate, one can always write a new accessor class
	template<typename T, typename Traits = lua_traits<T>>
	class simple_accessors {
		const T default_value;
		bool modifiable;

	protected:
		std::pair<T, bool> do_convert(lua::state *l, int index, const std::string &name)
		{
			if(l->isnil(index))
				return {default_value, true};

			if(l->type(index) != Traits::type) {
				NORM_ERR("Invalid value of type '%s' for setting '%s'. "
						 "Expected value of type '%s'.", l->type_name(l->type(index)),
						 name.c_str(), l->type_name(Traits::type) );
				return {default_value, false};
			}

			return Traits::convert(l, index, name);
		}

		std::pair<T, bool> setter_check(lua::state *l, bool init, const std::string &name)
		{
			if(!init && !modifiable) {
				NORM_ERR("Setting '%s' is not modifiable", name.c_str());
				return {default_value, false};
			} else
				return do_convert(l, -2, name);
		}

	public:
		simple_accessors(T default_value_ = T(), bool modifiable_ = false)
			: default_value(default_value_), modifiable(modifiable_)
		{}

		T getter(lua::state *l, const std::string &name)
		{
			lua::stack_sentry s(*l, -1);
			auto ret = do_convert(l, -1, name);
			l->pop();

			// setter function should make sure the value is valid
			assert(ret.second);

			return ret.first;
		}

		void lua_setter(lua::state *l, bool init, const std::string &name)
		{
			lua::stack_sentry s(*l, -2);

			auto ret = setter_check(l, init, name);
			if(ret.second)
				l->pop();
			else
				l->replace(-2);
			++s;
		}
	};

	template<typename T, typename Traits = lua_traits<T>>
	class range_checking_accessors: private simple_accessors<T, Traits> {
		typedef simple_accessors<T, Traits> Base;

		T min;
		T max;
	public:
		range_checking_accessors(T min_ = std::numeric_limits<T>::min(),
								 T max_ = std::numeric_limits<T>::max(),
								 T default_value_ = T(), bool modifiable_ = false)
			: Base(default_value_, modifiable_), min(min_), max(max_)
		{ assert(min_ <= default_value_ && default_value_ <= max_); }

		using Base::getter;

		void lua_setter(lua::state *l, bool init, const std::string &name)
		{
			lua::stack_sentry s(*l, -2);

			auto ret = Base::setter_check(l, init, name);
			if(ret.second) {
				if(ret.first < min || ret.first > max) {
					NORM_ERR("Value is out of range for setting '%s'", name.c_str());
					// we ignore out-of-range values. an alternative would be to clamp them. do
					// we want to do that?
					l->replace(-2);
				} else
					l->pop();
			} else
				l->replace(-2);
			++s;
		}
	};

	namespace priv {
		class config_setting_base {
		private:
			static void process_setting(lua::state &l, bool init);
			static int config__newindex(lua::state *l);

		protected:
			virtual void call_lua_setter(lua::state *l, bool init) = 0;

		public:
			const std::string name;

			config_setting_base(const std::string &name_);

			/*
			 * Set the setting manually.
			 * stack on entry: | ... new_value |
			 * stack on exit:  | ... |
			 */
			void lua_set(lua::state &l);

			friend void conky::check_config_settings(lua::state &l);
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
	template<typename T, typename Accessors = simple_accessors<T>>
	class config_setting: public priv::config_setting_base {
	public:
		config_setting(const std::string &name_, const Accessors &accessors_ = Accessors())
			: config_setting_base(name_), accessors(accessors_)
		{}

		T get(lua::state &l);

	protected:
		virtual void call_lua_setter(lua::state *l, bool init)
		{ accessors.lua_setter(l, init, name); }
	
	private:
		Accessors accessors;
	};

	template<typename T, typename Accessors>
	T config_setting<T, Accessors>::get(lua::state &l)
	{
		lua::stack_sentry s(l);
		l.checkstack(2);

		l.getglobal("conky");
		l.getfield(-1, "config");
		l.replace(-2);

		l.getfield(-1, name.c_str());
		l.replace(-2);

		return accessors.getter(&l, name);
	}

/////////// example settings, remove after real settings are available ///////
	extern config_setting<int, range_checking_accessors<int>> asdf;
}

#endif /* SETTING_HH */
