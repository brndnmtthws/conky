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

#include <config.h>

#include "setting.hh"

namespace conky {
	namespace priv {
		/*
		 * We cannot construct this object statically, because order of object construction in
		 * different modules is not defined, so config_setting_base could be called before this
		 * object is constructed. Therefore, we create it on the first call to
		 * config_setting_base constructor.
		 */
		config_settings_t *config_settings;

		config_setting_base::config_setting_base(const std::string &name_,
									const lua_setter_t &lua_setter_)
			: name(name_), lua_setter(lua_setter_)
		{
			struct config_settings_constructor {
				config_settings_constructor() { priv::config_settings = new config_settings_t; }
				~config_settings_constructor() { delete config_settings; config_settings = NULL; }
			};
			static config_settings_constructor constructor;

			bool inserted = config_settings->insert({name, this}).second;
			if(not inserted)
				throw std::logic_error("Setting with name '" + name + "' already registered");
		}

		void config_setting_base::lua_set(lua::state &l)
		{
			lua::stack_sentry s(l, -1);
			l.checkstack(2);

			l.getglobal("conky");
			l.rawgetfield(-1, "config");
			l.replace(-2);
			l.insert(-2);

			l.pushstring(name.c_str());
			l.insert(-2);

			l.settable(-3);
			l.pop();
		}
	}

	namespace {
		/*
		 * Performs the actual assignment of settings. Calls the setting-specific setter after
		 * some sanity-checking.
		 * stack on entry: | ..., new_config_table, key, value, old_value |
		 * stack on exit:  | ..., new_config_table, key |
		 */
		void process_setting(lua::state &l, bool init)
		{
			lua::stack_sentry s(l, -2);

			lua::Type type = l.type(-3);
			if(type != lua::TSTRING) {
				NORM_ERR("invalid setting of type '%s'", l.type_name(type));
				return;
			}

			std::string name = l.tostring(-3);
			auto iter = priv::config_settings->find(name);
			if(iter == priv::config_settings->end()) {
				NORM_ERR("Unknown setting '%s'", name.c_str());
				return;
			}

			iter->second->lua_setter(&l, init);
			l.pushvalue(-2);
			l.insert(-2);
			l.rawset(-4);
		}

		/*
		 * Called when user sets a new value for a setting
		 * stack on entry: | config_table, key, value |
		 * stack on exit:  | |
		 */
		int config__newindex(lua::state *l)
		{
			lua::stack_sentry s(*l, -3);
			l->checkstack(1);

			l->getmetatable(-3);
			l->replace(-4);

			l->pushvalue(-2);
			l->rawget(-4);
			process_setting(*l, false);

			return 0;
		}
	}

	void simple_lua_setter(lua::state *l, bool)
	{ l->pop(); }

	/*
	 * Called after the initial loading of the config file. Performs the initial assignments.
	 * at least one setting should always be registered, so config_settings will not be null
	 * stack on entry: | ... |
	 * stack on exit:  | ... |
	 */
	void check_config_settings(lua::state &l)
	{
		lua::stack_sentry s(l);
		l.checkstack(6);

		l.getglobal("conky"); {
			l.rawgetfield(-1, "config"); {
				if(l.type(-1) != lua::TTABLE)
					throw std::runtime_error("conky.config must be a table");

				// new conky.config table, containing only valid settings
				l.newtable(); {
					l.pushnil();
					while(l.next(-3)) {
						l.pushnil();
						process_setting(l, true);
					}
				} l.replace(-2);

				l.pushboolean(false);
				l.rawsetfield(-2, "__metatable");

				l.pushvalue(-1);
				l.rawsetfield(-2, "__index");

				l.pushfunction(&config__newindex);
				l.rawsetfield(-2, "__newindex");

				// conky.config will not be a table, but a userdata with some metamethods
				// we do this because we want to control access to the settings
				// we use the metatable for storing the settings, that means having a setting
				// whose name stars with "__" is a bad idea
				l.newuserdata(1);
				l.insert(-2);
				l.setmetatable(-2);
			} l.rawsetfield(-2, "config");
		} l.pop();
	}

/////////// example settings, remove after real settings are available ///////
	config_setting<std::string> asdf("asdf");
}
