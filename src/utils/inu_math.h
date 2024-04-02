#pragma once

#include "vectors.h"

// LIST OF FUNCTIONS YOU WILL NEED TO CREATE/IMPLEMENT
// DO THESE FOR VECTOR2 AND VECTOR3s
// dot product
// cross product
// vector multiplication
//    - vector + vector
//    - vector + float
// vector magnitude
// addition
// subtraction
// division (make sure not divide by 0)
// normalize


struct vec2;
struct vec3;

float dot_product(const vec2& vect1, const vec2& vect2);
float dot_product(const vec3& vect1, const vec3& vect2);

vec3 cross_product(const vec3& vect1, const vec3& vect2);

float magnitude(const vec2& vect);
float magnitude(const vec3& vect);

vec2 addition(const vec2& vect1, const vec2& vect2);
vec2 subtraction(const vec2& vect1, const vec2& vect2);
vec2 division(const vec2& vect, float scalar);

vec3 addition(const vec3& vect1, const vec3& vect2);
vec3 subtraction(const vec3& vect1, const vec3& vect2);
vec3 division(const vec3& vect, float scalar);

vec2 normalize(const vec2& vect);
vec3 normalize(const vec3& vect);