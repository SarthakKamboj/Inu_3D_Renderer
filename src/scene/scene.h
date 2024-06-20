#pragma once

#include <vector>
#include <unordered_set>
#include <string>

#include "utils/vectors.h"
#include "utils/mats.h"
#include "utils/transform.h"
#include "animation/animation.h"
#include "editor/pixel_perfect_sel.h"
#include "app_includes.h"
#include "inu_typedefs.h"

#define NUM_LIGHTS_SUPPORTED_IN_SHADER 3

struct object_t {
  int id = -1;
  std::string name;
  transform_t transform;
  mat4 model_mat;
  std::vector<int> child_objects; 
  int parent_obj = -1;
};

struct scene_t {
  std::unordered_set<int> parent_objs;
};

int create_object(transform_t& transform);
void attach_name_to_obj(int obj_id, std::string& name);
void attach_child_obj_to_obj(int obj_id, int child_obj_id);
void remove_obj_as_child(int obj_id);

struct vbo_t;
vbo_t* get_obj_vbo(int obj_id, int mesh_idx);

void populate_parent_field_of_nodes();
mat4 get_obj_model_mat(int obj_id);
void set_obj_as_parent(int obj_id);
void unset_obj_as_parent(int obj_id);
void update_obj_model_mats();
object_t* get_obj(int obj_id);

void init_scene_rendering();
void print_joint_transform_info();

void spotlight_pass();
void offline_final_render_pass();
void render_scene();

struct scene_iterator_t {
  int obj_id = -1;
};

struct scene_iterated_info_t {
  // these default values represent a node that supercedes all the individual parent nodes
  int obj_id = -1;
  int parent = -1;
  int child_idx = 0;
  int num_children = 0;
};

scene_iterator_t create_scene_iterator();
int iterate_scene_for_next_obj(scene_iterator_t& iterator);

