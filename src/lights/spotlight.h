#pragma once

#include "utils/transform.h"
#include "gfx_api/gfx.h"
#include "app_includes.h"

struct spotlight_t {
  int id = -1;
  transform_t transform;
  float radius = 1.f;
  static const float NEAR_PLANE;
  static const float FAR_PLANE;
  static const float SHADOW_MAP_WIDTH;
  static const float SHADOW_MAP_HEIGHT;
  vec3 dir;
  vec3 color;

#if SHOW_LIGHTS
  static int LIGHT_MESH_ID;
#endif
  static shader_t light_shader;
  framebuffer_t light_pass_fb;

  mat4 view;
  mat4 proj;
};

void init_light_data();
void init_spotlight_data();
int create_spotlight(vec3 pos);
void setup_spotlight_for_rendering(int light_id);
void remove_spotlight_from_rendering();
spotlight_t get_spotlight(int light_id);
int get_num_spotlights();
tex_id_t get_spotlight_fb_depth_tex(int light_id);
mat4 get_spotlight_proj_mat(int light_id);
mat4 get_spotlight_view_mat(int light_id);
vec3 get_spotlight_pos(int light_id);
void spotlight_pass();

