#include "spotlight.h"

#include <vector>

#include "scene/scene.h"
#include "scene/camera.h"
#include "utils/general.h"
#include "utils/log.h"
#include "windowing/window.h"
#include "model_loading/gltf/gltf.h"

extern window_t window;

static std::vector<spotlight_t> spotlights;

#if SHOW_LIGHTS
int spotlight_t::LIGHT_MESH_ID = -1;
#endif

shader_t spotlight_t::light_shader;
const float spotlight_t::NEAR_PLANE = 0.1f;
const float spotlight_t::FAR_PLANE = 50.f;

const float spotlight_t::SHADOW_MAP_WIDTH = 1280.0f / 4.f;
const float spotlight_t::SHADOW_MAP_HEIGHT = 1280.0f / 4.f;


void init_spotlight_data() {
  char resources_path[256]{};
  get_resources_folder_path(resources_path);
  char vert_shader_path[256]{};
  sprintf(vert_shader_path, "%s\\shaders\\light.vert", resources_path);
  char frag_shader_path[256]{};
  sprintf(frag_shader_path, "%s\\shaders\\light.frag", resources_path);
  spotlight_t::light_shader = create_shader(vert_shader_path, frag_shader_path);

#if SHOW_LIGHTS
  char light_mesh_full_file_path[256]{};
  // this file pretty much just has a mesh, no nodes
  sprintf(light_mesh_full_file_path, "%s\\models\\custom_light\\light_mesh.gltf", resources_path);
  gltf_load_file(light_mesh_full_file_path);
  spotlight_t::LIGHT_MESH_ID = latest_model_id();
#endif


}

int create_spotlight(vec3 pos) {
  if (spotlights.size() >= NUM_LIGHTS_SUPPORTED_IN_SHADER) {
    char buffer[256]{};
    sprintf(buffer, "can't support more than %i", NUM_LIGHTS_SUPPORTED_IN_SHADER);
    inu_assert_msg(buffer);
  }
  spotlight_t light;
  light.id = spotlights.size();
  light.transform.pos = pos;
  light.dir = {0,-1,0};
  light.light_pass_fb = create_framebuffer(spotlight_t::SHADOW_MAP_WIDTH, spotlight_t::SHADOW_MAP_HEIGHT, FB_TYPE::TEXTURE_DEPTH_STENCIL);
  spotlights.push_back(light);
  transform_t obj_t;
  obj_t.pos = pos;
  obj_t.scale = {1,1,1};
  int obj_id = create_object(obj_t);
  attach_name_to_obj(obj_id, std::string("light pos"));
  set_obj_as_parent(obj_id);
#if SHOW_LIGHTS
  attach_model_to_obj(obj_id, spotlight_t::LIGHT_MESH_ID);
#endif

  return light.id;
}

spotlight_t get_spotlight(int light_id) {
  return spotlights[light_id];
}

int get_num_spotlights() {
  return spotlights.size();
}

void setup_spotlight_for_rendering(int light_id) {
  spotlight_t& light = spotlights[light_id];

  bind_framebuffer(light.light_pass_fb);
  clear_framebuffer();

  vec3 fp = {light.transform.pos.x, light.transform.pos.y - 1, light.transform.pos.z};
  light.view = get_view_mat(light.transform.pos, fp);
  light.proj = proj_mat(60.f, spotlight_t::NEAR_PLANE, spotlight_t::FAR_PLANE, static_cast<float>(window.window_dim.x) / window.window_dim.y);
  shader_set_mat4(spotlight_t::light_shader, "light_view", light.view);
  shader_set_mat4(spotlight_t::light_shader, "light_projection", light.proj); 

  bind_shader(spotlight_t::light_shader);
}

void remove_spotlight_from_rendering() {
  unbind_shader();
  unbind_framebuffer();
}

tex_id_t get_spotlight_fb_depth_tex(int light_id) {
  return spotlights[light_id].light_pass_fb.depth_att;
}

mat4 get_spotlight_proj_mat(int light_id) {
  return spotlights[light_id].proj;
}

mat4 get_spotlight_view_mat(int light_id) {
  return spotlights[light_id].view;
}

vec3 get_spotlight_pos(int light_id) {
  return spotlights[light_id].transform.pos;
}
