#include "pixel_perfect_sel.h"

#include "utils/general.h"
#include "scene/camera.h"
#include "scene/scene.h"
#include "geometry/model.h"
#include "gfx_api/gfx.h"
#include "animation/skin.h"
#include "windowing/window.h"
#include "move_tool.h"

#include <vector>
#include <unordered_map>

#include "glew.h"

#include <cstdlib>

extern window_t window;
extern float width_of_screen;
extern float fb_width_on_screen_px;

static selectable_id running_id = 1;
static std::vector<selectable_element_t> selectable_elements;

static std::unordered_map<object_id, selectable_id> obj_id_to_sel_id;

static selection_state_t selection_state;

static int sel_fb_width = 160;
static int sel_fb_height = 90;

static selectable_id selection_id = 0;

shader_t selectable_element_t::SELECTION_SHADER;
framebuffer_t selectable_element_t::SELECTION_FB;

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

selectable_id create_selectable_element(object_id obj_id) {
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

  el.obj_id = obj_id;

  obj_id_to_sel_id[el.obj_id] = el.id;

  selectable_elements.push_back(el);
  return el.id;
}

selectable_element_t* get_sel_el_for_obj(object_id obj_id) {
  selectable_id sel_id = obj_id_to_sel_id[obj_id];
  if (sel_id != -1) {
    return &selectable_elements[sel_id - 1];
  }
  return NULL;
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

#if 0
void set_selection(selectable_id id) {
  selection_id = id;
}
#endif

void handle_selection_logic() {

  selection_state.left_mouse_down_selected = -1;
  selection_state.left_mouse_up_selected = -1;

  bool sel_will_update = window.input.left_mouse_up || window.input.left_mouse_down;
  selectable_id sel_id = -1;
  if (sel_will_update) {
    vec3 pixel_color = get_sel_pixel_color();
    printf("pixel_color: ");
    print_vec3(pixel_color);
    printf("\n");
    sel_id = get_sel_el_from_color(pixel_color);
  }
  if (window.input.left_mouse_up) {
    selection_state.left_mouse_up_selected = sel_id;
  } else if (window.input.left_mouse_down) {
    selection_state.left_mouse_down_selected = sel_id;
  }
}

bool is_obj_selected_left_mouse_up(object_id obj_id) {
  if (obj_id_to_sel_id.find(obj_id) == obj_id_to_sel_id.end()) return false;
  return obj_id_to_sel_id[obj_id] == selection_state.left_mouse_up_selected;
}

int get_obj_selected_left_mouse_up() {
  for (selectable_element_t& set : selectable_elements) {
    if (selection_state.left_mouse_up_selected == set.id) {
      return set.obj_id;
    }
  }
  return -1;
}

int get_obj_selected_left_mouse_down() {
  for (selectable_element_t& set : selectable_elements) {
    if (selection_state.left_mouse_down_selected == set.id) {
      return set.obj_id;
    }
  }
  return -1;
}

bool is_obj_selected_left_mouse_down(object_id obj_id) {
  if (obj_id_to_sel_id.find(obj_id) == obj_id_to_sel_id.end()) return false;
  return obj_id_to_sel_id[obj_id] == selection_state.left_mouse_down_selected;
}

selectable_element_t get_sel_el(selectable_id id) {
  return selectable_elements[id-1];
}

void render_obj_into_pixel_perfect_fb(selectable_element_t& sel_el, object_t& obj, object_id obj_model_id) {
  shader_t& shader = selectable_element_t::SELECTION_SHADER;

  shader_set_vec3(shader, "color", sel_el.color);

  if (obj_has_skin(obj.id)) {
    set_skin_in_shader_for_obj(shader, obj.id);
  } else {
    shader_set_int(shader, "skinned", 0);
    shader_set_mat4(shader, "model", obj.model_mat);
  } 

  bind_shader(shader);
  render_model_w_no_material_bind(obj_model_id);
}

void selection_render_pass() {
  bind_framebuffer(selectable_element_t::SELECTION_FB);
  clear_framebuffer();

  shader_t& shader = selectable_element_t::SELECTION_SHADER;

  mat4 proj = get_cam_proj_mat();
  mat4 view = get_cam_view_mat();
  shader_set_mat4(shader, "projection", proj);
  shader_set_mat4(shader, "view", view);

  scene_iterator_t iterator = create_scene_iterator(OBJECT_FLAGS::SELECTABLE);
  object_id obj_id = iterate_scene_for_next_obj(iterator);
  do {
    object_t* obj_p = get_obj(obj_id);
    inu_assert(obj_p);
    object_t& obj = *obj_p;

    object_id obj_model_id = get_obj_model_id(obj_id);
    if (obj_model_id != -1) {
      selectable_element_t* sel_el = get_sel_el_for_obj(obj_id);
      if (sel_el) {
        render_obj_into_pixel_perfect_fb(*sel_el, obj, obj_model_id);
      }
    }

    obj_id = iterate_scene_for_next_obj(iterator);
  }
  while (obj_id != -1);

  unbind_shader();
  unbind_framebuffer();
}

vec3 get_sel_pixel_color() {
  bind_framebuffer(selectable_element_t::SELECTION_FB);
  glReadBuffer(GL_COLOR_ATTACHMENT0);

  vec2 mouse_pct{}; 
  mouse_pct.x = static_cast<float>(window.input.mouse_pos.x) / static_cast<float>(window.window_dim.x);
  mouse_pct.y = static_cast<float>(window.input.mouse_pos.y) / static_cast<float>(window.window_dim.y);

  float gaps = window.window_dim.x - fb_width_on_screen_px;
  float gap_half = gaps / 2.0f;

  if (window.input.mouse_pos.x < gap_half ||
     (window.input.mouse_pos.x > (gap_half + fb_width_on_screen_px) )) {
    vec3 z{};
    return z;
  }

  vec2 mouse_rel_to_render{};
  mouse_rel_to_render.x = window.input.mouse_pos.x - gap_half;
  mouse_rel_to_render.y = window.input.mouse_pos.y;

  vec2 mouse_rel_to_render_pct{};
  mouse_rel_to_render_pct.x = mouse_rel_to_render.x / fb_width_on_screen_px;
  mouse_rel_to_render_pct.y = mouse_rel_to_render.y / window.window_dim.y;

  ivec2 sel_fb_row_col{};
  sel_fb_row_col.x = mouse_rel_to_render_pct.x * selectable_element_t::SELECTION_FB.width;
  sel_fb_row_col.y = mouse_rel_to_render_pct.y * selectable_element_t::SELECTION_FB.height;

  // printf("row, col: (%i %i)\n", sel_fb_row_col.x, sel_fb_row_col.y);

  float pixel[4]{};
  glReadPixels(sel_fb_row_col.x, sel_fb_row_col.y, 1, 1, GL_RGBA, GL_FLOAT, pixel);
  // printf("color: (%f %f %f)\n", pixel[0], pixel[1], pixel[2], pixel[3]);

  vec3 final_color = {pixel[0], pixel[1], pixel[2]};
 
  unbind_framebuffer();
  return final_color;
}
