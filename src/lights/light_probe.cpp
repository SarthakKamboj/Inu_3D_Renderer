#include "light_probe.h"

#include "gfx_api/gfx.h"
#include "geometry/model.h"

#include "glew.h"

#include <vector>

static std::vector<light_probe_t> light_probes;
int light_probe_t::LIGHT_PROBE_MATERIAL = -1;

static light_probe_id probe_id = 1;

void create_light_probe(transform_t& t, vec3 color) {

  if (light_probe_t::LIGHT_PROBE_MATERIAL == -1) {
    albedo_param_t albedo;
    albedo.base_color = {0,1,0,1};

    metallic_roughness_param_t met_rough_param;
    met_rough_param.roughness_factor = 1.f;
    met_rough_param.metallic_factor = 0.f;

    std::string probe_mat_name = "light material";
    int light_probe_mat_idx = create_material(probe_mat_name, albedo, met_rough_param);
    light_probe_t::LIGHT_PROBE_MATERIAL = light_probe_mat_idx;
    set_material_cull_back(light_probe_mat_idx, false);
  }

  light_probe_t light_probe;  
  light_probe.transform = t;
  light_probe.id = probe_id;
  light_probe.color = color;
  probe_id++;
  light_probes.push_back(light_probe);
}

light_probe_t* get_light_probe(light_probe_id id) {
  return &light_probes[id-1];
}

void render_light_probes() {
  disable_culling();

  set_material_on_model(basic_models_t::PLANE, light_probe_t::LIGHT_PROBE_MATERIAL);
  model_t* model_p = get_model(basic_models_t::PLANE);
  model_t& model = *model_p;

  shader_t& shader = material_t::associated_shader;

  shader_set_int(shader, "skinned", 0);
  for (light_probe_t& lp : light_probes) {
    mat4 m = get_model_matrix(lp.transform);
    shader_set_mat4(shader, "model", m);

    for (mesh_t& mesh : model.meshes) {

      material_t m = bind_material(mesh.mat_idx);
      vec4 lpc = {lp.color.x, lp.color.y, lp.color.z, 1};
      shader_set_vec4(material_t::associated_shader, "material.mesh_color", lpc);
      bind_shader(shader);

      bind_vao(mesh.vao);
      draw_ebo(mesh.ebo);
      unbind_vao();
      unbind_ebo();

    }
  }

  enable_culling();
}

void set_light_probe_in_shader(light_probe_id id, int shader_probe_idx, shader_t& shader) {
  light_probe_t* lp_p = get_light_probe(id);
  inu_assert(lp_p);
  light_probe_t& lp = *lp_p;

  char shader_arg[64]{};
  sprintf(shader_arg, "light_probes[%i].shape", shader_probe_idx);
  shader_set_int(shader, shader_arg, (int)lp.shape);
  
  vec3 color = {0,1,0};
  memset(shader_arg, 0, sizeof(shader_arg));
  sprintf(shader_arg, "light_probes[%i].color", shader_probe_idx);
  shader_set_vec3(shader, shader_arg, lp.color);

  memset(shader_arg, 0, sizeof(shader_arg));
  sprintf(shader_arg, "light_probes[%i].world_pos", shader_probe_idx);
  shader_set_vec3(shader, shader_arg, lp.transform.pos);

  mat4 m = get_model_matrix(lp.transform);
  memset(shader_arg, 0, sizeof(shader_arg));
  sprintf(shader_arg, "light_probes[%i].model", shader_probe_idx);
  shader_set_mat4(shader, shader_arg, m); 
}

void setup_light_probes_based_on_transform(transform_t& transform) {
  int num_light_probes = light_probes.size();
  shader_set_int(material_t::associated_shader, "num_light_probes_set", num_light_probes);

  for (int i = 0; i < num_light_probes; i++) {
    set_light_probe_in_shader(i+1, i, material_t::associated_shader);
  }
}
