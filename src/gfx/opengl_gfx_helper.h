#pragma once

#include "glew.h"
#include "gfx.h"

struct gl_tex_creation_meta_t {
	GLenum tex_target;
	GLint min_filter;
	GLint mag_filter;
	GLenum input_data_tex_format;
	GLint tex_format;
	GLint s_wrap_mode;
	GLint t_wrap_mode;
	GLenum data_type;
};

GLint internal_to_gl_wrap_mode(WRAP_MODE wrap_mode);
GLint internal_to_gl_tex_format(TEX_FORMAT format);
GLenum internal_to_gl_input_data_tex_format(TEX_FORMAT format);
GLenum internal_to_gl_tex_target(TEX_TYPE tex_type);
GLint internal_to_gl_tex_filter(TEX_FILTER_METHOD filter_method);
GLenum internal_to_gl_tex_data_type(TEX_DATA_TYPE type);
gl_tex_creation_meta_t internal_to_gl_tex_meta(tex_creation_meta_t& internal);
GLenum internal_to_gl_vao_data_type(VAO_ATTR_DATA_TYPE t);
