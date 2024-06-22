#include "opengl_gfx_helper.h"

#include <iostream>

#include "utils/log.h"
#include "utils/general.h"

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
		case TEX_FORMAT::SINGLE: {
			return GL_RED;
		}
		case TEX_FORMAT::RG: {
			return GL_RG;
		}
		case TEX_FORMAT::RGB: {
			return GL_RGB;
		}
		case TEX_FORMAT::RGBA: {
			return GL_RGBA;
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
		case TEX_FORMAT::SINGLE: {
			return GL_RED;
		}
		case TEX_FORMAT::RG: {
			return GL_RG;
		}
		case TEX_FORMAT::RGB: {
			return GL_RGB;
		}
		case TEX_FORMAT::RGBA: {
			return GL_RGBA;
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

char* custom_parse_shader_contents(const char* path) {

#define STARTING_CONTENTS_BUFFER_LEN 2048
#define MAX_LINE_LEN 256

	char resources_path[256]{};
  get_resources_folder_path(resources_path);

	int total_contents_len = STARTING_CONTENTS_BUFFER_LEN;
	char* shader_contents = (char*)malloc(STARTING_CONTENTS_BUFFER_LEN * sizeof(char));
	memset(shader_contents, 0, total_contents_len);
	int running_len = 0;

	FILE* file = fopen(path, "r");
  inu_assert(file, "could not open file to get file contents");
	size_t content_len = 0;

	while (!feof(file)) {
		size_t line_len = 0;
		// char* line = fgetline(file, &line_len);
		char line[MAX_LINE_LEN]{};
		fgets(line, MAX_LINE_LEN, file);
		bool include_line = strstr(line, "#include") != NULL;

		char* char_to_add = NULL;

		if (include_line) {
			char include_file_name[64]{};
			sscanf(line, "#include \"%s", include_file_name);
			include_file_name[strlen(include_file_name)-1] = 0;
			char include_shader_path[256]{};
  		sprintf(include_shader_path, "%s\\shaders\\%s", resources_path, include_file_name);
			char* include_file_contents = get_file_contents(include_shader_path);
			char_to_add = include_file_contents;
			// running_len += strlen(include_file_contents);	
		} else {
			char_to_add = line;
			// sprintf(shader_contents + running_len, "%s", line);
			// running_len += line_len;
		}	

		if (running_len + strlen(char_to_add) >= total_contents_len) {
			total_contents_len *= 2;
    	shader_contents = (char*)realloc(shader_contents, total_contents_len * sizeof(char));
		}

		sprintf(shader_contents + (int)running_len, "%s", char_to_add);
		running_len += strlen(char_to_add);	

		if (include_line) {
			free(char_to_add);
		}
	}

	shader_contents[running_len] = 0;

	fclose(file);
	
	return shader_contents;

#undef MAX_LINE_LEN
#undef STARTING_CONTENTS_BUFFER_LEN
}
