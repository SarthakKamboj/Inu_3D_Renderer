#pragma once

#include "gfx_api/gfx.h"
#include "inu_typedefs.h"

#define BONES_PER_SKIN_LIMIT 80
#define SHOW_BONES 0

struct skin_t {
  skin_id id = -1;
  int upper_most_joint_node_idx = -1;
  object_id joint_obj_ids[BONES_PER_SKIN_LIMIT]{};
  mat4 inverse_bind_matricies[BONES_PER_SKIN_LIMIT];
  int num_bones = -1;
  std::string name;

  static model_id BONE_MODEL_ID;
  
  skin_t();
};

void init_skin_system();

int register_skin(skin_t& skin);
skin_t* get_skin(skin_id id);
void attach_skin_to_obj(object_id obj_id, skin_id skin_id_to_attach);
bool obj_has_skin(object_id obj_id);
void set_skin_in_shader_for_obj(shader_t& shader, object_id obj_id);
