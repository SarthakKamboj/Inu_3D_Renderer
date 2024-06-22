#version 410 core

#include "model_mat.shader"

layout (location = 0) in vec3 vert_pos;
layout (location = 1) in vec2 tex0;
layout (location = 2) in vec2 tex1;
layout (location = 3) in vec3 vert_color;
layout (location = 4) in vec4 joints;
layout (location = 5) in vec4 weights;
layout (location = 6) in vec3 vert_normal;

struct light_t {
  vec3 pos;
}; 

// lights
// uniform light_t light;

uniform mat4 light_view;
uniform mat4 light_projection;

void main() {
  mat4 final_model = get_model_mat(joints, weights);
  gl_Position = light_projection * light_view * final_model * vec4(vert_pos, 1.0);
}
