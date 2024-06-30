#pragma once

#include "utils/vectors.h"
#include "utils/mats.h"
#include "inu_typedefs.h"
#include "gfx_api/gfx.h"

typedef int selectable_id;

struct selectable_element_t {
  selectable_id id = 0;
  vec3 color; 

  object_id obj_id = -1;

  static framebuffer_t SELECTION_FB;
  static shader_t SELECTION_SHADER;
};

struct selection_state_t {
  selectable_id left_mouse_down_selected = -1;
  selectable_id left_mouse_up_selected = -1;
};

void init_selection();
selectable_id create_selectable_element(object_id obj_id);
void handle_selection_logic();
vec3 get_sel_pixel_color();
selectable_id get_sel_el_from_color(vec3 color);

selectable_element_t get_sel_el(selectable_id id);
// void set_selection(selectable_id sel_id);

bool is_obj_selected_left_mouse_up(object_id obj_id);
bool is_obj_selected_left_mouse_down(object_id obj_id);

int get_obj_selected_left_mouse_up();
int get_obj_selected_left_mouse_down();

void selection_render_pass();
