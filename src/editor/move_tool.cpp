#include "move_tool.h"

#include "geometry/gltf/gltf.h"
#include "utils/general.h"

static move_tool_t move_tool;

void init_move_tool() {
  char resources_path[256]{};
  get_resources_folder_path(resources_path);

  char gltf_full_file_path[256]{};
  sprintf(gltf_full_file_path, "%s\\editor\\arrow.gltf", resources_path);
  gltf_load_file(gltf_full_file_path, false);

  move_tool.arrow_id = latest_model_id();
}

void render_move_tool() {

}
