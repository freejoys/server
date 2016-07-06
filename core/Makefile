include platform.mk

MAKE=make
SKYNET_DIR=skynet
OUT_DIR=out
SKYNET_OUT_DIR=bin
GAMELIB_OUT_DIR=lib
OUT_FILE=$(PLAT).zip

.PHONY: skynet src 3rd all clean lib_dir

lib_dir:
	mkdir -p $(OUT_DIR)/$(GAMELIB_OUT_DIR)/luaclib
	mkdir -p $(OUT_DIR)/$(GAMELIB_OUT_DIR)/cservice

skynet: 
	cd $(SKYNET_DIR) && $(MAKE) $(PLAT)
	mkdir -p $(OUT_DIR)/$(SKYNET_OUT_DIR)
	cp $(SKYNET_DIR)/skynet $(SKYNET_DIR)/cservice $(SKYNET_DIR)/service $(SKYNET_DIR)/lualib $(SKYNET_DIR)/luaclib $(OUT_DIR)/$(SKYNET_OUT_DIR) -r
	
src: lib_dir
	cd src && $(MAKE) OUT_DIR=../$(OUT_DIR)/$(GAMELIB_OUT_DIR)

3rd: lib_dir
	cd 3rd && $(MAKE) all OUT_DIR=../$(OUT_DIR)/$(GAMELIB_OUT_DIR)

all: skynet src 3rd
	cd $(OUT_DIR) && zip $(OUT_FILE) $(SKYNET_OUT_DIR) $(GAMELIB_OUT_DIR) -r

clean:
	rm -fr $(OUT_DIR)
