include platform.mk


MAKE=make
SKYNET_DIR=3rd/skynet
OUT=out
OUT_DIR=out/$(PLAT)

update3rd:
	git submodule update --init

skynet:
	cd 3rd/skynet && $(MAKE) linux

all: skynet
	mkdir -p $(OUT_DIR)
	rm -fr $(PLAT)
	cp $(SKYNET_DIR)/skynet $(SKYNET_DIR)/cservice $(SKYNET_DIR)/service $(SKYNET_DIR)/luaclib $(OUT_DIR) -r
	cd $(OUT) && zip $(PLAT).zip $(PLAT) -r
	rm -fr $(PLAT)
