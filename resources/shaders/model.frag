#version 410 core

#define NUM_CASCADES 3

#define PI 3.14159265359

#define VIEW_AMOUNT_IN_LIGHT 0
#define VIEW_LIGHT_MULTIPLIER 0
#define VIEW_LIGHT0_CLOSEST_DEPTH 0
#define VIEW_LIGHT1_CLOSEST_DEPTH 0
#define VIEW_LIGHT2_CLOSEST_DEPTH 0
#define VIEW_LIGHT0_DEPTH 0
#define VIEW_LIGHT1_DEPTH 0
#define VIEW_LIGHT2_DEPTH 0
#define VIEW_LIGHT0_CALCULATED_UVs 0
#define VIEW_LIGHT1_CALCULATED_UVs 0
#define VIEW_LIGHT2_CALCULATED_UVs 0
#define VIEW_LIGHT0_AMOUNT_IN_LIGHT 0
#define VIEW_LIGHT1_AMOUNT_IN_LIGHT 0
#define VIEW_LIGHT2_AMOUNT_IN_LIGHT 0
#define VIEW_NORMALS 0
#define VIEW_DIR_LIGHT_CLOSEST_DEPTH 0
#define VIEW_DIR_LIGHT_DEPTH 0
#define VIEW_DIR_LIGHT_AMOUNT_IN_LIGHT 0
#define VIEW_CASCADE 0
#define ENABLE_QUANTIZING 0

float s_rgb_to_linear(float s_rgb);
float get_roughness();
float get_metalness();

// https://stackoverflow.com/questions/66469497/gltf-setting-colors-basecolorfactor
float s_rgb_to_linear(float s_rgb) {
  return pow(s_rgb / 255.0, 2.2);
}

struct shader_tex {
  sampler2D samp; 
  int tex_id;
};

struct material_t {
  shader_tex base_color_tex;
  vec3 mesh_color;
  int use_base_color_tex;

  float surface_roughness;
  float metalness;
  shader_tex metal_rough_tex;
  int use_metal_rough_tex;
};

uniform material_t material;
uniform int override_color_bool;

struct spotlight_data_t {
  sampler2D depth_tex;
  vec3 pos;
  int light_active;
  int shadow_map_width;
  int shadow_map_height;
  float near_plane;
  float far_plane;
};
uniform spotlight_data_t spotlights_data[3];

struct dir_light_data_t {
  int light_active;
  sampler2DArray shadow_map;
  mat4 light_views[NUM_CASCADES];
  mat4 light_projs[NUM_CASCADES];
  float cascade_depths[NUM_CASCADES+1];
  vec3 light_dir;
};

uniform dir_light_data_t dir_light_data;

struct cam_data_t {
  float near_plane;
  float far_plane;
  vec3 cam_pos;
};
uniform cam_data_t cam_data;

in vec2 tex_coords[2];
in vec3 color;
in vec4 normal;

in vec4 spotlight_rel_screen_pos0;
in vec4 spotlight_rel_screen_pos1;
in vec4 spotlight_rel_screen_pos2;

in vec4 global;
in vec4 cam_rel_pos;

out vec4 frag_color;

struct norm_inter_vecs_t {
  vec3 normal;
  vec3 view_dir;
  vec3 world_pos;
  vec3 cam_rel_pos;
};

norm_inter_vecs_t calc_normalized_vectors() {
  norm_inter_vecs_t niv;  

  niv.normal = normalize(normal.xyz);

  vec3 normalized_global_pos = global.xyz / global.w;
  niv.view_dir = normalize(cam_data.cam_pos - normalized_global_pos);

  niv.world_pos = global.xyz / global.w;
  niv.cam_rel_pos = cam_rel_pos.xyz / cam_rel_pos.w;

  return niv;
}

struct pbr_light_data_t {
  vec3 to_light_dir;
  vec3 half_vector;
  float amount_in_light;
};

pbr_light_data_t create_pbr_light(norm_inter_vecs_t niv, vec3 to_light_dir, float amount_in_light) {
  pbr_light_data_t data;  
  data.to_light_dir = normalize(to_light_dir);
  data.amount_in_light = amount_in_light;
  data.half_vector = normalize(data.to_light_dir + niv.view_dir);
  return data;
}

float linearize_depth(spotlight_data_t light_data, vec4 light_rel_screen_pos) {
  // the z_pos is negative since light looks down the negative z axis
  float light_near = light_data.near_plane;
  float light_far = light_data.far_plane;
  float light_rel_z_pos = -light_rel_screen_pos.w;
  float linear_z = (-(light_rel_z_pos + light_near)) / (light_far - light_near);

  return linear_z * light_data.light_active;
}

vec4 quantize_color(vec4 c) {
  
  float num_bins = 32.0;
  float bin_size = 256.0 / num_bins;

  vec3 scaled_up = c.rgb * vec3(255.0);
  // ivec3 iscaled_up = ivec3(scaled_up.r, scaled_up.g, scaled_up.b);
  vec3 binned = scaled_up / vec3(bin_size);
  vec3 ceil_binned = vec3(floor(binned.x), floor(binned.y), floor(binned.z));
  vec3 quantized_color = ceil_binned * vec3(bin_size) / vec3(255.0);
  return vec4(quantized_color.rgb, 1.0);
#if 0
  vec4 quantized = vec4(0,0,0,1);
  quantized.x = (16 * (int(c.x * 255) / int(16))) / 255.0;
  quantized.y = (16 * (int(c.y * 255) / int(16))) / 255.0;
  quantized.z = (16 * (int(c.z * 255) / int(16))) / 255.0;
  return quantized;
#endif
}

struct is_in_spotlight_info_t {
  float amount_in_light;
  float depth;
  float closest_depth;
  vec2 tex_coords;
};

is_in_spotlight_info_t is_in_spotlight(norm_inter_vecs_t niv, spotlight_data_t light_data, vec4 light_rel_pos) {
  is_in_spotlight_info_t info;

  vec2 tex_coords = ((light_rel_pos.xy / light_rel_pos.w) + vec2(1)) / 2;
  tex_coords = tex_coords * vec2(light_data.light_active, light_data.light_active);
  info.tex_coords = tex_coords;

  float amount_in_light = 0.0;
  float bias = 0.00001;

  // z position of the vertex relative to the light, still [-1,1] for near to far
  info.depth = light_rel_pos.z / light_rel_pos.w;
  // depth [0,1] relative to light
  info.depth = light_data.light_active * ((info.depth+1)/2);
  info.closest_depth = 1;
  if (tex_coords.x >= 0 && tex_coords.x <= 1 && tex_coords.y >= 0 && tex_coords.y <= 1) {
    info.closest_depth = light_data.light_active * texture(light_data.depth_tex, tex_coords).r;
  }

  int pcf = 3;

  for (int x_offset = -(pcf/2); x_offset <= (pcf/2); x_offset++) {
    for (int y_offset = -(pcf/2); y_offset <= (pcf/2); y_offset++) {

      vec2 new_tex_coord = tex_coords + vec2(x_offset / light_data.shadow_map_width, y_offset / light_data.shadow_map_height);
      if (new_tex_coord.x < 0 || new_tex_coord.x > 1 || new_tex_coord.y < 0 || new_tex_coord.y > 1) continue;

      // depth buffer stores 0 to 1, for near to far respectively
      // so closest_depth is between 0 to 1
      float closest_depth = light_data.light_active * textureOffset(light_data.depth_tex, tex_coords, ivec2(x_offset, y_offset)).r;

      // z pos is closer to light than the texture sample says
      if (info.depth < (closest_depth + bias)) {
        // light
        amount_in_light += (1.0 / max(0.1, float(pcf * pcf)));
      }
    }
  }

  float albedo_factor = max(0, dot(niv.normal, normalize(light_data.pos - niv.world_pos)));
  info.amount_in_light = amount_in_light * albedo_factor * light_data.light_active;

  return info;
}

struct dir_light_rel_data_t {
  vec4 screen_rel_pos;
  // lower idx is higher precision
  int highest_precision_cascade;
};

// texture xy is in xy of return value
// depth is in z of return value
vec3 get_cascade_tex_depth_info(int cascade) {
  // looking down -z axis in camera's eye space
  mat4 light_projection = dir_light_data.light_projs[cascade];
  mat4 light_view = dir_light_data.light_views[cascade];
  vec4 screen_rel_pos = light_projection * light_view * global;
  screen_rel_pos /= screen_rel_pos.w;
  vec3 tex_plus_depth_info = (screen_rel_pos.xyz + vec3(1.0)) / 2.0;
  return tex_plus_depth_info;
}

struct is_in_dir_light_info_t {
  float amount_in_light;
  float closest_depth;
  float depth;
  vec3 tex_coords;
};

is_in_dir_light_info_t calc_light_rel_data() {

  is_in_dir_light_info_t in_dir_light_info;

  int highest_precision_cascade = NUM_CASCADES - 1;
  vec3 highest_prec_tex_plus_depth_info = vec3(0);

  for (int i = 0; i < NUM_CASCADES; i++) {
    vec3 tex_plus_depth_info = get_cascade_tex_depth_info(i);
    bool highest_prec = tex_plus_depth_info.x >= 0 && tex_plus_depth_info.x <= 1 && tex_plus_depth_info.y >= 0 && tex_plus_depth_info.y <= 1;
    if (highest_prec) {
      highest_precision_cascade = i; 
      highest_prec_tex_plus_depth_info = tex_plus_depth_info;
      break;
    }
  }

  vec3 tex_coords = vec3(highest_prec_tex_plus_depth_info.xy, highest_precision_cascade) * vec3(dir_light_data.light_active);
  in_dir_light_info.tex_coords = tex_coords;

  float amount_in_light = 0.0;

  float bias = 0.0005;
  bias = 0.00075;
  bias = 0.001;

  in_dir_light_info.depth = highest_prec_tex_plus_depth_info.z;

  int pcf = 3;

  ivec2 sm_dim = textureSize(dir_light_data.shadow_map, highest_precision_cascade).xy;
  for (int x_offset = -(pcf/2); x_offset <= (pcf/2); x_offset++) {
    for (int y_offset = -(pcf/2); y_offset <= (pcf/2); y_offset++) {

      vec3 new_tex_coord = tex_coords + vec3(x_offset / float(sm_dim.x), y_offset / float(sm_dim.y), 0);

      float depth;
      float closest_depth;
      
      if (new_tex_coord.x < 0 || new_tex_coord.x > 1 || new_tex_coord.y < 0 || new_tex_coord.y > 1) {
        if (highest_precision_cascade == NUM_CASCADES-1) continue;

        int less_precise_layer = highest_precision_cascade + 1;

        vec3 lesser_cascade_tex_depth_info = get_cascade_tex_depth_info(less_precise_layer);

        depth = lesser_cascade_tex_depth_info.z;

        vec3 new_frag_tex_coord = vec3(lesser_cascade_tex_depth_info.xy, less_precise_layer);
        closest_depth = dir_light_data.light_active * texture(dir_light_data.shadow_map, new_frag_tex_coord).r;
      } else {
        depth = highest_prec_tex_plus_depth_info.z;

        // depth buffer stores 0 to 1, for near to far respectively
        // so closest_depth is between 0 to 1
        closest_depth = dir_light_data.light_active * texture(dir_light_data.shadow_map, new_tex_coord).r;
      }

      // z pos is closer to light than the texture sample says
      if (depth <= (closest_depth + bias)) {
        // light
        amount_in_light += (1.0 / max(0.1, float(pcf * pcf)));
      }

      if (x_offset == 0 && y_offset == 0) {
        in_dir_light_info.closest_depth = closest_depth;
      }

    }
  } 

  in_dir_light_info.amount_in_light = min(max(0.0, amount_in_light), 1.0) * dir_light_data.light_active;

  return in_dir_light_info;
}

float normal_distrib(norm_inter_vecs_t niv, pbr_light_data_t pbr_light_data, float roughness) {
  float a = roughness * roughness;
  float a2 = a*a;
  float numerator = a2;
  float d = max(dot(niv.normal, pbr_light_data.half_vector), 0.0);
  float denom = PI * pow( ( pow(d,2) * ( a2-1.0 ) ) + 1, 2 );
  return numerator / denom;
}

float geom_helper(vec3 niv_normal, vec3 other_vec, float roughness) {
  float n_o = max(dot(niv_normal, other_vec), 0.0);
  float k = pow(roughness + 1, 2) / 8.0;
  float num = n_o;
  float den = ( n_o*(1-k) ) + k;
  return num / den;
}

float geom(norm_inter_vecs_t niv, pbr_light_data_t pbr_light_data, float roughness) {
  float a = geom_helper(niv.normal, niv.view_dir, roughness);
  float b = geom_helper(niv.normal, pbr_light_data.to_light_dir, roughness);
  return a*b;
}

vec3 fresnel_eq(norm_inter_vecs_t niv, pbr_light_data_t pbr_light_data, vec3 F0) {
  float hv = max(dot(pbr_light_data.half_vector, niv.view_dir), 0.0);
  float m = pow(clamp(1-hv, 0.0, 1.0), 5);
  vec3 neg_F0 = vec3(1.0) - F0;
  return F0 + (neg_F0 * m);
}

float cook_torrance_specular_brdf(norm_inter_vecs_t niv, pbr_light_data_t pbr_light_data) {
  float roughness = get_roughness();
  float D = normal_distrib(niv, pbr_light_data, roughness); 
  float G = geom(niv, pbr_light_data, roughness);

  float ln = max(dot(pbr_light_data.to_light_dir, niv.normal), 0.0);
  float vn = max(dot(niv.view_dir, niv.normal), 0.0);

  float denom = (4 * ln * vn) + 0.0001;
  float num = D * G;

  return num / denom;
}

vec3 lambert_diffuse() {
  vec4 diffuse; 
  if (material.base_color_tex.tex_id == -1) {
    if (material.use_base_color_tex == 0) {
      diffuse = vec4(material.mesh_color, 1);
    } else {
      diffuse = vec4(color, 1);
    }
    diffuse = vec4(0,1,0,1);
  } else {
    diffuse = texture(material.base_color_tex.samp, tex_coords[material.base_color_tex.tex_id]);
    // diffuse.r = s_rgb_to_linear(diffuse.r);
    // diffuse.g = s_rgb_to_linear(diffuse.g);
    // diffuse.b = s_rgb_to_linear(diffuse.b);
  }
  return diffuse.rgb;
}

float get_metalness() {
  float metalness;
  if (material.use_metal_rough_tex == 1) {
    vec4 info = texture(material.metal_rough_tex.samp, tex_coords[material.metal_rough_tex.tex_id]);
    metalness = s_rgb_to_linear(info.b);
  } else {
    metalness = material.metalness;
  }
  return metalness;
}

float get_roughness() {
  float roughness;
  if (material.use_metal_rough_tex == 1) {
    vec4 info = texture(material.metal_rough_tex.samp, tex_coords[material.metal_rough_tex.tex_id]);
    roughness = s_rgb_to_linear(info.g);
    // roughness = info.g;
  } else {
    roughness = material.surface_roughness;
  }
  return roughness;
}

vec3 get_base_reflectance() {
  vec3 dielectric_F0 = vec3(0.04, 0.04, 0.04);
  vec3 surface_color = lambert_diffuse();

  float metalness = get_metalness();

  vec3 refl = mix(dielectric_F0, surface_color, metalness);
  return refl;
}

vec3 pbr_for_light(norm_inter_vecs_t niv, pbr_light_data_t pbr_light_data) {
  vec3 F0 = get_base_reflectance();
  vec3 ks = fresnel_eq(niv, pbr_light_data, F0);

  float metalness = get_metalness();

  vec3 kd = vec3(1.0) - ks;
  kd *= (1.0 - metalness); 

  float cook = cook_torrance_specular_brdf(niv, pbr_light_data);
  vec3 diffuse = lambert_diffuse();
  vec3 brdf_output = (kd * diffuse / PI) + (ks * cook);

  float geom_term = max(dot(niv.normal, normalize(pbr_light_data.to_light_dir)), 0.0);
  vec3 pbr = brdf_output * pbr_light_data.amount_in_light * geom_term;
  return pbr;
}

vec3 pbr_brdf(norm_inter_vecs_t niv) {

  // dirlight
  is_in_dir_light_info_t in_dir0 = calc_light_rel_data();

  // spotlights
  is_in_spotlight_info_t in_spotlight0 = is_in_spotlight(niv, spotlights_data[0], spotlight_rel_screen_pos0);
  is_in_spotlight_info_t in_spotlight1 = is_in_spotlight(niv, spotlights_data[1], spotlight_rel_screen_pos1);
  is_in_spotlight_info_t in_spotlight2 = is_in_spotlight(niv, spotlights_data[2], spotlight_rel_screen_pos2);

  pbr_light_data_t pbr_lights[4];
  pbr_lights[0] = create_pbr_light(niv, -dir_light_data.light_dir, in_dir0.amount_in_light);
  pbr_lights[1] = create_pbr_light(niv, spotlights_data[0].pos - niv.world_pos, in_spotlight0.amount_in_light);
  pbr_lights[2] = create_pbr_light(niv, spotlights_data[1].pos - niv.world_pos, in_spotlight1.amount_in_light);
  pbr_lights[3] = create_pbr_light(niv, spotlights_data[2].pos - niv.world_pos, in_spotlight2.amount_in_light);

  // ambient light
  float ambient_factor = 0.03;
  vec3 diffuse = lambert_diffuse();
  vec3 color = vec3(ambient_factor) * diffuse;

  for (int i = 0; i < 4; i++) {
    color += pbr_for_light(niv, pbr_lights[i]);
  }

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0/2.2));

  return color;
}

void main() {

  norm_inter_vecs_t niv = calc_normalized_vectors();

  vec3 pbr = pbr_brdf(niv);
  frag_color = vec4(pbr, 1.0);

#if VIEW_AMOUNT_IN_LIGHT
  frag_color = vec4(max_in_light, max_in_light, max_in_light, 1);
#elif VIEW_LIGHT_MULTIPLIER
  frag_color = vec4(multiplier, multiplier, multiplier, 1);
#elif VIEW_LIGHT0_CLOSEST_DEPTH
  float v = pow(in_spotlight0.closest_depth, 80);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT1_CLOSEST_DEPTH
  float v = pow(in_spotlight1.closest_depth, 80);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT2_CLOSEST_DEPTH
  float v = pow(in_spotlight2.closest_depth, 80);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT0_DEPTH
  float v = linearize_depth(spotlights_data[0], spotlight_rel_screen_pos0);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT1_DEPTH
  float v = linearize_depth(spotlights_data[1], spotlight_rel_screen_pos1);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT2_DEPTH
  float v = linearize_depth(spotlights_data[2], spotlight_rel_screen_pos2);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT0_CALCULATED_UVs
  frag_color = vec4(in_spotlight0.tex_coords.x,0,0,1);
  frag_color = vec4(0,in_spotlight0.tex_coords.y,0,1);
  // frag_color = vec4(in_spotlight0.tex_coords.xy,0,1);
#elif VIEW_LIGHT1_CALCULATED_UVs
  frag_color = vec4(in_spotlight1.tex_coords.x,0,0,1);
  frag_color = vec4(0,in_spotlight1.tex_coords.y,0,1);
  // frag_color = vec4(in_spotlight1.tex_coords.xy,0,1);
#elif VIEW_LIGHT2_CALCULATED_UVs
  frag_color = vec4(in_spotlight2.tex_coords.x,0,0,1);
  frag_color = vec4(0,in_spotlight2.tex_coords.y,0,1);
  // frag_color = vec4(in_spotlight2.tex_coords.xy,0,1);
#elif VIEW_LIGHT0_AMOUNT_IN_LIGHT
  frag_color = vec4(in_spotlight0.amount_in_light,in_spotlight0.amount_in_light,in_spotlight0.amount_in_light,1);
#elif VIEW_LIGHT1_AMOUNT_IN_LIGHT
  frag_color = vec4(in_spotlight1.amount_in_light,in_spotlight1.amount_in_light,in_spotlight1.amount_in_light,1);
#elif VIEW_LIGHT2_AMOUNT_IN_LIGHT
  frag_color = vec4(in_spotlight2.amount_in_light,in_spotlight2.amount_in_light,in_spotlight2.amount_in_light,1);
#elif VIEW_NORMALS
  frag_color = vec4(niv.normal, 1.0);
#elif VIEW_DIR_LIGHT_CLOSEST_DEPTH
  float v = pow(in_dir0.closest_depth, 2);
  frag_color = vec4(v,v,v,1);
#elif VIEW_DIR_LIGHT_DEPTH
  float v = pow(in_dir0.depth, 1);
  frag_color = vec4(v,v,v,1);
#elif VIEW_DIR_LIGHT_AMOUNT_IN_LIGHT
  float ail = in_dir0.amount_in_light;
  frag_color = vec4(ail,ail,ail,1);
#elif VIEW_CASCADE
  int dir_light_layer = dir_light_rel_data.highest_precision_cascade;
  if (dir_light_layer == 0) {
    frag_color = vec4(1,0,0,1);
  } else if (dir_light_layer == 1) {
    frag_color = vec4(0,1,0,1);
  } else if (dir_light_layer == 2) {
    frag_color = vec4(0,0,1,1);
  } else {
    frag_color = vec4(1,1,1,1);
  }
#endif 

#if ENABLE_QUANTIZING
  frag_color = quantize_color(frag_color);
#endif 

  if (override_color_bool == 1) {
    frag_color = vec4(1,1,1,1);
  }
}
