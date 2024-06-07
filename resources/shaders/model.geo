#version 410 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

mat4 calc_tbn_mat(int idx);

struct shader_tex {
  sampler2D samp; 
  int tex_id;
};

struct material_t {
  // base color info
  shader_tex base_color_tex;
  vec3 mesh_color;
  int use_base_color_tex;

  // metal and roughness info
  float surface_roughness;
  float metalness;
  shader_tex metal_rough_tex;
  int use_metal_rough_tex;

  // emission info
  vec3 emission_factor;
  shader_tex emission_tex;
  int use_emission_tex;

  // normal info
  int use_normal_tex;
  shader_tex normal_tex;

  // occ info
  int use_occ_tex;
  shader_tex occ_tex;
};

uniform material_t material;

in VS_OUT {
  vec2 tex_coords[2];
  vec3 color;

  vec4 normal;

  vec4 spotlight_rel_screen_pos0;
  vec4 spotlight_rel_screen_pos1;
  vec4 spotlight_rel_screen_pos2;

  vec4 global;
  vec4 cam_rel_pos;
} gs_in[];

out vec2 tex_coords[2];
out vec3 color;

out vec4 normal;
out mat4 tbn_mat;

out vec4 spotlight_rel_screen_pos0;
out vec4 spotlight_rel_screen_pos1;
out vec4 spotlight_rel_screen_pos2;

out vec4 global;
out vec4 cam_rel_pos;

void emit_vert(int idx) {
  gl_Position = gl_in[idx].gl_Position;

  tex_coords[0] = gs_in[idx].tex_coords[0];
  tex_coords[1] = gs_in[idx].tex_coords[1];

  color = gs_in[idx].color;

  normal = gs_in[idx].normal;

  spotlight_rel_screen_pos0 = gs_in[idx].spotlight_rel_screen_pos0;
  spotlight_rel_screen_pos1 = gs_in[idx].spotlight_rel_screen_pos1;
  spotlight_rel_screen_pos2 = gs_in[idx].spotlight_rel_screen_pos2;

  global = gs_in[idx].global;
  cam_rel_pos = gs_in[idx].cam_rel_pos;

  tbn_mat = calc_tbn_mat(idx);

  EmitVertex();
}

mat4 calc_tbn_mat(int idx) {
  if (material.use_normal_tex == 0) {
    return mat4(1.0);
  }
  int tex_id = material.normal_tex.tex_id;

  vec2 uv0 = gs_in[0].tex_coords[tex_id];
  vec2 uv1 = gs_in[1].tex_coords[tex_id];
  vec2 uv2 = gs_in[2].tex_coords[tex_id];

  float u1 = uv1.x - uv0.x;
  float v1 = uv1.y - uv0.y;

  float u2 = uv2.x - uv1.x;
  float v2 = uv2.y - uv1.y;

  mat2 tex_coord_diff = mat2(vec2(u1, u2), vec2(v1, v2));
  mat2 inv_tex_coord_diff = inverse(tex_coord_diff);

#if 0
  vec3 combined_normal = normalize(gs_in[0].normal.xyz) + normalize(gs_in[1].normal.xyz) + normalize(gs_in[2].normal.xyz);
  vec3 surface_normal = normalize(combined_normal / 3.0);
  vec4 N = vec4(surface_normal, 0.0);
#else
  vec3 combined_normal = normalize(gs_in[idx].normal.xyz);
  vec4 N = vec4(combined_normal, 0.0);
#endif

  vec3 e1 = (gl_in[1].gl_Position.xyz / gl_in[1].gl_Position.w) - (gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w);
  vec3 e2 = (gl_in[2].gl_Position.xyz / gl_in[2].gl_Position.w) - (gl_in[1].gl_Position.xyz / gl_in[1].gl_Position.w);

  vec2 tb_x = inv_tex_coord_diff * vec2(e1.x, e2.x);
  vec2 tb_y = inv_tex_coord_diff * vec2(e1.y, e2.y);
  vec2 tb_z = inv_tex_coord_diff * vec2(e1.z, e2.z);

  vec4 T = vec4(tb_x.x, tb_y.x, tb_z.x, 0.0);
  vec4 B = vec4(tb_x.y, tb_y.y, tb_z.y, 0.0);

  return mat4(T, B, N, vec4(0,0,0,1));
}

void main() {
  // tbn_mat = calc_tbn_mat();
  emit_vert(0);
  emit_vert(1);
  emit_vert(2);
  EndPrimitive();
}
