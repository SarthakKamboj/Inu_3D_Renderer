#pragma once

#include "utils/transform.h"
#include "gfx_api/gfx.h"
#include "inu_typedefs.h"

#define SHOW_LIGHT_PROBES 1

// RECT_PLANAR by default is (1 unit x 1 unit)
enum class LIGHT_PROBE_SHAPE {
  RECT_PLANAR = 1
};

struct light_probe_t {
  // static int LIGHT_PROBE_MODEL_ID;
  static int LIGHT_PROBE_MATERIAL;

  light_probe_id id = 0;
  transform_t transform;
  vec3 color;
  LIGHT_PROBE_SHAPE shape = LIGHT_PROBE_SHAPE::RECT_PLANAR;
};

void create_light_probe(transform_t& t, vec3 color);
void setup_light_probes_based_on_transform(transform_t& transform);
void set_light_probe_in_shader(light_probe_id id, int shader_probe_idx, shader_t& shader);
light_probe_t* get_light_probe(light_probe_id id);
void render_light_probes();
