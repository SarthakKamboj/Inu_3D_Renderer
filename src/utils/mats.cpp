#include "mats.h"

#include <memory>
#include <cmath>

// mat 2x2
mat2::mat2() {
  memset(cols, 0, sizeof(cols));
}

float determinant(mat2& m) {
  return (m.a*m.d) - (m.b*m.c);
}

// mat 3x3
mat3::mat3() {
  memset(cols, 0, sizeof(cols));
}

float determinant(mat3& m) {
  int sign = 1; 
  float d = 0;
  for (int row = 0; row < 3; row++) {
    float place_sign = sign * m.first_col[row];
    sign *= -1;
    mat2 minor;
    float* f_p = &minor.m11;

    for (int tbt_col = 1; tbt_col < 3; tbt_col++) {
      for (int tbt_row = 0; tbt_row < 3; tbt_row++) {
        if (tbt_row == row) continue;
        *f_p = m.cols[tbt_col][tbt_row];
        f_p++;
      }
    }

    float minor_val = determinant(minor);
    d += (minor_val * place_sign);
  }

  return d;
}

mat4::mat4() {
  memset(cols, 0, sizeof(cols));
}

mat4 create_matrix(float diag_val) {
  mat4 m;
  m.first_col.x = diag_val;
  m.second_col.y = diag_val;
  m.third_col.z = diag_val;
  m.fourth_col.w = diag_val;
  return m;
}

mat4 mat_multiply_mat(mat4& m1, mat4& m2) {
  mat4 res;
  res.first_col = mat_multiply_vec(m1, m2.first_col);
  res.second_col = mat_multiply_vec(m1, m2.second_col);
  res.third_col = mat_multiply_vec(m1, m2.third_col);
  res.fourth_col = mat_multiply_vec(m1, m2.fourth_col);
  return res;
}

vec4 vec_multiply_float(vec4& v, float f) {
  vec4 res;
  res.x = v.x * f;
  res.y = v.y * f;
  res.z = v.z * f;
  res.w = v.w * f;
  return res;
}

vec4 mat_multiply_vec(mat4& m, vec4& v) {

  vec4 c0 = vec_multiply_float(m.cols[0], v.x);
  vec4 c1 = vec_multiply_float(m.cols[1], v.y);
  vec4 c2 = vec_multiply_float(m.cols[2], v.z);
  vec4 c3 = vec_multiply_float(m.cols[3], v.w);

  vec4 res;
  res.x = c0.x + c1.x + c2.x + c3.x;
  res.y = c0.y + c1.y + c2.y + c3.y;
  res.z = c0.z + c1.z + c2.z + c3.z;
  res.w = c0.w + c1.w + c2.w + c3.w;

  return res;
}


// z_min is further away dir light, this is far
// z_max to closer to dir light, this is near
// z_min needs to be put to 1
// z_max needs to be put to -1
mat4 ortho_mat(float x_min, float x_max, float y_min, float y_max, float z_min, float z_max) {
  // ortho matrix brings (-right,right) to (-1,1)
  // brings (-top,top) to (-1,1)
  // brings (near [z_max], far[z_min]) to (-1,1)
  // brings to origin
  mat4 translate = create_matrix(1.0f);
  translate.fourth_col.x = (x_max+x_min) / -2.f;
  translate.fourth_col.y = (y_max+y_min) / -2.f;
  translate.fourth_col.z = (z_max+z_min) / -2.f;

  float hor_scale = (x_max - x_min) / 2.f;
  float ver_scale = (y_max - y_min) / 2.f;
  float forwards_scale = (z_max - z_min) / -2.f;

  mat4 scale = create_matrix(1.0f);
  scale.cols[0].x = 1/hor_scale;
  scale.cols[1].y = 1/ver_scale;
  scale.cols[2].z = 1/forwards_scale;
  mat4 ortho = mat_multiply_mat(scale, translate);

  return ortho;
}

mat4 proj_mat(float fov, float near, float far, float aspect_ratio) {
  float multipler = 3.141526f / 180.f;
  float top = near * tan(fov * multipler / 2.f);
  float right = top * aspect_ratio;
  float x_angle = atan(right / near) / multipler;
  
  // pers proj keeps +z as out of screen and -z into screen
  mat4 pers = create_matrix(1.0f);
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
  mat4 translate = create_matrix(1.0f);
  translate.fourth_col.z = (far+near) / 2.f;

  mat4 scale = create_matrix(1.0f);
  scale.cols[0].x = 1/right;
  scale.cols[1].y = 1/top;
  // the negative is b/c in world space, +z comes out of the screen and -z goes into the screen, even after perspective matrix is applied
  // we flip this when doing this ortho projection so that -1 is out of screen and +1 is into the screen
  scale.cols[2].z = -2.f/(far-near);
  mat4 ortho = mat_multiply_mat(scale, translate);

  mat4 proj = mat_multiply_mat(ortho, pers);
  return proj;
}

mat4 scale_mat(float s) {
  mat4 scale = create_matrix(1.0f);
  scale.cols[0].x = s;
  scale.cols[1].y = s;
  scale.cols[2].z = s;
  return scale;
}

mat4 scale_mat(vec3& s) {
  mat4 scale = create_matrix(1.0f);
  scale.cols[0].x = s.x;
  scale.cols[1].y = s.y;
  scale.cols[2].z = s.z;
  return scale;
}

mat4 translate_mat(vec3& p) {
  mat4 translate = create_matrix(1.0f);
  translate.cols[3].x = p.x;
  translate.cols[3].y = p.y;
  translate.cols[3].z = p.z;
  return translate;
}

mat4 transpose(mat4& m) {
  mat4 t;
  for (int i = 0; i < 4; i++) {
    t.cols[i].x = *(static_cast<float*>(&m.cols[0].x) + i);
    t.cols[i].y = *(static_cast<float*>(&m.cols[1].x) + i);
    t.cols[i].z = *(static_cast<float*>(&m.cols[2].x) + i);
    t.cols[i].w = *(static_cast<float*>(&m.cols[3].x) + i);
  }
  return t;
}

void print_mat4(mat4& mat) {
  for (int i = 0; i < 4; i++) {
    float* start_of_row = (&mat.m11) + i;
    printf("%f %f %f %f\n", *start_of_row, *(start_of_row+4), *(start_of_row+8), *(start_of_row+12));
  }
}

mat3 mat4_minor(mat4& m, int row, int col) {
  mat3 minor;
  float* f_p = &minor.m11;

  for (int tbt_col = 0; tbt_col < 4; tbt_col++) {
    for (int tbt_row = 0; tbt_row < 4; tbt_row++) {
      if (tbt_row == row || tbt_col == col) continue;
      *f_p = m.cols[tbt_col][tbt_row];
      f_p++;
    }
  }

  return minor;
}

float determinant(mat4& m) {
  int sign = 1;
  float d = 0;
  for (int row = 0; row < 4; row++) {
    float place_sign = sign * m.first_col[row];
    sign *= -1;

    mat3 minor = mat4_minor(m, row, 0);

    float minor_val = determinant(minor);
    d += (minor_val * place_sign);
  }

  return d;
}

mat4 mat4_inverse(mat4& m) {
  mat4 inv;
  float det = determinant(m); 

  for (int j = 0; j < 4; j++) {
    for (int i = 0; i < 4; i++) {
      mat3 minor = mat4_minor(m, j, i);
      float d3 = determinant(minor);
      int sign = pow(-1, i+j);
      vec4& col = inv.cols[j];
      col[i] = sign * d3 / det;
    }
  }

  return inv;
}
