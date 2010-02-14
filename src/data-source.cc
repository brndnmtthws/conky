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

#include "data-source.hh"

#include <iostream>
#include <limits>
#include <sstream>

namespace conky {
	namespace {
		/*
		 * Returned when there is no data available.
		 * An alternative would be to throw an exception, but if we don't want to react too
		 * aggresively when the user e.g. uses a nonexisting variable, then returning NaN will do
		 * just fine.
		 */
		float NaN = std::numeric_limits<float>::quiet_NaN();

		/*
		 * We cannot construct this object statically, because order of object construction in
		 * different modules is not defined, so register_source could be called before this
		 * object is constructed. Therefore, we create it on the first call to register_source.
		 */
		data_sources_t *data_sources;

		void register_data_source_(const std::string &name, const data_source_factory &factory_func)
		{
			struct data_source_constructor {
				data_source_constructor()  { data_sources = new data_sources_t(); }
				~data_source_constructor() { delete data_sources; data_sources = NULL; }
			};
			static data_source_constructor constructor;

			bool inserted = data_sources->insert({name, factory_func}).second;
			if(not inserted)
				throw std::logic_error("Data source with name '" + name + "' already registered");
		}

		static void
		disabled_source_factory(lua::state &l, const std::string &name, const std::string &setting)
		{
			// XXX some generic way of reporting errors? NORM_ERR?
			std::cerr << "Support for variable '" << name
					  << "' has been disabled during compilation. Please recompile with '"
					  << setting << "'" << std::endl;

			simple_numeric_source<float>::factory(l, name, &NaN);
		}
	}

	double data_source_base::get_number() const
	{ return NaN; }

	std::string data_source_base::get_text() const
	{
		std::ostringstream s;
		s << get_number();
		return s.str();
	}

	template<typename T>
	void
	simple_numeric_source<T>::factory(lua::state &l, const std::string &name, const T *source)
	{
		l.pop();
		l.createuserdata<simple_numeric_source<T>>(name, source);
	}

	register_data_source::register_data_source(const std::string &name, 
			const data_source_factory &factory_func)
	{ register_data_source_(name, factory_func); }

	register_disabled_data_source::register_disabled_data_source(const std::string &name, 
			const std::string &setting)
	{
		register_data_source_(name,
				std::bind(disabled_source_factory,
							std::placeholders::_1, std::placeholders::_2, setting)
			);
	}

	// at least one data source should always be registered, so the pointer will not be null
	const data_sources_t& get_data_sources()
	{ return *data_sources; }
}

/////////// example data sources, remove after real data sources are available ///////
int asdf_ = 47;
conky::register_data_source asdf("asdf", std::bind(
			conky::simple_numeric_source<int>::factory,
			std::placeholders::_1, std::placeholders::_2, &asdf_)
		);

conky::register_disabled_data_source zxcv("zxcv", "BUILD_ZXCV");
