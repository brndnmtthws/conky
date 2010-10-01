/* tolua
** Support code for Lua bindings.
** Written by Waldemar Celes
** TeCGraf/PUC-Rio
** Aug 2003
** $Id:$
*/

/* This code is free software; you can redistribute it and/or modify it.
** The software provided hereunder is on an "as is" basis, and
** the author has no obligation to provide maintenance, support, updates,
** enhancements, or modifications.
*/

#include "tolua++.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void help (void)
{
 fprintf(stderr,"\n"
         "usage: tolua++ [options] input_file\n"
         "\n"
         "Command line options are:\n"
         "  -v       : print version information.\n"
         "  -o  file : set output file; default is stdout.\n"
         "  -H  file : create include file.\n"
         "  -n  name : set package name; default is input file root name.\n"
         "  -p       : parse only.\n"
         "  -P       : parse and print structure information (for debug).\n"
         "  -S       : disable support for c++ strings.\n"
         "  -1       : substract 1 to operator[] index (for compatibility with tolua5).\n"
         "  -L  file : run lua file (with dofile()) before doing anything.\n"
         "  -D       : disable automatic exporting of destructors for classes that have\n"
         "             constructors (for compatibility with tolua5)\n"
         "  -W       : disable warnings for unsupported features (for compatibility\n"
         "             with tolua5)\n"
         "  -C       : disable cleanup of included lua code (for easier debugging)\n"
         "  -E  value[=value] : add extra values to the luastate\n"
         "  -t       : export a list of types asociates with the C++ typeid name\n"
         "  -q       : don't print warnings to the console\n"
         "  -h       : print this message.\n"
         "Should the input file be omitted, stdin is assumed;\n"
         "in that case, the package name must be explicitly set.\n\n"
        );
}

static void version (void)
{
 fprintf(stderr, "%s (written by W. Celes, A. Manzur)\n",TOLUA_VERSION);
}

static void setfield (lua_State* L, int table, char* f, char* v)
{
 lua_pushstring(L,f);
 lua_pushstring(L,v);
 lua_settable(L,table);
}

static void add_extra (lua_State* L, char* value) {
	int len;
	lua_getglobal(L, "_extra_parameters");
	len = luaL_getn(L, -1);
	lua_pushstring(L, value);
	lua_rawseti(L, -2, len+1);
	lua_pop(L, 1);
};

static void error (char* o)
{
 fprintf(stderr,"tolua: unknown option '%s'\n",o);
 help();
 exit(1);
}

int main (int argc, char* argv[])
{
 #ifdef LUA_VERSION_NUM /* lua 5.1 */
 lua_State* L = luaL_newstate();
 luaL_openlibs(L);
 #else
 lua_State* L = lua_open();
 luaopen_base(L);
 luaopen_io(L);
 luaopen_string(L);
 luaopen_table(L);
 luaopen_math(L);
 luaopen_debug(L);
 #endif

 lua_pushstring(L,TOLUA_VERSION); lua_setglobal(L,"TOLUA_VERSION");
 lua_pushstring(L,LUA_VERSION); lua_setglobal(L,"TOLUA_LUA_VERSION");

 if (argc==1)
 {
  help();
  return 0;
 }
 else
 {
  int i, t;
  lua_newtable(L);
  lua_setglobal(L, "_extra_parameters");
  lua_newtable(L);
  lua_pushvalue(L,-1);
  lua_setglobal(L,"flags");
  t = lua_gettop(L);
  for (i=1; i<argc; ++i)
  {
   if (*argv[i] == '-')
   {
    switch (argv[i][1])
    {
     case 'v': version(); return 0;
     case 'h': help(); return 0;
     case 'p': setfield(L,t,"p",""); break;
     case 'P': setfield(L,t,"P",""); break;
     case 'o': setfield(L,t,"o",argv[++i]); break;
     case 'n': setfield(L,t,"n",argv[++i]); break;
     case 'H': setfield(L,t,"H",argv[++i]); break;
     case 'S': setfield(L,t,"S",""); break;
     case '1': setfield(L,t,"1",""); break;
     case 'L': setfield(L,t,"L",argv[++i]); break;
     case 'D': setfield(L,t,"D",""); break;
     case 'W': setfield(L,t,"W",""); break;
     case 'C': setfield(L,t,"C",""); break;
     case 'E': add_extra(L,argv[++i]); break;
     case 't': setfield(L,t,"t",""); break;
     case 'q': setfield(L,t,"q",""); break;
     default: error(argv[i]); break;
    }
   }
   else
   {
    setfield(L,t,"f",argv[i]);
    break;
   }
  }
  lua_pop(L,1);
 }
/* #define TOLUA_SCRIPT_RUN */
#ifndef TOLUA_SCRIPT_RUN
 {
  int tolua_tolua_open (lua_State* L);
  tolua_tolua_open(L);
 }
#else
 {
  char* p;
  char  path[BUFSIZ];
  strcpy(path,argv[0]);
  p = strrchr(path,'/');
  if (p==NULL) p = strrchr(path,'\\');
  p = (p==NULL) ? path : p+1;
  sprintf(p,"%s","../src/bin/lua/");
  lua_pushstring(L,path); lua_setglobal(L,"path");
		strcat(path,"all.lua");
  lua_dofile(L,path);
 }
#endif
 return 0;
}
