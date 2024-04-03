#pragma once

struct vec2 {
  float x = 0;
  float y = 0;
};
vec2 norm_vec2(vec2& v);

struct ivec2 {
  int x = 0;
  int y = 0;
};
float length(ivec2& v);

struct vec3 {
  float x = 0;
  float y = 0;
  float z = 0;
  // float operator[](int idx);
  float& operator[](int idx);
};
void print_vec3(vec3& v);
vec3 vec3_add(vec3& v1, vec3& v2);
vec3 norm_vec3(vec3& v);
vec3 cross_product(vec3& v1, vec3& v2);
float dot(vec3& v1, vec3& v2);
float length(vec3& v);
bool operator==(const vec3& v1, const vec3& v2);

struct vec4 {
  float x = 0;
  float y = 0;
  float z = 0;
  float w = 0;

  vec4();
  vec4(float _x, float _y, float _z, float _w);
  vec4(vec3& v, float _w);

  vec4 operator/(float divider);
  float& operator[](int idx);
};
float vec4_length(vec4& v);
