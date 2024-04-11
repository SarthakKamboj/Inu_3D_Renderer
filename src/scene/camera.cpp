#include "camera.h"

#include "utils/mats.h"
#include "utils/vectors.h"
#include "windowing/window.h"
#include "utils/general.h"
#include "utils/log.h"
#include "gfx/light.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include <math.h>

static const float cam_far_plane = 100.f;

extern bool update_dir_light_frustums;

static camera_t cam;

extern window_t window;

void update_cam() {
  bool rotate_enabled = window.input.mouse_pos_diff.length() != 0 && window.input.middle_mouse_down;
  bool translate_enabled = rotate_enabled && window.input.shift_down;
  bool zoomed_enabled = window.input.scroll_wheel_delta != 0;

  if (zoomed_enabled) {
    vec3 diff;
    diff.x = cam.transform.pos.x - cam.focal_pt.x;
    diff.y = cam.transform.pos.y - cam.focal_pt.y;
    diff.z = cam.transform.pos.z - cam.focal_pt.z;
    float l = diff.length();
    cam_zoom(window.input.scroll_wheel_delta * l / 20.f);
  } else if (translate_enabled) {
    float sensitivity = 0.1f;
    float lat = window.input.mouse_pos_diff.x * -2.f * sensitivity;
    float vert = window.input.mouse_pos_diff.y * 1.f * sensitivity;
    cam_translate(lat, vert); 
  } else if (rotate_enabled) {
    float sensitivity = 0.3f;
    float lat = window.input.mouse_pos_diff.x * -2.f * sensitivity;
    float vert = window.input.mouse_pos_diff.y * 1.f * sensitivity;
    cam_rotate(lat, vert);
  } 
  

  if (update_dir_light_frustums) {
    cam.far_plane = cam_far_plane;
  } else {
    cam.far_plane = cam_far_plane * 2.f;
  }

#define USE_PERS 1

  // cam.view = get_cam_view_mat();
  cam.view = get_view_mat(cam.transform.pos, cam.focal_pt);
#if USE_PERS
  cam.proj = proj_mat(60.f, cam.near_plane, cam.far_plane, static_cast<float>(window.window_dim.x) / window.window_dim.y);
#else
  float mid = (cam.near_plane + cam.far_plane) / 2.f;
  float multipler = 3.141526f / 180.f;
  float top = mid * tan(60.f * multipler / 2.f);
  float right = mid * static_cast<float>(window.window_dim.x) / window.window_dim.y;
  cam.proj = ortho_mat(-right, right, -top, top, cam.near_plane, cam.far_plane);
#endif
  // cam.proj = proj_mat(60.f, cam.near_plane, cam.far_plane, static_cast<float>(window.window_dim.x) / window.window_dim.y);
#undef USE_PERS
}

void create_camera(transform_t& t) {
  cam.near_plane = 0.01f;
  cam.far_plane = cam_far_plane;

  cam.transform.pos.x = t.pos.x;
  cam.transform.pos.y = t.pos.y;
  cam.transform.pos.z = t.pos.z;

  // scale doesn't matter
  cam.transform.scale.x = 1.f;
  cam.transform.scale.y = 1.f;
  cam.transform.scale.z = 1.f;

  // quat rot doesn't matter either b/c using on focal pt based rot logic
  cam.transform.rot.x = 0;
  cam.transform.rot.y = 0;
  cam.transform.rot.z = 0;
  cam.transform.rot.w = 1;
}

mat4 get_view_mat(vec3 pos, vec3 focal_pt) {
#if !USE_GLM
  vec3 inv_t = {-pos.x, -pos.y, -pos.z};
  mat4 inv_translate = translate_mat(inv_t);

  vec3 to_fp;
  to_fp.x = focal_pt.x - pos.x;
  to_fp.y = focal_pt.y - pos.y;
  to_fp.z = focal_pt.z - pos.z;
  to_fp = normalize(to_fp);

  vec3 world_up = {0,1,0};
  vec3 right;
  if (to_fp.x == 0 && to_fp.y == -1.f && to_fp.z == 0) {
    right = {-1,0,0};
  } else if (to_fp.x == 0 && to_fp.y == 1.f && to_fp.z == 0) {
    right = {1,0,0};
  } else {
    right = cross_product(to_fp, world_up);
  }

  right = normalize(right);
  vec3 up = cross_product(right, to_fp);
  up = normalize(up);

  mat4 rot_mat(1.0f);

  rot_mat.first_col.x = right.x;
  rot_mat.first_col.y = right.y;
  rot_mat.first_col.z = right.z;

  rot_mat.second_col.x = up.x;
  rot_mat.second_col.y = up.y;
  rot_mat.second_col.z = up.z;

  vec3 neg_to_fp = {-to_fp.x, -to_fp.y, -to_fp.z};
  rot_mat.third_col.x = neg_to_fp.x;
  rot_mat.third_col.y = neg_to_fp.y;
  rot_mat.third_col.z = neg_to_fp.z;

  mat4 inv_rot = rot_mat.transpose();
  return inv_rot * inv_translate;
#else
  // glm::mat4 l = glm::lookAt(glm::vec3(pos.x, pos.y, pos.z), glm::vec3(focal_pt.x, focal_pt.y,1focal_pt.z), glm::vec3(0, -1.0f, 0));
  glm::mat4 l = glm::lookAt(glm::vec3(pos.x, pos.y, pos.z), glm::vec3(focal_pt.x, focal_pt.y, focal_pt.z), glm::vec3(0, 1.0f, 0));
  mat4 l_out(1.0f);
  glm_to_internal(l, l_out);
  return l_out;
#endif
}

mat4 get_cam_proj_mat() {
  return cam.proj;
}

mat4 get_cam_view_mat() {
#if CHANGING_REF_TO_FIT
  vec3 inv_t = {-cam.transform.pos.x, -cam.transform.pos.y, -cam.transform.pos.z};
  mat4 inv_translate = translate_mat(inv_t);

  vec3 to_fp;
  to_fp.x = cam.focal_pt.x - cam.transform.pos.x;
  to_fp.y = cam.focal_pt.y - cam.transform.pos.y;
  to_fp.z = cam.focal_pt.z - cam.transform.pos.z;
  to_fp = normalize(to_fp);

  vec3 world_up = {0,1,0};
  vec3 right = cross_product(to_fp, world_up);

  vec3 neg_to_fp = {-to_fp.x, -to_fp.y, -to_fp.z};
  vec3 up = cross_product(right, to_fp);

  mat4 rot_mat(1.0f);

  rot_mat.first_col.x = right.x;
  rot_mat.first_col.y = right.y;
  rot_mat.first_col.z = right.z;

  rot_mat.second_col.x = up.x;
  rot_mat.second_col.y = up.y;
  rot_mat.second_col.z = up.z;

  rot_mat.third_col.x = neg_to_fp.x;
  rot_mat.third_col.y = neg_to_fp.y;
  rot_mat.third_col.z = neg_to_fp.z;

  mat4 inv_rot = transpose(rot_mat);
  return mat_multiply_mat(inv_rot, inv_translate);
#elif 0
  return get_view_mat(cam.transform.pos, cam.focal_pt);
#else
  return cam.view;
#endif
}

#if 0
mat4 get_cam_view_mat(vec3& diff) {
  cam.transform.pos.x += diff.x;
  cam.transform.pos.y += diff.y;
  cam.transform.pos.z += diff.z;
  return get_cam_view_mat();
}
#endif



void cam_translate(float lat, float vert) {
  vec2 offset_dir = {lat, vert};
  offset_dir = normalize(offset_dir);

  vec3 to_fp;
  to_fp.x = cam.focal_pt.x - cam.transform.pos.x;
  to_fp.y = cam.focal_pt.y - cam.transform.pos.y;
  to_fp.z = cam.focal_pt.z - cam.transform.pos.z;

  float to_fp_dist = to_fp.length();

  to_fp = normalize(to_fp);

#if 1
  vec3 forward_dir = {to_fp.x, 0, to_fp.z};
  vec3 world_up = {0,1,0};
  vec3 right = cross_product(forward_dir, world_up);

  vec3 diff = {0,0,0};

  diff.x += right.x * lat;
  diff.y += right.y * lat;
  diff.z += right.z * lat;

  diff.x += forward_dir.x * vert;
  diff.y += forward_dir.y * vert;
  diff.z += forward_dir.z * vert;

  diff = diff * to_fp_dist / 20.f;

  cam.transform.pos.x += diff.x;
  cam.transform.pos.y += diff.y;
  cam.transform.pos.z += diff.z;

  cam.focal_pt.x += diff.x;
  cam.focal_pt.y += diff.y;
  cam.focal_pt.z += diff.z;

#else
  vec3 world_up = {0,1,0};
  vec3 right;
  if (to_fp.x == 0 && to_fp.y == -1.f && to_fp.z == 0) {
    right = {-1,0,0};
  } else if (to_fp.x == 0 && to_fp.y == 1.f && to_fp.z == 0) {
    right = {1,0,0};
  } else {
    right = cross_product(to_fp, world_up);
  }

  vec3 diff = {0,0,0};
  diff.x += to_fp.x * offset_dir.y;
  diff.y += to_fp.y * offset_dir.y;
  diff.z += to_fp.z * offset_dir.y;

  diff.x += right.x * offset_dir.x;
  diff.y += right.y * offset_dir.x;
  diff.z += right.z * offset_dir.x;

  cam.transform.pos.x += diff.x;
  cam.transform.pos.y += diff.y;
  cam.transform.pos.z += diff.z;
#endif

}

void cam_zoom(float amount) {
  vec3 to_fp;
  to_fp.x = cam.focal_pt.x - cam.transform.pos.x;
  to_fp.y = cam.focal_pt.y - cam.transform.pos.y;
  to_fp.z = cam.focal_pt.z - cam.transform.pos.z;
  to_fp = normalize(to_fp);

    vec3 neg_to_fp = {-to_fp.x, -to_fp.y, -to_fp.z};
  float s = dot(cam.transform.pos, neg_to_fp);
  int side = sgn(s);

  inu_assert(side == 1);
  // if (side != 1) return;

  vec3 diff;
  diff.x = to_fp.x * amount;
  diff.y = to_fp.y * amount;
  diff.z = to_fp.z * amount;

  cam.transform.pos.x += diff.x;
  cam.transform.pos.y += diff.y;
  cam.transform.pos.z += diff.z;

  float new_s = dot(cam.transform.pos, neg_to_fp);
  float new_side = sgn(new_s);

  if (new_side != side) {
    cam.transform.pos.x -= diff.x;
    cam.transform.pos.y -= diff.y;
    cam.transform.pos.z -= diff.z;
  }
}

void cam_rotate(float lat_amount, float vert_amount) {
  vec3 to_fp;
  to_fp.x = cam.focal_pt.x - cam.transform.pos.x;
  to_fp.y = cam.focal_pt.y - cam.transform.pos.y;
  to_fp.z = cam.focal_pt.z - cam.transform.pos.z;

  vec3 n_to_fp = {-to_fp.x, -to_fp.y, -to_fp.z};

  to_fp = normalize(to_fp);

  vec3 focal_pt_up = {0,1,0};
  vec3 right = cross_product(to_fp, focal_pt_up);
  quaternion_t q_right = create_quaternion_w_rot(right, vert_amount);
  quaternion_t q_up = create_quaternion_w_rot(focal_pt_up, lat_amount);
  quaternion_t q = quat_multiply_quat(q_right, q_up);

  vec3 new_n_to_fp = get_rotated_position(n_to_fp, q);
  cam.transform.pos.x = new_n_to_fp.x + cam.focal_pt.x;
  cam.transform.pos.y = new_n_to_fp.y + cam.focal_pt.y;
  cam.transform.pos.z = new_n_to_fp.z + cam.focal_pt.z;
}

camera_t* get_cam() {
  return &cam; 
}

float get_cam_near_plane() {
  return cam.near_plane;
}

float get_cam_far_plane() {
  return cam.far_plane;
}
