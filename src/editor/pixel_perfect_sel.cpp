#include "pixel_perfect_sel.h"

#include "utils/general.h"
#include "scene/camera.h"
#include "scene/scene.h"
#include "model_loading/model_internal.h"
#include "gfx_api/gfx.h"

#include <vector>
#include <unordered_map>

#include <cstdlib>

static selectable_id running_id = 1;
static std::vector<selectable_element_t> selectable_elements;

static std::unordered_map<int, selectable_id> obj_id_to_sel_id;

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

selectable_id create_selectable_element(int obj_id) {
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

selectable_element_t* get_sel_el_for_obj(int obj_id) {
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

void set_selection(selectable_id id) {
  selection_id = id;
}

bool is_obj_selected(object_t& obj) {
  if (obj_id_to_sel_id.find(obj.id) == obj_id_to_sel_id.end()) return false; 
  return obj_id_to_sel_id[obj.id] == selection_id;
}

selectable_element_t get_sel_el(selectable_id id) {
  return selectable_elements[id-1];
}

void render_obj_into_pixel_perfect_fb(selectable_element_t& sel_el, object_t& obj) {
  shader_t& shader = selectable_element_t::SELECTION_SHADER;

  shader_set_vec3(shader, "color", sel_el.color);

  if (obj.is_skinned) {
    skin_t skin = get_skin(obj.skin_id);
    shader_set_int(shader, "skinned", 1);
    for (int i = 0; i < skin.num_bones; i++) {
      char mat4_name[64]{};
      sprintf(mat4_name, "joint_inverse_bind_mats[%i]", i);
      shader_set_mat4(shader, mat4_name, skin.inverse_bind_matricies[i]);

      memset(mat4_name, 0, sizeof(mat4_name));
      sprintf(mat4_name, "joint_model_matricies[%i]", i);
      mat4 joint_model_matrix = get_obj_model_mat(skin.joint_obj_ids[i]);
      shader_set_mat4(shader, mat4_name, joint_model_matrix);
    }

    // setting the rest to defaults
    for (int i = skin.num_bones; i < BONES_PER_SKIN_LIMIT; i++) {
      mat4 identity(1.0f);
      char mat4_name[64]{};
      sprintf(mat4_name, "joint_inverse_bind_mats[%i]", i);
      shader_set_mat4(shader, mat4_name, identity);

      memset(mat4_name, 0, sizeof(mat4_name));
      sprintf(mat4_name, "joint_model_matricies[%i]", i);
      shader_set_mat4(shader, mat4_name, identity);
    }
  } else {
    shader_set_int(shader, "skinned", 0);
    shader_set_mat4(shader, "model", obj.model_mat);
  }

  bind_shader(shader);
  render_sel_model_w_no_material_bind(obj.model_id);
}

void selection_render_pass() {
  bind_framebuffer(selectable_element_t::SELECTION_FB);
  clear_framebuffer();

  shader_t& shader = selectable_element_t::SELECTION_SHADER;

  mat4 proj = get_cam_proj_mat();
  mat4 view = get_cam_view_mat();
  shader_set_mat4(shader, "projection", proj);
  shader_set_mat4(shader, "view", view);

  scene_iterator_t iterator = create_scene_iterator();
  int obj_id = iterate_scene_for_next_obj(iterator);
  do {
    object_t* obj_p = get_obj(obj_id);
    inu_assert(obj_p);
    object_t& obj = *obj_p;

    selectable_element_t* sel_el = get_sel_el_for_obj(obj_id);
    if (sel_el) {
      render_obj_into_pixel_perfect_fb(*sel_el, obj);
    }

    obj_id = iterate_scene_for_next_obj(iterator);
  }
  while (obj_id != -1);

  unbind_shader();
  unbind_framebuffer();
}
