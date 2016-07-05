include platform.mk


MAKE=make
SKYNET_DIR=3rd/skynet
OUT_DIR=out
SKYNET_OUT_DIR=bin
GAMELIB_OUT_DIR=lib
OUT_FILE=$(PLAT).zip

update3rd:
	git submodule update --init

skynet: update3rd
	cd 3rd/skynet && $(MAKE) linux

all: skynet
	rm -fr $(OUT_DIR)
	mkdir -p $(OUT_DIR)/$(SKYNET_OUT_DIR)
	mkdir -p $(OUT_DIR)/$(GAMELIB_OUT_DIR)
	cp $(SKYNET_DIR)/skynet $(SKYNET_DIR)/cservice $(SKYNET_DIR)/service $(SKYNET_DIR)/lualib $(SKYNET_DIR)/luaclib $(OUT_DIR)/$(SKYNET_OUT_DIR) -r
	cd $(OUT_DIR) && zip $(OUT_FILE) $(SKYNET_OUT_DIR) $(GAMELIB_OUT_DIR) -r

clean:
	cd $(SKYNET_DIR) && make clean
	rm -fr $(OUT_DIR)
