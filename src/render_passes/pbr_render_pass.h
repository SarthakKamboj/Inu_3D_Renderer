#pragma once

#include "scene/camera.h"

struct obj_sort_info_t {
  int obj_id = -1;
  vec3 pos; 
};

struct obj_sort_info_comparator_t {
  bool operator()(obj_sort_info_t a, obj_sort_info_t b) {
    vec3 cam_pos = get_cam_pos();
    vec3 diff_a = a.pos - cam_pos;
    vec3 diff_b = b.pos - cam_pos;
    float dist_a = diff_a.length();
    float dist_b = diff_b.length();
    return dist_a > dist_b;
  }
};

void init_pbr_render_pass();
void set_up_shader_for_pbr_render_pass();
void pbr_render_pass();

