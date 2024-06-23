#version 410 core

#include "model_mat.shader"

#define NUM_CASCADES 3

layout (location = 0) in vec3 vert_pos;
layout (location = 1) in vec2 tex0;
layout (location = 2) in vec2 tex1;
layout (location = 3) in vec3 vert_color;
layout (location = 4) in vec4 joints;
layout (location = 5) in vec4 weights;
layout (location = 6) in vec3 vert_normal;

uniform mat4 view;
uniform mat4 projection;

// light 4x4 matrix info
struct spotlight_mat_data_t {
  mat4 light_view;
  mat4 light_projection;
};

uniform spotlight_mat_data_t spotlights_mat_data[3];

out VS_OUT {
  vec2 tex_coords[2];
  vec3 color;

  vec4 normal;

  vec4 spotlight_rel_screen_pos0;
  vec4 spotlight_rel_screen_pos1;
  vec4 spotlight_rel_screen_pos2;

  vec4 global;
  vec4 cam_rel_pos;
} output_data;

vec4 calc_spotlight_rel_pos(mat4 light_projection, mat4 light_view, mat4 model) {
  return light_projection * light_view * model * vec4(vert_pos, 1.0);
}

void main() {

  mat4 final_model = get_model_mat(joints, weights);
  
  output_data.global = final_model * vec4(vert_pos, 1.0);
  output_data.cam_rel_pos = view * output_data.global;
  gl_Position = projection * output_data.cam_rel_pos;

  output_data.tex_coords[0] = tex0;
  output_data.tex_coords[1] = tex1;
  output_data.color = vert_color;
  // must convert normal to global version
  output_data.normal = transpose(inverse(final_model)) * vec4(vert_normal, 0.0);

  output_data.spotlight_rel_screen_pos0 = calc_spotlight_rel_pos(spotlights_mat_data[0].light_projection, spotlights_mat_data[0].light_view, final_model);
  output_data.spotlight_rel_screen_pos1 = calc_spotlight_rel_pos(spotlights_mat_data[1].light_projection, spotlights_mat_data[1].light_view, final_model);
  output_data.spotlight_rel_screen_pos2 = calc_spotlight_rel_pos(spotlights_mat_data[2].light_projection, spotlights_mat_data[2].light_view, final_model);
}

