#pragma once

#include "utils/vectors.h"
#include "utils/mats.h"

struct quaternion_t {
  float x = 0;
  float y = 0;
  float z = 0;
  float w = 1;
};

quaternion_t create_quaternion(float x, float y, float z, float w);
quaternion_t create_quaternion_w_rot(vec3 axis, float angle);
quaternion_t quat_inverse(quaternion_t& q);
vec3 get_rotated_position(vec3& pos, quaternion_t& q);
vec3 get_rotated_position_raw(vec3& pos, quaternion_t& q);
quaternion_t quat_multiply_quat(quaternion_t& q1, quaternion_t& q2);
mat4 quat_as_mat4(quaternion_t& q1);
float quat_mag(quaternion_t& q);
float quat_dot(quaternion_t& a, quaternion_t& b);
quaternion_t quat_multiply_float(quaternion_t& q1, float f);
quaternion_t quat_add_quat(quaternion_t& q1, quaternion_t& q2);
quaternion_t norm_quat(quaternion_t& q);
void print_quat(quaternion_t& q);
bool is_nan_quat(quaternion_t& q);
