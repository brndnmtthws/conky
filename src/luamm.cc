/* -*- mode: c; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
 * vim: ts=4 sw=4 noet ai cindent syntax=cpp
 *
 * luamm:  C++ binding for lua
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

#include "luamm.hh"

namespace lua {
	namespace {

#if LUA_VERSION_NUM >= 502
		// These two functions were deprecated in 5.2. Limited backwards compatibility is
		// provided by macros. We want them as real functions, because we take their addresses.

#undef lua_equal
		int lua_equal(lua_State *L, int index1, int index2)
		{ return lua_compare(L, index1, index2, LUA_OPEQ); }

#undef lua_lessthan
		int lua_lessthan(lua_State *L, int index1, int index2)
		{ return lua_compare(L, index1, index2, LUA_OPLT); }
#endif

		// keys for storing values in lua registry
		const char cpp_exception_metatable[] = "lua::cpp_exception_metatable";
		const char cpp_function_metatable [] = "lua::cpp_function_metatable";
		const char lua_exception_namespace[] = "lua::lua_exception_namespace";
		const char this_cpp_object		  [] = "lua::this_cpp_object";

		// converts C++ exceptions to strings, so lua can do something with them
		int exception_to_string(lua_State *l)
		{
			std::exception_ptr *ptr = static_cast<std::exception_ptr *>(lua_touserdata(l, -1));
			assert(ptr);
			try {
				std::rethrow_exception(*ptr);
			}
			catch(std::exception &e) {
				lua_pushstring(l, e.what());
			}
			catch(...) {
				lua_pushstring(l, ptr->__cxa_exception_type()->name());
			}
			return 1;
		}

		int absindex(lua_State *l, int index) throw()
		{ return index<0 && -index<=lua_gettop(l) ? lua_gettop(l)+1+index : index; }

		// Just like getfield(), only without calling metamethods (or throwing random exceptions)
		inline void rawgetfield(lua_State *l, int index, const char *k) throw(std::bad_alloc)
		{
			index = absindex(l, index);
			if(not lua_checkstack(l, 1))
				throw std::bad_alloc();

			lua_pushstring(l, k);
			lua_rawget(l, index);
		}

		// Just like setfield(), only without calling metamethods (or throwing random exceptions)
		inline void rawsetfield(lua_State *l, int index, const char *k) throw(std::bad_alloc)
		{
			index = absindex(l, index);
			if(not lua_checkstack(l, 2))
				throw std::bad_alloc();

			lua_pushstring(l, k);
			lua_insert(l, -2);
			lua_rawset(l, index);
		}

		int closure_trampoline(lua_State *l)
		{
			lua_checkstack(l, 2);
			rawgetfield(l, REGISTRYINDEX, this_cpp_object);
			assert(lua_islightuserdata(l, -1));
			state *L = static_cast<state *>( lua_touserdata(l, -1) );
			lua_pop(l, 1);

			try {
				cpp_function *fn = static_cast<cpp_function *>( L->touserdata(lua_upvalueindex(1)) );
				assert(fn);
				return (*fn)(L);
			}
			catch(lua::exception &e) {
				// rethrow lua errors as such
				e.push_lua_error(L);
			}
			catch(...) {
				// C++ exceptions (pointers to them, actually) are stored as lua userdata and
				// then thrown
				L->createuserdata<std::exception_ptr>(std::current_exception());
				L->rawgetfield(REGISTRYINDEX, cpp_exception_metatable);
				L->setmetatable(-2);
			}

			// lua_error does longjmp(), so destructors for objects in this function will not be
			// called
			return lua_error(l);
		}

		/*
		 * This function is called when lua encounters an error outside of any protected
		 * environment
		 * Throwing the exception through lua code appears to work, even if it was compiled
		 * without -fexceptions. If it turns out, it fails in some conditions, it could be
		 * replaced with some longjmp() magic. But that shouldn't be necessary, as this function
		 * will not be called under normal conditions (we execute everything in protected mode).
		 */
		int panic_throw(lua_State *l)
		{
			if(not lua_checkstack(l, 1))
				throw std::bad_alloc();

			rawgetfield(l, REGISTRYINDEX, this_cpp_object);
			assert(lua_islightuserdata(l, -1));
			state *L = static_cast<state *>( lua_touserdata(l, -1) );
			lua_pop(l, 1);

			throw lua::exception(L);
		}

		// protected mode wrappers for various lua functions
		int safe_concat_trampoline(lua_State *l)
		{
			lua_concat(l, lua_gettop(l));
			return 1;
		}

		template<int (*compare)(lua_State *, int, int)>
		int safe_compare_trampoline(lua_State *l)
		{
			int r = compare(l, 1, 2);
			lua_pop(l, 2);
			lua_pushinteger(l, r);
			return 1;
		}

		int safe_gc_trampoline(lua_State *l)
		{
			int what = lua_tointeger(l, -2);
			int data = lua_tointeger(l, -1);
			lua_pop(l, 2);
			lua_pushinteger(l, lua_gc(l, what, data));
			return 1;
		}

		template<void (*misc)(lua_State *, int), int nresults>
		int safe_misc_trampoline(lua_State *l)
		{
			misc(l, 1);
			return nresults;
		}

		int safe_next_trampoline(lua_State *l)
		{
			int r = lua_next(l, 1);
			lua_checkstack(l, 1);
			lua_pushinteger(l, r);
			return r ? 3 : 1;
		}

	}

	std::string exception::get_error_msg(state *L)
	{
		static const std::string default_msg("Unknown lua exception");

		try {
			return L->tostring(-1);
		}
		catch(not_string_error &e) {
			return default_msg;
		}
	}

	exception::exception(state *l)
		: std::runtime_error(get_error_msg(l)), L(l)
	{
		L->checkstack(1);

		L->rawgetfield(REGISTRYINDEX, lua_exception_namespace);
		L->insert(-2);
		key = L->ref(-2);
		L->pop(1);
	}

	exception::~exception() throw()
	{
		if(not L)
			return;
		L->checkstack(1);

		L->rawgetfield(REGISTRYINDEX, lua_exception_namespace);
		L->unref(-1, key);
		L->pop();
	}

	void exception::push_lua_error(state *l)
	{
		if(l != L)
			throw std::runtime_error("Cannot transfer exceptions between different lua contexts");
		l->checkstack(2);

		l->rawgetfield(REGISTRYINDEX, lua_exception_namespace);
		l->rawgeti(-1, key);
		l->replace(-2);
	}

	state::state()
	{
		if(lua_State *l = luaL_newstate())
			cobj.reset(l, &lua_close);
		else {
			// docs say this can happen only in case of a memory allocation error
			throw std::bad_alloc();
		}

		// set our panic function
		lua_atpanic(cobj.get(), panic_throw);

		checkstack(2);

		// store a pointer to ourselves
		pushlightuserdata(this);
		rawsetfield(REGISTRYINDEX, this_cpp_object);

		// a metatable for C++ exceptions travelling through lua code
		newmetatable(cpp_exception_metatable);
		lua_pushcfunction(cobj.get(), &exception_to_string);
		rawsetfield(-2, "__tostring");
		pushboolean(false);
		rawsetfield(-2, "__metatable");
		pushdestructor<std::exception_ptr>();
		rawsetfield(-2, "__gc");
		pop();

		// a metatable for C++ functions callable from lua code
		newmetatable(cpp_function_metatable);
		pushboolean(false);
		rawsetfield(-2, "__metatable");
		pushdestructor<cpp_function>();
		rawsetfield(-2, "__gc");
		pop();

		// while they're travelling through C++ code, lua exceptions will reside here
		newtable();
		rawsetfield(REGISTRYINDEX, lua_exception_namespace);

		luaL_openlibs(cobj.get());
	}

	void state::call(int nargs, int nresults, int errfunc)
	{
		int r = lua_pcall(cobj.get(), nargs, nresults, errfunc);
		if(r == 0)
			return;

		if(r == LUA_ERRMEM) {
			// memory allocation error, cross your fingers
			throw std::bad_alloc();
		}

		checkstack(3);
		rawgetfield(REGISTRYINDEX, cpp_exception_metatable);
		if(getmetatable(-2)) {
			if(rawequal(-1, -2)) {
				// it's a C++ exception, rethrow it
				std::exception_ptr *ptr = static_cast<std::exception_ptr *>(touserdata(-3));
				assert(ptr);

				/*
				 * we create a copy, so we can pop the object without fearing the exception will
				 * be collected by lua's GC
				 */
				std::exception_ptr t(*ptr); ptr = NULL;
				pop(3);
				std::rethrow_exception(t);
			}
			pop(2);
		}
		// it's a lua exception, wrap it
		if(r == LUA_ERRERR)
			throw lua::errfunc_error(this);
		else
			throw lua::exception(this);
	}

	void state::checkstack(int extra) throw(std::bad_alloc)
	{
		if(not lua_checkstack(cobj.get(), extra))
			throw std::bad_alloc();
	}

	void state::concat(int n)
	{
		assert(n>=0);
		checkstack(1);
		lua_pushcfunction(cobj.get(), safe_concat_trampoline);
		insert(-n-1);
		call(n, 1, 0);
	}

	bool state::equal(int index1, int index2)
	{
		// avoid pcall overhead in trivial cases
		if( rawequal(index1, index2) )
			return true;

		return safe_compare(&safe_compare_trampoline<lua_equal>, index1, index2);
	}

	int state::gc(int what, int data)
	{
		checkstack(3);
		lua_pushcfunction(cobj.get(), safe_gc_trampoline);
		pushinteger(what);
		pushinteger(data);
		call(2, 1, 0);
		assert(isnumber(-1));
		int r = tointeger(-1);
		pop();
		return r;
	}

	void state::getfield(int index, const char *k)
	{
		checkstack(1);
		index = absindex(index);
		pushstring(k);
		gettable(index);
	}

	void state::getglobal(const char *name)
	{
#if LUA_VERSION_NUM >= 502
		checkstack(1);
		pushinteger(LUA_RIDX_GLOBALS);
		gettable(REGISTRYINDEX);
		getfield(-1, name);
#else
		getfield(LUA_GLOBALSINDEX, name);
#endif
	}

	void state::gettable(int index)
	{
		checkstack(2);
		pushvalue(index);
		insert(-2);
		lua_pushcfunction(cobj.get(), (&safe_misc_trampoline<&lua_gettable, 1>));
		insert(-3);
		call(2, 1, 0);
	}

	bool state::lessthan(int index1, int index2)
	{
		return safe_compare(&safe_compare_trampoline<&lua_lessthan>, index1, index2);
	}

	void state::loadfile(const char *filename)
						throw(lua::syntax_error, lua::file_error, std::bad_alloc)
	{
		switch(luaL_loadfile(cobj.get(), filename)) {
			case 0:
				return;
			case LUA_ERRSYNTAX:
				throw lua::syntax_error(this);
			case LUA_ERRFILE:
				throw lua::file_error(this);
			case LUA_ERRMEM:
				throw std::bad_alloc();
			default:
				assert(0);
		}
	}

	void state::loadstring(const char *s) throw(lua::syntax_error, std::bad_alloc)
	{
		switch(luaL_loadstring(cobj.get(), s)) {
			case 0:
				return;
			case LUA_ERRSYNTAX:
				throw lua::syntax_error(this);
			case LUA_ERRMEM:
				throw std::bad_alloc();
			default:
				assert(0);
		}
	}

	bool state::next(int index)
	{
		checkstack(2);
		pushvalue(index);
		insert(-2);
		lua_pushcfunction(cobj.get(), &safe_next_trampoline);
		insert(-3);

		call(2, MULTRET, 0);

		assert(isnumber(-1));
		int r = tointeger(-1);
		pop();
		return r;
	}

	void state::pushclosure(const cpp_function &fn, int n)
	{
		checkstack(2);

		createuserdata<cpp_function>(fn);
		rawgetfield(REGISTRYINDEX, cpp_function_metatable);
		setmetatable(-2);

		insert(-n-1);
		lua_pushcclosure(cobj.get(), &closure_trampoline, n+1);
	}

	void state::rawgetfield(int index, const char *k) throw(std::bad_alloc)
	{ lua::rawgetfield(cobj.get(), index, k); }

	void state::rawsetfield(int index, const char *k) throw(std::bad_alloc)
	{ lua::rawsetfield(cobj.get(), index, k); }

	bool state::safe_compare(lua_CFunction trampoline, int index1, int index2)
	{
		// if one of the indexes is invalid, return false
		if(isnone(index1) || isnone(index2))
			return false;

		// convert relative indexes into absolute
		index1 = absindex(index1);
		index2 = absindex(index2);

		checkstack(3);
		lua_pushcfunction(cobj.get(), trampoline);
		pushvalue(index1);
		pushvalue(index2);
		call(2, 1, 0);
		assert(isnumber(-1));
		int r = tointeger(-1);
		pop();
		return r;
	}

	void state::setfield(int index, const char *k)
	{
		checkstack(1);
		index = absindex(index);
		pushstring(k);
		insert(-2);
		settable(index);
	}

	void state::setglobal(const char *name)
	{
#if LUA_VERSION_NUM >= 502
		stack_sentry s(*this, -1);
		checkstack(1);
		pushinteger(LUA_RIDX_GLOBALS);
		gettable(REGISTRYINDEX);
		insert(-2);
		setfield(-2, name);
		pop();
#else
		setfield(LUA_GLOBALSINDEX, name);
#endif
	}

	void state::settable(int index)
	{
		checkstack(2);
		pushvalue(index);
		insert(-3);
		lua_pushcfunction(cobj.get(), (&safe_misc_trampoline<&lua_settable, 0>));
		insert(-4);
		call(3, 0, 0);
	}

	std::string state::tostring(int index) throw(lua::not_string_error)
	{
		size_t len;
		const char *str = lua_tolstring(cobj.get(), index, &len);
		if(not str)
			throw not_string_error();
		return std::string(str, len);
	}
}
