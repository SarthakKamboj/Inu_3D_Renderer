#include "model.h"

#include <vector>

#include "lights/dirlight.h"
#include "utils/general.h"
#include "geometry/gltf/gltf.h"
#include "lights/spotlight.h"

std::vector<model_t> models;

bool is_model_opaque(model_t& model) {
  for (mesh_t& mesh : model.meshes) {
    material_t m = get_material(mesh.mat_idx);
    bool non_opaque = (m.transparency_mode != TRANSPARENCY_MODE::OPQUE);
    if (non_opaque) {
      return false;
    }
  }
  return true;
}

int register_model(model_t& model) {
  model.id = models.size();
  model.is_non_opaque_mesh = !is_model_opaque(model);
  
  models.push_back(model);
  return model.id;
}

int latest_model_id() {
  return models[models.size()-1].id;
}

model_t* get_model(int model_id) {
  return &models[model_id];
}

void render_mesh(mesh_t& mesh) {
  bind_vao(mesh.vao);
  draw_ebo(mesh.ebo);
  unbind_vao();
  unbind_ebo();
}

void render_sel_model_w_no_material_bind(int model_id) {
  model_t& model = models[model_id];
  for (mesh_t& mesh : model.meshes) {
    render_mesh(mesh);
  }
}

void render_model(int model_id, bool light_pass, shader_t& shader) {
  model_t& model = models[model_id];
  for (mesh_t& mesh : model.meshes) {

    if (!light_pass) {
      if (model_id == spotlight_t::LIGHT_MESH_ID) {
        // shader_set_int(material_t::associated_shader, "override_color_bool", 1);
        shader_set_int(shader, "override_color_bool", 1);
      } else {
        shader_set_int(shader, "override_color_bool", 0);
      }
    }

    material_t m;
    if (!light_pass) {
      m = bind_material(mesh.mat_idx);
    } else {
      m = get_material(mesh.mat_idx);
      bind_shader(shader);
    }

#if 0
    bool rendering_only_if_textured_albedo = app_info.render_only_textured && m.albedo.base_color_img.tex_handle == -1;
    // bool transparent_mesh = (m.transparency_mode == TRANSPARENCY_MODE::TRANSPARENT);

    if (rendering_only_if_textured_albedo) {
      continue;
    }
#endif

#if SHOW_BONES
    // only shows bones
    if (obj.is_joint_obj) {
      bind_vao(mesh.vao);
      draw_ebo(mesh.ebo);
      unbind_vao();
      unbind_ebo();
    }
#else

#if 1
    render_mesh(mesh);
#else
    bind_vao(mesh.vao);
    draw_ebo(mesh.ebo);
    unbind_vao();
    unbind_ebo();
#endif

#endif
  }
}

void set_material_on_model(int model_id, int mat_idx) {
  model_t& m = models[model_id];
  for (mesh_t& mesh : m.meshes) {
    mesh.mat_idx = mat_idx;    
  }
  m.is_non_opaque_mesh = !is_model_opaque(m);
}

int basic_models_t::PLANE = -1;
int basic_models_t::CUBE = -1;
int basic_models_t::SPHERE = -1;
int basic_models_t::CIRCLE = -1;

void create_basic_models() {
  char resources_path[256]{};
  get_resources_folder_path(resources_path);

  const char* basic_models_folder = "models\\basic_models";

  char basic_model_gltf_path[256]{};
  sprintf(basic_model_gltf_path, "%s\\%s\\circle.gltf", resources_path, basic_models_folder);
  gltf_load_file(basic_model_gltf_path, false);
  basic_models_t::CIRCLE = latest_model_id();

  memset(basic_model_gltf_path, 0, 256);
  sprintf(basic_model_gltf_path, "%s\\%s\\cube.gltf", resources_path, basic_models_folder);
  gltf_load_file(basic_model_gltf_path, false);
  basic_models_t::CUBE = latest_model_id();

  memset(basic_model_gltf_path, 0, 256);
  sprintf(basic_model_gltf_path, "%s\\%s\\sphere.gltf", resources_path, basic_models_folder);
  gltf_load_file(basic_model_gltf_path, false);
  basic_models_t::SPHERE = latest_model_id();

  memset(basic_model_gltf_path, 0, 256);
  sprintf(basic_model_gltf_path, "%s\\%s\\plane.gltf", resources_path, basic_models_folder);
  gltf_load_file(basic_model_gltf_path, false);
  basic_models_t::PLANE = latest_model_id();
}
