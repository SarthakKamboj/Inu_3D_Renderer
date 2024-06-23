#pragma once

#include "inu_typedefs.h"

#include <vector>
#include <string>

struct animation_t {
  animation_id id = -1;
  std::string name;
  std::vector<anim_chunk_id> data_chunk_ids;
};
int register_animation(animation_t& anim);
void play_next_anim();

enum class ANIM_INTERPOLATION_MODE {
  LINEAR,
  STEP
};

struct animation_data_chunk_t {
  anim_chunk_id id = -1;
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
  anim_chunk_id chunk_id = -1;
  ANIM_TARGET_ON_NODE target = ANIM_TARGET_ON_NODE::NONE;
};

anim_chunk_id register_anim_data_chunk(animation_data_chunk_t& data);
animation_data_chunk_t* get_anim_data_chunk(anim_chunk_id id);

struct obj_anim_attachment_t {
  object_id obj_id; 
  std::vector<animation_chunk_data_ref_t> anim_chunk_refs;
};
void attach_anim_chunk_ref_to_obj(object_id obj_id, animation_chunk_data_ref_t& ref);

struct animation_globals_t {
  float anim_time = 0;
  float anim_start_time = 0;
  float anim_end_time = 0;
};

void update_animations();
void print_animation_data(std::string& anim_name);
bool is_chunk_in_anim(animation_t& anim, anim_chunk_id chunk_id);
