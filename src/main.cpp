#include <stdio.h>

#include "windowing/window.h"

#define TEST_MATH 1

extern window_t window;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
#if TEST_MATH
  // add your testing functionality here that you were writing in the int main() in test.cpp
#else
  create_window(hInstance, 400, 300); 
  while (window.running) {
    poll_events();
    swap_buffers();
  }
#endif
}
