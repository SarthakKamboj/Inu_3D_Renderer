#include "window.h"

#include "utils/log.h"

#include <iostream>

#include <Windowsx.h>
#include <wingdi.h>
#include <winuser.h>
#include "glew.h"
#include "wglew.h"
// #include <utils/wglext.h>

#include "gfx/gfx.h"
#include "utils/inu_time.h"

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define SHOW_CONSOLE 1

window_t window;

LRESULT CALLBACK window_procedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void create_window(HINSTANCE h_instance, int width, int height) {

#if SHOW_CONSOLE
  AllocConsole();
  AttachConsole(GetCurrentProcessId());
  freopen("CON", "w", stdout);
#endif

  const char* CLASS_NAME = "Inu Renderer";

  WNDCLASS win_class = {};
  win_class.lpfnWndProc = window_procedure;
  win_class.hInstance = h_instance;
  win_class.lpszClassName = TEXT(CLASS_NAME);

  RegisterClass(&win_class);

  RECT adjusted_win_size{};
  adjusted_win_size.right = width;
  adjusted_win_size.bottom = height;
  AdjustWindowRectEx(
      &adjusted_win_size, 
      WS_OVERLAPPEDWINDOW,
      FALSE,
      0
  );

  window.win32_wnd = CreateWindowEx(
    0,
    TEXT(CLASS_NAME),
    TEXT("Inu 3D Renderer"),
    WS_OVERLAPPEDWINDOW,

    CW_USEDEFAULT, 
    CW_USEDEFAULT, 

    // width and height to this function needs to include non-client and client area combined
    adjusted_win_size.right - adjusted_win_size.left, 
    adjusted_win_size.bottom - adjusted_win_size.top,

    NULL,
    NULL,
    h_instance,
    NULL
  );

  inu_assert(window.win32_wnd != NULL, "window could not be created");

  window.window_dim.x = width;
  window.window_dim.y = height;

  window.running = true;

  ShowWindow(window.win32_wnd, SW_SHOWNORMAL);

  // OPENGL INITIALIZATION
  HDC device_context = GetDC(window.win32_wnd);

  PIXELFORMATDESCRIPTOR pixel_format = { 
    sizeof(PIXELFORMATDESCRIPTOR),   // size of this pfd  
    1,                     // version number  
    PFD_DRAW_TO_WINDOW |   // support window  
    PFD_SUPPORT_OPENGL |   // support OpenGL  
    PFD_DOUBLEBUFFER,      // double buffered  
    PFD_TYPE_RGBA,         // RGBA type  
    24,                    // 24-bit color depth  
    0, 0, 0, 0, 0, 0,      // color bits ignored  
    0,                     // no alpha buffer  
    0,                     // shift bit ignored  
    0,                     // no accumulation buffer  
    0, 0, 0, 0,            // accum bits ignored  
    32,                    // 32-bit z-buffer  
    0,                     // no stencil buffer  
    0,                     // no auxiliary buffer  
    PFD_MAIN_PLANE,        // main layer  
    0,                     // reserved  
    0, 0, 0                // layer masks ignored  
  };

  int pixel_format_idx = ChoosePixelFormat(device_context, &pixel_format);
  SetPixelFormat(device_context, pixel_format_idx, &pixel_format);

  int gl_attribs[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
      WGL_CONTEXT_MINOR_VERSION_ARB, 1,
      WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
      0,
  };

  HGLRC gl_render_context_for_context_func = wglCreateContext(device_context);
  wglMakeCurrent(device_context, gl_render_context_for_context_func);
  PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC) wglGetProcAddress("wglCreateContextAttribsARB");
  HGLRC gl_render_context = wglCreateContextAttribsARB(device_context, nullptr, gl_attribs);
  wglMakeCurrent(device_context, NULL);
  wglDeleteContext(gl_render_context_for_context_func);
  wglMakeCurrent(device_context, gl_render_context);

  HDC gl_device_context = wglGetCurrentDC();
  inu_assert(gl_device_context == device_context, "opengl device context not the same as the window's\n");

  GLenum err = glewInit();
  inu_assert(err == GLEW_OK, glewGetErrorString(err)); 

  printf("version: %s\n", glGetString(GL_VERSION));
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glFrontFace(GL_CCW);
  // glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
}

LRESULT CALLBACK window_procedure(HWND h_window, UINT u_msg, WPARAM w_param, LPARAM l_param) {
  switch (u_msg) {
    case WM_SIZE: {
      window.resized = true;
      unsigned int width = LOWORD(l_param);
      unsigned int height = HIWORD(l_param);
      glViewport(0, 0, width, height);
      window.window_dim.x = width;
      window.window_dim.y = height;
      break;
    }
    case WM_DESTROY: {
      window.running = false;
      PostQuitMessage(0);
      return 0;
    }
    case WM_LBUTTONUP: {
      window.input.left_mouse_up = true;
      break;
    }
    case WM_RBUTTONUP: {
      window.input.right_mouse_up = true;
      break;
    }
    case WM_MOUSEMOVE: {
      // bottom left should be (0,0)
      ivec2 orig = {window.input.mouse_pos.x, window.input.mouse_pos.y};
      window.input.mouse_pos.x = GET_X_LPARAM(l_param);
      window.input.mouse_pos.y = window.window_dim.y - GET_Y_LPARAM(l_param);
      window.input.middle_mouse_down = (w_param & MK_MBUTTON) != 0;

      window.input.mouse_pos_diff.x = window.input.mouse_pos.x - orig.x;
      window.input.mouse_pos_diff.y = window.input.mouse_pos.y - orig.y;
      break;
    }
    case WM_MOUSEWHEEL: {
      // fwKeys = GET_KEYSTATE_WPARAM(wParam);
      float scroll_wheel_delta = GET_WHEEL_DELTA_WPARAM(w_param);
      window.input.scroll_wheel_delta = scroll_wheel_delta / WHEEL_DELTA;
      return 0;
    }
    case WM_KEYUP: {
      switch (w_param) {
        case VK_SHIFT: {
          window.input.shift_down = false;
          break;
        }
        case 0x41: {
          window.input.a_up = true;
          break;
        }
        case 0x42: {
          window.input.b_up = true;
          break;
        }
        case 0x43: {
          window.input.c_up = true;
          break;
        }
      }
      break; 
    }
    case WM_KEYDOWN: {
      switch (w_param) {
        case VK_SHIFT: {
          window.input.shift_down = true;
          break;
        }
      }
      break; 
    }
    case WM_CHAR: {
      int a = 5;
      break; 
    }
    case WM_SYSKEYUP: {
      int a = 5;
      break; 
    }
  }
  return DefWindowProc(h_window, u_msg, w_param, l_param);
}

void poll_events() {
  window.resized = false;
  window.input.scroll_wheel_delta = 0;
  window.input.middle_mouse_down = false;
  window.input.mouse_pos_diff = {0,0};
  window.input.left_mouse_up = false;
  window.input.right_mouse_up = false;

  window.input.a_up = false;
  window.input.b_up = false;
  window.input.c_up = false;
  // window.input.shift_down = false;
  MSG msg{};
  while (PeekMessage(&msg, window.win32_wnd, 0, 0, 0)) {
    bool quit_msg = (GetMessage(&msg, NULL, 0, 0) == 0);
    if (quit_msg) break;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void swap_buffers() {
  HDC gl_device_context = wglGetCurrentDC();
  SwapBuffers(gl_device_context);
}
