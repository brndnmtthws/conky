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

#include "logging.h"
#include "luamm.hh"

namespace conky {

	/*
	 * Checks settings, and does initial calls to the setters.
	 * Should be called after reading the user config.
	 * stack on entry: | ... |
	 * stack on exit:  | ... |
	 */
	void set_config_settings(lua::state &l);

	/*
	 * Calls cleanup functions.
	 * Should be called before exit/restart.
	 * stack on entry: | ... |
	 * stack on exit:  | ... |
	 */
	void cleanup_config_settings(lua::state &l);

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
		typedef lua::integer Type;

		static inline std::pair<Type, bool>
		convert(lua::state &l, int index, const std::string &)
		{ return {l.tointeger(index), true}; }
	};

	// specialization for floating point types
	template<typename T>
	struct lua_traits<T, false, true, false> {
		static const lua::Type type = lua::TNUMBER;
		typedef lua::number Type;

		static inline std::pair<Type, bool>
		convert(lua::state &l, int index, const std::string &)
		{ return {l.tonumber(index), true}; }
	};

	// specialization for std::string
	template<>
	struct lua_traits<std::string, false, false, false> {
		static const lua::Type type = lua::TSTRING;
		typedef std::string Type;

		static inline std::pair<Type, bool>
		convert(lua::state &l, int index, const std::string &)
		{ return {l.tostring(index), true}; }
	};

	// specialization for bool
	template<>
	struct lua_traits<bool, true, false, false> {
		static const lua::Type type = lua::TBOOLEAN;
		typedef bool Type;

		static inline std::pair<Type, bool>
		convert(lua::state &l, int index, const std::string &)
		{ return {l.toboolean(index), true}; }
	};

	// specialization for enums
	// to use this, one first has to declare string<->value map
	template<typename T>
	struct lua_traits<T, false, false, true> {
		static const lua::Type type = lua::TSTRING;
		typedef T Type;

		typedef std::initializer_list<std::pair<std::string, T>> Map;
		static Map map;

		static std::pair<T, bool> convert(lua::state &l, int index, const std::string &name)
		{
			std::string val = l.tostring(index);

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

	namespace priv {
		class config_setting_base {
		private:
			static void process_setting(lua::state &l, bool init);
			static int config__newindex(lua::state *l);
			static void make_conky_config(lua::state &l);

			// copying is a REALLY bad idea
			config_setting_base(const config_setting_base &) = delete;
			config_setting_base& operator=(const config_setting_base &) = delete;

		protected:
			/*
			 * Set the setting, if the value is sane
			 * stack on entry: | ... potential_new_value old_value |
			 * stack on exit:  | ... real_new_value |
			 * real_new_value can be the old value if the new value doesn't make sense
			 */
			virtual void lua_setter(lua::state &l, bool init) = 0;

			/*
			 * Called on exit/restart.
			 * stack on entry: | ... new_value |
			 * stack on exit:  | ... |
			 */
			virtual void cleanup(lua::state &l) { l.pop(); }

		public:
			const std::string name;
			const size_t seq_no;

			static bool seq_compare(const config_setting_base *a, const config_setting_base *b)
			{ return a->seq_no < b->seq_no; }

			explicit config_setting_base(const std::string &name_);
			virtual ~config_setting_base() {}

			/*
			 * Set the setting manually.
			 * stack on entry: | ... new_value |
			 * stack on exit:  | ... |
			 */
			void lua_set(lua::state &l);

			friend void conky::set_config_settings(lua::state &l);
			friend void conky::cleanup_config_settings(lua::state &l);
		};
	}

	// If you need some very exotic setting, derive it from this class. Otherwise, scroll down.
	template<typename T>
	class config_setting_template: public priv::config_setting_base {
	public:
		explicit config_setting_template(const std::string &name_)
			: config_setting_base(name_)
		{}

		// get the value of the setting as a C++ type
		T get(lua::state &l);

	protected:
		/*
		 * Convert the value into a C++ type.
		 * stack on entry: | ... value |
		 * stack on exit:  | ... |
		 */
		virtual T getter(lua::state &l) = 0;
	};

	template<typename T>
	T config_setting_template<T>::get(lua::state &l)
	{
		lua::stack_sentry s(l);
		l.checkstack(2);

		l.getglobal("conky");
		l.getfield(-1, "config");
		l.replace(-2);

		l.getfield(-1, name.c_str());
		l.replace(-2);

		return getter(l);
	}

	/*
	 * Declares a setting <name> in the conky.config table.
	 * Getter function is used to translate the lua value into C++. It recieves the value on the
	 * lua stack. It should pop it and return the C++ value. In case the value is nil, it should
	 * return a predefined default value. Translation into basic types works with the help of
	 * lua_traits template above
	 * The lua_setter function is called when someone tries to set the value.  It recieves the
	 * new and the old value on the stack (old one is on top). It should return the new value for
	 * the setting. It doesn't have to be the value the user set, if e.g. the value doesn't make
	 * sense. The second parameter is true if the assignment occurs during the initial parsing of
	 * the config file, and false afterwards. Some settings obviously cannot be changed (easily?)
	 * when conky is running, but some (e.g. x/y position of the window) can.
	 */
	template<typename T, typename Traits = lua_traits<T>>
	class simple_config_setting: public config_setting_template<T> {
		typedef config_setting_template<T> Base;

	public:
		simple_config_setting(const std::string &name_, const T &default_value_ = T(),
													bool modifiable_ = false)
			: Base(name_), default_value(default_value_), modifiable(modifiable_)
		{}

	protected:
		const T default_value;
		const bool modifiable;

		virtual std::pair<typename Traits::Type, bool> do_convert(lua::state &l, int index);
		virtual void lua_setter(lua::state &l, bool init);

		virtual T getter(lua::state &l)
		{
			lua::stack_sentry s(l, -1);
			auto ret = do_convert(l, -1);
			l.pop();

			// setter function should make sure the value is valid
			assert(ret.second);

			return ret.first;
		}
	};

	template<typename T, typename Traits>
	std::pair<typename Traits::Type, bool>
	simple_config_setting<T, Traits>::do_convert(lua::state &l, int index)
	{
		if(l.isnil(index))
			return {default_value, true};

		if(l.type(index) != Traits::type) {
			NORM_ERR("Invalid value of type '%s' for setting '%s'. "
					 "Expected value of type '%s'.", l.type_name(l.type(index)),
					 Base::name.c_str(), l.type_name(Traits::type) );
			return {default_value, false};
		}

		return Traits::convert(l, index, Base::name);
	}

	template<typename T, typename Traits>
	void simple_config_setting<T, Traits>::lua_setter(lua::state &l, bool init)
	{
		lua::stack_sentry s(l, -2);

		bool ok = true;
		if(!init && !modifiable) {
			NORM_ERR("Setting '%s' is not modifiable", Base::name.c_str());
			ok = false;
		}

		if(ok && do_convert(l, -2).second)
			l.pop();
		else
			l.replace(-2);
		++s;
	}

	template<typename Signed1, typename Signed2>
	bool between(Signed1 value, Signed2 min,
				typename std::enable_if<std::is_signed<Signed2>::value, Signed2>::type max)
	{ return value >= min && value <= max; }

	template<typename Signed1, typename Unsigned2>
	bool between(Signed1 value, Unsigned2 min,
				typename std::enable_if<std::is_unsigned<Unsigned2>::value, Unsigned2>::type max)
	{ return value >= 0 && value >= min && value <= max; }

	// Just like simple_config_setting, except that in only accepts value in the [min, max] range
	template<typename T, typename Traits = lua_traits<T>>
	class range_config_setting: public simple_config_setting<T, Traits> {
		typedef simple_config_setting<T, Traits> Base;

		const T min;
		const T max;
	public:
		range_config_setting(const std::string &name_,
						const T &min_ = std::numeric_limits<T>::min(),
						const T &max_ = std::numeric_limits<T>::max(),
						const T &default_value_ = T(),
						bool modifiable_ = false)
			: Base(name_, default_value_, modifiable_), min(min_), max(max_)
		{ assert(min <= Base::default_value && Base::default_value <= max); }

	protected:
		virtual std::pair<typename Traits::Type, bool> do_convert(lua::state &l, int index)
		{
			auto ret = Base::do_convert(l, index);
			if(ret.second && !between(ret.first, min, max)) {
				NORM_ERR("Value is out of range for setting '%s'", Base::name.c_str());
				// we ignore out-of-range values. an alternative would be to clamp them. do we
				// want to do that?
				ret.second = false;
			}
			return ret;
		}
	};

/////////// example settings, remove after real settings are available ///////
	extern range_config_setting<int> asdf;
}

#endif /* SETTING_HH */
