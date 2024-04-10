#include "gfx.h"

#include <stddef.h>
#include <stdlib.h>
#include <string>
#include <stdexcept>
#include <vector>

#include "model_loading/image/stb_image.h"

#include "utils/general.h"
#include "utils/log.h"
#include "windowing/window.h"

std::vector<material_t> materials;
extern window_t window;

// VBO
vbo_t create_vbo(const void* vertices, const int data_size) {
	vbo_t vbo;
	glGenBuffers(1, &vbo.id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
	glBufferData(GL_ARRAY_BUFFER, data_size, vertices, GL_STATIC_DRAW);
	return vbo;
}

vbo_t create_dyn_vbo(const int data_size) {
	vbo_t vbo;
	glGenBuffers(1, &vbo.id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
	glBufferData(GL_ARRAY_BUFFER, data_size, NULL, GL_DYNAMIC_DRAW);
	return vbo;
}

void update_vbo_data(const vbo_t& vbo, const void* vertices, const int data_size) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
	glBufferSubData(GL_ARRAY_BUFFER, 0, data_size, vertices);
}

void bind_vbo(const vbo_t& vbo) {
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id);
}

void unbind_vbo() {
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void delete_vbo(const vbo_t& vbo) {
	glDeleteBuffers(1, &vbo.id);
}

// EBO
ebo_t create_ebo(const unsigned int* indicies, const int size_of_buffer) {
	ebo_t ebo;
	ebo.num_indicies = size_of_buffer / sizeof(indicies[0]);
	glGenBuffers(1, &ebo.id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo.id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_of_buffer, indicies, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return ebo;
}

void draw_ebo(const ebo_t& ebo) {
	glDrawElements(GL_TRIANGLES, ebo.num_indicies, GL_UNSIGNED_INT, 0);
}

void bind_ebo(const ebo_t& ebo) {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo.id);
}

void unbind_ebo() {
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void delete_ebo(const ebo_t& ebo) {
	glDeleteBuffers(1, &ebo.id);
}

// VAO
vao_t create_vao() {
	vao_t vao;
	glGenVertexArrays(1, &vao.id);
	return vao;
}

void bind_vao(const vao_t& vao) {
	glBindVertexArray(vao.id);
}

void unbind_vao() {
	glBindVertexArray(0);
}

void vao_enable_attribute(vao_t& vao, const vbo_t& vbo, const int attr_id, const int num_values, const int d_type, const int stride, const int offset) {
	bind_vao(vao);
	bind_vbo(vbo);
	glVertexAttribPointer(attr_id, num_values, d_type, GL_FALSE, stride, reinterpret_cast<void*>(offset));
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
	glDeleteVertexArrays(1, &vao.id);
}

// Shaders
shader_t create_shader(const char* vert_source_path, const char* frag_source_path) {
	shader_t shader;
	shader.id = glCreateProgram();

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

	glAttachShader(shader.id, vert);
	glAttachShader(shader.id, frag);
	glLinkProgram(shader.id);

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
	shader.id = glCreateProgram();

	if (shader.id == 10) {
		int a = 5;
	}

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

	glAttachShader(shader.id, vert);
	glAttachShader(shader.id, geom);
	glAttachShader(shader.id, frag);
	glLinkProgram(shader.id);

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
	glUseProgram(shader.id);
}

void unbind_shader() {
	glUseProgram(0);
}

void shader_set_mat4(shader_t& shader, const char* var_name, mat4& mat) {
	glUseProgram(shader.id);
	GLint loc = glGetUniformLocation(shader.id, var_name);
  if (loc == -1) {
      printf("%s does not exist in shader %i\n", var_name, shader.id);
  }
	glUniformMatrix4fv(loc, 1, GL_FALSE, (float*)&mat.cols[0].x);
	unbind_shader();
}

void shader_set_int(shader_t& shader, const char* var_name, int val) {
	glUseProgram(shader.id);
	GLint loc = glGetUniformLocation(shader.id, var_name);
  if (loc == -1) {
      printf("%s does not exist in shader %i\n", var_name, shader.id);
  }
	glUniform1i(loc, val);
	unbind_shader();
}

void shader_set_float(shader_t& shader, const char* var_name, float val) {
	glUseProgram(shader.id);
	GLint loc = glGetUniformLocation(shader.id, var_name);
    if (loc == -1) {
        printf("%s does not exist in shader %i\n", var_name, shader.id);
    }
	glUniform1f(loc, val);
	unbind_shader();
}

void shader_set_vec3(shader_t& shader, const char* var_name, vec3 vec) {
	glUseProgram(shader.id);
	GLint loc = glGetUniformLocation(shader.id, var_name);
    if (loc == -1) {
        printf("%s does not exist in shader %i\n", var_name, shader.id);
    }
	glUniform3fv(loc, 1, (GLfloat*)&vec);
	unbind_shader();
}

// TEXTURES
static std::vector<texture_t> textures;
int create_texture(const char* img_path, int tex_slot) {
	for (texture_t& t : textures) {
		if (strcmp(img_path, t.path.c_str()) == 0) {
			return t.id;
		}
	}

	texture_t texture;
	texture.tex_slot = tex_slot;	
	texture.path = std::string(img_path);

	stbi_set_flip_vertically_on_load(false);
	unsigned char* data = stbi_load(img_path, &texture.width, &texture.height, &texture.num_channels, 0);
	inu_assert(data, "image data not loaded");

	texture.id = textures.size();

	glGenTextures(1, &texture.gl_id);
	glBindTexture(GL_TEXTURE_2D, texture.gl_id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	if (texture.num_channels == 3) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.width, texture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	} else if (texture.num_channels == 4) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	} else if (texture.num_channels == 1) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, texture.width, texture.height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
	} else {
		stbi_image_free(data);
		glDeleteTextures(1, &texture.gl_id);
		return -1;
	}
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
  glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(data);
	textures.push_back(texture);
	return texture.id;
}

texture_t bind_texture(int tex_id) {
	texture_t& tex = textures[tex_id];
	glActiveTexture(GL_TEXTURE0 + tex.tex_slot);
	glBindTexture(GL_TEXTURE_2D, tex.gl_id);
	return tex;
}

void unbind_texture() {
	glBindTexture(GL_TEXTURE_2D, 0);
}

// MATERIALS
shader_t material_t::associated_shader;

metallic_roughness_param_t::metallic_roughness_param_t() {}

albedo_param_t::albedo_param_t() {}

material_t::material_t() {}

int create_material(vec4 color, material_image_t base_color_img) {
	material_t mat;
	mat.albedo.base_color_img = base_color_img;
	mat.albedo.color_factor = color;
	mat.albedo.color_factor.w = 1;
	materials.push_back(mat);
	return materials.size()-1;
}

material_t get_material(int mat_idx) {
	return materials[mat_idx];
}

material_t bind_material(int mat_idx) {
	inu_assert(mat_idx < materials.size(), "mat idx out of bounds");
	shader_t& shader = material_t::associated_shader;

	material_t& mat = materials[mat_idx];
	if (mat.albedo.base_color_img.tex_handle != -1) {
		material_image_t& color_tex = mat.albedo.base_color_img;
		texture_t& texture = bind_texture(color_tex.tex_handle);
		inu_assert(texture.tex_slot == ALBEDO_IMG_TEX_SLOT, "albedo texture slot must be 0");
		shader_set_int(shader, "base_color_tex.samp", texture.tex_slot);
		shader_set_int(shader, "base_color_tex.tex_id", color_tex.tex_coords_idx);
		shader_set_int(shader, "use_mesh_color", 0);
	} else {
		shader_set_int(shader, "base_color_tex.samp", 0);
		shader_set_int(shader, "base_color_tex.tex_id", -1);
		shader_set_int(shader, "use_mesh_color", 1);
		vec3 c = {mat.albedo.color_factor.x, mat.albedo.color_factor.y, mat.albedo.color_factor.z};
		shader_set_vec3(shader, "mesh_color", c);
	}

  bind_shader(shader);
  return mat;
}

framebuffer_t create_framebuffer(int width, int height, FB_TYPE fb_type) {
	framebuffer_t fb;
	glGenFramebuffers(1, &fb.id);
	glBindFramebuffer(GL_FRAMEBUFFER, fb.id);		

	if (fb_type == FB_TYPE::RENDER_BUFFER_DEPTH_STENCIL) {

		glGenTextures(1, &fb.color_att);
		glBindTexture(GL_TEXTURE_2D, fb.color_att);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.color_att, 0);

		unsigned int rbo;
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	} else if (fb_type == FB_TYPE::TEXTURE_DEPTH_STENCIL) {

		glGenTextures(1, &fb.color_att);
		glBindTexture(GL_TEXTURE_2D, fb.color_att);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.color_att, 0);

		glGenTextures(1, &fb.depth_att);
		glBindTexture(GL_TEXTURE_2D, fb.depth_att);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
  	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fb.depth_att, 0);
	} else if (fb_type == FB_TYPE::MULTIPLE_DEPTH_TEXTURE) {

		glGenTextures(1, &fb.color_att);
		glBindTexture(GL_TEXTURE_2D, fb.color_att);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.color_att, 0);

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
	} else if (fb_type == FB_TYPE::NO_COLOR_ATT_MULTIPLE_DEPTH_TEXTURE) {
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
}

void unbind_framebuffer() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, window.window_dim.x, window.window_dim.y);
}

void clear_framebuffer(framebuffer_t& fb) {
	glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
