#include <stdio.h>

#include "windowing/window.h"
#include "model_loading/model_internal.h"
#include "model_loading/gltf/gltf.h"
#include "gfx/gfx.h"
#include "gfx/online_renderer.h"
#include "gfx/light.h"
#include "scene/scene.h"
#include "scene/camera.h"
#include "utils/transform.h"
#include "utils/general.h"
#include "utils/app_info.h"
#include "utils/mats.h"
#include "utils/quaternion.h"

static float win_width = 1280.f;
static float win_height = 960.f;

bool update_dir_light_frustums = true;
bool render_dir_orthos = true;

extern window_t window;
extern animation_globals_t animation_globals;
extern framebuffer_t offline_fb;
// extern framebuffer_t light_pass_fb;

extern bool make_orthos_square;
extern bool make_orthos_square_consistent;
extern bool make_orthos_texel_snapped;

app_info_t app_info;

#if 0
struct test_t {
  int a = 5;
  int b = 10;

  test_t() {}
  test_t(int _a) {
    a = _a;
  }
#if 0
  test_t(int _a, int _b) {
    a = _a;
    b = _b;
  }
#endif
};
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

#if 0
  vec4 oatmeal = {150,27,5,3};
  vec4 milk = {80,11,8,0};
  vec4 yogurt = {90,15,7,0};
  vec4 protein_powder = {130,3,24,2};
  vec4 biscoff = {170, 17,0,11};

  vec4 total = (oatmeal * 2.f) + (milk * 1.f) + (yogurt * 0.75f) + (protein_powder * 1.5f) + (biscoff * 1.f);

#endif

  create_window(hInstance, win_width, win_height);

  if (wcscmp(pCmdLine, L"running_in_vs") == 0) {
    app_info.running_in_vs = true;
  }

  transform_t t;
  t.pos.y = 10.f;
  t.pos.x = 2.f;
  create_camera(t);
  init_online_renderer();

  char resources_path[256]{};
  get_resources_folder_path(resources_path);
  printf("resources_path: %s\n", resources_path);

  init_scene_rendering();

  char vert_shader_path[256]{};
  sprintf(vert_shader_path, "%s\\shaders\\model.vert", resources_path);
  char frag_shader_path[256]{};
  sprintf(frag_shader_path, "%s\\shaders\\model.frag", resources_path);
  material_t::associated_shader = create_shader(vert_shader_path, frag_shader_path); 

  // const char* gltf_file_resources_folder_rel_path =  "box\\Box.gltf";
  // const char* gltf_file_resources_folder_rel_path =  "box_interleaved\\BoxInterleaved.gltf";
  // const char* gltf_file_resources_folder_rel_path = "box_textured\\BoxTextured.gltf";
  // const char* gltf_file_resources_folder_rel_path = "box_textured_non_power_of_2\\BoxTexturedNonPowerOfTwo.gltf";
  // const char* gltf_file_resources_folder_rel_path = "animated_cube\\AnimatedCube.gltf";
  // const char* gltf_file_resources_folder_rel_path = "box_animated\\BoxAnimated.gltf";
  // const char* gltf_file_resources_folder_rel_path = "two_cylinder_engine\\2CylinderEngine.gltf";
  // const char* gltf_file_resources_folder_rel_path = "box_with_spaces\\Box With Spaces.gltf";
  // const char* gltf_file_resources_folder_rel_path = "box_vertex_colors\\BoxVertexColors.gltf";
  // const char* gltf_file_resources_folder_rel_path = "cube_non_smooth_face\\Cube.gltf";
  // const char* gltf_file_resources_folder_rel_path = "duck\\Duck.gltf";
  // const char* gltf_file_resources_folder_rel_path = "avacado\\Avocado.gltf";
  // const char* gltf_file_resources_folder_rel_path = "suzan\\Suzanne.gltf";
  // const char* gltf_file_resources_folder_rel_path = "cartoon_car\\combined.gltf";
  // const char* gltf_file_resources_folder_rel_path = "stylized_ww1_plane\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "ferrari_enzo\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "buggy\\Buggy.gltf";
  // const char* gltf_file_resources_folder_rel_path = "stylized_mushrooms\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "little_chestnut\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "milk_truck\\CesiumMilkTruck.gltf";
  // const char* gltf_file_resources_folder_rel_path = "rigged_simple\\RiggedSimple.gltf";
  // const char* gltf_file_resources_folder_rel_path = "rigged_figure\\RiggedFigure.gltf";
  // const char* gltf_file_resources_folder_rel_path = "rigged_figure\\blender_export.gltf";
  // const char* gltf_file_resources_folder_rel_path = "cesium_man\\CesiumMan.gltf";
  // const char* gltf_file_resources_folder_rel_path = "brain_stem\\BrainStem.gltf";
  const char* gltf_file_resources_folder_rel_path = "medieval_fantasy_book\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "shadow_test\\test.gltf";
  // const char* gltf_file_resources_folder_rel_path = "fox\\Fox.gltf";
  // const char* gltf_file_resources_folder_rel_path = "virtual_city\\VC.gltf";
  // const char* gltf_file_resources_folder_rel_path = "low-poly_truck_car_drifter\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "yusuke_urameshi\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "junkrat\\scene.gltf";
  // const char* gltf_file_resources_folder_rel_path = "reap_the_whirlwind\\scene.gltf";

  if (strcmp(gltf_file_resources_folder_rel_path, "stylized_ww1_plane\\scene.gltf") == 0
      || strcmp(gltf_file_resources_folder_rel_path, "ferrari_enzo\\scene.gltf") == 0 //) {
      || strcmp(gltf_file_resources_folder_rel_path, "rigged_figure\\RiggedFigure.gltf") == 0) {
    app_info.render_only_textured = true;
  }

#if SHOW_BONES
  char bone_mesh_full_file_path[256]{};
  // this file pretty much just has a mesh, no nodes
  sprintf(bone_mesh_full_file_path, "%s\\bone_mesh\\custom_bone_mesh.gltf", resources_path);
  gltf_load_file(bone_mesh_full_file_path);
  skin_t::BONE_MODEL_ID = latest_model_id();
#endif

  char gltf_full_file_path[256]{};
  sprintf(gltf_full_file_path, "%s\\%s", resources_path, gltf_file_resources_folder_rel_path);
  gltf_load_file(gltf_full_file_path);

  play_next_anim();

  init_light_data();
#if 0
  // create_light({2,10,0});
  // create_light({-2,3,0});
  create_light({2,8,0});
  create_light({-20,30,0});
  create_light({10,5,-5});
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

  int RENDER_DEPTH = 0;
  while (window.running) {
    inu_timer_t frame_timer;
    start_timer(frame_timer);

    // WINDOW + INPUT PASS
    poll_events();

    if (window.input.a_up) {
      make_orthos_square = !make_orthos_square;
    } else if (window.input.b_up) {
      make_orthos_square_consistent = !make_orthos_square_consistent;
    } else if (window.input.c_up) {
      make_orthos_texel_snapped = !make_orthos_texel_snapped;
    }

    // UPDATE PASS
    if (window.input.left_mouse_up) {
      render_dir_orthos = !render_dir_orthos;
      if (RENDER_DIR_LIGHT_ORTHOS) {
        dir_light_t* dir = get_dir_light(0);
        for (int i = 0; i < NUM_SM_CASCADES; i++) {
          if (render_dir_orthos && i == LIGHT_ORTHO_CASCADE_TO_VIEW) {
            set_obj_as_parent(dir->debug_ortho_obj_ids[i]);
          } else {
            unset_obj_as_parent(dir->debug_ortho_obj_ids[i]);
          }
        }
      }
    }
    //
    if (window.input.right_mouse_up) {
      update_dir_light_frustums = !update_dir_light_frustums;
      play_next_anim();
      // RENDER_DEPTH = 1-RENDER_DEPTH;
    }
    
    update_cam();
    update_animations();

    // vec4 pt = {10,1,-25,0};
    vec4 pt = {-10.25744f,4.49857f,17.58958f,1.0f};
    vec4 pt_m1 = mat_multiply_vec(get_cam_view_mat(), pt);
    vec4 pt_m2 = mat_multiply_vec(get_cam_proj_mat(), pt_m1);
    pt_m2 = pt_m2 / pt_m2.w;
    int a = 5;

    // RENDERING PASS

    // offline rendering pass  
    render_scene();

    // online rendering pass
    if (RENDER_DEPTH == 0) {
      render_online(offline_fb.color_att, 0);
    } else {
      // render_online(offline_fb.depth_att);
      GLuint depth_att = get_light_fb_depth_tex(0);
      render_online(depth_att, 1);
    }

    swap_buffers();

    end_timer(frame_timer);
    app_info.delta_time = frame_timer.elapsed_time_sec;
  }
}
