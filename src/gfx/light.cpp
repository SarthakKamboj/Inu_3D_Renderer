#include "light.h"

#include "gfx_api/gfx.h"
#include "windowing/window.h"
#include "utils/general.h"
#include "scene/scene.h"
#include "scene/camera.h"
#include "model_loading/gltf/gltf.h"
#include "scene/scene.h"
#include "animation/interpolation.h"
#include "windowing/window.h"
#include "utils/mats.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <math.h>
#include <algorithm>
#include <vector>

extern window_t window;

bool update_dir_light_frustums = true;

static std::vector<spotlight_t> spotlights;
static std::vector<dir_light_t> dir_lights;

#if SHOW_LIGHTS
int spotlight_t::LIGHT_MESH_ID = -1;
#endif

extern float fb_width;
extern float fb_height;

shader_t spotlight_t::light_shader;
const float spotlight_t::NEAR_PLANE = 0.1f;
const float spotlight_t::FAR_PLANE = 50.f;

// const float spotlight_t::SHADOW_MAP_WIDTH = fb_width / 4.f;
// const float spotlight_t::SHADOW_MAP_HEIGHT = fb_height / 4.f;
const float spotlight_t::SHADOW_MAP_WIDTH = 1280.0f / 4.f;
const float spotlight_t::SHADOW_MAP_HEIGHT = 1280.0f / 4.f;

const float dir_light_t::SHADOW_MAP_WIDTH = 2048.f;
const float dir_light_t::SHADOW_MAP_HEIGHT = 2048.f;

shader_t dir_light_t::light_shader;

#if USE_DIR_LIGHT_DEBUG_FBOS
shader_t dir_light_t::debug_shader;
#endif

#if DISPLAY_DIR_LIGHT_SHADOW_MAPS
shader_t dir_light_t::display_shadow_map_shader;
#endif

bool render_dir_orthos = true;

void init_light_data() {
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

  char geom_shader_path[256]{};

  memset(vert_shader_path, 0, sizeof(vert_shader_path));
  memset(geom_shader_path, 0, sizeof(geom_shader_path));
  memset(frag_shader_path, 0, sizeof(frag_shader_path));
  sprintf(vert_shader_path, "%s\\shaders\\dir_light.vert", resources_path);
  sprintf(geom_shader_path, "%s\\shaders\\dir_light.geo", resources_path);
  sprintf(frag_shader_path, "%s\\shaders\\dir_light.frag", resources_path);
  dir_light_t::light_shader = create_shader(vert_shader_path, geom_shader_path, frag_shader_path);

#if USE_DIR_LIGHT_DEBUG_FBOS
  memset(vert_shader_path, 0, sizeof(vert_shader_path));
  memset(frag_shader_path, 0, sizeof(frag_shader_path));
  sprintf(vert_shader_path, "%s\\shaders\\light.vert", resources_path);
  sprintf(frag_shader_path, "%s\\shaders\\light.frag", resources_path);
  dir_light_t::debug_shader = create_shader(vert_shader_path, frag_shader_path);
#endif

#if DISPLAY_DIR_LIGHT_SHADOW_MAPS
  memset(vert_shader_path, 0, sizeof(vert_shader_path));
  memset(frag_shader_path, 0, sizeof(frag_shader_path));
  sprintf(vert_shader_path, "%s\\shaders\\dir_shadow_maps.vert", resources_path);
  sprintf(frag_shader_path, "%s\\shaders\\dir_shadow_maps.frag", resources_path);
  dir_light_t::display_shadow_map_shader = create_shader(vert_shader_path, frag_shader_path);
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

#if (RENDER_DIR_LIGHT_FRUSTUMS || RENDER_DIR_LIGHT_ORTHOS)
void create_frustum_and_ortho_models(dir_light_t& light) {
  vec4 colors[NUM_SM_CASCADES];
  colors[0] = {1,0,0,1};
  colors[1] = {0,1,0,1};
  colors[2] = {0,0,1,1};

  material_image_t def_mat_image;

  int materials[NUM_SM_CASCADES];

  metallic_roughness_param_t met_rough;

  albedo_param_t albedo;

  albedo.base_color = colors[0];
  materials[0] = create_material(albedo, met_rough);

  albedo.base_color = colors[1];
  materials[1] = create_material(albedo, met_rough);

  albedo.base_color = colors[2];
  materials[2] = create_material(albedo, met_rough);

  // debug_i = 0 is related to dir light frustums and their objects
  // debug_i = 1 is related to dir light orthos and their objects
  const int FRUSTUM_MODEL_CREATION_ITER = 0;
  const int ORTHO_PROJ_MODEL_CREATION_ITER = 1;
  for (int debug_i = 0; debug_i <= ORTHO_PROJ_MODEL_CREATION_ITER; debug_i++) {
    for (int cascade = 0; cascade < NUM_SM_CASCADES; cascade++) {
      model_t debug_model;

      vertex_t vertices[NUM_CUBE_CORNERS];
      for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
        vertex_t& v = vertices[i];
        v.tex0 = {0,0};
        v.tex1 = {0,0};
        v.normal = {0,0,0};
        v.color = {1,0,0};

        v.joints[0] = 0;
        v.joints[1] = 0;
        v.joints[2] = 0;
        v.joints[3] = 0;

        v.weights[0] = 0;
        v.weights[1] = 0;
        v.weights[2] = 0;
        v.weights[3] = 0;
      }

      vertices[BTR].position = {1,1,1};
      vertices[FTR].position = {1,1,-1};
      vertices[BBR].position = {1,-1,1};
      vertices[FBR].position = {1,-1,-1};
      vertices[BTL].position = {-1,1,1};
      vertices[FTL].position = {-1,1,-1};
      vertices[BBL].position = {-1,-1,1};
      vertices[FBL].position = {-1,-1,-1};

      unsigned int indicies[36] = {
        // top face
        BTR, FTL, FTR,
        BTR, BTL, FTL,

        // right face
        FTR, BBR, BTR,
        FTR, FBR, BBR,

        // back face
        BTR, BBR, BBL,
        BTR, BBL, BTL,

        // left face
        FTL, BBL, FBL,
        FTL, BTL, BBL,

        // bottom face
        FBL, BBL, BBR,
        FBL, BBR, FBR,

        // front face
        FTL, FBL, FBR,
        FTL, FBR, FTR
      };

      mesh_t mesh;
      mesh.mat_idx = materials[cascade];
      mesh.vao = create_vao();
      mesh.vbo = create_dyn_vbo(sizeof(vertices));
      update_vbo_data(mesh.vbo, vertices, sizeof(vertices));
      mesh.ebo = create_ebo(indicies, sizeof(indicies));

      vao_enable_attribute(mesh.vao, mesh.vbo, 0, 3, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, position));
      vao_enable_attribute(mesh.vao, mesh.vbo, 1, 2, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex0));
      vao_enable_attribute(mesh.vao, mesh.vbo, 2, 2, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex1));
      vao_enable_attribute(mesh.vao, mesh.vbo, 3, 3, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
      vao_enable_attribute(mesh.vao, mesh.vbo, 4, 4, VAO_ATTR_DATA_TYPE::UNSIGNED_INT, sizeof(vertex_t), offsetof(vertex_t, joints));
      vao_enable_attribute(mesh.vao, mesh.vbo, 5, 4, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, weights));
      vao_enable_attribute(mesh.vao, mesh.vbo, 6, 3, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(vertex_t), offsetof(vertex_t, normal));
      vao_bind_ebo(mesh.vao, mesh.ebo);
      
      debug_model.meshes.push_back(mesh);
      int model_id = register_model(debug_model);

      transform_t t;
      t.scale = {1,1,1};
      int obj_id = create_object(t);
      attach_model_to_obj(obj_id, model_id);

      if (debug_i == FRUSTUM_MODEL_CREATION_ITER) {
#if RENDER_DIR_LIGHT_FRUSTUMS
        attach_name_to_obj(obj_id, std::string("frustum_" + std::to_string(cascade)) );
        light.debug_frustum_obj_ids[cascade] = obj_id;
        set_obj_as_parent(obj_id);
#endif
      } else if (debug_i == ORTHO_PROJ_MODEL_CREATION_ITER) {
#if RENDER_DIR_LIGHT_ORTHOS
        attach_name_to_obj(obj_id, std::string("light_ortho_" + std::to_string(cascade)) );
        light.debug_ortho_obj_ids[cascade] = obj_id;
        if (cascade != LIGHT_ORTHO_CASCADE_TO_VIEW) continue;
        set_obj_as_parent(obj_id);
#endif
      }

    }
  }
}
#endif

int create_dir_light(vec3 dir) {
  dir_light_t light;
  light.dir = normalize(dir);
  light.id = dir_lights.size();
  light.light_pass_fb = create_framebuffer(dir_light_t::SHADOW_MAP_WIDTH, dir_light_t::SHADOW_MAP_HEIGHT, FB_TYPE::NO_COLOR_ATT_MULTIPLE_DEPTH_TEXTURE); 

#if USE_DIR_LIGHT_DEBUG_FBOS
  light.debug_light_pass_fbs[0] = create_framebuffer(dir_light_t::SHADOW_MAP_WIDTH, dir_light_t::SHADOW_MAP_HEIGHT, FB_TYPE::TEXTURE_DEPTH_STENCIL);
  light.debug_light_pass_fbs[1] = create_framebuffer(dir_light_t::SHADOW_MAP_WIDTH, dir_light_t::SHADOW_MAP_HEIGHT, FB_TYPE::TEXTURE_DEPTH_STENCIL);
  light.debug_light_pass_fbs[2] = create_framebuffer(dir_light_t::SHADOW_MAP_WIDTH, dir_light_t::SHADOW_MAP_HEIGHT, FB_TYPE::TEXTURE_DEPTH_STENCIL);
#endif

#if (RENDER_DIR_LIGHT_FRUSTUMS || RENDER_DIR_LIGHT_ORTHOS)
  create_frustum_and_ortho_models(light);
#endif

#if DISPLAY_DIR_LIGHT_SHADOW_MAPS
  create_dir_light_shadow_map_img_buffers(light);
#endif

  dir_lights.push_back(light);
  return light.id;
}

int get_num_dir_lights() {
  return dir_lights.size();
}

#if RENDER_DIR_LIGHT_ORTHOS
void update_dir_light_ortho_models(dir_light_t& dir_light, int cascade, float x_min, float x_max, float y_min, float y_max, float z_min, float z_max) {
  frustum_t light_ortho_world_frustum;
  light_ortho_world_frustum.frustum_corners[BTR] = {x_max,y_max,z_min};
  light_ortho_world_frustum.frustum_corners[FTR] = {x_max,y_max,z_max};
  light_ortho_world_frustum.frustum_corners[BBR] = {x_max,y_min,z_min};
  light_ortho_world_frustum.frustum_corners[FBR] = {x_max,y_min,z_max};
  light_ortho_world_frustum.frustum_corners[BTL] = {x_min,y_max,z_min};
  light_ortho_world_frustum.frustum_corners[FTL] = {x_min,y_max,z_max};
  light_ortho_world_frustum.frustum_corners[BBL] = {x_min,y_min,z_min};
  light_ortho_world_frustum.frustum_corners[FBL] = {x_min,y_min,z_max};

  mat4 inverse_light_view = dir_light.light_views[cascade].inverse();
  for (int corner = 0; corner < NUM_CUBE_CORNERS; corner++) {
    vec4 corner4(light_ortho_world_frustum.frustum_corners[corner], 1);
    vec4 world_corner = inverse_light_view * corner4;
    world_corner = world_corner / world_corner.w;
    vec3 wc3 = {world_corner.x, world_corner.y, world_corner.z};
    light_ortho_world_frustum.frustum_corners[corner] = wc3;
  }

  int ortho_frustum_obj_id = dir_light.debug_ortho_obj_ids[cascade];
  vbo_t* vbo = get_obj_vbo(ortho_frustum_obj_id, 0);

  vertex_t ortho_vertices[NUM_CUBE_CORNERS];
  for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
    vertex_t& v = ortho_vertices[i];

    v.position = light_ortho_world_frustum.frustum_corners[i];

    v.tex0 = {0,0};
    v.tex1 = {0,0};
    v.normal = {0,0,0};
    v.color = {1,0,0};

    v.joints[0] = 0;
    v.joints[1] = 0;
    v.joints[2] = 0;
    v.joints[3] = 0;

    v.weights[0] = 0;
    v.weights[1] = 0;
    v.weights[2] = 0;
    v.weights[3] = 0;
  }

  update_vbo_data(*vbo, ortho_vertices, sizeof(ortho_vertices));
}
#endif

#if RENDER_DIR_LIGHT_FRUSTUMS
void update_dir_light_frustum_models(dir_light_t& dir_light, int cascade, vertex_t* vertices) {
  int frustum_obj_id = dir_light.debug_frustum_obj_ids[cascade];
  vbo_t* vbo = get_obj_vbo(frustum_obj_id, 0);

  for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
    vertex_t& v = vertices[i];
    v.tex0 = {0,0};
    v.tex1 = {0,0};
    v.normal = {0,0,0};
    v.color = {1,0,0};

    v.joints[0] = 0;
    v.joints[1] = 0;
    v.joints[2] = 0;
    v.joints[3] = 0;

    v.weights[0] = 0;
    v.weights[1] = 0;
    v.weights[2] = 0;
    v.weights[3] = 0;
  }

  update_vbo_data(*vbo, vertices, NUM_CUBE_CORNERS * sizeof(vertex_t));
}
#endif

void gen_dir_light_matricies(int light_id, camera_t* camera) {
  dir_light_t& dir_light = dir_lights[light_id];

#if (RENDER_DIR_LIGHT_FRUSTUMS || RENDER_DIR_LIGHT_ORTHOS)
  if (window.input.right_mouse_up) {
    update_dir_light_frustums = !update_dir_light_frustums;
  }

  if (window.input.left_mouse_up) {
    render_dir_orthos = !render_dir_orthos;
    dir_light_t* dir = get_dir_light(0);
    for (int i = 0; i < NUM_SM_CASCADES; i++) {
#if RENDER_DIR_LIGHT_ORTHOS
      if (render_dir_orthos && i == LIGHT_ORTHO_CASCADE_TO_VIEW) {
        set_obj_as_parent(dir_light.debug_ortho_obj_ids[i]);
      } else {
        unset_obj_as_parent(dir_light.debug_ortho_obj_ids[i]);
      }
#endif

#if RENDER_DIR_LIGHT_FRUSTUMS
      if (render_dir_orthos) {
        set_obj_as_parent(dir_light.debug_frustum_obj_ids[i]);
      } else {
        unset_obj_as_parent(dir_light.debug_frustum_obj_ids[i]);
      }
#endif
    }
  }
#endif

  // calculate camera frustum in world coordinates
  frustum_t cam_frustum_ndc_corners = {
    {
      // near bottom left
      {-1,-1,-1},
      // far bottom left
      {-1,-1,1},
      // near top left
      {-1,1,-1},
      // far top left 
      {-1,1,1},
      // near bottom right
      {1,-1,-1},
      // far bottom right
      {1,-1,1},
      // near top right
      {1,1,-1},
      // far top right
      {1,1,1}
    }
  };

  static mat4 last_cam_view(1.0f);
  static mat4 last_cam_proj(1.0f);
  mat4 cam_view;
  mat4 cam_proj;
  if (update_dir_light_frustums) {
    cam_view = get_cam_view_mat();
    last_cam_view = cam_view;

    cam_proj = get_cam_proj_mat();
    last_cam_proj = cam_proj;
  } else {
    cam_view = last_cam_view;
    cam_proj = last_cam_proj;
  }


  frustum_t world_cam_frustum;
  for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
    // this does change every frame but the actual frustum should remain the same length always
    mat4 c = cam_proj * cam_view;
    vec4 corner = vec4(cam_frustum_ndc_corners.frustum_corners[i], 1.0f);
    vec4 world_unnorm = c.inverse() * corner;
    vec4 world_norm = world_unnorm / world_unnorm.w;
    world_cam_frustum.frustum_corners[i] = {world_norm.x, world_norm.y, world_norm.z};
  }

  // split camera frustum in cascades
  const int N = NUM_SM_CASCADES;
  const float N_f = static_cast<float>(N);
  frustum_t world_cascaded_frustums[N];

  for (int cascade = 0; cascade < NUM_SM_CASCADES; cascade++) {
    frustum_t& world_cascade_frustum = world_cascaded_frustums[cascade];

    static float last_n = -1;
    static float last_f = -1;

    float n, f;
    if (update_dir_light_frustums) {
      n = camera->near_plane;
      f = camera->far_plane;
      last_n = n;
      last_f = f;
    } else {
      n = last_n;
      f = last_f;
    }

    float z_near = (0.5f*n*pow(f/n, cascade/N_f)) + (0.5f*(n+(cascade/N_f*(f-n))));
    float z_far = (0.5f*n*pow(f/n, (cascade+1)/N_f)) + (0.5f*(n+((cascade+1)/N_f*(f-n))));

    dir_light.cacade_depths[cascade] = z_near;
    dir_light.cacade_depths[cascade+1] = z_far;

    float inter_n = (z_near - n) / (f - n);
    float inter_f = (z_far - n) / (f - n);
   
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < NUM_CUBE_CORNERS / 2; j++) {
        int fc_near_idx = j*2;
        int fc_far_idx = fc_near_idx + 1;
        world_cascade_frustum.frustum_corners[fc_near_idx] = vec3_linear(world_cam_frustum.frustum_corners[fc_near_idx], world_cam_frustum.frustum_corners[fc_far_idx], inter_n);
        world_cascade_frustum.frustum_corners[fc_far_idx] = vec3_linear(world_cam_frustum.frustum_corners[fc_near_idx], world_cam_frustum.frustum_corners[fc_far_idx], inter_f);
      }
    } 

    vertex_t vertices[NUM_CUBE_CORNERS];

    vertices[BTR].position = world_cascade_frustum.frustum_corners[7];
    vertices[FTR].position = world_cascade_frustum.frustum_corners[6];
    vertices[BBR].position = world_cascade_frustum.frustum_corners[5];
    vertices[FBR].position = world_cascade_frustum.frustum_corners[4];
    vertices[BTL].position = world_cascade_frustum.frustum_corners[3];
    vertices[FTL].position = world_cascade_frustum.frustum_corners[2];
    vertices[BBL].position = world_cascade_frustum.frustum_corners[1];
    vertices[FBL].position = world_cascade_frustum.frustum_corners[0];

    static float cascade_lens[NUM_SM_CASCADES]{};
    static bool calculated_diags = false;

    float cascade_len = 0;
    if (!calculated_diags) {
      vec3 longest_diag1 = vertices[FBR].position - vertices[BTL].position;
      vec3 longest_diag2 = vertices[BBR].position - vertices[BTL].position;
      float cascade_len1 = longest_diag1.length();
      float cascade_len2 = longest_diag2.length();
      cascade_len = fmax(cascade_len1, cascade_len2);
      cascade_lens[cascade] = cascade_len;
      if (cascade == NUM_SM_CASCADES-1) {
        calculated_diags = true;
      }
    } else {
      cascade_len = cascade_lens[cascade];
    } 
 
    // get center
    vec3 frustum_center{};
    vec3 focal_pt = dir_light.dir + frustum_center;
    // calc view mat
    dir_light.light_views[cascade] = get_view_mat(frustum_center, focal_pt); 

    float world_len_per_texel = cascade_len / dir_light_t::SHADOW_MAP_WIDTH;

    // get mins and maxs
    float x_min = FLT_MAX, x_max = -FLT_MAX;
    float y_min = FLT_MAX, y_max = -FLT_MAX;
    float z_min = FLT_MAX, z_max = -FLT_MAX;
    for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
      vec4 pt = {world_cascade_frustum.frustum_corners[i].x, world_cascade_frustum.frustum_corners[i].y, world_cascade_frustum.frustum_corners[i].z, 1.0f};
      vec4 light_rel_view_pt = dir_light.light_views[cascade] * pt;
      x_min = fmin(x_min, light_rel_view_pt.x);
      x_max = fmax(x_max, light_rel_view_pt.x);
      y_min = fmin(y_min, light_rel_view_pt.y);
      y_max = fmax(y_max, light_rel_view_pt.y);
      z_min = fmin(z_min, light_rel_view_pt.z);
      z_max = fmax(z_max, light_rel_view_pt.z);
    }

    float x_mid = (x_min + x_max) / 2.f;
    float y_mid = (y_min + y_max) / 2.f;

    x_min = x_mid - (cascade_len / 2.f);
    x_max = x_mid + (cascade_len / 2.f);

    y_min = y_mid - (cascade_len / 2.f);
    y_max = y_mid + (cascade_len / 2.f);

#define TESTING 1
    // will need to calculate z_min and z_max accordingly to scene bounds
#if TESTING
    z_max = 50.f;
    z_min = -50.f;
#else
    float z_multiplier = 10.f;
    if (z_max < 0) {
      z_max = z_max / z_multiplier;
    } else {
      z_max = z_max * z_multiplier;
    }

    if (z_min < 0) {
      z_min = z_min * z_multiplier;
    } else {
      z_min = z_min / z_multiplier;
    } 
#endif
#undef TESTING

    // pixel snapping
    x_min = round(x_min / world_len_per_texel) * world_len_per_texel;
    x_max = round(x_max / world_len_per_texel) * world_len_per_texel;
    y_min = round(y_min / world_len_per_texel) * world_len_per_texel;
    y_max = round(y_max / world_len_per_texel) * world_len_per_texel;
   
    // calc ortho mat
    dir_light.light_orthos[cascade] = ortho_mat(x_min, x_max, y_min, y_max, -z_max, -z_min);

    // debug view for dir light frustums
#if RENDER_DIR_LIGHT_FRUSTUMS
    update_dir_light_frustum_models(dir_light, cascade, vertices);
#endif

    // debug view for dir light orthos
#if RENDER_DIR_LIGHT_ORTHOS
    update_dir_light_ortho_models(dir_light, cascade, x_min, x_max, y_min, y_max, z_min, z_max);
#endif
  }
}

void setup_dir_light_for_rendering(int light_id, camera_t* camera) {
  dir_light_t& dir_light = dir_lights[light_id];

  bind_framebuffer(dir_light.light_pass_fb);
  clear_framebuffer();
  
  // set up shader uniforms
  for (int i = 0; i < NUM_SM_CASCADES; i++) {
    char var_name[256]{};
    sprintf(var_name, "light_views[%i]", i);
    shader_set_mat4(dir_light_t::light_shader, var_name, dir_light.light_views[i]);

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "light_projs[%i]", i);
    shader_set_mat4(dir_light_t::light_shader, var_name, dir_light.light_orthos[i]);
  }
  bind_shader(dir_light_t::light_shader);
}

void remove_dir_light_from_rendering() {
  unbind_shader();
  unbind_framebuffer();
}

#if USE_DIR_LIGHT_DEBUG_FBOS
void setup_dir_light_for_rendering_debug(int light_id, camera_t* camera, int cascade) {
  dir_light_t& dir_light = dir_lights[light_id];

  bind_framebuffer(dir_light.debug_light_pass_fbs[cascade]);
  clear_framebuffer();

  gen_dir_light_matricies(light_id, camera);

  // set up shader uniforms
  shader_set_mat4(dir_light_t::debug_shader, "light_view", dir_light.light_views[cascade]);
  shader_set_mat4(dir_light_t::debug_shader, "light_projection", dir_light.light_orthos[cascade]);
  bind_shader(dir_light_t::debug_shader);
}

void remove_dir_light_from_rendering_debug() {
  unbind_shader();
  unbind_framebuffer();
}
#endif

#if DISPLAY_DIR_LIGHT_SHADOW_MAPS
void create_dir_light_shadow_map_img_buffers(dir_light_t& light) {
  light.display_shadow_map_vao = create_vao();
  light.display_shadow_map_vbo = create_dyn_vbo(sizeof(dir_light_shadow_map_vert_t) * 4);
  unsigned int indicies[6] {
    0, 2, 1,
    0, 3, 2
  };
  light.display_shadow_map_ebo = create_ebo(indicies, sizeof(indicies));

  vao_enable_attribute(light.display_shadow_map_vao, light.display_shadow_map_vbo, 0, 2, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(dir_light_shadow_map_vert_t), offsetof(dir_light_shadow_map_vert_t, pos));
  vao_enable_attribute(light.display_shadow_map_vao, light.display_shadow_map_vbo, 1, 2, VAO_ATTR_DATA_TYPE::FLOAT, sizeof(dir_light_shadow_map_vert_t), offsetof(dir_light_shadow_map_vert_t, tex));
  vao_bind_ebo(light.display_shadow_map_vao, light.display_shadow_map_ebo);
}

void render_dir_light_shadow_maps(int dir_light_id) {
  clear_framebuffer_depth();
  dir_light_t& dir_light = dir_lights[dir_light_id];

  mat4 o = ortho_mat(0, window.window_dim.x, 0, window.window_dim.y, 10, -10);
  shader_set_mat4(dir_light_t::display_shadow_map_shader, "projection", o);
  shader_set_int(dir_light_t::display_shadow_map_shader, "shadow_map", 0);

  float display_dim = 200.f;
  float gap = 40.f;

  for (int i = 0; i < NUM_SM_CASCADES; i++) {
    shader_set_int(dir_light_t::display_shadow_map_shader, "cascade", i);
    vec2 top_left = {(display_dim + gap)*i, 700};

    dir_light_shadow_map_vert_t verts[4]{};
    verts[0].pos = top_left;
    verts[0].tex = {0,1};

    verts[1].pos = {top_left.x + display_dim, top_left.y};
    verts[1].tex = {1,1};

    verts[2].pos = {top_left.x + display_dim, top_left.y - display_dim};
    verts[2].tex = {1,0};

    verts[3].pos = {top_left.x, top_left.y - display_dim};
    verts[3].tex = {0,0};

    update_vbo_data(dir_light.display_shadow_map_vbo, verts, sizeof(verts));

    bind_texture(dir_light.light_pass_fb.depth_att, 0);

    bind_shader(dir_light_t::display_shadow_map_shader);
    bind_vao(dir_light.display_shadow_map_vao);
    draw_ebo(dir_light.display_shadow_map_ebo);
    unbind_vao();
    unbind_ebo();
  }

}
#endif

dir_light_t* get_dir_light(int id) {
  return &dir_lights[id];
}
