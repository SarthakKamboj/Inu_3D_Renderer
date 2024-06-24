#version 410 core

layout (location = 0) in vec3 vert_pos;
layout (location = 1) in vec2 tex0;
layout (location = 2) in vec2 tex1;
layout (location = 3) in vec3 vert_color;
layout (location = 4) in vec4 joints;
layout (location = 5) in vec4 weights;
layout (location = 6) in vec3 vert_normal;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main() {
  gl_Position = projection * view * model * vec4(vert_pos, 1.0);
}
