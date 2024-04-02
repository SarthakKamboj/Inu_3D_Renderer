# include <iostream>
#inlcude "inu_math.h"

using namespace std;
//
void testDot2() {
	vec2 a = {1,2};
	vec2 b = {3,4};

	float result = dot_product(a, b);
	float expected = 11;

	if (result == expected) {
		print("Dot product (vec2) test passed");
	}
	else {
		print("Dot product(vec2) test failed");
	}
}

void testDotProductVec3() {
    vec3 a = { 1, 2, 3 };
    vec3 b = { 4, 5, 6 };
    float result = dot_product(a, b);
    float expected = 32;
    if (result == expected) {
        print("Dot product (vec3) test passed.");
    }
    else {
        print("Dot product (vec3) test failed.");
    }
}

void testCrossProduct() {
    vec3 a = { 1, 2, 3 };
    vec3 b = { 4, 5, 6 };
    vec3 result = cross_product(a, b);
    vec3 expected = { -3, 6, -3 };
    if (result.x == expected.x && result.y == expected.y && result.z == expected.z) {
        print("Cross product test passed.");
    }
    else {
        print("Cross product test failed.");
    }
}

void testMagnitudeVec2() {
    vec2 a = { 3, 4 };
    float result = magnitude(a);
    float expected = 5;
    if (result == expected) {
        print("Magnitude (vec2) test passed.");
    }
    else {
        print("Magnitude (vec2) test failed.");
    }
}

void testSubtractionVec2() {
    vec2 a = { 5, 6 };
    vec2 b = { 3, 2 };
    vec2 result = subtraction(a, b);
    vec2 expected = { 2, 4 };
    if (result.x == expected.x && result.y == expected.y) {
        print("Subtraction (vec2) test passed.");
    }
    else {
        print("Subtraction (vec2) test failed.");
    }
}

void testDivisionVec2() {
    vec2 a = { 10, 20 };
    float scalar = 2;
    vec2 result = division(a, scalar);
    vec2 expected = { 5, 10 };
    if (result.x == expected.x && result.y == expected.y) {
        print("Division (vec2) test passed.");
    }
    else {
        print("Division (vec2) test failed.");
    }
}

void testNormalizationVec3() {
    vec3 a = { 3, 1, 2 };
    vec3 result = normalize(a);
    float length = sqrt(result.x * result.x + result.y * result.y + result.z * result.z);
    if (length == 1.0) {
        print("Normalization (vec3) test passed.");
    }
    else {
        print("Normalization (vec3) test failed.");
    }
}



int main() {
	testDot2();
    testDotProductVec3();
    testCrossProduct();
    testMagnitudeVec2();
    testSubtractionVec2();
    testDivisionVec2();
    testNormalizationVec3()


	return 0;
}
