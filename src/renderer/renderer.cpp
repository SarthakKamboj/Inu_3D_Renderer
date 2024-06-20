#include "renderer.h"

#include "lights/spotlight.h"
#include "lights/dirlight.h"
#include "editor/pixel_perfect_sel.h"
#include "render_passes/pbr_render_pass.h"

void offline_render() {
  spotlight_pass();
  dirlight_pass();

#if EDITOR
  selection_render_pass();
#endif

  pbr_render_pass();
}
