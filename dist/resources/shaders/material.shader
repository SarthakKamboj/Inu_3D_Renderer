
struct shader_tex {
  sampler2D samp; 
  int tex_id;
};

struct material_t {
  // base color info
  shader_tex base_color_tex;
  vec4 mesh_color;
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

