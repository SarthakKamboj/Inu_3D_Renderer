#include "mats.h"

#include <memory>
#include <cmath>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

// mat 2x2
mat2::mat2() {
  memset(cols, 0, sizeof(cols));
}

float mat2::determinant() {
  return (a*d) - (b*c);
}

// mat 3x3
mat3::mat3() {
  memset(cols, 0, sizeof(cols));
}

float mat3::determinant() {
  int sign = 1; 
  float d = 0;
  for (int row = 0; row < 3; row++) {
    float place_sign = sign * first_col[row];
    sign *= -1;
    mat2 minor;
    float* f_p = &minor.m11;

    for (int tbt_col = 1; tbt_col < 3; tbt_col++) {
      for (int tbt_row = 0; tbt_row < 3; tbt_row++) {
        if (tbt_row == row) continue;
        *f_p = cols[tbt_col][tbt_row];
        f_p++;
      }
    }

    float minor_val = minor.determinant();
    d += (minor_val * place_sign);
  }

  return d;
}

mat4::mat4() {
  memset(cols, 0, sizeof(cols));
}

mat4::mat4(float diag_val) {
  memset(cols, 0, sizeof(cols));
  first_col.x = diag_val;
  second_col.y = diag_val;
  third_col.z = diag_val;
  fourth_col.w = diag_val;
}

mat4 mat4::operator*(mat4& other) {
  mat4 res;
  res.first_col = *this * other.first_col;
  res.second_col = *this * other.second_col;
  res.third_col = *this * other.third_col;
  res.fourth_col = *this * other.fourth_col;
  return res;
}

vec4 mat4::operator*(vec4& v) {
  vec4 c0 = cols[0] * v.x;
  vec4 c1 = cols[1] * v.y;
  vec4 c2 = cols[2] * v.z;
  vec4 c3 = cols[3] * v.w;

  vec4 res = c0 + c1 + c2 + c3;

  return res;
}


// z_min is further away dir light, this is far
// z_max to closer to dir light, this is near
// z_min needs to be put to 1
// z_max needs to be put to -1
mat4 ortho_mat(float x_min, float x_max, float y_min, float y_max, float z_near, float z_far) {
#if !USE_GLM

  float z_min = -z_far;
  float z_max = -z_near;

  // ortho matrix brings (-right,right) to (-1,1)
  // brings (-top,top) to (-1,1)
  // brings (-near, -far) to (-1,1)
  // brings to origin
  mat4 translate(1.0f);
  translate.fourth_col.x = (x_max+x_min) / -2.f;
  translate.fourth_col.y = (y_max+y_min) / -2.f;
  translate.fourth_col.z = (z_max+z_min) / -2.f;

  float hor_scale = (x_max - x_min) / 2.f;
  float ver_scale = (y_max - y_min) / 2.f;
  float forwards_scale = (z_max - z_min) / -2.f;

  mat4 scale(1.0f);
  scale.cols[0].x = 1/hor_scale;
  scale.cols[1].y = 1/ver_scale;
  scale.cols[2].z = 1/forwards_scale;
  mat4 ortho = scale * translate;

  return ortho;
#else
  glm::mat4 o = glm::ortho(x_min, x_max, y_min, y_max, z_near, z_far);
  mat4 o_out(1.0f);
  glm_to_internal(o, o_out);
  return o_out;
#endif
}

void internal_to_glm(mat4& m1, glm::mat4& m2) {
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      m2[col][row] = m1.cols[col][row];
    }
  }
}

void glm_to_internal(glm::mat4& m1, mat4& m2) {
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      m2.cols[col][row] = m1[col][row];
    }
  }
}

mat4 proj_mat(float fov, float near, float far, float aspect_ratio) {
#if !USE_GLM
  float multipler = 3.141526f / 180.f;
  float top = near * tan(fov * multipler / 2.f);
  float right = top * aspect_ratio;
  float x_angle = atan(right / near) / multipler;
  
  // pers proj keeps +z as out of screen and -z into screen
  mat4 pers(1.0f);
  pers.cols[0].x = near;

  pers.cols[1].y = near;

  pers.cols[2].z = near + far;
  pers.cols[2].w = -1.f;

  pers.cols[3].z = near * far;
  pers.cols[3].w = 0.f;

  // ortho matrix brings (-right,right) to (-1,1)
  // brings (-top,top) to (-1,1)
  // brings (near,far) to (-1,1)
  // brings to origin
  mat4 translate(1.0f);
  translate.fourth_col.z = (far+near) / 2.f;

  mat4 scale(1.0f);
  scale.cols[0].x = 1/right;
  scale.cols[1].y = 1/top;
  // the negative is b/c in world space, +z comes out of the screen and -z goes into the screen, even after perspective matrix is applied
  // we flip this when doing this ortho projection so that -1 is out of screen and +1 is into the screen
  scale.cols[2].z = -2.f/(far-near);
  mat4 ortho = scale * translate;

  mat4 proj = ortho * pers;
  return proj;
#else
  glm::mat4 p = glm::perspective(fov / 2.f, aspect_ratio, near, far);
  mat4 i(1.0f);
  glm_to_internal(p, i);
  return i;
#endif
}

mat4 scale_mat(float s) {
  mat4 scale(1.0f);
  scale.cols[0].x = s;
  scale.cols[1].y = s;
  scale.cols[2].z = s;
  return scale;
}

mat4 scale_mat(vec3& s) {
  mat4 scale(1.0f);
  scale.cols[0].x = s.x;
  scale.cols[1].y = s.y;
  scale.cols[2].z = s.z;
  return scale;
}

mat4 translate_mat(vec3& p) {
  mat4 translate(1.0f);
  translate.cols[3].x = p.x;
  translate.cols[3].y = p.y;
  translate.cols[3].z = p.z;
  return translate;
}

mat4 mat4::transpose() {
  mat4 t{};
  for (int i = 0; i < 4; i++) {
    t.cols[i].x = *(static_cast<float*>(&cols[0].x) + i);
    t.cols[i].y = *(static_cast<float*>(&cols[1].x) + i);
    t.cols[i].z = *(static_cast<float*>(&cols[2].x) + i);
    t.cols[i].w = *(static_cast<float*>(&cols[3].x) + i);
  }
  return t;
}

void print_mat4(mat4& mat) {
  for (int i = 0; i < 4; i++) {
    float* start_of_row = (&mat.m11) + i;
    printf("%f %f %f %f\n", *start_of_row, *(start_of_row+4), *(start_of_row+8), *(start_of_row+12));
  }
}

mat3 mat4::minor(int row, int col) {
  mat3 minor;
  float* f_p = &minor.m11;

  for (int tbt_col = 0; tbt_col < 4; tbt_col++) {
    for (int tbt_row = 0; tbt_row < 4; tbt_row++) {
      if (tbt_row == row || tbt_col == col) continue;
      *f_p = cols[tbt_col][tbt_row];
      f_p++;
    }
  }

  return minor;
}

float mat4::determinant() {
  int sign = 1;
  float d = 0;
  for (int row = 0; row < 4; row++) {
    float place_sign = sign * first_col[row];
    sign *= -1;

    mat3 m = minor(row, 0);

    float minor_val = m.determinant();
    d += (minor_val * place_sign);
  }

  return d;
}

mat4 mat4::inverse() {
#if !USE_GLM
  mat4 inv{};
  float det = determinant(); 

  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      mat3 m = minor(j, i);
      float d3 = m.determinant();
      int sign = pow(-1, i+j);
      vec4& col = inv.cols[j];
      col[i] = sign * d3 / det;
    }
  }

  return inv;
#else
  glm::mat4 m2(1.0f);
  internal_to_glm(m, m2);
  glm::mat4 inv = glm::inverse(m2);
  mat4 out(1.0f);
  glm_to_internal(inv, out);
  return out;
#endif
}
