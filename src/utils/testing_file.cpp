#include <iostream>
#include "inu_math.h"

class MathTest {
public:
    void runTests() {
        testVec2Operations();
        testVec3Operations();
    }

private:
    void testVec2Operations() {
        vec2 v1 = { 3, 4 };
        vec2 v2 = { 1, 2 };

        std::cout << "Testing vec2 operations:" << std::endl;
        std::cout << "Dot product of v1 and v2: " << dot_product(v1, v2) << std::endl;
        std::cout << "Magnitude of v1: " << magnitude(v1) << std::endl;
        std::cout << "Addition of v1 and v2: (" << addition(v1, v2).x << ", " << addition(v1, v2).y << ")" << std::endl;
        std::cout << "Subtraction of v1 and v2: (" << subtraction(v1, v2).x << ", " << subtraction(v1, v2).y << ")" << std::endl;
        std::cout << "Normalization of v1: (" << normalize(v1).x << ", " << normalize(v1).y << ")" << std::endl;
    }

    void testVec3Operations() {
        vec3 u1 = { 1, 2, 3 };
        vec3 u2 = { 4, 5, 6 };

        std::cout << "\nTesting vec3 operations:" << std::endl;
        std::cout << "Dot product of u1 and u2: " << dot_product(u1, u2) << std::endl;
        std::cout << "Magnitude of u1: " << magnitude(u1) << std::endl;
        std::cout << "Addition of u1 and u2: (" << addition(u1, u2).x << ", " << addition(u1, u2).y << ", " << addition(u1, u2).z << ")" << std::endl;
        std::cout << "Subtraction of u1 and u2: (" << subtraction(u1, u2).x << ", " << subtraction(u1, u2).y << ", " << subtraction(u1, u2).z << ")" << std::endl;
        std::cout << "Normalization of u1: (" << normalize(u1).x << ", " << normalize(u1).y << ", " << normalize(u1).z << ")" << std::endl;
    }
};

int main() {
    MathTest mathTest;
    mathTest.runTests();

    return 0;
}
