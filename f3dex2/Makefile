# SPDX-FileCopyrightText: 2026 Dragorn421
# SPDX-License-Identifier: CC0-1.0

ROMNAME := benchmark3d_f3dex2
ROM_TITLE := "$(ROMNAME)"

include ../N64_GCCPREFIX.mk
$(shell mkdir -p build/libdragon)
N64_INST := $(realpath build/libdragon)
$(info Using N64_INST = $(N64_INST))
ifeq ($(wildcard $(N64_INST)/include/n64.mk),)
  $(error You must first build libdragon by running ./build_libdragon.sh)
endif

all: $(ROMNAME).z64
.PHONY: all

BUILD_DIR = build
include $(N64_INST)/include/n64.mk

C_FILES = $(shell find src -name '*.c')
S_FILES = $(shell find src -name '*.S')
OBJS = $(addprefix $(BUILD_DIR)/,$(C_FILES:.c=.o) $(S_FILES:.S=.o))

$(ROMNAME).z64: N64_ROM_TITLE = $(ROM_TITLE)

$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) *.z64
.PHONY: clean

-include $(wildcard $(BUILD_DIR)/*.d)
