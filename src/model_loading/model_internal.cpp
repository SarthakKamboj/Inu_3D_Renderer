#include "model_internal.h"

#include <vector>

#include "gfx/light.h"
#include "utils/general.h"
#include "model_loading/gltf/gltf.h"

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

void render_model(int model_id, bool light_pass, shader_t& shader) {
  model_t& model = models[model_id];
  for (mesh_t& mesh : model.meshes) {

    if (!light_pass) {
      if (model_id == spotlight_t::LIGHT_MESH_ID) {
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
    bind_vao(mesh.vao);
    draw_ebo(mesh.ebo);
    unbind_vao();
    unbind_ebo();
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

#if 0
int generate_plane(int mat_idx) {
  model_t plane_model{};

  mesh_t mesh; 

  mesh.mat_idx = mat_idx;
  mesh.vao = create_vao();

  vertex_t vert1{};
  vertex_t vert2{};
  vertex_t vert3{};
  vertex_t vert4{};

  vert1.position = {-0.5f, 0.5f, 0};
  vert2.position = {-0.5f, -0.5f, 0};
  vert3.position = {0.5f, -0.5f, 0};
  vert4.position = {0.5, 0.5f, 0};

  vert1.normal = {0, 1.f, 0};
  vert2.normal = {0, 1.f, 0};
  vert3.normal = {0, 1.f, 0};
  vert4.normal = {0, 1.f, 0};

  mesh.vertices.push_back(vert1);
  mesh.vertices.push_back(vert2);
  mesh.vertices.push_back(vert3);
  mesh.vertices.push_back(vert4);

  mesh.indicies = std::vector<unsigned int>({0,2,1,2,0,3});

  mesh.vbo = create_vbo((void*)mesh.vertices.data(), mesh.vertices.size() * sizeof(vertex_t));
  mesh.ebo = create_ebo(mesh.indicies.data(), mesh.indicies.size() * sizeof(unsigned int));

  vao_enable_attribute(mesh.vao, mesh.vbo, 0, 3, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, position));
  vao_enable_attribute(mesh.vao, mesh.vbo, 1, 2, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex0));
  vao_enable_attribute(mesh.vao, mesh.vbo, 2, 2, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex1));
  vao_enable_attribute(mesh.vao, mesh.vbo, 3, 3, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
  vao_enable_attribute(mesh.vao, mesh.vbo, 4, 4, VAO_ATTR_DATA_TYPE::UNSIGNED_INT, sizeof(vertex_t), offsetof(vertex_t, joints));
  vao_enable_attribute(mesh.vao, mesh.vbo, 5, 4, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, weights));
  vao_enable_attribute(mesh.vao, mesh.vbo, 6, 3, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, normal));

  vao_bind_ebo(mesh.vao, mesh.ebo);
  
  plane_model.meshes.push_back(mesh);

  int plane_model_id = register_model(plane_model);
  return plane_model_id;
}
#endif

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
