#include "opengl_gfx_helper.h"

#include "utils/log.h"

GLint internal_to_gl_wrap_mode(WRAP_MODE wrap_mode) {	
	switch (wrap_mode) {
		case WRAP_MODE::REPEAT: {
			return GL_REPEAT;
		}
		case WRAP_MODE::CLAMP_TO_EDGE: {
			return GL_CLAMP_TO_EDGE;
		}
		case WRAP_MODE::CLAMP_TO_BORDER: {
			return GL_CLAMP_TO_BORDER;
		}
	}
	inu_assert_msg("wrap mode not found");
	return GL_NONE;
}

GLint internal_to_gl_tex_format(TEX_FORMAT tex_format) {	
	switch (tex_format) {
		case TEX_FORMAT::RGB: {
			return GL_RGB;
		}
		case TEX_FORMAT::RGBA: {
			return GL_RGBA;
		}
		case TEX_FORMAT::SINGLE: {
			return GL_RED;
		}
		case TEX_FORMAT::DEPTH_STENCIL: {
			return GL_DEPTH24_STENCIL8;
		}
		case TEX_FORMAT::DEPTH: {
			return GL_DEPTH_COMPONENT32F;
		}
	}
	inu_assert_msg("tex format not found");
	return GL_NONE;
}

GLenum internal_to_gl_input_data_tex_format(TEX_FORMAT tex_format) {	
	switch (tex_format) {
		case TEX_FORMAT::RGB: {
			return GL_RGB;
		}
		case TEX_FORMAT::RGBA: {
			return GL_RGBA;
		}
		case TEX_FORMAT::SINGLE: {
			return GL_RED;
		}
		case TEX_FORMAT::DEPTH_STENCIL: {
			return GL_DEPTH_STENCIL;
		}
		case TEX_FORMAT::DEPTH: {
			return GL_DEPTH_COMPONENT;
		}
	}
	inu_assert_msg("input data tex format not found");
	return GL_NONE;
}

GLint internal_to_gl_tex_filter(TEX_FILTER_METHOD filter_method) {
	switch (filter_method) {
		case TEX_FILTER_METHOD::LINEAR: {
			return GL_LINEAR;
		}
		case TEX_FILTER_METHOD::NEAREST: {
			return GL_NEAREST;
		}
	}
	inu_assert_msg("tex filter not found");
	return GL_NONE;
}

GLenum internal_to_gl_tex_data_type(TEX_DATA_TYPE type) {
	switch (type) {
		case TEX_DATA_TYPE::UNSIGNED_BYTE: {
 		 return GL_UNSIGNED_BYTE;
		}
		case TEX_DATA_TYPE::DEPTH_STENCIL: {
 		 return GL_UNSIGNED_INT_24_8;
		}
		case TEX_DATA_TYPE::FLOAT: {
 		 return GL_FLOAT;
		}
	}
	inu_assert_msg("tex data type format not found");
	return GL_NONE;
}

GLenum internal_to_gl_tex_target(TEX_TYPE tex_type) {
	switch (tex_type) {
		case TEX_TYPE::TEXTURE_2D: return GL_TEXTURE_2D;
		case TEX_TYPE::TEXTURE_2D_ARRAY: return GL_TEXTURE_2D_ARRAY;
	}
	inu_assert_msg("tex target not found");
	return GL_NONE;
}

gl_tex_creation_meta_t internal_to_gl_tex_meta(tex_creation_meta_t& internal) {
  gl_tex_creation_meta_t gl_meta;

	gl_meta.min_filter = internal_to_gl_tex_filter(internal.min_filter);
	gl_meta.mag_filter = internal_to_gl_tex_filter(internal.mag_filter);
	gl_meta.input_data_tex_format = internal_to_gl_input_data_tex_format(internal.input_data_tex_format);
	gl_meta.tex_format = internal_to_gl_tex_format(internal.tex_format);
	gl_meta.s_wrap_mode = internal_to_gl_wrap_mode(internal.s_wrap_mode);
	gl_meta.t_wrap_mode = internal_to_gl_wrap_mode(internal.t_wrap_mode);
	gl_meta.tex_target = internal_to_gl_tex_target(internal.tex_type);
	gl_meta.data_type = internal_to_gl_tex_data_type(internal.data_type);

	return gl_meta;
}

GLenum internal_to_gl_vao_data_type(VAO_ATTR_DATA_TYPE t) {
	if (t == VAO_ATTR_DATA_TYPE::FLOAT) return GL_FLOAT;
	if (t == VAO_ATTR_DATA_TYPE::UNSIGNED_INT) return GL_UNSIGNED_INT;

	inu_assert_msg("vao data type not found");
	return GL_NONE;
}
