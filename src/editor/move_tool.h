#pragma once

#include "utils/transform.h"
#include "inu_typedefs.h"

struct move_tool_t {
  transform_t transform; 
  model_id arrow_id;
};

void init_move_tool();
