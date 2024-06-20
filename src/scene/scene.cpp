#include "scene.h"

#include <algorithm>
#include <vector>

#include "glew.h"
#include "wglew.h"

#include "utils/mats.h"
#include "geometry/model.h"
#include "utils/app_info.h"
#include "lights/dirlight.h"
#include "lights/light_probe.h"
#include "gfx_api/gfx.h"
#include "utils/mats.h"
#include "windowing/window.h"
#include "scene/camera.h"
#include "editor/pixel_perfect_sel.h"
#include "utils/log.h"
#include "lights/spotlight.h"
#include "animation/skin.h"
#include "render_passes/pbr_render_pass.h"

void traverse_obj_hierarchy_opaque(int obj_id, bool parent, bool light_pass, shader_t& shader);
void render_non_opaque_objs(std::vector<obj_sort_info_t>& non_opaque_objs, bool light_pass, shader_t& shader);
void render_scene_obj(int obj_id, bool light_pass, shader_t& shader);

std::vector<object_t> objs;

static scene_t scene;

extern app_info_t app_info;
extern window_t window;

void init_scene_rendering() {
}

int create_object(transform_t& transform) {
  object_t obj;
  static int i = 0;
  obj.id = i++;
  memcpy(&obj.transform, &transform, sizeof(transform_t));
  objs.push_back(obj);

#if EDITOR
  create_selectable_element(obj.id);
#endif

  return obj.id;
}

void remove_obj_as_child(int obj_id) {
  for (int i = 0; i < objs.size(); i++) {
    auto child_it = std::find(objs[i].child_objects.begin(), objs[i].child_objects.end(), obj_id);
    if (child_it != objs[i].child_objects.end()) {
      objs[i].child_objects.erase(child_it);
    }
  }
}

void attach_child_obj_to_obj(int obj_id, int child_obj_id) {
  if (child_obj_id < objs.size() && obj_has_skin(child_obj_id)) {
    return;
  }
  objs[obj_id].child_objects.push_back(child_obj_id);
}

void populate_parent_field_of_nodes_helper(int parent, int child) {
  objs[child].parent_obj = parent;
  for (int gc : objs[child].child_objects) {
    populate_parent_field_of_nodes_helper(child, gc);
  }
}

void populate_parent_field_of_nodes() {
  for (int parent_id : scene.parent_objs) {
    objs[parent_id].parent_obj = -1;
    for (int child : objs[parent_id].child_objects) {
      populate_parent_field_of_nodes_helper(parent_id, child);
    }
  }
}

void set_obj_as_parent(int obj_id) {
  scene.parent_objs.insert(obj_id);
}

void unset_obj_as_parent(int obj_id) {
  scene.parent_objs.erase(obj_id);
}

mat4 get_obj_model_mat(int obj_id) {
  return objs[obj_id].model_mat;
}

static std::unordered_set<int> updated_idxs;
void update_obj_model_mats_recursive(int obj_id, mat4& running_model) {
  object_t& obj = objs[obj_id];
  if (isnan(obj.transform.pos.x) || isnan(obj.transform.pos.y) || isnan(obj.transform.pos.z)) {
    inu_assert_msg("obj transform pos is nan");
  }
  mat4 local_model_mat = get_model_matrix(objs[obj_id].transform);
  for (int i = 0; i < 16; i++) {
    if (isnan(local_model_mat.vals[i])) {
      inu_assert_msg("obj model matrix from transform is nan");
    }
  }
  objs[obj_id].model_mat = running_model * local_model_mat;
  // update_sel_el_on_obj(obj_id);
  updated_idxs.insert(obj_id);
  for (int child_id : objs[obj_id].child_objects) {
    update_obj_model_mats_recursive(child_id, objs[obj_id].model_mat);
  }
}

object_t* get_obj(int obj_id) {
  return &objs[obj_id];
}

// when doing skinned animation, 
//  first pass is: non-skinned objs need to be updated
//  second pass is : then joints + their children (if not handled in first pass) - (essentially parent bone nodes which have no parents)
void update_obj_model_mats() {
  updated_idxs.clear();

  // update starting from parent objects in the scene
  for (int parent_id : scene.parent_objs) {
    mat4 running_model_mat(1.0f);
    update_obj_model_mats_recursive(parent_id, running_model_mat);
  }

  // update objects that have not been updates because they are skeletons
  for (object_t& obj : objs) {
    if (updated_idxs.find(obj.id) == updated_idxs.end() && obj_has_skin(obj.id) && obj.parent_obj == -1) {
      mat4 running_model_mat(1.0f);
      update_obj_model_mats_recursive(obj.id, running_model_mat);
    }
  }

  // make sure all joints for skeletons have been updated
  for (object_t& obj: objs) {
    if (obj_has_skin(obj.id) && updated_idxs.find(obj.id) == updated_idxs.end()) {
      inu_assert_msg("joint was not updated");
    }
  }

}

void attach_name_to_obj(int obj_id, std::string& name) {
  objs[obj_id].name = name;
}

// all objects here should have a model attached to them
void render_non_opaque_objs(std::vector<obj_sort_info_t>& non_opaque_objs, bool light_pass, shader_t& shader) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  for (obj_sort_info_t& osi : non_opaque_objs) {
    object_t& obj = objs[osi.obj_id];
    bool not_render_obj = light_pass && get_obj_model_id(osi.obj_id) == spotlight_t::LIGHT_MESH_ID;
    if (not_render_obj) continue;
    render_scene_obj(osi.obj_id, light_pass, shader);
  }
}

void traverse_obj_hierarchy_opaque(int obj_id, bool parent, bool light_pass, shader_t& shader) {
  object_t& obj = objs[obj_id];

  int model_id = get_obj_model_id(obj_id);
  bool not_render_obj = (model_id == -1) || (light_pass && model_id == spotlight_t::LIGHT_MESH_ID);
  if (!not_render_obj && is_model_opaque(model_id)) {
    render_scene_obj(obj_id, light_pass, shader);
  }

  for (int child : obj.child_objects) {
    traverse_obj_hierarchy_opaque(child, false, light_pass, shader);
  }
}

void render_scene_obj(int obj_id, bool light_pass, shader_t& shader) {
  object_t& obj = objs[obj_id];

  if (obj_has_skin(obj_id)) {
    set_skin_in_shader_for_obj(shader, obj_id);
  } else {
    shader_set_int(shader, "skinned", 0);
    shader_set_mat4(shader, "model", obj.model_mat);
  } 

#if 1
  if (is_obj_selected(obj_id)) {
    set_render_mode(RENDER_MODE::WIREFRAME);
  }

  int model_id = get_obj_model_id(obj_id);
  render_model(model_id, light_pass, shader);

  if (is_obj_selected(obj_id)) {
    set_render_mode(RENDER_MODE::NORMAL);
  }
#else 

  model_t& model = models[obj.model_id];
  for (mesh_t& mesh : model.meshes) {

    if (!light_pass) {
      if (obj.model_id == spotlight_t::LIGHT_MESH_ID) {
        shader_set_int(material_t::associated_shader, "override_color_bool", 1);
      } else {
        shader_set_int(material_t::associated_shader, "override_color_bool", 0);
      }
    }

    material_t m;
    if (!light_pass) {
      m = bind_material(mesh.mat_idx);
    } else {
      m = get_material(mesh.mat_idx);
      bind_shader(shader);
    }

    bool rendering_only_if_textured_albedo = app_info.render_only_textured && m.albedo.base_color_img.tex_handle == -1;
    // bool transparent_mesh = (m.transparency_mode == TRANSPARENCY_MODE::TRANSPARENT);

    if (rendering_only_if_textured_albedo) {
      continue;
    }

#if SHOW_BONES
    // only shows bones
    if (obj.is_joint_obj) {
      bind_vao(mesh.vao);
      draw_ebo(mesh.ebo);
      unbind_vao();
      unbind_ebo();
    }
#else
    bind_vao(mesh.vao);
    draw_ebo(mesh.ebo);
    unbind_vao();
    unbind_ebo();
#endif
  }
  // }
#endif

#if 0
  for (int child : obj.child_objects) {
    render_scene_obj(child, false, light_pass, shader);
  }
#endif

}

void render_scene() {
  spotlight_pass();
  dirlight_pass();

#if EDITOR
  selection_render_pass();
#endif

  pbr_render_pass();
}

vbo_t* get_obj_vbo(int obj_id, int mesh_idx) {
  int model_id = get_obj_model_id(obj_id);
  model_t* model = get_model(model_id);
  return &model->meshes[mesh_idx].vbo;
}

static std::vector<scene_iterated_info_t> scene_iterator_infos;
scene_iterator_t create_scene_iterator() {
  scene_iterator_t scene_iterator{};
  scene_iterator_infos.clear();

  // default values of this represents the root node over parent nodes
  scene_iterated_info_t info;
  info.num_children = scene.parent_objs.size();
  scene_iterator_infos.push_back(info);

  return scene_iterator;
}

scene_iterated_info_t* get_idx_of_scene_iterator_info(int obj_id) {
  for (int j = 0; j < scene_iterator_infos.size(); j++)   {
    if (scene_iterator_infos[j].obj_id == obj_id) return &scene_iterator_infos[j];
  }
  return NULL;
}

int iterate_scene_for_next_obj(scene_iterator_t& iterator) {

  scene_iterated_info_t* scene_iter_info = get_idx_of_scene_iterator_info(iterator.obj_id);
  inu_assert(scene_iter_info != NULL);

  if (scene_iter_info->child_idx >= scene_iter_info->num_children) {
    // this object has no more children so we have to move up the hierarchy

    // we are already at the root node of the scene
    if (iterator.obj_id == -1) return -1;

    // we have to move up the hierarchy
    iterator.obj_id = scene_iter_info->parent;
    return iterate_scene_for_next_obj(iterator);
  }

  // there is a child object we can go to

  // we are at the root node above parent nodes
  if (iterator.obj_id == -1) {
    std::vector<int> parent_obj_ids; 
    for (auto i : scene.parent_objs) {
      parent_obj_ids.push_back(i);
    }
    std::sort(parent_obj_ids.begin(), parent_obj_ids.end());

    scene_iterated_info_t child_info{};
    child_info.obj_id = parent_obj_ids[scene_iter_info->child_idx];
    scene_iter_info->child_idx++;

    object_t& child_obj = objs[child_info.obj_id];
    child_info.num_children = child_obj.child_objects.size();

    scene_iterator_infos.push_back(child_info);
    iterator.obj_id = child_info.obj_id;

    return iterator.obj_id;
  }

  // we are not at the root node above parent nodes
  object_t& obj = objs[scene_iter_info->obj_id];
  scene_iterated_info_t child_info{};

  child_info.obj_id = obj.child_objects[scene_iter_info->child_idx];
  scene_iter_info->child_idx++;

  child_info.parent = scene_iter_info->obj_id;
  
  object_t& child_obj = objs[child_info.obj_id];
  child_info.num_children = child_obj.child_objects.size();

  scene_iterator_infos.push_back(child_info);
  iterator.obj_id = child_info.obj_id;

  return iterator.obj_id;
}
