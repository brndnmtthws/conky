#ifndef LUA_H_
#define LUA_H_

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

void llua_init();
void llua_load(const char *script);
char *llua_getstring(const char *args);
int llua_getinteger(const char *args, int *per);
void llua_close();

#endif /* LUA_H_*/
