#include <stdio.h>
#include <iostream>

#include "windowing/window.h"
#include "geometry/model.h"
#include "geometry/gltf/gltf.h"
#include "gfx_api/gfx.h"
#include "scene/online_renderer.h"
#include "lights/dirlight.h"
#include "lights/spotlight.h"
#include "lights/light_probe.h"
#include "scene/scene.h"
#include "scene/camera.h"
#include "utils/transform.h"
#include "utils/general.h"
#include "utils/app_info.h"
#include "utils/mats.h"
#include "utils/quaternion.h"
#include "utils/inu_math.h"
#include "render_passes/pbr_render_pass.h"

static float win_width = 1280.f;
static float win_height = 960.f;

extern window_t window;
extern animation_globals_t animation_globals;
extern framebuffer_t offline_fb;

app_info_t app_info;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

  create_window(hInstance, win_width, win_height);

  if (wcscmp(pCmdLine, L"running_in_vs") == 0) {
    app_info.running_in_vs = true;
  }

  transform_t t;
  t.pos.y = 2.3f;
  t.pos.x = 2.3f;
  create_camera(t);
  init_online_renderer();

  char resources_path[256]{};
  get_resources_folder_path(resources_path);
  printf("resources_path: %s\n", resources_path);

  init_scene_rendering();
  init_pbr_render_pass();

  char vert_shader_path[256]{};
  sprintf(vert_shader_path, "%s\\shaders\\model.vert", resources_path);
  char geo_shader_path[256]{};
  sprintf(geo_shader_path, "%s\\shaders\\model.geo", resources_path);
  char frag_shader_path[256]{};
  sprintf(frag_shader_path, "%s\\shaders\\model.frag", resources_path);
  material_t::associated_shader = create_shader(vert_shader_path, geo_shader_path, frag_shader_path); 

  init_selection();

  // const char* gltf_file_resources_folder_rel_path =  "box\\Box.gltf";
  // const char* gltf_file_resources_folder_rel_path =  "box_interleaved\\BoxInterleaved.gltf";
  // const char* gltf_file_resources_folder_rel_path = "box_textured\\BoxTextured.gltf";
  // const char* gltf_file_resources_folder_rel_path = "box_textured_non_power_of_2\\BoxTexturedNonPowerOfTwo.gltf";
  // const char* gltf_file_resources_folder_rel_path = "animated_cube\\AnimatedCube.gltf";
  // const char* gltf_file_resources_folder_rel_path = "box_animated\\BoxAnimated.gltf";
  // const char* gltf_file_resources_folder_rel_path2 = "two_cylinder_engine\\2CylinderEngine.gltf";
  // const char* gltf_file_resources_folder_rel_path = "box_with_spaces\\Box With Spaces.gltf";
  // const char* gltf_file_resources_folder_rel_path = "box_vertex_colors\\BoxVertexColors.gltf";
  // const char* gltf_file_resources_folder_rel_path = "cube_non_smooth_face\\Cube.gltf";
  // const char* gltf_file_resources_folder_rel_path = "duck\\Duck.gltf";
  // const char* gltf_file_resources_folder_rel_path = "avacado\\Avocado.gltf";
  // const char* gltf_file_resources_folder_rel_path = "sci_fi_helmet\\SciFiHelmet.gltf";
  // const char* gltf_file_resources_folder_rel_path = "damaged_helmet\\DamagedHelmet.gltf";
  // const char* gltf_file_resources_folder_rel_path = "lantern\\Lantern.gltf";
  // const char* gltf_file_resources_folder_rel_path = "water_bottle\\WaterBottle.gltf";
  // const char* gltf_file_resources_folder_rel_path = "suzan\\Suzanne.gltf";
  // const char* gltf_file_resources_folder_rel_path = "cartoon_car\\combined.gltf";
  // const char* gltf_file_resources_folder_rel_path = "ferrari_enzo\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path2 = "buggy\\Buggy.gltf";
  // const char* gltf_file_resources_folder_rel_path2 = "stylized_mushrooms\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "little_chestnut\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "milk_truck\\CesiumMilkTruck.gltf";
  // const char* gltf_file_resources_folder_rel_path = "rigged_simple\\RiggedSimple.gltf";
  // const char* gltf_file_resources_folder_rel_path = "rigged_figure\\RiggedFigure.gltf";
  // const char* gltf_file_resources_folder_rel_path = "rigged_figure\\blender_export.gltf";
  // const char* gltf_file_resources_folder_rel_path = "cesium_man\\CesiumMan.gltf";
  // const char* gltf_file_resources_folder_rel_path = "shadow_test\\test.gltf";
  // const char* gltf_file_resources_folder_rel_path = "fox\\Fox.gltf";
  // const char* gltf_file_resources_folder_rel_path = "low-poly_truck_car_drifter\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "yusuke_urameshi\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "junkrat\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "reap_the_whirlwind\\scene.gltf";

  // const char* gltf_file_resources_folder_rel_path = "medieval_fantasy_book\\scene.gltf";
  const char* gltf_file_resources_folder_rel_path = "virtual_city\\VC.gltf";
  // const char* gltf_file_resources_folder_rel_path = "brain_stem\\BrainStem.gltf";
  // const char* gltf_file_resources_folder_rel_path = "global_illum_test\\global_illum_test.gltf";
  //

#if 0
  const char* gltf_file_resources_folder_rel_path = "global_illum_test\\global_illum_test_2.gltf";
  char gltf_full_file_path[256]{};
  sprintf(gltf_full_file_path, "%s\\models\\%s", resources_path, gltf_file_resources_folder_rel_path);
  gltf_load_file(gltf_full_file_path);
#endif
  // const char* gltf_file_resources_folder_rel_path = "pixel_perfect_sel\\pixel_perfect_sel.gltf";
  char gltf_full_file_path[256]{};
  sprintf(gltf_full_file_path, "%s\\models\\%s", resources_path, gltf_file_resources_folder_rel_path);
  gltf_load_file(gltf_full_file_path);

#if 0
  // const char* gltf_file_resources_folder_rel_path2 = "stylized_ww1_plane\\scene.gltf";
  char gltf_full_file_path2[256]{};
  sprintf(gltf_full_file_path2, "%s\\models\\%s", resources_path, gltf_file_resources_folder_rel_path2);
  gltf_load_file(gltf_full_file_path2);
#endif

#if 0
  if (strcmp(gltf_file_resources_folder_rel_path, "stylized_ww1_plane\\scene.gltf") == 0
      || strcmp(gltf_file_resources_folder_rel_path, "ferrari_enzo\\scene.gltf") == 0 //) {
      || strcmp(gltf_file_resources_folder_rel_path, "rigged_figure\\RiggedFigure.gltf") == 0) {
    // app_info.render_only_textured = true;
  }
#endif

#if SHOW_BONES
  char bone_mesh_full_file_path[256]{};
  // this file pretty much just has a mesh, no nodes
  sprintf(bone_mesh_full_file_path, "%s\\bone_mesh\\custom_bone_mesh.gltf", resources_path);
  gltf_load_file(bone_mesh_full_file_path, false);
  skin_t::BONE_MODEL_ID = latest_model_id();
#endif

  create_basic_models();

#if 0
  char gltf_full_file_path2[256]{};
  sprintf(gltf_full_file_path2, "%s\\models\\%s", resources_path, gltf_file_resources_folder_rel_path2);
  gltf_load_file(gltf_full_file_path2);
#endif

#if SHOW_LIGHT_PROBES == 1
  albedo_param_t albedo;
  albedo.base_color = {0,1,0,1};

  metallic_roughness_param_t met_rough_param;
  met_rough_param.roughness_factor = 1.f;
  met_rough_param.metallic_factor = 0.f;

  std::string probe_mat_name = "light material";
  int light_probe_mat_idx = create_material(probe_mat_name, albedo, met_rough_param);
#endif

  play_next_anim();

  init_spotlight_data();
  init_light_data();
#if 0
  // create_light({2,10,0});
  // create_light({-2,3,0});
  // create_light({-20,30,0});
#if 1
  create_spotlight({2,8,0});
  create_spotlight({-20,10,0});
  create_spotlight({10,5,-5});
#endif
  // create_light({-8,5,-5});
  // create_light({-5,3,0});
  
  // create_light({0,10,0});
  // create_light({2,10,0});6
  // create_light({4,10,0});

#else
  // create_light({0,30,0});
#endif

#if HAVE_DIR_LIGHT
  create_dir_light({-1,-1,0});
#endif

  transform_t lp_t;
  // lp_t.pos = {0.5f,0.5f,0.2f};
  // lp_t.pos = {-3.0f,0.2,0};
  lp_t.pos = {0,0.2f,0};
  lp_t.rot = create_quaternion_w_rot({1,0,0}, 90.f);
  lp_t.scale = {11,11,11};
  // lp_t.scale = {1,1,1};
  create_light_probe(lp_t, {0, 1, 0});

  transform_t lp_t2;
  lp_t2.pos = {1.5,4.0,0};
  lp_t2.rot = create_quaternion_w_rot({0,1,0}, 90.f);
  lp_t2.scale = {6.5f,6.f,8};
  create_light_probe(lp_t2, {1, 0, 0});

#if 0
  scene_iterator_t iterator = create_scene_iterator();
  int j = -1;
  do {
    j = iterate_scene_for_next_obj(iterator);
  }
  while (j != -1);
#endif

  int RENDER_DEPTH = 0;
  while (window.running) {
    inu_timer_t frame_timer;
    start_timer(frame_timer);

    // WINDOW + INPUT PASS
    poll_events();

    // UPDATE PASS

    if (window.input.right_mouse_up) {
      // RENDER_DEPTH = 1-RENDER_DEPTH;
    }

    handle_selection_logic() ;
    
    update_cam();
    update_animations();
    
    // update object transforms
    update_obj_model_mats();

    // RENDERING PASS
    
    // offline rendering pass  
    render_scene();

    // online rendering pass
    if (RENDER_DEPTH == 0) {
      render_online(offline_fb.color_att, 0);
    } else {
      tex_id_t depth_att = get_spotlight_fb_depth_tex(0);
      render_online(depth_att, 1);
    }

    swap_buffers();

    end_timer(frame_timer);
    app_info.delta_time = frame_timer.elapsed_time_sec;
  }
// #endif
}
