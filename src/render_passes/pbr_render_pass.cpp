#include "pbr_render_pass.h"

#include <algorithm>

#include "gfx_api/gfx.h"
#include "scene/scene.h"
#include "inu_typedefs.h"
#include "lights/dirlight.h"
#include "lights/spotlight.h"
#include "lights/light_probe.h"
#include "animation/skin.h"

framebuffer_t offline_fb;

void init_pbr_render_pass() {
  float fb_width = 1920;
  float fb_height = 1080;

  offline_fb = create_framebuffer(fb_width, fb_height, FB_TYPE::RENDER_BUFFER_DEPTH_STENCIL);
}

void render_scene_obj(int obj_id) {
  shader_t& shader = material_t::associated_shader;

  int model_id = get_obj_model_id(obj_id);

  if (obj_has_skin(obj_id)) {
    set_skin_in_shader_for_obj(shader, obj_id);
  } else {
    shader_set_int(shader, "skinned", 0);
    mat4 model_mat = get_obj_model_mat(obj_id);
    shader_set_mat4(shader, "model", model_mat);
  } 

  if (is_obj_selected(obj_id)) {
    set_render_mode(RENDER_MODE::WIREFRAME);
  }

#if SHOW_SPOTLIGHTS == 1
  if (model_id == spotlight_t::LIGHT_MESH_ID) {
    shader_set_int(shader, "override_color_bool", 1);
    shader_set_vec3(shader, "override_color", {1,1,1});
    bind_shader(shader);
    render_model_w_no_material_bind(model_id);
    unbind_shader();
  } else {
    render_model(model_id, false, shader);
  }
#else
  render_model(model_id, false, shader);
#endif


  if (is_obj_selected(obj_id)) {
    set_render_mode(RENDER_MODE::NORMAL);
  }

}

void pbr_render_pass() {
  // PBR RENDER PASS
  bind_framebuffer(offline_fb);
  clear_framebuffer();

  mat4 proj = get_cam_proj_mat();
  mat4 view = get_cam_view_mat();
  shader_set_mat4(material_t::associated_shader, "projection", proj);
  shader_set_mat4(material_t::associated_shader, "view", view);

  shader_set_float(material_t::associated_shader, "cam_data.near_plane", get_cam_near_plane());
  shader_set_float(material_t::associated_shader, "cam_data.far_plane", get_cam_far_plane());
  shader_set_vec3(material_t::associated_shader, "cam_data.cam_pos", get_cam_pos());

  set_up_shader_for_pbr_render_pass();
  
  // scene_iterator_t scene_iterator = create_scene_iterator(OBJECT_FLAGS::GAMEOBJECT);
  // scene_iterator_t scene_iterator = create_scene_iterator(OBJECT_FLAGS::EDITOR_OBJ);
  scene_iterator_t scene_iterator = create_scene_iterator(OBJECT_FLAGS::NONE);

  std::vector<object_id> obj_ids_w_opaque_models;
  std::vector<obj_sort_info_t> non_opaque_objs;
  int obj_id = iterate_scene_for_next_obj(scene_iterator);
  do {
    int model_id = get_obj_model_id(obj_id);
    if (model_id != -1) {
      if(is_model_opaque(model_id)) {
        obj_ids_w_opaque_models.push_back(obj_id);
      } else {
        mat4 model_mat = get_obj_model_mat(obj_id);
        transform_t final_transform = get_transform_from_matrix(model_mat);

        obj_sort_info_t info{};
        info.obj_id = obj_id;
        info.pos = final_transform.pos;
        non_opaque_objs.push_back(info);
      }
    }
    obj_id = iterate_scene_for_next_obj(scene_iterator);
  }
  while (obj_id != -1);
  obj_sort_info_comparator_t obj_sort_info_comparator;
  std::sort(non_opaque_objs.begin(), non_opaque_objs.end(), obj_sort_info_comparator);

  // PASS 1 will be of only opaque objects
  for (int obj_id : obj_ids_w_opaque_models) {
    render_scene_obj(obj_id);
  } 

  // PASS 2 will be of only non opaque objects...altho these will need to be sorted and rendered back to front
  // sort non-opaque objects 
  for (obj_sort_info_t& info : non_opaque_objs) {
    render_scene_obj(info.obj_id);
  }
 
  unbind_shader();

#if DISPLAY_DIR_LIGHT_SHADOW_MAPS
  if (num_dir_lights > 0) {
    // show depth maps 
    render_dir_light_shadow_maps(0);
  }
#endif
}


void set_up_shader_for_pbr_render_pass() {
  // spotlights for offline shader
  int num_lights = get_num_spotlights();
  for (int i = 0; i < NUM_LIGHTS_SUPPORTED_IN_SHADER; i++) {
    mat4 identity(1.0f);
    bool inactive = (i >= num_lights);

    char var_name[64]{};
    sprintf(var_name, "spotlights_mat_data[%i].light_view", i);
    if (inactive) {
      shader_set_mat4(material_t::associated_shader, var_name, identity);
    } else {
      mat4 light_view = get_spotlight_view_mat(i);
      shader_set_mat4(material_t::associated_shader, var_name, light_view);
    }
    
    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_mat_data[%i].light_projection", i);
    if (inactive) {
      shader_set_mat4(material_t::associated_shader, var_name, identity);
    } else {
      mat4 light_proj = get_spotlight_proj_mat(i);
      shader_set_mat4(material_t::associated_shader, var_name, light_proj);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].depth_tex", i);
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, LIGHT0_SHADOW_MAP_TEX + i);
      tex_id_t depth_att = get_spotlight_fb_depth_tex(i);
      bind_texture(depth_att, LIGHT0_SHADOW_MAP_TEX + i);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].pos", i);
    if (inactive) {
      shader_set_vec3(material_t::associated_shader, var_name, {0,0,0});
    } else {
      vec3 p = get_spotlight_pos(i);
      shader_set_vec3(material_t::associated_shader, var_name, p);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].light_active", i);
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, 1);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].shadow_map_width", i);
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, spotlight_t::SHADOW_MAP_WIDTH);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].shadow_map_height", i);
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, spotlight_t::SHADOW_MAP_HEIGHT);
    }
    
    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].near_plane", i);
    if (inactive) {
      shader_set_float(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_float(material_t::associated_shader, var_name, spotlight_t::NEAR_PLANE);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "spotlights_data[%i].far_plane", i);
    if (inactive) {
      shader_set_float(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_float(material_t::associated_shader, var_name, spotlight_t::FAR_PLANE);
    }
  }

  // set up dir lights in offline shader
  int num_dir_lights = get_num_dir_lights(); 
  const int NUM_MAX_DIR_LIGHTS = 1;
  for (int i = 0; i < NUM_MAX_DIR_LIGHTS * HAVE_DIR_LIGHT; i++) {

    mat4 identity(1.0f);
    bool inactive = (i >= num_dir_lights);

    if (inactive) {
      const char* var_name = "dir_light_data.light_active";
      shader_set_int(material_t::associated_shader, var_name, 0);
      continue;
    }

    dir_light_t* dir_light = get_dir_light(i);

    for (int j = 0; j < NUM_SM_CASCADES; j++) {

      char var_name[64]{};
      sprintf(var_name, "dir_light_data.light_views[%i]", j);
      if (inactive) {
        shader_set_mat4(material_t::associated_shader, var_name, identity);
      } else {
        shader_set_mat4(material_t::associated_shader, var_name, dir_light->light_views[j]);
      }

      memset(var_name, 0, sizeof(var_name));
      sprintf(var_name, "dir_light_data.light_projs[%i]", j);
      if (inactive) {
        shader_set_mat4(material_t::associated_shader, var_name, identity);
      } else {
        shader_set_mat4(material_t::associated_shader, var_name, dir_light->light_orthos[j]);
      }

      memset(var_name, 0, sizeof(var_name));
      sprintf(var_name, "dir_light_data.cascade_depths[%i]", j);
      if (inactive) {
        shader_set_float(material_t::associated_shader, var_name, 0);
      } else {
        shader_set_float(material_t::associated_shader, var_name, dir_light->cacade_depths[j]);
      }
    }
    
    char var_name[64]{};
    sprintf(var_name, "dir_light_data.cascade_depths[%i]", NUM_SM_CASCADES);
    if (inactive) {
      shader_set_float(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_float(material_t::associated_shader, var_name, dir_light->cacade_depths[NUM_SM_CASCADES]);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "dir_light_data.light_dir");
    if (inactive) {
      shader_set_vec3(material_t::associated_shader, var_name, {0,0,0});
    } else {
      shader_set_vec3(material_t::associated_shader, var_name, dir_light->dir);
    }
    
    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "dir_light_data.shadow_map");
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, DIR_LIGHT_SHADOW_MAP_TEX);
      tex_id_t depth_att = dir_light->light_pass_fb.depth_att;
      bind_texture(depth_att, DIR_LIGHT_SHADOW_MAP_TEX);
    }

    memset(var_name, 0, sizeof(var_name));
    sprintf(var_name, "dir_light_data.light_active");
    if (inactive) {
      shader_set_int(material_t::associated_shader, var_name, 0);
    } else {
      shader_set_int(material_t::associated_shader, var_name, 1);
    }

  }

#if 0
  shader_set_int(material_t::associated_shader, "light_prob.shape", LIGHT_PROBE_SHAPE::RECT_PLANAR);
  vec3 color = {0,1,0};
  shader_set_vec3(material_t::associated_shader, "light_prob.color", color);
  shader_set_vec3(material_t::associated_shader, "light_prob.world_pos", color);
#endif
#if 0
  set_light_probe_in_shader(1, 0, material_t::associated_shader);
  set_light_probe_in_shader(2, 1, material_t::associated_shader);
#else
  // will later move this call to be object specific
  transform_t t{};
  setup_light_probes_based_on_transform(t);
#endif

}
