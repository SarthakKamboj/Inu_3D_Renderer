
#include "utils/inu_time.h"

struct app_info_t {
  bool running_in_vs = false;
  
  // TODO: can likely be removed in the future
  bool render_only_textured = false;

  time_count_t delta_time = 0;
};
