#include "lualib.h"
#include "lauxlib.h"

#include "tarray.h"


int a[10] = {1,2,3,4,5,6,7,8,9,10};
Point p[10] = {{0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8},{8,9},{9,10}};
Point* pp[10];

int ma[10];
Point mp[10];
Point* mpp[10];

Array array;
Array* parray = &array;

int main (void)
{
	int  tolua_tarray_open (lua_State*);
	lua_State* L = lua_open();
	int i;

	for (i=0; i<10; ++i)
	{
		pp[i] = &p[i];

		ma[i] = a[i];
		mp[i] = p[i];
		mpp[i] = pp[i];

		array.a[i] = a[i];
		array.p[i] = p[i];
		array.pp[i] = pp[i];
	}

	luaopen_base(L);
	tolua_tarray_open(L);

	lua_dofile(L,"tarray.lua");

	lua_close(L);
	return 0;
}

