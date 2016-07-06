LUA_CLIB_PATH ?= $(OUT_DIR)/luaclib
SKYNET_PATH = ../skynet
LUA_INC ?= $(SKYNET_PATH)/3rd/lua

CFLAGS = -g -Wall -fPIC -I$(LUA_INC) $(MYCFLAGS)
CXXFLAGS = -g -Wall -fPIC 
# CFLAGS += -DUSE_PTHREAD_LOCK
SHARED = --shared

LUA_CLIB = protobuf cjson lfs

# pbc
PBC_BUILD := pbc/build
PBC_STATICLIB := pbc/build/libpbc.a
PBC_INC := pbc

$(PBC_STATICLIB) :
	cd pbc && $(MAKE) CC=$(CC) lib

all : \
  $(foreach v, $(LUA_CLIB), $(LUA_CLIB_PATH)/$(v).so) 

$(LUA_CLIB_PATH) :
	mkdir $(LUA_CLIB_PATH)

# 3rd
$(LUA_CLIB_PATH)/protobuf.so : pbc/binding/lua53/pbc-lua53.c $(PBC_STATICLIB)
	$(CC) $(CFLAGS) $(SHARED) -I$(PBC_INC) $^ -o $@

$(LUA_CLIB_PATH)/cjson.so : lua-cjson/lua_cjson.c lua-cjson/strbuf.c lua-cjson/fpconv.c
	$(CC) $(CFLAGS) $(SHARED) -I$lua-cjson $^ -o $@ 

$(LUA_CLIB_PATH)/lfs.so : luafilesystem/src/lfs.c
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@
#	cd luafilesystem && $(MAKE) CC=$(CC)
#	cp luafilesystem/src/lfs.so ./$(LUA_CLIB_PATH)
