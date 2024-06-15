#include "pixel_perfect_sel.h"

#include "utils/general.h"
#include "scene/camera.h"

#include <vector>

#include <cstdlib>

static selectable_id running_id = 1;
static std::vector<selectable_element_t> selectable_elements;

static int sel_fb_width = 32;
static int sel_fb_height = 32;
framebuffer_t selectable_element_t::SELECTION_FB;

shader_t selectable_element_t::SELECTION_SHADER;

void init_selection() {
  selectable_element_t::SELECTION_FB = create_framebuffer(sel_fb_width, sel_fb_height, FB_TYPE::RENDER_BUFFER_DEPTH_STENCIL);

  char resources_path[256]{};
  get_resources_folder_path(resources_path);
  char vert_shader_path[256]{};
  sprintf(vert_shader_path, "%s\\shaders\\selection.vert", resources_path);
  char frag_shader_path[256]{};
  sprintf(frag_shader_path, "%s\\shaders\\selection.frag", resources_path);
  selectable_element_t::SELECTION_SHADER = create_shader(vert_shader_path, frag_shader_path);
}

selectable_id create_selectable_element() {
  selectable_element_t el;
  el.id = running_id;
  running_id++;
  vec3 color;
  float x = rand();
  float y = rand();
  float z = rand();
  color.x = x / RAND_MAX;
  color.y = y / RAND_MAX;
  color.z = z / RAND_MAX;
  el.color = color;
  selectable_elements.push_back(el);
  return el.id;
}

void render_sel_elements() {
  bind_framebuffer(selectable_element_t::SELECTION_FB);
  clear_framebuffer();

  shader_t& shader = selectable_element_t::SELECTION_SHADER;

  mat4 proj = get_cam_proj_mat();
  mat4 view = get_cam_view_mat();
  shader_set_mat4(shader, "projection", proj);
  shader_set_mat4(shader, "view", view);

  for (selectable_element_t& sel_el : selectable_elements) {
    shader_set_mat4(shader, "model", sel_el.model_mat);
    shader_set_vec3(shader, "color", sel_el.color);
  }
}
