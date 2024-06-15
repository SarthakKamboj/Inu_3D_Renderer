#pragma once

#include "utils/vectors.h"
#include "utils/mats.h"

#include "gfx/gfx.h"

typedef int selectable_id;

struct selectable_element_t {
  selectable_id id = 0;
  vec3 color; 
  mat4 model_mat;

  static framebuffer_t SELECTION_FB;
  static shader_t SELECTION_SHADER;
};

void init_selection();
selectable_id create_selectable_element();
void update_sel_el_on_obj();
void render_sel_elements();
