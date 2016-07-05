LUA_CLIB_PATH ?= $(OUT_DIR)/luaclib
CSERVICE_PATH ?= $(OUT_DIR)/cservice
SKYNET_PATH = ../skynet
THIRD_PATH = ../3rd
SRC_PATH = .
LUA_INC ?= $(SKYNET_PATH)/3rd/lua

CFLAGS = -g -Wall -fPIC -I$(LUA_INC) $(MYCFLAGS)
CXXFLAGS = -g -Wall -fPIC 
# CFLAGS += -DUSE_PTHREAD_LOCK
SHARED = --shared

CSERVICE = 
LUA_CLIB = aoi config pack time pathfinder random util 


all : \
  $(foreach v, $(CSERVICE), $(CSERVICE_PATH)/$(v).so) \
  $(foreach v, $(LUA_CLIB), $(LUA_CLIB_PATH)/$(v).so) 
	echo $(OUT_DIR)

$(LUA_CLIB_PATH) :
	mkdir $(LUA_CLIB_PATH)

$(CSERVICE_PATH) :
	mkdir $(CSERVICE_PATH)

# game add, authcode 
$(LUA_CLIB_PATH)/aoi.so : $(SRC_PATH)/lua-aoi.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@

$(LUA_CLIB_PATH)/config.so : $(SRC_PATH)/lua-config.c
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(SKYNET_PATH)/skynet-src 

$(LUA_CLIB_PATH)/pack.so : $(SRC_PATH)/lua-pack.c
	$(CC) $(CFLAGS) $(SHARED) -I$(SKYNET_PATH)/skynet-src $^ -o $@

$(LUA_CLIB_PATH)/time.so : $(SRC_PATH)/lua-time.c
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@

$(LUA_CLIB_PATH)/pathfinder.so : $(SRC_PATH)/pathfinder/lua-pathfinder.c $(SRC_PATH)/pathfinder/minheap.c
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@

$(LUA_CLIB_PATH)/random.so : $(SRC_PATH)/lua-random.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ 	

$(LUA_CLIB_PATH)/util.so : $(SRC_PATH)/lua-util.c $(SRC_PATH)/authcode/base64.o \
  	$(SRC_PATH)/authcode/com_encrypt.o \
  	$(SRC_PATH)/authcode/md5.c \
  	$(SRC_PATH)/authcode/md5cpp.o \
  	$(SRC_PATH)/authcode/proto-encrypt.c 
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -I$(SRC_PATH)/authcode -lstdc++

clean :
	rm -f $(LUA_CLIB_PATH)/*.so 
