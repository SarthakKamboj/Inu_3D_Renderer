#pragma once

#include <map>

#include "utils/vectors.h"
#include "gfx_api/gfx.h"

struct editor_mat_t {
  vec3 color;  
};

struct editor_system_t {
  shader_t editor_shader;
  std::map<int, editor_mat_t> obj_to_editor_mat;
};

void init_editor_system();
void attach_editor_mat_to_obj(int obj_id, editor_mat_t& mat);
void editor_pass();
