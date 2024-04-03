#include "light.h"

#include "gfx/gfx.h"
#include "windowing/window.h"
#include "utils/general.h"
#include "scene/scene.h"
#include "scene/camera.h"
#include "model_loading/gltf/gltf.h"
#include "scene/scene.h"
#include "animation/interpolation.h"

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

// const float dir_light_t::SHADOW_MAP_WIDTH = 4096.f;
// const float dir_light_t::SHADOW_MAP_HEIGHT = 4096.f;
const float dir_light_t::SHADOW_MAP_WIDTH = 1024.f;
const float dir_light_t::SHADOW_MAP_HEIGHT = 1024.f;
shader_t dir_light_t::light_shader;
shader_t dir_light_t::debug_shader;

extern bool render_dir_orthos;

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

  dir_lights.push_back(light);
  return light.id;
}

struct frustum_t {
  vec3 frustum_corners[NUM_CUBE_CORNERS]{};
};

void gen_dir_light_matricies(int light_id, camera_t* camera) {
  dir_light_t& dir_light = dir_lights[light_id];

  // calculate camera frustum in world coordinates
#if 1
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
#else
  vec4 cam_frustum_ndc_corners[NUM_CUBE_CORNERS] = {
    // near bottom left
    {-1,-1,-1,1},
    // far bottom left
    {-1,-1,1,1},
    // near top left
    {-1,1,-1,1},
    // far top left 
    {-1,1,1,1},
    // near bottom right
    {1,-1,-1,1},
    // far bottom right
    {1,-1,1,1},
    // near top right
    {1,1,-1,1},
    // far top right
    {1,1,1,1}
  };
#endif
  frustum_t world_cam_frustum;
  mat4 cam_view = get_cam_view_mat();
  mat4 cam_proj = get_cam_proj_mat();
  for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
    mat4 c = mat_multiply_mat(cam_proj, cam_view);
    vec4 corner = vec4(cam_frustum_ndc_corners.frustum_corners[i], 1.0f);
    vec4 world_unnorm = mat_multiply_vec(mat4_inverse(c), corner);
    vec4 world_norm = world_unnorm / world_unnorm.w;
    world_cam_frustum.frustum_corners[i] = {world_norm.x, world_norm.y, world_norm.z};
  }

  // split camera frustum in cascades
  const int N = NUM_SM_CASCADES;
  const float N_f = static_cast<float>(N);
  frustum_t cascaded_frustums[N];
  for (int cascade = 0; cascade < NUM_SM_CASCADES; cascade++) {
    frustum_t& vf = cascaded_frustums[cascade];

    float n = camera->near_plane;
    float f = camera->far_plane;

    float z_near = (0.5f*n*pow(f/n, cascade/N_f)) + (0.5f*(n+(cascade/N_f*(f-n))));
    float z_far = (0.5f*n*pow(f/n, (cascade+1)/N_f)) + (0.5f*(n+((cascade+1)/N_f*(f-n))));

    dir_light.cacade_depths[cascade] = z_near;
    dir_light.cacade_depths[cascade+1] = z_far;
    
    float zs[2] = {z_near, z_far};
    for (int i = 0; i < 2; i++) {
      float inter_n = (z_near - n) / (f - n);
      float inter_f = (z_far - n) / (f - n);

      for (int j = 0; j < NUM_CUBE_CORNERS / 2; j++) {
        int fc_near_idx = j*2;
        int fc_far_idx = fc_near_idx + 1;
        vf.frustum_corners[fc_near_idx] = vec3_linear(world_cam_frustum.frustum_corners[fc_near_idx], world_cam_frustum.frustum_corners[fc_far_idx], inter_n);
        vf.frustum_corners[fc_far_idx] = vec3_linear(world_cam_frustum.frustum_corners[fc_near_idx], world_cam_frustum.frustum_corners[fc_far_idx], inter_f);
      }
    }

#if RENDER_DIR_LIGHT_FRUSTUMS
    int frustum_obj_id = dir_light.debug_obj_ids[cascade];
    vbo_t* vbo = get_obj_vbo(frustum_obj_id, 0);

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

    vertices[BTR].position = vf.frustum_corners[7];
    vertices[FTR].position = vf.frustum_corners[6];
    vertices[BBR].position = vf.frustum_corners[5];
    vertices[FBR].position = vf.frustum_corners[4];
    vertices[BTL].position = vf.frustum_corners[3];
    vertices[FTL].position = vf.frustum_corners[2];
    vertices[BBL].position = vf.frustum_corners[1];
    vertices[FBL].position = vf.frustum_corners[0];

    update_vbo_data(*vbo, vertices, sizeof(vertices));
#endif
  }

  // calculate view and ortho mats for each cascaded frustum
  for (int cascade = 0; cascade < NUM_SM_CASCADES; cascade++) {
    // get center
    frustum_t& vf = cascaded_frustums[cascade];
    vec3 frustum_center{};
    for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
      frustum_center.x += vf.frustum_corners[i].x;
      frustum_center.y += vf.frustum_corners[i].y;
      frustum_center.z += vf.frustum_corners[i].z;
    }
    frustum_center.x /= static_cast<float>(NUM_CUBE_CORNERS);
    frustum_center.y /= static_cast<float>(NUM_CUBE_CORNERS);
    frustum_center.z /= static_cast<float>(NUM_CUBE_CORNERS);

    // calc view mat
    dir_light.light_views[cascade] = get_view_mat(frustum_center, vec3_add(frustum_center, dir_light.dir));

    // get mins and maxs
#if 0 
    float x_min = vf.frustum_corners[0].x, x_max = vf.frustum_corners[0].x;
    float y_min = vf.frustum_corners[0].y, y_max = vf.frustum_corners[0].y;
    float z_min = vf.frustum_corners[0].z, z_max = vf.frustum_corners[0].z;
#else
    float x_min = FLT_MAX, x_max = -FLT_MAX;
    float y_min = FLT_MAX, y_max = -FLT_MAX;
    float z_min = FLT_MAX, z_max = -FLT_MAX;
#endif
    for (int i = 0; i < NUM_CUBE_CORNERS; i++) {
      vec4 pt = {vf.frustum_corners[i].x, vf.frustum_corners[i].y, vf.frustum_corners[i].z, 1.0f};
      vec4 light_rel_view_pt = mat_multiply_vec(dir_light.light_views[cascade], pt);
      x_min = min(x_min, light_rel_view_pt.x);
      x_max = max(x_max, light_rel_view_pt.x);
      y_min = min(y_min, light_rel_view_pt.y);
      y_max = max(y_max, light_rel_view_pt.y);
      z_min = min(z_min, light_rel_view_pt.z);
      z_max = max(z_max, light_rel_view_pt.z);
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

    // calc ortho mat
    dir_light.light_orthos[cascade] = ortho_mat(x_min, x_max, y_min, y_max, z_min, z_max);


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

    vertices[BTR].position = light_ortho_world_frustum.frustum_corners[BTR];
    vertices[FTR].position = light_ortho_world_frustum.frustum_corners[FTR];
    vertices[BBR].position = light_ortho_world_frustum.frustum_corners[BBR];
    vertices[FBR].position = light_ortho_world_frustum.frustum_corners[FBR];
    vertices[BTL].position = light_ortho_world_frustum.frustum_corners[BTL];
    vertices[FTL].position = light_ortho_world_frustum.frustum_corners[FTL];
    vertices[BBL].position = light_ortho_world_frustum.frustum_corners[BBL];
    vertices[FBL].position = light_ortho_world_frustum.frustum_corners[FBL];

    update_vbo_data(*vbo, vertices, sizeof(vertices)); 
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

dir_light_t* get_dir_light(int id) {
  return &dir_lights[id];
}
