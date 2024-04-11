#include "gltf.h"

#include <ctype.h>
#include <unordered_set>
#include <algorithm>
#include <errno.h>
#include <stdio.h>
#include <unordered_map>
#include <cmath>

#include "utils/log.h"
#include "utils/vectors.h"
#include "model_loading/model_internal.h"
#include "animation/animation_internal.h"
#include "gfx/gfx.h"
#include "scene/scene.h"

#define SET_MESHES_TO_WHITE 0

/*
 https://github.com/KhronosGroup/glTF-Sample-Models/blob/main/2.0/README.md#showcase
 */

char* data = NULL;
int offset = -1;
int data_len = -1;

static int active_scene = -1;
static std::vector<gltf_scene_t> gltf_scenes; 
static std::vector<gltf_node_t> gltf_nodes; 
static std::vector<gltf_mesh_t> gltf_meshes;
static std::vector<gltf_buffer_t> gltf_buffers;
static std::vector<gltf_buffer_view_t> gltf_buffer_views;
static std::vector<gltf_accessor_t> gltf_accessors; 
static std::vector<gltf_image_t> gltf_images;
static std::vector<gltf_texture_t> gltf_textures;
static std::vector<gltf_sampler_t> gltf_samplers;
static std::vector<gltf_material_t> gltf_materials;
static std::vector<gltf_animation_t> gltf_animations;
static std::vector<gltf_skin_t> gltf_skins;

static std::unordered_map<int, int> gltf_mesh_id_to_internal_model_id;

static char folder_path[256];

void gltf_skip_section() {
  // skip logic
  int num_opening_seen = 0;  
  bool looking_for_comma = true;
  char opening = 0;
  char closing = 0;
  while (true) {
    if (looking_for_comma && gltf_peek() == ',') {
      gltf_eat();
      break;
    } else if (!looking_for_comma && num_opening_seen == 0) {
      break;
    }
    char peeked = gltf_peek();

    if (peeked == '{' && opening == 0) {
      opening = '{';
      closing = '}';
    } else if (peeked == '[' && opening == 0) {
      opening = '[';
      closing = ']';
    }

    if ((peeked == ']' || peeked == '}') && looking_for_comma) {
      break;
    }

    if (peeked == opening) {
      looking_for_comma = false;
      num_opening_seen++;
    } else if (peeked == closing) {
      num_opening_seen--;
    }
    gltf_eat();
  }

  if (!looking_for_comma && gltf_peek() == ',') {
    gltf_eat();
  }
}

void gltf_parse_asset_section() {
  inu_assert(gltf_peek() == '{');
  gltf_eat();

  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();

    inu_assert(gltf_peek() == ':');
    gltf_eat();

    if (key == "version") {
      std::string version = gltf_parse_string();
      float fversion = atof(version.c_str());
      if (fversion < 2.f || fversion >= 3.f) {
        inu_assert_msg("gltf file is not major version 2");
      }
    } else if (key == "copyright") {
      gltf_parse_string();
    } else if (key == "generator") {
      gltf_parse_string();
    } else {
      gltf_skip_section();
    }

    if (gltf_peek() == ',') {
      gltf_eat();
    }
  }
  gltf_eat(); 
}

std::vector<int> gltf_parse_integer_array() {
  std::vector<int> ints;
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  char* int_str_start = data + offset;
  while (gltf_peek() != ']') {
    if (gltf_peek() == ',') {
      gltf_eat();
      data[offset-1] = 0;
      int val = atoi(int_str_start);
      ints.push_back(val);
      int_str_start = data + offset;
      continue;
    }
    gltf_eat();
  }
  gltf_eat();
  data[offset-1] = 0;
  int val = atoi(int_str_start);
  ints.push_back(val);
  return ints;
}

float gltf_parse_float() {
  bool looking_for_comma = true;
  char* start = data + offset;
  while (gltf_peek() != ',' && gltf_peek() != '}' && gltf_peek() != ']') {
    gltf_eat();
  }
  char c = gltf_peek();
  data[offset] = 0;
  float v = atof(start);
  data[offset] = c;
  return v;
}

vec3 gltf_parse_vec3() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();

  vec3 v;

  bool looking_for_comma = true;
  v.x = gltf_parse_float();
  if (gltf_peek() == ',') gltf_eat();
  v.y = gltf_parse_float();
  if (gltf_peek() == ',') gltf_eat();
  v.z = gltf_parse_float();

  inu_assert(gltf_peek() == ']');
  gltf_eat();

  return v;
}

mat4 gltf_parse_mat4() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();

  mat4 m;
  float* cur_float = &m.m11;
  for (int i = 0; i < 16; i++) {
    *cur_float = gltf_parse_float();
    if (gltf_peek() == ',') gltf_eat();
    cur_float++;
  }

  inu_assert(gltf_peek() == ']');
  gltf_eat();

  return m;
}

vec4 gltf_parse_vec4() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();

  vec4 v;

  v.x = gltf_parse_float();
  if (gltf_peek() == ',') gltf_eat();
  v.y = gltf_parse_float();
  if (gltf_peek() == ',') gltf_eat();
  v.z = gltf_parse_float();
  if (gltf_peek() == ',') gltf_eat();
  v.w = gltf_parse_float();

  inu_assert(gltf_peek() == ']');
  gltf_eat();

  return v;
}

void gltf_parse_scene() {

  gltf_scene_t scene;

  inu_assert(gltf_peek() == '{');
  gltf_eat();
  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();

    if (key == "nodes") {
      // get the root nodes
      scene.root_nodes = gltf_parse_integer_array();
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  gltf_scenes.push_back(scene);
}

void gltf_parse_scenes_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  while (gltf_peek() != ']') {
    gltf_parse_scene();
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
}

void gltf_parse_node() {
  gltf_node_t node;

  inu_assert(gltf_peek() == '{');
  gltf_eat();
  bool scale_parsed = false;
  bool ignore_trs = false;
  while (gltf_peek() != '}') {

    std::string key = gltf_parse_string();

    inu_assert(gltf_peek() == ':');
    gltf_eat();

    if (key == "children") {
      // get the root nodes
      node.child_node_idxs = gltf_parse_integer_array();
      if (gltf_peek() == ',') gltf_eat();
    } else if (key == "mesh") {
      node.gltf_mesh_handle = gltf_parse_integer();
    } else if (key == "translation") {
      vec3 t = gltf_parse_vec3();
      if (!ignore_trs) {
        node.translation = t;
      }
    } else if (key == "scale") {
      vec3 s = gltf_parse_vec3();
      if (!ignore_trs) {
        node.scale = s;
        scale_parsed = true;
      }
    } else if (key == "rotation") {
      vec4 rot = gltf_parse_vec4();
      if (!ignore_trs) {
        node.rot.x = rot.x;
        node.rot.y = rot.y;
        node.rot.z = rot.z;
        node.rot.w = rot.w;
        if (is_nan_quat(node.rot)) {
          inu_assert_msg("gltf node rot is nan");
        } 
      }
    } else if (key == "matrix") {
      mat4 mat = gltf_parse_mat4();
      transform_t t = get_transform_from_matrix(mat);
      node.translation = t.pos;
      node.scale = t.scale;
      node.rot = t.rot;
      if (is_nan_quat(node.rot)) {
        inu_assert_msg("gltf node rot is nan");
        transform_t t = get_transform_from_matrix(mat);
      }
      scale_parsed = true;
      ignore_trs = true;
    } else if (key == "skin") {
      node.gltf_skin_idx = gltf_parse_integer();
    } else if (key == "name") {
      node.name = gltf_parse_string();
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  
  if (!scale_parsed) {
    node.scale.x = 1;
    node.scale.y = 1;
    node.scale.z = 1;
  }

  gltf_nodes.push_back(node);
}

void gltf_parse_nodes_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  while (gltf_peek() != ']') {
    gltf_parse_node();
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
}

std::string gltf_parse_string() {
  inu_assert(gltf_peek() == '\"');
  gltf_eat();
  char* start = data + offset;
  while (gltf_peek() != '\"') {
    gltf_eat();
  }
  data[offset] = 0;
  std::string val = start;
  data[offset] = '\"';
  gltf_eat();

  return val;
}

gltf_attributes_t gltf_parse_attribs() {
  gltf_attributes_t attrib;

  for (int i = 0; i < MAX_SUPPORTED_TEX_COORDS; i++) {
    attrib.tex_coord_accessor_indicies[i] = -1;
  }

  inu_assert(gltf_peek() == '{');
  gltf_eat();

  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();

    inu_assert(gltf_peek() == ':');
    gltf_eat();

    if (key == "NORMAL") {
      attrib.normals_accessor_idx = gltf_parse_integer();
    } else if (key == "POSITION") {
      attrib.positions_accessor_idx = gltf_parse_integer();
    } else if (strstr(key.c_str(),  "TEXCOORD") != NULL) {
      const char* tex_coord_idx_char = strstr(key.c_str(), "_") + 1;
      int tex_coord_idx = atoi(tex_coord_idx_char);
      if (tex_coord_idx < MAX_SUPPORTED_TEX_COORDS) {
        attrib.tex_coord_accessor_indicies[tex_coord_idx] = gltf_parse_integer();
      }
    } else if (key == "COLOR_0") {
      attrib.color_0_accessor_idx = gltf_parse_integer();
    } else if (key == "JOINTS_0") {
      attrib.joints_0_accessor_idx = gltf_parse_integer();
    } else if (key == "WEIGHTS_0") {
      attrib.weights_0_accessor_idx = gltf_parse_integer();
    } else {
      // not sure if this works for individual elements
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }  
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();

  return attrib;
}

gltf_primitive_t gltf_parse_primitive() {
  gltf_primitive_t prim;

  inu_assert(gltf_peek() == '{');
  gltf_eat();

  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();

    inu_assert(gltf_peek() == ':');
    gltf_eat();

    if (key == "attributes") {
      prim.attribs = gltf_parse_attribs();
    } else if (key == "indices") {
      prim.indicies_accessor_idx = gltf_parse_integer();
    } else if (key == "mode") {
      prim.mode = static_cast<GLTF_PRIMITIVE_MODE>(gltf_parse_integer());
    } else if (key == "material") {
      prim.material_idx = gltf_parse_integer();
    } else {
      gltf_skip_section();
    }
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();

  return prim;
}

std::vector<gltf_primitive_t> gltf_parse_primitives_section() {
  std::vector<gltf_primitive_t> prims;
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  while (gltf_peek() != ']') {
    gltf_primitive_t prim = gltf_parse_primitive();
    prims.push_back(prim);
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  return prims;
}

void gltf_parse_mesh() {
  gltf_mesh_t mesh;

  inu_assert(gltf_peek() == '{');
  gltf_eat();

  while (gltf_peek() != '}') {

    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();

    if (key == "primitives") {
      mesh.primitives = gltf_parse_primitives_section();
    } else if (key == "name") {
      mesh.name = gltf_parse_string();
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();

  gltf_meshes.push_back(mesh);
}

void gltf_parse_meshes_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  while (gltf_peek() != ']') {
    gltf_parse_mesh();
  }
  gltf_eat();
}

bool gltf_parse_boolean() {
  char* start = data + offset; 
  while (gltf_peek() != ',' && gltf_peek() != '}' && gltf_peek() != ']') {
    gltf_eat();
  }
  char c = data[offset];
  data[offset] = 0;
  if (strcmp(start, "true") == 0) {
    data[offset] = c;
    return true;
  } else if (strcmp(start, "false") == 0) {
    data[offset] = c;
    return false;
  }
  inu_assert_msg("boolean parameter does not have true or false");
}

int gltf_parse_integer() {
  char* start = data + offset; 
  int val = -1;
  while (true) {
    if (gltf_peek() == ',') {
      gltf_eat();
      data[offset-1] = 0;
      val = atoi(start);
      break;
    } else if (gltf_peek() == '}') {
      data[offset] = 0;
      val = atoi(start);
      data[offset] = '}';
      break;
    }
    gltf_eat();
  }
  return val;
}

std::string replace_uri_encoded_space(std::string& in) {
  std::string val = "";
  int i;
  for (i = 0; i < in.size() - 3; i++) {
    if (in[i] == '%' && in[i+1] == '2' && in[i+2] == '0') {
      val = val + " ";
      i = i + 2;
    } else {
      val = val + in[i];
    }
  }
  val = val + in[i];
  val = val + in[i+1];
  val = val + in[i+2];
  return val;
}

void gltf_parse_buffer() {
  gltf_buffer_t buffer; 
  inu_assert(gltf_peek() == '{');
  gltf_eat();
  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "byteLength") {
      buffer.byte_length = gltf_parse_integer();
    } else if (key == "uri") {
      buffer.uri = gltf_parse_string();
      // std::replace(buffer.uri.begin(), buffer.uri.end(), std::string("%20"), std::string(" "));
      buffer.uri = replace_uri_encoded_space(buffer.uri);
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  gltf_buffers.push_back(buffer);
}

void gltf_parse_buffers_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  while (gltf_peek() != ']') {
    gltf_parse_buffer();
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
}

void gltf_parse_accessor() {
  gltf_accessor_t accessor; 
  inu_assert(gltf_peek() == '{');
  gltf_eat();
  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "bufferView") {
      accessor.buffer_view_idx = gltf_parse_integer();
    } else if (key == "byteOffset") {
      accessor.byte_offset = gltf_parse_integer();
    } else if (key == "componentType") {
      accessor.component_type = static_cast<ACC_COMPONENT_TYPE>(gltf_parse_integer());
    } else if (key == "count") {
      accessor.count = gltf_parse_integer();
    } else if (key == "type") {
      std::string type_str = gltf_parse_string();
      if (type_str == "SCALAR") {
        accessor.element_type = ACC_ELEMENT_TYPE::SCALAR;
      } else if (type_str == "VEC2") {
        accessor.element_type = ACC_ELEMENT_TYPE::VEC2;
      } else if (type_str == "VEC3") {
        accessor.element_type = ACC_ELEMENT_TYPE::VEC3;
      } else if (type_str == "VEC4") {
        accessor.element_type = ACC_ELEMENT_TYPE::VEC4;
      } else if (type_str == "MAT2") {
        accessor.element_type = ACC_ELEMENT_TYPE::MAT2;
      } else if (type_str == "MAT3") {
        accessor.element_type = ACC_ELEMENT_TYPE::MAT3;
      } else if (type_str == "MAT4") {
        accessor.element_type = ACC_ELEMENT_TYPE::MAT4;
      }
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  gltf_accessors.push_back(accessor);
}

void gltf_parse_accessors_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  while (gltf_peek() != ']') {
    gltf_parse_accessor();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
}

void gltf_parse_buffer_view() {
  gltf_buffer_view_t buffer_view;

  inu_assert(gltf_peek() == '{');
  gltf_eat();

  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "buffer") {
      buffer_view.gltf_buffer_index = gltf_parse_integer();
    } else if (key == "byteOffset") {
      buffer_view.byte_offset = gltf_parse_integer();
    } else if (key == "byteLength") {
      buffer_view.byte_length = gltf_parse_integer();
    } else if (key == "byteStride") {
      buffer_view.byte_stride = gltf_parse_integer();
    } else if (key == "target") {
      buffer_view.target = static_cast<BUFFER_VIEW_TARGET>(gltf_parse_integer());
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();

  gltf_buffer_views.push_back(buffer_view);
}

void gltf_parse_buffer_views_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  while (gltf_peek() != ']') {
    gltf_parse_buffer_view();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
}

void gltf_parse_image() {
  inu_assert(gltf_peek() == '{');
  gltf_eat();

  gltf_image_t img;

  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "uri") {
      img.uri = gltf_parse_string();
      img.uri = replace_uri_encoded_space(img.uri);
    } else {
      gltf_skip_section();
    }

    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();

  gltf_images.push_back(img);
}

void gltf_parse_images_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  while (gltf_peek() != ']') {
    gltf_parse_image();
  }
  gltf_eat();
}

void gltf_parse_texture() {
  inu_assert(gltf_peek() == '{');
  gltf_eat();

  gltf_texture_t tex;

  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "sampler") {
      tex.sampler_idx = gltf_parse_integer();
    } else if (key == "source") {
      tex.image_source_idx = gltf_parse_integer();
    } else {
      gltf_skip_section();
    }

    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();

  gltf_textures.push_back(tex);
}

void gltf_parse_textures_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  while (gltf_peek() != ']') {
    gltf_parse_texture();
  }
  gltf_eat();
}

void gltf_parse_sampler() {
  inu_assert(gltf_peek() == '{');
  gltf_eat();

  gltf_sampler_t sampler;

  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "magFilter") {
      sampler.mag_filter = static_cast<MAG_FILTER>(gltf_parse_integer());
    } else if (key == "minFilter") {
      sampler.min_filter = static_cast<MIN_FILTER>(gltf_parse_integer());
    } else if (key == "wrapS") {
      sampler.wrap_s = static_cast<SAMPLER_WRAP>(gltf_parse_integer());
    } else if (key == "wrapT") {
      sampler.wrap_t = static_cast<SAMPLER_WRAP>(gltf_parse_integer());
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();

  gltf_samplers.push_back(sampler);
}

void gltf_parse_samplers_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();

  while (gltf_peek() != ']') {
    gltf_parse_sampler();
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
}

gltf_mat_image_info_t gltf_parse_mat_image_info() {
  gltf_mat_image_info_t info;
  inu_assert(gltf_peek() == '{');
  gltf_eat();
  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "index") {
      info.gltf_texture_idx = gltf_parse_integer();
    } else if (key == "texCoord") {
      info.tex_coord_idx = gltf_parse_integer();
    } else {
      info.extra_params[key] = gltf_parse_string();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  return info;
}

gltf_pbr_metallic_roughness_t gltf_parse_pbr_met_rough() {
  inu_assert(gltf_peek() == '{');
  gltf_eat();

  gltf_pbr_metallic_roughness_t pbr;
  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "baseColorFactor") {
      pbr.base_color_factor = gltf_parse_vec4();
      pbr.base_color_tex_multiplers = pbr.base_color_factor;
    } else if (key == "baseColorTexture") {
      pbr.base_color_tex_info = gltf_parse_mat_image_info();
    } else if (key == "metallicFactor") {
      pbr.metallic_factor = gltf_parse_float();
    } else if (key == "roughnessFactor") {
      pbr.metallic_factor = gltf_parse_float();
    } else if (key == "metallicRoughnessTexture") {
      pbr.metal_rough_tex_info = gltf_parse_mat_image_info();
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();

  return pbr;
}

gltf_normal_tex_info_t gltf_parse_normal_tex_info() {
  gltf_mat_image_info_t info = gltf_parse_mat_image_info();

  gltf_normal_tex_info_t normal_tex_info;
  normal_tex_info.tex_info.gltf_texture_idx = info.gltf_texture_idx;
  normal_tex_info.tex_info.tex_coord_idx = info.tex_coord_idx;
  if (info.extra_params.find("scale") != info.extra_params.end()) {
    std::string scale_str = info.extra_params["scale"];
    normal_tex_info.x_y_normals_scale = atof(scale_str.c_str());
  }
   
  return normal_tex_info;
}

gltf_occ_tex_info_t gltf_parse_occ_tex_info() {
  gltf_mat_image_info_t info = gltf_parse_mat_image_info();

  gltf_occ_tex_info_t occ_tex_info;
  occ_tex_info.tex_info.gltf_texture_idx = info.gltf_texture_idx;
  occ_tex_info.tex_info.tex_coord_idx = info.tex_coord_idx;
  if (info.extra_params.find("strength") != info.extra_params.end()) {
    std::string strength_str = info.extra_params["strength"];
    occ_tex_info.strength = atof(strength_str.c_str());
  }
  return occ_tex_info;
}

void gltf_parse_material() {
  inu_assert(gltf_peek() == '{');
  gltf_eat();

  gltf_material_t mat;

  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "pbrMetallicRoughness") {
      mat.pbr = gltf_parse_pbr_met_rough();
    } else if (key == "name") {
      mat.name = gltf_parse_string();
    } else if (key == "alphaMode") {
      std::string alpha_mode_str = gltf_parse_string();
      if (alpha_mode_str == "OPAQUE") {
        mat.alpha_mode = ALPHA_MODE::OPQUE;
      } else if (alpha_mode_str == "MASK") {
        mat.alpha_mode = ALPHA_MODE::MASK;
      } else if (alpha_mode_str == "BLEND") {
        mat.alpha_mode = ALPHA_MODE::BLEND;
      }
    } else if (key == "alphaCutoff") {
      mat.alpha_cutoff = gltf_parse_float();
    } else if (key == "doubleSided") {
      mat.double_sided = gltf_parse_boolean();
    } else if (key == "normalTexture") {
      mat.normal_tex_info = gltf_parse_normal_tex_info();
    } else if (key == "occlusionTexture") {
      mat.occ_tex_info = gltf_parse_occ_tex_info();
    } else if (key == "emissiveTexture") {
      mat.emissive_tex_info = gltf_parse_mat_image_info();
    } else if (key == "emissiveFactor") {
      mat.emissive_factor = gltf_parse_vec3();
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();

  gltf_materials.push_back(mat);
}

void gltf_parse_materials_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();

  while (gltf_peek() != ']') {
    gltf_parse_material();
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
}

gltf_channel_target_t gltf_parse_channel_target() {
  gltf_channel_target_t target;
  inu_assert(gltf_peek() == '{');
  gltf_eat();
  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();  
    if (key == "node") {
      target.gltf_node_idx = gltf_parse_integer();
    } else if (key == "path") {
      std::string path_str = gltf_parse_string();
      if (path_str == "rotation") {
        target.path = CHANNEL_TARGET_PATH::ROTATION;
      } else if (path_str == "scale") {
        target.path = CHANNEL_TARGET_PATH::SCALE;
      } else if (path_str == "translation") {
        target.path = CHANNEL_TARGET_PATH::TRANSLATION;
      } else if (path_str == "weights") {
        target.path = CHANNEL_TARGET_PATH::WEIGHTS;
      }
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  return target;
}

gltf_channel_t gltf_parse_anim_channel() {
  gltf_channel_t channel;
  inu_assert(gltf_peek() == '{');
  gltf_eat();
  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat(); 
    
    if (key == "sampler") {
      channel.gltf_anim_sampler_idx = gltf_parse_integer();
    } else if (key == "target") {
      channel.target = gltf_parse_channel_target();
    } else {
      gltf_skip_section();
    }

    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  return channel;
}

std::vector<gltf_channel_t> gltf_parse_anim_channels() {
  std::vector<gltf_channel_t> channels;  
  inu_assert(gltf_peek() == '[');
  gltf_eat();
  while (gltf_peek() != ']') {
    gltf_channel_t channel = gltf_parse_anim_channel();
    if (channel.target.gltf_node_idx != -1) {
      channels.push_back(channel);
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  return channels;
}

gltf_anim_sampler_t gltf_parse_anim_sampler() {
  gltf_anim_sampler_t anim_sampler;
  inu_assert(gltf_peek() == '{');
  gltf_eat();
  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();

    if (key == "input") {
      anim_sampler.input_accessor_idx = gltf_parse_integer();
    } else if (key == "output") {
      anim_sampler.output_accessor_idx = gltf_parse_integer();
    } else if (key == "interpolation") {
      std::string inter_str = gltf_parse_string();
      if (inter_str == "LINEAR") {
        anim_sampler.interpolation_mode = GLTF_INTERPOLATION_MODE::LINEAR;
      } else if (inter_str == "STEP") {
        anim_sampler.interpolation_mode = GLTF_INTERPOLATION_MODE::STEP;
      } else if (inter_str == "CUBICSPLINE") {
        anim_sampler.interpolation_mode = GLTF_INTERPOLATION_MODE::CUBICSPLINE;
      }
    } else {
      gltf_skip_section();
    }

    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  return anim_sampler;
}

std::vector<gltf_anim_sampler_t> gltf_parse_anim_samplers() {
  std::vector<gltf_anim_sampler_t> anim_samplers;
  inu_assert(gltf_peek() == '[');
  gltf_eat();

  while (gltf_peek() != ']') {
    gltf_anim_sampler_t sampler = gltf_parse_anim_sampler();
    anim_samplers.push_back(sampler);
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  return anim_samplers;
}

void gltf_parse_animation() {
  inu_assert(gltf_peek() == '{');
  gltf_eat();

  gltf_animation_t gltf_anim;
  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "name") {
      gltf_anim.name = gltf_parse_string();
    } else if (key == "channels") {
      gltf_anim.channels = gltf_parse_anim_channels();
    } else if (key == "samplers") {
      gltf_anim.anim_samplers = gltf_parse_anim_samplers();
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();

  gltf_animations.push_back(gltf_anim);
}

void gltf_parse_animations_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();

  while (gltf_peek() != ']') {
    gltf_parse_animation();
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
}

void gltf_parse_skin() {
  gltf_skin_t skin;

  inu_assert(gltf_peek() == '{');
  gltf_eat();
  while (gltf_peek() != '}') {
    std::string key = gltf_parse_string();
    inu_assert(gltf_peek() == ':');
    gltf_eat();
    if (key == "inverseBindMatrices") {
      skin.inverse_bind_matrix_acc_idx = gltf_parse_integer();
    } else if (key == "skeleton") {
      skin.upper_most_joint_node_idx = gltf_parse_integer();
    } else if (key == "joints") {
      skin.joint_node_idxs = gltf_parse_integer_array();
    } else if (key == "name") {
      skin.name = gltf_parse_string();
    } else {
      gltf_skip_section();
    }
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
  gltf_skins.push_back(skin);
}

void gltf_parse_skins_section() {
  inu_assert(gltf_peek() == '[');
  gltf_eat();

  while (gltf_peek() != ']') {
    gltf_parse_skin();
    if (gltf_peek() == ',') gltf_eat();
  }
  gltf_eat();
  if (gltf_peek() == ',') gltf_eat();
}

void gltf_parse_section() {

  std::string key = gltf_parse_string();

  inu_assert(gltf_peek() == ':', "colon is not seen"); 
  gltf_eat();

  if (key == "asset") {
    gltf_parse_asset_section();
  } else if (key == "nodes") {
    gltf_parse_nodes_section();
  } else if (key == "scenes") {
    gltf_parse_scenes_section();
  } else if (key == "meshes") {
    gltf_parse_meshes_section();
  } else if (key == "scene") {
    active_scene = gltf_parse_integer();
  } else if (key == "buffers") {
    gltf_parse_buffers_section();
  } else if (key == "bufferViews") {
    gltf_parse_buffer_views_section();
  } else if (key == "accessors") {
    gltf_parse_accessors_section();
  } else if (key == "images") {
    gltf_parse_images_section();
  } else if (key == "textures") {
    gltf_parse_textures_section();
  } else if (key == "samplers") {
    gltf_parse_samplers_section();
  } else if (key == "materials") {
    gltf_parse_materials_section();
  } else if (key == "animations") {
    gltf_parse_animations_section();
  } else if (key == "skins") {
    gltf_parse_skins_section();
  } else {
    gltf_skip_section();
  }

  if (gltf_peek() == ',') {
    gltf_eat();
  }
}

char gltf_peek() {
  if (offset < data_len) {
    return data[offset];
  }
  return 0;
}

void gltf_eat() {
  if (offset < data_len) {
    offset++;
  }
}

void gltf_preprocess(const char* filepath) {
  int buffer_len = 1000;
  char* buffer = (char*)malloc(buffer_len * sizeof(uint8_t));
  FILE* gltf_file = fopen(filepath, "r");
  inu_assert(gltf_file, "gltf file not found");
  int count = 0;
  bool preprocessing_str = false;
  while (!feof(gltf_file)) {
    char c = fgetc(gltf_file);
    if (c == '\"') {
      preprocessing_str = !preprocessing_str;
    }
    if (c > 0 && (preprocessing_str || (!preprocessing_str &&!isspace(c)))) {
      buffer[count] = c;
      count++;
      if (count == buffer_len) {
        buffer_len *= 2;
        buffer = (char*)realloc(buffer, buffer_len);
      }
    } 
  }
  fclose(gltf_file);

  if (buffer_len > count) {
    char* end = buffer + count;
    memset(end, 0, buffer_len - count);
  }

  data = buffer;
  data_len = count; 
  offset = 0;
}

int get_num_components_for_gltf_element(ACC_ELEMENT_TYPE el) {
  int num_comps = 0;
  if (el == ACC_ELEMENT_TYPE::SCALAR) {
    num_comps = 1;
  } else if (el == ACC_ELEMENT_TYPE::VEC3) {
    num_comps = 3; 
  } else if (el == ACC_ELEMENT_TYPE::VEC2) {
    num_comps = 2;
  } else if (el == ACC_ELEMENT_TYPE::VEC4) {
    num_comps = 4;
  } else if (el == ACC_ELEMENT_TYPE::MAT2) {
    num_comps = 4;
  } else if (el == ACC_ELEMENT_TYPE::MAT3) {
    num_comps = 9;
  }  else if (el == ACC_ELEMENT_TYPE::MAT4) {
    num_comps = 16;
  } else {
    inu_assert_msg("this accessor element type is not supported");
  }
  return num_comps;
}

void* gltf_read_accessor_data(int accessor_idx) {
  inu_assert(accessor_idx != -1);
  gltf_accessor_t& acc = gltf_accessors[accessor_idx];
  gltf_buffer_view_t& buffer_view = gltf_buffer_views[acc.buffer_view_idx];
  
  int size_of_component = 0;
  switch (acc.component_type) {
    case ACC_COMPONENT_TYPE::BYTE: {
      size_of_component = sizeof(char);
      break;
    }
    case ACC_COMPONENT_TYPE::UNSIGNED_BYTE: {
      size_of_component = sizeof(unsigned char);
      break;
    }
  case ACC_COMPONENT_TYPE::SHORT: {
      size_of_component = sizeof(short);
      break;
    }
  case ACC_COMPONENT_TYPE::UNSIGNED_SHORT: {
      size_of_component = sizeof(unsigned short);
      break;
    }
  case ACC_COMPONENT_TYPE::UNSIGNED_INT: {
      size_of_component = sizeof(unsigned int);
      break;
    }
  case ACC_COMPONENT_TYPE::FLOAT: {
      size_of_component = sizeof(float);
      break;
    }
  }

  int size_of_element = size_of_component * get_num_components_for_gltf_element(acc.element_type);
  int size_of_acc_data = size_of_element * acc.count;

  inu_assert(size_of_acc_data <= buffer_view.byte_length);
  char* data = (char*)malloc(size_of_acc_data);

  gltf_buffer_t& buffer = gltf_buffers[buffer_view.gltf_buffer_index];
  char buffer_uri_full_path[256]{};
  sprintf(buffer_uri_full_path, "%s\\%s", folder_path, buffer.uri.c_str());

  FILE* uri_file = fopen(buffer_uri_full_path, "rb");
  inu_assert(uri_file, "uri file does not exist");

  int start_offset = acc.byte_offset + buffer_view.byte_offset;

  int stride = -1;
  if (buffer_view.byte_stride != -1) {
    stride = buffer_view.byte_stride;
  } else {
    stride = size_of_element;
  }

  int count = 0;
  for (int i = 0; i < acc.count; i++) {
    int offset = start_offset + (i * stride);
    int err = fseek(uri_file, offset, SEEK_SET);
    for (int j = 0; j < size_of_element; j++) {
      char c = 0;
      size_t bytes_read = fread(&c, sizeof(char), 1, uri_file);
      data[count] = c;
      count++;
    }
  }

  inu_assert(count == size_of_acc_data, "size of actual data not equal to final count");
  
  fclose(uri_file);
  
  return (void*)data;
}

int gltf_read_texture(int gltf_tex_idx, int tex_slot) {
  inu_assert(gltf_tex_idx < gltf_textures.size());
  gltf_texture_t& gltf_tex = gltf_textures[gltf_tex_idx];
  gltf_image_t& img = gltf_images[gltf_tex.image_source_idx];
  std::string& img_file_name = img.uri;
  char img_full_path[256]{};
  sprintf(img_full_path, "%s\\%s", folder_path, img_file_name.c_str());
  return create_texture(img_full_path, tex_slot);
}

material_image_t gltf_mat_img_to_internal_mat_img(gltf_mat_image_info_t& gltf_mat_image_info, int tex_slot) {
  int mat_gltf_tex_idx = gltf_mat_image_info.gltf_texture_idx;
  int tex_handle = -1;
  if (mat_gltf_tex_idx != -1) {
    tex_handle = gltf_read_texture(mat_gltf_tex_idx, tex_slot);
  }
  material_image_t mat_img;
  mat_img.tex_handle = tex_handle;
  mat_img.tex_coords_idx = gltf_mat_image_info.tex_coord_idx;
  return mat_img;
}

void gltf_load_file(const char* filepath) {


  printf("loading gltf file: %s\n", filepath);

  const char* last_slash = strrchr(filepath, '\\');
  memset(folder_path, 0, 256);
  memcpy(folder_path, filepath, last_slash - filepath);

  // 0. RESET VALUES
  char* data = NULL;
  offset = -1;
  data_len = -1;

  active_scene = -1;
  gltf_scenes.clear(); 
  gltf_nodes.clear(); 
  gltf_meshes.clear();
  gltf_buffers.clear();
  gltf_buffer_views.clear();
  gltf_accessors.clear(); 
  gltf_images.clear();
  gltf_textures.clear();
  gltf_samplers.clear();
  gltf_materials.clear();
  gltf_animations.clear();
  gltf_skins.clear();

  gltf_mesh_id_to_internal_model_id.clear();


  // 1. PREPROCESS
  gltf_preprocess(filepath);

  // 2. PARSE FILE
  inu_assert('{' == gltf_peek(), "initial character is not opening curly brace");
  gltf_eat();

  while (gltf_peek() != '}') {
    gltf_parse_section();
  }

  free(data);
  data = NULL;

  // 3. LOAD INTO INTERNAL FORMAT/ LOAD RAW DATA
  for (gltf_material_t& mat : gltf_materials) {
    material_image_t base_color_img = gltf_mat_img_to_internal_mat_img(mat.pbr.base_color_tex_info, ALBEDO_IMG_TEX_SLOT);
#if SET_MESHES_TO_WHITE == 0
    create_material(mat.pbr.base_color_factor, base_color_img);
#else
    material_image_t d;
    create_material({1,1,1,1}, d);
#endif
  }
 
  // mesh processing
  int gltf_mesh_idx = 0;
  for (gltf_mesh_t& gltf_mesh : gltf_meshes) {
    model_t model;
    
    // each prim could have its own vao, vbo, and ebo
    for (gltf_primitive_t& prim : gltf_mesh.primitives) {
      inu_assert(prim.mode == GLTF_PRIMITIVE_MODE::TRIANGLES, "meshes right now can only be triangles");

      mesh_t mesh;

      int vert_count = -1;

      inu_assert(prim.attribs.positions_accessor_idx != -1, "mesh must specify positions");
      void* positions_data = gltf_read_accessor_data(prim.attribs.positions_accessor_idx);
      gltf_accessor_t& acc = gltf_accessors[prim.attribs.positions_accessor_idx];
      if (acc.component_type == ACC_COMPONENT_TYPE::FLOAT) {
        // can do direct static cast b/c vec3 is made up of floats
        vec3* pos_data = static_cast<vec3*>(positions_data);
        if (mesh.vertices.size() == 0) {
          mesh.vertices.resize(acc.count);
        }
        vert_count = acc.count;
        for (int i = 0; i < acc.count; i++) {
          vertex_t& vert = mesh.vertices[i];
          vert.position.x = pos_data[i].x;
          vert.position.y = pos_data[i].y;
          vert.position.z = pos_data[i].z;
        }
      } else {
        inu_assert_msg("this type for positions data is not supported yet");
      }
      free(positions_data);

      if (prim.indicies_accessor_idx != -1) {
        void* index_data = gltf_read_accessor_data(prim.indicies_accessor_idx);
        gltf_accessor_t& acc = gltf_accessors[prim.indicies_accessor_idx];
        if (acc.component_type == ACC_COMPONENT_TYPE::UNSIGNED_INT) {
          unsigned int* uint_data = static_cast<unsigned int*>(index_data);
          for (int i = 0; i < acc.count; i++) {
            mesh.indicies.push_back(uint_data[i]);
          }
        } else if (acc.component_type == ACC_COMPONENT_TYPE::UNSIGNED_SHORT) {
          unsigned short* ushort_data = static_cast<unsigned short*>(index_data);
          for (int i = 0; i < acc.count; i++) {
            unsigned int val = ushort_data[i];
            mesh.indicies.push_back(val);
          } 
        } else {
          inu_assert("indicies data type not supported");
        }
        free(index_data);
      } else {
        for (unsigned int i = 0; i < vert_count; i++) {
          mesh.indicies.push_back(i);
        }
      }

      if (prim.attribs.normals_accessor_idx != -1) {
        void* normals_data = gltf_read_accessor_data(prim.attribs.normals_accessor_idx);
        gltf_accessor_t& normals_acc = gltf_accessors[prim.attribs.normals_accessor_idx];
        inu_assert(normals_acc.component_type == ACC_COMPONENT_TYPE::FLOAT, "normals need to be of float type");
        inu_assert(normals_acc.element_type == ACC_ELEMENT_TYPE::VEC3, "normals need to be of vec3 type");
        // can do direct cast b/c vec3 is made of floats
        vec3* v_normals_data = static_cast<vec3*>(normals_data);
        for (int i = 0; i < acc.count; i++) {
          vertex_t& vert = mesh.vertices[i];
          vec3 n = v_normals_data[i];
          vert.normal = norm_vec3(n);
        }
        free(normals_data);
      } else {
        inu_assert_msg("this model does not have normals");
      }

      if (prim.attribs.color_0_accessor_idx != -1) {
        void* color_data = gltf_read_accessor_data(prim.attribs.color_0_accessor_idx);
        gltf_accessor_t& acc = gltf_accessors[prim.attribs.color_0_accessor_idx];
        if (acc.component_type == ACC_COMPONENT_TYPE::FLOAT) {
          // can do direct static cast b/c vec3 is made up of floats
          vec3* col_data = static_cast<vec3*>(color_data);
          inu_assert(acc.count == mesh.vertices.size(), "count of vertices for color data different from count of actual loading in vertices from position data");
          for (int i = 0; i < vert_count; i++) {
            vertex_t& vert = mesh.vertices[i];
            vert.color = col_data[i];
          }
        } else {
          printf("this type for colors data is not supported yet\n");
          for (int i = 0; i < vert_count; i++) {
            vertex_t& vert = mesh.vertices[i];
		        vert.color.x = 1;
		        vert.color.y = 1;
		        vert.color.z = 1;
          }
        }
        free(color_data);
      } else {
        for (int i = 0; i < vert_count; i++) {
          vertex_t& vert = mesh.vertices[i];
          vert.color.x = 1;
          vert.color.y = 1;
          vert.color.z = 1;
        }
      }
      
      for (int i = 0; i < MAX_SUPPORTED_TEX_COORDS; i++) {
        int accessor_idx = prim.attribs.tex_coord_accessor_indicies[i];
        if (accessor_idx == -1) continue;
        void* tex_data_void = gltf_read_accessor_data(accessor_idx);
        gltf_accessor_t& tex_acc = gltf_accessors[accessor_idx];
        if (tex_acc.component_type == ACC_COMPONENT_TYPE::FLOAT) {
          // can do direct static cast b/c vec2 is made up of floats
          vec2* tex_data = static_cast<vec2*>(tex_data_void);
          for (int j = 0; j < tex_acc.count; j++) {
            vertex_t& vert = mesh.vertices[j];
            vec2* tex_ptr = &vert.tex0;
            *(tex_ptr+i) = tex_data[j];
          }
        } else {
          inu_assert_msg("this type for texture data is not supported yet");
        }
        free(tex_data_void);
      }

      if (prim.attribs.joints_0_accessor_idx != -1) {
        gltf_accessor_t& joint_acc = gltf_accessors[prim.attribs.joints_0_accessor_idx];
        inu_assert(joint_acc.count == vert_count);
        inu_assert(joint_acc.element_type == ACC_ELEMENT_TYPE::VEC4);
        void* joint_data = gltf_read_accessor_data(prim.attribs.joints_0_accessor_idx);
        uint8_t* u8_joint_data = (uint8_t*)joint_data;
        uint16_t* u16_joint_data = (uint16_t*)joint_data;
        for (int j = 0; j < vert_count; j++) {
          vertex_t& vert = mesh.vertices[j];
          for (int k = 0; k < 4; k++) {
            int idx_into_joint_data = (j*4) + k;
            unsigned int joint_idx;
            if (joint_acc.component_type == ACC_COMPONENT_TYPE::UNSIGNED_BYTE) {
              joint_idx = static_cast<unsigned int>(u8_joint_data[idx_into_joint_data]);
            } else if (joint_acc.component_type == ACC_COMPONENT_TYPE::UNSIGNED_SHORT) {
              joint_idx = static_cast<unsigned int>(u16_joint_data[idx_into_joint_data]);
            } else {
              inu_assert_msg("invalid type for joint data");
            }
            inu_assert(joint_idx >= 0);
            vert.joints[k] = joint_idx;
          }
        }
        free(joint_data);
        inu_assert(prim.attribs.weights_0_accessor_idx != -1, "weights must be specified when joints are");
      }
      

#define idx_into_weights_data ((j*4)+k)
      if (prim.attribs.weights_0_accessor_idx != -1) {
        gltf_accessor_t& weights_acc = gltf_accessors[prim.attribs.weights_0_accessor_idx];
        inu_assert(weights_acc.count == vert_count);
        inu_assert(weights_acc.element_type == ACC_ELEMENT_TYPE::VEC4);
        inu_assert(
          weights_acc.component_type == ACC_COMPONENT_TYPE::FLOAT ||
          weights_acc.component_type == ACC_COMPONENT_TYPE::UNSIGNED_BYTE ||
          weights_acc.component_type == ACC_COMPONENT_TYPE::UNSIGNED_SHORT
        );
        void* weights_data = gltf_read_accessor_data(prim.attribs.weights_0_accessor_idx);
        float* f_weights_data = (float*)weights_data;
        uint8_t* u8_weights_data = (uint8_t*)weights_data;
        uint16_t* u16_weights_data = (uint16_t*)weights_data;
        for (int j = 0; j < vert_count; j++) {
          vertex_t& vert = mesh.vertices[j];
          if (weights_acc.component_type == ACC_COMPONENT_TYPE::FLOAT) {
            float f_sum = 0;
            for (int k = 0; k < 4; k++) {
              f_sum += f_weights_data[idx_into_weights_data];
            }
            inu_assert(f_sum >= 0.95f);
            for (int k = 0; k < 4; k++) {
              vert.weights[k] = f_weights_data[idx_into_weights_data];
            }
          } else if (weights_acc.component_type == ACC_COMPONENT_TYPE::UNSIGNED_BYTE) {
            uint8_t u8_sum = 0;
            for (int k = 0; k < 4; k++) {
              u8_sum += u8_weights_data[idx_into_weights_data];
            }
            inu_assert(u8_sum == 255);
            for (int k = 0; k < 4; k++) {
              vert.weights[k] = static_cast<float>(u8_weights_data[idx_into_weights_data]) / 255.f;
            }
          } else if (weights_acc.component_type == ACC_COMPONENT_TYPE::UNSIGNED_SHORT) {
            uint16_t u16_sum = 0;
            for (int k = 0; k < 4; k++) {
              u16_sum += u16_weights_data[idx_into_weights_data];
            }
            inu_assert(u16_sum == 65535);
            for (int k = 0; k < 4; k++) {
              vert.weights[k] = static_cast<float>(u16_weights_data[idx_into_weights_data]) / 65535.f;
            }
          }
#undef idx_into_weights_data

          std::unordered_set<unsigned int> seen;
          for (int k = 0; k < 4; k++) {
            if (vert.weights[k] > 0) {
              if (seen.find(vert.joints[k]) == seen.end()) {
                seen.insert(vert.joints[k]);
              } else {
                inu_assert_msg("same joint index has multiple non-zero weights");
              }
            }
          }

        }
      }

      if (prim.material_idx != -1) {
        mesh.mat_idx = prim.material_idx;
      } else {
        vec4 color;
#if 1
        color.x = 0;
        color.y = 0;
        color.z = 0;
        color.w = 1;
#else
        color.x = rand() / static_cast<float>(RAND_MAX);
        color.y = rand() / static_cast<float>(RAND_MAX);
        color.z = rand() / static_cast<float>(RAND_MAX);
        color.w = 1.f;
#endif

        material_image_t base_img;
        mesh.mat_idx = create_material(color, base_img);
      }

      mesh.vao = create_vao();
      mesh.vbo = create_vbo((void*)mesh.vertices.data(), mesh.vertices.size() * sizeof(vertex_t));
      mesh.ebo = create_ebo(mesh.indicies.data(), mesh.indicies.size() * sizeof(unsigned int));

      vao_enable_attribute(mesh.vao, mesh.vbo, 0, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, position));
      vao_enable_attribute(mesh.vao, mesh.vbo, 1, 2, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex0));
      vao_enable_attribute(mesh.vao, mesh.vbo, 2, 2, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex1));
#if 0
      vao_enable_attribute(mesh.vao, mesh.vbo, 3, 2, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex2));
      vao_enable_attribute(mesh.vao, mesh.vbo, 4, 2, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex3));
      vao_enable_attribute(mesh.vao, mesh.vbo, 5, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
      vao_enable_attribute(mesh.vao, mesh.vbo, 6, 4, GL_UNSIGNED_INT, sizeof(vertex_t), offsetof(vertex_t, joints));
      vao_enable_attribute(mesh.vao, mesh.vbo, 7, 4, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, weights));
#else
      vao_enable_attribute(mesh.vao, mesh.vbo, 3, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
      vao_enable_attribute(mesh.vao, mesh.vbo, 4, 4, GL_UNSIGNED_INT, sizeof(vertex_t), offsetof(vertex_t, joints));
      vao_enable_attribute(mesh.vao, mesh.vbo, 5, 4, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, weights));
      vao_enable_attribute(mesh.vao, mesh.vbo, 6, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, normal));
#endif
      vao_bind_ebo(mesh.vao, mesh.ebo);
      
      model.meshes.push_back(mesh);
    }

    int internal_model_id = register_model(model);
    gltf_mesh_id_to_internal_model_id[gltf_mesh_idx] = internal_model_id;
    gltf_mesh_idx++; 
  } 

  // 4. STORE OBJECTS IN INTERNAL NODE HIERARCHY
  int offset_gltf_node_to_internal_obj_id = -1;
  for (int i = 0; i < gltf_nodes.size(); i++) {
    gltf_node_t& node = gltf_nodes[i];

    transform_t t;
    t.pos.x = node.translation.x;
    t.pos.y = node.translation.y;
    t.pos.z = node.translation.z;

    std::vector<int>& root_nodes = gltf_scenes[active_scene].root_nodes;
#if 0
    if (std::find(root_nodes.begin(), root_nodes.end(), i) != root_nodes.end()) {
      t.pos.z -= 0.1;
    }
#endif

    t.scale.x = node.scale.x;
    t.scale.y = node.scale.y;
    t.scale.z = node.scale.z;

    t.rot.x = node.rot.x;
    t.rot.y = node.rot.y;
    t.rot.z = node.rot.z;
    t.rot.w = node.rot.w;

    int obj_id = create_object(t);
    attach_name_to_obj(obj_id, node.name);
    offset_gltf_node_to_internal_obj_id = obj_id - i;
    if (node.gltf_mesh_handle != -1) {
      attach_model_to_obj(obj_id, gltf_mesh_id_to_internal_model_id[node.gltf_mesh_handle]);
    }

    if (node.gltf_skin_idx != -1) {
      inu_assert(node.child_node_idxs.size() == 0, "currently do not support children for skinned objects");
    }

    for (int child_id : node.child_node_idxs) {
      attach_child_obj_to_obj(obj_id, child_id + offset_gltf_node_to_internal_obj_id);
    } 
    
  }

  // parse skins/skeletons
  int offset_gltf_skin_to_internal_skin_id = -1;
  for (int i = 0; i < gltf_skins.size(); i++) {
    gltf_skin_t& gltf_skin = gltf_skins[i];
    skin_t skin; 
    inu_assert(gltf_skin.joint_node_idxs.size() <= BONES_PER_SKIN_LIMIT, "too many bones for this skin");
    skin.num_bones = gltf_skin.joint_node_idxs.size();
    for (int j = 0; j < gltf_skin.joint_node_idxs.size(); j++) {
      int gltf_node_idx = gltf_skin.joint_node_idxs[j];
      skin.joint_obj_ids[j] = gltf_node_idx + offset_gltf_node_to_internal_obj_id;
    }
    skin.upper_most_joint_node_idx = gltf_skin.upper_most_joint_node_idx + offset_gltf_node_to_internal_obj_id;
    skin.name = gltf_skin.name;
    if (gltf_skin.inverse_bind_matrix_acc_idx != -1) {
      gltf_accessor_t& acc = gltf_accessors[gltf_skin.inverse_bind_matrix_acc_idx];
      inu_assert(acc.count >= gltf_skin.joint_node_idxs.size(), "there must be at least the same number of inverse bind matricies as joints in a skin");
      mat4* inverse_bind_mat_data = (mat4*)gltf_read_accessor_data(gltf_skin.inverse_bind_matrix_acc_idx);
      for (int j = 0; j < acc.count; j++) {
        mat4& mat = inverse_bind_mat_data[j];
        skin.inverse_bind_matricies[j] = mat;
      }
      free(inverse_bind_mat_data);
    }
    int skin_id = register_skin(skin);
    offset_gltf_skin_to_internal_skin_id = skin_id - i; 
    printf("skin %i has %i bones\n", skin_id, skin.num_bones);
  }

  // attach skins and skeletons to objects
  for (int i = 0; i < gltf_nodes.size(); i++) {
    gltf_node_t& node = gltf_nodes[i];
    if (node.gltf_skin_idx != -1) {
      int obj_id = i + offset_gltf_node_to_internal_obj_id;
      int skin_id = node.gltf_skin_idx + offset_gltf_skin_to_internal_skin_id;
      attach_skin_to_obj(obj_id, skin_id);
    }
  }

  // mark parent objects
  if (active_scene != -1) {
    for (int gltf_node_idx : gltf_scenes[active_scene].root_nodes) {
      set_obj_as_parent(gltf_node_idx + offset_gltf_node_to_internal_obj_id);
    }
  }

  populate_parent_field_of_nodes();

  // update object transforms
  update_obj_model_mats();

  // 5. ANIMATION PROCESSING
  for (gltf_animation_t& gltf_anim : gltf_animations) {
    animation_t anim;
    anim.name = gltf_anim.name;

    // process anim sampler data
    std::unordered_map<int,int> gltf_anim_sampler_id_to_internal_chunk_id;
    for (int j = 0; j < gltf_anim.anim_samplers.size(); j++) {
      gltf_anim_sampler_t& sampler = gltf_anim.anim_samplers[j];
      gltf_accessor_t& time_data_acc = gltf_accessors[sampler.input_accessor_idx];
      inu_assert(time_data_acc.component_type == ACC_COMPONENT_TYPE::FLOAT);
      inu_assert(time_data_acc.element_type == ACC_ELEMENT_TYPE::SCALAR);
      
      animation_data_chunk_t data_chunk;

      if (sampler.interpolation_mode == GLTF_INTERPOLATION_MODE::LINEAR) {
        data_chunk.interpolation_mode = ANIM_INTERPOLATION_MODE::LINEAR;
      } else if (sampler.interpolation_mode == GLTF_INTERPOLATION_MODE::STEP) {
        data_chunk.interpolation_mode = ANIM_INTERPOLATION_MODE::STEP;
      } else {
        inu_assert_msg("currently cannot support this gltf interpolation mode");
      }

      // read timestamp data
      float* timestamp_data = (float*)gltf_read_accessor_data(sampler.input_accessor_idx);
      data_chunk.num_timestamps = time_data_acc.count;
      for (int i = 0; i < data_chunk.num_timestamps; i++) {
        float timestamp = *(timestamp_data + i);
        data_chunk.timestamps.push_back(timestamp);
      }
      free(timestamp_data);

      // read keyframe data
      void* keyframe_data = gltf_read_accessor_data(sampler.output_accessor_idx);
      gltf_accessor_t& key_frame_acc = gltf_accessors[sampler.output_accessor_idx];
      if (key_frame_acc.component_type == ACC_COMPONENT_TYPE::FLOAT) {
        data_chunk.keyframe_data = (float*)keyframe_data;
      } else {
        // need to convert keyframe data into floats
        int num_components = key_frame_acc.count * get_num_components_for_gltf_element(key_frame_acc.element_type);
        float* float_keyframe_data = (float*)malloc(sizeof(float) * num_components);
        for (int c = 0; c < num_components; c++) {
          if (key_frame_acc.component_type == ACC_COMPONENT_TYPE::BYTE) {
            int8_t* byte_data = (int8_t*)keyframe_data;
            int8_t int_val = byte_data[c];
            float f = int_val / 127.0f;
            float_keyframe_data[c] = fmax(f, -1.0f);
          } else if (key_frame_acc.component_type == ACC_COMPONENT_TYPE::UNSIGNED_BYTE) {
            uint8_t* ubyte_data = (uint8_t*)keyframe_data;
            uint8_t uint = ubyte_data[c];
            float f = uint / 255.f;
            float_keyframe_data[c] = f;
          } else if (key_frame_acc.component_type == ACC_COMPONENT_TYPE::SHORT) {
            int16_t* short_data = (int16_t*)keyframe_data;
            int16_t s = short_data[c];
            float f = s / 32767.0f;
            float_keyframe_data[c] = fmax(f, -1.0f);
          } else if (key_frame_acc.component_type == ACC_COMPONENT_TYPE::BYTE) {
            uint16_t* ushort_data = (uint16_t*)keyframe_data;
            uint16_t ushort = ushort_data[c];
            float f = ushort / 65535.0f;
            float_keyframe_data[c] = f;
          } else {
            inu_assert_msg("this component type for animation data is not supported");
          }
        }
        data_chunk.keyframe_data = float_keyframe_data;
        free(keyframe_data);
      }

      int internal_chunk_id = register_anim_data_chunk(data_chunk);
      gltf_anim_sampler_id_to_internal_chunk_id[j] = internal_chunk_id;
      anim.data_chunk_ids.push_back(internal_chunk_id);
    }

    // process channel data
    for (int j = 0; j < gltf_anim.channels.size(); j++) {
      animation_chunk_data_ref_t ref; 

      gltf_channel_t& channel = gltf_anim.channels[j];
      ref.chunk_id = gltf_anim_sampler_id_to_internal_chunk_id[channel.gltf_anim_sampler_idx];
      gltf_channel_target_t& target = channel.target; 
      if (target.path == CHANNEL_TARGET_PATH::ROTATION) {
        ref.target = ANIM_TARGET_ON_NODE::ROTATION;
      } else if (target.path == CHANNEL_TARGET_PATH::SCALE) {
        ref.target = ANIM_TARGET_ON_NODE::SCALE;
      } else if (target.path == CHANNEL_TARGET_PATH::TRANSLATION) {
        ref.target = ANIM_TARGET_ON_NODE::POSITION;
      }

      int obj_id = offset_gltf_node_to_internal_obj_id + target.gltf_node_idx;
      attach_anim_chunk_ref_to_obj(obj_id, ref) ;
    }

    register_animation(anim);

  }

  printf("finished gltf file: %s\n", filepath);

}
