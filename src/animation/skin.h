#pragma once

#include "gfx_api/gfx.h"

#define BONES_PER_SKIN_LIMIT 80
#define SHOW_BONES 0

struct skin_t {
  int id = -1;
  int upper_most_joint_node_idx = -1;
  int joint_obj_ids[BONES_PER_SKIN_LIMIT]{};
  mat4 inverse_bind_matricies[BONES_PER_SKIN_LIMIT];
  int num_bones = -1;
  std::string name;

  static int BONE_MODEL_ID;
  
  skin_t();
};
int register_skin(skin_t& skin);
skin_t* get_skin(int skin_id);
void attach_skin_to_obj(int obj_id, int skin_id);
bool obj_has_skin(int obj_id);
void set_skin_in_shader_for_obj(shader_t& shader, int obj_id);
