#include <stdio.h>

#include "windowing/window.h"
#include "utils/inu_math.h"
#include "utils/test.cpp"
#include <iostream>
#include <sstream>

using namespace std;

#define TEST_MATH 1

extern window_t window;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
#if TEST_MATH
  // add your testing functionality here that you were writing in the int main() in test.cpp
    testDot2();
    testDotProductVec3();
    testCrossProduct();
    testMagnitudeVec2();
    testSubtractionVec2();
    testDivisionVec2();
    testNormalizationVec3();



#else
  create_window(hInstance, 400, 300); 
  while (window.running) {
    poll_events();
    swap_buffers();
  }
#endif
}
