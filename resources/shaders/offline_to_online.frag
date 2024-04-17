#version 410 core

in vec2 out_tex;

uniform sampler2D img;

uniform int render_depth;

out vec4 frag_color;

void main() {
  if (render_depth == 0) {
    frag_color = texture(img, out_tex);
  } else {
    float pre_v = texture(img, out_tex).r;
    float v = (pre_v + 1) / 2;
    // float v5 = v;
    float v5 = pow(v, 80);
    frag_color = vec4(v5,v5,v5,1);
  }
}
