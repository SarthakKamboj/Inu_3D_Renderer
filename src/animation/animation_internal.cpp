#include "animation_internal.h"

#include "utils/app_info.h"
#include "scene/scene.h"
#include "utils/log.h"
#include "interpolation.h"
#include "utils/general.h"
#include "windowing/window.h"

#include <unordered_set>
#include <algorithm>

#define STEP_BY_STEP_ANIM 0

extern std::vector<object_t> objs;

animation_globals_t animation_globals;
extern app_info_t app_info;
extern window_t window;

static std::vector<animation_data_chunk_t> anim_data_chunks;
static std::vector<animation_t> animations;

#if STEP_BY_STEP_ANIM
static std::unordered_set<float> timestamps_set;
static std::vector<float> timestamps;
static int frame_idx = 0;
#endif

static int playing_anim_idx = -1;

int register_animation(animation_t& anim) {
  anim.id = animations.size();
  animations.push_back(anim);
  return anim.id;
}

void play_next_anim() {
  if (animations.size() == 0) return;
  int orig = playing_anim_idx;

  playing_anim_idx++;
  if (playing_anim_idx >= animations.size()) {
    playing_anim_idx = 0;
  } 

  if (orig == playing_anim_idx) return;

  animation_t& anim = animations[playing_anim_idx];
  animation_globals.anim_time = 0;
  animation_globals.anim_end_time = 0;
#if STEP_BY_STEP_ANIM
  timestamps_set.clear();
  timestamps.clear();
#endif
  animation_globals.anim_time = 0;
  animation_globals.anim_end_time = 0;
  for (int j = 0; j < anim.data_chunk_ids.size(); j++) {
    int data_chunk_id = anim.data_chunk_ids[j];
    animation_data_chunk_t& adc = anim_data_chunks[data_chunk_id];
    int nt = adc.num_timestamps;
    animation_globals.anim_end_time = adc.timestamps[nt-1];
#if STEP_BY_STEP_ANIM
    timestamps_set.insert(adc.timestamps.begin(), adc.timestamps.end());
#endif
  }

#if STEP_BY_STEP_ANIM
  timestamps.insert(timestamps.end(), timestamps_set.begin(), timestamps_set.end());
  std::sort(timestamps.begin(), timestamps.end());
#endif

}

animation_data_chunk_t* get_anim_data_chunk(int data_id) {
  return &anim_data_chunks[data_id];
}

bool is_chunk_in_anim(animation_t& anim, int chunk_id) {
  return std::find(anim.data_chunk_ids.begin(), anim.data_chunk_ids.end(), chunk_id) != anim.data_chunk_ids.end();
}

void print_animation_data(std::string& anim_name) {
  for (animation_t& anim : animations) {
    if (anim.name != anim_name) continue;
    printf("--- ANIM: %s ---\n", anim_name.c_str());
    std::vector<int> bones = get_bone_objs();
    for (int bone_obj_id : bones) {
      object_t* bone = get_obj(bone_obj_id);
      printf("\n--------- BONE %s --------\n", bone->name.c_str());
      for (animation_chunk_data_ref_t& ref : bone->anim_chunk_refs) {
        if (is_chunk_in_anim(anim, ref.chunk_id)) {
          animation_data_chunk_t* chunk = get_anim_data_chunk(ref.chunk_id);

          if (ref.target == ANIM_TARGET_ON_NODE::POSITION) {
            printf("\n----- POSITION -----\n");
          } else if (ref.target == ANIM_TARGET_ON_NODE::ROTATION) {
            printf("\n----- QUAT -----\n");
          } else if (ref.target == ANIM_TARGET_ON_NODE::SCALE) {
            printf("\n----- SCALE -----\n");
          }

          vec3* vkeyframe = (vec3*)chunk->keyframe_data;
          quaternion_t* qkeyframe = (quaternion_t*)chunk->keyframe_data;
          for (int i = 0; i < chunk->num_timestamps; i++) {
            printf(" [  frame %i  ] ", i);
            // print anim chunk info
            if (ref.target == ANIM_TARGET_ON_NODE::POSITION) {
              print_vec3(vkeyframe[i]);
            } else if (ref.target == ANIM_TARGET_ON_NODE::ROTATION) {
              print_quat(qkeyframe[i]);
            } else if (ref.target == ANIM_TARGET_ON_NODE::SCALE) {
              print_vec3(vkeyframe[i]);
            }
          }
        }
      }
    }
  }
}

int register_anim_data_chunk(animation_data_chunk_t& data) {
  data.id = anim_data_chunks.size();
  anim_data_chunks.push_back(data);
#if 0
  animation_globals.anim_end_time = fmax(animation_globals.anim_end_time, data.timestamps[data.num_timestamps-1]);
  timestamps_set.insert(data.timestamps.begin(), data.timestamps.end());
  timestamps.clear();
  printf("timestamps: ");
  for (int i = 0; i < data.num_timestamps; i++) {
    printf("%f ", data.timestamps[i]);
  }
  printf("\n");
  timestamps.insert(timestamps.end(), timestamps_set.begin(), timestamps_set.end());
  std::sort(timestamps.begin(), timestamps.end());
#endif
  return data.id;
}

void update_animations() {

  if (window.input.right_mouse_up) {
    play_next_anim();
  }

#if STEP_BY_STEP_ANIM
  if (window.input.left_mouse_up) {
    frame_idx++;
    if (frame_idx >= timestamps.size()) {
      frame_idx = 0;
    }
    printf("anim idx: %i\n", frame_idx);
    animation_globals.anim_time = timestamps[frame_idx];
  } 
#else 
  animation_globals.anim_time += app_info.delta_time;
  if (animation_globals.anim_time > animation_globals.anim_end_time) {
    animation_globals.anim_time = animation_globals.anim_start_time;
  }
#endif

  for (object_t& obj : objs) { 
    vec3 orig = obj.transform.pos;
    quaternion_t orig_rot = obj.transform.rot;
    for (animation_chunk_data_ref_t& ref : obj.anim_chunk_refs) {	
      animation_data_chunk_t* chunk = get_anim_data_chunk(ref.chunk_id);
      std::vector<int>& cur_anim_chunks = animations[playing_anim_idx].data_chunk_ids;
      if (std::find(cur_anim_chunks.begin(), cur_anim_chunks.end(), chunk->id) == cur_anim_chunks.end()) {
        continue;
      }

      quaternion_t* rot_anim_data = static_cast<quaternion_t*>((void*)chunk->keyframe_data); 
      vec3* vec3_data = static_cast<vec3*>((void*)chunk->keyframe_data);

      int left_anim_frame_idx = -1;
      int right_anim_frame_idx = -1;
      for (int i = 0; i < chunk->num_timestamps; i++) {
        if (animation_globals.anim_time >= chunk->timestamps[i]) {
          left_anim_frame_idx = i;
          right_anim_frame_idx = i+1;
        }
      }

      if (left_anim_frame_idx != -1) {
        right_anim_frame_idx = clamp<int>(right_anim_frame_idx, 0, chunk->num_timestamps-1);
      }

      if (left_anim_frame_idx == -1) {
        // OUTSIDE ANIMATION FRAMES, CLOSEST ONE IS THE FIRST FRAME

        // quaternion interpolation
        if (ref.target == ANIM_TARGET_ON_NODE::ROTATION) {
          obj.transform.rot = rot_anim_data[0];
          obj.transform.rot = norm_quat(obj.transform.rot);
        }
        // position interpolation
		    else if (ref.target == ANIM_TARGET_ON_NODE::POSITION) {
          obj.transform.pos = vec3_data[0];
		    }
        // scale interpolation
		    else if (ref.target == ANIM_TARGET_ON_NODE::SCALE) {
          obj.transform.scale = vec3_data[0];
		    }
      } else if (left_anim_frame_idx == chunk->num_timestamps-1) {
        // OUTSIDE ANIMATION FRAMES, CLOSEST ONE IS THE LAST FRAME

        // quaternion interpolation
        if (ref.target == ANIM_TARGET_ON_NODE::ROTATION) {
          obj.transform.rot = rot_anim_data[left_anim_frame_idx];
          obj.transform.rot = norm_quat(obj.transform.rot);
        }
        // position interpolation
		    else if (ref.target == ANIM_TARGET_ON_NODE::POSITION) {
          obj.transform.pos = vec3_data[left_anim_frame_idx];
		    }
        // scale interpolation
		    else if (ref.target == ANIM_TARGET_ON_NODE::SCALE) {
          obj.transform.scale = vec3_data[left_anim_frame_idx];
		    }
      } else {
        // ANIMATING BETWEEN FRAMES
        float frame_duration = chunk->timestamps[right_anim_frame_idx] - chunk->timestamps[left_anim_frame_idx];
        float time_into_frame = animation_globals.anim_time - chunk->timestamps[left_anim_frame_idx];
        float t = 0;
        // if there is not just one frame
        if (frame_duration != 0) {
          t = time_into_frame / frame_duration;
        }

        // quaternion interpolation
        if (ref.target == ANIM_TARGET_ON_NODE::ROTATION) {
          if (chunk->interpolation_mode == ANIM_INTERPOLATION_MODE::LINEAR) {
            obj.transform.rot = spherical_linear(rot_anim_data[left_anim_frame_idx], rot_anim_data[right_anim_frame_idx], t);

            float quat_len = quat_mag(obj.transform.rot);
            if (isnan(obj.transform.rot.x) || isnan(obj.transform.rot.y) || isnan(obj.transform.rot.z) || isnan(obj.transform.rot.w)) {
              inu_assert_msg("obj transform rot is nan");
#if 0
              quaternion_t l = rot_anim_data[left_anim_frame_idx];
              quaternion_t r = rot_anim_data[right_anim_frame_idx];
              quaternion_t test = spherical_linear(l, r, t);
#endif
            }
              

          } else if (chunk->interpolation_mode == ANIM_INTERPOLATION_MODE::STEP) {
            obj.transform.rot = rot_anim_data[left_anim_frame_idx];
          }
          obj.transform.rot = norm_quat(obj.transform.rot);
        }
		    // position interpolation
		    else if (ref.target == ANIM_TARGET_ON_NODE::POSITION) {
		      if (chunk->interpolation_mode == ANIM_INTERPOLATION_MODE::LINEAR) {
			      obj.transform.pos = vec3_linear(vec3_data[left_anim_frame_idx], vec3_data[right_anim_frame_idx], t);
            if (isnan(obj.transform.pos.x) || isnan(obj.transform.pos.y) || isnan(obj.transform.pos.z)) {
              inu_assert_msg("obj transform pos is nan");
#if 0
			        vec3 l = vec3_data[left_anim_frame_idx];
			        vec3 r = vec3_data[right_anim_frame_idx];
			        vec3 f = vec3_linear(l, r, t);
#endif
            }
		      } else if (chunk->interpolation_mode == ANIM_INTERPOLATION_MODE::STEP) {
			      obj.transform.pos = vec3_data[left_anim_frame_idx];
		      }
		    }
		    // scale interpolation
		    else if (ref.target == ANIM_TARGET_ON_NODE::SCALE) {
          if (chunk->interpolation_mode == ANIM_INTERPOLATION_MODE::LINEAR) {
			      obj.transform.scale = vec3_linear(vec3_data[left_anim_frame_idx], vec3_data[right_anim_frame_idx], t);
		      } else if (chunk->interpolation_mode == ANIM_INTERPOLATION_MODE::STEP) {
			      obj.transform.scale = vec3_data[left_anim_frame_idx];
		      }
		    }
		  }
    } 
    vec3 diff = {obj.transform.pos.x - orig.x, obj.transform.pos.y - orig.y, obj.transform.pos.z - orig.z };
    inu_assert(length(obj.transform.scale) != 0, "scale is 0");
    if (isnan(obj.transform.pos.x) || isnan(obj.transform.pos.y) || isnan(obj.transform.pos.z)) {
      inu_assert_msg("obj transform pos is nan");
    }
    if (isnan(obj.transform.rot.x) || isnan(obj.transform.rot.y) || isnan(obj.transform.rot.z) || isnan(obj.transform.rot.w)) {
      inu_assert_msg("obj transform rot is nan");
    }
  }

  // update object transforms
  update_obj_model_mats();
}

