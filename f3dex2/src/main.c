// SPDX-FileCopyrightText: 2026 Dragorn421
// SPDX-License-Identifier: CC0-1.0

#include <limits.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>

#include <libdragon.h>

#define F3DEX_GBI_2
#include "libultra/gbi.h"

#include "f3dex2_exec.h"

#define ARRAY_COUNT(arr) (s32)(sizeof(arr) / sizeof(arr[0]))

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

alignas(16) uint16_t zbuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

struct on_task_finished_arg_data {
  surface_t *surf;
  int i_used_loaded_model;
};

struct GfxCtx {
  alignas(16) Gfx workBuffer[1000];
  struct on_task_finished_arg_data on_task_finished_arg_data;
};

struct loaded_model {
  bool is_loaded;
  void *data;
  int n_users;
};
struct loaded_model loaded_models[3] = {0};
int i_cur_loaded_model;

void on_task_finished(void *callback_arg) {
  struct on_task_finished_arg_data *arg = callback_arg;
  display_show(arg->surf);
  struct loaded_model *used_model = &loaded_models[arg->i_used_loaded_model];
  assert(used_model->n_users > 0);
  used_model->n_users--;
  if (used_model->n_users == 0 &&
      arg->i_used_loaded_model != i_cur_loaded_model) {
    used_model->is_loaded = false;
    free(used_model->data);
    used_model->data = NULL;
  }
}

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

int main() {
  debug_init_isviewer();

  joypad_init();

  display_init((resolution_t){SCREEN_WIDTH, SCREEN_HEIGHT}, DEPTH_16_BPP, 2,
               GAMMA_NONE, FILTERS_RESAMPLE_ANTIALIAS_DEDITHER);

  dfs_init(DFS_DEFAULT_LOCATION);

  struct GfxCtx *gfx_ctx_buf =
      aligned_alloc(alignof(struct GfxCtx),
                    sizeof(struct GfxCtx) * display_get_num_buffers());
  int next_gfx_ctx_i = 0;

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

  struct {
    const char *filepath;
    uint32_t dl;
  } models[] = {
#include "../assets/build/Suzanne100K/Suzanne1K.h"
      {"rom:/Suzanne1K.bin", Suzanne1K_Suzanne_Suzanne_dl},
#include "../assets/build/Suzanne100K/Suzanne10K.h"
      {"rom:/Suzanne10K.bin", Suzanne10K_Suzanne_Suzanne_dl},
#include "../assets/build/Suzanne100K/Suzanne50K.h"
      {"rom:/Suzanne50K.bin", Suzanne50K_Suzanne_Suzanne_dl},
  };
  int i_cur_model = -1;

  while (true) {
    joypad_poll();

    joypad_buttons_t input = joypad_get_buttons(JOYPAD_PORT_1);
    int i_next_model = i_cur_model;
    if (input.d_up) {
      i_next_model++;
    }
    if (input.d_down) {
      i_next_model--;
    }

    i_next_model %= ARRAY_COUNT(models);
    if (i_next_model < 0) {
      i_next_model += ARRAY_COUNT(models);
    }

    if (i_next_model != i_cur_model) {
      i_cur_model = i_next_model;
      i_cur_loaded_model++;
      i_cur_loaded_model %= ARRAY_COUNT(loaded_models);
      assert(!loaded_models[i_cur_loaded_model].is_loaded);
      loaded_models[i_cur_loaded_model].data =
          asset_load(models[i_cur_model].filepath, NULL);
      loaded_models[i_cur_loaded_model].n_users = 0;
      loaded_models[i_cur_loaded_model].is_loaded = true;
    }

    loaded_models[i_cur_loaded_model].n_users++;

    debugf("%-30s %f fps\n", models[i_cur_model].filepath, display_get_fps());

    surface_t *surf = display_get();

    struct GfxCtx *gfx_ctx = &gfx_ctx_buf[next_gfx_ctx_i];
    next_gfx_ctx_i++;
    next_gfx_ctx_i %= display_get_num_buffers();

    Gfx *work = gfx_ctx->workBuffer;

    gDPSetScissor(work++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WIDTH,
                  SCREEN_HEIGHT);

    // Clear zbuffer
    gDPSetColorImage(work++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                     zbuffer);
    gDPSetCycleType(work++, G_CYC_FILL);
    gDPSetRenderMode(work++, G_RM_NOOP, G_RM_NOOP2);
    gDPSetFillColor(work++,
                    (GPACK_ZDZ(G_MAXFBZ, 0) << 16) | GPACK_ZDZ(G_MAXFBZ, 0));
    gDPFillRectangle(work++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    gDPPipeSync(work++);

    gDPSetColorImage(work++, G_IM_FMT_RGBA, G_IM_SIZ_16b, SCREEN_WIDTH,
                     surf->buffer);

    gDPSetCycleType(work++, G_CYC_FILL);
    gDPSetRenderMode(work++, G_RM_NOOP, G_RM_NOOP2);
    u8 r = abs((get_ticks_ms() / 15) % 200 - 100), g = 100, b = 50;
    gDPSetFillColor(work++, (GPACK_RGBA5551(r, g, b, 1) << 16) |
                                GPACK_RGBA5551(r, g, b, 1));
    gDPFillRectangle(work++, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
    gDPPipeSync(work++);

    gDPSetDepthImage(work++, zbuffer);

    gSPViewport(work++, &vp);
    gSPMatrix(work++, &projMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    // not sure what the logic is but using 2 for PerspNormalize appears to make
    // normalized coordinates work (with proj == modelView == identity a -1,-1
    // to 1,1 bounded triangle takes up the whole screen)
    gSPPerspNormalize(work++, 2);
    gSPMatrix(work++, &modelViewMtx,
              G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

    gSPSegment(work++, 6, loaded_models[i_cur_loaded_model].data);
    gSPDisplayList(work++, models[i_cur_model].dl);

    gDPFullSync(work++);

    gSPEndDisplayList(work++);

    gfx_ctx->on_task_finished_arg_data.surf = surf;
    gfx_ctx->on_task_finished_arg_data.i_used_loaded_model = i_cur_loaded_model;
    f3dex2_exec_task(gfx_ctx->workBuffer, work, on_task_finished,
                     &gfx_ctx->on_task_finished_arg_data);
  }
}
