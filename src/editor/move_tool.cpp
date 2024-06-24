#include "move_tool.h"

#include "geometry/gltf/gltf.h"
#include "utils/general.h"
#include "scene/scene.h"
#include "editor/editor_pass.h"

static move_tool_t move_tool;

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
  int z_obj_id = create_object(t, OBJECT_FLAGS::SELECTABLE | OBJECT_FLAGS::EDITOR_OBJ);
  attach_name_to_obj(z_obj_id, std::string("z arrow"));
  attach_child_obj_to_obj(move_tool_parent_obj_id, z_obj_id);
  attach_model_to_obj(z_obj_id, move_tool.arrow_id);

  editor_mat.color = {0,0,1};
  attach_editor_mat_to_obj(z_obj_id, editor_mat);

  t.rot = create_quaternion_w_rot({1,0,0}, 90.f);
  int x_obj_id = create_object(t, OBJECT_FLAGS::SELECTABLE | OBJECT_FLAGS::EDITOR_OBJ);
  attach_name_to_obj(x_obj_id, std::string("x arrow"));
  attach_child_obj_to_obj(move_tool_parent_obj_id, x_obj_id);
  attach_model_to_obj(x_obj_id, move_tool.arrow_id);

  editor_mat.color = {1,0,0};
  attach_editor_mat_to_obj(x_obj_id, editor_mat);

  t.rot = create_quaternion_w_rot({0,0,1}, 90.f);
  int y_obj_id = create_object(t, OBJECT_FLAGS::SELECTABLE | OBJECT_FLAGS::EDITOR_OBJ);
  attach_name_to_obj(y_obj_id, std::string("y arrow"));
  attach_child_obj_to_obj(move_tool_parent_obj_id, y_obj_id);
  attach_model_to_obj(y_obj_id, move_tool.arrow_id);

  editor_mat.color = {0,1,0};
  attach_editor_mat_to_obj(y_obj_id, editor_mat);
}
