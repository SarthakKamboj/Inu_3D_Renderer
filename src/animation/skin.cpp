#include "skin.h"

#include "scene/scene.h"
#include "utils/log.h"

#include <unordered_map>
#include <vector>

static std::unordered_map<object_id, skin_id> obj_id_to_skin_id;
static std::vector<skin_t> skins;

model_id skin_t::BONE_MODEL_ID = -1;

void init_skin_system() {
#if SHOW_BONES
  char bone_mesh_full_file_path[256]{};
  // this file pretty much just has a mesh, no nodes
  sprintf(bone_mesh_full_file_path, "%s\\bone_mesh\\custom_bone_mesh.gltf", resources_path);
  gltf_load_file(bone_mesh_full_file_path, false);
  skin_t::BONE_MODEL_ID = latest_model_id();
#endif
}

skin_t::skin_t() {
  id = -1;
  upper_most_joint_node_idx = -1;
  memset(joint_obj_ids, 0, sizeof(joint_obj_ids));
  for (int i = 0; i < BONES_PER_SKIN_LIMIT; i++) {
    inverse_bind_matricies[i] = mat4(1.0f);
  }
}

int register_skin(skin_t& skin) {
  skin.id = skins.size();
  printf("Skin %s at idx %i has %i bones\n", skin.name.c_str(), skin.id, skin.num_bones);
#if 0
  for (int i = 0; i < skin.num_bones; i++) {
    int node_idx = skin.joint_obj_ids[i];
    objs[node_idx].is_joint_obj = true;
#if SHOW_BONES
    attach_model_to_obj(skin_t::BONE_MODEL_ID);
#endif
  }
#endif
  skins.push_back(skin);
  return skin.id;
}

skin_t* get_skin(skin_id id) {
  if (id >= skins.size()) {
    return NULL;
  }
  return &skins[id];
}

#if 0
void print_joint_transform_info_helper(int obj_id) {
  object_t& obj = objs[obj_id];
  if (obj.is_joint_obj) {
    printf("------bone name: %s----------\n", obj.name.c_str()) ;
    printf("\n--local transform--\n");
    print_transform(obj.transform);
    printf("\n--global model matrix--\n");
    print_mat4(obj.model_mat);
    transform_t decoded = get_transform_from_matrix(obj.model_mat);
    printf("\n--global transform--\n");
    print_transform(decoded);
    printf("\n");
  }
  for (int c : objs[obj_id].child_objects) {
    print_joint_transform_info_helper(c);
  }
}
#endif

#if 0
void print_joint_transform_info() {
  for (int parent_id : scene.parent_objs) {
    print_joint_transform_info_helper(parent_id);
  }

  for (object_t& obj : objs) {
    if (obj.is_joint_obj && obj.parent_obj == -1) {
      print_joint_transform_info_helper(obj.id);
    }
  }  
}
#endif

#if 0
std::vector<int> get_bone_objs() {
  std::vector<int> bones;
  for (object_t& obj : objs) {
    if (obj.is_joint_obj) {
      bones.push_back(obj.id);
    }
  }
  return bones;
}
#endif

void attach_skin_to_obj(object_id obj_id, skin_id skin_id_to_attach) {
  obj_id_to_skin_id[obj_id] = skin_id_to_attach;
  // remove any parents of the object 
#if 0
  int parent_joint = skins[skin_id].upper_most_joint_node_idx;
  remove_obj_as_child(parent_joint);
  set_obj_as_parent(parent_joint);
#endif
}

bool obj_has_skin(object_id obj_id) {
  return obj_id_to_skin_id.find(obj_id) != obj_id_to_skin_id.end();
}

void set_skin_in_shader_for_obj(shader_t& shader, object_id obj_id) {
  skin_id skin_id_for_obj = obj_id_to_skin_id[obj_id];
  skin_t* skin_p = get_skin(skin_id_for_obj);
  inu_assert(skin_p);
  skin_t& skin = *skin_p;

  shader_set_int(shader, "skinned", 1);
  for (int i = 0; i < skin.num_bones; i++) {
    char mat4_name[64]{};
    sprintf(mat4_name, "joint_inverse_bind_mats[%i]", i);
    shader_set_mat4(shader, mat4_name, skin.inverse_bind_matricies[i]);

    memset(mat4_name, 0, sizeof(mat4_name));
    sprintf(mat4_name, "joint_model_matricies[%i]", i);
    mat4 joint_model_matrix = get_obj_model_mat(skin.joint_obj_ids[i]);
    shader_set_mat4(shader, mat4_name, joint_model_matrix);
  }

  // setting the rest to defaults
  for (int i = skin.num_bones; i < BONES_PER_SKIN_LIMIT; i++) {
    mat4 identity(1.0f);
    char mat4_name[64]{};
    sprintf(mat4_name, "joint_inverse_bind_mats[%i]", i);
    shader_set_mat4(shader, mat4_name, identity);

    memset(mat4_name, 0, sizeof(mat4_name));
    sprintf(mat4_name, "joint_model_matricies[%i]", i);
    shader_set_mat4(shader, mat4_name, identity);
  }
}
