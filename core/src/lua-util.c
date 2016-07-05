#include <lua.h>
#include <lauxlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "iconv.h"
#include "com_encrypt.h"


int
_microtime_str(lua_State *L) {
	time_t mcr_time;
	time_t sec_time;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	mcr_time = tv.tv_usec;
	sec_time = tv.tv_sec;

	char s[64];
	sprintf(s, "0.%010ld %010ld", mcr_time, sec_time);
	lua_pushstring(L,s);

	return 1;
}

#define MAX_NAME_LEN 50

int
_utf2gbk_len(lua_State *L) {
	size_t size = 0;
	const char * name = lua_tolstring(L, 1, &size);
	if (MAX_NAME_LEN < size) {
		lua_pushnumber(L, -1);
		return 1;
	}
	
	iconv_t handle = iconv_open( "gb2312", "utf-8");
	
	char outbuf[MAX_NAME_LEN] = {0};

	char *in = (char *)name;
	char *out = outbuf;

	size_t outlen = MAX_NAME_LEN;

	int r = iconv(handle, &in, &size, &out, &outlen);
	iconv_close(handle);
	
	if(r == -1) {
		lua_pushnumber(L, -2);
		return 1;
	}
	outlen = strlen(outbuf);
	lua_pushnumber(L, outlen);
	return 1;
}


static struct luaL_Reg utilLib[] = {
  {"authcode", _com_encrypt},
  {"microtime_str", _microtime_str},
  {"base64encode", _base64encode},
  {"base64decode", _base64decode},
  {"utf2gbk_len", _utf2gbk_len},
  {NULL, NULL}
};

int
luaopen_util_core(lua_State *L) {
	lua_createtable(L, 0, (sizeof(utilLib)) / sizeof(luaL_Reg) - 1);
	luaL_setfuncs(L, utilLib, 0);
	return 1;
}