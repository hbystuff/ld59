#pragma once

#include "basic.h"
#include <stdbool.h>

enum {
  UI_APP_BAR_H = 21,
};

#define UI_ENDIAN_SWAP_UINT32(val) (  \
    (((val) & 0xff000000) >> 24) | \
    (((val) & 0x00ff0000) >> 8 ) | \
    (((val) & 0x0000ff00) << 8 ) | \
    (((val) & 0x000000ff) << 24)   \
  )
#define UI_COLOR3(color) UI_ENDIAN_SWAP_UINT32(((color) << 8) | 0xff)

#define UI_COLOR_BLUE__STR   "79847d"
#define UI_COLOR_YELLOW__STR "79847d"
#define UI_COLOR_ERROR__STR  "79847d"
#define UI_COLOR_FG_HL__STR  "79847d"

enum {
  UI_COLOR_FG_HL = UI_COLOR3(0x79847d),
  UI_COLOR_BG = UI_COLOR3(0x0e0e0e),
  UI_COLOR_FG_MUTED = UI_COLOR3(0x79847d),
  //UI_COLOR_FG = UI_COLOR3(0xc0c0c0),/*UI_COLOR3(0x696969),*/
  UI_COLOR_FG = UI_COLOR_FG_MUTED,
  UI_COLOR_DISABLED = UI_COLOR3(0x29322c),
  UI_COLOR_ERROR = UI_COLOR3(0x79847d),
  UI_COLOR_ALERT = UI_COLOR3(0x79847d),
  UI_COLOR_ALMOST_BG = UI_COLOR3(0x101512),

  UI_COLOR_BLUE = UI_COLOR3(0x353a90),
  UI_COLOR_YELLOW = UI_COLOR3(0x79847d),

  //UI_COLOR_FG_HL = UI_COLOR3(0xb2b2b2),
  UI_COLOR_FG_ACTIVE = UI_COLOR3(0xcbdc2d),

  UI_COLOR_FG_FOCUS    = UI_COLOR3(0xb4b4b4),
  UI_COLOR_FG_FOCUS_HL = UI_COLOR3(0xffffff),
};

typedef uint32_t Ui_Window_Id;
typedef uint32_t Ui_Widget_Id;
typedef uint32_t Ui_Panel_Id;
typedef u64 Ui_Priority;

enum {
  UI_BOX_SIDE_TOP    = 0x1,
  UI_BOX_SIDE_BOTTOM = 0x2,
  UI_BOX_SIDE_LEFT   = 0x4,
  UI_BOX_SIDE_RIGHT  = 0x8,
  UI_BOX_SIDE_ALL =
    UI_BOX_SIDE_TOP | UI_BOX_SIDE_BOTTOM |
    UI_BOX_SIDE_LEFT | UI_BOX_SIDE_RIGHT,
};

const static Ui_Widget_Id UI_TRANSIENT_ID = -1;

typedef struct {
  float x,y,w,h;
} Ui_Box;

typedef struct {
  Ui_Box box;
} Ui_Layout;

typedef struct {
  Ui_Box box;
  bool enabled;
} Ui_Scissor;

typedef struct {
  bool hide_close_button;
  bool out_close_button_clicked;
  bool out_minimize_button_clicked;
  Ui_Box minimize_box;
  bool minimize;
  struct Font *title_font;
  bool no_title;
  bool force_box;
  bool is_dropdown_; // internal use
  bool close_button_disabled;
} Ui_Window_Options;

typedef struct {
  Ui_Window_Id window_id;
  Ui_Window_Id parent_window_id;
  Ui_Priority parent_window_priority;
  uint64_t priority;
  Ui_Box box;
  Ui_Box parent_box;
  float minimize_t;
  int cmd_begin;
  float parent_scroll_y;
  Ui_Box parent_content_box;
  Ui_Scissor parent_scissor;
  bool parent_minimized;
} Ui_Window;

typedef struct {
  Ui_Panel_Id panel_id;
  Ui_Panel_Id parent_panel_id;
  bool parent_panel_disabled;

  float scroll_x;
  float scroll_y;
  Ui_Box parent_box;
  Ui_Box content_box_local_space;
  Ui_Scissor scissor;
  float parent_scroll_y;
  Ui_Widget_Id scroll_handle_id;

  bool has_requested_scroll_y;
  float requested_scroll_y;
  float scroll_mouse_offset_y;

  // Sometimes we want the content of the panel to load in
  // slowly.
  bool has_loading_t;
  float loading_t;
  Ui_Box last_loaded_box; // This is computed at ui_panel_end, since at
                          // ui_panel_begin we don't know exactly how big the
                          // content is going to be.

  // Keep track of the parent panel's information.
  Ui_Box parent_loaded_box;
  bool parent_has_panel_loaded_box;
} Ui_Panel;

typedef struct {
  float x,y;
  float u,v;
  uint32_t color;
} Ui_Vertex;

typedef struct {
  Ui_Widget_Id id;
  Ui_Panel panel;
} Ui_Dropdown_Simple;

typedef struct {
  int value;
  const char *display;
} Ui_Dropdown_Item;

typedef struct {
  bool hovered;
  bool clicked;
  bool active;
  bool became_active;
  bool disabled;
  bool focused;
} Ui_Widget;

typedef struct {
  char *buffer;
  int buffer_size;
  const char *correct_string;
  Font_Text_Effect *text_effects;
  int num_text_effects;
} Ui_Input_Options;

typedef enum {
  UI_CURSOR_NORMAL,
  UI_CURSOR_WAIT,
  UI_CURSOR_HAND,
  UI_CURSOR_TEXT_EDIT,
} Ui_Cursor;

void ui_set_cursor(Ui_Cursor cursor);
Ui_Box ui_box(float x, float y, float w, float h);
Ui_Box ui_box_shrink(Ui_Box box, float padding);
Ui_Box ui_box_pad(Ui_Box box, float padding);
Ui_Box ui_box_border_left(Ui_Box box);
Ui_Box ui_box_border_top(Ui_Box box);
Ui_Box ui_box_border_bottom(Ui_Box box);
Ui_Box ui_box_border_right(Ui_Box box);
Ui_Box ui_box_set_height(Ui_Box box, float height);
Ui_Box ui_box_expand(Ui_Box a, Ui_Box b);
Ui_Box ui_box_lerp(Ui_Box a, Ui_Box b, float t);
Ui_Box ui_box_scissor(Ui_Box box, Ui_Box scissor_box);

Ui_Box ui_get_current_client_box(void);

//==============================================
// Call ui_begin before submitting commands
// Call ui_end, and then ui_draw.

void ui_begin(float width, float height, float mouse_x, float mouse_y, float dt);

void ui_disable_all_input(void);
void ui_set_font(Font *font);
Font *ui_get_font(void);
void ui_set_next_img_color(uint32_t color);
void ui_end(void);
void ui_draw(void);
void ui_set_next_panel_loading_t(float t);
void ui_panel_begin(Ui_Panel *panel, Ui_Box box);
void ui_panel_disable(void);
void ui_panel_end(Ui_Panel *panel);
void ui_panel_scroll_to_top(Ui_Panel *panel);
uint32_t ui_color(float r, float g, float b, float a);
float ui_get_text_width(const char *text);
float ui_get_text_width_sv(String_View sv);
float ui_get_text_height(void);
bool ui_dotted_input_field(Ui_Widget_Id *id, const char *label, Ui_Box box, Ui_Input_Options options);

float ui_get_mouse_x(void);
float ui_get_mouse_y(void);

void ui_scope(Ui_Box box);

bool ui_hover(Ui_Box box);
bool ui_is_current_hovered_window(void);
Ui_Widget ui_widget(Ui_Box box);
Ui_Widget ui_widget_with_id(Ui_Box box, Ui_Widget_Id id);
Ui_Widget ui_widget_with_id_ptr(Ui_Box box, Ui_Widget_Id *id);
void ui_progress_bar(float t, Ui_Box box);
bool ui_just_became_active(void);
bool ui_is_active(void);
bool ui_gained_focus(void);
Ui_Box ui_get_last_box(void);
void ui_expand_content_box(Ui_Box box);
void ui_disable_input_for_non_window_widgets_above_app_bar(void);

void ui_add_command(uint16_t op, const void *ptr, int size);
bool ui_window(Ui_Window *window, const char *title, Ui_Box box, Ui_Window_Options *options);
Ui_Box ui_get_window_client_box(Ui_Window *window);
bool ui_selectable(const char *text, bool selected, Ui_Box box);
void ui_window_end(Ui_Window *window);
bool ui_window_is_front(Ui_Window *window);
void ui_window_bring_to_front(Ui_Window *window);
void ui_set_color(float r, float g, float b, float a);
void ui_set_next_color(uint32_t color);
void ui_set_next_disabled(bool disabled);
void ui_set_next_cannot_hover(void);
void ui_color_text(uint32_t color, const char *text, float x, float y);
bool ui_dropdown(Ui_Widget_Id *id, const char *display, Ui_Box box);
void ui_close_dropdown(Ui_Widget_Id id);
void ui_dropdown_end();
bool ui_dotted_button(uint32_t fg_color, Ui_Widget_Id *id, const char *label, Ui_Box box);
void ui_text(const char *text, float x, float y);

// Returns true if value changed
bool ui_dropdown_simple(Ui_Dropdown_Simple *dropdown, int *selected_value, const Ui_Dropdown_Item *items, int num_items, Ui_Box box);

void ui_redact(Ui_Box box);

void ui_overlay_begin(void);
void ui_overlay_end(void);

void ui_transform_box(Ui_Box *box); /*TODO: A bit of a better description/name for this. */
void ui_transform_pos_to_absolute(float *x, float *y);

static Ui_Box ui_transform_box_value(Ui_Box box) {
  ui_transform_box(&box);
  return box;
}

typedef struct Ui_Text_Ex {
  const Font_Text_Effect *text_effects;
  int num_text_effects;
  bool centered;
  bool fill_bg;
} Ui_Text_Ex;

static bool ui_window_is_minimized(Ui_Window *window) {
  return window->minimize_t > 0;
}

typedef enum {
  UI_BOX_FILL,
  UI_BOX_CHECKERBOARD,
  UI_BOX_LINE,
} Ui_Box_Type;

void ui_cmd_box_type(Ui_Box_Type type, uint32_t color, Ui_Box box);
void ui_cmd_box(uint32_t color, Ui_Box box);
void ui_cmd_img(Gfx_Texture *texture, float x, float y);
void ui_cmd_img_box(Gfx_Texture *texture, Ui_Box box);
void ui_cmd_img_box_q(Gfx_Texture *texture, Ui_Box box, float quad[4]);
void ui_cmd_color_img(uint32_t color, Gfx_Texture *texture, float x, float y);
void ui_cmd_color_img_box_q(uint32_t color, Gfx_Texture *texture, Ui_Box box, float quad[4]);
void ui_cmd_text(const char *text, float x, float y);
void ui_cmd_text_ex(uint32_t color, const char *text, float x, float y, Ui_Text_Ex ex);
void ui_cmd_color_text(uint32_t color, const char *text, float x, float y);
void ui_cmd_color_text_part_ex(uint32_t color, const char *text, int text_len, float x, float y, Ui_Text_Ex ex);
void ui_cmd_color_text_part(uint32_t color, const char *text, int text_len, float x, float y);
void ui_cmd_vertices(const Ui_Vertex *vertices, int count);
void ui_cmd_checkerboard_box(uint32_t color, Ui_Box box);
void ui_cmd_scissor_begin(Ui_Scissor *scissor, Ui_Box box);
void ui_cmd_scissor_end(Ui_Scissor *scissor);
void ui_cmd_dotted_line_box(uint32_t color, Ui_Box box, uint32_t sides);
void ui_cmd_line_box(uint32_t color, Ui_Box box, uint32_t sides);
void ui_cmd_line(uint32_t color, float x0, float y0, float x1, float y1);

typedef void Ui_Cmd_Box_Callback_Type(Ui_Box box, void *arg);
void ui_cmd_box_callback(Ui_Box box, Ui_Cmd_Box_Callback_Type *callback, void *arg);

