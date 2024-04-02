#include "inu_math.h"
#include <cmath>
#include "vectors.h"
#include <iostream>
#include <cassert>


Vector2D::Vector2D():x(0),y(0){}
Vector2D::Vector2D(float x, float y) : x(x), y(y) {}
Vector2D::~Vector2D(){}


Vector3D::Vector3D() : x(0), y(0), z(0) {}
Vector3D::Vector3D(float x, float y, float z) : x(x), y(y), z(z) {}
Vector3D::~Vector3D() {}



Vector2D Vector2D::addition(const Vector2D& vect) const { //add 2 vectors
	return Vector2D(x + vect.x, y + vect.y);
}

Vector3D Vector3D::addition(const Vector3D& vect) const { //add 2 vectors
	return Vector3D(x + vect.x, y + vect.y, z+vect.x);
}

Vector2D Vector2D::subtraction(const Vector2D& vect) const {	//subtract 2 vectors
	return Vector2D(x - vect.x, y - vect.y);
}

Vector3D Vector3D::subtraction(const Vector3D& vect) const {	//subtract 2 vectors
	return Vector3D(x - vect.x, y - vect.y, z-vect.z);
}

Vector2D Vector2D::multiplication(float scalar) const {	//multiply vector by a scalar
	return Vector2D(x*scalar, y*scalar);
}

//multiple 2D vectors
Vector3D Vector3D::multiplication(float scalar) const { //add 2 vectors
	return Vector3D(x * scalar, y * scalar, z*scalar);
}

Vector2D Vector2D::multiplication(const Vector2D& vect) const {
	return Vector2D(x * vect.x, y * vect.y);

}Vector3D Vector3D::multiplication(const Vector3D& vect) const {
	return Vector3D(x * vect.x, y * vect.y, z*vect.z);
}

Vector2D Vector2D::division(float scalar) const {	//multiply vector by a scalar
	if (scalar != 0) {
		return Vector2D(x / scalar, y / scalar);
	}
	else {
		return Vector2D();
	}
}


Vector3D Vector3D::division(float scalar) const {	//multiply vector by a scalar
	if (scalar != 0) {
		return Vector3D(x / scalar, y / scalar, z / scalar);
	}
	else {
		return Vector3D();
	}
}



Vector2D Vector2D::division(const Vector2D& vect) const {	
	if (vect.x!=0 && vect.y !=0) {
		return Vector2D(x / vect.x, y/vect.y);
	}
	else {
		return Vector2D();
	}
}

Vector3D Vector3D::division(const Vector3D& vect) const {	//multiply vector by a scalar
	if (vect.x != 0 && vect.y != 0 && vect.z !=0) {
		return Vector3D(x / vect.x, y / vect.y, z / vect.z);
	}
	else {
		return Vector3D();
	}
}

float Vector2D::magnitude() const {
	return std::sqrt(x * x + y * y);
}

float Vector3D::magnitude() const {
	return std::sqrt(x * x + y * y +z * z);
}


Vector2D Vector2D::normalize() const {
	float m = magnitude();
	if (m <= 0) return Vector2D();
	else {
		return Vector2D(x / m, y / m);
	}
}


Vector3D Vector3D::normalize() const {
	float m = magnitude();
	if (m <= 0) return Vector3D();
	else {
		return Vector3D(x / m, y / m, z / m);
	}
}


float Vector2D::dotproduct(const Vector2D& vect) const {
	return x * vect.x + y*vect.y;
}

float Vector3D::dotproduct(const Vector3D& vect) const {
	return x * vect.x + y * vect.y + z * vect.z;
}

float Vector2D::crossproduct(const Vector2D& vect) const {
	return x * vect.x - y * vect.y;
}

float Vector3D::crossproduct(const Vector3D& vect) const {
	return x * vect.x - y * vect.y - z * vect.z;
}
