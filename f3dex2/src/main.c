// SPDX-FileCopyrightText: 2026 Dragorn421
// SPDX-License-Identifier: CC0-1.0

#include <libdragon.h>

#define F3DEX_GBI_2
#include "libultra/gbi.h"

#include "f3dex2_exec.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

alignas(16) Gfx workBuffer[1000];

void set_mtx_scale(Mtx *mtx, float scale) {
  int32_t scale_fixed = scale * 0x10000;
  uint16_t scale_int = (scale_fixed >> 16) & 0xFFFF;
  uint16_t scale_frac = scale_fixed & 0xFFFF;
  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      mtx->intPart[j][i] = i == j ? scale_int : 0;
      mtx->fracPart[j][i] = i == j ? scale_frac : 0;
    }
  }
}

int main() {
  debug_init_isviewer();

  display_init((resolution_t){SCREEN_WIDTH, SCREEN_HEIGHT}, DEPTH_16_BPP, 2,
               GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);

  f3dex2_exec_init();

  static Vp vp = {{
      {(SCREEN_WIDTH / 2) * 4, (SCREEN_HEIGHT / 2) * 4, G_MAXZ / 2, 0},
      {(SCREEN_WIDTH / 2) * 4, (SCREEN_HEIGHT / 2) * 4, G_MAXZ / 2, 0},
  }};
  static Mtx projMtx;
  set_mtx_scale(&projMtx, 1.0f);
  data_cache_hit_writeback(&projMtx, sizeof(projMtx));
  static Mtx modelViewMtx;
  set_mtx_scale(&modelViewMtx, 1.0f);
  data_cache_hit_writeback(&modelViewMtx, sizeof(modelViewMtx));
#define VTX(x, y, z, s, t, crnx, cgny, cbnz, a)                                \
  {                                                                            \
    {                                                                          \
      {x, y, z}, 0, {s, t}, { crnx, cgny, cbnz, a }                            \
    }                                                                          \
  }
  static Vtx verts[] = {
      VTX(0, -1, 0, 0, 0, 0, 0, 0, 0),
      VTX(-1, 1, 0, 0, 0, 0, 0, 0, 0),
      VTX(1, 1, 0, 0, 0, 0, 0, 0, 0),
  };

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

    gSPViewport(work++, &vp);
    gSPMatrix(work++, &projMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    // not sure what the logic is but using 2 for PerspNormalize appears to make
    // normalized coordinates work (with proj == modelView == identity a -1,-1
    // to 1,1 bounded triangle takes up the whole screen)
    gSPPerspNormalize(work++, 2);
    gSPMatrix(work++, &modelViewMtx,
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    gDPSetCycleType(work++, G_CYC_1CYCLE);
    gDPSetRenderMode(work++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCombineMode(work++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
    gDPSetPrimColor(work++, 0, 0, 255, 255, 255, 255);
    gSPLoadGeometryMode(work++, 0);

    gSPVertex(work++, verts, 3, 0);
    gSP1Triangle(work++, 0, 1, 2, 0);

    gDPFullSync(work++);

    gSPEndDisplayList(work++);

    f3dex2_exec_task(workBuffer, work, (void *)display_show, surf);
  }
}
