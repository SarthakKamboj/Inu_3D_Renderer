#include "editor_pass.h"

#include "utils/general.h"
#include "scene/scene.h"
#include "scene/camera.h"
#include "geometry/model.h"

static editor_system_t editor_system;

extern framebuffer_t offline_fb;

void init_editor_system() {
  char resources_path[256]{};
  get_resources_folder_path(resources_path);

  char vert_shader_path[256]{};
  sprintf(vert_shader_path, "%s\\editor\\editor.vert", resources_path);
  char frag_shader_path[256]{};
  sprintf(frag_shader_path, "%s\\editor\\editor.frag", resources_path);
  editor_system.editor_shader = create_shader(vert_shader_path, frag_shader_path);
}

void setup_editor_obj_in_shader(int obj_id) {
  mat4 model_mat = get_obj_model_mat(obj_id);
  shader_set_mat4(editor_system.editor_shader, "model", model_mat);

  editor_mat_t& mat = editor_system.obj_to_editor_mat[obj_id];
  shader_set_vec3(editor_system.editor_shader, "color", mat.color);
}

void attach_editor_mat_to_obj(int obj_id, editor_mat_t& mat) {
  editor_system.obj_to_editor_mat[obj_id] = mat;
}

void render_editor_obj(int obj_id, int model_id) {
  shader_t& shader = editor_system.editor_shader;

  setup_editor_obj_in_shader(obj_id);

  bind_shader(shader);

  if (is_obj_selected(obj_id)) {
    set_render_mode(RENDER_MODE::WIREFRAME);
  }

  render_model_w_no_material_bind(model_id);

  if (is_obj_selected(obj_id)) {
    set_render_mode(RENDER_MODE::NORMAL);
  }
}

void editor_pass() {
  // PBR RENDER PASS
  bind_framebuffer(offline_fb);

  mat4 proj = get_cam_proj_mat();
  mat4 view = get_cam_view_mat();
  shader_set_mat4(editor_system.editor_shader, "projection", proj);
  shader_set_mat4(editor_system.editor_shader, "view", view);

  scene_iterator_t scene_iterator = create_scene_iterator(OBJECT_FLAGS::EDITOR_OBJ);

  int obj_id = iterate_scene_for_next_obj(scene_iterator);
  do {
    int model_id = get_obj_model_id(obj_id);
    if (model_id != -1) {
      render_editor_obj(obj_id, model_id);
    }
    obj_id = iterate_scene_for_next_obj(scene_iterator);
  }
  while (obj_id != -1);
}
