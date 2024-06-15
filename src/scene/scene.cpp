#include "scene.h"

#include <algorithm>
#include <vector>

#include "glew.h"
#include "wglew.h"

#include "utils/mats.h"
#include "model_loading/model_internal.h"
#include "utils/app_info.h"
#include "gfx/light.h"
#include "gfx/light_probe.h"
#include "gfx/gfx.h"
#include "utils/mats.h"
#include "windowing/window.h"
#include "scene/camera.h"

void traverse_obj_hierarchy_opaque(int obj_id, bool parent, bool light_pass, shader_t& shader);
void render_non_opaque_objs(std::vector<obj_sort_info_t>& non_opaque_objs, bool light_pass, shader_t& shader);
void render_scene_obj(int obj_id, bool light_pass, shader_t& shader);

struct obj_sort_info_comparator_t {
  bool operator()(obj_sort_info_t a, obj_sort_info_t b) {
    vec3 cam_pos = get_cam_pos();
    vec3 diff_a = a.pos - cam_pos;
    vec3 diff_b = b.pos - cam_pos;
    float dist_a = diff_a.length();
    float dist_b = diff_b.length();
    return dist_a > dist_b;
  }
};

std::vector<object_t> objs;
std::vector<skin_t> skins;

static scene_t scene;
float fb_width = 1280 / 1.f;
float fb_height = 960 / 1.f;

framebuffer_t offline_fb;

extern std::vector<model_t> models;
extern app_info_t app_info;
extern window_t window;

int skin_t::BONE_MODEL_ID = -1;

void init_scene_rendering() {
  offline_fb = create_framebuffer(fb_width, fb_height, FB_TYPE::RENDER_BUFFER_DEPTH_STENCIL);
}

int create_object(transform_t& transform) {
  object_t obj;
  static int i = 0;
  obj.id = i++;
  memcpy(&obj.transform, &transform, sizeof(transform_t));
  obj.sel_id = create_selectable_element();
  objs.push_back(obj);
  return obj.id;
}

void attach_model_to_obj(int obj_id, int model_id) {
  printf("attached model id %i to obj: %i\n", model_id, obj_id);
  objs[obj_id].model_id = model_id;
}

void attach_child_obj_to_obj(int obj_id, int child_obj_id) {
  if (child_obj_id < objs.size() && objs[child_obj_id].is_skinned) {
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
    if (updated_idxs.find(obj.id) == updated_idxs.end() && obj.is_joint_obj && obj.parent_obj == -1) {
      mat4 running_model_mat(1.0f);
      update_obj_model_mats_recursive(obj.id, running_model_mat);
    }
  }

  // make sure all joints for skeletons have been updated
  for (object_t& obj: objs) {
    if (obj.is_joint_obj && updated_idxs.find(obj.id) == updated_idxs.end()) {
      inu_assert_msg("joint was not updated");
    }
  }

}

void attach_anim_chunk_ref_to_obj(int obj_id, animation_chunk_data_ref_t& ref) {
  object_t& obj = objs[obj_id];
  obj.anim_chunk_refs.push_back(ref);

#if 0
  animation_data_chunk_t* data = get_anim_data_chunk(ref.chunk_id);
  vec3* v_anim_data = (vec3*)data->keyframe_data;
  quaternion_t* q_anim_data = (quaternion_t*)data->keyframe_data;
  printf("\n\nobj name: %s\n", obj.name.c_str());
  for (int i = 0; i < data->num_timestamps; i++) {
    printf("timestamp: %f frame: %i ", data->timestamps[i], i+1);
    if (ref.target == ANIM_TARGET_ON_NODE::ROTATION) {
      printf("rot: ");
      print_quat(q_anim_data[i]);
    } else if (ref.target == ANIM_TARGET_ON_NODE::POSITION) {
      printf("pos: ");
      print_vec3(v_anim_data[i]);
    } else if (ref.target == ANIM_TARGET_ON_NODE::SCALE) {
      printf("scale: ");
      print_vec3(v_anim_data[i]);
    }
    printf("\n");
  }
#endif 
}

void attach_name_to_obj(int obj_id, std::string& name) {
  objs[obj_id].name = name;
}

void attach_skin_to_obj(int obj_id, int skin_id) {
  object_t& obj = objs[obj_id];
  obj.skin_id = skin_id;
  objs[obj_id].is_skinned = true;
  for (int i = 0; i < objs.size(); i++) {
    auto child_it = std::find(objs[i].child_objects.begin(), objs[i].child_objects.end(), obj_id);
    if (child_it != objs[i].child_objects.end()) {
      // int idx = child_it - objs[i].child_objects.begin();
      objs[i].child_objects.erase(child_it);
    }
  }
}

// all objects here should have a model attached to them
void render_non_opaque_objs(std::vector<obj_sort_info_t>& non_opaque_objs, bool light_pass, shader_t& shader) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  for (obj_sort_info_t& osi : non_opaque_objs) {
    object_t obj = objs[osi.obj_id];
    bool not_render_obj = light_pass && obj.model_id == spotlight_t::LIGHT_MESH_ID;
    if (not_render_obj) continue;
    render_scene_obj(osi.obj_id, light_pass, shader);
  }
}

void traverse_obj_hierarchy_opaque(int obj_id, bool parent, bool light_pass, shader_t& shader) {
  object_t& obj = objs[obj_id];

  bool not_render_obj = (obj.model_id == -1) || (light_pass && obj.model_id == spotlight_t::LIGHT_MESH_ID);
  if (!not_render_obj) {
    model_t& model = models[obj.model_id];
    if (!model.is_non_opaque_mesh) {
        render_scene_obj(obj_id, light_pass, shader);
    }
  }

  for (int child : obj.child_objects) {
    traverse_obj_hierarchy_opaque(child, false, light_pass, shader);
  }
}

void render_scene_obj(int obj_id, bool light_pass, shader_t& shader) {
  object_t& obj = objs[obj_id];

#if 0
  // only render obj if it should be...otherwise move onto its children objs


#if SHOW_LIGHTS
  bool not_render_obj = (obj.model_id == -1) || (light_pass && obj.model_id == spotlight_t::LIGHT_MESH_ID);
#else
  bool not_render_obj = obj.model_id == -1;
#endif
  if (not_render_obj) return;

#endif
  // if (!not_render_obj) { 

  if (obj.is_skinned) {
    skin_t skin = get_skin(obj.skin_id);
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
  } else {
    shader_set_int(shader, "skinned", 0);
    shader_set_mat4(shader, "model", obj.model_mat);

    transform_t final_transform = get_transform_from_matrix(obj.model_mat);
    if (isnan(final_transform.pos.x) || isnan(final_transform.pos.y) || isnan(final_transform.pos.z)) {
      inu_assert_msg("final transform is nan");
    }
  }

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

#if 0
  for (int child : obj.child_objects) {
    render_scene_obj(child, false, light_pass, shader);
  }
#endif
}

void spotlight_pass() {
  // LIGHT PASS
  int num_spotlights = get_num_spotlights();
  for (int i = 0; i < num_spotlights; i++) {
    setup_spotlight_for_rendering(i);

    for (int parent_id : scene.parent_objs) {
      traverse_obj_hierarchy_opaque(parent_id, true, true, spotlight_t::light_shader);
    }
    for (object_t& obj : objs) {
      if (obj.is_skinned) {
        traverse_obj_hierarchy_opaque(obj.id, false, true, spotlight_t::light_shader);
      }
    }
    remove_spotlight_from_rendering();
  }
}

void dirlight_pass() {

#if HAVE_DIR_LIGHT
  // DIR light
  int num_dir_lights = get_num_dir_lights();
  camera_t* cam = get_cam();

  for (int i = 0; i < num_dir_lights; i++) { 
    gen_dir_light_matricies(i, cam); 
    setup_dir_light_for_rendering(i, cam);

    for (int parent_id : scene.parent_objs) {
      traverse_obj_hierarchy_opaque(parent_id, true, true, dir_light_t::light_shader);
    }

    for (object_t& obj : objs) {
      if (obj.is_skinned) {
        traverse_obj_hierarchy_opaque(obj.id, false, true, dir_light_t::light_shader);
      }
    }

    remove_dir_light_from_rendering();
  }
#endif
}

void offline_final_render_pass() {
  // OFFLINE RENDER PASS
  bind_framebuffer(offline_fb);
  clear_framebuffer();

  mat4 proj = get_cam_proj_mat();
  mat4 view = get_cam_view_mat();
  shader_set_mat4(material_t::associated_shader, "projection", proj);
  shader_set_mat4(material_t::associated_shader, "view", view);

  shader_set_float(material_t::associated_shader, "cam_data.near_plane", get_cam_near_plane());
  shader_set_float(material_t::associated_shader, "cam_data.far_plane", get_cam_far_plane());
  shader_set_vec3(material_t::associated_shader, "cam_data.cam_pos", get_cam_pos());

  // spotlights for offline shader
  int num_lights = get_num_spotlights();
  for (int i = 0; i < NUM_LIGHTS_SUPPORTED_IN_SHADER; i++) {
    mat4 identity(1.0f);
    bool inactive = (i >= num_lights);

    char var_name[64]{};
    sprintf(var_name, "spotlights_mat_data[%i].light_view", i);
    if (inactive) {
      shader_set_mat4(material_t::associated_shader, var_name, identity);
    } else {
      mat4 light_view = get_spotlight_view_mat(i);
      shader_set_mat4(material_t::associated_shader, var_name, light_view);
    }
    
    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_mat_data[%i].light_projection", i);
    if (inactive) {
      shader_set_mat4(material_t::associated_shader, var_name, identity);
    } else {
      mat4 light_proj = get_spotlight_proj_mat(i);
      shader_set_mat4(material_t::associated_shader, var_name, light_proj);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].depth_tex", i);
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, LIGHT0_SHADOW_MAP_TEX + i);
      tex_id_t depth_att = get_spotlight_fb_depth_tex(i);
      bind_texture(depth_att, LIGHT0_SHADOW_MAP_TEX + i);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].pos", i);
    if (inactive) {
      shader_set_vec3(material_t::associated_shader, var_name, {0,0,0});
    } else {
      vec3 p = get_spotlight_pos(i);
      shader_set_vec3(material_t::associated_shader, var_name, p);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].light_active", i);
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, 1);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].shadow_map_width", i);
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, spotlight_t::SHADOW_MAP_WIDTH);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].shadow_map_height", i);
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, spotlight_t::SHADOW_MAP_HEIGHT);
    }
    
    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].near_plane", i);
    if (inactive) {
      shader_set_float(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_float(material_t::associated_shader, var_name, spotlight_t::NEAR_PLANE);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].far_plane", i);
    if (inactive) {
      shader_set_float(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_float(material_t::associated_shader, var_name, spotlight_t::FAR_PLANE);
    }
  }

  // set up dir lights in offline shader
  int num_dir_lights = get_num_dir_lights(); 
  const int NUM_MAX_DIR_LIGHTS = 1;
  for (int i = 0; i < NUM_MAX_DIR_LIGHTS * HAVE_DIR_LIGHT; i++) {

    mat4 identity(1.0f);
    bool inactive = (i >= num_dir_lights);

    if (inactive) {
      const char* var_name = "dir_light_data.light_active";
      shader_set_int(material_t::associated_shader, var_name, 0);
      continue;
    }

    dir_light_t* dir_light = get_dir_light(i);

    for (int j = 0; j < NUM_SM_CASCADES; j++) {

      char var_name[64]{};
      sprintf(var_name, "dir_light_data.light_views[%i]", j);
      if (inactive) {
        shader_set_mat4(material_t::associated_shader, var_name, identity);
      } else {
        shader_set_mat4(material_t::associated_shader, var_name, dir_light->light_views[j]);
      }

      memset(var_name, 0, sizeof(var_name));
      sprintf(var_name, "dir_light_data.light_projs[%i]", j);
      if (inactive) {
        shader_set_mat4(material_t::associated_shader, var_name, identity);
      } else {
        shader_set_mat4(material_t::associated_shader, var_name, dir_light->light_orthos[j]);
      }

      memset(var_name, 0, sizeof(var_name));
      sprintf(var_name, "dir_light_data.cascade_depths[%i]", j);
      if (inactive) {
        shader_set_float(material_t::associated_shader, var_name, 0);
      } else {
        shader_set_float(material_t::associated_shader, var_name, dir_light->cacade_depths[j]);
      }
    }
    
    char var_name[64]{};
    sprintf(var_name, "dir_light_data.cascade_depths[%i]", NUM_SM_CASCADES);
    if (inactive) {
      shader_set_float(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_float(material_t::associated_shader, var_name, dir_light->cacade_depths[NUM_SM_CASCADES]);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "dir_light_data.light_dir");
    if (inactive) {
      shader_set_vec3(material_t::associated_shader, var_name, {0,0,0});
    } else {
      shader_set_vec3(material_t::associated_shader, var_name, dir_light->dir);
    }
    
    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "dir_light_data.shadow_map");
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, DIR_LIGHT_SHADOW_MAP_TEX);
      tex_id_t depth_att = dir_light->light_pass_fb.depth_att;
      bind_texture(depth_att, DIR_LIGHT_SHADOW_MAP_TEX);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "dir_light_data.light_active");
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, 1);
    }

  }

#if 0
  shader_set_int(material_t::associated_shader, "light_prob.shape", LIGHT_PROBE_SHAPE::RECT_PLANAR);
  vec3 color = {0,1,0};
  shader_set_vec3(material_t::associated_shader, "light_prob.color", color);
  shader_set_vec3(material_t::associated_shader, "light_prob.world_pos", color);
#endif
  set_light_probe_in_shader(1, 0, material_t::associated_shader);
  set_light_probe_in_shader(2, 1, material_t::associated_shader);

  // set_light_probe_in_shader(1, 0, material_t::associated_shader);
  

  // PASS 1 will be of only opaque objects
  for (int parent_id : scene.parent_objs) {
    traverse_obj_hierarchy_opaque(parent_id, true, false, material_t::associated_shader);
  }

  // TODO: need to check but i think this happens because skinned objects are not considered parents 
  // but are still heads of their own hierarchy?
  for (object_t& obj : objs) {
    if (obj.is_skinned) {
      traverse_obj_hierarchy_opaque(obj.id, false, false, material_t::associated_shader);
    }
  }
  unbind_shader();

  // PASS 2 will be of only non opaque objects...altho these will need to be sorted and rendered back to front
  // sort non-opaque objects 

  obj_sort_info_comparator_t obj_sort_info_comparator;  
  std::vector<obj_sort_info_t> non_opaque_objs;
  for (object_t& obj : objs) {
    if (obj.model_id == -1) continue;
    model_t& model = models[obj.model_id];
    if (model.is_non_opaque_mesh) {
      obj_sort_info_t info;
      transform_t final_transform = get_transform_from_matrix(obj.model_mat);
      info.obj_id = obj.id;
      info.pos = final_transform.pos;
      non_opaque_objs.push_back(info);
    }
  }

  std::sort(non_opaque_objs.begin(), non_opaque_objs.end(), obj_sort_info_comparator);
  render_non_opaque_objs(non_opaque_objs, false, material_t::associated_shader);

#if DISPLAY_DIR_LIGHT_SHADOW_MAPS
  if (num_dir_lights > 0) {
    // show depth maps 
    render_dir_light_shadow_maps(0);
  }
#endif
}

void render_scene() { 
  spotlight_pass();
  dirlight_pass();
  offline_final_render_pass();

  // render_light_probes();
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
  for (int i = 0; i < skin.num_bones; i++) {
    int node_idx = skin.joint_obj_ids[i];
    objs[node_idx].is_joint_obj = true;
#if SHOW_BONES
    attach_model_to_obj(skin_t::BONE_MODEL_ID);
#endif
  }
  skins.push_back(skin);
  return skin.id;
}

skin_t get_skin(int skin_id) {
  return skins[skin_id];
}

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

std::vector<int> get_bone_objs() {
  std::vector<int> bones;
  for (object_t& obj : objs) {
    if (obj.is_joint_obj) {
      bones.push_back(obj.id);
    }
  }
  return bones;
}

vbo_t* get_obj_vbo(int obj_id, int mesh_idx) {
  model_t* model = get_model(objs[obj_id].model_id);
  return &model->meshes[mesh_idx].vbo;
}
