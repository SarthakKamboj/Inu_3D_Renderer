#include "model_internal.h"

#include <vector>

std::vector<model_t> models;

int register_model(model_t& model) {
  model.id = models.size();
  models.push_back(model);
  return model.id;
}

int latest_model_id() {
  return models[models.size()-1].id;
}

model_t* get_model(int model_id) {
  return &models[model_id];
}
