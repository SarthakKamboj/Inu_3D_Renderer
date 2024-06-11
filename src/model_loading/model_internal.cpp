#include "model_internal.h"

#include <vector>

std::vector<model_t> models;

int register_model(model_t& model) {
  model.id = models.size();

  for (mesh_t& mesh : model.meshes) {
    material_t m = get_material(mesh.mat_idx);
    bool non_opaque = (m.transparency_mode != TRANSPARENCY_MODE::OPQUE);
    if (non_opaque) {
      model.is_non_opaque_mesh = true;
      break;
    }
  }

  models.push_back(model);
  return model.id;
}

int latest_model_id() {
  return models[models.size()-1].id;
}

model_t* get_model(int model_id) {
  return &models[model_id];
}
