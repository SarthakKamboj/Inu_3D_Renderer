#pragma once

#include "utils/vectors.h"
#include "utils/mats.h"

#include "gfx_api/gfx.h"

typedef int selectable_id;

struct selectable_element_t {
  selectable_id id = 0;
  vec3 color; 

  int obj_id = -1;

  static framebuffer_t SELECTION_FB;
  static shader_t SELECTION_SHADER;
};

void init_selection();
selectable_id create_selectable_element(int obj_id);
selectable_id get_sel_el_from_color(vec3 color);

selectable_element_t get_sel_el(selectable_id id);
void set_selection(selectable_id sel_id);

bool is_obj_selected(int obj_id);

void selection_render_pass();
