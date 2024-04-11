#pragma once

#include <string>

#include "glew.h"

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

struct ebo_t {
	GLuint id = 0;
	int num_indicies = -1;
};
ebo_t create_ebo(const unsigned int* indicies, const int size_of_buffer);
void draw_ebo(const ebo_t& ebo);
void bind_ebo(const ebo_t& ebo);
void unbind_ebo();
void delete_ebo(const ebo_t& ebo);

struct vbo_t {
	GLuint id = 0;
};
vbo_t create_vbo(const void* vertices, const int data_size);
vbo_t create_dyn_vbo(const int data_size);
void bind_vbo(const vbo_t& vbo);
void update_vbo_data(const vbo_t& vbo, const void* vertices, const int data_size);
void unbind_vbo();
void delete_vbo(const vbo_t& vbo);

struct vao_t {
	GLuint id = 0;
};
vao_t create_vao();
void bind_vao(const vao_t& vao);
void unbind_vao();
void vao_enable_attribute(vao_t& vao, const vbo_t& vbo, const int attr_id, const int num_values, const int d_type, const int stride, const int offset);
void vao_bind_ebo(vao_t& vao, ebo_t& ebo);
void delete_vao(const vao_t& vao);

struct shader_t {
	GLuint id = 0;
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

struct texture_t {
	int id = -1;
	GLuint gl_id = -1;
	int tex_slot = 0;	
	int width = -1;
	int height = -1;
	int num_channels = -1;
	std::string path;
};
int create_texture(const char* img_path, int tex_slot);
texture_t bind_texture(int tex_id);
void unbind_texture();

enum class MATERIAL_PARAM_VARIANT {
	FLOAT,
	VEC3,
	VEC4,
	MAT_IMG
};

struct material_image_t {
	// the internal texture handle
	int tex_handle = -1;
	// which texture coordinatest to use for this texture
	int tex_coords_idx = 0;
};

struct emission_param_t {
	union {
		vec3 emission_factor = {1,1,1};
		material_image_t emissive_tex_info;
	};
	MATERIAL_PARAM_VARIANT variant = MATERIAL_PARAM_VARIANT::VEC3;
};

struct metallic_roughness_param_t {
	union {
		struct {
			float metallic_factor;
  		float roughness_factor;	
		};
		material_image_t met_rough_tex;
	};
	MATERIAL_PARAM_VARIANT variant = MATERIAL_PARAM_VARIANT::FLOAT;
	metallic_roughness_param_t();
};

struct albedo_param_t {	
	union {
		vec4 color_factor;
		struct {
			vec4 multipliers;
			material_image_t base_color_img;
		};
	};
	MATERIAL_PARAM_VARIANT variant = MATERIAL_PARAM_VARIANT::VEC4;
	albedo_param_t();
};

struct material_t {
	static shader_t associated_shader;
	albedo_param_t albedo;
	metallic_roughness_param_t metal_rough;
	material_image_t normals_tex;
	material_image_t occlusion_tex;
	emission_param_t emission;

	material_t();
};
int create_material(vec4 color, material_image_t base_color_img);
material_t bind_material(int mat_idx);
material_t get_material(int mat_idx);

enum class FB_TYPE {
	RENDER_BUFFER_DEPTH_STENCIL = 0,
	TEXTURE_DEPTH_STENCIL,
	// currently 3 depth textures deep
	MULTIPLE_DEPTH_TEXTURE,
	NO_COLOR_ATT_MULTIPLE_DEPTH_TEXTURE,
};

struct framebuffer_t {
	GLuint id = -1;

	FB_TYPE fb_type = FB_TYPE::RENDER_BUFFER_DEPTH_STENCIL;
	GLuint color_att = -1;
	GLuint depth_att = -1;

	int width = -1;
	int height = -1;
};
framebuffer_t create_framebuffer(int width, int height, FB_TYPE fb_type);
void bind_framebuffer(framebuffer_t& fb);
void clear_framebuffer(framebuffer_t& fb);
void unbind_framebuffer();

void get_gfx_error();
