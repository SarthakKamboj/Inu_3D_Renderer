#pragma once

#include <vector>
#include <stdint.h>

#include "utils/vectors.h"
#include "utils/log.h"
#include "gfx_api/gfx.h"

struct vertex_t {
  vec3 position; 
  vec2 tex0;
  vec2 tex1;
  vec3 normal;
  vec3 color;
  unsigned int joints[4];
  float weights[4];
};

struct mesh_t {
  vao_t vao;

  vbo_t vbo;
  std::vector<vertex_t> vertices; 

  ebo_t ebo;
  std::vector<unsigned int> indicies;

  int mat_idx;
};

struct model_t {
  int id;
  std::vector<mesh_t> meshes;
  bool is_non_opaque_mesh = false;
  bool hidden = false;
};

void model_toggle_hidden(int obj_id, bool val);
bool is_model_opaque(model_t& model);
bool is_model_opaque(int model_id);
int register_model(model_t& model);
int get_obj_model_id(int obj_id);
void attach_model_to_obj(int obj_id, int model_id);
int latest_model_id();
model_t* get_model(int model_id);
void render_model(int model_id, bool light_pass, shader_t& shader);
void render_model_w_no_material_bind(int model_id);
void set_material_on_model(int model_id, int mat_idx);

void render_mesh(mesh_t& mesh);

// these models should have only 1 mesh
struct basic_models_t {
  static int PLANE;
  static int CUBE;
  static int SPHERE;
  static int CIRCLE;
};

void create_basic_models();
