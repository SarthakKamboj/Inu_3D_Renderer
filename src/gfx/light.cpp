#include "light.h"

#include "gfx/gfx.h"
#include "windowing/window.h"
#include "utils/general.h"
#include "scene/scene.h"
#include "scene/camera.h"
#include "model_loading/gltf/gltf.h"
#include "scene/scene.h"
#include "animation/interpolation.h"
#include "windowing/window.h"

#include <vector>

extern window_t window;

static std::vector<light_t> lights;
static std::vector<dir_light_t> dir_lights;

#if SHOW_LIGHTS
int light_t::LIGHT_MESH_ID = -1;
#endif

extern float fb_width;
extern float fb_height;

shader_t light_t::light_shader;
const float light_t::NEAR_PLANE = 0.1f;
const float light_t::FAR_PLANE = 50.f;
const float light_t::SHADOW_MAP_WIDTH = fb_width / 4.f;
const float light_t::SHADOW_MAP_HEIGHT = fb_height / 4.f;

#if 0
const float dir_light_t::SHADOW_MAP_WIDTH = 4096.f;
const float dir_light_t::SHADOW_MAP_HEIGHT = 4096.f;
#else
const float dir_light_t::SHADOW_MAP_WIDTH = 1024.f;
const float dir_light_t::SHADOW_MAP_HEIGHT = 1024.f;
#endif
shader_t dir_light_t::light_shader;
shader_t dir_light_t::debug_shader;
shader_t dir_light_t::display_shadow_map_shader;

extern bool render_dir_orthos;

#if 1
bool make_orthos_square = true;
bool make_orthos_square_consistent = true;
bool make_orthos_texel_snapped = true;
#else
bool make_orthos_square = false;
bool make_orthos_square_consistent = false;
bool make_orthos_texel_snapped = false;
#endif

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

  char geom_shader_path[256]{};

  memset(vert_shader_path, 0, sizeof(vert_shader_path));
  memset(geom_shader_path, 0, sizeof(geom_shader_path));
  memset(frag_shader_path, 0, sizeof(frag_shader_path));
  sprintf(vert_shader_path, "%s\\shaders\\dir_light.vert", resources_path);
  sprintf(geom_shader_path, "%s\\shaders\\dir_light.geo", resources_path);
  sprintf(frag_shader_path, "%s\\shaders\\dir_light.frag", resources_path);
  dir_light_t::light_shader = create_shader(vert_shader_path, geom_shader_path, frag_shader_path);

  memset(vert_shader_path, 0, sizeof(vert_shader_path));
  memset(frag_shader_path, 0, sizeof(frag_shader_path));
  sprintf(vert_shader_path, "%s\\shaders\\light.vert", resources_path);
  sprintf(frag_shader_path, "%s\\shaders\\light.frag", resources_path);
  dir_light_t::debug_shader = create_shader(vert_shader_path, frag_shader_path);

  memset(vert_shader_path, 0, sizeof(vert_shader_path));
  memset(frag_shader_path, 0, sizeof(frag_shader_path));
  sprintf(vert_shader_path, "%s\\shaders\\dir_shadow_maps.vert", resources_path);
  sprintf(frag_shader_path, "%s\\shaders\\dir_shadow_maps.frag", resources_path);
  dir_light_t::display_shadow_map_shader = create_shader(vert_shader_path, frag_shader_path);
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
  light.light_pass_fb = create_framebuffer(light_t::SHADOW_MAP_WIDTH, light_t::SHADOW_MAP_HEIGHT, FB_TYPE::TEXTURE_DEPTH_STENCIL);
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
  light.view = get_view_mat(light.transform.pos, fp);
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

int create_dir_light(vec3 dir) {
  dir_light_t light;
  light.dir = norm_vec3(dir);
  light.id = dir_lights.size();
  light.debug_light_pass_fbs[0] = create_framebuffer(dir_light_t::SHADOW_MAP_WIDTH, dir_light_t::SHADOW_MAP_HEIGHT, FB_TYPE::TEXTURE_DEPTH_STENCIL);
  light.debug_light_pass_fbs[1] = create_framebuffer(dir_light_t::SHADOW_MAP_WIDTH, dir_light_t::SHADOW_MAP_HEIGHT, FB_TYPE::TEXTURE_DEPTH_STENCIL);
  light.debug_light_pass_fbs[2] = create_framebuffer(dir_light_t::SHADOW_MAP_WIDTH, dir_light_t::SHADOW_MAP_HEIGHT, FB_TYPE::TEXTURE_DEPTH_STENCIL);
  light.light_pass_fb = create_framebuffer(dir_light_t::SHADOW_MAP_WIDTH, dir_light_t::SHADOW_MAP_HEIGHT, FB_TYPE::NO_COLOR_ATT_MULTIPLE_DEPTH_TEXTURE); 

#if (RENDER_DIR_LIGHT_FRUSTUMS || RENDER_DIR_LIGHT_ORTHOS)
  vec4 colors[NUM_SM_CASCADES];
  colors[0] = {1,0,0,1};
  colors[1] = {0,1,0,1};
  colors[2] = {0,0,1,1};

  material_image_t def_mat_image;

  int materials[NUM_SM_CASCADES];
  materials[0] = create_material(colors[0], def_mat_image);
  materials[1] = create_material(colors[1], def_mat_image);
  materials[2] = create_material(colors[2], def_mat_image);

  // for (int debug_i = 0; debug_i < RENDER_DIR_LIGHT_ORTHOS + RENDER_DIR_LIGHT_FRUSTUMS; debug_i++) {
  for (int debug_i = 0; debug_i < 2; debug_i++) {
    for (int cascade = 0; cascade < NUM_SM_CASCADES; cascade++) {
      model_t frustum_model;

      vertex_t vertices[8];
      for (int i = 0; i < 8; i++) {
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

      vao_enable_attribute(mesh.vao, mesh.vbo, 0, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, position));
      vao_enable_attribute(mesh.vao, mesh.vbo, 1, 2, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex0));
      vao_enable_attribute(mesh.vao, mesh.vbo, 2, 2, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex1));
      vao_enable_attribute(mesh.vao, mesh.vbo, 3, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
      vao_enable_attribute(mesh.vao, mesh.vbo, 4, 4, GL_UNSIGNED_INT, sizeof(vertex_t), offsetof(vertex_t, joints));
      vao_enable_attribute(mesh.vao, mesh.vbo, 5, 4, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, weights));
      vao_enable_attribute(mesh.vao, mesh.vbo, 6, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, normal));
      vao_bind_ebo(mesh.vao, mesh.ebo);
      
      frustum_model.meshes.push_back(mesh);
      int model_id = register_model(frustum_model);

      transform_t t;
      t.scale = {1,1,1};
      int obj_id = create_object(t);
      attach_model_to_obj(obj_id, model_id);

      if (debug_i == 0) {
#if RENDER_DIR_LIGHT_FRUSTUMS
        attach_name_to_obj(obj_id, std::string("frustum_" + std::to_string(cascade)) );
        light.debug_obj_ids[cascade] = obj_id;
        set_obj_as_parent(obj_id);
#endif
      } else {
#if RENDER_DIR_LIGHT_ORTHOS
        attach_name_to_obj(obj_id, std::string("light_ortho_" + std::to_string(cascade)) );
        light.debug_ortho_obj_ids[cascade] = obj_id;
        if (cascade != LIGHT_ORTHO_CASCADE_TO_VIEW) continue;
        set_obj_as_parent(obj_id);
#endif
      }

    }
  }
#endif

  light.display_shadow_map_vao = create_vao();
  light.display_shadow_map_vbo = create_dyn_vbo(sizeof(dir_light_shadow_map_vert_t) * 4);
  unsigned int indicies[6] {
    0, 2, 1,
    0, 3, 2
  };
  light.display_shadow_map_ebo = create_ebo(indicies, sizeof(indicies));

  vao_enable_attribute(light.display_shadow_map_vao, light.display_shadow_map_vbo, 0, 2, GL_FLOAT, sizeof(dir_light_shadow_map_vert_t), offsetof(dir_light_shadow_map_vert_t, pos));
  vao_enable_attribute(light.display_shadow_map_vao, light.display_shadow_map_vbo, 1, 2, GL_FLOAT, sizeof(dir_light_shadow_map_vert_t), offsetof(dir_light_shadow_map_vert_t, tex));
  vao_bind_ebo(light.display_shadow_map_vao, light.display_shadow_map_ebo);

  dir_lights.push_back(light);
  return light.id;
}

struct frustum_t {
  vec3 frustum_corners[NUM_CUBE_CORNERS]{};
};

extern bool update_dir_light_frustums;
void gen_dir_light_matricies(int light_id, camera_t* camera) {
  dir_light_t& dir_light = dir_lights[light_id];

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

  frustum_t test_frustum;
  static mat4 last_cam_view = create_matrix(1.0f);
  mat4 cam_view;
  if (update_dir_light_frustums) {
    cam_view = get_cam_view_mat();
    last_cam_view = cam_view;
  } else {
    cam_view = last_cam_view;
  }
  // mat4 cam_view ;
  mat4 cam_proj = get_cam_proj_mat();

#if 0
  for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
    // this does change every frame but the actual frustum should remain the same length always
    vec4 corner = vec4(cam_frustum_ndc_corners.frustum_corners[i], 1.0f);
    vec4 world_unnorm = mat_multiply_vec(mat4_inverse(cam_proj), corner);
    vec4 world_norm = world_unnorm / world_unnorm.w;
    test_frustum.frustum_corners[i] = {world_norm.x, world_norm.y, world_norm.z};
  }
  printf("test frustum edge length: %f\n", length(test_frustum.frustum_corners[5] - test_frustum.frustum_corners[4]));
#endif

#if 0
  for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
    // this does change every frame but the actual frustum should remain the same length always
    vec4 corner = vec4(cam_frustum_ndc_corners.frustum_corners[i], 1.0f);
    vec4 world_unnorm = mat_multiply_vec(mat4_inverse(cam_view), corner);
    vec4 world_norm = world_unnorm / world_unnorm.w;
    test_frustum.frustum_corners[i] = {world_norm.x, world_norm.y, world_norm.z};
  }
  printf("test frustum edge length: %f\n", length(test_frustum.frustum_corners[7] - test_frustum.frustum_corners[6]));
#endif

  frustum_t world_cam_frustum;
  for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
    // this does change every frame but the actual frustum should remain the same length always
    mat4 c = mat_multiply_mat(cam_proj, cam_view);
    vec4 corner = vec4(cam_frustum_ndc_corners.frustum_corners[i], 1.0f);
    vec4 world_unnorm = mat_multiply_vec(mat4_inverse(c), corner);
    vec4 world_norm = world_unnorm / world_unnorm.w;
    world_cam_frustum.frustum_corners[i] = {world_norm.x, world_norm.y, world_norm.z};
  }

  // seems like frustum edge is not consistent...could be a problem with the projection or view matricies
  // printf("frustum edge length: %f\n", length(world_cam_frustum.frustum_corners[7] - world_cam_frustum.frustum_corners[6]));

  // split camera frustum in cascades
  const int N = NUM_SM_CASCADES;
  const float N_f = static_cast<float>(N);
  frustum_t world_cascaded_frustums[N];

  const float frustum_percent_splits[NUM_SM_CASCADES+1] = {0, 0.15f, 0.4f, 1.0f};
  for (int cascade = 0; cascade < NUM_SM_CASCADES; cascade++) {
    frustum_t& world_cascade_frustum = world_cascaded_frustums[cascade];

    float n = camera->near_plane;
    float f = camera->far_plane;

#if 1
    float z_near = (0.5f*n*pow(f/n, cascade/N_f)) + (0.5f*(n+(cascade/N_f*(f-n))));
    float z_far = (0.5f*n*pow(f/n, (cascade+1)/N_f)) + (0.5f*(n+((cascade+1)/N_f*(f-n))));

    dir_light.cacade_depths[cascade] = z_near;
    dir_light.cacade_depths[cascade+1] = z_far;

    float inter_n = (z_near - n) / (f - n);
    float inter_f = (z_far - n) / (f - n);
#else
    float inter_n = frustum_percent_splits[cascade];
    float inter_f = frustum_percent_splits[cascade+1];

    float z_near = linear(n, f, inter_n);
    float z_far = linear(n, f, inter_f);

    dir_light.cacade_depths[cascade] = z_near;
    dir_light.cacade_depths[cascade+1] = z_far;
#endif
    
    // float zs[2] = {z_near, z_far};
    for (int i = 0; i < 2; i++) {

#if 0
      if (cascade == 0) {
        printf("i: %i inter_n: %f inter_f: %f || ", i, inter_n, inter_f);
        if (i == 1) {
          printf("\n");
        }
      }
#endif

      for (int j = 0; j < NUM_CUBE_CORNERS / 2; j++) {
        int fc_near_idx = j*2;
        int fc_far_idx = fc_near_idx + 1;
        world_cascade_frustum.frustum_corners[fc_near_idx] = vec3_linear(world_cam_frustum.frustum_corners[fc_near_idx], world_cam_frustum.frustum_corners[fc_far_idx], inter_n);
        world_cascade_frustum.frustum_corners[fc_far_idx] = vec3_linear(world_cam_frustum.frustum_corners[fc_near_idx], world_cam_frustum.frustum_corners[fc_far_idx], inter_f);
        
#if 0
        vec3 c = world_cascade_frustum.frustum_corners[fc_far_idx] - world_cascade_frustum.frustum_corners[fc_near_idx];
        // this changes frame to frame...but why?
        float total_len = length(world_cam_frustum.frustum_corners[fc_far_idx] - world_cam_frustum.frustum_corners[fc_near_idx]);

        float percent_of_full_frustum_len = length(c) / total_len;
        
        if (cascade == 0) {
          printf("percent: %f\n", percent_of_full_frustum_len);
          printf("cam frustum edge total_len: %f\n", total_len);
        }
#endif

      }
      
    } 

    vertex_t vertices[8];

    vertices[BTR].position = world_cascade_frustum.frustum_corners[7];
    vertices[FTR].position = world_cascade_frustum.frustum_corners[6];
    vertices[BBR].position = world_cascade_frustum.frustum_corners[5];
    vertices[FBR].position = world_cascade_frustum.frustum_corners[4];
    vertices[BTL].position = world_cascade_frustum.frustum_corners[3];
    vertices[FTL].position = world_cascade_frustum.frustum_corners[2];
    vertices[BBL].position = world_cascade_frustum.frustum_corners[1];
    vertices[FBL].position = world_cascade_frustum.frustum_corners[0];

#if RENDER_DIR_LIGHT_FRUSTUMS
    int frustum_obj_id = dir_light.debug_obj_ids[cascade];
    vbo_t* vbo = get_obj_vbo(frustum_obj_id, 0);

    for (int i = 0; i < 8; i++) {
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

    update_vbo_data(*vbo, vertices, sizeof(vertices));
#endif

#if 0
  }

  // calculate view and ortho mats for each cascaded frustum
  for (int cascade = 0; cascade < NUM_SM_CASCADES; cascade++) {
    frustum_t& world_cascade_frustum = cascaded_frustums[cascade];
#endif

#if 0
    float cascade_len = 0;
    for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
      for (int j = i+1; j < NUM_CUBE_CORNERS; j++) {
        vec3 diff = world_cascade_frustum.frustum_corners[i] - world_cascade_frustum.frustum_corners[j];
        float diff_len = length(diff);
        if (diff_len > cascade_len) {
          cascade_len = diff_len;
          // printf("pos of i: %i and pos of j: %i\n", 7-i, 7-j);
        }
      }  
    }

    if (cascade == 0) {
      // printf("cascade_len: %f\n", cascade_len);
    }

#else

    static float cascade_lens[NUM_SM_CASCADES]{};
    static bool calculated_diags = false;

    float cascade_len = 0;
    if (!calculated_diags) {
      vec3 longest_diag1 = vertices[FBR].position - vertices[BTL].position;
      vec3 longest_diag2 = vertices[BBR].position - vertices[BTL].position;
      float cascade_len1 = length(longest_diag1);
      float cascade_len2 = length(longest_diag2);
      cascade_len = fmax(cascade_len1, cascade_len2);
      cascade_len = cascade_len1;
      cascade_lens[cascade] = cascade_len;
      if (cascade == NUM_SM_CASCADES-1) {
        calculated_diags = true;
      }
    } else {
      cascade_len = cascade_lens[cascade];
    } 
    // inu_assert(cascade_len2 == cascade_len);
#endif

    if (cascade == 0) {
      printf("cascade_len: %f\n", cascade_len);
    } 

#if 0
    if (make_orthos_texel_snapped) {
      for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
        vec3& corner = world_cascade_frustum.frustum_corners[i];
        vec3 orig_corner = corner;
#if 0
        float bucket = 0;
        bucket = round(corner.x / world_len_per_texel);
        corner.x = bucket * world_len_per_texel;
        bucket = round(corner.y / world_len_per_texel);
        corner.y = bucket * world_len_per_texel;
        bucket = round(corner.z / world_len_per_texel);
        corner.z = bucket * world_len_per_texel;
#else
        corner.x = round(corner.x / world_len_per_texel) * world_len_per_texel;
        corner.y = round(corner.y / world_len_per_texel) * world_len_per_texel;
        corner.z = round(corner.z / world_len_per_texel) * world_len_per_texel;
#endif
      }
    }
#endif

    // get center
    vec3 frustum_center{};
    for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
      frustum_center.x += world_cascade_frustum.frustum_corners[i].x;
      frustum_center.y += world_cascade_frustum.frustum_corners[i].y;
      frustum_center.z += world_cascade_frustum.frustum_corners[i].z;
    }
    frustum_center.x /= static_cast<float>(NUM_CUBE_CORNERS);
    frustum_center.y /= static_cast<float>(NUM_CUBE_CORNERS);
    frustum_center.z /= static_cast<float>(NUM_CUBE_CORNERS);

#if 0
    frustum_center.x = round(frustum_center.x / world_len_per_texel) * world_len_per_texel;
    frustum_center.y = round(frustum_center.y / world_len_per_texel) * world_len_per_texel;
    frustum_center.z = round(frustum_center.z / world_len_per_texel) * world_len_per_texel;
#endif

#if 0
    // float world_len_per_texel = cascade_len / dir_light_t::SHADOW_MAP_WIDTH;
    float bucket = 0;
    bucket = round(frustum_center.x / world_len_per_texel);
    frustum_center.x = bucket * world_len_per_texel;
    bucket = round(frustum_center.y / world_len_per_texel);
    frustum_center.y = bucket * world_len_per_texel;
    bucket = round(frustum_center.z / world_len_per_texel);
    frustum_center.z = bucket * world_len_per_texel;
#endif

    // calc view mat
    dir_light.light_views[cascade] = get_view_mat(frustum_center, vec3_add(frustum_center, dir_light.dir));

    float proj_dim = 0;
    if (make_orthos_square_consistent) {
        proj_dim = cascade_len;
#if 0
      static float proj_dims[NUM_SM_CASCADES] = {0,0,0};
      static bool calc_proj_dim = false;

      vec4 cascade_test_pt = {cascade_len,0,0,1}; 
      vec4 light_projected_cascaded_test_pt = mat_multiply_vec(dir_light.light_views[cascade], cascade_test_pt);
      if (!calc_proj_dim) {
        proj_dim = vec4_length(light_projected_cascaded_test_pt);
        proj_dim = cascade_len;
        proj_dims[cascade] = proj_dim;
        if (cascade == NUM_SM_CASCADES-1) {
          calc_proj_dim = true;
        }
      } else {
        proj_dim = proj_dims[cascade];
      }
      if (cascade == 0) {
        // should not be changing
        printf("cascade_len: %f proj_dim: %f\n", cascade_len, proj_dim);
      }
#endif
    }

    // vec4 p1 = {proj_dim,0,0,1};

    // float world_len_per_texel = proj_dim / dir_light_t::SHADOW_MAP_WIDTH;

    mat4 light_space_to_world_space = mat4_inverse(dir_light.light_views[cascade]);
    vec4 cascade_test_pt1 = {proj_dim, 0,0,1};
    vec4 cascade_test_pt2 = {0, 0,0,1};

    vec4 new_ct1 = mat_multiply_vec(light_space_to_world_space, cascade_test_pt1);
    vec4 new_ct2 = mat_multiply_vec(light_space_to_world_space, cascade_test_pt2);

    new_ct1 = new_ct1 / new_ct1.w;
    new_ct2 = new_ct2 / new_ct2.w;

    vec3 d = {new_ct1.x - new_ct2.x, new_ct1.y - new_ct2.y, new_ct1.z - new_ct2.z};
    float world_units = length(d);
    
    float world_len_per_texel = world_units / dir_light_t::SHADOW_MAP_WIDTH;

    // get mins and maxs
    float x_min = FLT_MAX, x_max = -FLT_MAX;
    float y_min = FLT_MAX, y_max = -FLT_MAX;
    float z_min = FLT_MAX, z_max = -FLT_MAX;
    for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
      vec4 pt = {world_cascade_frustum.frustum_corners[i].x, world_cascade_frustum.frustum_corners[i].y, world_cascade_frustum.frustum_corners[i].z, 1.0f};
      vec4 light_rel_view_pt = mat_multiply_vec(dir_light.light_views[cascade], pt);
      x_min = fmin(x_min, light_rel_view_pt.x);
      x_max = fmax(x_max, light_rel_view_pt.x);
      y_min = fmin(y_min, light_rel_view_pt.y);
      y_max = fmax(y_max, light_rel_view_pt.y);
      z_min = fmin(z_min, light_rel_view_pt.z);
      z_max = fmax(z_max, light_rel_view_pt.z);
    }

#if 1
    float x_mid = (x_min + x_max) / 2.f;
    float y_mid = (y_min + y_max) / 2.f;
#else
    // projected frustum center will be at (0,0,0)
    float x_mid = 0;
    float y_mid = 0;
#endif

#if 1

#if 0
    float proj_dim = fmax(abs(x_max - x_min), abs(y_max - y_min));
#elif 0
    vec4 cascade_test_pt = {cascade_len, 0,0,0};
    vec4 light_projected_cascaded_test_pt = mat_multiply_vec(dir_light.light_views[cascade], cascade_test_pt);
    float proj_dim = vec4_length(light_projected_cascaded_test_pt);

    printf("proj dim: %f\n", proj_dim);
#else

    // float proj_dim = 0;
    if (make_orthos_square) {
      // proj_dim = fmax(abs(x_max - x_min), abs(y_max - y_min)); 
    }

#if 0
    if (make_orthos_square_consistent) {
      static float proj_dims[NUM_SM_CASCADES] = {0,0,0};
      static bool calc_proj_dim = false;

      vec4 cascade_test_pt = {cascade_len, 0,0,0};
      if (cascade == 0) {
        // should not be changing
        // printf("cascade_len_diag: %f\n", cascade_len_diag);
      }
      vec4 light_projected_cascaded_test_pt = mat_multiply_vec(dir_light.light_views[cascade], cascade_test_pt);
      proj_dim = 0;
      if (!calc_proj_dim) {
        proj_dim = vec4_length(light_projected_cascaded_test_pt);
        proj_dims[cascade] = proj_dim;
        if (cascade == NUM_SM_CASCADES-1) {
          calc_proj_dim = true;
        }
      } else {
        proj_dim = proj_dims[cascade];
      }
    }
#endif

#endif

#if 0
    if (cascade == 0) {
      printf("proj dim: %f\n", proj_dim);
    }
#endif

    if (make_orthos_square || make_orthos_square_consistent) {
      x_min = x_mid - (proj_dim / 2.f);
      x_max = x_mid + (proj_dim / 2.f);

      y_min = y_mid - (proj_dim / 2.f);
      y_max = y_mid + (proj_dim / 2.f);
      
#endif
    }

    // z_min and z_max will likely both be neative since we are looking down the negative z axis
    float z_multiplier = 4.f;
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

#if 1
    // float world_len_per_texel = cascade_len / dir_light_t::SHADOW_MAP_WIDTH;
    // float world_len_per_texel = cascade_len / dir_light_t::SHADOW_MAP_WIDTH;

    if (make_orthos_texel_snapped) {
      float c;
      float rc;

      c = x_min / world_len_per_texel;
      rc = round(c);
      x_min = rc * world_len_per_texel;

      c = x_max / world_len_per_texel;
      rc = round(c);
      x_max = rc * world_len_per_texel;

      c = y_min / world_len_per_texel;
      rc = round(c);
      y_min = rc * world_len_per_texel;

      c = y_max / world_len_per_texel;
      rc = round(c);
      y_max = rc * world_len_per_texel;
      // z_min = round(z_min / world_len_per_texel) * world_len_per_texel;
      // z_max = round(z_max / world_len_per_texel) * world_len_per_texel;
    }

#endif

#if 0
    // float world_len_per_texel = cascade_len_diag / dir_light_t::SHADOW_MAP_WIDTH;
    float xs[2] = {x_min, x_max};
    float ys[2] = {y_min, y_max};
    float zs[2] = {z_min, z_max};

    vec3 new_world_cascade_pts[8]{};
    int new_i = 0;

    mat4 light_space_to_world_space = mat4_inverse(dir_light.light_views[cascade]);
    for (int x_i = 0; x_i < 2; x_i++) {
      for (int y_i = 0; y_i < 2; y_i++) {
        for (int z_i = 0; z_i < 2; z_i++) {
          vec4 pt = {xs[x_i], ys[y_i], zs[z_i], 1.0f};
          vec4 new_world_pt = mat_multiply_vec(light_space_to_world_space, pt);
          new_world_pt = new_world_pt / new_world_pt.w; 
          new_world_pt.x = round(new_world_pt.x / world_len_per_texel) * world_len_per_texel;
          new_world_pt.y = round(new_world_pt.y / world_len_per_texel) * world_len_per_texel;
          new_world_pt.z = round(new_world_pt.z / world_len_per_texel) * world_len_per_texel;
          new_world_cascade_pts[new_i] = {new_world_pt.x, new_world_pt.y, new_world_pt.z};
          new_i++;
        }
      }
    }

    float o_x_min = x_min, o_x_max = x_max;
    float o_y_min = y_min, o_y_max = y_max;
    float o_z_min = z_min, o_z_max = z_max;

    // get mins and maxs
    x_min = FLT_MAX, x_max = -FLT_MAX;
    y_min = FLT_MAX, y_max = -FLT_MAX;
    z_min = FLT_MAX, z_max = -FLT_MAX;
    for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
      vec4 pt = {new_world_cascade_pts[i].x, new_world_cascade_pts[i].y, new_world_cascade_pts[i].z, 1.0f};
      vec4 light_rel_view_pt = mat_multiply_vec(dir_light.light_views[cascade], pt);
      x_min = min(x_min, light_rel_view_pt.x);
      x_max = max(x_max, light_rel_view_pt.x);
      y_min = min(y_min, light_rel_view_pt.y);
      y_max = max(y_max, light_rel_view_pt.y);
      z_min = min(z_min, light_rel_view_pt.z);
      z_max = max(z_max, light_rel_view_pt.z);
    }

#endif

    if (cascade == 0) {
      // printf("%f %f %f %f %f %f\n", x_min, x_max, y_min, y_max, z_min, z_max);
    }

    // calc ortho mat
#if 1
    dir_light.light_orthos[cascade] = ortho_mat(x_min, x_max, y_min, y_max, z_min, z_max);
#else
    mat4 ortho1 = create_matrix(1.0f);
    ortho1.first_col.x = 2.0f / (x_max - x_min);
    ortho1.second_col.y = 2.0f / (y_max - y_min);
    ortho1.third_col.z = -2.0f / (z_max - z_min);
    ortho1.fourth_col = {
      -(x_max + x_min) / (x_max - x_min),
      -(y_max + y_min) / (y_max - y_min),
      -(z_max + z_min) / (z_max - z_min),
      1.0f
    };

    mat4 ortho2 = create_matrix(1.0f);
    ortho2.third_col.z = 0.5f;
    ortho2.fourth_col.z = 0.5f;

    dir_light.light_orthos[cascade] = mat_multiply_mat(ortho2, ortho1);
		
#endif

#if RENDER_DIR_LIGHT_ORTHOS
    // debug view for light orthos

    frustum_t light_ortho_world_frustum;
    light_ortho_world_frustum.frustum_corners[BTR] = {x_max,y_max,z_min};
    light_ortho_world_frustum.frustum_corners[FTR] = {x_max,y_max,z_max};
    light_ortho_world_frustum.frustum_corners[BBR] = {x_max,y_min,z_min};
    light_ortho_world_frustum.frustum_corners[FBR] = {x_max,y_min,z_max};
    light_ortho_world_frustum.frustum_corners[BTL] = {x_min,y_max,z_min};
    light_ortho_world_frustum.frustum_corners[FTL] = {x_min,y_max,z_max};
    light_ortho_world_frustum.frustum_corners[BBL] = {x_min,y_min,z_min};
    light_ortho_world_frustum.frustum_corners[FBL] = {x_min,y_min,z_max};

    mat4 inverse_light_view = mat4_inverse(dir_light.light_views[cascade]);
    for (int corner = 0; corner < NUM_CUBE_CORNERS; corner++) {
      vec4 corner4(light_ortho_world_frustum.frustum_corners[corner], 1);
      vec4 world_corner = mat_multiply_vec(inverse_light_view, corner4);
      world_corner = world_corner / world_corner.w;
      vec3 wc3 = {world_corner.x, world_corner.y, world_corner.z};
      light_ortho_world_frustum.frustum_corners[corner] = wc3;
    }

    int ortho_frustum_obj_id = dir_light.debug_ortho_obj_ids[cascade];
    vbo_t* vbo = get_obj_vbo(ortho_frustum_obj_id, 0);

    vertex_t ortho_vertices[8];
    for (int i = 0; i < 8; i++) {
      vertex_t& v = ortho_vertices[i];
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
 
    ortho_vertices[BTR].position = light_ortho_world_frustum.frustum_corners[BTR];
    ortho_vertices[FTR].position = light_ortho_world_frustum.frustum_corners[FTR];
    ortho_vertices[BBR].position = light_ortho_world_frustum.frustum_corners[BBR];
    ortho_vertices[FBR].position = light_ortho_world_frustum.frustum_corners[FBR];
    ortho_vertices[BTL].position = light_ortho_world_frustum.frustum_corners[BTL];
    ortho_vertices[FTL].position = light_ortho_world_frustum.frustum_corners[FTL];
    ortho_vertices[BBL].position = light_ortho_world_frustum.frustum_corners[BBL];
    ortho_vertices[FBL].position = light_ortho_world_frustum.frustum_corners[FBL];

    update_vbo_data(*vbo, ortho_vertices, sizeof(ortho_vertices)); 
#endif
  }
}

void setup_dir_light_for_rendering(int light_id, camera_t* camera) {
  dir_light_t& dir_light = dir_lights[light_id];

  bind_framebuffer(dir_light.light_pass_fb);
  clear_framebuffer(dir_light.light_pass_fb);

  
  // gen_dir_light_matricies(light_id, camera);

#if 0
  vec3 fp = {0,0,0};
  light.view = get_view_mat(light.transform.pos, fp);
  shader_set_mat4(light_t::light_shader, "light_view", light.view);
  shader_set_mat4(light_t::light_shader, "light_projection", light.proj); 
#endif

  // set up shader uniforms
  for (int i = 0; i < NUM_SM_CASCADES; i++) {

#if 0
    mat4 light_space_mat = mat_multiply_mat(dir_light.light_orthos[i], dir_light.light_views[i]);
    char var_name[256]{};
    sprintf(var_name, "lightSpaceMatrices[%i]", i);
    shader_set_mat4(dir_light_t::light_shader, var_name, dir_light.light_views[i]);
#else

    char var_name[256]{};
    sprintf(var_name, "light_views[%i]", i);
    shader_set_mat4(dir_light_t::light_shader, var_name, dir_light.light_views[i]);

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "light_projs[%i]", i);
    shader_set_mat4(dir_light_t::light_shader, var_name, dir_light.light_orthos[i]);
#endif
  }
  bind_shader(dir_light_t::light_shader);
}

void remove_dir_light_from_rendering() {
  unbind_shader();
  unbind_framebuffer();
}

void setup_dir_light_for_rendering_debug(int light_id, camera_t* camera, int cascade) {
  dir_light_t& dir_light = dir_lights[light_id];

  bind_framebuffer(dir_light.debug_light_pass_fbs[cascade]);
  clear_framebuffer(dir_light.debug_light_pass_fbs[cascade]);

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

void render_dir_light_shadow_maps(int dir_light_id) {
  glClear(GL_DEPTH_BUFFER_BIT);
  dir_light_t& dir_light = dir_lights[dir_light_id];

  mat4 o = ortho_mat(0, window.window_dim.x, 0, window.window_dim.y, -10, 10);
  shader_set_mat4(dir_light_t::display_shadow_map_shader, "projection", o);
  shader_set_int(dir_light_t::display_shadow_map_shader, "shadow_map", 0);

  float display_dim = 200.f;

  for (int i = 0; i < NUM_SM_CASCADES; i++) {
    shader_set_int(dir_light_t::display_shadow_map_shader, "cascade", i);
    vec2 top_left = {display_dim*i, 700};

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

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, dir_light.light_pass_fb.depth_att);

    bind_shader(dir_light_t::display_shadow_map_shader);
    bind_vao(dir_light.display_shadow_map_vao);
    draw_ebo(dir_light.display_shadow_map_ebo);
    unbind_vao();
    unbind_ebo();
  }

}

dir_light_t* get_dir_light(int id) {
  return &dir_lights[id];
}
