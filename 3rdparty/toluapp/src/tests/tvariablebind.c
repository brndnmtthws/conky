/*
** Lua binding: tvariable
** Generated automatically by tolua 5.0a-CDLVS2 on 08/08/03 17:06:18.
*/

#ifndef __cplusplus
#include "stdlib.h"
#endif
#include "string.h"

#include "tolua.h"

/* Exported function */
TOLUA_API int tolua_tvariable_open (lua_State* tolua_S);

#include "tvariable.h"

/* function to register type */
static void tolua_reg_types (lua_State* tolua_S)
{
 tolua_usertype(tolua_S,"A");
 tolua_usertype(tolua_S,"U");
 tolua_usertype(tolua_S,"B");
}

/* get function: i of class  A */
static int tolua_get_A_i(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'i'",NULL);
#endif
 tolua_pushnumber(tolua_S,(double)self->i);
 return 1;
}

/* set function: i of class  A */
static int tolua_set_A_i(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'i'",NULL);
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->i = ((int)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: f of class  A */
static int tolua_get_A_f(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'f'",NULL);
#endif
 tolua_pushnumber(tolua_S,(double)self->f);
 return 1;
}

/* set function: f of class  A */
static int tolua_set_A_f(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'f'",NULL);
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->f = ((float)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: d of class  A */
static int tolua_get_A_d(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'd'",NULL);
#endif
 tolua_pushnumber(tolua_S,(double)self->d);
 return 1;
}

/* set function: d of class  A */
static int tolua_set_A_d(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'd'",NULL);
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->d = ((double)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: s of class  A */
static int tolua_get_A_s(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 's'",NULL);
#endif
 tolua_pushstring(tolua_S,(const char*)self->s);
 return 1;
}

/* set function: s of class  A */
static int tolua_set_A_s(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 's'",NULL);
 if (!tolua_isstring(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->s = ((char*)  tolua_tostring(tolua_S,2,0));
 return 0;
}

/* get function: v of class  A */
static int tolua_get_A_v(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'v'",NULL);
#endif
 tolua_pushuserdata(tolua_S,(void*)self->v);
 return 1;
}

/* set function: v of class  A */
static int tolua_set_A_v(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'v'",NULL);
 if (!tolua_isuserdata(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->v = ((void*)  tolua_touserdata(tolua_S,2,0));
 return 0;
}

/* get function: n of class  A */
static int tolua_get_A_n(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'n'",NULL);
#endif
 tolua_pushstring(tolua_S,(const char*)self->n);
 return 1;
}

/* set function: n of class  A */
static int tolua_set_A_n(lua_State* tolua_S)
{
  A* self = (A*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'n'",NULL);
 if (!tolua_isstring(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 strncpy(self->n,tolua_tostring(tolua_S,2,0),64-1);
 return 0;
}

/* get function: a of class  B */
static int tolua_get_B_a(lua_State* tolua_S)
{
  B* self = (B*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'a'",NULL);
#endif
 tolua_pushusertype(tolua_S,(void*)&self->a,"A");
 return 1;
}

/* set function: a of class  B */
static int tolua_set_B_a(lua_State* tolua_S)
{
  B* self = (B*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'a'",NULL);
 if (!tolua_isusertype(tolua_S,2,"A",0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->a = *((A*)  tolua_tousertype(tolua_S,2,0));
 return 0;
}

/* get function: b of class  B */
static int tolua_get_B_b_ptr(lua_State* tolua_S)
{
  B* self = (B*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'b'",NULL);
#endif
 tolua_pushusertype(tolua_S,(void*)self->b,"B");
 return 1;
}

/* set function: b of class  B */
static int tolua_set_B_b_ptr(lua_State* tolua_S)
{
  B* self = (B*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'b'",NULL);
 if (!tolua_isusertype(tolua_S,2,"B",0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->b = ((B*)  tolua_tousertype(tolua_S,2,0));
 return 0;
}

/* get function: i of class  U */
static int tolua_get_U_i(lua_State* tolua_S)
{
  U* self = (U*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'i'",NULL);
#endif
 tolua_pushnumber(tolua_S,(double)self->i);
 return 1;
}

/* set function: i of class  U */
static int tolua_set_U_i(lua_State* tolua_S)
{
  U* self = (U*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'i'",NULL);
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->i = ((int)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: f of class  U */
static int tolua_get_U_f(lua_State* tolua_S)
{
  U* self = (U*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'f'",NULL);
#endif
 tolua_pushnumber(tolua_S,(double)self->f);
 return 1;
}

/* set function: f of class  U */
static int tolua_set_U_f(lua_State* tolua_S)
{
  U* self = (U*)  tolua_tousertype(tolua_S,1,0);
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!self) tolua_error(tolua_S,"invalid 'self' in accessing variable 'f'",NULL);
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  self->f = ((float)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: i */
static int tolua_get_i(lua_State* tolua_S)
{
 tolua_pushnumber(tolua_S,(double)i);
 return 1;
}

/* set function: i */
static int tolua_set_i(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  i = ((int)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: f */
static int tolua_get_f(lua_State* tolua_S)
{
 tolua_pushnumber(tolua_S,(double)f);
 return 1;
}

/* set function: f */
static int tolua_set_f(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  f = ((float)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: d */
static int tolua_get_d(lua_State* tolua_S)
{
 tolua_pushnumber(tolua_S,(double)d);
 return 1;
}

/* set function: d */
static int tolua_set_d(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  d = ((double)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: s */
static int tolua_get_s(lua_State* tolua_S)
{
 tolua_pushstring(tolua_S,(const char*)s);
 return 1;
}

/* get function: v */
static int tolua_get_v(lua_State* tolua_S)
{
 tolua_pushuserdata(tolua_S,(void*)v);
 return 1;
}

/* set function: v */
static int tolua_set_v(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isuserdata(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  v = ((void*)  tolua_touserdata(tolua_S,2,0));
 return 0;
}

/* get function: n */
static int tolua_get_n(lua_State* tolua_S)
{
 tolua_pushstring(tolua_S,(const char*)n);
 return 1;
}

/* set function: n */
static int tolua_set_n(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isstring(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
 strncpy(n,tolua_tostring(tolua_S,2,0),64-1);
 return 0;
}

/* get function: a */
static int tolua_get_a(lua_State* tolua_S)
{
 tolua_pushusertype(tolua_S,(void*)&a,"A");
 return 1;
}

/* set function: a */
static int tolua_set_a(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"A",0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  a = *((A*)  tolua_tousertype(tolua_S,2,0));
 return 0;
}

/* get function: b */
static int tolua_get_b_ptr(lua_State* tolua_S)
{
 tolua_pushusertype(tolua_S,(void*)b,"B");
 return 1;
}

/* set function: b */
static int tolua_set_b_ptr(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"B",0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  b = ((B*)  tolua_tousertype(tolua_S,2,0));
 return 0;
}

/* get function: u */
static int tolua_get_u(lua_State* tolua_S)
{
 tolua_pushusertype(tolua_S,(void*)&u,"U");
 return 1;
}

/* set function: u */
static int tolua_set_u(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"U",0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  u = *((U*)  tolua_tousertype(tolua_S,2,0));
 return 0;
}

/* get function: mi */
static int tolua_get_M_mi(lua_State* tolua_S)
{
 tolua_pushnumber(tolua_S,(double)mi);
 return 1;
}

/* set function: mi */
static int tolua_set_M_mi(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  mi = ((int)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: mf */
static int tolua_get_M_mf(lua_State* tolua_S)
{
 tolua_pushnumber(tolua_S,(double)mf);
 return 1;
}

/* set function: mf */
static int tolua_set_M_mf(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  mf = ((float)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: md */
static int tolua_get_M_md(lua_State* tolua_S)
{
 tolua_pushnumber(tolua_S,(double)md);
 return 1;
}

/* set function: md */
static int tolua_set_M_md(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isnumber(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  md = ((double)  tolua_tonumber(tolua_S,2,0));
 return 0;
}

/* get function: ms */
static int tolua_get_M_ms(lua_State* tolua_S)
{
 tolua_pushstring(tolua_S,(const char*)ms);
 return 1;
}

/* get function: mv */
static int tolua_get_M_mv(lua_State* tolua_S)
{
 tolua_pushuserdata(tolua_S,(void*)mv);
 return 1;
}

/* set function: mv */
static int tolua_set_M_mv(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isuserdata(tolua_S,2,0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  mv = ((void*)  tolua_touserdata(tolua_S,2,0));
 return 0;
}

/* get function: mn */
static int tolua_get_M_mn(lua_State* tolua_S)
{
 tolua_pushstring(tolua_S,(const char*)mn);
 return 1;
}

/* get function: a */
static int tolua_get_M_a(lua_State* tolua_S)
{
 tolua_pushusertype(tolua_S,(void*)&a,"A");
 return 1;
}

/* set function: a */
static int tolua_set_M_a(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"A",0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  a = *((A*)  tolua_tousertype(tolua_S,2,0));
 return 0;
}

/* get function: ma */
static int tolua_get_M_ma(lua_State* tolua_S)
{
 tolua_pushusertype(tolua_S,(void*)&ma,"A");
 return 1;
}

/* set function: ma */
static int tolua_set_M_ma(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"A",0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  ma = *((A*)  tolua_tousertype(tolua_S,2,0));
 return 0;
}

/* get function: mb */
static int tolua_get_M_mb_ptr(lua_State* tolua_S)
{
 tolua_pushusertype(tolua_S,(void*)mb,"B");
 return 1;
}

/* set function: mb */
static int tolua_set_M_mb_ptr(lua_State* tolua_S)
{
#ifndef TOLUA_RELEASE
 tolua_Error tolua_err;
 if (!tolua_isusertype(tolua_S,2,"B",0,&tolua_err))
 tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);
#endif
  mb = ((B*)  tolua_tousertype(tolua_S,2,0));
 return 0;
}

/* Open function */
TOLUA_API int tolua_tvariable_open (lua_State* tolua_S)
{
 tolua_open(tolua_S);
 tolua_reg_types(tolua_S);
 tolua_module(tolua_S,NULL,1);
 tolua_beginmodule(tolua_S,NULL);
 tolua_cclass(tolua_S,"A","A","",NULL);
 tolua_beginmodule(tolua_S,"A");
 tolua_variable(tolua_S,"i",tolua_get_A_i,tolua_set_A_i);
 tolua_variable(tolua_S,"f",tolua_get_A_f,tolua_set_A_f);
 tolua_variable(tolua_S,"d",tolua_get_A_d,tolua_set_A_d);
 tolua_variable(tolua_S,"s",tolua_get_A_s,tolua_set_A_s);
 tolua_variable(tolua_S,"v",tolua_get_A_v,tolua_set_A_v);
 tolua_variable(tolua_S,"n",tolua_get_A_n,tolua_set_A_n);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"B","B","",NULL);
 tolua_beginmodule(tolua_S,"B");
 tolua_variable(tolua_S,"a",tolua_get_B_a,tolua_set_B_a);
 tolua_variable(tolua_S,"b",tolua_get_B_b_ptr,tolua_set_B_b_ptr);
 tolua_endmodule(tolua_S);
 tolua_cclass(tolua_S,"U","U","",NULL);
 tolua_beginmodule(tolua_S,"U");
 tolua_variable(tolua_S,"i",tolua_get_U_i,tolua_set_U_i);
 tolua_variable(tolua_S,"f",tolua_get_U_f,tolua_set_U_f);
 tolua_endmodule(tolua_S);
 tolua_variable(tolua_S,"i",tolua_get_i,tolua_set_i);
 tolua_variable(tolua_S,"f",tolua_get_f,tolua_set_f);
 tolua_variable(tolua_S,"d",tolua_get_d,tolua_set_d);
 tolua_variable(tolua_S,"s",tolua_get_s,NULL);
 tolua_variable(tolua_S,"v",tolua_get_v,tolua_set_v);
 tolua_variable(tolua_S,"n",tolua_get_n,tolua_set_n);
 tolua_variable(tolua_S,"a",tolua_get_a,tolua_set_a);
 tolua_variable(tolua_S,"b",tolua_get_b_ptr,tolua_set_b_ptr);
 tolua_variable(tolua_S,"u",tolua_get_u,tolua_set_u);
 tolua_module(tolua_S,"M",1);
 tolua_beginmodule(tolua_S,"M");
 tolua_variable(tolua_S,"mi",tolua_get_M_mi,tolua_set_M_mi);
 tolua_variable(tolua_S,"mf",tolua_get_M_mf,tolua_set_M_mf);
 tolua_variable(tolua_S,"md",tolua_get_M_md,tolua_set_M_md);
 tolua_variable(tolua_S,"ms",tolua_get_M_ms,NULL);
 tolua_variable(tolua_S,"mv",tolua_get_M_mv,tolua_set_M_mv);
 tolua_variable(tolua_S,"mn",tolua_get_M_mn,NULL);
 tolua_variable(tolua_S,"a",tolua_get_M_a,tolua_set_M_a);
 tolua_variable(tolua_S,"ma",tolua_get_M_ma,tolua_set_M_ma);
 tolua_variable(tolua_S,"mb",tolua_get_M_mb_ptr,tolua_set_M_mb_ptr);
 tolua_endmodule(tolua_S);
 tolua_endmodule(tolua_S);
 return 1;
}
