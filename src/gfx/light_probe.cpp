#include "light_probe.h"

#include "gfx/gfx.h"
#include "model_loading/model_internal.h"

#include <vector>

static std::vector<light_probe_t> light_probes;
int light_probe_t::LIGHT_PROBE_MODEL_ID = -1;

light_probe_id probe_id = 1;

void create_light_probe(transform_t& t) {
  light_probe_t light_probe;  
  light_probe.transform = t;
  light_probe.id = probe_id;
  probe_id++;
  light_probes.push_back(light_probe);
}

void render_light_probes() {

  model_t* model_p = get_model(light_probe_t::LIGHT_PROBE_MODEL_ID);
  model_t& model = *model_p;

  shader_t& shader = material_t::associated_shader;

  shader_set_int(shader, "skinned", 0);
  for (light_probe_t& lp : light_probes) {
    mat4 m = get_model_matrix(lp.transform);
    shader_set_mat4(shader, "model", m);

    for (mesh_t& mesh : model.meshes) {

      material_t m = bind_material(mesh.mat_idx);
      // shader_set_int(material_t::associated_shader, "override_color_bool", 1);

      bind_vao(mesh.vao);
      draw_ebo(mesh.ebo);
      unbind_vao();
      unbind_ebo();

    }
  }
}

void set_light_probe_in_shader(int id, int shader_probe_idx, shader_t& shader) {
  light_probe_t& lp = light_probes[id-1];

  // char shader_arg[64]{};
  // sprintf(shader_arg, "light_probe[%i].shape", shader_probe_idx);

  shader_set_int(shader, "light_probe.shape", (int)lp.shape);
  vec3 color = {0,1,0};
  shader_set_vec3(shader, "light_probe.color", color);
  shader_set_vec3(shader, "light_probe.world_pos", lp.transform.pos);
  mat4 m = get_model_matrix(lp.transform);
  shader_set_mat4(shader, "light_probe.model", m);
 
}
