#version 410 core

uniform sampler2DArray shadow_map;

uniform int cascade;

in vec2 tex;

out vec4 frag_color;

void main() {
  float r = texture(shadow_map, vec3(tex, cascade)).r;
  frag_color = vec4(r,r,r,1.0);
}
