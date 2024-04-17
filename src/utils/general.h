#pragma once

char* get_file_contents(const char* full_path);
void get_resources_folder_path(char path_buffer[256]);
int sgn(float val);

template<typename T>
T clamp(T v, T low, T high) {
  if (v < low) return low;
  if (v > high) return high;
  return v;
}
