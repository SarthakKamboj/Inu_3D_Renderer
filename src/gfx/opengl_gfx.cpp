#include "gfx.h"

#include <stddef.h>
#include <stdlib.h>
#include <string>
#include <stdexcept>
#include <vector>
#include <unordered_map>

#include "model_loading/image/stb_image.h"
#include "glew.h"

#include "opengl_gfx_helper.h"
#include "utils/general.h"
#include "utils/log.h"
#include "windowing/window.h"
#include "light.h"

static std::unordered_map<tex_id_t, GLuint> tex_id_to_gl_id;
static std::unordered_map<fb_id_t, GLuint> fb_id_to_gl_id;
static std::unordered_map<ebo_id_t, GLuint> ebo_id_to_gl_id;
static std::unordered_map<vbo_id_t, GLuint> vbo_id_to_gl_id;
static std::unordered_map<vao_id_t, GLuint> vao_id_to_gl_id;
static std::unordered_map<shader_id_t, GLuint> shader_id_to_gl_id;

static fb_id_t internal_fb_running_id = 1;
static tex_id_t internal_tex_running_id = 1;
static ebo_id_t internal_ebo_running_id = 1;
static vbo_id_t internal_vbo_running_id = 1;
static vao_id_t internal_vao_running_id = 1;
static shader_id_t internal_shader_running_id = 1;

static std::vector<texture_t> textures;
static std::vector<file_texture_t> file_textures;

std::vector<material_t> materials;
extern window_t window;

GLuint get_internal_tex_gluint(tex_id_t id);

// VBO
vbo_t create_vbo(const void* vertices, const int data_size) {
	vbo_t vbo;
	vbo.id = internal_vbo_running_id;
	internal_vbo_running_id++;
	GLuint gl_vbo_id;
	glGenBuffers(1, &gl_vbo_id);
	vbo_id_to_gl_id[vbo.id] = gl_vbo_id;
	glBindBuffer(GL_ARRAY_BUFFER, gl_vbo_id);
	glBufferData(GL_ARRAY_BUFFER, data_size, vertices, GL_STATIC_DRAW);
	return vbo;
}

vbo_t create_dyn_vbo(const int data_size) {
	vbo_t vbo;
	vbo.id = internal_vbo_running_id;
	internal_vbo_running_id++;
	GLuint gl_vbo_id;
	glGenBuffers(1, &gl_vbo_id);
	vbo_id_to_gl_id[vbo.id] = gl_vbo_id;
	glBindBuffer(GL_ARRAY_BUFFER, gl_vbo_id);
	glBufferData(GL_ARRAY_BUFFER, data_size, NULL, GL_DYNAMIC_DRAW);
	return vbo;
}

void update_vbo_data(const vbo_t& vbo, const void* vertices, const int data_size) {
	GLuint gl_vbo_id = vbo_id_to_gl_id[vbo.id];
	glBindBuffer(GL_ARRAY_BUFFER, gl_vbo_id);
	glBufferSubData(GL_ARRAY_BUFFER, 0, data_size, vertices);
}

void bind_vbo(const vbo_t& vbo) {
	GLuint gl_vbo_id = vbo_id_to_gl_id[vbo.id];
	glBindBuffer(GL_ARRAY_BUFFER, gl_vbo_id);
}

void unbind_vbo() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void delete_vbo(const vbo_t& vbo) {
	GLuint gl_vbo_id = vbo_id_to_gl_id[vbo.id];
	glDeleteBuffers(1, &gl_vbo_id);
}

// EBO
ebo_t create_ebo(const unsigned int* indicies, const int size_of_buffer) {
	ebo_t ebo;
	ebo.id = internal_ebo_running_id;
	internal_ebo_running_id++;
	ebo.num_indicies = size_of_buffer / sizeof(indicies[0]);
	GLuint gl_ebo_id;
	glGenBuffers(1, &gl_ebo_id);
	ebo_id_to_gl_id[ebo.id] = gl_ebo_id;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_ebo_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_of_buffer, indicies, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return ebo;
}

void draw_ebo(const ebo_t& ebo) {
	glDrawElements(GL_TRIANGLES, ebo.num_indicies, GL_UNSIGNED_INT, 0);
}

void bind_ebo(const ebo_t& ebo) {
	GLuint gl_ebo_id = ebo_id_to_gl_id[ebo.id];
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_ebo_id);
}

void unbind_ebo() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void delete_ebo(const ebo_t& ebo) {
	GLuint gl_ebo_id = ebo_id_to_gl_id[ebo.id];
	glDeleteBuffers(1, &gl_ebo_id);
}

// VAO
vao_t create_vao() {
	vao_t vao;
	vao.id = internal_vao_running_id;
	internal_vao_running_id++;
	GLuint gl_vao_id;
	glGenVertexArrays(1, &gl_vao_id);
	vao_id_to_gl_id[vao.id] = gl_vao_id;
	return vao;
}

void bind_vao(const vao_t& vao) {
	GLuint gl_vao_id = vao_id_to_gl_id[vao.id];
	glBindVertexArray(gl_vao_id);
}

void unbind_vao() {
	glBindVertexArray(0);
}

// void vao_enable_attribute(vao_t& vao, const vbo_t& vbo, const int attr_id, const int num_values, const int d_type, const int stride, const int offset) {
void vao_enable_attribute(vao_t& vao, const vbo_t& vbo, const int attr_id, const int num_values, const VAO_ATTR_DATA_TYPE d_type, const int stride, const int offset) {
	bind_vao(vao);
	bind_vbo(vbo);
	GLenum gl_d_type = internal_to_gl_vao_data_type(d_type);
	glVertexAttribPointer(attr_id, num_values, gl_d_type, GL_FALSE, stride, reinterpret_cast<void*>(offset));
	glEnableVertexAttribArray(attr_id);
	unbind_vbo();
	unbind_vao();
}

void vao_bind_ebo(vao_t& vao, ebo_t& ebo) {
	bind_vao(vao);
	bind_ebo(ebo);
	unbind_vao();
	unbind_ebo();
}

void delete_vao(const vao_t& vao) {
	GLuint gl_vao_id = vao_id_to_gl_id[vao.id];
	glDeleteVertexArrays(1, &gl_vao_id);
}

// Shaders
shader_t create_shader(const char* vert_source_path, const char* frag_source_path) {
	printf("new shader\n\n");
	shader_t shader;

	shader.id = internal_shader_running_id;
	internal_shader_running_id++;

	GLuint gl_shader_id = glCreateProgram();
	shader_id_to_gl_id[shader.id] = gl_shader_id;

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	char* vert_source = get_file_contents(vert_source_path);
	glShaderSource(vert, 1, &vert_source, NULL);

	int success;
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char info_log[512]{};
		glGetShaderInfoLog(vert, 512, NULL, info_log);
		printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED %s\n", info_log);
		throw std::runtime_error("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" + std::string(info_log));
	}
	get_gfx_error();

	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	char* frag_source = get_file_contents(frag_source_path);
	glShaderSource(frag, 1, &frag_source, NULL);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char info_log[512]{};
		glGetShaderInfoLog(frag, 512, NULL, info_log);
		printf("ERROR::SHADER::FRAG::COMPILATION_FAILED %s\n", info_log);
		throw std::runtime_error("ERROR::SHADER::FRAG::COMPILATION_FAILED\n" + std::string(info_log));
	}
	get_gfx_error();

	glAttachShader(gl_shader_id, vert);
	get_gfx_error();
	glAttachShader(gl_shader_id, frag);
	get_gfx_error();

	glLinkProgram(gl_shader_id);

	get_gfx_error();

	glDeleteShader(vert);
	glDeleteShader(frag);

	free(vert_source);
	free(frag_source);

	shader.vert_name = vert_source_path;
	shader.frag_name = frag_source_path;

	return shader;
}

shader_t create_shader(const char* vert_source_path, const char* geom_source_path, const char* frag_source_path) {
	shader_t shader;

	shader.id = internal_shader_running_id;
	internal_shader_running_id++;

	GLuint gl_shader_id = glCreateProgram();
	shader_id_to_gl_id[shader.id] = gl_shader_id;

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	char* vert_source = get_file_contents(vert_source_path);
	glShaderSource(vert, 1, &vert_source, NULL);

	int success;
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char info_log[512]{};
		glGetShaderInfoLog(vert, 512, NULL, info_log);
		printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED %s\n", info_log);
		throw std::runtime_error("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" + std::string(info_log));
	}

	GLuint geom = glCreateShader(GL_GEOMETRY_SHADER);
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
    	// Log or print `err`.
		printf("error\n");
	}
	char* geom_source = get_file_contents(geom_source_path);
	glShaderSource(geom, 1, &geom_source, NULL);
	err = glGetError();
	if (err != GL_NO_ERROR) {
    	// Log or print `err`.
		printf("error\n");
	}

	glCompileShader(geom);
	glGetShaderiv(geom, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
    		// Log or print `err`.
		}

		GLint error_len = 0;
		glGetShaderiv(geom, GL_INFO_LOG_LENGTH, &error_len);
		GLchar* info_log = (GLchar*)malloc(sizeof(GLchar) * error_len);
		glGetShaderInfoLog(geom, error_len, &error_len, &info_log[0]);
		printf("ERROR::SHADER::GEOM::COMPILATION_FAILED %s\n", info_log);
		throw std::runtime_error("ERROR::SHADER::GEOM::COMPILATION_FAILED\n" + std::string(info_log));
	}

	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	char* frag_source = get_file_contents(frag_source_path);
	glShaderSource(frag, 1, &frag_source, NULL);
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char info_log[512]{};
		glGetShaderInfoLog(frag, 512, NULL, info_log);
		printf("ERROR::SHADER::FRAG::COMPILATION_FAILED %s\n", info_log);
		throw std::runtime_error("ERROR::SHADER::FRAG::COMPILATION_FAILED\n" + std::string(info_log));
	}

	glAttachShader(gl_shader_id, vert);
	glAttachShader(gl_shader_id, geom);
	glAttachShader(gl_shader_id, frag);
	glLinkProgram(gl_shader_id);

	glDeleteShader(vert);
	glDeleteShader(geom);
	glDeleteShader(frag);

	free(vert_source);
	free(geom_source);
	free(frag_source);

	shader.vert_name = vert_source_path;
	shader.geom_name = geom_source_path;
	shader.frag_name = frag_source_path;

	return shader;
}

void bind_shader(shader_t& shader) {
	GLuint gl_id = shader_id_to_gl_id[shader.id];
	glUseProgram(gl_id);
}

void unbind_shader() {
	glUseProgram(0);
}

void shader_set_mat4(shader_t& shader, const char* var_name, mat4& mat) {
	GLuint gl_id = shader_id_to_gl_id[shader.id];
	glUseProgram(gl_id);
	GLint loc = glGetUniformLocation(gl_id, var_name);
  if (loc == -1) {
      // printf("%s does not exist in shader %i\n", var_name, gl_id);
  }
	glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&mat.cols[0].x);
	unbind_shader();
}

void shader_set_int(shader_t& shader, const char* var_name, int val) {
	GLuint gl_id = shader_id_to_gl_id[shader.id];
	glUseProgram(gl_id);
	GLint loc = glGetUniformLocation(gl_id, var_name);
  if (loc == -1) {
      // printf("%s does not exist in shader %i\n", var_name, gl_id);
  }
	glUniform1i(loc, val);
	unbind_shader();
}

void shader_set_float(shader_t& shader, const char* var_name, float val) {
	GLuint gl_id = shader_id_to_gl_id[shader.id];
	glUseProgram(gl_id);
	GLint loc = glGetUniformLocation(gl_id, var_name);
    if (loc == -1) {
        // printf("%s does not exist in shader %i\n", var_name, gl_id);
    }
	glUniform1f(loc, val);
	unbind_shader();
}

void shader_set_vec3(shader_t& shader, const char* var_name, vec3 vec) {
	GLuint gl_id = shader_id_to_gl_id[shader.id];
	glUseProgram(gl_id);
	GLint loc = glGetUniformLocation(gl_id, var_name);
  if (loc == -1) {
      // printf("%s does not exist in shader %i\n", var_name, gl_id);
  }
	glUniform3fv(loc, 1, (GLfloat*)&vec);
	unbind_shader();
}


void shader_set_vec4(shader_t& shader, const char* var_name, vec4 vec) {
	GLuint gl_id = shader_id_to_gl_id[shader.id];
	glUseProgram(gl_id);
	GLint loc = glGetUniformLocation(gl_id, var_name);
  if (loc == -1) {
      // printf("%s does not exist in shader %i\n", var_name, gl_id);
  }
	glUniform4fv(loc, 1, (GLfloat*)&vec);
	unbind_shader();
}

// TEXTURES

file_texture_t create_file_texture(const char* img_path, int tex_slot) {
	tex_creation_meta_t meta;
	return create_file_texture(img_path, tex_slot, meta);
}

file_texture_t create_file_texture(const char* img_path, int tex_slot, tex_creation_meta_t& meta_data) {
	for (file_texture_t& ft : file_textures) {
		if (strcmp(img_path, ft.path.c_str()) == 0) {
			texture_t& tex = textures[ft.id-1];
			if (tex.tex_slot == tex_slot) {
				return ft;
			} else {
				file_texture_t new_ft;
				new_ft.id = create_texture_from_another_texture(ft.id, tex_slot, meta_data);
				new_ft.path = std::string(img_path);
				file_textures.push_back(new_ft);
				return new_ft;
			}
		}
	}

	file_texture_t ft;

	int width = -1;
	int height = -1;
	int num_channels = -1;

	stbi_set_flip_vertically_on_load(false);
	unsigned char* data = stbi_load(img_path, &width, &height, &num_channels, 0);
	inu_assert(data, "image data not loaded");

	if (num_channels == 1) {
		meta_data.input_data_tex_format = TEX_FORMAT::SINGLE;
		meta_data.tex_format = TEX_FORMAT::SINGLE;
	} else if (num_channels == 2) {
		meta_data.input_data_tex_format = TEX_FORMAT::RG;
		meta_data.tex_format = TEX_FORMAT::RG;
	} else if (num_channels == 3) {
		meta_data.input_data_tex_format = TEX_FORMAT::RGB;
		meta_data.tex_format = TEX_FORMAT::RGB;
	} else if (num_channels == 4) {
		meta_data.input_data_tex_format = TEX_FORMAT::RGBA;
		meta_data.tex_format = TEX_FORMAT::RGBA;
	} else {
		stbi_image_free(data);
		ft.id = -1;
		return ft;
	}

	ft.id = create_texture(data, tex_slot, width, height, 1, meta_data);
	ft.path = std::string(img_path);

	stbi_image_free(data);

	file_textures.push_back(ft);

	return ft;
}

tex_id_t create_texture(unsigned char* data, int tex_slot, int width, int height, int depth, tex_creation_meta_t& meta_data) {

	gl_tex_creation_meta_t gl_meta = internal_to_gl_tex_meta(meta_data);

	texture_t texture;

	texture.id = internal_tex_running_id;
	internal_tex_running_id++;
	
	GLuint gl_tex_id;
	glGenTextures(1, &gl_tex_id);
	tex_id_to_gl_id[texture.id] = gl_tex_id;
	glBindTexture(gl_meta.tex_target, gl_tex_id);

	texture.width = width;
	texture.height = height;
	texture.depth = depth;
	texture.tex_slot = tex_slot;	

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (gl_meta.tex_target == GL_TEXTURE_2D) {
		glTexImage2D(gl_meta.tex_target, 0, gl_meta.tex_format, width, height, 0, gl_meta.input_data_tex_format, gl_meta.data_type, data);
	} else if (gl_meta.tex_target == GL_TEXTURE_2D_ARRAY) {
		glTexImage3D(gl_meta.tex_target, 0, gl_meta.tex_format, width, height, depth, 0, gl_meta.input_data_tex_format, gl_meta.data_type, data);
	}

	glTexParameteri(gl_meta.tex_target, GL_TEXTURE_MAG_FILTER, gl_meta.mag_filter);
  glTexParameteri(gl_meta.tex_target, GL_TEXTURE_MIN_FILTER, gl_meta.min_filter);
  glTexParameteri(gl_meta.tex_target, GL_TEXTURE_WRAP_S, gl_meta.s_wrap_mode);
  glTexParameteri(gl_meta.tex_target, GL_TEXTURE_WRAP_T, gl_meta.t_wrap_mode);

	constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(gl_meta.tex_target, GL_TEXTURE_BORDER_COLOR, bordercolor);

	if (gl_meta.tex_target == GL_TEXTURE_2D) {
		glGenerateMipmap(gl_meta.tex_target);
	}

	glBindTexture(gl_meta.tex_target, 0);

	texture.tex_creation_meta = meta_data;
	texture.gl_tex_creation_meta = static_cast<gl_tex_creation_meta_t*>(malloc(sizeof(gl_tex_creation_meta_t)));
	*texture.gl_tex_creation_meta = gl_meta;

	textures.push_back(texture);
	return texture.id;
}

tex_id_t create_texture_from_another_texture(tex_id_t other, int tex_slot, tex_creation_meta_t& meta_data) {

	texture_t texture;
	texture.id = internal_tex_running_id;
	internal_tex_running_id++;

	texture_t& other_tex = textures[other-1];

	texture.tex_slot = tex_slot;
	texture.width = other_tex.width;
	texture.height = other_tex.height;
	texture.depth = other_tex.depth;
	texture.num_channels = other_tex.num_channels;

	gl_tex_creation_meta_t gl_meta = internal_to_gl_tex_meta(meta_data);
	texture.tex_creation_meta = meta_data;
	texture.gl_tex_creation_meta = static_cast<gl_tex_creation_meta_t*>(malloc(sizeof(gl_tex_creation_meta_t)));
	*texture.gl_tex_creation_meta = gl_meta;

	textures.push_back(texture);
	return texture.id;
}

GLuint get_internal_tex_gluint(tex_id_t id) {
	return tex_id_to_gl_id[id];
}

const texture_t bind_texture(tex_id_t tex_id) {
	texture_t& tex = textures[tex_id-1];
	glActiveTexture(GL_TEXTURE0 + tex.tex_slot);
	GLuint gl_id = tex_id_to_gl_id[tex_id];
	glBindTexture(tex.gl_tex_creation_meta->tex_target, gl_id);
	return tex;
}

const texture_t bind_texture(tex_id_t tex_id, int override_slot) {
	texture_t& tex = textures[tex_id-1];
	glActiveTexture(GL_TEXTURE0 + override_slot);
	GLuint gl_id = tex_id_to_gl_id[tex_id];
	glBindTexture(tex.gl_tex_creation_meta->tex_target, gl_id);
	return tex;
}

void unbind_texture() {
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void unbind_texture(int slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

// MATERIALS
shader_t material_t::associated_shader;

metallic_roughness_param_t::metallic_roughness_param_t() {
	metallic_factor = 0.0f;
	roughness_factor = 1.0f;
}

albedo_param_t::albedo_param_t() {}

material_t::material_t() {}

int get_num_materials() {
	return materials.size();
}

int create_material(std::string& mat_name, albedo_param_t& albedo_param, metallic_roughness_param_t& met_rough_param) {
	material_t mat;

	inu_assert(albedo_param.variant == MATERIAL_PARAM_VARIANT::VEC4 || albedo_param.variant == MATERIAL_PARAM_VARIANT::MAT_IMG, "albedo only has types of mat img and vec4");
	mat.albedo = albedo_param;

	inu_assert(met_rough_param.variant == MATERIAL_PARAM_VARIANT::FLOAT || met_rough_param.variant == MATERIAL_PARAM_VARIANT::MAT_IMG, "metal roughness only has types of mat img and float");
	mat.metal_rough = met_rough_param;

	mat.name = mat_name;

	materials.push_back(mat);

	return materials.size()-1;
}

int create_material(std::string& mat_name, albedo_param_t& albedo_param, metallic_roughness_param_t& met_rough_param, emission_param_t& emission_param, normals_param_t& normals_param, occ_param_t& occ_param, TRANSPARENCY_MODE transparency_mode, bool cull_back) {
	material_t mat;

	inu_assert(albedo_param.variant == MATERIAL_PARAM_VARIANT::VEC4 || albedo_param.variant == MATERIAL_PARAM_VARIANT::MAT_IMG, "albedo only has types of mat img and vec4");
	mat.albedo = albedo_param;

	inu_assert(met_rough_param.variant == MATERIAL_PARAM_VARIANT::FLOAT || met_rough_param.variant == MATERIAL_PARAM_VARIANT::MAT_IMG, "metal roughness only has types of mat img and float");
	mat.metal_rough = met_rough_param;

	inu_assert(emission_param.variant == MATERIAL_PARAM_VARIANT::MAT_IMG || emission_param.variant == MATERIAL_PARAM_VARIANT::VEC3, "emission only has types of mat img and vec3");
	mat.emission = emission_param;

	inu_assert(normals_param.variant == MATERIAL_PARAM_VARIANT::MAT_IMG || normals_param.variant == MATERIAL_PARAM_VARIANT::VEC3, "emission only has types of mat img and vec3");
	mat.normals = normals_param;

	inu_assert(occ_param.variant == MATERIAL_PARAM_VARIANT::MAT_IMG || occ_param.variant == MATERIAL_PARAM_VARIANT::NONE, "occ map only has types of mat img and none");
	mat.occ = occ_param;

	mat.transparency_mode = transparency_mode;

	mat.cull_back = cull_back;
	
	mat.name = mat_name;

	materials.push_back(mat);

	return materials.size()-1;
}

void set_material_cull_back(int mat_id, bool cull_back) {
	materials[mat_id].cull_back = cull_back;
}

material_t get_material(int mat_idx) {
	return materials[mat_idx];
}

material_t bind_material(int mat_idx) {
	inu_assert(mat_idx < materials.size(), "mat idx out of bounds");
	shader_t& shader = material_t::associated_shader;

	material_t& mat = materials[mat_idx];

	for (int i = ALBEDO_IMG_TEX_SLOT; i <= EMISSION_IMG_TEX_SLOT; i++) {
		unbind_texture(i);
	}

	// set albedo information
	if (mat.albedo.variant == MATERIAL_PARAM_VARIANT::MAT_IMG) {
		material_image_t& color_tex = mat.albedo.base_color_img;
		const texture_t& texture = bind_texture(color_tex.tex_handle);
		inu_assert(texture.tex_slot == ALBEDO_IMG_TEX_SLOT, "albedo texture slot must be 0");
		shader_set_int(shader, "material.base_color_tex.samp", texture.tex_slot);
		shader_set_int(shader, "material.base_color_tex.tex_id", color_tex.tex_coords_idx);
		shader_set_int(shader, "material.use_base_color_tex", 1);
	} else if (mat.albedo.variant == MATERIAL_PARAM_VARIANT::VEC4) {
		shader_set_int(shader, "material.base_color_tex.samp", 0);
		shader_set_int(shader, "material.base_color_tex.tex_id", -1);
		shader_set_int(shader, "material.use_base_color_tex", 0);
#if 0
		vec3 c = {mat.albedo.base_color.x, mat.albedo.base_color.y, mat.albedo.base_color.z};
		shader_set_vec3(shader, "material.mesh_color", c);
#else

#if 0
		vec4 black(0,0,0,1);
		if (black == mat.albedo.base_color) {
			int b = 10;
			// mat.albedo.base_color = vec4(0,0,0,0);
		}
#endif

		shader_set_vec4(shader, "material.mesh_color", mat.albedo.base_color);
#endif
	} else {
		inu_assert_msg("this albedo variant is not supported");
	}

	// set metal roughness information
	if (mat.metal_rough.variant == MATERIAL_PARAM_VARIANT::MAT_IMG) {
		material_image_t& met_rough_tex = mat.metal_rough.met_rough_tex;
		const texture_t& texture = bind_texture(met_rough_tex.tex_handle);
		inu_assert(texture.tex_slot == METAL_ROUGH_IMG_TEX_SLOT, "metal roughness texture slot is not correct");
		shader_set_int(shader, "material.metal_rough_tex.samp", texture.tex_slot);
		shader_set_int(shader, "material.metal_rough_tex.tex_id", met_rough_tex.tex_coords_idx);
		shader_set_int(shader, "material.use_metal_rough_tex", 1);
	} else if (mat.metal_rough.variant == MATERIAL_PARAM_VARIANT::FLOAT) {
		shader_set_int(shader, "material.metal_rough_tex.samp", 0);
		shader_set_int(shader, "material.metal_rough_tex.tex_id", -1);
		shader_set_int(shader, "material.use_metal_rough_tex", 0);
		shader_set_float(shader, "material.surface_roughness", mat.metal_rough.roughness_factor);
		shader_set_float(shader, "material.metalness", mat.metal_rough.metallic_factor);
	} else {
		inu_assert_msg("this metal-rough variant is not supported");
	}

	// set emission information
#if 0
	static bool occ_on = true;
	if (window.input.right_mouse_up) {
		occ_on = !occ_on;
	}
#endif
	emission_param_t& emission_param = mat.emission;
	shader_set_int(shader, "material.use_emission_tex", (emission_param.variant == MATERIAL_PARAM_VARIANT::MAT_IMG));

	if (emission_param.variant == MATERIAL_PARAM_VARIANT::MAT_IMG) {
		const texture_t& emission_texture = bind_texture(emission_param.emissive_tex_info.tex_handle);
		inu_assert(emission_texture.tex_slot == EMISSION_IMG_TEX_SLOT, "emission texture slot is not correct");
		shader_set_int(shader, "material.emission_tex.samp", emission_texture.tex_slot);
		shader_set_int(shader, "material.emission_tex.tex_id", emission_param.emissive_tex_info.tex_coords_idx);
		shader_set_vec3(shader, "material.emission_factor", emission_param.emission_factor);
	} else {
		shader_set_int(shader, "material.emission_tex.samp", 0);
		shader_set_int(shader, "material.emission_tex.tex_id", 0);
		vec3 zero;
		shader_set_vec3(shader, "material.emission_factor", zero);
	}

	// set normals information
	normals_param_t& normals = mat.normals;
	shader_set_int(shader, "material.use_normal_tex", normals.variant == MATERIAL_PARAM_VARIANT::MAT_IMG);
	if (normals.variant == MATERIAL_PARAM_VARIANT::MAT_IMG) {
		const texture_t& normal_map = bind_texture(normals.normal_map.tex_handle);
		inu_assert(normal_map.tex_slot == NORMALS_IMG_TEX_SLOT, "normal map slot is not correct");
		shader_set_int(shader, "material.normal_tex.samp", normal_map.tex_slot);
		shader_set_int(shader, "material.normal_tex.tex_id", normals.normal_map.tex_coords_idx);
	}

	// ambient occlusion information
	occ_param_t& occ = mat.occ;
	shader_set_int(shader, "material.use_occ_tex", (occ.variant == MATERIAL_PARAM_VARIANT::MAT_IMG));
	if (occ.variant == MATERIAL_PARAM_VARIANT::MAT_IMG) {
		const texture_t& occ_map = bind_texture(occ.occ_map.tex_handle);
		inu_assert(occ_map.tex_slot == OCC_IMG_TEX_SLOT, "occ map slot is not correct");
		shader_set_int(shader, "material.occ_tex.samp", occ_map.tex_slot);
		shader_set_int(shader, "material.occ_tex.tex_id", occ.occ_map.tex_coords_idx);
	}

	// culling information
	if (mat.cull_back) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	} else {
		glDisable(GL_CULL_FACE);
	}

  bind_shader(shader);
  return mat;
}

framebuffer_t create_framebuffer(int width, int height, FB_TYPE fb_type) {
	framebuffer_t fb;

	GLuint gl_fb_id;
	glGenFramebuffers(1, &gl_fb_id);

	fb.id = internal_fb_running_id;
	internal_fb_running_id++;
	fb_id_to_gl_id[fb.id] = gl_fb_id;

	glBindFramebuffer(GL_FRAMEBUFFER, gl_fb_id);		

	if (fb_type == FB_TYPE::RENDER_BUFFER_DEPTH_STENCIL) {
#if 0
		glGenTextures(1, &fb.color_att);
		glBindTexture(GL_TEXTURE_2D, fb.color_att);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.color_att, 0);
#else
		tex_creation_meta_t meta_data;
		meta_data.min_filter = TEX_FILTER_METHOD::LINEAR;
		meta_data.mag_filter = TEX_FILTER_METHOD::LINEAR;
		fb.color_att = create_texture(0, 0, width, height, 1, meta_data);
		GLuint gl_color_att = get_internal_tex_gluint(fb.color_att);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_color_att, 0);
#endif

		unsigned int rbo;
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	} else if (fb_type == FB_TYPE::TEXTURE_DEPTH_STENCIL) {

#if 0
		glGenTextures(1, &fb.color_att);
		glBindTexture(GL_TEXTURE_2D, fb.color_att);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.color_att, 0);
#else
		tex_creation_meta_t meta_data;
		meta_data.min_filter = TEX_FILTER_METHOD::LINEAR;
		meta_data.mag_filter = TEX_FILTER_METHOD::LINEAR;
		fb.color_att = create_texture(0, 0, width, height, 1, meta_data);
		GLuint gl_color_att = get_internal_tex_gluint(fb.color_att);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_color_att, 0);
#endif


#if 0
		glGenTextures(1, &fb.depth_att);
		glBindTexture(GL_TEXTURE_2D, fb.depth_att);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
  	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fb.depth_att, 0);
#else
		tex_creation_meta_t depth_meta_data;
		depth_meta_data.min_filter = TEX_FILTER_METHOD::LINEAR;
		depth_meta_data.mag_filter = TEX_FILTER_METHOD::LINEAR;
		depth_meta_data.tex_format = TEX_FORMAT::DEPTH_STENCIL;
		depth_meta_data.input_data_tex_format = TEX_FORMAT::DEPTH_STENCIL;
		depth_meta_data.data_type = TEX_DATA_TYPE::DEPTH_STENCIL;

		fb.depth_att = create_texture(0, 0, width, height, 1, depth_meta_data);
		GLuint gl_depth_att = get_internal_tex_gluint(fb.depth_att);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, gl_depth_att, 0);
#endif
	} else if (fb_type == FB_TYPE::MULTIPLE_DEPTH_TEXTURE) {

#if 0
		glGenTextures(1, &fb.color_att);
		glBindTexture(GL_TEXTURE_2D, fb.color_att);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.color_att, 0);
#else
		tex_creation_meta_t meta_data;
		meta_data.min_filter = TEX_FILTER_METHOD::LINEAR;
		meta_data.mag_filter = TEX_FILTER_METHOD::LINEAR;
		fb.color_att = create_texture(0, 0, width, height, 1, meta_data);
		GLuint gl_color_att = get_internal_tex_gluint(fb.color_att);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_color_att, 0);
#endif

#if 0
		glGenTextures(1, &fb.depth_att);
		get_gfx_error();
		glBindTexture(GL_TEXTURE_2D_ARRAY, fb.depth_att);
		get_gfx_error();
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0,
    	GL_DEPTH24_STENCIL8,
    	width,
    	height,
    	3,
    	0,
    	GL_DEPTH_STENCIL,
    	GL_UNSIGNED_INT_24_8,
    	nullptr);
		get_gfx_error();
    // glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		// glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		get_gfx_error();

		constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);
		get_gfx_error();
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, fb.depth_att, 0);
		get_gfx_error();
#else
		tex_creation_meta_t depth_meta_data;
		depth_meta_data.min_filter = TEX_FILTER_METHOD::LINEAR;
		depth_meta_data.mag_filter = TEX_FILTER_METHOD::LINEAR;
		depth_meta_data.s_wrap_mode = WRAP_MODE::CLAMP_TO_BORDER;
		depth_meta_data.t_wrap_mode = WRAP_MODE::CLAMP_TO_BORDER;
		depth_meta_data.tex_type = TEX_TYPE::TEXTURE_2D_ARRAY;
		fb.depth_att = create_texture(0, 0, width, height, NUM_SM_CASCADES, depth_meta_data);
		GLuint gl_depth_att = get_internal_tex_gluint(fb.depth_att);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, gl_depth_att, 0);
#endif
	} else if (fb_type == FB_TYPE::NO_COLOR_ATT_MULTIPLE_DEPTH_TEXTURE) {
#if 0
		glGenTextures(1, &fb.depth_att);
		get_gfx_error();
		glBindTexture(GL_TEXTURE_2D_ARRAY, fb.depth_att);
		get_gfx_error();
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0,
    	GL_DEPTH_COMPONENT32F,
    	width,
    	height,
    	3,
    	0,
    	GL_DEPTH_COMPONENT,
    	GL_FLOAT,
    	nullptr);
		get_gfx_error();
    // glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		// glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		get_gfx_error();

		constexpr float bordercolor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BORDER_COLOR, bordercolor);
		get_gfx_error();
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fb.depth_att, 0);
		get_gfx_error();
#else
		tex_creation_meta_t meta_data;
		meta_data.min_filter = TEX_FILTER_METHOD::LINEAR;
		meta_data.mag_filter = TEX_FILTER_METHOD::LINEAR;
		meta_data.s_wrap_mode = WRAP_MODE::CLAMP_TO_BORDER;
		meta_data.t_wrap_mode = WRAP_MODE::CLAMP_TO_BORDER;
		meta_data.tex_type = TEX_TYPE::TEXTURE_2D_ARRAY;
		meta_data.input_data_tex_format = TEX_FORMAT::DEPTH;
		meta_data.tex_format = TEX_FORMAT::DEPTH;
		meta_data.data_type = TEX_DATA_TYPE::FLOAT;

		fb.depth_att = create_texture(0, 0, width, height, NUM_SM_CASCADES, meta_data);
		GLuint gl_depth_att = get_internal_tex_gluint(fb.depth_att);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, gl_depth_att, 0);
#endif

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		inu_assert_msg("framebuffer not made successfully");
	}

	fb.fb_type = fb_type;
	fb.width = width;
	fb.height = height;

	return fb;
}

void bind_framebuffer(framebuffer_t& fb) {
	glBindFramebuffer(GL_FRAMEBUFFER, fb.id);
	glViewport(0, 0, fb.width, fb.height);
	// glViewport(0, 0, 1280, 960);
}

void unbind_framebuffer() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window.window_dim.x, window.window_dim.y);
}

void clear_framebuffer() {
	glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void clear_framebuffer_depth() {
  glClear(GL_DEPTH_BUFFER_BIT);
}

void get_gfx_error() {
	GLenum err = glGetError();
	if (err == GL_NO_ERROR) {
		printf("No error");
	} else if (err == GL_INVALID_ENUM) {
		printf("An unacceptable value is specified for an enumerated argument.");
	} else if (err == GL_INVALID_VALUE) {
		printf("A numeric argument is out of range.");
	} else {
		printf("error not recognized");
	}
	printf("\n");
}
