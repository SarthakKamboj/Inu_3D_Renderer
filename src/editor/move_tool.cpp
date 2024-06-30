#include "move_tool.h"

#include "geometry/gltf/gltf.h"
#include "utils/general.h"
#include "scene/scene.h"
#include "editor/editor_pass.h"
#include "windowing/window.h"
#include "pixel_perfect_sel.h"

extern window_t window;

static move_tool_t move_tool;

bool currently_selecting() {
  return move_tool.selected_x || move_tool.selected_y || move_tool.selected_z;
}

void init_move_tool() {
  char resources_path[256]{};
  get_resources_folder_path(resources_path);

  char gltf_full_file_path[256]{};
  sprintf(gltf_full_file_path, "%s\\editor\\arrow.gltf", resources_path);
  gltf_load_file(gltf_full_file_path, false);

  move_tool.transform.scale = {1,1,1};
  move_tool.arrow_id = latest_model_id();

  int move_tool_parent_obj_id = create_object(move_tool.transform, OBJECT_FLAGS::EDITOR_OBJ);
  attach_name_to_obj(move_tool_parent_obj_id, std::string("move tool"));
  set_obj_as_parent(move_tool_parent_obj_id);

  editor_mat_t editor_mat; 

  transform_t t{};
  t.scale = {1,1,1};

  OBJECT_FLAGS arrow_flags = OBJECT_FLAGS::EDITOR_OBJ | OBJECT_FLAGS::SELECTABLE;

  t.rot = create_quaternion_w_rot({1,0,0}, 90.f);
  
  int z_obj_id = create_object(t, arrow_flags);
  attach_name_to_obj(z_obj_id, std::string("z arrow"));
  attach_child_obj_to_obj(move_tool_parent_obj_id, z_obj_id);
  attach_model_to_obj(z_obj_id, move_tool.arrow_id);

  editor_mat.color = {0,0,1};
  attach_editor_mat_to_obj(z_obj_id, editor_mat);

  t.rot = create_quaternion_w_rot({0,0,1}, -90.f);
  int x_obj_id = create_object(t, arrow_flags);
  attach_name_to_obj(x_obj_id, std::string("x arrow"));
  attach_child_obj_to_obj(move_tool_parent_obj_id, x_obj_id);
  attach_model_to_obj(x_obj_id, move_tool.arrow_id);

  editor_mat.color = {1,0,0};
  attach_editor_mat_to_obj(x_obj_id, editor_mat);

  t.rot = create_quaternion_w_rot({0,0,1}, 0.f);
  int y_obj_id = create_object(t, arrow_flags);
  attach_name_to_obj(y_obj_id, std::string("y arrow"));
  attach_child_obj_to_obj(move_tool_parent_obj_id, y_obj_id);
  attach_model_to_obj(y_obj_id, move_tool.arrow_id);

  editor_mat.color = {0,1,0};
  attach_editor_mat_to_obj(y_obj_id, editor_mat);

  move_tool.x_obj_id = x_obj_id;
  move_tool.z_obj_id = z_obj_id;
  move_tool.y_obj_id = y_obj_id;
  move_tool.parent_obj = move_tool_parent_obj_id;

  model_toggle_hidden(x_obj_id, true);
}

void update_move_tool() {

  if (window.input.left_mouse_up) {
    if (!currently_selecting()) {
      int new_selected = get_obj_selected_left_mouse_up();
      if (new_selected != -1) {
        model_toggle_hidden(move_tool.x_obj_id, false);
        mat4 model_mat = get_obj_model_mat(new_selected);
        std::string selected_obj_name = get_obj_name(new_selected);
        transform_t t = get_transform_from_matrix(model_mat);
        printf("selected %s\n", selected_obj_name.c_str());
        print_transform(t);
        set_move_tool_as_transform(t);
      } else {
        model_toggle_hidden(move_tool.x_obj_id, true);
      }
    } else {
      move_tool.selected_x = false;
      move_tool.selected_y = false;
      move_tool.selected_z = false;
      model_toggle_hidden(move_tool.x_obj_id, false);
    }
  }

#if 0
  if (window.input.left_mouse_down) {
    int new_selected = get_obj_selected_left_mouse_down();
    if (new_selected == move_tool.x_obj_id ||
        new_selected == move_tool.y_obj_id ||
        new_selected == move_tool.z_obj_id) {
      model_toggle_hidden(move_tool.x_obj_id, false);
      mat4 model_mat = get_obj_model_mat(new_selected);
      std::string selected_obj_name = get_obj_name(new_selected);
      transform_t t = get_transform_from_matrix(model_mat);
      printf("selected %s\n", selected_obj_name.c_str());
      print_transform(t);
      set_move_tool_as_transform(t);
    } else {
      model_toggle_hidden(move_tool.x_obj_id, true);
    }
  }
#endif

#if 1
  if (window.input.left_mouse_down) {
    if ( is_obj_selected_left_mouse_down(move_tool.x_obj_id)
      || is_obj_selected_left_mouse_down(move_tool.y_obj_id)
      || is_obj_selected_left_mouse_down(move_tool.z_obj_id)) {

      model_toggle_hidden(move_tool.x_obj_id, true);

      move_tool.selected_x = false;
      move_tool.selected_y = false;
      move_tool.selected_z = false;

      if (is_obj_selected_left_mouse_down(move_tool.x_obj_id)) {
        move_tool.selected_x = true;  
        printf("x selected\n");
      } else if (is_obj_selected_left_mouse_down(move_tool.y_obj_id)) {
        move_tool.selected_y = true;  
        printf("y selected\n");
      } else if (is_obj_selected_left_mouse_down(move_tool.z_obj_id)) {
        move_tool.selected_z = true;  
        printf("z selected\n");
      }
    } else { 
      model_toggle_hidden(move_tool.x_obj_id, false);
    }
  }
#endif

  if (currently_selecting()) {
    ivec2 diff = window.input.mouse_pos_diff;

    vec3 pos_diff = {move_tool.selected_x, move_tool.selected_y, move_tool.selected_z};

    if (diff.x != 0) {
      vec3 offset = pos_diff * diff.x * 0.2f;
      // move_tool.transform.pos = move_tool.transform.pos + offset;
      // set_transform_on_obj(move_tool.parent_obj, move_tool.transform);
    }
  }
}

void set_move_tool_as_position(vec3 pos) {
  move_tool.transform.pos = pos;
  set_transform_on_obj(move_tool.parent_obj, move_tool.transform);
}

void set_move_tool_as_transform(transform_t& t) {
  move_tool.transform = t;
  set_transform_on_obj(move_tool.parent_obj, move_tool.transform);
}
