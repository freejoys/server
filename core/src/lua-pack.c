#include <lua.h>
#include <lauxlib.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "skynet_malloc.h"


/*
C pack:
short:整个消息包长
short:包索引
short:消息id,
string:消息包体

整个消息包长:
		前端要带此字段，不过来到后端时在socket_server.c里已经读完，来此文件
		时已经没有此字段.
包索引:
		是前端对于当前登录必须自增的一个索引,重登后清0,
		大量刷包工具都是拿到一个整包,循环往后端发。发现此字段非自增侧T掉连接.

前端发来的包要用指定的key和原来的包做xor,提高前端刷包门槛,
服务端拿到整包后用一个指定的key做xor操作得到原来包


S pack:
short:整个消息包长
short:消息id,
uint8:此包有有多少个小包,
uint8:小包索引
string:消息包体
*/


/*
arg 1:userdata,arg 2:长度,arg 3:key
arg 1:string,arg 2:key
若是userdata,直接改变这个userdata的内容,
若是string,返回一个新string
*/
int
_xor_encrypt(lua_State *L) {
	int index;
	char * ptr;
	char * result;
	size_t size;
	if (lua_isuserdata(L,1)) {
		ptr = (char *)lua_touserdata(L,1);
		size = luaL_checkinteger(L, 2);
		result = ptr;
		index = 3;
	} else {
		ptr = (char *)luaL_checklstring(L, 1, &size);
		result = skynet_malloc(size);
		memset(result,0,size);
		index = 2;
	}

	size_t keysz;
	const char * key = lua_tolstring(L,index,&keysz);

	int i;
	for (i = 0;i < size;i++) {
		result[i] = ptr[i] ^ key[i%keysz];
	}

	if (!lua_isuserdata(L,1)) {
		lua_pushlstring(L,result,size);
		skynet_free(result);
		return 1;
	}
	return 0;
}

#define RC4_BOX_LEN 255

int
_rc4_box(lua_State *L) {
	size_t size;
	const char * key = (const char *)luaL_checklstring(L, 1, &size);

	unsigned char box[RC4_BOX_LEN] = {0};
	unsigned char rnd_key[RC4_BOX_LEN] = {0};
	
	int i;
	for(i = 0;i < RC4_BOX_LEN;i++) {
		box[i] = i;
		rnd_key[i] = key[i % size];
	}

	int j = 0;
	for(i = 0;i < RC4_BOX_LEN;i++) {
		j = (j + box[i] + rnd_key[i]) % RC4_BOX_LEN;
		unsigned char tmp = box[i];
		box[i] = box[j];
		box[j] = tmp;
	}

	lua_pushlstring(L,(const char *)box,RC4_BOX_LEN);
	return 1;
}

/*
读取包头的消息id和索引
*/
int
_read_pack_head(lua_State *L) {
	if (lua_isuserdata(L,1) == 0) {
		luaL_error(L,"_read_pack_head:Must be userdata");
		return 0;
	}
	
	const char * ptr = (const char *)lua_touserdata(L,1);
	int size = luaL_checkinteger(L, 2);

	if (size < sizeof(short) * 2) {
		luaL_error(L,"_read_pack_head:Header size error:%d\n",size);
		return 0;
	}

	const char * key = NULL;
	size_t keysize = 0;
	if (lua_isnil(L,3) == 0) {
		key = lua_tolstring(L, 3, &keysize);
	}

	uint8_t str[4] = {0};
	str[0] = *ptr;
	str[1] = *(ptr+1);
	str[2] = *(ptr+2);
	str[3] = *(ptr+3);

	if (key != NULL) {
		int i;
		for(i=0;i < 4;i++) {
			str[i] = str[i] ^ key[i % keysize];
		}
	}
	
	ushort index = str[0] | (str[1] << 8);
	ushort id = str[2] | (str[3] << 8);

	lua_pushinteger(L,index);
	lua_pushinteger(L,id);

	return 2;
}

/*
读取包消息id和索引，和消息体
同时释放从gate里来的消息
*/
int 
_read_pack(lua_State *L) {
	static int HEADER_SIZE = sizeof(short) * 2; 

	if (lua_isuserdata(L,1) == 0) {
		luaL_error(L,"read pack must be userdata");
		return 0;
	}

	char * ptr = lua_touserdata(L,1);
	int size = lua_tointeger(L,2);
	
	if (size < HEADER_SIZE) {
		skynet_free(ptr);
		luaL_error(L,"_read_pack:Error pack size:%d\n",size);
		return 0;
	}

	lua_pushcfunction(L,_read_pack_head);
	lua_pushvalue(L,1);
	lua_pushvalue(L,2);
	if (lua_pcall(L, 2, 2, 0) != LUA_OK) {
		skynet_free(ptr);
		luaL_error(L,"_read_pack:%s\n",lua_tostring(L,-1));
		return 0;
	}

	if (HEADER_SIZE == size) {
		skynet_free(ptr);
		return 2;
	}
	lua_pushlstring(L,ptr + HEADER_SIZE,size - HEADER_SIZE);
	skynet_free(ptr);
	return 3;
}

//for server
int
_make_server_pack(lua_State *L) {
	int id = luaL_checkinteger(L, 1);
	size_t size = 0;
	const char *message = luaL_checklstring(L, 2, &size);

	struct header {
		ushort len; 		//total length of pack
		ushort id;  		//message id
		uint8_t num;	//pack total num
		uint8_t index;	//pack index,start from 1
	};

	static int MAX_SIZE = 1024 * 60 - sizeof(struct header);

	int pack_num = 1;
	if (size > MAX_SIZE) {
		pack_num = size / MAX_SIZE;
		if (size % MAX_SIZE > 0) {
			pack_num++;
		}
	}
	
	lua_newtable(L);

	int index = 1;
	int offset = 0;
	for(;;) {
		int pack_size = 0;
		if (size - offset > MAX_SIZE) {
			pack_size = MAX_SIZE;
		} else {
			pack_size = size - offset;
		}

		int len = sizeof(struct header) + pack_size;
		struct header * ptr = (struct header *)skynet_malloc(len);
	
		ptr->len = (((len - 2) & 0xff) << 8) | (((len - 2) >> 8 ) & 0xff);
		ptr->id = id;
		ptr->num = pack_num;
		ptr->index = index;
		memcpy((void*)(ptr+1),message + offset,pack_size);

		lua_newtable(L);
		lua_pushlightuserdata(L,ptr);
		lua_setfield(L,-2,"ptr");
		lua_pushinteger(L,len);
		lua_setfield(L,-2,"len");
		lua_rawseti(L,-2,index);

		index++;
		offset += pack_size;

		if (size - offset <= 0) 
			break;
	}

	return 1;
}

//for client
int
_make_client_pack(lua_State *L) {
	int id = lua_tointeger(L, 1);
	int index = lua_tointeger(L,2);
	size_t sz = 0;
	const char * body = lua_tolstring(L, 3, &sz);

	int len = sizeof(short) * 3 + sz;

	char * pack = skynet_malloc(len);
	char * ptr = pack;
	ptr[0] = (len-2) >> 8;
	ptr[1] = (char)(len-2);
	ptr += 2;
	memcpy(ptr,(char*)&index,2);
	ptr += 2;
	memcpy(ptr,(char*)&id,2);
	ptr += 2;

	if (sz != 0 ) {
		memcpy(ptr,body,sz);
	}
	
	#if 0
	lua_pushcfunction(L,_xor_encrypt);
	lua_pushlightuserdata(L,pack + sizeof(short));
	lua_pushinteger(L,len - sizeof(short));
	lua_pushvalue(L,4);
	if (lua_pcall(L, 3, 1, 0) != LUA_OK) {
		size_t size = 0;
		const char * error = lua_tolstring(L, -1, &size);
		luaL_error(L,"_make_client_pack:xor encrypt:%s\n",error);
		return 0;
	}
	#endif

	lua_pushlightuserdata(L,pack);
	lua_pushinteger(L,len);
	return 2;
}

int
_littleendian(lua_State *L) {
	size_t len = 0;
	char * stream = (char *)lua_tolstring(L,1,&len);
	uint8_t header[2] = {0};
	header[0] = (uint8_t)stream[0];
	header[1] = (uint8_t)stream[1];
	short value = header[0] << 8 | header[1];
	lua_pushinteger(L,value);
	return 1;
}

int
_bytes2integer(lua_State *L) {
	size_t len = 0;
	char * strv = (char *)lua_tolstring(L,1,&len);
	if (len != 2 && len != 4) {
		luaL_error(L, "to integer %d\n",len);
		return 0;
	}

	int value = 0;
	memcpy(&value,strv,len);
	lua_pushinteger(L,value);
	return 1;
}

static int
_decode_client_msg(lua_State* L) {
	int type = lua_type(L, 1);
	const char* buffer;
	size_t sz;
	if (type == LUA_TSTRING) {
		buffer = lua_tolstring(L, 1, &sz);
	} else {
		buffer = lua_touserdata(L, 1);
		sz = lua_tointeger(L, 2);
	}

	if (sz < 2) {
		return 0;
	}

	unsigned short msg_id = *((unsigned short*)buffer);
	lua_pushinteger(L, (int)msg_id);
	lua_pushlstring(L, buffer + 2, sz - 2);
	return 2;
}

static int 
_encode_client_msg(lua_State *L) {
	if (lua_isnoneornil(L,1)) {
		return 0;
	}
	size_t buf_len = 0;
	int method_id = luaL_checkinteger(L,1);
	const char *buffer = lua_tolstring(L, 2, &buf_len);
	int total_len = sizeof(short) * 2 + buf_len;

	int msg_len = buf_len + sizeof(short);
	short low = msg_len & 0xff;
	short high = (msg_len >> 8) & 0xff;
	msg_len = (low << 8) | high;
	char *msg = skynet_malloc(total_len);
	*((short*)msg) = msg_len;//compress flag

	*((short*)(msg + sizeof(short))) = method_id;//compress flag
	memcpy(msg + sizeof(short) * 2, buffer,buf_len);
	lua_pushlightuserdata(L, msg);
	lua_pushinteger(L, total_len);
	return 2;
}


static struct luaL_Reg streamLib[] = {
	{ "xor_encrypt", _xor_encrypt },
	{ "rc4_box", _rc4_box },
	{ "read_pack", _read_pack },
	{ "read_pack_head", _read_pack_head },
	{ "make_server_pack", _make_server_pack },
	{ "make_client_pack", _make_client_pack },
	{ "littleendian", _littleendian },
	{ "bytes2integer", _bytes2integer },
	{ "decode_client_msg", _decode_client_msg},
	{ "encode_client_msg", _encode_client_msg},
  	{NULL, NULL}
};

int luaopen_pack(lua_State *L) {
	luaL_checkversion(L);
	lua_createtable(L, 0, (sizeof(streamLib)) / sizeof(luaL_Reg) - 1);
	luaL_setfuncs(L, streamLib, 0);
	return 1;
}