PLAT ?= none
PLATS = linux macosx

CC ?= gcc

.PHONY: none $(PLATS) clean all cleanall

#ifneq ($(PLAT), none)

.PHONY: default

default:
	$(MAKE) $(PLAT)

#endif

none:
	@echo "Please do 'make PLATFORM' where PLATFORM is one of these: "
	@echo "  $(PLATS)"

linux: PLAT = linux
macosx: PLAT = macosx

linux macosx:
	make all PLAT=$@

