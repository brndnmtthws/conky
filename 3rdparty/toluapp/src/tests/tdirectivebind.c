/*
** Lua binding: tdirective
** Generated automatically by tolua 5.0a-CDLVS2 on 08/08/03 17:06:24.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua.h"

/* Exported function */
TOLUA_API int tolua_tdirective_open (lua_State* tolua_S);

#include "lualib.h"
#include "lauxlib.h"
int a;
extern int a;
int main (void)
{
 lua_State* L = lua_open();
	luaopen_base(L);
 tolua_tdirective_open(L);
	lua_dofile(L,"tdirective.lua");
	lua_close(L);
 return 0;
}

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
}

/* get function: a */
static int tolua_get_a(lua_State* tolua_S)
{
 tolua_pushnumber(tolua_S,(double)a);
 return 1;
}

/* set function: a */
static int tolua_set_a(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  a = ((int)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* Open function */
TOLUA_API int tolua_tdirective_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,1);
 tolua_beginmodule(tolua_S,NULL);
 tolua_variable(tolua_S,"a",tolua_get_a,tolua_set_a);
{
 a = 3;
}

 { /* begin embedded lua code */
 static unsigned char B[] = {
  10, 65, 32, 61, 32, 52,32
 };
 lua_dobuffer(tolua_S,(char*)B,sizeof(B),"tolua: embedded Lua code");
 } /* end of embedded lua code */


 { /* begin embedded lua code */
 static unsigned char B[] = {
  10,102,117,110, 99,116,105,111,110, 32,102,117,110, 99, 32,
  40, 41, 10,114,101,116,117,114,110, 32, 53, 10,101,110,100,
 32
 };
 lua_dobuffer(tolua_S,(char*)B,sizeof(B),"tolua: embedded Lua code");
 } /* end of embedded lua code */

 tolua_endmodule(tolua_S);
 return 1;
}
