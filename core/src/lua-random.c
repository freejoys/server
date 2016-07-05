#include <lua.h>
#include <lauxlib.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

unsigned long m_seed = 1;

int 
lseed(lua_State *L)
{
	m_seed = luaL_checkinteger(L, 1);
	return 0;
}

int 
lrand(lua_State *L)
{
	int top = lua_gettop(L);
	if (top != 1)
		return 0;
	
	unsigned long max = luaL_checkinteger(L, 1);
	m_seed = m_seed * 1103515245 + 12345;
	unsigned long rand = m_seed / 65536 % 32768 % max;
	lua_pushinteger(L, rand);
	return 1;
}

//////////////////////////////////////////////////////////////////////////
//only use for Fight 
unsigned long m_fightseed = 1;

int 
lfight_seed(lua_State *L)
{
	m_fightseed = luaL_checkinteger(L, 1);
	return 1;
}

int 
lfight_rand(lua_State *L)
{
	int top = lua_gettop(L);
	if (top != 1)
		return 0;

	unsigned long max = luaL_checkinteger(L, 1);
	m_fightseed = m_fightseed * 1103515245 + 12345;
	unsigned long rand = m_fightseed / 65536 % 32768 % max;
	lua_pushinteger(L, rand);
	return 1;
}
//////////////////////////////////////////////////////////////////////////

int
luaopen_random(lua_State *L) {
	luaL_checkversion(L);
	luaL_Reg l[] = {
		{ "seed", lseed },
		{ "rand", lrand },
		{ "fight_seed", lfight_seed },
		{ "fight_rand", lfight_rand },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	return 1;
}
