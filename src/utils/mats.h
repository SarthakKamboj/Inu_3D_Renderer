#pragma once

#include "vectors.h"

#include "glm/glm.hpp"

struct mat2 {
  union {
    float vals[4];
    struct {
      float m11,m21;
      float m12,m22;
    };
    vec2 cols[2];
    struct {
      vec2 first_col;
      vec2 second_col;
    };
    struct {
      float a,c;
      float b,d;
    };
  };
  mat2();

  float determinant();
};

struct mat3 {
  union {
    float vals[9];
    struct {
      float m11,m21,m31;
      float m12,m22,m32;
      float m13,m23,m33;
    };
    vec3 cols[3];
    struct {
      vec3 first_col;
      vec3 second_col;
      vec3 third_col;
    };
  };
  mat3();

  float determinant();
};

struct mat4 {
  union {
    float vals[16];
    struct {
      float m11,m21,m31,m41;
      float m12,m22,m32,m42;
      float m13,m23,m33,m43;
      float m14,m24,m34,m44;
    };
    vec4 cols[4];
    struct {
      vec4 first_col;
      vec4 second_col;
      vec4 third_col;
      vec4 fourth_col;
    };
  };
  mat4();
  mat4(float diag_val);

  mat4 operator*(mat4& other);
  vec4 operator*(vec4& v);

  float determinant();
  mat3 minor(int row, int col);
  mat4 inverse();
  mat4 transpose();
};

template <typename T>
float determinant(T& mat) {
  return mat.determinant();
}

void print_mat4(mat4& mat);

mat4 proj_mat(float fov, float near, float far, float aspect_ratio);
mat4 ortho_mat(float x_min, float x_max, float y_min, float y_max, float z_near, float z_far);
mat4 scale_mat(float s);
mat4 scale_mat(vec3& scale);
mat4 translate_mat(vec3& p);

void glm_to_internal(glm::mat4& m1, mat4& m2);
void internal_to_glm(mat4& m1, glm::mat4& m2);
