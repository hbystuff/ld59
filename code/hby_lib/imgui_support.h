#pragma once

#include "basic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Imgui_Config {
  const char *ttf_font_path;
  float ttf_font_size;
  bool use_bg_alpha;
  float bg_alpha;
} Imgui_Config;

void setup_imgui(void);
void setup_imgui_with_config(Imgui_Config *conf);
bool imgui_wants_mouse(void);
bool imgui_wants_keyboard(void);
void enter_imgui_frame(void);
void draw_imgui(void);
bool imgui_any_hovered(void);
bool imgui_begin_simple_floating_window(const char *title, int x, int y, int w, int h);
bool imgui_begin_simple_side_window(const char *title, bool leave_space_for_menu_bar);

#ifdef __cplusplus
} // extern "C"
#endif
