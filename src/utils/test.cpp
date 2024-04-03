# include <iostream>
# include "inu_math.h"

using namespace std;
//
void testDot2() {
	vec2 a = {1,2};
	vec2 b = {3,4};

	float result = dot_product(a, b);
	float expected = 11;

	if (result == expected) {
		printf("Dot product (vec2) test passed");
	}
	else {
        printf("Dot product(vec2) test failed");
	}
}

void testDotProductVec3() {
    vec3 a = { 1, 2, 3 };
    vec3 b = { 4, 5, 6 };
    float result = dot_product(a, b);
    float expected = 32;
    if (result == expected) {
        printf("Dot product (vec3) test passed.");
    }
    else {
        printf("Dot product (vec3) test failed.");
    }
}

void testCrossProduct() {
    vec3 a = { 1, 2, 3 };
    vec3 b = { 4, 5, 6 };
    vec3 result = cross_product(a, b);
    vec3 expected = { -3, 6, -3 };
    if (result.x == expected.x && result.y == expected.y && result.z == expected.z) {
        printf("Cross product test passed.");
    }
    else {
        printf("Cross product test failed.");
    }
}

void testMagnitudeVec2() {
    vec2 a = { 3, 4 };
    float result = magnitude(a);
    float expected = 5;
    if (result == expected) {
        printf("Magnitude (vec2) test passed.");
    }
    else {
        printf("Magnitude (vec2) test failed.");
    }
}

void testSubtractionVec2() {
    vec2 a = { 5, 6 };
    vec2 b = { 3, 2 };
    vec2 result = subtraction(a, b);
    vec2 expected = { 2, 4 };
    if (result.x == expected.x && result.y == expected.y) {
        printf("Subtraction (vec2) test passed.");
    }
    else {
        printf("Subtraction (vec2) test failed.");
    }
}

void testDivisionVec2() {
    vec2 a = { 10, 20 };
    float scalar = 2;
    vec2 result = division(a, scalar);
    vec2 expected = { 5, 10 };
    if (result.x == expected.x && result.y == expected.y) {
        printf("Division (vec2) test passed.");
    }
    else {
        printf("Division (vec2) test failed.");
    }
}

void testNormalizationVec3() {
    vec3 a = { 3, 1, 2 };
    vec3 result = normalize(a);
    float length = sqrt(result.x * result.x + result.y * result.y + result.z * result.z);
    if (length == 1.0) {
        printf("Normalization (vec3) test passed.");
    }
    else {
        printf("Normalization (vec3) test failed.");
    }
}



