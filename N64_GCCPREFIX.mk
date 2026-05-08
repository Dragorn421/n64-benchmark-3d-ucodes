# SPDX-FileCopyrightText: 2026 Dragorn421
# SPDX-License-Identifier: CC0-1.0

ifeq ($(N64_GCCPREFIX),)
  ifeq ($(N64_INST),)
    $(error Neither N64_GCCPREFIX nor N64_INST is set)
  endif
  export N64_GCCPREFIX ?= $(N64_INST)
  $(info N64_GCCPREFIX set to $(N64_GCCPREFIX) (from N64_INST))
else
  $(info N64_GCCPREFIX kept as $(N64_GCCPREFIX))
endif
