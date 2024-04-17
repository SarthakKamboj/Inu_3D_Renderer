#pragma once

#include "model_loading/model_internal.h"
#include "utils/vectors.h"
#include "utils/quaternion.h"
#include "utils/mats.h"

#include <vector>
#include <string>
#include <unordered_map>

#define MAX_SUPPORTED_TEX_COORDS 2

struct gltf_param_t {
  std::string key;
  std::string value;
};

struct gltf_node_t {
  std::vector<int> child_node_idxs;
  int gltf_mesh_handle = -1;
  vec3 translation;
  vec3 scale;
  quaternion_t rot;
  int gltf_skin_idx = -1;
  std::string name;
};

struct gltf_scene_t {
  std::vector<int> root_nodes;
};

struct gltf_attributes_t {
  int normals_accessor_idx = -1;
  int positions_accessor_idx = -1;
  int color_0_accessor_idx = -1;
  int tex_coord_accessor_indicies[MAX_SUPPORTED_TEX_COORDS];
  int joints_0_accessor_idx = -1;
  int weights_0_accessor_idx = -1;
};

enum class GLTF_PRIMITIVE_MODE {
  POINTS = 0,
  LINES,
  LINE_LOOP,
  LINE_STRIP,
  TRIANGLES,
  TRIANGLE_STRIP,
  TRIANGLE_FAN
};

struct gltf_primitive_t {
  int indicies_accessor_idx = -1;
  int material_idx = -1;
  // gpu topology type
  GLTF_PRIMITIVE_MODE mode = GLTF_PRIMITIVE_MODE::TRIANGLES;
  gltf_attributes_t attribs;
};

struct gltf_mesh_t {
  std::string name;
  std::vector<gltf_primitive_t> primitives; 
};

struct gltf_buffer_t {
  int byte_length = -1;
  std::string uri;
};

enum class BUFFER_VIEW_TARGET {
  NONE = 0,
  ARRAY_BUFFER = 34962,
  ELEMENT_ARRAY_BUFFER = 34963
};

struct gltf_buffer_view_t {
  int gltf_buffer_index = -1;
  int byte_offset = 0;
  int byte_length = -1;
  BUFFER_VIEW_TARGET target = BUFFER_VIEW_TARGET::NONE;
  // will need to look into later on when this value becomes more important
  int byte_stride = -1; 
};

struct gltf_mat_image_info_t {
  int gltf_texture_idx = -1;
  int tex_coord_idx = 0;
  std::unordered_map<std::string, std::string> extra_params;
};

struct gltf_normal_tex_info_t {
  gltf_mat_image_info_t tex_info;
  float x_y_normals_scale = 1;
};

struct gltf_occ_tex_info_t {
  gltf_mat_image_info_t tex_info;
  float strength = -1;
};

enum class ALPHA_MODE {
  OPQUE = 0,
  MASK,
  BLEND
};

struct gltf_pbr_metallic_roughness_t {
  // base colors
  // option 1
  vec4 base_color_factor = {1,1,1,1};
  // option 2
  vec4 base_color_tex_multiplers = {1,1,1,1};
  gltf_mat_image_info_t base_color_tex_info;

  // metallicness/roughness
  // option 1
  float metallic_factor = 1;
  float roughness_factor = 1;
  // option 2
  gltf_mat_image_info_t metal_rough_tex_info;
 
};

struct gltf_material_t {
  std::string name;
  gltf_pbr_metallic_roughness_t pbr;

  // normals tex info
  gltf_normal_tex_info_t normal_tex_info;

  // occlusion
  gltf_occ_tex_info_t occ_tex_info;
   
  // emission
  // option 1
  vec3 emissive_factor;
  // option 2
  gltf_mat_image_info_t emissive_tex_info;

  // alpha settings
  ALPHA_MODE alpha_mode = ALPHA_MODE::OPQUE;
  float alpha_cutoff = 0.5f;

  // double-sidedness
  bool double_sided = false;
};

enum class ACC_COMPONENT_TYPE {
  BYTE = 5120,
  UNSIGNED_BYTE = 5121,
  SHORT = 5122,
  UNSIGNED_SHORT = 5123,
  UNSIGNED_INT = 5125,
  FLOAT = 5126
};

enum class ACC_ELEMENT_TYPE {
  SCALAR = 0,
  VEC2,
  VEC3,
  VEC4,
  MAT2,
  MAT3,
  MAT4
};
int get_num_components_for_gltf_element(ACC_ELEMENT_TYPE el);

struct gltf_accessor_t {
  int buffer_view_idx = -1;
  int byte_offset = 0;
  ACC_COMPONENT_TYPE component_type = ACC_COMPONENT_TYPE::BYTE;
  int count = -1;
  ACC_ELEMENT_TYPE element_type = ACC_ELEMENT_TYPE::SCALAR;
};

struct gltf_image_t {
  std::string uri;
  // need to add support for image in buffer view
};

struct gltf_texture_t {
  int sampler_idx = -1;
  int image_source_idx = -1;
};

enum class MAG_FILTER {
  NONE = 0,
  NEAREST = 9728,
  LINEAR = 9729
};

enum class MIN_FILTER {
  NONE = 0,
  NEAREST = 9728,
  LINEAR = 9729,
  NEAREST_MIPMAP_NEAREST = 9984,
  LINEAR_MIPMAP_NEAREST = 9985,
  NEAREST_MIPMAP_LINEAR = 9986,
  LINEAR_MIPMAP_LINEAR = 9987
};

enum class SAMPLER_WRAP {
  NONE = 0,
  CLAMP_TO_EDGE = 33071,
  MIRRORED_REPEAT = 33648,
  REPEAT = 10497
};

struct gltf_sampler_t {
  MAG_FILTER mag_filter = MAG_FILTER::LINEAR;
  MIN_FILTER min_filter = MIN_FILTER::LINEAR;
  SAMPLER_WRAP wrap_s = SAMPLER_WRAP::REPEAT;
  SAMPLER_WRAP wrap_t = SAMPLER_WRAP::REPEAT;
};

enum class CHANNEL_TARGET_PATH {
  NONE = 0,
  ROTATION,
  SCALE,
  TRANSLATION,
  WEIGHTS
};

struct gltf_channel_target_t {
  int gltf_node_idx = -1; 
  CHANNEL_TARGET_PATH path = CHANNEL_TARGET_PATH::NONE;
};

struct gltf_channel_t {
  int gltf_anim_sampler_idx = -1;
  gltf_channel_target_t target;
};

enum class GLTF_INTERPOLATION_MODE {
  LINEAR,
  STEP,
  CUBICSPLINE
};

struct gltf_anim_sampler_t {
  int input_accessor_idx = -1;
  int output_accessor_idx = -1;
  GLTF_INTERPOLATION_MODE interpolation_mode = GLTF_INTERPOLATION_MODE::LINEAR;
};

/*
 - Multiple nodes can be references in one animation
 - 2 nodes in the same animation can have the same sampler
 */

struct gltf_animation_t {
  std::string name; 
  std::vector<gltf_channel_t> channels;
  std::vector<gltf_anim_sampler_t> anim_samplers;
};

struct gltf_skin_t {
  int inverse_bind_matrix_acc_idx = -1;
  int upper_most_joint_node_idx = -1;
  std::vector<int> joint_node_idxs;
  std::string name;
};

bool gltf_parse_boolean();
int gltf_parse_integer();
gltf_mat_image_info_t gltf_parse_mat_image_info();
std::string gltf_parse_string();
std::vector<int> gltf_parse_integer_array();
vec4 gltf_parse_vec4();
vec3 gltf_parse_vec3();
mat4 gltf_parse_mat4();
float gltf_parse_float();

gltf_primitive_t gltf_parse_primitive();
std::vector<gltf_primitive_t> gltf_parse_primitives_section();
gltf_attributes_t gltf_parse_attribs();

/*
 gltf sections

  asset
  scene
  scenes --
    nodes --
  nodes
    children --
    matrix
    mesh --
  meshes --
    primitives --
      attributes --
        normal --
        position --
      indices --
      mode --
      material --
    name --
  accessors
    bufferView --
    byteOffset --
    componentType --
    count --
    max
    min
    type --
  materials
    pbrMetallicRoughness
      baseColorFactor
      metallicFactor
    name
  bufferViews --
    buffer --
    byteOffset --
    byteLength --
    target --
    byteStride --
  buffers --
    byteLength --
    uri --
  textures
    sampler
    source
  images
    magFilter
    minFilter
    wrapS
    wrapT
 */

void gltf_skip_section();
void gltf_parse_scenes_section();
void gltf_parse_asset_section();
void gltf_parse_nodes_section();
void gltf_parse_meshes_section();
void gltf_parse_buffers_section();
void gltf_parse_buffer_views_section();
void gltf_parse_accessors_section();
void gltf_parse_images_section();
void gltf_parse_textures_section();
void gltf_parse_samplers_section();
void gltf_parse_materials_section();

void gltf_eat();
void gltf_parse_section();
char gltf_peek();
void gltf_preprocess(const char* filepath);
void gltf_load_file(const char* filepath);
