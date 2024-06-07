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
struct spotlight_mat_data_t {
  mat4 light_view;
  mat4 light_projection;
};

uniform spotlight_mat_data_t spotlights_mat_data[3];

out VS_OUT {
  vec2 tex_coords[2];
  vec3 color;

  vec4 normal;

  vec4 spotlight_rel_screen_pos0;
  vec4 spotlight_rel_screen_pos1;
  vec4 spotlight_rel_screen_pos2;

  vec4 global;
  vec4 cam_rel_pos;
} output_data;

#if 0
out vec2 tex_coords[2];
out vec3 color;

out vec4 normal;

out vec4 spotlight_rel_screen_pos0;
out vec4 spotlight_rel_screen_pos1;
out vec4 spotlight_rel_screen_pos2;

out vec4 global;
out vec4 cam_rel_pos;
#endif

vec4 calc_spotlight_rel_pos(mat4 light_projection, mat4 light_view, mat4 model) {
  return light_projection * light_view * model * vec4(vert_pos, 1.0);
}

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

  output_data.global = final_model * vec4(vert_pos, 1.0);
  output_data.cam_rel_pos = view * output_data.global;
  gl_Position = projection * output_data.cam_rel_pos;

  output_data.tex_coords[0] = tex0;
  output_data.tex_coords[1] = tex1;
  output_data.color = vert_color;
  // must convert normal to global version
  output_data.normal = transpose(inverse(final_model)) * vec4(vert_normal, 0.0);

  output_data.spotlight_rel_screen_pos0 = calc_spotlight_rel_pos(spotlights_mat_data[0].light_projection, spotlights_mat_data[0].light_view, final_model);
  output_data.spotlight_rel_screen_pos1 = calc_spotlight_rel_pos(spotlights_mat_data[1].light_projection, spotlights_mat_data[1].light_view, final_model);
  output_data.spotlight_rel_screen_pos2 = calc_spotlight_rel_pos(spotlights_mat_data[2].light_projection, spotlights_mat_data[2].light_view, final_model);
}

