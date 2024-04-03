#include "vectors.h"

#include "utils/log.h"

#include <math.h>
#include <stdio.h>

vec2 norm_vec2(vec2& v) {
  float length = sqrt(v.x*v.x + v.y*v.y); 
  vec2 res;
  res.x = v.x / length;
  res.y = v.y / length;
  return res;
}

float length(ivec2& v) {
  return sqrt(v.x*v.x + v.y*v.y);
}

vec3 norm_vec3(vec3& v) {
  float length = sqrt(v.x*v.x + v.y*v.y + v.z*v.z); 
  vec3 res;
  res.x = v.x / length;
  res.y = v.y / length;
  res.z = v.z / length;
  return res;
}

vec3 vec3_add(vec3& v1, vec3& v2) {
  return {v1.x+v2.x, v1.y+v2.y, v1.z+v2.z};
}

void print_vec3(vec3& v) {
  printf("%f, %f, %f", v.x, v.y, v.z);
}

float& vec3::operator[](int idx) {
  inu_assert(idx < 3, "idx cannot be more than 2");
  float* f = &x;
  return *(f+idx);
}

#if 0
float vec3::operator[](int idx) {
  inu_assert(idx < 3, "idx cannot be more than 2");
  float* f = &x;
  return *(f+idx);
}
#endif

vec3 cross_product(vec3& v1, vec3& v2) {
  vec3 c;
  float a1 = v1.x;
  float a2 = v1.y;
  float a3 = v1.z;
  float b1 = v2.x;
  float b2 = v2.y;
  float b3 = v2.z;
  c.x = (a2 * b3) - (a3 * b2);
  c.y = -( (a1*b3) - (a3*b1) );
  c.z = (a1*b2 - a2*b1);
  return c;
}

float dot(vec3& v1, vec3& v2) {
  return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
}

float length(vec3& v) {
  return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

bool operator==(const vec3& v1, const vec3& v2) {
  return (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z);
}

float vec4_length(vec4& v) {
  return sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
}

float& vec4::operator[](int idx) {
  inu_assert(idx < 4, "idx cannot be more than 4");
  float* f = &x;
  return *(f+idx);
}

vec4 vec4::operator/(float divider) {
  vec4 r;
  r.x = this->x / divider;
  r.y = this->y / divider;
  r.z = this->z / divider;
  r.w = this->w / divider;
  return r;
}

vec4::vec4() {}

vec4::vec4(float _x, float _y, float _z, float _w) {
  x = _x;
  y = _y;
  z = _z;
  w = _w;
}

vec4::vec4(vec3& v, float _w) {
  x = v.x;
  y = v.y;
  z = v.z;
  w = _w;
}
