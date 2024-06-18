#pragma once

#include <vector>
#include <string>

struct animation_t {
  int id = -1;
  std::string name;
  std::vector<int> data_chunk_ids;
};
int register_animation(animation_t& anim);
void play_next_anim();

enum class ANIM_INTERPOLATION_MODE {
  LINEAR,
  STEP
};

struct animation_data_chunk_t {
  int id = -1;
  int num_timestamps = -1;
  std::vector<float> timestamps; 
  ANIM_INTERPOLATION_MODE interpolation_mode = ANIM_INTERPOLATION_MODE::LINEAR;
  float* keyframe_data = NULL;
};

enum class ANIM_TARGET_ON_NODE {
  NONE = 0,
  ROTATION,
  SCALE,
  POSITION
};

struct animation_chunk_data_ref_t {
  int chunk_id = -1;
  ANIM_TARGET_ON_NODE target = ANIM_TARGET_ON_NODE::NONE;
};

int register_anim_data_chunk(animation_data_chunk_t& data);
animation_data_chunk_t* get_anim_data_chunk(int id);

struct animation_globals_t {
  float anim_time = 0;
  float anim_start_time = 0;
  float anim_end_time = 0;
};

void update_animations();
void print_animation_data(std::string& anim_name);
bool is_chunk_in_anim(animation_t& anim, int chunk_id);
