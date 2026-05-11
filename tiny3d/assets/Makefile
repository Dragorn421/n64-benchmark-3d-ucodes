ifeq ($(BLENDER_4_5),)
  $(error Set BLENDER_4_5 to the path to a Blender 4.5 executable)
endif

BLENDER := $(BLENDER_4_5)
ASSETS_GLB :=

all:
.PHONY: all

define Suzanne100Kglb
$(1): ../../assets/Suzanne100K.blend ./export.py
	$(BLENDER) --background $$< --python-exit-code 1 --python ./export.py -- $$@ $(2)
ASSETS_GLB += $(1)
endef

$(eval $(call Suzanne100Kglb,build/Suzanne100K/Suzanne1K.glb,0.01))
$(eval $(call Suzanne100Kglb,build/Suzanne100K/Suzanne10K.glb,0.1))
$(eval $(call Suzanne100Kglb,build/Suzanne100K/Suzanne50K.glb,0.5))

all: $(ASSETS_GLB)
