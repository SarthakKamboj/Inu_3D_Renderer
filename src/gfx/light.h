#pragma once

#include "gfx/gfx.h"
#include "utils/transform.h"
#include "utils/mats.h"
#include "utils/vectors.h"
#include "model_loading/model_internal.h"

#define CHANGING_REF_TO_FIT 0
#define USE_GLM 0

#define SHOW_LIGHTS 1
#define NUM_SM_CASCADES 3
#define HAVE_DIR_LIGHT 1
#define NUM_CUBE_CORNERS 8

#define RENDER_DIR_LIGHT_FRUSTUMS 0

#define RENDER_DIR_LIGHT_ORTHOS 0
// define which ortho projection we want to view
#define LIGHT_ORTHO_CASCADE_TO_VIEW 0

#define DISPLAY_DIR_LIGHT_SHADOW_MAPS 0

#define USE_DIR_LIGHT_DEBUG_FBOS 0

#define BTR 0
#define FTR 1
#define BBR 2
#define FBR 3
#define BTL 4
#define FTL 5
#define BBL 6
#define FBL 7

// will hopefully use shadow volumes at some pt
struct spotlight_t {
  int id = -1;
  transform_t transform;
  float radius = 1.f;
  static const float NEAR_PLANE;
  static const float FAR_PLANE;
  static const float SHADOW_MAP_WIDTH;
  static const float SHADOW_MAP_HEIGHT;
  vec3 dir;
  vec3 color;

#if SHOW_LIGHTS
  static int LIGHT_MESH_ID;
#endif
  static shader_t light_shader;
  framebuffer_t light_pass_fb;

  mat4 view;
  mat4 proj;
};

void init_light_data();
int create_spotlight(vec3 pos);
void setup_spotlight_for_rendering(int light_id);
void remove_spotlight_from_rendering();
spotlight_t get_spotlight(int light_id);
int get_num_spotlights();
tex_id_t get_spotlight_fb_depth_tex(int light_id);
mat4 get_spotlight_proj_mat(int light_id);
mat4 get_spotlight_view_mat(int light_id);
vec3 get_spotlight_pos(int light_id);

struct dir_light_shadow_map_vert_t {
  vec2 pos;
  vec2 tex;
};

struct frustum_t {
  vec3 frustum_corners[NUM_CUBE_CORNERS]{};
};

// will use cascading shadow maps
struct dir_light_t {
  int id = -1;
  static const float SHADOW_MAP_WIDTH;
  static const float SHADOW_MAP_HEIGHT;
  vec3 dir;

  static shader_t light_shader;

#if USE_DIR_LIGHT_DEBUG_FBOS
  static shader_t debug_shader;
  framebuffer_t debug_light_pass_fbs[NUM_SM_CASCADES];
#endif

#if DISPLAY_DIR_LIGHT_SHADOW_MAPS
  static shader_t display_shadow_map_shader;

  vao_t display_shadow_map_vao;
  vbo_t display_shadow_map_vbo;
  ebo_t display_shadow_map_ebo;
#endif

#if RENDER_DIR_LIGHT_FRUSTUMS
  int debug_frustum_obj_ids[NUM_SM_CASCADES];
#endif

#if RENDER_DIR_LIGHT_ORTHOS
  int debug_ortho_obj_ids[NUM_SM_CASCADES];
#endif

  framebuffer_t light_pass_fb;

  mat4 light_views[NUM_SM_CASCADES];
  mat4 light_orthos[NUM_SM_CASCADES];
  float cacade_depths[NUM_SM_CASCADES+1];
};

int create_dir_light(vec3 dir);
struct camera_t;
void setup_dir_light_for_rendering(int light_id, camera_t* camera);
void remove_dir_light_from_rendering();

void setup_dir_light_for_rendering_debug(int light_id, camera_t* camera, int cascade);
void remove_dir_light_from_rendering_debug();

#if DISPLAY_DIR_LIGHT_SHADOW_MAPS
void create_dir_light_shadow_map_img_buffers(dir_light_t& light);
void render_dir_light_shadow_maps(int dir_light_id);
#endif

int get_num_dir_lights();
dir_light_t* get_dir_light(int id);
void gen_dir_light_matricies(int light_id, camera_t* camera);

#if (RENDER_DIR_LIGHT_FRUSTUMS || RENDER_DIR_LIGHT_ORTHOS)
void create_frustum_and_ortho_models(dir_light_t& light);

#if RENDER_DIR_LIGHT_ORTHOS
void update_dir_light_ortho_models(dir_light_t& dir_light, int cascade, float x_min, float x_max, float y_min, float y_max, float z_min, float z_max);
#endif

#if RENDER_DIR_LIGHT_FRUSTUMS
void update_dir_light_frustum_models(dir_light_t& dir_light, int cascade, vertex_t* vertices);
#endif

#endif
