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

#ifndef LUAMM_HH
#define LUAMM_HH

#include <assert.h>
#include <memory>
#include <mutex>
#include <stdexcept>

#include <lua.hpp>

namespace lua {
	class state;

	typedef lua_Integer integer;
	typedef lua_Number number;
	typedef std::function<int(state *)> cpp_function;

	enum {
		REGISTRYINDEX = LUA_REGISTRYINDEX
	};

	enum {
		GCSTOP		 = LUA_GCSTOP,
		GCRESTART	 = LUA_GCRESTART,
		GCCOLLECT	 = LUA_GCCOLLECT,
		GCCOUNT		 = LUA_GCCOUNT,
		GCCOUNTB	 = LUA_GCCOUNTB,
		GCSTEP		 = LUA_GCSTEP,
		GCSETPAUSE	 = LUA_GCSETPAUSE,
		GCSETSTEPMUL = LUA_GCSETSTEPMUL
	};

	enum {
		MULTRET = LUA_MULTRET
	};

	enum Type {
		TBOOLEAN	   = LUA_TBOOLEAN,
		TFUNCTION	   = LUA_TFUNCTION,
		TLIGHTUSERDATA = LUA_TLIGHTUSERDATA,
		TNIL		   = LUA_TNIL,
		TNONE		   = LUA_TNONE,
		TNUMBER		   = LUA_TNUMBER,
		TSTRING		   = LUA_TSTRING,
		TTABLE		   = LUA_TTABLE,
		TTHREAD		   = LUA_TTHREAD,
		TUSERDATA	   = LUA_TUSERDATA
	};

	// we reserve one upvalue for the function pointer
	inline int upvalueindex(int n)
	{ return lua_upvalueindex(n+1); }

	/*
	 * Lua error()s are wrapped in this class when rethrown into C++ code. what() returns the
	 * error message. push_lua_error() pushes the error onto lua stack. The error can only be
	 * pushed into the same state it was generated in.
	 */
	class exception: public std::runtime_error {
		/*
		 * We only allow moving, to avoid complications with multiple references. It shouldn't be
		 * difficult to modify this to work with copying, if that proves unavoidable.
		 */
		state *L;
		int key;

		static std::string get_error_msg(state *L);

		exception(const exception &) = delete;
		const exception& operator=(const exception &) = delete;

	public:
		exception(exception &&other)
			: std::runtime_error(std::move(other)), L(other.L), key(other.key)
		{ other.L = NULL; }

		explicit exception(state *l);
		virtual ~exception() throw();

		void push_lua_error(state *l);
	};

	class not_string_error: public std::runtime_error {
	public:
		not_string_error()
			: std::runtime_error("Cannot convert value to a string")
		{}
	};

	// the name says it all
	class syntax_error: public lua::exception {
		syntax_error(const syntax_error &) = delete;
		const syntax_error& operator=(const syntax_error &) = delete;

	public:
		syntax_error(state *L)
			: lua::exception(L)
		{}

		syntax_error(syntax_error &&other)
			: lua::exception(std::move(other))
		{}
	};

	// loadfile() encountered an error while opening/reading the file
	class file_error: public lua::exception {
		file_error(const file_error &) = delete;
		const file_error& operator=(const file_error &) = delete;

	public:
		file_error(state *L)
			: lua::exception(L)
		{}

		file_error(file_error &&other)
			: lua::exception(std::move(other))
		{}
	};

	// double fault, lua encountered an error while running the error handler function
	class errfunc_error: public lua::exception {
		errfunc_error(const errfunc_error &) = delete;
		const errfunc_error& operator=(const errfunc_error &) = delete;

	public:
		errfunc_error(state *L)
			: lua::exception(L)
		{}

		errfunc_error(errfunc_error &&other)
			: lua::exception(std::move(other))
		{}
	};

	// a fancy wrapper around lua_State
	class state: private std::mutex {
		std::shared_ptr<lua_State> cobj;

		// destructor for C++ objects stored as lua userdata
		template<typename T>
		static int destroy_cpp_object(lua_State *l)
		{
			T *ptr = static_cast<T *>(lua_touserdata(l, -1));
			assert(ptr);
			try {
				// throwing exceptions in destructors is a bad idea
				// but we catch (and ignore) them, just in case
				ptr->~T();
			}
			catch(...) {
			}
			return 0;
		}

		bool safe_compare(lua_CFunction trampoline, int index1, int index2);
	public:
		state();

		/*
		 * Lua functions come in three flavours
		 * a) functions that never throw an exception
		 * b) functions that throw only in case of a memory allocation error
		 * c) functions that throw other kinds of errors
		 *
		 * Calls to type a functions are simply forwarded to the C api.
		 * Type c functions are executed in protected mode, to make sure they don't longjmp()
		 * over us (and our destructors). This add a certain amount overhead. If you care about
		 * performance, try using the raw versions (if possible).
		 * Type b functions are not executed in protected mode atm. as memory allocation errors
		 * don't happen that often (as opposed to the type c, where the user get deliberately set
		 * a metamethod that throws an error). That means those errors will do something
		 * undefined, but hopefully that won't be a problem.
		 *
		 * Semantics are mostly identical to those of the underlying C api. Any deviation is
		 * noted in the respective functions comment. The most important difference is that
		 * instead of return values, we use exceptions to indicate errors.	The lua and C++
		 * exception mechanisms are integrated. That means one can throw a C++ exception and
		 * catch it in lua (with pcall). Lua error()s can be caught in C++ as exceptions of type
		 * lua::exception.
		 */

		// type a, never throw
		int absindex(int index) throw() { return index<0 && -index<=gettop() ? gettop()+1+index : index; }
		bool getmetatable(int index) throw() { return lua_getmetatable(cobj.get(), index); }
		int gettop() throw() { return lua_gettop(cobj.get()); }
		void insert(int index) throw() { lua_insert(cobj.get(), index); }
		bool isboolean(int index) throw() { return lua_isboolean(cobj.get(), index); }
		bool isfunction(int index) throw() { return lua_isfunction(cobj.get(), index); }
		bool islightuserdata(int index) throw() { return lua_islightuserdata(cobj.get(), index); }
		bool isnil(int index) throw() { return lua_isnil(cobj.get(), index); }
		bool isnone(int index) throw() { return lua_isnone(cobj.get(), index); }
		bool isnumber(int index) throw() { return lua_isnumber(cobj.get(), index); }
		bool isstring(int index) throw() { return lua_isstring(cobj.get(), index); }
		void pop(int n = 1) throw() { lua_pop(cobj.get(), n); }
		void pushboolean(bool b) throw() { lua_pushboolean(cobj.get(), b); }
		void pushinteger(integer n) throw() { lua_pushinteger(cobj.get(), n); }
		void pushlightuserdata(void *p) throw() { lua_pushlightuserdata(cobj.get(), p); }
		void pushnil() throw() { lua_pushnil(cobj.get()); }
		void pushnumber(number n) throw() { lua_pushnumber(cobj.get(), n); }
		void pushvalue(int index) throw() { lua_pushvalue(cobj.get(), index); }
		void rawget(int index) throw() { lua_rawget(cobj.get(), index); }
		void rawgeti(int index, int n) throw() { lua_rawgeti(cobj.get(), index, n); }
		bool rawequal(int index1, int index2) throw() { return lua_rawequal(cobj.get(), index1, index2); }
		void replace(int index) throw() { lua_replace(cobj.get(), index); }
		// lua_setmetatable returns int, but docs don't specify it's meaning :/
		int setmetatable(int index) throw() { return lua_setmetatable(cobj.get(), index); }
		void settop(int index) throw() { return lua_settop(cobj.get(), index); }
		bool toboolean(int index) throw() { return lua_toboolean(cobj.get(), index); }
		integer tointeger(int index) throw() { return lua_tointeger(cobj.get(), index); }
		number tonumber(int index) throw() { return lua_tonumber(cobj.get(), index); }
		void* touserdata(int index) throw() { return lua_touserdata(cobj.get(), index); }
		Type type(int index) throw() { return static_cast<Type>(lua_type(cobj.get(), index)); }
		// typename is a reserved word :/
		const char* type_name(Type tp) throw() { return lua_typename(cobj.get(), tp); }
		void unref(int t, int ref) throw() { return luaL_unref(cobj.get(), t, ref); }

		// type b, throw only on memory allocation errors
		// checkstack correctly throws bad_alloc, because lua_checkstack kindly informs us of
		// that sitution
		void checkstack(int extra) throw(std::bad_alloc);
		const char* gsub(const char *s, const char *p, const char *r) { return luaL_gsub(cobj.get(), s, p, r); }
		bool newmetatable(const char *tname) { return luaL_newmetatable(cobj.get(), tname); }
		void newtable() { lua_newtable(cobj.get()); }
		void *newuserdata(size_t size) { return lua_newuserdata(cobj.get(), size); }
		// cpp_function can be anything that std::function can handle, everything else remains
		// identical
		void pushclosure(const cpp_function &fn, int n);
		void pushfunction(const cpp_function &fn) { pushclosure(fn, 0); }
		void pushstring(const char *s) { lua_pushstring(cobj.get(), s); }
		void pushstring(const char *s, size_t len) { lua_pushlstring(cobj.get(), s, len); }
		void pushstring(const std::string &s) { lua_pushlstring(cobj.get(), s.c_str(), s.size()); }
		void rawgetfield(int index, const char *k) throw(std::bad_alloc);
		void rawset(int index) { lua_rawset(cobj.get(), index); }
		void rawsetfield(int index, const char *k) throw(std::bad_alloc);
		int ref(int t) { return luaL_ref(cobj.get(), t); }
		// len recieves length, if not null. Returned value may contain '\0'
		const char* tocstring(int index, size_t *len = NULL) { return lua_tolstring(cobj.get(), index, len); }
		// Don't use pushclosure() to create a __gc function. The problem is that lua calls them
		// in an unspecified order, and we may end up destroying the object holding the
		// std::function before we get a chance to call it. This pushes a function that simply
		// calls ~T when the time comes. Only set it as __gc on userdata of type T.
		template<typename T>
		void pushdestructor()
		{ lua_pushcfunction(cobj.get(), &destroy_cpp_object<T>); }

		// type c, throw everything but the kitchen sink
		// call() is a protected mode call, we don't allow unprotected calls
		void call(int nargs, int nresults, int errfunc = 0);
		void concat(int n);
		bool equal(int index1, int index2);
		int gc(int what, int data);
		void getfield(int index, const char *k);
		void getglobal(const char *name);
		void gettable(int index);
		bool lessthan(int index1, int index2);
		void loadfile(const char *filename) throw(lua::syntax_error, lua::file_error, std::bad_alloc);
		void loadstring(const char *s) throw(lua::syntax_error, std::bad_alloc);
		bool next(int index);
		// register is a reserved word :/
		void register_fn(const char *name, const cpp_function &f) { pushfunction(f); setglobal(name); }
		void setfield(int index, const char *k);
		void setglobal(const char *name);
		void settable(int index);
		// lua_tostring uses NULL to indicate conversion error, since there is no such thing as a
		// NULL std::string, we throw an exception. Returned value may contain '\0'
		std::string tostring(int index) throw(lua::not_string_error);
		// allocate a new lua userdata of appropriate size, and create a object in it
		// pushes the userdata on stack and returns the pointer
		template<typename T, typename... Args>
		T* createuserdata(Args&&... args);

		using std::mutex::lock;
		using std::mutex::unlock;
		using std::mutex::try_lock;
	};

	/*
	 * Can be used to automatically pop temporary values off the lua stack on exit from the
	 * function/block (e.g. via an exception). It's destructor makes sure the stack contains
	 * exactly n items. The constructor initializes n to l.gettop()+n_, but that can be later
	 * changed with the overloaded operators. It is an error if stack contains less than n
	 * elements at entry into the destructor.
	 *
	 * Proposed stack discipline for functions is this:
	 * - called function always pops parameters off the stack.
	 * - if functions returns normally, it's return values are on the stack.
	 * - if function throws an exception, there are no return values on the stack.
	 * The last point differs from lua C api, which return an error message on the stack. But
	 * since we have exception.what() for that, putting the message on the stack is not
	 * necessary.
	 */
	class stack_sentry {
		state *L;
		int n;
	
		stack_sentry(const stack_sentry &) = delete;
		const stack_sentry& operator=(const stack_sentry &) = delete;
	public:
		explicit stack_sentry(state &l, int n_ = 0) throw()
			: L(&l), n(l.gettop()+n_)
		{ assert(n >= 0); }

		~stack_sentry()			throw() { assert(L->gettop() >= n); L->settop(n); }

		void operator++()		throw() { ++n; }
		void operator--()		throw() { --n; assert(n >= 0); }
		void operator+=(int n_) throw() { n+=n_; }
		void operator-=(int n_) throw() { n-=n_; assert(n >= 0); }
	};

	template<typename T, typename... Args>
	T* state::createuserdata(Args&&... args)
	{
		stack_sentry s(*this);

		void *t = newuserdata(sizeof(T));
		new(t) T(std::forward<Args>(args)...);
		++s;
		return static_cast<T *>(t);
	}
}

#endif /* LUAMM_HH */
