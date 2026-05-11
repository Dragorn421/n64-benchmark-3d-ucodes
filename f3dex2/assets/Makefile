ifeq ($(BLENDER_4_5),)
  $(error Set BLENDER_4_5 to the path to a Blender 4.5 executable)
endif

BLENDER := $(BLENDER_4_5)
ASSETS_BIN :=

all:
.PHONY: all

include ../../N64_GCCPREFIX.mk

define Suzanne100Kc
$(1): ../../assets/Suzanne100K.blend ./export.py
	$(BLENDER) --background $$< --python-exit-code 1 --python ./export.py -- $$@ $(2)
ASSETS_BIN += $(1:.c=.bin)
$(1:.c=.bin): DL_NAME := Suzanne_Suzanne_dl
endef

$(eval $(call Suzanne100Kc,build/Suzanne100K/Suzanne1K.c,0.01))
$(eval $(call Suzanne100Kc,build/Suzanne100K/Suzanne10K.c,0.1))
$(eval $(call Suzanne100Kc,build/Suzanne100K/Suzanne50K.c,0.5))

build/%.o: build/%.c
	mkdir -p $(@D)
	$(N64_GCCPREFIX)/bin/mips64-elf-gcc -I. -D_LANGUAGE_C -DF3DEX_GBI_2 -Wl,-Tldscript.txt $^ -o $@

build/%.bin: build/%.o
	printf '#define $(notdir $*)_$(DL_NAME) 0x%s\n' $$($(N64_GCCPREFIX)/bin/mips64-elf-nm -f bsd $< | grep $(DL_NAME) | cut -d' ' -f1) > $(@:.bin=.h)
	$(N64_GCCPREFIX)/bin/mips64-elf-objcopy -O binary $< $@

.SECONDARY: $(ASSETS_BIN:%.bin=%.o)

all: $(ASSETS_BIN)
