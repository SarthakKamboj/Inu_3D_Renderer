#include "online_renderer.h"

#include "utils/general.h"
#include "windowing/window.h"
#include "gfx_api/gfx.h"

static online_renderer_t online_renderer;
extern framebuffer_t offline_fb;

extern window_t window;

offline_to_online_vertex_t create_offline_to_online_vertex(vec2 pos, vec2 tex) {
  offline_to_online_vertex_t v;
  v.position = pos;
  v.tex = tex;
  return v;
}

float fb_width_on_screen_px;
float width_of_screen;
void update_online_vertices(framebuffer_t& final_offline_fb) {
  mesh_t& offline_to_online_quad = online_renderer.offline_to_online_quad;
#if 1
  float fb_ratio_height = static_cast<float>(window.window_dim.y) / final_offline_fb.height;
  // float fb_width_on_screen = static_cast<float>(final_offline_fb.width) * fb_ratio_height ;
  fb_width_on_screen_px = static_cast<float>(final_offline_fb.width) * fb_ratio_height;
  // float width_of_screen = fb_width_on_screen / window.window_dim.x;
  width_of_screen = fb_width_on_screen_px / window.window_dim.x;
  float not_width_of_screen = 1.f - width_of_screen;
  online_renderer.verts[0] = create_offline_to_online_vertex({-1+not_width_of_screen,-1}, {0,0});
  online_renderer.verts[1] = create_offline_to_online_vertex({1-not_width_of_screen,-1}, {1,0});
  online_renderer.verts[2] = create_offline_to_online_vertex({1-not_width_of_screen,1}, {1,1});
  online_renderer.verts[3] = create_offline_to_online_vertex({-1+not_width_of_screen,1}, {0,1});
#else
  float fb_ratio = static_cast<float>(final_offline_fb.width) / final_offline_fb.height;
  float win_ratio = static_cast<float>(window.window_dim.x) / window.window_dim.y;
  float width_of_screen = fb_ratio / win_ratio;
  float not_width_of_screen = 1.f - width_of_screen;
  online_renderer.verts[0] = create_offline_to_online_vertex({-1+not_width_of_screen,-1}, {0,0});
  online_renderer.verts[1] = create_offline_to_online_vertex({1-not_width_of_screen,-1}, {1,0});
  online_renderer.verts[2] = create_offline_to_online_vertex({1-not_width_of_screen,1}, {1,1});
  online_renderer.verts[3] = create_offline_to_online_vertex({-1+not_width_of_screen,1}, {0,1});
#endif
  update_vbo_data(offline_to_online_quad.vbo, (float*)online_renderer.verts, sizeof(online_renderer.verts));
}

void init_online_renderer() {
  char resources_path[256]{};
  get_resources_folder_path(resources_path);
  printf("resources_path: %s\n", resources_path);

  char vert_offline_to_online_path[256]{};
  sprintf(vert_offline_to_online_path, "%s\\shaders\\offline_to_online.vert", resources_path);
  char frag_offline_to_online_path[256]{};
  sprintf(frag_offline_to_online_path, "%s\\shaders\\offline_to_online.frag", resources_path);
  online_renderer.offline_to_online_shader = create_shader(vert_offline_to_online_path, frag_offline_to_online_path);
  shader_set_int(online_renderer.offline_to_online_shader, "img", 0);

  unsigned int indicies[6] {
    0,2,3,
    1,2,0
  };

  mesh_t& offline_to_online_quad = online_renderer.offline_to_online_quad;
  offline_to_online_quad.vao = create_vao();

  offline_to_online_quad.vbo = create_dyn_vbo(sizeof(online_renderer.verts));

  offline_to_online_quad.ebo = create_ebo(indicies, sizeof(indicies));

  vao_enable_attribute(offline_to_online_quad.vao, offline_to_online_quad.vbo, 0, 2, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(offline_to_online_vertex_t), offsetof(offline_to_online_vertex_t, position));
  vao_enable_attribute(offline_to_online_quad.vao, offline_to_online_quad.vbo, 1, 2, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(offline_to_online_vertex_t), offsetof(offline_to_online_vertex_t, tex));
  vao_bind_ebo(offline_to_online_quad.vao, offline_to_online_quad.ebo);
}

void render_online(tex_id_t final_att, int render_depth) {
  unbind_framebuffer();

  if (online_renderer.first_render || window.resized) {
    online_renderer.first_render = false;
    // need to change this logic to not rely on another fbo but rather just the online fbo and/or window dimensions
    update_online_vertices(offline_fb);
  }

  shader_set_int(online_renderer.offline_to_online_shader, "render_depth", render_depth);
  clear_framebuffer();
#if 0
  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, final_att);
#else
  bind_texture(final_att, 0);
#endif
  bind_shader(online_renderer.offline_to_online_shader);
  bind_vao(online_renderer.offline_to_online_quad.vao);
  draw_ebo(online_renderer.offline_to_online_quad.ebo);
  unbind_vao();
  unbind_ebo();
}
