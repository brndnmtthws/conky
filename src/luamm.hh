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

#include <assert.h>
#include <memory>
#include <stdexcept>

#include <lua.hpp>

namespace lua {
    class state;

    typedef lua_Integer integer;
    typedef lua_Number number;
    typedef std::function<int(state *)> cpp_function;

    enum {
        ENVIRONINDEX = LUA_ENVIRONINDEX,
        GLOBALSINDEX = LUA_GLOBALSINDEX,
        REGISTRYINDEX = LUA_REGISTRYINDEX
    };

    enum {
        GCSTOP       = LUA_GCSTOP,
        GCRESTART    = LUA_GCRESTART,
        GCCOLLECT    = LUA_GCCOLLECT,
        GCCOUNT      = LUA_GCCOUNT,
        GCCOUNTB     = LUA_GCCOUNTB,
        GCSTEP       = LUA_GCSTEP,
        GCSETPAUSE   = LUA_GCSETPAUSE,
        GCSETSTEPMUL = LUA_GCSETSTEPMUL
    };

    enum {
        MULTRET = LUA_MULTRET
    };

    enum {
        TBOOLEAN       = LUA_TBOOLEAN,
        TFUNCTION      = LUA_TFUNCTION,
        TLIGHTUSERDATA = LUA_TLIGHTUSERDATA,
        TNIL           = LUA_TNIL,
        TNONE          = LUA_TNONE,
        TNUMBER        = LUA_TNUMBER,
        TSTRING        = LUA_TSTRING,
        TTABLE         = LUA_TTABLE,
        TTHREAD        = LUA_TTHREAD,
        TUSERDATA      = LUA_TUSERDATA
    };

    // we reserve one upvalue for the function pointer
    int upvalueindex(int n)
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
    class state {
        std::shared_ptr<lua_State> cobj;

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
         * instead of return values, we use exceptions to indicate errors.  The lua and C++
         * exception mechanisms are integrated. That means one can throw a C++ exception and
         * catch it in lua (with pcall). Lua error()s can be caught in C++ as exceptions of type
         * lua::exception.
         */

        // type a, never throw
        int absindex(int index) throw() { return index<0 && -index<=gettop() ? gettop()+1+index : index; }
        bool getmetatable(int index) throw() { return lua_getmetatable(cobj.get(), index); }
        int gettop() throw() { return lua_gettop(cobj.get()); }
        void insert(int index) throw() { lua_insert(cobj.get(), index); }
        bool isfunction(int index) throw() { return lua_isfunction(cobj.get(), index); }
        bool islightuserdata(int index) throw() { return lua_islightuserdata(cobj.get(), index); }
        bool isnone(int index) throw() { return lua_isnone(cobj.get(), index); }
        bool isnumber(int index) throw() { return lua_isnumber(cobj.get(), index); }
        bool isstring(int index) throw() { return lua_isstring(cobj.get(), index); }
        void pop(int n = 1) throw() { lua_pop(cobj.get(), n); }
        void pushboolean(bool b) throw() { lua_pushboolean(cobj.get(), b); }
        void pushinteger(integer n) throw() { lua_pushinteger(cobj.get(), n); }
        void pushlightuserdata(void *p) throw() { lua_pushlightuserdata(cobj.get(), p); }
        void pushnil() throw() { lua_pushnil(cobj.get()); }
        void pushvalue(int index) throw() { lua_pushvalue(cobj.get(), index); }
        void rawget(int index) throw() { lua_rawget(cobj.get(), index); }
        void rawgeti(int index, int n) throw() { lua_rawgeti(cobj.get(), index, n); }
        bool rawequal(int index1, int index2) throw() { return lua_rawequal(cobj.get(), index1, index2); }
        void rawset(int index) throw() { lua_rawset(cobj.get(), index); }
        void replace(int index) throw() { lua_replace(cobj.get(), index); }
        // lua_setmetatable returns int, but docs don't specify it's meaning :/
        int setmetatable(int index) throw() { return lua_setmetatable(cobj.get(), index); }
        integer tointeger(int index) throw() { return lua_tointeger(cobj.get(), index); }
        number tonumber(int index) throw() { return lua_tonumber(cobj.get(), index); }
        void* touserdata(int index) throw() { return lua_touserdata(cobj.get(), index); }
        int type(int index) throw() { return lua_type(cobj.get(), index); }
        // typename is a reserved word :/
        const char* type_name(int tp) throw() { return lua_typename(cobj.get(), tp); }
        void unref(int t, int ref) throw() { return luaL_unref(cobj.get(), t, ref); }

        // type b, throw only on memory allocation errors
        // checkstack correctly throws bad_alloc, because lua_checkstack kindly informs us of
        // that sitution
        void checkstack(int extra) throw(std::bad_alloc);
        bool newmetatable(const char *tname) { return luaL_newmetatable(cobj.get(), tname); }
        void newtable() { lua_newtable(cobj.get()); }
        void *newuserdata(size_t size) { return lua_newuserdata(cobj.get(), size); }
        // cpp_function can be anything that std::function can handle, everything else remains
        // identical
        void pushclosure(const cpp_function &fn, int n);
        void pushfunction(const cpp_function &fn) { pushclosure(fn, 0); }
        void pushstring(const char *s) { lua_pushstring(cobj.get(), s); }
        void rawgetfield(int index, const char *k) throw(std::bad_alloc);
        void rawsetfield(int index, const char *k) throw(std::bad_alloc);
        int ref(int t) { return luaL_ref(cobj.get(), t); }
        // len recieves length, if not null. Returned value may contain '\0'
        const char* tocstring(int index, size_t *len = NULL) { return lua_tolstring(cobj.get(), index, len); }

        // type c, throw everything but the kitchen sink
        // call() is a protected mode call, we don't allow unprotected calls
        void call(int nargs, int nresults, int errfunc = 0);
        void concat(int n);
        bool equal(int index1, int index2);
        int gc(int what, int data);
        void getfield(int index, const char *k);
        void gettable(int index);
        void getglobal(const char *name) { getfield(GLOBALSINDEX, name); }
        bool lessthan(int index1, int index2);
        void loadstring(const char *s) throw(lua::syntax_error, std::bad_alloc);
        bool next(int index);
        // register is a reserved word :/
        void register_fn(const char *name, const cpp_function &f) { pushfunction(f); setglobal(name); }
        void setfield(int index, const char *k);
        void setglobal(const char *name) { setfield(GLOBALSINDEX, name); }
        void settable(int index);
        // lua_tostring uses NULL to indicate conversion error, since there is no such thing as a
        // NULL std::string, we throw an exception. Returned value may contain '\0'
        std::string tostring(int index) throw(lua::not_string_error);
    };

    /*
     * Can be used to automatically pop temporary values off the lua stack on exit from the
     * function/block (e.g. via an exception). The constructor parameter indicates the number of
     * values to pop(). That can be later changed with the overloaded operators. The idiom is:
     * stack_sentry s(L);
     * L.an_operation_that_pushes_something(); ++s;
     * ...
     */
    class stack_sentry {
        state *L;
        int n;
    
        stack_sentry(const stack_sentry &) = delete;
        const stack_sentry& operator=(const stack_sentry &) = delete;
    public:
        explicit stack_sentry(state &l, int n_ = 0) throw()
            : L(&l), n(n_)
        {}

        ~stack_sentry()         throw() { L->pop(n); }

        void operator++()       throw() { ++n; }
        void operator--()       throw() { --n; }
        void operator+=(int n_) throw() { n+=n_; }
        void operator-=(int n_) throw() { n-=n_; }
    };
}
