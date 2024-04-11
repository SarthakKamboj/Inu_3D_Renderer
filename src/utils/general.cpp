#include "general.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "log.h"
#include "utils/app_info.h"

char* get_file_contents(const char* full_path) {
  FILE* file = fopen(full_path, "r");
  inu_assert(file, "could not open file to get file contents");

  int size = 100;
  char* contents_buffer = (char*)malloc(size * sizeof(char));
  int count = 0;

  while (!feof(file)) {
    if (count >= size) {
      size *= 2; 
      contents_buffer = (char*)realloc(contents_buffer, size * sizeof(char));
    }
    char c = fgetc(file);
    if (c > 0) {
        contents_buffer[count] = c;
        count++;
    }
  }
  if (count == size) {
    contents_buffer = (char*)realloc(contents_buffer, (size+1) * sizeof(char));
    contents_buffer[size] = 0;
  } else {
    memset(contents_buffer + count, 0, size - count);
  }

  fclose(file);
  return contents_buffer;
}

extern app_info_t app_info;
void get_resources_folder_path(char path_buffer[256]) {	
  // C:\Sarthak\projects\3d_anim_renderer\out\build\Release\three_d_renderer.exe
	static bool got_root_path = false;
	static char s_folder[256]{};
	if (!got_root_path) {
		char folder[256]{};
    GetModuleFileNameA(NULL, folder, 256);
		if (app_info.running_in_vs) { 
      char* slash_before_exe_name = strrchr(folder, '\\');
      *slash_before_exe_name = 0;
		  char* slash_before_vs_sol_config_folder = strrchr(folder, '\\');
		  *slash_before_vs_sol_config_folder = 0;
		  char* slash_before_build_folder = strrchr(folder, '\\');
		  *slash_before_build_folder = 0;
      char* slash_before_out_folder = strrchr(folder, '\\');
		  *slash_before_out_folder = 0;
		} else {
      char* slash_before_exe_name = strrchr(folder, '\\');
      *slash_before_exe_name = 0;
		}

		sprintf(s_folder, "%s\\resources", folder);
		got_root_path = true;
	}
	memcpy(path_buffer, s_folder, strlen(s_folder)); 
}

int sgn(float val) {
  if (val == 0) return 0;
  if (val > 0) return 1;
  return -1;
}

// int clamp(int v, int low, int high) {

