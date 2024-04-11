#include <cmath> 

#include "inu_math.h"

float dot_product(const vec2& vect1, const vec2& vect2) {
    return vect1.x * vect2.x + vect1.y * vect2.y;
}

float dot_product(const vec3& vect1, const vec3& vect2) {
    return vect1.x * vect2.x + vect1.y * vect2.y + vect1.z * vect2.z;
}

vec3 cross_product(const vec3& vect1, const vec3& vect2) {
    return {
        vect1.y * vect2.z - vect1.z * vect2.y,
        vect1.z * vect2.x - vect1.x * vect2.z,
        vect1.x * vect2.y - vect1.y * vect2.x
    };
}

float magnitude(const vec2& vect) {
    return std::sqrt(vect.x * vect.x + vect.y * vect.y);
}

float magnitude(const vec3& vect) {
    return std::sqrt(vect.x * vect.x + vect.y * vect.y + vect.z * vect.z);
}

vec2 addition(const vec2& vect1, const vec2& vect2) {
    return { vect1.x + vect2.x, vect1.y + vect2.y };
}

vec2 subtraction(const vec2& vect1, const vec2& vect2) {
    return { vect1.x - vect2.x, vect1.y - vect2.y };
}

vec2 division(const vec2& vect, float scalar) {
    if (scalar != 0)
        return { vect.x / scalar, vect.y / scalar };
    else
        return { 0, 0 };
}

vec3 addition(const vec3 & vect1, const vec3 & vect2) {
	return { vect1.x + vect2.x, vect1.y + vect2.y, vect1.z + vect2.z };
}

vec3 subtraction(const vec3 & vect1, const vec3 & vect2) {
	return { vect1.x - vect2.x, vect1.y - vect2.y, vect1.z - vect2.z };
}

vec3 division(const vec3 & vect, float scalar) {
	if (scalar != 0)
		return { vect.x / scalar, vect.y / scalar, vect.z / scalar };
	else
		return { 0, 0, 0 };
}

vec2 normalize(const vec2& vect) {
	int mag = magnitude(vect);
	if (mag != 0)
		return { vect.x / mag, vect.y / mag };
	else
		return { 0, 0 };
}

vec3 normalize(const vec3& vect) {
	int mag = magnitude(vect);
	if (mag != 0)
		return { vect.x / mag, vect.y / mag, vect.z / mag };
	else
		return { 0, 0, 0 };
}
