#pragma once

#include "utils/quaternion.h"
#include "utils/vectors.h"

quaternion_t spherical_linear(quaternion_t& qa, quaternion_t& qb, float t);
float linear(float a, float b, float t);
vec3 vec3_linear(vec3 a, vec3 b, float t);
