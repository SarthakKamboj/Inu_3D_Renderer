#include "light_probe.h"

#include "gfx/gfx.h"
#include "model_loading/model_internal.h"

#include "glew.h"

#include <vector>

static std::vector<light_probe_t> light_probes;
int light_probe_t::LIGHT_PROBE_MODEL_ID = -1;

light_probe_id probe_id = 1;

void create_light_probe(transform_t& t, vec3 color) {
  light_probe_t light_probe;  
  light_probe.transform = t;
  light_probe.id = probe_id;
  light_probe.color = color;
  probe_id++;
  light_probes.push_back(light_probe);
}

void render_light_probes() {
  glDisable(GL_CULL_FACE);

  model_t* model_p = get_model(light_probe_t::LIGHT_PROBE_MODEL_ID);
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
      // shader_set_vec3(material_t::associated_shader, "override_color", lp.color);

      bind_vao(mesh.vao);
      draw_ebo(mesh.ebo);
      unbind_vao();
      unbind_ebo();

    }
  }

  glEnable(GL_CULL_FACE);
}

void set_light_probe_in_shader(int id, int shader_probe_idx, shader_t& shader) {
  light_probe_t& lp = light_probes[id-1];

  char shader_arg[64]{};
  sprintf(shader_arg, "light_probes[%i].shape", shader_probe_idx);
  shader_set_int(shader, shader_arg, (int)lp.shape);
  
  vec3 color = {0,1,0};
  memset(shader_arg, 0, sizeof(shader_arg));
  sprintf(shader_arg, "light_probes[%i].color", shader_probe_idx);
  // shader_set_vec3(shader, shader_arg, color);
  shader_set_vec3(shader, shader_arg, lp.color);

  memset(shader_arg, 0, sizeof(shader_arg));
  sprintf(shader_arg, "light_probes[%i].world_pos", shader_probe_idx);
  shader_set_vec3(shader, shader_arg, lp.transform.pos);

  mat4 m = get_model_matrix(lp.transform);
  memset(shader_arg, 0, sizeof(shader_arg));
  sprintf(shader_arg, "light_probes[%i].model", shader_probe_idx);
  shader_set_mat4(shader, shader_arg, m); 
}
