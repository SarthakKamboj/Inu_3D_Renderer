#version 410 core

#include "model_mat.shader"

layout (location = 0) in vec3 vert_pos;
layout (location = 1) in vec2 tex0;
layout (location = 2) in vec2 tex1;
layout (location = 3) in vec3 vert_color;
layout (location = 4) in vec4 joints;
layout (location = 5) in vec4 weights;
layout (location = 6) in vec3 vert_normal;

uniform mat4 view;
uniform mat4 projection;

void main() {
  mat4 final_model = get_model_mat(joints, weights);
  gl_Position = projection * view * final_model * vec4(vert_pos, 1.0);
}
