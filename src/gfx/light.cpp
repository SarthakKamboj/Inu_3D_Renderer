#include "light.h"

#include "gfx/gfx.h"
#include "windowing/window.h"
#include "utils/general.h"
#include "scene/scene.h"
#include "scene/camera.h"
#include "model_loading/gltf/gltf.h"
#include "scene/scene.h"

#include <vector>

extern window_t window;

static std::vector<light_t> lights;

#if SHOW_LIGHTS
int light_t::LIGHT_MESH_ID = -1;
#endif

shader_t light_t::light_shader;

extern float fb_width;
extern float fb_height;

const float light_t::NEAR_PLANE = 0.1f;
const float light_t::FAR_PLANE = 50.f;
const float light_t::SHADOW_MAP_WIDTH = fb_width / 4.f;
const float light_t::SHADOW_MAP_HEIGHT = fb_height / 4.f;

void init_light_data() {
  char resources_path[256]{};
  get_resources_folder_path(resources_path);
  char vert_shader_path[256]{};
  sprintf(vert_shader_path, "%s\\shaders\\light.vert", resources_path);
  char frag_shader_path[256]{};
  sprintf(frag_shader_path, "%s\\shaders\\light.frag", resources_path);
  light_t::light_shader = create_shader(vert_shader_path, frag_shader_path);

#if SHOW_LIGHTS
  char light_mesh_full_file_path[256]{};
  // this file pretty much just has a mesh, no nodes
  sprintf(light_mesh_full_file_path, "%s\\custom_light\\light_mesh.gltf", resources_path);
  gltf_load_file(light_mesh_full_file_path);
  light_t::LIGHT_MESH_ID = latest_model_id();
#endif

}

int create_light(vec3 pos) {
  if (lights.size() >= NUM_LIGHTS_SUPPORTED_IN_SHADER) {
    char buffer[256]{};
    sprintf(buffer, "can't support more than %i", NUM_LIGHTS_SUPPORTED_IN_SHADER);
    inu_assert_msg(buffer);
  }
  light_t light;
  light.id = lights.size();
  light.transform.pos = pos;
  light.dir = {0,-1,0};
  light.light_pass_fb = create_framebuffer(light_t::SHADOW_MAP_WIDTH, light_t::SHADOW_MAP_HEIGHT, true);
  lights.push_back(light);
  transform_t obj_t;
  obj_t.pos = pos;
  obj_t.scale = {1,1,1};
  int obj_id = create_object(obj_t);
  attach_name_to_obj(obj_id, std::string("light pos"));
  set_obj_as_parent(obj_id);
#if SHOW_LIGHTS
  attach_model_to_obj(obj_id, light_t::LIGHT_MESH_ID);
#endif

  return light.id;
}

void set_lights_in_shader() {
  light_t& light = lights[0];
  // shader_set_vec3(light_t::light_shader, "light.pos", light.transform.pos);
}

light_t get_light(int light_id) {
  return lights[light_id];
}

int get_num_lights() {
  return lights.size();
}

void setup_light_for_rendering(int light_id) {
  light_t& light = lights[light_id];

  bind_framebuffer(light.light_pass_fb);
  clear_framebuffer(light.light_pass_fb);

  vec3 fp = {light.transform.pos.x, light.transform.pos.y - 1, light.transform.pos.z};
  // fp.x = ;
  light.view = get_view_mat(light.transform.pos, fp);
  // float light_near_plane = 0.1f;
  // float light_far_plane = 50.f;
  light.proj = proj_mat(60.f, light_t::NEAR_PLANE, light_t::FAR_PLANE, static_cast<float>(window.window_dim.x) / window.window_dim.y);
  shader_set_mat4(light_t::light_shader, "light_view", light.view);
  shader_set_mat4(light_t::light_shader, "light_projection", light.proj); 

  bind_shader(light_t::light_shader);
}

void remove_light_from_rendering() {
  unbind_shader();
  unbind_framebuffer();
}

GLuint get_light_fb_depth_tex(int light_id) {
  return lights[light_id].light_pass_fb.depth_att;
}

mat4 get_light_proj_mat(int light_id) {
  return lights[light_id].proj;
}

mat4 get_light_view_mat(int light_id) {
  return lights[light_id].view;
}

vec3 get_light_pos(int light_id) {
  return lights[light_id].transform.pos;
}

