#include "move_tool.h"

#include "geometry/gltf/gltf.h"
#include "utils/general.h"
#include "scene/scene.h"
#include "editor/editor_pass.h"
#include "windowing/window.h"
#include "pixel_perfect_sel.h"
#include "geometry/model.h"

extern window_t window;

static move_tool_t move_tool;
static axes_model_t axis_model;

bool currently_selecting() {
  return move_tool.selected_x || move_tool.selected_y || move_tool.selected_z;
}

int create_axis_model(vec3 pos1, vec3 pos2, std::string& name) {
  model_t axis_model; 

  mesh_t mesh; 

  vertex_t v1;
  v1.position = pos1;

  vertex_t v2;
  v2.position = pos2;

  mesh.vertices.push_back(v1);
  mesh.vertices.push_back(v2);

  mesh.indicies.push_back(0);
  mesh.indicies.push_back(1);

  mesh.mesh_draw_mode = MESH_DRAW_MODE::LINE;

  mesh.vao = create_vao();
  mesh.vbo = create_vbo((void*)mesh.vertices.data(), mesh.vertices.size() * sizeof(vertex_t));
  mesh.ebo = create_ebo(mesh.indicies.data(), mesh.indicies.size() * sizeof(unsigned int));

  vao_enable_attribute(mesh.vao, mesh.vbo, 0, 3, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, position));
  vao_enable_attribute(mesh.vao, mesh.vbo, 1, 2, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex0));
  vao_enable_attribute(mesh.vao, mesh.vbo, 2, 2, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex1));
  vao_enable_attribute(mesh.vao, mesh.vbo, 3, 3, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
  vao_enable_attribute(mesh.vao, mesh.vbo, 4, 4, VAO_ATTR_DATA_TYPE::UNSIGNED_INT, sizeof(vertex_t), offsetof(vertex_t, joints));
  vao_enable_attribute(mesh.vao, mesh.vbo, 5, 4, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, weights));
  vao_enable_attribute(mesh.vao, mesh.vbo, 6, 3, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, normal));

  vao_bind_ebo(mesh.vao, mesh.ebo);
  axis_model.meshes.push_back(mesh);

  int model_id = register_model(axis_model);

  transform_t t;
  t.scale = { 1,1,1 };

  int obj_id = create_object(t, OBJECT_FLAGS::EDITOR_OBJ);
  attach_name_to_obj(obj_id, name);
  attach_model_to_obj(obj_id, model_id);
  attach_child_obj_to_obj(move_tool.parent_obj, obj_id);

  model_toggle_hidden(obj_id, true);

  return obj_id;
}

void create_axes() {
  axis_model.x_axis_obj_id = create_axis_model({-100,0,0}, {100,0,0}, std::string("x axis"));
  axis_model.y_axis_obj_id = create_axis_model({0,-100,0}, {0,100,0}, std::string("y axis"));
  axis_model.z_axis_obj_id = create_axis_model({0,0,-100}, {0,0,100}, std::string("z axis"));

  editor_mat_t editor_mat; 

  editor_mat.color = {1,0,0};
  attach_editor_mat_to_obj(axis_model.x_axis_obj_id, editor_mat);

  editor_mat.color = {0,1,0};
  attach_editor_mat_to_obj(axis_model.y_axis_obj_id, editor_mat);

  editor_mat.color = {0,0,1};
  attach_editor_mat_to_obj(axis_model.z_axis_obj_id, editor_mat);
}

void create_arrows() {
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

void init_move_tool() {
  create_arrows();
  create_axes();
}

void update_move_tool() {

  if (window.input.left_mouse_up) {
    hide_axes();
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
        model_toggle_hidden(axis_model.x_axis_obj_id, false);
        printf("x selected\n");
      } else if (is_obj_selected_left_mouse_down(move_tool.y_obj_id)) {
        move_tool.selected_y = true;  
        model_toggle_hidden(axis_model.y_axis_obj_id, false);
        printf("y selected\n");
      } else if (is_obj_selected_left_mouse_down(move_tool.z_obj_id)) {
        move_tool.selected_z = true;  
        model_toggle_hidden(axis_model.z_axis_obj_id, false);
        printf("z selected\n");
      }
    } else { 
      model_toggle_hidden(move_tool.x_obj_id, false);
      hide_axes();
    }
  }

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

void hide_axes() {
  model_toggle_hidden(axis_model.x_axis_obj_id, true);
  model_toggle_hidden(axis_model.y_axis_obj_id, true);
  model_toggle_hidden(axis_model.z_axis_obj_id, true);
}

void set_move_tool_as_position(vec3 pos) {
  move_tool.transform.pos = pos;
  set_transform_on_obj(move_tool.parent_obj, move_tool.transform);
}

void set_move_tool_as_transform(transform_t& t) {
  move_tool.transform = t;
  set_transform_on_obj(move_tool.parent_obj, move_tool.transform);
}
