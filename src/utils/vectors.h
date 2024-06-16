#pragma once

struct vec2 {
  float x = 0;
  float y = 0;

  float length();
  vec2 operator/(float divider);
};

struct ivec2 {
  int x = 0;
  int y = 0;

  float length();
};

struct vec3;
struct ivec3 {
  int x = 0; 
  int y = 0; 
  int z = 0; 

  ivec3() = default;
  ivec3(vec3& other);
  ivec3 operator=(vec3& other);
};

struct vec3 {
  float x = 0;
  float y = 0;
  float z = 0;

  float& operator[](int idx);
  vec3 operator/(float divider);
  vec3 operator*(float multiplier);
  vec3 operator-(vec3 v);
  vec3 operator+(vec3 v);
  bool operator==(const vec3& v);

  float length();
};
void print_vec3(vec3& v);

vec3 cross_product(vec3& v1, vec3& v2);
float dot(vec3& v1, vec3& v2);

struct vec4 {
  float x = 0;
  float y = 0;
  float z = 0;
  float w = 0;

  vec4();
  vec4(float _x, float _y, float _z, float _w);
  vec4(vec2& v, float _z, float _w);
  vec4(vec3& v, float _w);

  vec4 operator/(float divider);
  vec4 operator*(float multiplier);
  vec4 operator+(vec4& other);
  float& operator[](int idx);
  bool operator==(const vec4& v);

  float length();
};

template<typename T>
T normalize(T& v) {
  T res = v / v.length();
  return res;
}

template <typename T>
float length(T& v) {
  return v.length();
}
