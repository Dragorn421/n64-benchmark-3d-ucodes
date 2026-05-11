# SPDX-FileCopyrightText: 2026 Dragorn421
# SPDX-License-Identifier: CC0-1.0

ROMNAME := benchmark3d_tiny3d
ROM_TITLE := "$(ROMNAME)"

include ../N64_GCCPREFIX.mk
$(shell mkdir -p build/libdragon)
N64_INST := $(realpath build/libdragon)
$(info Using N64_INST = $(N64_INST))
ifeq ($(wildcard $(N64_INST)/include/n64.mk),)
  $(error You must first build libdragon by running ./build_libdragon_and_tiny3d.sh)
endif
ifeq ($(wildcard $(N64_INST)/include/t3d.mk),)
  $(error You must first build tiny3d by running ./build_libdragon_and_tiny3d.sh)
endif

all: $(ROMNAME).z64
.PHONY: all

BUILD_DIR = build
include $(N64_INST)/include/n64.mk
include $(N64_INST)/include/t3d.mk

C_FILES = $(shell find src -name '*.c')
OBJS = $(addprefix $(BUILD_DIR)/,$(C_FILES:.c=.o))

$(ROMNAME).z64: N64_ROM_TITLE = $(ROM_TITLE)

define suzanne100Kt3dm
ifeq ($$(wildcard assets/build/Suzanne100K/$(1:.t3dm=.glb)),)
  $$(error You must first build the assets by running cd assets && make)
endif
$$(BUILD_DIR)/dfs/$(1): assets/build/Suzanne100K/$(1:.t3dm=.glb)
	mkdir -p $$(@D)
	$$(T3D_GLTF_TO_3D) $$< $$@
$$(BUILD_DIR)/dfs.dfs: $$(BUILD_DIR)/dfs/$(1)
endef

$(eval $(call suzanne100Kt3dm,Suzanne1K.t3dm))
$(eval $(call suzanne100Kt3dm,Suzanne10K.t3dm))
$(eval $(call suzanne100Kt3dm,Suzanne50K.t3dm))
N64_MKDFS_ROOT := $(BUILD_DIR)/dfs
$(ROMNAME).z64: $(BUILD_DIR)/dfs.dfs

$(BUILD_DIR)/$(ROMNAME).elf: $(OBJS)

clean:
	rm -rf $(BUILD_DIR) *.z64
.PHONY: clean

-include $(shell find $(BUILD_DIR) -name '*.d')
