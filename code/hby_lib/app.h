#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifndef _countof
#define _countof(a) (sizeof(a)/sizeof(a[0]))
#endif

enum {
  KEY_UNKNOWN,

  KEY_MOUSE_LEFT,
  KEY_MOUSE_RIGHT,
  KEY_MOUSE_MIDDLE,

  KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G,
  KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
  KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U,
  KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

  KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6,
  KEY_7, KEY_8, KEY_9,

  KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
  KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,

  KEY_ESCAPE, KEY_RETURN, KEY_LCTRL, KEY_LALT,
  KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN,
  KEY_LSHIFT, KEY_SPACE,
  KEY_MINUS, KEY_EQUAL,

  KEY_RALT, KEY_RSHIFT, KEY_RCTRL,

  KEY_TAB, KEY_BACKSPACE, KEY_DELETE,
  KEY_COMMA, KEY_PERIOD,

  KEY_BACKTICK,

  KEY_MAX_
};

enum {
  KEY_MOUSE_MAX_ = KEY_MOUSE_MIDDLE,
  KEY_MOUSE_MIN_ = KEY_MOUSE_LEFT,
};

typedef struct App_Monitor_Info {
  int index;
  int width;
  int height;
  char name[64];
} App_Monitor_Info;

typedef void App_On_Drop_File(const char *path);

typedef struct App_Config {
  int width;
  int height;
  const char *title;
  bool full_screen;
  bool use_system_dpi;
  bool allow_resize;
  bool disable_ctrl_w_to_quit;
  bool use_default_cwd;
  bool transparent_background; // Win32: request a composited OpenGL surface with alpha for transparent window backgrounds.
  int start_monitor;
  const char *start_monitor_name;

  App_On_Drop_File *on_drop_file;
} App_Config;

typedef enum App_Cursor {
  APP_CURSOR_NORMAL,
  APP_CURSOR_TEXT_SELECT,
  APP_CURSOR_HAND,
  APP_CURSOR_WAIT,
  APP_CURSOR_COUNT_
} App_Cursor;

typedef struct App_File_Iterator {
  void *handle;
  char file_name[512];
  bool is_directory;
} App_File_Iterator;

typedef struct App_File_Info {
  bool exists;
  bool is_directory;
  char file_name[256];
} App_File_Info;

typedef struct App_Thread_Handle {
  void *handle;
} App_Thread_Handle;

typedef struct App_File_Handle {
  void *value;
} App_File_Handle;

typedef enum App_File_Mode {
  APP_FILE_MODE_READ,
  APP_FILE_MODE_WRITE,
  APP_FILE_MODE_READ_WRITE,
  APP_FILE_MODE_APPEND,
} App_File_Mode;

#ifdef __cplusplus
extern "C" {
#endif

bool app_get_monitor(int index, App_Monitor_Info *info);
int app_join_paths(const char *a, const char *b, char *out_buf, int buf_capacity_including_null_term);
float app_get_dpi_scale(void);
void app_setup(App_Config conf);
void app_setup_p(App_Config *conf);
void app_set_cursor(App_Cursor cursor);
float app_get_fps(void);
void *app_load_gl_function(const char *name); // TODO: Make this platform specific.
int app_get_char_input(int index);
float app_mouse_wheel_y(void);
int app_mouse_x(void);
int app_mouse_y(void);
int app_get_width(void);
int app_get_height(void);
bool app_keydown(int key);
bool app_keypress(int key);
bool app_double_click(int key);
bool app_keypress_allow_repeat(int key);
bool app_keyrelease(int key);
void app_quit(void);
void app_run(void (*proc)(void));
float app_delta(void);
double app_time(void);
void app_show_cursor(bool show);
double app_last_time(void);
uint64_t app_frame_count();
bool app_iterate_directory(const char *path, App_File_Iterator *enumerator);
void app_toggle_fullscreen(void);
void app_set_mouse_pos(int x, int y);
bool app_has_focus(void);
float app_get_mouse_delta_x(void);
float app_get_mouse_delta_y(void);
bool app_is_full_screen(void);
void app_set_title(const char *title);
void *app_launch_another_instance(const char *args);
App_Thread_Handle app_new_thread(int (*proc)(void *arg), void *arg);
void app_sleep(double seconds);
void *get_win32_window_handle(void);
void *app_get_win32_dc(void);
bool app_set_clipboard(const char *text);
const char *app_get_clipboard(void);

uint64_t app_get_file_write_timestamp(const char *path);
bool app_copy_file(const char *path, const char *dest_path, bool force);
bool app_delete_file(const char *path);
bool app_copy_file_from_to(const char *from, const char *to);
App_File_Handle app_open_file(const char *path, App_File_Mode mode);
void app_close_file(App_File_Handle handle);
int app_write_file(const char *path, const void *ptr, int size);
int app_write_file_handle(App_File_Handle handle, const void *ptr, int size);

const char *app_win32_get_executable_dir(void);
bool app_win32_open_file_dialog(char *filename_buf, size_t filename_buf_size);
bool app_win32_save_file_dialog(char *filename_buf, size_t filename_buf_size);

void app_debug_break(void); // Breaks in the debugger if debugger is present. Only works on windows with visual studio

#ifdef __cplusplus
} // extern "C"
#endif

#if defined(_WIN32)

#include "win32.h"
#define APP_WINMAIN() int __stdcall WinMain( \
  HINSTANCE hInstance, \
  HINSTANCE hPrevInstance, \
  char *lpCmdLine, \
  int   nShowCmd)

#if defined(HBY_RELEASE)

#define APP_MAIN() APP_WINMAIN()
#else // defined(HBY_RELEASE)
#define APP_MAIN() int main(void)
#endif // defined(HBY_RELEASE)

// defined(_WIN32)
#elif defined(__EMSCRIPTEN__)
#define APP_MAIN() int main(void)
#endif
