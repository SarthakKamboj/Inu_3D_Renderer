#pragma once

#include <vector>
#include <stdint.h>

#include "utils/vectors.h"
#include "utils/log.h"
#include "gfx/gfx.h"

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
};

int register_model(model_t& model);
int latest_model_id();
model_t* get_model(int model_id);
