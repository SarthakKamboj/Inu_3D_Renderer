#pragma once

// actual Win32 include
#include <windows.h>

#include "utils/vectors.h"

struct input_t {
  ivec2 mouse_pos_diff; 
  ivec2 mouse_pos; 
  float scroll_wheel_delta = 0;
  bool middle_mouse_down = false;
  bool left_mouse_up = false;
  bool right_mouse_up = false;
  bool shift_down = false;

  bool a_up = false;
  bool b_up = false;
  bool c_up = false;
};

struct window_t {
  input_t input; 
  ivec2 window_dim;
  bool resized = false;

  HWND win32_wnd = NULL;
  bool running = true;
};

void create_window(HINSTANCE h_instance, int width, int height);
void poll_events();
void swap_buffers();
