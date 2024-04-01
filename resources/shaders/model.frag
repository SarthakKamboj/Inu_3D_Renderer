#version 410 core

#define NUM_CASCADES 3

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
#define VIEW_NORMALS 0
#define VIEW_DIR_LIGHT_CLOSEST_DEPTH 0
#define VIEW_DIR_LIGHT_DEPTH 0
#define VIEW_DIR_LIGHT_AMOUNT_IN_LIGHT 0
#define ENABLE_QUANTIZING 0

struct shader_tex {
  sampler2D samp; 
  int tex_id;
};

uniform shader_tex base_color_tex;
uniform vec3 mesh_color;
uniform int use_mesh_color;

uniform int override_color_bool;

struct light_data_t {
  vec3 pos;
  sampler2D depth_tex;
  int light_active;
  int shadow_map_width;
  int shadow_map_height;
  float near_plane;
  float far_plane;
};

struct dir_light_mat_data_t {
  mat4 light_views[NUM_CASCADES];
  mat4 light_projs[NUM_CASCADES];
  float cascade_depths[NUM_CASCADES+1];
};
uniform dir_light_mat_data_t dir_light_mat_data;

struct dir_light_data_t {
  sampler2DArray shadow_map;
  int light_active;
};

uniform light_data_t lights_data[3];
uniform dir_light_data_t dir_light_data;

in vec2 tex_coords[2];
in vec3 color;
in vec4 normal;

in vec4 pos;

in vec4 light_rel_screen_pos0;
in vec4 light_rel_screen_pos1;
in vec4 light_rel_screen_pos2;

#if 0
in vec4 dir_light_rel_screen_pos;
flat in int dir_light_layer;
#else
in vec4 global;
in vec4 cam_rel_pos;
#endif

out vec4 frag_color;

float linearize_depth(light_data_t light_data, vec4 light_rel_screen_pos) {
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

struct is_in_light_info_t {
  float amount_in_light;
  float depth;
  float closest_depth;
  vec2 tex_coords;
};

is_in_light_info_t is_in_light(light_data_t light_data, vec4 light_rel_pos) {
  is_in_light_info_t info;

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

#if 0
  info.amount_in_light = min(max(0.0, amount_in_light), 1.0) * light_data.light_active;
#else
  vec4 normalized_pos = pos / pos.w;
  vec4 normal_norm = normal / normal.w;
  float albedo_factor = max(0, dot(normalize(normal.xyz), normalize(light_data.pos - normalized_pos.xyz)));
  info.amount_in_light = amount_in_light * albedo_factor;
#endif

  return info;
}

struct is_in_dir_light_info_t {
  float amount_in_light;
  float closest_depth;
  float depth;
  vec3 tex_coords;
};

is_in_dir_light_info_t is_in_dir_light(dir_light_data_t dir_light_data, vec4 dir_light_rel_screen_pos, int dir_light_layer) {
  is_in_dir_light_info_t info;

  vec2 tex_coords = ((dir_light_rel_screen_pos.xy / dir_light_rel_screen_pos.w) + vec2(1)) / 2;
  tex_coords = tex_coords * vec2(dir_light_data.light_active, dir_light_data.light_active);
  info.tex_coords = vec3(tex_coords, dir_light_layer);

  float amount_in_light = 0.0;
  float bias = 0.001;

  // z position of the vertex relative to the light, still [-1,1] for near to far
  info.depth = dir_light_rel_screen_pos.z / dir_light_rel_screen_pos.w;
  // depth [0,1] relative to light
  info.depth = dir_light_data.light_active * ((info.depth+1)/2);

#if 0
  info.closest_depth = 1;
  if (info.tex_coords.x >= 0 && info.tex_coords.x <= 1 && info.tex_coords.y >= 0 && info.tex_coords.y <= 1) {
    info.closest_depth = dir_light_data.light_active * texture(dir_light_data.shadow_map, info.tex_coords).r;
  }

  if (info.depth <= (info.closest_depth + bias)) {
    // light
    info.amount_in_light = 1.0;
  } else {
    info.amount_in_light = 0.3;
  }
#else
  int pcf = 3;

  ivec2 sm_dim = textureSize(dir_light_data.shadow_map, dir_light_layer).xy;
  for (int x_offset = -(pcf/2); x_offset <= (pcf/2); x_offset++) {
    for (int y_offset = -(pcf/2); y_offset <= (pcf/2); y_offset++) {

      vec2 new_tex_coord = tex_coords + vec2(x_offset / float(sm_dim.x), y_offset / float(sm_dim.y));

      if (new_tex_coord.x < 0 || new_tex_coord.x > 1 || new_tex_coord.y < 0 || new_tex_coord.y > 1) {
        if (dir_light_layer == NUM_CASCADES-1) continue;
        int less_precise_layer = dir_light_layer + 1;

        mat4 light_projection = dir_light_mat_data.light_projs[less_precise_layer];
        mat4 light_view = dir_light_mat_data.light_views[less_precise_layer];
        vec4 new_screen_rel_pos = light_projection * light_view * global;
        vec4 normed_info = ((new_screen_rel_pos / new_screen_rel_pos.w) + vec4(1)) / 2;
        vec2 new_frag_tex_coord = normed_info.xy;
        float new_frag_depth = normed_info.z;

        float closest_depth = dir_light_data.light_active * texture(dir_light_data.shadow_map, vec3(new_frag_tex_coord, less_precise_layer)).r;
        // z pos is closer to light than the texture sample says
        if (new_frag_depth <= (closest_depth + bias)) {
          // light
          amount_in_light += (1.0 / max(0.1, float(pcf * pcf)));
        }
      } else {

        // depth buffer stores 0 to 1, for near to far respectively
        // so closest_depth is between 0 to 1
        info.closest_depth = dir_light_data.light_active * texture(dir_light_data.shadow_map, vec3(new_tex_coord, dir_light_layer)).r;

        // z pos is closer to light than the texture sample says
        if (info.depth <= (info.closest_depth + bias)) {
          // light
          amount_in_light += (1.0 / max(0.1, float(pcf * pcf)));
        }
      }
    }
  }
  info.amount_in_light = min(max(0.0, amount_in_light), 1.0) * dir_light_data.light_active;
#endif

  return info;
}

struct dir_light_rel_data_t {
  vec4 screen_rel_pos;
  // lower idx is higher precision
  int highest_precision_cascade;
};

dir_light_rel_data_t calc_light_rel_data(dir_light_mat_data_t dir_light_mat_data) {
  dir_light_rel_data_t rel_data;
  // rel_data.highest_precision_cascade = -1;
  rel_data.highest_precision_cascade = 0;

  vec4 norm_cam_rel_pos = cam_rel_pos / cam_rel_pos.w;

#if 0
  for (int i = 0; i < NUM_CASCADES; i++) {
    // looking down -z axis in camera's eye space
    if (-norm_cam_rel_pos.z >= dir_light_mat_data.cascade_depths[i] && -norm_cam_rel_pos.z <= dir_light_mat_data.cascade_depths[i+1]) {
      rel_data.highest_precision_cascade = i; 
      break;
    }
  }
#else
  for (int i = 0; i < NUM_CASCADES; i++) {
    // looking down -z axis in camera's eye space
    mat4 light_projection = dir_light_mat_data.light_projs[i];
    mat4 light_view = dir_light_mat_data.light_views[i];
    vec4 screen_rel_pos = light_projection * light_view * global;
    vec2 tex_coords = ((screen_rel_pos.xy / screen_rel_pos.w) + vec2(1)) / 2;
    bool highest_prec = tex_coords.x >= 0 && tex_coords.x <= 1 && tex_coords.y >= 0 && tex_coords.y <= 1;
    if (highest_prec) {
      rel_data.highest_precision_cascade = i; 
      break;
    }
  }
#endif

  mat4 light_projection = dir_light_mat_data.light_projs[rel_data.highest_precision_cascade];
  mat4 light_view = dir_light_mat_data.light_views[rel_data.highest_precision_cascade];
  rel_data.screen_rel_pos = light_projection * light_view * global;
  return rel_data;
}

void main() {

  if (base_color_tex.tex_id == -1) {
    if (use_mesh_color == 1) {
      frag_color = vec4(mesh_color, 1);
    } else {
      frag_color = vec4(color, 1);
    }
  } else {
    frag_color = texture(base_color_tex.samp, tex_coords[base_color_tex.tex_id]);
  }

  is_in_light_info_t in_light0 = is_in_light(lights_data[0],  light_rel_screen_pos0);
  is_in_light_info_t in_light1 = is_in_light(lights_data[1], light_rel_screen_pos1);
  is_in_light_info_t in_light2 = is_in_light(lights_data[2], light_rel_screen_pos2);

  dir_light_rel_data_t dir_light_rel_data = calc_light_rel_data(dir_light_mat_data);
  is_in_dir_light_info_t in_dir0 = is_in_dir_light(dir_light_data, dir_light_rel_data.screen_rel_pos, dir_light_rel_data.highest_precision_cascade);

  float max_in_light = max(max(max(in_light0.amount_in_light, in_light1.amount_in_light), in_light2.amount_in_light), in_dir0.amount_in_light);

  // float shadow_damp_factor = 0.2;
  // float multiplier = ((1.0 - max_in_light) * shadow_damp_factor) + max_in_light;

  // frag_color = vec4(normal, 1.0);
  float ambient_factor = 0.2;
#if 0
  vec4 normalized_pos = pos / pos.w;
  vec4 normal_norm = normal / normal.w;
  float albedo_factor = max(0, dot(normalize(normal.xyz), normalize(lights_data[0].pos - normalized_pos.xyz)));
  float multiplier = ambient_factor + (max_in_light * albedo_factor);
#else
  float multiplier = ambient_factor + max_in_light;
#endif
  multiplier = max(0, min(1, multiplier));

  frag_color.x *= multiplier;
  frag_color.y *= multiplier;
  frag_color.z *= multiplier; 


#if VIEW_AMOUNT_IN_LIGHT
  frag_color = vec4(max_in_light, max_in_light, max_in_light, 1);
#elif VIEW_LIGHT_MULTIPLIER
  frag_color = vec4(multiplier, multiplier, multiplier, 1);
#elif VIEW_LIGHT0_CLOSEST_DEPTH
  float v = pow(in_light0.closest_depth, 80);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT1_CLOSEST_DEPTH
  float v = pow(in_light1.closest_depth, 80);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT2_CLOSEST_DEPTH
  float v = pow(in_light2.closest_depth, 80);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT0_DEPTH
  float v = linearize_depth(lights_data[0], light_rel_screen_pos0);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT1_DEPTH
  float v = linearize_depth(lights_data[1], light_rel_screen_pos1);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT2_DEPTH
  float v = linearize_depth(lights_data[2], light_rel_screen_pos2);
  frag_color = vec4(v,v,v,1);
#elif VIEW_LIGHT0_CALCULATED_UVs
  frag_color = vec4(in_light0.tex_coords.x,0,0,1);
  frag_color = vec4(0,in_light0.tex_coords.y,0,1);
  // frag_color = vec4(in_light0.tex_coords.xy,0,1);
#elif VIEW_LIGHT1_CALCULATED_UVs
  frag_color = vec4(in_light1.tex_coords.x,0,0,1);
  frag_color = vec4(0,in_light1.tex_coords.y,0,1);
  // frag_color = vec4(in_light1.tex_coords.xy,0,1);
#elif VIEW_LIGHT2_CALCULATED_UVs
  frag_color = vec4(in_light2.tex_coords.x,0,0,1);
  frag_color = vec4(0,in_light2.tex_coords.y,0,1);
  // frag_color = vec4(in_light2.tex_coords.xy,0,1);
#elif VIEW_NORMALS
  frag_color = normal_norm;
#elif VIEW_DIR_LIGHT_CLOSEST_DEPTH
  float v = pow(in_dir0.closest_depth, 2);
  frag_color = vec4(v,v,v,1);
#elif VIEW_DIR_LIGHT_DEPTH
  float v = pow(in_dir0.depth, 1);
  frag_color = vec4(v,v,v,1);
#elif VIEW_DIR_LIGHT_AMOUNT_IN_LIGHT
  float ail = in_dir0.amount_in_light;
  frag_color = vec4(ail,ail,ail,1);
#endif 

#if ENABLE_QUANTIZING
  frag_color = quantize_color(frag_color);
#endif 

  if (override_color_bool == 1) {
    frag_color = vec4(1,1,1,1);
  }

#if 0
  int dir_light_layer = dir_light_rel_data.highest_precision_cascade;
  if (dir_light_layer == 0) {
    frag_color = vec4(1,0,0,1);
  } 
  else if (dir_light_layer == 1) {
    frag_color = vec4(0,1,0,1);
  } else if (dir_light_layer == 2) {
    frag_color = vec4(0,0,1,1);
  } else {
    frag_color = vec4(1,1,1,1);
  }

  // frag_color = vec4(mesh_color, 1);
#endif
}
