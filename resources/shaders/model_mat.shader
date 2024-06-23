
uniform mat4 model;
uniform int skinned;

uniform mat4 joint_model_matricies[80];
uniform mat4 joint_inverse_bind_mats[80];

mat4 get_model_mat(vec4 joints, vec4 weights) {
  
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

  return final_model;
}
