#include "move_tool.h"

#include "geometry/gltf/gltf.h"
#include "utils/general.h"
#include "scene/scene.h"

static move_tool_t move_tool;

void init_move_tool() {
  char resources_path[256]{};
  get_resources_folder_path(resources_path);

  char gltf_full_file_path[256]{};
  sprintf(gltf_full_file_path, "%s\\editor\\arrow.gltf", resources_path);
  gltf_load_file(gltf_full_file_path, false);

  move_tool.arrow_id = latest_model_id();

  int move_tool_parent_obj_id = create_object(move_tool.transform, OBJECT_FLAGS::EDITOR_OBJ);
  attach_name_to_obj(move_tool_parent_obj_id, std::string("move tool"));
  set_obj_as_parent(move_tool_parent_obj_id);

  transform_t t{};
  int z_obj_id = create_object(t, OBJECT_FLAGS::SELECTABLE | OBJECT_FLAGS::EDITOR_OBJ);
  attach_name_to_obj(z_obj_id, std::string("z arrow"));
  attach_child_obj_to_obj(move_tool_parent_obj_id, z_obj_id);

  attach_model_to_obj(z_obj_id, move_tool.arrow_id);
}
