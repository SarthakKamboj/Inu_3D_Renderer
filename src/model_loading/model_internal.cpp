#include "model_internal.h"

#include <vector>

std::vector<model_t> models;

int register_model(model_t& model) {
  model.id = models.size();

  for (mesh_t& mesh : model.meshes) {
    material_t m = get_material(mesh.mat_idx);
    bool non_opaque = (m.transparency_mode != TRANSPARENCY_MODE::OPQUE);
    if (non_opaque) {
      model.is_non_opaque_mesh = true;
      break;
    }
  }

  models.push_back(model);
  return model.id;
}

int latest_model_id() {
  return models[models.size()-1].id;
}

model_t* get_model(int model_id) {
  return &models[model_id];
}

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
