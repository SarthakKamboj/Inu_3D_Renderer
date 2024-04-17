#include "utils/transform.h"
#include "utils/mats.h"

struct camera_t {
  // scale doesn't matter
  transform_t transform;
  vec3 focal_pt;
  mat4 view;
  mat4 proj;
  float near_plane;
  float far_plane;
};

void create_camera(transform_t& t);
float get_cam_near_plane();
float get_cam_far_plane();
mat4 get_cam_view_mat();
mat4 get_cam_proj_mat();
mat4 get_view_mat(vec3 pos, vec3 focal_pt);
void cam_zoom(float amount);
void cam_rotate(float lat_amount, float vert_amount);
void cam_translate(float lat, float vert);

void update_cam();
camera_t* get_cam();
