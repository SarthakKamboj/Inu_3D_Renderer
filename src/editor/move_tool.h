#pragma once

#include "utils/transform.h"
#include "inu_typedefs.h"

struct move_tool_t {
  transform_t transform; 
  model_id arrow_id;

  int parent_obj = -1;
  int x_obj_id = -1;
  int y_obj_id = -1;
  int z_obj_id = -1;

  bool selected_x = false;
  bool selected_y = false;
  bool selected_z = false;

  int selected_obj_id = -1;
};

struct axes_model_t {
  int x_axis_obj_id = -1;  
  int y_axis_obj_id = -1;  
  int z_axis_obj_id = -1;  
};
void create_axes();
void hide_axes();

void init_move_tool();
void update_move_tool();
bool currently_selecting();
void set_move_tool_as_position(vec3 pos);
void set_move_tool_as_transform(transform_t& t);
