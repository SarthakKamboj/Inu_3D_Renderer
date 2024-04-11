#include "transform.h"

#define _USE_MATH_DEFINES
#include "math.h"
#include <cstdio>

#include "utils/log.h"

mat4 get_model_matrix(transform_t& t) {
  mat4 scale = scale_mat(t.scale);
  mat4 translate = translate_mat(t.pos);
  mat4 rot = quat_as_mat4(t.rot);
#if 0
  mat4 inter = mat_multiply_mat(translate, scale);
  mat4 model = mat_multiply_mat(inter, rot);
#else
  mat4 inter = mat_multiply_mat(translate, rot);
  mat4 model = mat_multiply_mat(inter, scale);
#endif
  return model;
}

// this method only works if scale is positive
transform_t get_transform_from_matrix(mat4& m) {
  transform_t t;  

  // get position
  t.pos.x = m.fourth_col.x;
  t.pos.y = m.fourth_col.y;
  t.pos.z = m.fourth_col.z;

  // get scale
  t.scale.x = vec4_length(m.first_col);
  t.scale.y = vec4_length(m.second_col);
  t.scale.z = vec4_length(m.third_col);

  // get rotation
  mat4 rot_mat = create_matrix(1.0f);
  rot_mat.cols[0] = m.cols[0] / t.scale.x;
  rot_mat.cols[1] = m.cols[1] / t.scale.y;
  rot_mat.cols[2] = m.cols[2] / t.scale.z;
#if 0
  vec4 fir = m.sep_cols.first_col / t.scale.x;
  vec4 sec = m.sep_cols.second_col / t.scale.y;
  vec4 thi = m.sep_cols.third_col / t.scale.z;

  t.rot.z = sqrt(
    (thi.y + sec.z)*(thi.x + fir.z)/(4*(sec.x + fir.y))
  );
  t.rot.y = (thi.y + sec.z) / (4*t.rot.z);
  t.rot.x = (sec.x + fir.y) / (4*t.rot.y);
  t.rot.w = (sec.x - (2*t.rot.x*t.rot.y)) / (-2*t.rot.z);
#else
  // float m11 = m.sep_cols.first_col.x / t.scale.x;
  // float m22 = m.sep_cols.second_col.y / t.scale.y;
  // float m33 = m.sep_cols.third_col.z / t.scale.z;
  float n = 1 + rot_mat.m11 + rot_mat.m22 + rot_mat.m33;
  // solves issue where when n is really really small but not 0, sqrt() returns nan for some reason
  if (fabs(n) < 0.01f) {
    n = 0;
  }
  t.rot.w = sqrt(n/4.f);
  float d = t.rot.w;

  if (d != 0) {
    t.rot.x = (rot_mat.m32 - rot_mat.m23) / (4*d);
    t.rot.y = (rot_mat.m13 - rot_mat.m31) / (4*d);
    t.rot.z = (rot_mat.m21 - rot_mat.m12) / (4*d);
  } else {
    // we have a 180 degree rotation
    float k = rot_mat.m11 + rot_mat.m22;
    if (fabs(k) < 0.01f) {
      k = 0;
    }
    t.rot.z = sqrt(k / -2.f);
    if (t.rot.z != 0) {
      float c = t.rot.z;
      t.rot.x = rot_mat.m13 / (2*c);
      t.rot.y = rot_mat.m23 / (2*c);
    } else {
      float l = rot_mat.m11 + rot_mat.m33;
      if (fabs(l) < 0.01f) {
        l = 0;
      }
      t.rot.y = sqrt(l / -2.f);
      if (t.rot.y != 0) {
        t.rot.x = rot_mat.m21 / (2 * t.rot.y);
      } else {
        float h = rot_mat.m22 + rot_mat.m33;
        if (fabs(h) < 0.01f) {
          h = 0;
        }
        t.rot.x = sqrt(h / -2.f);
      }
    }
  }
#if 0
  float m21 = m.sep_cols.first_col.y;
  float m12 = m.sep_cols.second_col.x;

  float m13 = m.sep_cols.third_col.x;
  float m31 = m.sep_cols.first_col.z;

  float m32 = m.sep_cols.
#endif
#if 0
  float d2 = t.rot.w * t.rot.w;
  t.rot.x = sqrt(d2-((m22+m33)/2.f));
  t.rot.y = sqrt(d2-((m11+m33)/2.f));
  t.rot.z = sqrt(d2-((m11+m22)/2.f));

  // returns 0 to pi
  float abs_angle_rad = acos(t.rot.w);
  float neg_pi_to_pi_angle = abs_angle_rad;
  if (abs_angle_rad > (M_PI / 2.f)) {
    // neg_pi_to_pi_angle -= M_PI;
    t.rot.w *= -1;
  }
  // b/c abs_angle_rad in [0,pi], sin(abs_angle_rad) will always be positive
#if 0
  float imaginary_scalar = sin(abs_angle_rad);
  float q_x_scaled = t.rot.x / imaginary_scalar;
  float q_y_scaled = t.rot.y / imaginary_scalar;
  float q_z_scaled = t.rot.z / imaginary_scalar;
#endif
#endif

#endif
  float mag = quat_mag(t.rot);
  inu_assert(mag != 0.f, "quaternion is of magntitude 0");
  // inu_assert(mag >= 0.96f, "quaternion is not close enough to magntitude 1");
  t.rot = norm_quat(t.rot);
  return t;
}

void print_transform(transform_t& t) {
  printf("pos: %f, %f, %f\n", t.pos.x, t.pos.y, t.pos.z);
  printf("quat: %f, %f, %f, %f\n", t.rot.x, t.rot.y, t.rot.z, t.rot.w);
  printf("scale: %f, %f, %f\n", t.scale.x, t.scale.y, t.scale.z);
}
