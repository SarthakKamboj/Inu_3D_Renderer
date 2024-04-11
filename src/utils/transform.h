#pragma once

#include "utils/vectors.h"
#include "utils/quaternion.h"
#include "utils/mats.h"

struct transform_t {
  vec3 pos;
  vec3 scale;
  // vec3 rot;
  quaternion_t rot;
};

mat4 get_model_matrix(transform_t& t);
transform_t get_transform_from_matrix(mat4& m);
void print_transform(transform_t& t);
