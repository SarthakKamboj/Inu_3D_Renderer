#pragma once

#include "utils/transform.h"
#include "gfx_api/gfx.h"

#define SHOW_LIGHT_PROBES 1

typedef int light_probe_id;

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

// void create_light_probe(transform_t& t);
void create_light_probe(transform_t& t, vec3 color);
void set_light_probe_in_shader(int id, int shader_probe_idx, shader_t& shader);
void render_light_probes();
