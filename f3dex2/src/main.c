// SPDX-FileCopyrightText: 2026 Dragorn421
// SPDX-License-Identifier: CC0-1.0

#include <libdragon.h>

#define F3DEX_GBI_2
#include "libultra/gbi.h"

#include "f3dex2_exec.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

alignas(16) Gfx workBuffer[1000];

int main() {
  debug_init_isviewer();

  display_init((resolution_t){SCREEN_WIDTH, SCREEN_HEIGHT}, DEPTH_16_BPP, 2,
               GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);

  f3dex2_exec_init();

  while (true) {
    surface_t *surf = display_get();

    Gfx *work = workBuffer;

    gDPSetScissor(work++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH,
                  SCREEN_HEIGHT);

    gDPSetColorImage(work++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                     surf->buffer);

    gDPSetCycleType(work++, G_CYC_FILL);
    gDPSetRenderMode(work++, G_RM_NOOP, G_RM_NOOP2);
    u8 r = abs((get_ticks_ms() / 15) % 200 - 100), g = 100, b = 50;
    gDPSetFillColor(work++, (GPACK_RGBA5551(r, g, b, 1) << 16) |
                                GPACK_RGBA5551(r, g, b, 1));
    gDPFillRectangle(work++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    gDPPipeSync(work++);

    gDPFullSync(work++);

    gSPEndDisplayList(work++);

    f3dex2_exec_task(workBuffer, work, (void *)display_show, surf);
  }
}
