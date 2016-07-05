/*
 * utility.c
 *
 *  Created on: 2013年8月14日
 *      Author: cai
 */

#include <lua.h>
#include <lauxlib.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

static inline int 
g_localtime(const time_t time, struct tm *t) {
	if (t == NULL)
		return -1;

	memset(t, 0, sizeof(*t));

	localtime_r(&time, t);
	return 0;
}

int 
_same_day(lua_State *L) {
	time_t t1 = luaL_checkinteger(L, 1);
	time_t t2 = luaL_checkinteger(L, 2);

	struct tm p1;
	if (g_localtime(t1, &p1) < 0)
		return 0;

	struct tm p2;
	if (g_localtime(t2, &p2) < 0)
		return 0;

	int ret;
	if (p1.tm_year == p2.tm_year && p1.tm_mon == p2.tm_mon && p1.tm_mday == p2.tm_mday)
		ret = 1;
	else
		ret = 0;

	lua_pushboolean(L, ret);
	return 1;
}

int 
_same_week(lua_State *L) {
	time_t t1 = luaL_checkinteger(L, 1);
	time_t t2 = luaL_checkinteger(L, 2);

	struct tm p1;
	if (g_localtime(t1, &p1) < 0)
		return 0;

	struct tm p2;
	if (g_localtime(t2, &p2) < 0)
		return 0;

	int ret = 0;
	if (p1.tm_year == p2.tm_year)
	{
		if (abs(p1.tm_yday - p2.tm_yday) <= 7 && abs(p1.tm_wday - p2.tm_wday) <= 6)
		{
			ret = 1;
		}
	}
	lua_pushboolean(L, ret);
	return 1;
}

int 
_next_midnight(lua_State *L) {
	time_t cur_time = luaL_checkinteger(L, 1);
	struct tm p;
	if (g_localtime(cur_time, &p) < 0)
		return 0;

	p.tm_sec = 0;
	p.tm_min = 0;
	p.tm_hour = 0;

	time_t next_time = mktime(&p);
	if (next_time <= cur_time)
		next_time += 24 * 60 * 60;

	lua_pushinteger(L, next_time);
	return 1;
}

int 
_time_to_year(lua_State *L) {
	time_t tick = luaL_checkinteger(L, 1);
	struct tm tm_time;
	if (g_localtime(tick, &tm_time) < 0)
		return 0;

	lua_pushinteger(L, tm_time.tm_year);
	return 1;
}

int 
_time_to_mon(lua_State *L) {
	time_t tick = luaL_checkinteger(L, 1);
	struct tm tm_time;
	if (g_localtime(tick, &tm_time) < 0)
		return 0;

	lua_pushinteger(L, tm_time.tm_mon);
	return 1;
}

int 
_time_to_yday(lua_State *L) {
	time_t tick = luaL_checkinteger(L, 1);
	struct tm tm_time;
	if (g_localtime(tick, &tm_time) < 0)
		return 0;

	lua_pushinteger(L, tm_time.tm_yday);
	return 1;
}

int 
_time_to_mday(lua_State *L) {
	time_t tick = luaL_checkinteger(L, 1);
	struct tm tm_time;
	if (g_localtime(tick, &tm_time) < 0)
		return 0;

	lua_pushinteger(L, tm_time.tm_mday);
	return 1;
}

int 
_time_to_wday(lua_State *L) {
	time_t tick = luaL_checkinteger(L, 1);
	struct tm tm_time;
	if (g_localtime(tick, &tm_time) < 0)
		return 0;

	lua_pushinteger(L, tm_time.tm_wday);
	return 1;
}

int 
_time_to_hour(lua_State *L) {
	time_t tick = luaL_checkinteger(L, 1);
	struct tm tm_time;
	if (g_localtime(tick, &tm_time) < 0)
		return 0;

	lua_pushinteger(L, tm_time.tm_hour);
	return 1;
}

int 
_time_to_min(lua_State *L) {
	time_t tick = luaL_checkinteger(L, 1);
	struct tm tm_time;
	if (g_localtime(tick, &tm_time) < 0)
		return 0;

	lua_pushinteger(L, tm_time.tm_min);
	return 1;
}

int 
_time_to_sec(lua_State *L) {
	time_t tick = luaL_checkinteger(L, 1);
	struct tm tm_time;
	if (g_localtime(tick, &tm_time) < 0)
		return 0;

	lua_pushinteger(L, tm_time.tm_sec);
	return 1;
}

int
_time_to_date(lua_State *L) {
	time_t tick = luaL_checkinteger(L, 1);
	struct tm tm_time;
	if (g_localtime(tick, &tm_time) < 0)
		return 0;

	lua_newtable(L);
	lua_pushinteger(L,tm_time.tm_year);
	lua_setfield(L,-2,"year");
	lua_pushinteger(L,tm_time.tm_mon + 1);
	lua_setfield(L,-2,"mon");
	lua_pushinteger(L,tm_time.tm_mday);
	lua_setfield(L,-2,"day");
	lua_pushinteger(L,tm_time.tm_hour);
	lua_setfield(L,-2,"hour");
	lua_pushinteger(L,tm_time.tm_min);
	lua_setfield(L,-2,"min");
	lua_pushinteger(L,tm_time.tm_sec);
	lua_setfield(L,-2,"sec");

	return 1;
}

//当天已经过去的秒数
int 
_time_to_daysec(lua_State *L) {
	time_t tick = luaL_checkinteger(L, 1);
	struct tm tm_time;
	if (g_localtime(tick, &tm_time) < 0)
		return 0;

	int daysec = tm_time.tm_hour * 60 * 60 + tm_time.tm_min * 60 + tm_time.tm_sec;
	lua_pushinteger(L, daysec);
	return 1;
}


int luaopen_time_core(lua_State *L)
{
	luaL_checkversion(L);
	luaL_Reg l[] = {
			{ "same_day", _same_day },
			{ "same_week", _same_week },
			{ "next_midnight", _next_midnight },
			{ "time_to_year", _time_to_year },
			{ "time_to_mon", _time_to_mon },
			{ "time_to_yday", _time_to_yday },
			{ "time_to_mday", _time_to_mday },
			{ "time_to_wday", _time_to_wday },
			{ "time_to_hour", _time_to_hour },
			{ "time_to_min", _time_to_min },
			{ "time_to_sec", _time_to_sec },
			{ "time_to_date", _time_to_date },
			{ "time_to_daysec", _time_to_daysec },
			{ NULL, NULL } };

	lua_createtable(L, 0, (sizeof(l)) / sizeof(luaL_Reg) - 1);
	luaL_setfuncs(L, l, 0);
	return 1;
}
