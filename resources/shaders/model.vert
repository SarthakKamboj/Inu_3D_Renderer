#version 410 core

#define NUM_CASCADES 3

layout (location = 0) in vec3 vert_pos;
layout (location = 1) in vec2 tex0;
layout (location = 2) in vec2 tex1;
layout (location = 3) in vec3 vert_color;
layout (location = 4) in vec4 joints;
layout (location = 5) in vec4 weights;
layout (location = 6) in vec3 vert_normal;

uniform int skinned;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat4 joint_model_matricies[80];
uniform mat4 joint_inverse_bind_mats[80];

// light 4x4 matrix info
struct light_mat_data_t {
  mat4 light_view;
  mat4 light_projection;
};

#if 0
struct dir_light_mat_data_t {
  mat4 light_views[NUM_CASCADES];
  mat4 light_projs[NUM_CASCADES];
  float cascade_depths[NUM_CASCADES+1];
};
uniform dir_light_mat_data_t dir_light_mat_data;
#endif

uniform light_mat_data_t lights_mat_data[3];

out vec2 tex_coords[2];
out vec3 color;
out vec4 normal;
out vec4 pos;

out vec4 light_rel_screen_pos0;
out vec4 light_rel_screen_pos1;
out vec4 light_rel_screen_pos2;

#if 0
out vec4 dir_light_rel_screen_pos;
flat out int dir_light_layer;
#else
out vec4 global;
out vec4 cam_rel_pos;
#endif

struct light_rel_data_t {
  vec4 screen_rel_pos;
};

light_rel_data_t calc_light_rel_data(mat4 light_projection, mat4 light_view, mat4 model) {
  light_rel_data_t data;
  data.screen_rel_pos = light_projection * light_view * model * vec4(vert_pos, 1.0);
  return data;
}

#if 0
struct dir_light_rel_data_t {
  vec4 screen_rel_pos;
  // lower idx is higher precision
  int highest_precision_cascade;
};

dir_light_rel_data_t calc_light_rel_data(dir_light_mat_data_t dir_light_mat_data, mat4 model) {
  dir_light_rel_data_t rel_data;
  // rel_data.highest_precision_cascade = -1;
  rel_data.highest_precision_cascade = 0;

  vec4 cam_rel_pos = view * model * vec4(vert_pos, 1.0);
  cam_rel_pos = cam_rel_pos / cam_rel_pos.w;

  for (int i = 0; i < NUM_CASCADES; i++) {
    // looking down -z axis in camera's eye space
    if (-cam_rel_pos.z >= dir_light_mat_data.cascade_depths[i] && -cam_rel_pos.z <= dir_light_mat_data.cascade_depths[i+1]) {
      rel_data.highest_precision_cascade = i; 
      break;
    }
  }

  mat4 light_projection = dir_light_mat_data.light_projs[rel_data.highest_precision_cascade];
  mat4 light_view = dir_light_mat_data.light_views[rel_data.highest_precision_cascade];
  rel_data.screen_rel_pos = light_projection * light_view * model * vec4(vert_pos, 1.0);
  return rel_data;
}
#endif

void main() {

  mat4 final_model = mat4(0.0);
  if (skinned == 1) {
    uint ui = 0;
    mat4 joint_model_mat = mat4(0.0);
    mat4 scaled_jmm = mat4(0.0);

    // 1st joint
    ui = uint(joints.x);
    joint_model_mat = joint_model_matricies[ui] * joint_inverse_bind_mats[ui];
    scaled_jmm = joint_model_mat * weights.x;
    final_model += scaled_jmm;

    // 2nd joint
    ui = uint(joints.y);
    joint_model_mat = joint_model_matricies[ui] * joint_inverse_bind_mats[ui];
    scaled_jmm = joint_model_mat * weights.y;
    final_model += scaled_jmm;

    // 3rd joint
    ui = uint(joints.z);
    joint_model_mat = joint_model_matricies[ui] * joint_inverse_bind_mats[ui];
    scaled_jmm = joint_model_mat * weights.z;
    final_model += scaled_jmm;

    // 4th joint
    ui = uint(joints.w);
    joint_model_mat = joint_model_matricies[ui] * joint_inverse_bind_mats[ui];
    scaled_jmm = joint_model_mat * weights.w;
    final_model += scaled_jmm; 
  } else {
    final_model = model;
  }

  global = final_model * vec4(vert_pos, 1.0);
  cam_rel_pos = view * global;
  gl_Position = projection * cam_rel_pos;

  tex_coords[0] = tex0;
  tex_coords[1] = tex1;
  color = vert_color;
  // must convert normal to global version
  normal = transpose(inverse(final_model)) * vec4(vert_normal, 0.0);
  pos = global;

  light_rel_data_t light_rel_data0 = calc_light_rel_data(lights_mat_data[0].light_projection, lights_mat_data[0].light_view, final_model);
  light_rel_screen_pos0 = light_rel_data0.screen_rel_pos;

  light_rel_data_t light_rel_data1 = calc_light_rel_data(lights_mat_data[1].light_projection, lights_mat_data[1].light_view, final_model);
  light_rel_screen_pos1 = light_rel_data1.screen_rel_pos;

  light_rel_data_t light_rel_data2 = calc_light_rel_data(lights_mat_data[2].light_projection, lights_mat_data[2].light_view, final_model);
  light_rel_screen_pos2 = light_rel_data2.screen_rel_pos;

#if 0
  dir_light_rel_data_t dir_rel_data = calc_light_rel_data(dir_light_mat_data, final_model);
  dir_light_rel_screen_pos = dir_rel_data.screen_rel_pos;
  dir_light_layer = dir_rel_data.highest_precision_cascade;
#endif
}

