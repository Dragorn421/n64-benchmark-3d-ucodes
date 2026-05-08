// SPDX-FileCopyrightText: 2026 Dragorn421
// SPDX-License-Identifier: CC0-1.0

#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>

#include <libdragon.h>

#define F3DEX_GBI_2
#include "libultra/gbi.h"

#include "f3dex2_exec.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

struct GfxCtx {
  alignas(16) Gfx workBuffer[1000];
};

struct RuntimeGeoCtx {
  Gfx dl[5000];
  Vtx verts[20000];
};

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
  mtx->intPart[3][3] = 1;
  mtx->fracPart[3][3] = 0;
}

#define VTX(x, y, z, s, t, crnx, cgny, cbnz, a)                                \
  {                                                                            \
    {                                                                          \
      {x, y, z}, 0, {s, t}, { crnx, cgny, cbnz, a }                            \
    }                                                                          \
  }

void push_vtx(Vtx **verts_p, int16_t x, int16_t y) {
  **verts_p = (Vtx)VTX(x, y, 0, 0, 0, 0, 0, 0, 0);
  (*verts_p)++;
}

void generate_geometry(struct RuntimeGeoCtx *runtime_geo_ctx, int load_amount) {
  Gfx *dl = runtime_geo_ctx->dl;
  Vtx *verts_p = runtime_geo_ctx->verts;

  gDPSetCycleType(dl++, G_CYC_1CYCLE);
  gDPSetRenderMode(dl++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
  gDPSetCombineMode(dl++, G_CC_PRIMITIVE, G_CC_PRIMITIVE);
  gDPSetPrimColor(dl++, 0, 0, 255, 255, 255, 255);
  gSPLoadGeometryMode(dl++, 0);

  push_vtx(&verts_p, 0, -1024);
  push_vtx(&verts_p, -1024, 1024);
  push_vtx(&verts_p, 1024, 1024);

  gSPVertex(dl++, runtime_geo_ctx->verts, 3, 0);
  gSP1Triangle(dl++, 0, 1, 2, 0);

  gSPEndDisplayList(dl++);
}

int main() {
  debug_init_isviewer();

  joypad_init();

  display_init((resolution_t){SCREEN_WIDTH, SCREEN_HEIGHT}, DEPTH_16_BPP, 2,
               GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);

  struct GfxCtx *gfx_ctx_buf =
      aligned_alloc(alignof(struct GfxCtx),
                    sizeof(struct GfxCtx) * display_get_num_buffers());
  int next_gfx_ctx_i = 0;
  // We don't necessarily change RuntimeGeoCtx every frame, but it is possible,
  // hence N-buffering
  struct RuntimeGeoCtx *runtime_geo_ctx_buf =
      aligned_alloc(alignof(struct RuntimeGeoCtx),
                    sizeof(struct RuntimeGeoCtx) * display_get_num_buffers());
  int next_runtime_geo_ctx_i = 0;

  f3dex2_exec_init();

  static Vp vp = {{
      {(SCREEN_WIDTH / 2) * 4, (SCREEN_HEIGHT / 2) * 4, G_MAXZ / 2, 0},
      {(SCREEN_WIDTH / 2) * 4, (SCREEN_HEIGHT / 2) * 4, G_MAXZ / 2, 0},
  }};
  static Mtx projMtx;
  set_mtx_scale(&projMtx, 1.0f);
  data_cache_hit_writeback(&projMtx, sizeof(projMtx));
  static Mtx modelViewMtx;
  set_mtx_scale(&modelViewMtx, 1.0f / 1024);
  data_cache_hit_writeback(&modelViewMtx, sizeof(modelViewMtx));

  int load_amount = 1;
  struct RuntimeGeoCtx *runtime_geo_ctx = NULL;

  while (true) {
    joypad_poll();

    joypad_buttons_t input = joypad_get_buttons(JOYPAD_PORT_1);
    if (input.d_up) {
      load_amount *= 2;
      runtime_geo_ctx = NULL;
    }
    if (input.d_down) {
      load_amount /= 2;
      if (load_amount < 1) {
        load_amount = 1;
      }
      runtime_geo_ctx = NULL;
    }

    if (runtime_geo_ctx == NULL) {
      runtime_geo_ctx = &runtime_geo_ctx_buf[next_runtime_geo_ctx_i];
      next_runtime_geo_ctx_i++;
      next_runtime_geo_ctx_i %= display_get_num_buffers();
      generate_geometry(runtime_geo_ctx, load_amount);
    }

    surface_t *surf = display_get();

    struct GfxCtx *gfx_ctx = &gfx_ctx_buf[next_gfx_ctx_i];
    next_gfx_ctx_i++;
    next_gfx_ctx_i %= display_get_num_buffers();

    Gfx *work = gfx_ctx->workBuffer;

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

    gSPDisplayList(work++, runtime_geo_ctx->dl);

    gDPFullSync(work++);

    gSPEndDisplayList(work++);

    f3dex2_exec_task(gfx_ctx->workBuffer, work, (void *)display_show, surf);
  }
}
