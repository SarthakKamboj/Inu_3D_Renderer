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
selectable_id get_sel_el_from_color(vec3 color);
void update_sel_el_on_obj(int obj_id);
// void render_sel_elements();
selectable_element_t get_sel_el(selectable_id id);
void set_selection(selectable_id sel_id);

struct object_t;
bool is_obj_selected(object_t& obj);
