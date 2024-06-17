#include "pixel_perfect_sel.h"

#include "utils/general.h"
#include "scene/camera.h"
#include "scene/scene.h"

#include <vector>

#include <cstdlib>

static selectable_id running_id = 1;
static std::vector<selectable_element_t> selectable_elements;

static int sel_fb_width = 160;
static int sel_fb_height = 90;
framebuffer_t selectable_element_t::SELECTION_FB;

static selectable_id selection_id = 0;

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

void update_sel_el_on_obj(int obj_id) {
  object_t* obj_p = get_obj(obj_id);
  object_t& obj = *obj_p;
  if (obj.sel_id != -1) {
    mat4 model_mat = get_obj_model_mat(obj_id);
    selectable_elements[obj.sel_id - 1].model_mat = model_mat;
  }
}

selectable_id get_sel_el_from_color(vec3 color) {
  ivec3 color_cmp = color * 100.0f;
  for (selectable_element_t& sel_el : selectable_elements) {
    ivec3 icolor = sel_el.color * 100.0f;
    // add epsilons for more leniant comparison
    if (abs(color_cmp.x - icolor.x) <= 2 &&
        abs(color_cmp.y - icolor.y) <= 2&&
        abs(color_cmp.z - icolor.z) <= 2
        ) {
      return sel_el.id;
    }
  }
  return 0;
}

void set_selection(selectable_id id) {
  selection_id = id;
}

bool is_obj_selected(object_t& obj) {
  return obj.sel_id == selection_id;
}

#if 0
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
    
    bind_shader(shader);

  }
}
#endif

selectable_element_t get_sel_el(selectable_id id) {
  return selectable_elements[id-1];
}
