#include "app.h"
#include "gfx.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static struct {
  float delta;
  int  width;
  int  height;

  bool keys_delta[KEY_MAX_];
  bool keys_delta_repeat[KEY_MAX_];
  bool keys_double_click[KEY_MAX_];
  bool keys_down[KEY_MAX_];

  double double_click_timers[3];

  bool   quit;
  double last_frame_time;
  int    mouse_x;
  int    mouse_y;
  float  wheel_y;
  float  mouse_delta_x;
  float  mouse_delta_y;

  int       window_width; // window height when not in full screen
  int       window_height;
  bool      is_full_screen;
  bool      allow_resize;
  unsigned *text_input;
  int       text_input_size;
  int       text_input_capacity;
  uint64_t  frame_count;
  float     delta_accum[16];
  int       delta_accum_cursor;

  int  mouse_pos_requested_x;
  int  mouse_pos_requested_y;
  bool mouse_pos_requested;

  bool ctrl_w_to_quit;
  bool is_ready;

  App_On_Drop_File *on_drop_file;

  int start_monitor;
} app_;

static void app_update_delta_accum(void) {
  app_.delta_accum[app_.delta_accum_cursor++] = app_delta();
  app_.delta_accum_cursor %= _countof(app_.delta_accum);
}

bool app_is_full_screen(void) {
  return app_.is_full_screen;
}

float app_get_fps(void) {
  float accum = 0;
  int i;
  for (i = 0; i < _countof(app_.delta_accum); i++) {
    accum += app_.delta_accum[i];
  }
  if (accum == 0)
    return 0;
  return 1.f / (accum / _countof(app_.delta_accum));
}

float app_mouse_wheel_y(void) {
  return app_.wheel_y;
}

int app_mouse_x(void) {
  return app_.mouse_x;
}

int app_mouse_y(void) {
  return app_.mouse_y;
}

int app_get_width() {
  return app_.width;
}
int app_get_height() {
  return app_.height;
}

double app_last_time() {
  return app_.last_frame_time;
}

float app_delta(void) {
  return app_.delta;
}

int app_join_paths(const char *a, const char *b, char *out_buf, int buf_capacity_including_null_term) {
  int buf_capacity = buf_capacity_including_null_term;
  int joined_length = 0;
  int a_len = (!a || !a[0]) ? 0 : (int)strlen(a);
  int b_len = (!b || !b[0]) ? 0 : (int)strlen(b);
  char *out_buf_ptr = out_buf;

  if (!a_len && !b_len) {
    joined_length = 0;
    if (buf_capacity)
      out_buf[0] = 0;
  }
  else if (!a_len) {
    if (buf_capacity && buf_capacity < joined_length+1)
      return 0;
    if (buf_capacity) {
      memcpy(out_buf, b, b_len);
      out_buf[b_len] = 0;
    }
  }
  else if (!b_len) {
    if (buf_capacity && buf_capacity < joined_length+1)
      return 0;
    if (buf_capacity) {
      memcpy(out_buf, a, a_len);
      out_buf[a_len] = 0;
    }
  }
  else {
    bool has_delimiter_already = a[a_len-1] == '/';
    if (has_delimiter_already) {
      joined_length = a_len + b_len;
    }
    else {
      joined_length = a_len + b_len + 1;
    }
    if (buf_capacity && buf_capacity < joined_length+1)
      return 0;

    if (buf_capacity) {
      memcpy(out_buf_ptr, a, a_len);
      out_buf_ptr += a_len;

      if (!has_delimiter_already) {
        memcpy(out_buf_ptr, "/", 1);
        out_buf_ptr += 1;
      }

      memcpy(out_buf_ptr, b, b_len);
      out_buf_ptr += b_len;

      *out_buf_ptr = 0;
    }
  }

  return joined_length;
}

int app_get_char_input(int index) {
  if (index < app_.text_input_size)
    return app_.text_input[index];
  return 0;
}
uint64_t app_frame_count() {
  return app_.frame_count;
}

static void app_setup_default_conf_(App_Config *conf) {
  conf->title  = conf->title  ? conf->title  : "Untitled";
  conf->width  = conf->width  ? conf->width  : 800;
  conf->height = conf->height ? conf->height : 600;
}

static void app_on_text_input_(unsigned int char_input) {
  if (app_.text_input_size >= app_.text_input_capacity) {
    app_.text_input_capacity = app_.text_input_capacity ? app_.text_input_capacity * 2 : 256;
    app_.text_input = (unsigned *)realloc(app_.text_input, sizeof(unsigned) * app_.text_input_capacity);
  }
  app_.text_input[app_.text_input_size++] = char_input;
}

static void app_on_keydown_(int key, bool repeat) {
  if (key) {
    if (!repeat) {
      app_.keys_down[key] = true;
      app_.keys_delta[key] = true;
      int double_click_timer_index = -1;
      switch (key) {
      case KEY_MOUSE_LEFT:
        double_click_timer_index = 0;
        break;
      case KEY_MOUSE_MIDDLE:
        double_click_timer_index = 1;
        break;
      case KEY_MOUSE_RIGHT:
        double_click_timer_index = 2;
        break;
      }
      const double double_click_threshold = 0.15;
      if (double_click_timer_index != -1) {
        double *last_click = &app_.double_click_timers[double_click_timer_index];
        double cur_time = app_time();
        if (cur_time - *last_click < double_click_threshold) {
          app_.keys_double_click[key] = true;
          *last_click = 0;
        }
        else {
          *last_click = cur_time;
        }
      }
    }
    app_.keys_delta_repeat[key] = 1;
  }
}

static void app_on_keyup_(int key) {
  if (key) {
    app_.keys_down[key] = 0;
    app_.keys_delta[key] = 1;
  }
}

static void clear_keydowns(void) {
  memset(app_.keys_down, 0, sizeof(app_.keys_delta));
}
static void reset_input(void) {
  memset(app_.keys_delta, 0, sizeof(app_.keys_delta));
  memset(app_.keys_delta_repeat, 0, sizeof(app_.keys_delta_repeat));
  memset(app_.keys_double_click, 0, sizeof(app_.keys_double_click));
  app_.wheel_y = 0;
  app_.text_input_size = 0;
  app_.mouse_delta_x = 0;
  app_.mouse_delta_y = 0;
}

static void update_timing() {
  double now = app_time();
  app_.delta = (float)(now - app_.last_frame_time);
  app_.last_frame_time = now;
  app_.frame_count++;
}

bool app_keydown(int key) {
  assert(key >= 0 && key < KEY_MAX_);
  return app_.keys_down[key];
}
bool app_keypress(int key) {
  assert(key >= 0 && key < KEY_MAX_);
  return app_.keys_down[key] && app_.keys_delta[key];
}
bool app_double_click(int key) {
  return app_.keys_double_click[key];
}
bool app_keypress_allow_repeat(int key) {
  return app_.keys_down[key] && app_.keys_delta_repeat[key];
}
bool app_keyrelease(int key) {
  assert(key >= 0 && key < KEY_MAX_);
  return !app_.keys_down[key] && app_.keys_delta[key];
}
void app_quit() {
  app_.quit = 1;
}

#if defined(_WIN32)
/* { */

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <commdlg.h>
#include <wingdi.h>
#include <shellapi.h>
#include <dwmapi.h>
//#include <Shlobj_core.h>
//#include <shellscalingapi.h>
#include "Shlobj.h"

#pragma comment(lib, "user32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "opengl32")
#pragma comment(lib, "shell32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "dwmapi")

#ifndef PFD_SUPPORT_COMPOSITION
#define PFD_SUPPORT_COMPOSITION 0x00008000
#endif

#ifndef WGL_DRAW_TO_WINDOW_ARB
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#endif
#ifndef WGL_SUPPORT_OPENGL_ARB
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#endif
#ifndef WGL_DOUBLE_BUFFER_ARB
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#endif
#ifndef WGL_PIXEL_TYPE_ARB
#define WGL_PIXEL_TYPE_ARB 0x2013
#endif
#ifndef WGL_COLOR_BITS_ARB
#define WGL_COLOR_BITS_ARB 0x2014
#endif
#ifndef WGL_ALPHA_BITS_ARB
#define WGL_ALPHA_BITS_ARB 0x201B
#endif
#ifndef WGL_DEPTH_BITS_ARB
#define WGL_DEPTH_BITS_ARB 0x2022
#endif
#ifndef WGL_STENCIL_BITS_ARB
#define WGL_STENCIL_BITS_ARB 0x2023
#endif
#ifndef WGL_TYPE_RGBA_ARB
#define WGL_TYPE_RGBA_ARB 0x202B
#endif
#ifndef WGL_SUPPORT_COMPOSITION_ARB
#define WGL_SUPPORT_COMPOSITION_ARB 0x200C
#endif

static struct {
  HWND   window;
  HDC    dc;
  double frequency;
  bool   use_system_dpi;
  bool   transparent_background;
  HCURSOR cursors[APP_CURSOR_COUNT_];
  App_Cursor next_cursor;
  App_Cursor cur_cursor;
  char *clipboard_text;

  WCHAR *executable_dir_w;
  char *executable_dir;

  bool had_focus;
  bool has_focus;
} app_win32_;

typedef BOOL (__stdcall *wglChoosePixelFormatARB_t)(
    HDC hdc,
    const int *piAttribIList,
    const FLOAT *pfAttribFList,
    UINT nMaxFormats,
    int *piFormats,
    UINT *nNumFormats);

static char *app_win32_from_wstring(const WCHAR *path16) {
  int len16 = 0;
  int len = 0;
  char *ret = NULL;
  if (!path16 || !path16[0])
    return NULL;
  len16 = (int)wcslen(path16);
  len = WideCharToMultiByte(CP_UTF8, 0, path16, len16, NULL, 0, NULL, NULL);
  ret = (char *)malloc((len+1) * sizeof(char));
  if (len != WideCharToMultiByte(CP_UTF8, 0, path16, len16, ret, len+1, NULL, NULL)) {
    free(ret);
    ret = NULL;
  } else {
    ret[len] = 0;
  }
  return ret;
}

static WCHAR *app_win32_to_wstring(const char *path) {
  int len = 0;
  int len16 = 0;
  WCHAR *ret = NULL;
  if (!path || !path[0])
    return NULL;
  len = (int)strlen(path);
  len16 = MultiByteToWideChar(CP_UTF8, 0, path, len, NULL, 0);
  ret = (WCHAR *)malloc((len16+1) * sizeof(WCHAR));
  if (len16 != MultiByteToWideChar(CP_UTF8, 0, path, len, ret, len16+1)) {
    free(ret);
    ret = NULL;
  }
  else {
    ret[len16] = 0;
  }
  return ret;
}

const char *app_get_clipboard(void) {
  while (!OpenClipboard(app_win32_.window)) {
    return NULL;
  }

  HANDLE handle = GetClipboardData(CF_UNICODETEXT);
  if (!handle) {
    CloseClipboard();
    return NULL;
  }

  WCHAR *text16 = (WCHAR *)GlobalLock(handle);
  if (!text16) {
      CloseClipboard();
      return NULL;
  }

  free(app_win32_.clipboard_text);
  app_win32_.clipboard_text = app_win32_from_wstring(text16);

  GlobalUnlock(handle);
  CloseClipboard();

  return app_win32_.clipboard_text;
}

void app_set_title(const char *title) {
  WCHAR *title_w = app_win32_to_wstring(title);
  SetWindowTextW(app_win32_.window, title_w);
  free(title_w);
}

bool app_set_clipboard(const char *text) {
  int length_with_null_term = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
  if (!length_with_null_term)
    return false;

  HANDLE object = GlobalAlloc(GMEM_MOVEABLE, length_with_null_term * sizeof(WCHAR));
  if (!object) {
    return false;
  }

  WCHAR *buffer = (WCHAR *)GlobalLock(object);
  if (!buffer) {
    return false;
  }

  MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer, length_with_null_term);
  GlobalUnlock(object);

  while (!OpenClipboard(app_win32_.window)) {
    return false;
  }

  EmptyClipboard();
  SetClipboardData(CF_UNICODETEXT, object);
  CloseClipboard();
  return true;
}

uint64_t app_get_file_write_timestamp(const char *path) {
  WCHAR path16[MAX_PATH];
  SECURITY_ATTRIBUTES secrity_attr = {0};
  HANDLE file_handle = 0;
  FILETIME write_time = {0};
  uint64_t ret = 0;

  int len = (int)strlen(path);
  int realLen = MultiByteToWideChar(CP_UTF8, 0, path, len, path16, _countof(path16));
  if (realLen != len)
    return 0;
  if (realLen >= _countof(path16))
    return 0;
  path16[realLen] = 0;

  secrity_attr.nLength = sizeof(secrity_attr);
  file_handle = CreateFileW(path16, FILE_SHARE_READ, 0, &secrity_attr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (INVALID_HANDLE_VALUE == file_handle)
    return 0;

  if (GetFileTime(file_handle, NULL, NULL, &write_time)) {
    ret =
      (uint64_t)write_time.dwLowDateTime |
      ((uint64_t)write_time.dwHighDateTime << 32);
  }

  CloseHandle(file_handle);
  return ret;
}

bool app_copy_file(const char *path, const char *dest_path, bool force) {
  bool ret = 0;
  WCHAR *path16 = app_win32_to_wstring(path);
  if (path16) {
    WCHAR *dest_path16 = app_win32_to_wstring(dest_path);
    if (dest_path16) {
      ret = !!CopyFileW(path16, dest_path16, !force);
      free(dest_path16);
    }
    free(path16);
  }
  return ret;
}

bool app_copy_file_from_to(const char *from, const char *to) {
  return app_copy_file(from, to, true);
}

bool app_delete_file(const char *path) {
  WCHAR *path16 = app_win32_to_wstring(path);
  BOOL status = FALSE;
  if (!path16)
    return 0;
  status = DeleteFileW(path16);
  free(path16);
  return status != FALSE;
}

int app_write_file(const char *path, const void *ptr, int size) {
  int ret = 0;
  App_File_Handle handle = app_open_file(path, APP_FILE_MODE_WRITE);
  if (handle.value) {
    ret = app_write_file_handle(handle, ptr, size);
    app_close_file(handle);
  }
  return ret;
}

static void app_normalize_path_(char *path) {
  size_t size = 0;
  size_t last_slash_i = 0;
  size_t i = 0;
  if (!path) return;
  size = strlen(path);
  last_slash_i = -1;
  for (i = 0; i < size; i++) {
    if (path[i] == '\\' || path[i] == '/') {
      if (last_slash_i + 1 == i) {
        memmove(path+i, path+i+1, size-(i+1));
        path[--size] = 0;
      }
      else {
        path[i] = '/';
        last_slash_i = i;
      }
    }
  }

  if (last_slash_i+1 == size) {
    path[last_slash_i] = 0;
  }
}

static char *app_join_paths_alloc(const char *a, const char *b) {
  int size = app_join_paths(a, b, NULL, 0);
  char *ret = (char *)malloc(size+1);
  if (ret) {
    if (size != app_join_paths(a, b, ret, size+1)) {
      free(ret);
      ret = NULL;
    }
  }
  return ret;
}

static char *app_extract_directory_name(const char *path) {
  int len = 0;
  char *ret = 0;
  int i = 0;
  if (!path || !path[0])
    return NULL;

  len = (int)strlen(path);
  ret = (char *)malloc(len+1);
  if (!ret) return NULL;

  memcpy(ret, path, sizeof(path[0])*len);
  ret[len] = 0;

  for (i = len - 1; i >= 0; i--) {
    if (ret[i] == '/') {
      ret[i] = 0;
      break;
    }
  }

  app_normalize_path_(ret);
  return ret;
}

App_File_Handle app_open_file(const char *path, App_File_Mode mode) {
  App_File_Handle ret = {0};
  WCHAR *path16 = app_win32_to_wstring(path);
  if (path16) {
    DWORD access = 0;
    DWORD create_type = 0;

    SECURITY_ATTRIBUTES secrity_attr = {0};
    secrity_attr.nLength = sizeof(secrity_attr);

    switch (mode) {
    case APP_FILE_MODE_READ:
      access = FILE_SHARE_READ;
      create_type = OPEN_EXISTING;
      break;
    case APP_FILE_MODE_WRITE:
      access = FILE_SHARE_WRITE;
      create_type = CREATE_ALWAYS;
      break;
    case APP_FILE_MODE_READ_WRITE:
      access = FILE_SHARE_WRITE | FILE_SHARE_READ;
      create_type = OPEN_EXISTING;
      break;
    case APP_FILE_MODE_APPEND:
      access = FILE_APPEND_DATA;
      create_type = OPEN_EXISTING;
      break;
    }
    {
      HANDLE file_handle =
        CreateFileW(path16, access, create_type, &secrity_attr, create_type, FILE_ATTRIBUTE_NORMAL, NULL);
      if (file_handle != INVALID_HANDLE_VALUE) {
        ret.value = file_handle;
      }
    }
    free(path16);
  }
  return ret;
}

void app_close_file(App_File_Handle handle) {
  if (!handle.value)
    return;
  CloseHandle(handle.value);
}

size_t app_get_file_size(App_File_Handle handle) {
  size_t ret = 0;
  if (handle.value) {
    LARGE_INTEGER large_size = {0};
    if (GetFileSizeEx(handle.value, &large_size)) {
      if (sizeof(size_t) > sizeof(uint32_t)) {
        ret = large_size.QuadPart;
      } else if (0 == large_size.HighPart) {
        ret = large_size.LowPart;
      }
    }
  }
  return ret;
}

size_t app_read_whole_file(App_File_Handle handle, size_t size, char *out_data) {
  size_t ret = 0;
  DWORD num_read = 0;
  if (!handle.value) return ret;
  if (ReadFile(handle.value, out_data, (DWORD)size, &num_read, NULL)) {
    ret = (size_t)num_read;
  }
  return ret;
}

int app_write_file_handle(App_File_Handle handle, const void *ptr, int size) {
  int ret = 0;
  if (handle.value) {
    DWORD written = 0;
    if (WriteFile(handle.value, ptr, size, &written, NULL)) {
      ret = written;
    }
  }
  return ret;
}

void app_debug_break(void) {
  if (IsDebuggerPresent()) {
    DebugBreak();
  }
}

bool app_iterate_directory_impl(const char *path, App_File_Iterator *enumerator) {

  WIN32_FIND_DATAA find_data = {0};

  while (1) {
    if (!enumerator->handle) {
      char search_term[MAX_PATH] = {0};
      size_t path_len = strlen(path);
      if (sizeof(search_term)+2+1 <= path_len)
        return 0;

      memcpy(search_term, path, path_len);
      search_term[path_len] = '/';
      search_term[path_len+1] = '*';
      search_term[path_len+2] = '\0';

      // TODO: Use wide string
      {
        HANDLE handle = FindFirstFileA(search_term, &find_data);
        if (!handle)
          return 0;
        enumerator->handle = handle;
      }
    } else {
      if (!FindNextFileA(enumerator->handle, &find_data)) {
        return 0;
      }
    }

    if (!strcmp(find_data.cFileName, ".") ||
        !strcmp(find_data.cFileName, ".."))
    {
      continue;
    }

    enumerator->is_directory = find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

    {
      size_t name_len = strlen(find_data.cFileName);
      if (sizeof(enumerator->file_name) < name_len+1) {
        return 0;
      }
      memcpy(enumerator->file_name, find_data.cFileName, name_len+1);
    }
    break;
  }

  return 1;
}

bool app_iterate_directory(const char *path, App_File_Iterator *enumerator) {
  bool ret = app_iterate_directory_impl(path, enumerator);
  if (!ret) {
    if (enumerator->handle) {
      FindClose(enumerator->handle);
      memset(enumerator, 0, sizeof(*enumerator));
    }
  }
  return ret;
}

void *app_get_win32_dc(void) {
  return app_win32_.dc;
}

void *get_win32_window_handle(void) {
  return app_win32_.window;
}

int win32_translate_key(WPARAM syskey, LPARAM lparam) {
  if (syskey >= 0x41 && syskey <= 0x5A) return (int)syskey - 0x41 + KEY_A;
  if (syskey >= 0x30 && syskey <= 0x39) return (int)syskey - 0x30 + KEY_0;
  if (syskey >= VK_F1 && syskey <= VK_F12) return (int)syskey - VK_F1 + KEY_F1;
  switch (syskey) {
  case VK_TAB: return KEY_TAB;
  case VK_BACK: return KEY_BACKSPACE;
  case VK_DELETE: return KEY_DELETE;
  case VK_ESCAPE: return KEY_ESCAPE;
  case VK_LEFT: return KEY_LEFT;
  case VK_RIGHT: return KEY_RIGHT;
  case VK_UP: return KEY_UP;
  case VK_DOWN: return KEY_DOWN;
  case VK_SPACE: return KEY_SPACE;
  case VK_RETURN: return KEY_RETURN;
  case VK_OEM_MINUS: return KEY_MINUS;
  case VK_OEM_PLUS: return KEY_EQUAL;
  case 192: return KEY_BACKTICK;

  case VK_CONTROL:
    if (lparam >> 24 & 0x1)
      return KEY_RCTRL;
    else
      return KEY_LCTRL;

  case VK_SHIFT:
    if (lparam >> 24 & 0x1)
      return KEY_RSHIFT;
    else
      return KEY_LSHIFT;

  case VK_OEM_PERIOD:
    return KEY_PERIOD;
  case VK_OEM_COMMA:
    return KEY_COMMA;

  case VK_MENU:
    if (lparam >> 24 & 0x1)
      return KEY_RALT;
    else
      return KEY_LALT;
  }
  return 0;
}

LRESULT CALLBACK win32_window_proc(
  _In_ HWND   hwnd,
  _In_ UINT   uMsg,
  _In_ WPARAM wParam,
  _In_ LPARAM lParam
) {
  if (hwnd && hwnd != app_win32_.window) {
    return DefWindowProcW(hwnd,uMsg,wParam,lParam);
  }

  switch (uMsg) {
    case WM_QUIT:
    case WM_DESTROY:
      app_.quit = 1;
      break;

    case WM_DROPFILES:
    {
      if (app_.on_drop_file) {
        HDROP drop_handle = (HDROP)wParam;
        WCHAR *buf = NULL;
        UINT name_buffer_capacity = 0;
        UINT count = DragQueryFileW(drop_handle, 0xffffffff, NULL, 0);
        for (UINT i = 0; i < count; i++) {
          UINT name_buffer_len = DragQueryFileW(drop_handle, i, NULL, 0);
          if (name_buffer_len > name_buffer_capacity) {
            name_buffer_capacity = MAX(1, name_buffer_len);
            buf = (WCHAR *)realloc(buf, (name_buffer_capacity+1) * sizeof(WCHAR));
          }
          DragQueryFileW(drop_handle, i, buf, name_buffer_capacity+1);
          char *buf8 = app_win32_from_wstring(buf);

          app_.on_drop_file(buf8);
          free(buf8);
        }
        free(buf);
        DragFinish(drop_handle);
      }
    } break;

    case WM_SIZE:
    {
      UINT width = LOWORD(lParam);
      UINT height = HIWORD(lParam);
      app_.width  = width;
      app_.height = height;
    } break;

    case WM_ERASEBKGND:
      if (app_win32_.transparent_background) {
        return 1;
      }
      break;

    case WM_KILLFOCUS:
    {
      reset_input();
      clear_keydowns();
    } break;

    case WM_UNICHAR:
    {
      if (wParam == UNICODE_NOCHAR)
        return TRUE;
    } // Fallthrough
    case WM_CHAR:
    {
      if (wParam >= 0x00 && wParam <= 0x08) {}
      else if (wParam >= 0x0b && wParam <= 0x1f) {}
      else {
        app_on_text_input_((unsigned)wParam);
      }
    } break;

    case WM_LBUTTONDOWN:
      SetCapture(hwnd);
      app_on_keydown_(KEY_MOUSE_LEFT, 0);
      break;
    case WM_LBUTTONUP:
      ReleaseCapture();
      app_on_keyup_(KEY_MOUSE_LEFT);
      break;
    case WM_RBUTTONDOWN:
      SetCapture(hwnd);
      app_on_keydown_(KEY_MOUSE_RIGHT, 0);
      break;
    case WM_RBUTTONUP:
      ReleaseCapture();
      app_on_keyup_(KEY_MOUSE_RIGHT);
      break;
    case WM_MBUTTONDOWN:
      SetCapture(hwnd);
      app_on_keydown_(KEY_MOUSE_MIDDLE, 0);
      break;
    case WM_MBUTTONUP:
      ReleaseCapture();
      app_on_keyup_(KEY_MOUSE_MIDDLE);
      break;

    case WM_MOUSEWHEEL:
    {
      INT16 val = (INT16)((wParam & 0xFFFF0000) >> 16) / (INT16)WHEEL_DELTA;
      app_.wheel_y = val;
    } break;

    case WM_SYSCOMMAND:
    {
      if ((wParam & 0xFFF0) == SC_KEYMENU) {
        return (0);
      }
    } break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
      int key = win32_translate_key(wParam, lParam);
      if (key)
        app_on_keyup_(key);
    } break;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
      int key = win32_translate_key(wParam, lParam);
      if (key)
        app_on_keydown_(key, (lParam & 0x40000000) != 0);
      if (key == VK_MENU || key == WM_MENUCHAR)
        return 0;
    } break;
  }
  return DefWindowProcW(hwnd,uMsg,wParam,lParam);
}

static bool app_win32_open_file_dialog_impl(bool is_open, char *filename_buf, size_t filename_buf_size) {
  WCHAR *buf_w = 0;
  OPENFILENAMEW info = {0};
  bool ret = 0;

  if (!filename_buf_size)
    return 0;

  buf_w = (WCHAR *)malloc(filename_buf_size * sizeof(WCHAR));
  if (!buf_w) {
    return 0;
  }
  buf_w[0] = 0;

  if (is_open) {
    info.Flags |= OFN_FILEMUSTEXIST;
  }
  else {
    info.Flags |= OFN_OVERWRITEPROMPT;
  }

  info.lStructSize = sizeof(info);
  info.hwndOwner = app_win32_.window;
  info.hInstance = GetModuleHandle(0);
  info.lpstrFilter = L"All Files\0*.*\0";
  info.lpstrFile = buf_w;
  info.nMaxFile = (DWORD)filename_buf_size;

  if (is_open) {
    ret = GetOpenFileNameW(&info);
  }
  else {
    ret = GetSaveFileNameW(&info);
  }
  if (ret) {
    WideCharToMultiByte(CP_UTF8, 0, buf_w, (int)filename_buf_size, filename_buf, (int)filename_buf_size, NULL, NULL);
  }
  free(buf_w);
  return ret;
}

bool app_win32_save_file_dialog(char *filename_buf, size_t filename_buf_size) {
  return app_win32_open_file_dialog_impl(0, filename_buf, filename_buf_size);
}

bool app_win32_open_file_dialog(char *filename_buf, size_t filename_buf_size) {
  return app_win32_open_file_dialog_impl(1, filename_buf, filename_buf_size);
}

static void init_gl(HDC dc) {
  if (dc != NULL) {
    HGLRC glContext = 0;
    PIXELFORMATDESCRIPTOR pixelFormat = {0};
    int selected_pixel_format = 0;
    pixelFormat.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pixelFormat.nVersion = 1;
    pixelFormat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    if (app_win32_.transparent_background) {
      pixelFormat.dwFlags |= PFD_SUPPORT_COMPOSITION;
    }
    pixelFormat.iPixelType = PFD_TYPE_RGBA;
    pixelFormat.cColorBits = 32;
    pixelFormat.cAlphaBits = app_win32_.transparent_background ? 8 : 0;
    pixelFormat.cAccumBits = 0;
    pixelFormat.cDepthBits = 24;
    pixelFormat.cStencilBits = 8;
    pixelFormat.cAuxBuffers = 0;
    pixelFormat.iLayerType = PFD_MAIN_PLANE;
    selected_pixel_format = ChoosePixelFormat(dc, &pixelFormat);

    if (app_win32_.transparent_background) {
      const WCHAR *dummy_class_name = L"app-dummy-gl-window";
      static bool dummy_class_registered = false;
      HINSTANCE hInstance = GetModuleHandle(0);
      HWND dummy_window = NULL;
      HDC dummy_dc = NULL;
      HGLRC dummy_gl_context = 0;

      if (!dummy_class_registered) {
        WNDCLASSW dummy_window_class = {0};
        dummy_window_class.style = CS_OWNDC;
        dummy_window_class.lpfnWndProc = DefWindowProcW;
        dummy_window_class.hInstance = hInstance;
        dummy_window_class.lpszClassName = dummy_class_name;
        dummy_class_registered = RegisterClassW(&dummy_window_class) != 0;
      }

      if (dummy_class_registered) {
        dummy_window = CreateWindowW(
            dummy_class_name, L"", WS_OVERLAPPEDWINDOW,
            0, 0, 1, 1, 0, 0, hInstance, 0);
      }

      if (dummy_window) {
        PIXELFORMATDESCRIPTOR dummy_pixel_format = pixelFormat;
        int dummy_pixel_format_id = 0;
        dummy_dc = GetDC(dummy_window);
        if (dummy_dc) {
          dummy_pixel_format_id = ChoosePixelFormat(dummy_dc, &dummy_pixel_format);
          if (dummy_pixel_format_id &&
              SetPixelFormat(dummy_dc, dummy_pixel_format_id, &dummy_pixel_format))
          {
            dummy_gl_context = wglCreateContext(dummy_dc);
          }
        }
      }

      if (dummy_gl_context && wglMakeCurrent(dummy_dc, dummy_gl_context)) {
        wglChoosePixelFormatARB_t wglChoosePixelFormatARB = 0;
        wglChoosePixelFormatARB =
          (wglChoosePixelFormatARB_t)wglGetProcAddress("wglChoosePixelFormatARB");
        if (wglChoosePixelFormatARB) {
          static const int pixel_format_attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, TRUE,
            WGL_SUPPORT_OPENGL_ARB, TRUE,
            WGL_DOUBLE_BUFFER_ARB, TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB, 32,
            WGL_ALPHA_BITS_ARB, 8,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,
            WGL_SUPPORT_COMPOSITION_ARB, TRUE,
            0,
          };
          UINT format_count = 0;
          int transparent_pixel_format = 0;
          if (wglChoosePixelFormatARB(
                  dc,
                  pixel_format_attribs,
                  NULL,
                  1,
                  &transparent_pixel_format,
                  &format_count) &&
              format_count > 0 &&
              transparent_pixel_format)
          {
            selected_pixel_format = transparent_pixel_format;
          }
        }
      }

      if (dummy_gl_context) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_gl_context);
      }
      if (dummy_dc) {
        ReleaseDC(dummy_window, dummy_dc);
      }
      if (dummy_window) {
        DestroyWindow(dummy_window);
      }
    }

    if (selected_pixel_format) {
      PIXELFORMATDESCRIPTOR selected_pixel_desc = {0};
      DescribePixelFormat(dc, selected_pixel_format, sizeof(selected_pixel_desc), &selected_pixel_desc);
      SetPixelFormat(dc, selected_pixel_format, &selected_pixel_desc);
    }

    glContext = wglCreateContext(dc);
    wglMakeCurrent(dc, glContext);

    { // Specify the required opengl version. Amd drivers sometimes need this.
      typedef HGLRC (__stdcall *wglCreateContextAttribsARB_t)(HDC hDC, HGLRC hShareContext, const int *attribList);
      wglCreateContextAttribsARB_t wglCreateContextAttribsARB = 0;
      wglCreateContextAttribsARB = (wglCreateContextAttribsARB_t)wglGetProcAddress("wglCreateContextAttribsARB");
      if (wglCreateContextAttribsARB) {
        static const int attribs[] = {
          0x2091 /* major version */, 3,
          0x2092 /* minor version */, 2,
          //0x9126 /* masks */,         0x00000004, /*WGL_CONTEXT_ES2_PROFILE_BIT_EXT*/
          0,
        };
        HGLRC new_ctx = wglCreateContextAttribsARB(dc, 0, attribs);
        if (new_ctx) {
          wglDeleteContext(glContext);
          wglMakeCurrent(dc, new_ctx);
          glContext = new_ctx;
        }
      }
    }

    { // Set swap interval
      typedef BOOL(APIENTRY wglSwapInterval_t)(int interval);
      wglSwapInterval_t *wglSwapInterval = (wglSwapInterval_t *)wglGetProcAddress("wglSwapIntervalEXT");
      if (wglSwapInterval) {
        wglSwapInterval(1);
      }
    }
  }
  else {
    assert(0 && "Cannot initialize opengl.");
  }
}

static DWORD app_get_window_ex_style_(bool transparent_background) {
  DWORD ex_style = 0;
  (void)transparent_background;
  return ex_style;
}

static void app_enable_win32_transparent_background_(HWND window) {
  MARGINS margins = {-1};
  DWM_BLURBEHIND blur_behind = {0};

  if (!window) {
    return;
  }

  blur_behind.dwFlags = DWM_BB_ENABLE;
  blur_behind.fEnable = TRUE;
  DwmEnableBlurBehindWindow(window, &blur_behind);
  DwmExtendFrameIntoClientArea(window, &margins);
}

typedef struct App_Win32_Monitor_ {
  RECT monitor_rect;
  RECT work_rect;
  char name[64];
  bool valid;
} App_Win32_Monitor_;

typedef struct App_Win32_Monitor_Enum_ {
  int target_index;
  int current_index;
  App_Win32_Monitor_ result;
} App_Win32_Monitor_Enum_;

static void app_monitor_get_name_(MONITORINFOEXW *info, char *out, int out_size) {
  WideCharToMultiByte(CP_UTF8, 0, info->szDevice, -1, out, out_size, NULL, NULL);
  out[out_size - 1] = 0;
}

static BOOL CALLBACK app_monitor_enum_proc_(HMONITOR monitor, HDC dc, LPRECT rect, LPARAM user) {
  App_Win32_Monitor_Enum_ *ctx = (App_Win32_Monitor_Enum_ *)user;
  MONITORINFOEXW info = {0};
  info.cbSize = sizeof(info);

  if (!GetMonitorInfoW(monitor, (MONITORINFO *)&info))
    return TRUE;

  if (ctx->current_index == ctx->target_index) {
    ctx->result.monitor_rect = info.rcMonitor;
    ctx->result.work_rect = info.rcWork;
    app_monitor_get_name_(&info, ctx->result.name, sizeof(ctx->result.name));
    ctx->result.valid = true;
    return FALSE;
  }

  ctx->current_index++;
  return TRUE;
}

static App_Win32_Monitor_ app_get_monitor_(int index) {
  App_Win32_Monitor_Enum_ ctx = {0};
  ctx.target_index = index;
  EnumDisplayMonitors(NULL, NULL, app_monitor_enum_proc_, (LPARAM)&ctx);

  if (!ctx.result.valid) {
    POINT p = {0, 0};
    HMONITOR primary = MonitorFromPoint(p, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEXW info = {0};
    info.cbSize = sizeof(info);
    if (GetMonitorInfoW(primary, (MONITORINFO *)&info)) {
      ctx.result.monitor_rect = info.rcMonitor;
      ctx.result.work_rect = info.rcWork;
      app_monitor_get_name_(&info, ctx.result.name, sizeof(ctx.result.name));
      ctx.result.valid = true;
    }
  }

  return ctx.result;
}

bool app_get_monitor(int index, App_Monitor_Info *info) {
  App_Win32_Monitor_Enum_ ctx = {0};
  ctx.target_index = index;
  EnumDisplayMonitors(NULL, NULL, app_monitor_enum_proc_, (LPARAM)&ctx);
  if (!ctx.result.valid)
    return false;

  info->index = index;
  info->width = ctx.result.monitor_rect.right - ctx.result.monitor_rect.left;
  info->height = ctx.result.monitor_rect.bottom - ctx.result.monitor_rect.top;
  memcpy(info->name, ctx.result.name, sizeof(info->name));
  return true;
}

typedef struct App_Win32_Monitor_Name_Search_ {
  const char *target_name;
  int found_index;
  int current_index;
} App_Win32_Monitor_Name_Search_;

static BOOL CALLBACK app_monitor_name_search_proc_(HMONITOR monitor, HDC dc, LPRECT rect, LPARAM user) {
  App_Win32_Monitor_Name_Search_ *ctx = (App_Win32_Monitor_Name_Search_ *)user;
  MONITORINFOEXW info = {0};
  info.cbSize = sizeof(info);

  if (!GetMonitorInfoW(monitor, (MONITORINFO *)&info))
    return TRUE;

  char name[64];
  app_monitor_get_name_(&info, name, sizeof(name));

  if (strstr(name, ctx->target_name)) {
    ctx->found_index = ctx->current_index;
    return FALSE;
  }

  ctx->current_index++;
  return TRUE;
}

static int app_find_monitor_by_name_(const char *name) {
  App_Win32_Monitor_Name_Search_ ctx = {0};
  ctx.target_name = name;
  ctx.found_index = -1;
  EnumDisplayMonitors(NULL, NULL, app_monitor_name_search_proc_, (LPARAM)&ctx);
  return ctx.found_index;
}

enum {
  APP_WIN32_FULLSCREEN_STYLE = WS_POPUP,
  APP_WIN32_WINDOWED_STYLE   = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
};

typedef struct App_Win32_Window_Info_ {
  UINT style;
  INT window_x, window_y;
  INT window_width, window_height;
  INT client_width, client_height;
} App_Win32_Window_Info_;

// Need the old window if we're trying to modify flags on an existing window.
// There are flags set that are already there. If we just use a new flag it will
// stop working.
static App_Win32_Window_Info_ app_get_new_window_info_(HWND window, bool fullscreen, bool allow_resize, int monitor_index) {
  App_Win32_Window_Info_ ret = {0};

  App_Win32_Monitor_ monitor = app_get_monitor_(monitor_index);
  const int screen_width  = monitor.work_rect.right  - monitor.work_rect.left;
  const int screen_height = monitor.work_rect.bottom - monitor.work_rect.top;
  const int screen_x = monitor.work_rect.left;
  const int screen_y = monitor.work_rect.top;

  if (window) {
    ret.style = (UINT)GetWindowLongPtrW(window, GWL_STYLE);
  }

  if (fullscreen) {
    ret.style &= ~APP_WIN32_WINDOWED_STYLE;
    ret.style |= APP_WIN32_FULLSCREEN_STYLE;
    ret.window_width  = screen_width;
    ret.window_height = screen_height;
    ret.client_width  = screen_width;
    ret.client_height = screen_height;
    ret.window_x = screen_x;
    ret.window_y = screen_y;
  }
  else {
    ret.style &= ~APP_WIN32_FULLSCREEN_STYLE;
    ret.style |= APP_WIN32_WINDOWED_STYLE;
    if (allow_resize) {
      ret.style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
    }

    ret.client_width  = app_.window_width;
    ret.client_height = app_.window_height;

    {
      RECT window_rect = {0};
      window_rect.right  = ret.client_width;
      window_rect.bottom = ret.client_height;
      AdjustWindowRect(&window_rect, ret.style, 0);

      ret.window_width  = window_rect.right  - window_rect.left;
      ret.window_height = window_rect.bottom - window_rect.top;

      ret.window_x = screen_x + (screen_width  - ret.window_width ) / 2;
      ret.window_y = screen_y + (screen_height - ret.window_height) / 2;
    }
  }

  return ret;
}


void app_toggle_fullscreen(void) {
  App_Win32_Window_Info_ info = {0};
  if (!app_win32_.window)
    return;

  app_.is_full_screen = !app_.is_full_screen;

  info = app_get_new_window_info_(app_win32_.window, app_.is_full_screen, app_.allow_resize, app_.start_monitor);

  SetWindowLongPtrW(app_win32_.window, GWL_STYLE, info.style);
  SetWindowPos(app_win32_.window, NULL,
      info.window_x,     info.window_y,
      info.window_width, info.window_height, 0);

  app_.width  = info.client_width;
  app_.height = info.client_height;

  gfx_set_size(app_.width, app_.height);
}

float app_get_dpi_scale(void) {
  if (!app_win32_.use_system_dpi) {
    HMODULE lib = LoadLibraryA("shcore");
    if (lib) {
      typedef enum MONITOR_DPI_TYPE {
        MDT_EFFECTIVE_DPI = 0,
        MDT_ANGULAR_DPI = 1,
        MDT_RAW_DPI = 2,
        MDT_DEFAULT
      } MONITOR_DPI_TYPE;
      typedef HRESULT (*GetDpiForMonitor_Proc)(
        /*[in] */ HMONITOR         hmonitor,
        /*[in] */ MONITOR_DPI_TYPE dpiType,
        /*[out]*/ UINT             *dpiX,
        /*[out]*/ UINT             *dpiY
      );
      GetDpiForMonitor_Proc GetDpiForMonitor = (GetDpiForMonitor_Proc)GetProcAddress(lib, "GetDpiForMonitor");

      if (GetDpiForMonitor) {
        HMONITOR monitor = MonitorFromWindow(app_win32_.window, MONITOR_DEFAULTTOPRIMARY);
        UINT dx, dy;
        GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dx, &dy);
        return (float)(dx) / 96.f;
      }
    }
  }

  return 1;
}

void *app_launch_another_instance(const char *args) {
  STARTUPINFOW startup_info = {0};
  PROCESS_INFORMATION proc_info = {0};
  void *ret = NULL;
  WCHAR *args16 = 0;

  WCHAR filename_buf[MAX_PATH] = {0};
  GetModuleFileNameW(NULL, filename_buf, MAX_PATH);

  args16 = app_win32_to_wstring(args);
  if (args && args[0] && !args16)
    return NULL;

  if (CreateProcessW(filename_buf, args16, NULL, NULL,
        TRUE /*bInheritHandles*/,
        0    /*dwCreationFlags*/,
        NULL /*lpEnvironment*/,
        NULL /*lpCurrentDirectory*/,
        &startup_info,
        &proc_info))
  {
    ret = proc_info.hProcess;
  }
  free(args16);
  return ret;
}

typedef struct Win32_Thread_Context {
  void *arg;
  int (*proc)(void *arg);
  HANDLE handle;
  DWORD thread_id;
} Win32_Thread_Context;

DWORD WINAPI win32_thread_proc(_In_ LPVOID param) {
  Win32_Thread_Context *ctx = (Win32_Thread_Context *)param;
  return ctx->proc(ctx->arg);
}

void app_sleep(double seconds) {
  Sleep((DWORD)(seconds * 1000));
}

App_Thread_Handle app_new_thread(int (*proc)(void *arg), void *arg) {
  App_Thread_Handle ret = {0};
  Win32_Thread_Context *ctx = NULL;
  if (!proc)
    return ret;

  ctx = (Win32_Thread_Context *)calloc(1, sizeof(Win32_Thread_Context));
  if (!ctx)
    return ret;

  ctx->arg = arg;
  ctx->proc = proc;
  ctx->handle = CreateThread(NULL, 1024 * 64 /*64 KB*/, win32_thread_proc, ctx, 0, &ctx->thread_id);
  if (!ctx->handle) {
    free(ctx);
    return ret;
  }

  ret.handle = ctx;
  return ret;
}

static const WCHAR *win32_get_executable_dir(void) {
  if (app_win32_.executable_dir_w)
    return app_win32_.executable_dir_w;
  int capacity = 0;
  WCHAR *buffer = NULL;
  DWORD ret = 0;
  DWORD i = 0;

  do {
    WCHAR *new_buffer = NULL;
    capacity = capacity == 0 ? 256 : capacity * 2;
    new_buffer = (WCHAR *)realloc(buffer, sizeof(WCHAR) * capacity);
    if (!new_buffer) {
      free(buffer);
      return NULL;
    }
    buffer = new_buffer;
    ret = GetModuleFileNameW(NULL, buffer, capacity);
  } while (ret == ERROR_INSUFFICIENT_BUFFER || !ret);

  for (i = ret; i > 0; i--) {
    if (buffer[i - 1] == '\\' || buffer[i - 1] == '/') {
      buffer[i - 1] = 0;
      break;
    }
  }

  app_win32_.executable_dir_w = buffer;
  return buffer;
}

const char *app_win32_get_executable_dir(void) {
  if (!app_win32_.executable_dir) {
    app_win32_.executable_dir = app_win32_from_wstring(win32_get_executable_dir());
  }
  return app_win32_.executable_dir;
}

void app_setup_p(App_Config *conf) {
  App_Config def = {0};
  if (!conf)
    conf = &def;
  app_setup(*conf);
}

void app_set_cursor(App_Cursor cursor) {
  if (!app_.is_ready)
    return;

  app_win32_.next_cursor = cursor;
}

void app_setup(App_Config conf) {
  if (app_.is_ready)
    return;
  if (conf.start_monitor_name) {
    int idx = app_find_monitor_by_name_(conf.start_monitor_name);
    if (idx >= 0) {
      conf.start_monitor = idx;
      printf("Using monitor: %s (index %d)\n", conf.start_monitor_name, idx);
    } else {
      printf("Monitor \"%s\" not found, falling back to index %d\n", conf.start_monitor_name, conf.start_monitor);
    }
  }

  app_.start_monitor = conf.start_monitor;
  app_.on_drop_file = conf.on_drop_file;

  HMODULE hInstance = 0;
  const WCHAR *window_class_name = L"app-main-window";
  WNDCLASSW window_class = {0};

  const int screen_width = GetSystemMetrics(SM_CXSCREEN);
  const int screen_height = GetSystemMetrics(SM_CYSCREEN);

  app_setup_default_conf_(&conf);

  app_win32_.use_system_dpi = conf.use_system_dpi;
  app_win32_.transparent_background = conf.transparent_background;
  if (!conf.use_system_dpi) {
    SetProcessDPIAware(); // Don't scale with the window scaling
  }

  app_.allow_resize   = conf.allow_resize;
  app_.is_full_screen = conf.full_screen;
  app_.window_width    = conf.width;
  app_.window_height   = conf.height;
  app_.ctrl_w_to_quit  = !conf.disable_ctrl_w_to_quit;

  {
    LARGE_INTEGER win32_frequency;
    QueryPerformanceFrequency(&win32_frequency);
    app_win32_.frequency = (double)win32_frequency.QuadPart;
  }

  if (!conf.use_default_cwd) {
    const WCHAR *path = win32_get_executable_dir();
    if (path) {
      SetCurrentDirectoryW(path);
    }
  }

  hInstance = GetModuleHandle(0);

  {
    HCURSOR normal_cursor = LoadCursor(NULL, (LPTSTR)IDC_ARROW);
    for (int i = 0; i < APP_CURSOR_COUNT_; i++) {
      app_win32_.cursors[i] = normal_cursor;
    }
    app_win32_.cursors[APP_CURSOR_TEXT_SELECT] = LoadCursor(NULL, (LPTSTR)IDC_IBEAM);
    app_win32_.cursors[APP_CURSOR_HAND]        = LoadCursor(NULL, (LPTSTR)IDC_HAND);
    app_win32_.cursors[APP_CURSOR_WAIT]        = LoadCursor(NULL, (LPTSTR)IDC_WAIT);
  }

  {
    App_Win32_Window_Info_ info = {0};
    WNDCLASSW window_class = {0};
    HWND window = NULL;
    window_class.style         = /*CS_DBLCLKS | */CS_PARENTDC;
    window_class.lpfnWndProc   = &win32_window_proc;
    window_class.cbClsExtra    = 0;
    window_class.cbWndExtra    = 0;
    window_class.hInstance     = hInstance;
    window_class.hIcon         = NULL;
    window_class.hCursor       = NULL;
    window_class.hbrBackground = NULL;
    window_class.lpszMenuName  = NULL;
    window_class.lpszClassName = window_class_name;

    RegisterClassW(&window_class);

    info = app_get_new_window_info_(NULL, app_.is_full_screen, app_.allow_resize, app_.start_monitor);
    app_.width  = info.client_width;
    app_.height = info.client_height;

    {
      WCHAR *title_w = app_win32_to_wstring(conf.title);
      window = CreateWindowExW(
          app_get_window_ex_style_(conf.transparent_background),
          window_class_name, title_w, info.style,
          info.window_x, info.window_y, info.window_width, info.window_height,
          0,0,hInstance,0);
      free(title_w);
    }

    DragAcceptFiles(window, TRUE);
    SetCapture(window);
    SetCursor(app_win32_.cursors[APP_CURSOR_NORMAL]);
    app_win32_.window = window;

    if (conf.transparent_background) {
      app_enable_win32_transparent_background_(window);
    }

    {
      HDC dc = GetDC(window);
      app_win32_.dc = dc;
      init_gl(dc);
    }
  }

  {
    WCHAR path_local_appdata[MAX_PATH] = {0};
    if (S_OK == SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, path_local_appdata)) {
      wprintf(L"%s\n", path_local_appdata);
    }
  }

  gfx_setup(app_.width, app_.height, &app_load_gl_function);
  app_.is_ready = true;
}

double app_time(void) {
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (double)counter.QuadPart / app_win32_.frequency;
}

static bool win32_show_cursor = 1;
void app_show_cursor(bool show) {
  if (win32_show_cursor && !show) {
    ShowCursor(0);
  }
  else if (!win32_show_cursor && show) {
    ShowCursor(1);
  }
  win32_show_cursor = show;
}

void app_run(void (*proc)(void)) {
  ShowWindow(app_win32_.window, SW_SHOW);

  app_.last_frame_time = app_time();
  while (!app_.quit) {
    app_win32_.had_focus = app_win32_.has_focus;
    app_win32_.has_focus = 
      GetFocus() == app_win32_.window &&
      GetForegroundWindow() == app_win32_.window;

    reset_input();

    update_timing();
    app_update_delta_accum();

    /*
     * NOTE(hby): window handle passed to PeekMessageW must be NULL! There are
     * messages sent to windows that are not the single window that you
     * created, for example, any IME windows that the OS creates on your
     * behalf. If you don't handle those messages and pass them along, your
     * event queue can be clogged and your program will become unresponsive.
     *
     * Thanks, Raymond Chen for your blog post explaining this:
     *       https://devblogs.microsoft.com/oldnewthing/20050209-00/?p=36493
     *
     */
    {
      MSG msg = { 0 };
      while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
      }
    }

    if (!app_win32_.has_focus) {
      memset(app_.keys_down, 0, sizeof(app_.keys_down));
    }

    if (app_win32_.has_focus) {
      if (app_.mouse_pos_requested) {
        POINT mouse_point;
        GetCursorPos(&mouse_point);
        ScreenToClient(app_win32_.window, &mouse_point);

        app_.mouse_pos_requested = 0;
        {
          POINT set_mouse_point = {0};
          set_mouse_point.x = app_.mouse_pos_requested_x;
          set_mouse_point.y = app_.mouse_pos_requested_y;
          ClientToScreen(app_win32_.window, &set_mouse_point);
          SetCursorPos(set_mouse_point.x, set_mouse_point.y);
        }

        app_.mouse_delta_x = (float)(mouse_point.x - app_.mouse_pos_requested_x);
        app_.mouse_delta_y = (float)(mouse_point.y - app_.mouse_pos_requested_y);
        app_.mouse_x = app_.mouse_pos_requested_x;
        app_.mouse_y = app_.mouse_pos_requested_y;
      }
      else {
        POINT mouse_point;
        GetCursorPos(&mouse_point);
        ScreenToClient(app_win32_.window, &mouse_point);

        app_.mouse_delta_x = (float)(mouse_point.x - app_.mouse_x);
        app_.mouse_delta_y = (float)(mouse_point.y - app_.mouse_y);
        app_.mouse_x = mouse_point.x;
        app_.mouse_y = mouse_point.y;
      }
    }

    if (app_.ctrl_w_to_quit) {
      if (app_keydown(KEY_LCTRL) && app_keypress(KEY_W)) {
        app_quit();
      }
    }

    if (proc)
      proc();

    if (app_win32_.cur_cursor != app_win32_.next_cursor) {
      SetCursor(app_win32_.cursors[app_win32_.next_cursor]);
      app_win32_.cur_cursor = app_win32_.next_cursor;
    }
    SwapBuffers(app_win32_.dc);
  }
}

float app_get_mouse_delta_x(void) {
  return app_.mouse_delta_x;
}
float app_get_mouse_delta_y(void) {
  return app_.mouse_delta_y;
}

bool app_has_focus(void) {
  return app_win32_.has_focus &&
    app_win32_.had_focus;
}

void app_set_mouse_pos(int x, int y) {
  app_.mouse_pos_requested_x = x;
  app_.mouse_pos_requested_y = y;
  app_.mouse_pos_requested = 1;
}

void *app_load_gl_function(const char *name) {
  void *ptr = wglGetProcAddress(name);
  if (!ptr) {
    HMODULE lib = LoadLibraryW(L"opengl32");
    if (lib) {
      ptr = GetProcAddress(lib, name);
    }
  }
  return ptr;
}

/* } */

#elif defined(__EMSCRIPTEN__)


void app_set_mouse_pos(int x, int y) {}
void app_show_cursor(bool show) {}
float app_get_mouse_delta_x(void) { return 0; }
float app_get_mouse_delta_y(void) { return 0; }

/* { */

#include <emscripten/html5.h>
#include <emscripten/emscripten.h>

void *app_load_gl_function(const char *name) {
  return NULL;
}

double app_time(void) {
  return emscripten_get_now() / 1000.0;
}

static void (*app_emscripten_frame)(void);

static void em_main_loop(void) {
  update_timing();
  app_update_delta_accum();
  app_update_delta_accum();

  if (app_emscripten_frame)
    app_emscripten_frame();

  reset_input();
}

static int app_em_translate_key_(int key) {
  if (key >= 65 && key <= 65+'z'-'a') {
    return ((key-65) + KEY_A);
  }
  if (key >= 48 && key <= 48+'0'-'9') {
    return ((key-48) + KEY_0);
  }
  switch (key) {
    case 37: return KEY_LEFT;
    case 39: return KEY_RIGHT;
    case 38: return KEY_UP;
    case 40: return KEY_DOWN;
    case 16: return KEY_LSHIFT;
    //case 32: return KEY_SPACE;
    //case 189: return KEY_MINUS;
    //case 187: return KEY_EQUAL;
  }
  return 0;
}

EM_BOOL app_em_key_down_(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
  int k = app_em_translate_key_(keyEvent->keyCode);
  if (k) {
    app_on_keydown_(k, 0 != keyEvent->repeat);
  }
  return 1;
}
EM_BOOL app_em_key_up_(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData) {
  int k = app_em_translate_key_(keyEvent->keyCode);
  if (k) {
    app_on_keyup_(k);
  }
  return 1;
}

void app_setup(App_Config conf) {
  app_setup_default_conf_(&conf);

  app_.width  = conf.width;
  app_.height = conf.height;
  app_.allow_resize = conf.allow_resize;

  const char *id = "#canvas";

  emscripten_set_canvas_element_size(id, app_.width, app_.height);

  {
    EmscriptenWebGLContextAttributes attr;
    attr.alpha = EM_TRUE;
    attr.depth = EM_TRUE;
    attr.stencil = EM_FALSE;
    attr.antialias = EM_TRUE;
    attr.premultipliedAlpha = EM_TRUE;
    attr.preserveDrawingBuffer = EM_TRUE;
    //attr.preferLowPowerToHighPerformance = EM_TRUE;
    attr.failIfMajorPerformanceCaveat = EM_FALSE;
    attr.majorVersion = 2;
    attr.minorVersion = 0;
    attr.enableExtensionsByDefault = EM_FALSE;
    int ctx = emscripten_webgl_create_context(id, &attr);
    emscripten_webgl_make_context_current(ctx);
    //emscripten_set_resize_callback(0, 0, EM_TRUE, &hb_on_resize_);
  }

  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, &app_em_key_down_);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, NULL, 1, &app_em_key_up_);

  gfx_setup(app_.width, app_.height, &app_load_gl_function);
}

void app_run(void (*proc)(void)) {
  app_emscripten_frame = proc;
  emscripten_set_main_loop(em_main_loop, 0, 1);
}

void app_setup_p(App_Config *conf) {
  App_Config def = {0};
  if (!conf)
    conf = &def;
  app_setup(*conf);
}

void app_set_cursor(App_Cursor cursor) {}

/* } */
#endif


#ifdef __cplusplus
} // extern "C"
#endif
