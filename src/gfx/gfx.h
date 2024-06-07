#pragma once

#include <string>

#include "utils/mats.h"
#include "utils/vectors.h"

#define ALBEDO_IMG_TEX_SLOT 0
#define METAL_ROUGH_IMG_TEX_SLOT 1
#define NORMALS_IMG_TEX_SLOT 2
#define OCC_IMG_TEX_SLOT 3
#define EMISSION_IMG_TEX_SLOT 4
#define LIGHT0_SHADOW_MAP_TEX 5
#define LIGHT1_SHADOW_MAP_TEX 6
#define LIGHT2_SHADOW_MAP_TEX 7
#define DIR_LIGHT_SHADOW_MAP_TEX 8

#define USING_OPENGL 1

typedef int tex_id_t;
typedef int fb_id_t;
typedef int ebo_id_t;
typedef int vbo_id_t;
typedef int vao_id_t;

typedef int shader_id_t;

struct ebo_t {
	ebo_id_t id = 0;
	int num_indicies = -1;
};
ebo_t create_ebo(const unsigned int* indicies, const int size_of_buffer);
void draw_ebo(const ebo_t& ebo);
void bind_ebo(const ebo_t& ebo);
void unbind_ebo();
void delete_ebo(const ebo_t& ebo);

struct vbo_t {
	vbo_id_t id = 0;
};
vbo_t create_vbo(const void* vertices, const int data_size);
vbo_t create_dyn_vbo(const int data_size);
void bind_vbo(const vbo_t& vbo);
void update_vbo_data(const vbo_t& vbo, const void* vertices, const int data_size);
void unbind_vbo();
void delete_vbo(const vbo_t& vbo);

struct vao_t {
	vao_id_t id = 0;
};

#define USE_DT_ENUM 1

enum class VAO_ATTR_DATA_TYPE {
	FLOAT,
	UNSIGNED_INT
};

vao_t create_vao();
void bind_vao(const vao_t& vao);
void unbind_vao();
#if USE_DT_ENUM
void vao_enable_attribute(vao_t& vao, const vbo_t& vbo, const int attr_id, const int num_values, const VAO_ATTR_DATA_TYPE d_type, const int stride, const int offset);
#else
void vao_enable_attribute(vao_t& vao, const vbo_t& vbo, const int attr_id, const int num_values, const int d_type, const int stride, const int offset);
#endif
void vao_bind_ebo(vao_t& vao, ebo_t& ebo);
void delete_vao(const vao_t& vao);

struct shader_t {
	shader_id_t id = 0;
	std::string vert_name;
	std::string geom_name;
	std::string frag_name;
};
shader_t create_shader(const char* vert_source_path, const char* frag_source_path);
shader_t create_shader(const char* vert_source_path, const char* geom_source_path, const char* frag_source_path);
void bind_shader(shader_t& shader);
void unbind_shader();
void shader_set_float(shader_t& shader, const char* var_name, float val);
void shader_set_vec3(shader_t& shader, const char* var_name, vec3 vec);
void shader_set_int(shader_t& shader, const char* var_name, int val);
void shader_set_mat4(shader_t& shader, const char* var_name, mat4& mat);

enum class TEX_FILTER_METHOD {
	LINEAR = 0,
	NEAREST
};

enum class TEX_FORMAT {
	SINGLE,
	RG,
	RGB,
	RGBA,
	DEPTH_STENCIL,
	DEPTH
};

enum class TEX_DATA_TYPE {
	UNSIGNED_BYTE,
	DEPTH_STENCIL,
	FLOAT
};

enum class WRAP_MODE {
	REPEAT,
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER
};

enum class TEX_TYPE {
	TEXTURE_2D,
	TEXTURE_2D_ARRAY
};

struct tex_creation_meta_t {
	TEX_TYPE tex_type = TEX_TYPE::TEXTURE_2D;
	TEX_FILTER_METHOD min_filter = TEX_FILTER_METHOD::NEAREST;
	TEX_FILTER_METHOD mag_filter = TEX_FILTER_METHOD::NEAREST;
	TEX_FORMAT input_data_tex_format = TEX_FORMAT::RGB;
	TEX_FORMAT tex_format = TEX_FORMAT::RGB;
	WRAP_MODE s_wrap_mode = WRAP_MODE::REPEAT;
	WRAP_MODE t_wrap_mode = WRAP_MODE::REPEAT;
	TEX_DATA_TYPE data_type = TEX_DATA_TYPE::UNSIGNED_BYTE;
};


#if USING_OPENGL
struct gl_tex_creation_meta_t;
#endif

struct texture_t {
	tex_id_t id = -1;
	int tex_slot = 0;	
	int width = -1;
	int height = -1;
	int depth = 1;
	int num_channels = -1;

	tex_creation_meta_t tex_creation_meta;

#if USING_OPENGL
	gl_tex_creation_meta_t* gl_tex_creation_meta;
#endif
};

struct file_texture_t {
	tex_id_t id;
	std::string path;
};

tex_id_t create_texture(unsigned char* data, int tex_slot, int width, int height, int depth, tex_creation_meta_t& meta_data);
file_texture_t create_file_texture(const char* img_path, int tex_slot, tex_creation_meta_t& meta_data);
file_texture_t create_file_texture(const char* img_path, int tex_slot);

const texture_t bind_texture(tex_id_t tex_id);
const texture_t bind_texture(tex_id_t tex_id, int override_slot);
void unbind_texture();

enum class MATERIAL_PARAM_VARIANT {
	NONE,
	FLOAT,
	VEC3,
	VEC4,
	MAT_IMG
};

struct material_image_t {
	// the internal texture handle
	tex_id_t tex_handle = -1;
	// which texture coordinatest to use for this texture
	int tex_coords_idx = 0;
};

struct emission_param_t {
	vec3 emission_factor;
	material_image_t emissive_tex_info;
	MATERIAL_PARAM_VARIANT variant = MATERIAL_PARAM_VARIANT::MAT_IMG;
};

struct metallic_roughness_param_t {

	// float variant
	struct {
		float metallic_factor;
  	float roughness_factor;	
	};

	// image variant
	material_image_t met_rough_tex;

	MATERIAL_PARAM_VARIANT variant = MATERIAL_PARAM_VARIANT::FLOAT;

	metallic_roughness_param_t();
};

struct albedo_param_t {	
	// vec4 variant
	vec4 base_color;

	// mat image variant
	struct {
		vec4 multipliers;
		material_image_t base_color_img;
	};

	MATERIAL_PARAM_VARIANT variant = MATERIAL_PARAM_VARIANT::VEC4;
	albedo_param_t();
};

struct normals_param_t {
	material_image_t normal_map;
	// the VEC3 variant will mean to use vertex normals
	MATERIAL_PARAM_VARIANT variant = MATERIAL_PARAM_VARIANT::MAT_IMG;
};

struct occ_param_t {
	material_image_t occ_map;
	// the NONE variant will mean to not use occlusion mapping
	// the MAT_IMG variant will mean to use occlusion mapping
	MATERIAL_PARAM_VARIANT variant = MATERIAL_PARAM_VARIANT::MAT_IMG;
};

struct material_t {
	static shader_t associated_shader;
	std::string name;
	albedo_param_t albedo;
	metallic_roughness_param_t metal_rough;

	emission_param_t emission;
	normals_param_t normals;
	occ_param_t occ;

#if 0
	// will be important later
	material_image_t occlusion_tex;
#endif

	material_t();
};
// int create_material(vec4 color, material_image_t base_color_img);
int create_material(std::string& mat_name, albedo_param_t& albedo_param, metallic_roughness_param_t& base_color_img);
int create_material(std::string& mat_name, albedo_param_t& albedo_param, metallic_roughness_param_t& base_color_img, emission_param_t& emission_param, normals_param_t& normals, occ_param_t& occ);
material_t bind_material(int mat_idx);
material_t get_material(int mat_idx);
int get_num_materials();

enum class FB_TYPE {
	RENDER_BUFFER_DEPTH_STENCIL = 0,
	TEXTURE_DEPTH_STENCIL,
	// currently 3 depth textures deep
	MULTIPLE_DEPTH_TEXTURE,
	NO_COLOR_ATT_MULTIPLE_DEPTH_TEXTURE,
};

struct framebuffer_t {
	fb_id_t id = -1;

	FB_TYPE fb_type = FB_TYPE::RENDER_BUFFER_DEPTH_STENCIL;
	tex_id_t color_att = -1;
	tex_id_t depth_att = -1;

	int width = -1;
	int height = -1;
};
framebuffer_t create_framebuffer(int width, int height, FB_TYPE fb_type);
void bind_framebuffer(framebuffer_t& fb);
void clear_framebuffer();
void clear_framebuffer_depth();
void unbind_framebuffer();

void get_gfx_error();
