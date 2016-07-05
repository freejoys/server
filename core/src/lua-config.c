#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "rwlock.h"

#define DEFAULT_SIZE 64

struct data {
	void * ptr;
	int ref;
};

struct node {
	uint32_t keyhash;
	char * key;
	int size;
	struct data * value;
	int next;
};

struct table {
	int sizehash;
	struct node * hash;
};

static struct table *G_TABLE = NULL;
static struct rwlock G_LOCK;

static uint32_t
calchash(const char * str, size_t l) {
	uint32_t h = (uint32_t)l;
	size_t l1;
	size_t step = (l >> 5) + 1;
	for (l1 = l; l1 >= step; l1 -= step) {
		h = h ^ ((h<<5) + (h>>2) + (uint8_t)(str[l1 - 1]));
	}
	return h;
}

static int
ltable_singleton(lua_State *L) {
	rwlock_rlock(&G_LOCK);
	if (G_TABLE == NULL) {
		rwlock_runlock(&G_LOCK);
		rwlock_wlock(&G_LOCK);

		G_TABLE = malloc(sizeof(*G_TABLE));
		G_TABLE->sizehash = DEFAULT_SIZE;
		G_TABLE->hash = malloc(sizeof(struct node) * G_TABLE->sizehash);
		memset(G_TABLE->hash,0,sizeof(struct node) * G_TABLE->sizehash);
		int i;
		for(i = 0;i < G_TABLE->sizehash;i++) {
			G_TABLE->hash[i].next = -1;
		}
		rwlock_wunlock(&G_LOCK);
		return 1;
	} else {
		lua_pushlightuserdata(L,G_TABLE);
		rwlock_runlock(&G_LOCK);
		return 1;
	}
}


static struct data *
table_search(struct table * tbl,const char * str,int size) {
	uint32_t keyhash = calchash(str,size);
	struct node * n = &tbl->hash[keyhash % tbl->sizehash];
	//对应hash值的node的key为空,证明此hash值链表不存在
	if (n->key == NULL) {
		return NULL;
	}

	for(;;) {
		//key长度和key都相等,侧为查找结果
		if (n->size == size && memcmp(str,n->key,size) == 0) {
			return n->value;
		}
		if (n->next == -1)
			return NULL;
		//链表中下一个node
		n = &tbl->hash[n->next];
	}
}

static int
releaseobj(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_getfield(L,1,"data");
	struct data *d = lua_touserdata(L,-1);
	if (__sync_sub_and_fetch(&d->ref, 1) == 0) {
		lua_getfield(L,1,"function");
		lua_pushlightuserdata(L,d->ptr);
		if (lua_pcall(L,1,0,0) != LUA_OK) {
			luaL_error(L,lua_tostring(L,-1));
		}
		free(d);
	}
	return 0;
}

static int
ltable_search(lua_State *L) {
	rwlock_rlock(&G_LOCK);

	assert(G_TABLE != NULL);

	luaL_checktype(L, 1, LUA_TSTRING);
	size_t size;
	const char * key = lua_tolstring(L,1,&size);

	struct data * d = table_search(G_TABLE,key,size);
	if (d == NULL)
		return 0;

	__sync_add_and_fetch(&d->ref, 1);

	lua_newtable(L);
	lua_pushlightuserdata(L,d->ptr);
	lua_setfield(L,-2,"obj");
	lua_pushlightuserdata(L,d);
	lua_setfield(L,-2,"data");
	lua_pushvalue(L,2);
	lua_setfield(L,-2,"function");

	if (luaL_newmetatable(L, "meta")) {
		lua_pushcfunction(L, releaseobj);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);

	rwlock_runlock(&G_LOCK);
	return 1;
}

static int
table_add(lua_State *L, struct table * tbl,const char * str,int size,struct data * value) {
	uint32_t keyhash = calchash(str,size);
	struct node * mainpos = &tbl->hash[keyhash % tbl->sizehash];
	//对应hash值的node已被占?
	if (mainpos->value != NULL) {
		struct node * n = mainpos;
		for(;;) {
			//对应的node的key是否相等?相等的话报错，不支持key相等时再add，只能是update
			if (n->size == size && memcmp(n->key,str,size) == 0) {
				return -1;
			}
			if (n->next == -1) {
				break;
			}
			//查找相同hash值组成的链表的下个node
			n = &tbl->hash[n->next];
		}

		int empty = -1;
		int i;
		//找到链表中最后一个node,然后从0开始查找一个空node,因为add不频繁，就线性查找
		for(i = 0;i < tbl->sizehash;i++) {
			if (tbl->hash[i].value == NULL) {
				empty = i;
				break;
			}
		}
		//没有空node,重新hash
		if (empty == -1) {
			int osize = tbl->sizehash;
			struct node * ohash = tbl->hash;
			tbl->sizehash = osize * 2;
			tbl->hash = malloc(sizeof(struct node) * tbl->sizehash);
			memset(tbl->hash,0,sizeof(struct node) * tbl->sizehash);
			int i;
			for(i = 0;i < tbl->sizehash;i++) {
				tbl->hash[i].next = -1;
			}

			for(i = 0;i < osize;i++) {
				if (ohash[i].value != NULL) {
					assert(table_add(L,tbl,ohash[i].key,ohash[i].size,ohash[i].value) == 0);
				}
			}
			assert(table_add(L,tbl,str,size,value) == 0);
			for(i = 0;i < osize;i++) {
				if (ohash[i].value != NULL) {
					free(ohash[i].key);
					if (__sync_sub_and_fetch(&ohash[i].value->ref,1) == 0) {
						lua_pushvalue(L,3);
						lua_pushlightuserdata(L,ohash[i].value->ptr);
						if (lua_pcall(L,1,0,0) != LUA_OK) {
							assert(0);
						}
						free(ohash[i].value);
					}
				}
			}
			free(ohash);

			return 0;
		} else {
			//找到空node,赋值后链到链表上
			__sync_add_and_fetch(&value->ref, 1);
			n->next = empty;
			tbl->hash[empty].value = value;
			tbl->hash[empty].keyhash = keyhash;
			tbl->hash[empty].next = -1;
			tbl->hash[empty].key = malloc(size+1);
			memcpy(tbl->hash[empty].key,str,size);
			tbl->hash[empty].key[size] = '\0';
			tbl->hash[empty].size = size;
			return 0;
		}
	
	} else {
		//直接在空node上赋值
		__sync_add_and_fetch(&value->ref, 1);
		mainpos->value = value;
		mainpos->keyhash = keyhash;
		mainpos->next = -1;
		mainpos->key = malloc(size+1);
		memcpy(mainpos->key,str,size);
		mainpos->key[size] = '\0';
		mainpos->size = size;
		return 0;
	}
}

static int
ltable_add(lua_State *L) {
	rwlock_wlock(&G_LOCK);

	assert(G_TABLE != NULL);
	luaL_checktype(L, 1, LUA_TSTRING);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);

	size_t size;
	const char * key = lua_tolstring(L,1,&size);
	void * ptr = lua_touserdata(L,2);

	struct data * value = malloc(sizeof(*value));
	value->ptr = ptr;
	value->ref = 0;

	if (table_add(L,G_TABLE,key,size,value) == -1) {
		rwlock_wunlock(&G_LOCK);
		luaL_error(L,"Config %s already add",key);
	}

	rwlock_wunlock(&G_LOCK);
	return 0;
}

static struct data *
table_update(struct table * tbl,const char * str,int size,void *ptr) {
	uint32_t keyhash = calchash(str,size);
	struct node * n = &tbl->hash[keyhash % tbl->sizehash];
	if (n->key == NULL)
		return NULL;

	for(;;) {
		if (n->size == size && memcmp(str,n->key,size) == 0)
			break;
		if (n->next == -1)
			return NULL;
		n = &tbl->hash[n->next];
	}
	struct data *od = n->value;
	struct data *nd = malloc(sizeof(*nd));
	nd->ptr = ptr;
	nd->ref = 0;
	__sync_add_and_fetch(&nd->ref, 1);
	n->value = nd;
	return od;
}

static int
ltable_update(lua_State *L) {
	rwlock_wlock(&G_LOCK);

	assert(G_TABLE != NULL);
	luaL_checktype(L, 1, LUA_TSTRING);
	luaL_checktype(L, 2, LUA_TLIGHTUSERDATA);

	size_t size;
	const char * key = lua_tolstring(L,1,&size);
	void * value = lua_touserdata(L,2);

	struct data *od = table_update(G_TABLE,key,size,value);
	if (od == NULL) {
		rwlock_wunlock(&G_LOCK);
		luaL_error(L,"Config update error,key:%s not exist",key);
	}	
	if (__sync_sub_and_fetch(&od->ref,1) == 0) {
		lua_pushvalue(L,3);
		lua_pushlightuserdata(L,od->ptr);
		if (lua_pcall(L,1,0,0) != LUA_OK) {
			rwlock_wunlock(&G_LOCK);
			luaL_error(L,lua_tostring(L,-1));
		}
		free(od);
	}
	rwlock_wunlock(&G_LOCK);
	return 0;
}

static int
ltable_dump(lua_State *L) {
	assert(G_TABLE != NULL);
	int i;
	for(i=0;i<G_TABLE->sizehash;i++) {
		if (G_TABLE->hash[i].key != NULL) {
			printf("key:%-15s hash:%-15u pos:%-3d next:%-3d value:%p ref:%d\n",G_TABLE->hash[i].key,G_TABLE->hash[i].keyhash,i,G_TABLE->hash[i].next,G_TABLE->hash[i].value->ptr,G_TABLE->hash[i].value->ref);
		}
	}
	return 0;
}

static int
ltable_all(lua_State *L) {
	rwlock_rlock(&G_LOCK);

	lua_newtable(L);
	int i;
	for(i=0;i<G_TABLE->sizehash;i++) {
		if (G_TABLE->hash[i].key != NULL && G_TABLE->hash[i].value != NULL) {
			lua_pushlightuserdata(L,G_TABLE->hash[i].value->ptr);
			lua_setfield(L,1,G_TABLE->hash[i].key);
		}
	}
	rwlock_runlock(&G_LOCK);

	return 1;
}

int
luaopen_config_core(lua_State *L) {
	luaL_Reg l[] = {
		{ "singleton",ltable_singleton },
		{ "add",ltable_add },
		{ "search",ltable_search },
		{ "update",ltable_update },
		{ "dump",ltable_dump },
		{ "all",ltable_all },
		{ NULL, NULL },
	};
	luaL_checkversion(L);
	luaL_newlib(L, l);

	return 1;
}