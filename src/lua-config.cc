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

#include "lua-config.hh"

#include "data-source.hh"

namespace conky {
	namespace {
		const char data_source_metatable[] = "conky::data_source_metatable";

		data_source_base& get_data_source(lua::state *l)
		{
			if(l->gettop() != 1)
				throw std::runtime_error("Wrong number of parameters");

			l->rawgetfield(lua::REGISTRYINDEX, data_source_metatable);
			if(not l->getmetatable(-2) or not l->rawequal(-1, -2))
				throw std::runtime_error("Invalid parameter");

			return **static_cast<std::shared_ptr<data_source_base> *>(l->touserdata(1));
		}

		int data_source_asnumber(lua::state *l)
		{
			double x = get_data_source(l).get_number();
			l->pushnumber(x);
			return 1;
		}

		int data_source_astext(lua::state *l)
		{
			std::string x = get_data_source(l).get_text();
			l->pushstring(x.c_str());
			return 1;
		}

		int create_data_source(lua::state *l, const data_sources_t::value_type &v)
		{
			l->createuserdata<std::shared_ptr<data_source_base>>(v.second(*l, v.first));
			l->rawgetfield(lua::REGISTRYINDEX, data_source_metatable);
			l->setmetatable(-2);
			return 1;
		}

		const char data_source__index[] = 
			"local table, key = ...;\n"
			"if key == 'num' then\n"
			"  return conky.asnumber(table);\n"
			"elseif key == 'text' then\n"
			"  return conky.astext(table);\n"
			"else\n"
			"  print(string.format([[Invalid data source operation: '%s']], key));\n"
			"  return 0/0;\n"
			"end\n";
	}

	void export_symbols(lua::state &l)
	{
		lua::stack_sentry s(l);
		l.checkstack(3);

		l.newmetatable(data_source_metatable); ++s; {
			l.pushboolean(false); ++s;
			l.rawsetfield(-2, "__metatable"); --s;

			l.pushdestructor<std::shared_ptr<data_source_base>>(); ++s;
			l.rawsetfield(-2, "__gc"); --s;

			l.loadstring(data_source__index); ++s;
			l.rawsetfield(-2, "__index"); --s;
		} l.pop(); --s;

		l.newtable(); ++s; {
			l.newtable(); ++s; {
				const data_sources_t &ds = get_data_sources();
				for(auto i = ds.begin(); i != ds.end(); ++i) {
					l.pushfunction(std::bind(create_data_source, 
										std::placeholders::_1, std::cref(*i)
								)); ++s;
					l.rawsetfield(-2, i->first.c_str()); --s;
				}
			} l.rawsetfield(-2, "variables"); --s;

			l.pushfunction(data_source_asnumber); ++s;
			l.rawsetfield(-2, "asnumber"); --s;

			l.pushfunction(data_source_astext); ++s;
			l.rawsetfield(-2, "astext"); --s;
		} l.setglobal("conky"); --s;
	}
}
