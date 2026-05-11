#include "t3d/t3dmodel.h"
#include <libdragon.h>

#include <t3d/t3d.h>
#include <t3d/t3dmath.h>

int main() {
  debug_init_isviewer();
  debug_init_usblog();

  dfs_init(DFS_DEFAULT_LOCATION);

  display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE,
               FILTERS_RESAMPLE);
  rdpq_init();

  t3d_init((T3DInitParams){});

  T3DModel *model = t3d_model_load("rom:/Suzanne1K.t3dm");

  T3DMat4 modelMat;
  t3d_mat4_identity(&modelMat);
  T3DMat4FP *modelMatFP = malloc_uncached(sizeof(T3DMat4FP));

  uint8_t colorAmbient[4] = {255, 255, 255, 255};

  T3DViewport viewport = t3d_viewport_create();

  rspq_block_t *dplDraw = NULL;

  for (;;) {
    t3d_viewport_set_ortho(&viewport, -1, 1, -1, 1, 0, 100);
    t3d_viewport_look_at(&viewport, &(T3DVec3){{0, 0, 1}},
                         &(T3DVec3){{0, 0, 0}}, &(T3DVec3){{0, 1, 0}});

    t3d_mat4_identity(&modelMat);
    float s = 1.0f / 1024 * 100 / 64;
    t3d_mat4_scale(&modelMat, s, s, s);
    t3d_mat4_to_fixed(modelMatFP, &modelMat);

    rdpq_attach(display_get(), display_get_zbuf());
    t3d_frame_start();

    t3d_viewport_attach(&viewport);

    t3d_screen_clear_color(RGBA32(100, 0, 100, 0));
    t3d_screen_clear_depth();

    t3d_light_set_ambient(colorAmbient);
    t3d_light_set_count(0);

    if (!dplDraw) {
      rspq_block_begin();

      t3d_model_draw(model);

      dplDraw = rspq_block_end();
    }

    rspq_block_run(dplDraw);

    rdpq_detach_show();
  }

  t3d_destroy();
  return 0;
}
