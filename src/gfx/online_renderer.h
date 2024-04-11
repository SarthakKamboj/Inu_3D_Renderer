#pragma once

#include "model_loading/model_internal.h"
#include "gfx/gfx.h"

struct offline_to_online_vertex_t {
  vec2 position; 
  vec2 tex;
};
offline_to_online_vertex_t create_offline_to_online_vertex(vec2 pos, vec2 tex);

struct online_renderer_t {
  mesh_t offline_to_online_quad; 
  offline_to_online_vertex_t verts[4];
  shader_t offline_to_online_shader;
  bool first_render = true;
};

void update_online_vertices(framebuffer_t& final_offline_fb);
void init_online_renderer();
void render_online(GLuint final_att, int render_depth = 0);
// void render_online(framebuffer_t& final_offline_fb);
