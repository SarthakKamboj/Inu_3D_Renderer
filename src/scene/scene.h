#pragma once

#include <vector>
#include <unordered_set>
#include <string>

#include "utils/vectors.h"
#include "utils/mats.h"
#include "utils/transform.h"
#include "animation/animation_internal.h"

#define BONES_PER_SKIN_LIMIT 80
#define SHOW_BONES 0
#define NUM_LIGHTS_SUPPORTED_IN_SHADER 3

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
skin_t get_skin(int skin_id);

struct object_t {
  std::string name;
  int id = -1;
  transform_t transform;
  int model_id = -1;
  std::vector<int> child_objects; 
  int parent_obj = -1;
  mat4 model_mat;
  std::vector<animation_chunk_data_ref_t> anim_chunk_refs;
  bool is_joint_obj = false;
  bool is_skinned = false;
  int skin_id = -1;
};

struct scene_t {
  std::unordered_set<int> parent_objs;
};

int create_object(transform_t& transform);
void attach_name_to_obj(int obj_id, std::string& name);
void attach_model_to_obj(int obj_id, int model_id);
void attach_child_obj_to_obj(int obj_id, int child_obj_id);
void attach_skin_to_obj(int obj_id, int skin_id);
struct vbo_t;
vbo_t* get_obj_vbo(int obj_id, int mesh_idx);

void populate_parent_field_of_nodes();
mat4 get_obj_model_mat(int obj_id);
void set_obj_as_parent(int obj_id);
void unset_obj_as_parent(int obj_id);
void update_obj_model_mats();
void attach_anim_chunk_ref_to_obj(int obj_id, animation_chunk_data_ref_t& ref);
object_t* get_obj(int obj_id);
std::vector<int> get_bone_objs();

void init_scene_rendering();
void print_joint_transform_info();

void spotlight_pass();
void dirlight_pass();
void offline_final_render_pass();
void render_scene();

