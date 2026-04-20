#include "ui.h"
#include "gfx.h"

#include <assert.h>

#define UI_CLEAR_ZERO(x) memset(&(x), 0, sizeof(x))

enum {
  UI_CMD_NOP,
  UI_CMD_BOX,
  UI_CMD_BOX_BG,
  UI_CMD_BOX_REDACT,
  UI_CMD_TEXT,
  UI_CMD_FONT,
  UI_CMD_LINE,
  UI_CMD_SET_COLOR,
  UI_CMD_IMG_BOX,
  UI_CMD_VERTICES,
  UI_CMD_SCISSOR,
  UI_CMD_UNSET_SCISSOR,
  UI_CMD_SCOPE,
  UI_CMD_BOX_CALLBACK,
};

typedef struct {
  Ui_Box box;
  void *arg;
  Ui_Cmd_Box_Callback_Type *callback;
} Ui_Cmd_Box_Callback;

typedef struct {
  uint32_t color;
} Ui_Cmd_Set_Color;

typedef struct  {
  Ui_Box box;
  Ui_Window_Id window_id;
} Ui_Cmd_Scope;

typedef struct {
  uint32_t color;
  float x0, y0, x1, y1;
} Ui_Cmd_Line;

typedef struct {
  String_Bank_Id str_id;
  float x, y;
  uint32_t color;
  uint16_t text_effect_index, num_text_effects;
} Ui_Cmd_Text;

typedef struct {
  Font *font;
} Ui_Cmd_Font;

typedef struct {
  float x,y,width,height;
  uint32_t color;
} Ui_Cmd_Box;

typedef struct {
  float x,y,width,height;
  float qx, qy, qw, qh;
  Gfx_Texture *texture;
  uint32_t color;
} Ui_Cmd_Img_Box;

typedef struct {
  Ui_Box box_in_screen_space;
  Ui_Window_Id window_id;
  Ui_Priority priority;
} Ui_Redaction;

typedef struct {
  uint32_t num_vertices;
} Ui_Cmd_Vertices;

typedef struct {
  Ui_Box box;
} Ui_Cmd_Scissor;

typedef struct {
  Ui_Window_Id window_id;
  Ui_Priority priority;
  int begin, num_bytes;
} Ui_Command_List;

typedef struct {
  Ui_Box box;
} Ui_Debug_Shape;

typedef struct {
  Ui_Box box;
  Ui_Window_Id window_id;
  Ui_Priority priority;
} Ui_Scope_Box;

//=========================================================
static struct {
  String_Builder bank;
  String_Builder cmd_buffer;
  String_Builder orphan_cmd_buffer;
  String_Builder overlay_cmd_buffer;
  String_Builder window_cmd_buffer;
  Font_Text_Effect *text_effects;
  Ui_Command_List *command_lists;
  Ui_Debug_Shape *debug_shapes;

  Ui_Widget_Id max_widget_id;
  Ui_Widget_Id active_widget_id;
  Ui_Widget_Id focused_widget_id;
  Ui_Window_Id hovered_widget_id;
  Ui_Window_Id last_hovered_widget_id;

  Font *cur_font;
  Ui_Window_Id max_window_id;
  Ui_Priority max_priority;
  Ui_Panel_Id max_panel_id;
  Ui_Window_Id cur_window_id;
  Ui_Priority cur_window_priority;
  Ui_Panel_Id cur_panel_id;
  bool cur_panel_disabled;
  bool started;
  float width, height, dt;

  Ui_Box cur_box_screen_space;
  Ui_Box content_box_local_space;
  float scroll_y;

  float mouse_x, mouse_y;
  float last_mouse_x, last_mouse_y;

  Ui_Window_Id drag_window_id;
  float drag_window_mouse_offset_x;
  float drag_window_mouse_offset_y;

  Font *default_font;

  Ui_Priority  next_hovered_window_priority;
  Ui_Window_Id next_hovered_window_id;
  Ui_Window_Id hovered_window_id;

  uint32_t next_color;
  bool has_next_color : 1;
  bool next_cannot_hover : 1;
  bool next_disabled  : 1;
  bool disable_input  : 1;
  bool submitting_overlay  : 1;
  bool cur_window_minimized;
  float input_cursor_flashing_t;

  Texture *checker_board_texture;
  Texture *scroll_bar_arrow_up_texture;
  Texture *scroll_bar_arrow_down_texture;

  Ui_Scissor scissor;
  Ui_Widget last_widget;
  Ui_Box last_widget_box;

  Ui_Window dropdown_window;
  Ui_Widget_Id opened_dropdown_id;
  bool dropdown_just_opened;
  Ui_Box dropdown_content_box;

  bool disable_input_for_non_window_widgets_above_app_bar;
  bool disable_input_for_non_window_widgets_above_app_bar_next;

  Ui_Priority max_priority_seen_last_frame;
  Ui_Priority max_priority_seen_this_frame;

  bool has_next_panel_loading_t;
  float next_panel_loading_t;

  Ui_Box panel_loaded_box;
  bool has_panel_loaded_box;

  Ui_Redaction *redactions;
  bool is_hovering_redaction;
  Ui_Priority max_redaction_priority;

  Ui_Scope_Box *scope_boxes;
  bool next_hovering_scope;
  bool hovering_scope;

  Ui_Cursor cursor;
} ui_;

void ui_set_cursor(Ui_Cursor cursor) {
  if (cursor == UI_CURSOR_HAND || cursor ==UI_CURSOR_TEXT_EDIT) {
    ui_.cursor = cursor;
  }
  else if (cursor == UI_CURSOR_WAIT) {
    if (ui_.cursor == UI_CURSOR_NORMAL)
      ui_.cursor = cursor;
  }
}

void ui_set_next_panel_loading_t(float t) {
  if (t == 1) {
    ui_.has_next_panel_loading_t = false;
    return;
  }
  ui_.has_next_panel_loading_t = true;
  ui_.next_panel_loading_t = t;
}

static Ui_Widget_Id ui_allocate_id(void) {
  return ++ui_.max_widget_id;
}

void ui_disable_all_input(void) {
  ui_.disable_input = 1;
}

static void ui_debug_box(Ui_Box box) {
  Ui_Debug_Shape shape = {0};
  shape.box = box;
  arr_add(ui_.debug_shapes, shape);
}

static bool ui_hover_raw(Ui_Box box) {
  return ui_.last_mouse_x >= box.x && ui_.last_mouse_x <= box.x+box.w &&
         ui_.last_mouse_y >= box.y && ui_.last_mouse_y <= box.y+box.h;
}

Ui_Box ui_get_last_box(void) {
  return ui_.last_widget_box;
}

void ui_begin(float width, float height, float mouse_x, float mouse_y, float dt) {
  if (!ui_.default_font) {
    ui_.default_font = gfx_new_default_font();
  }
  assert(!ui_.started && "Called ui_begin when ui is already being built.");
  ui_.started = true;
  ui_.last_mouse_x = ui_.mouse_x;
  ui_.last_mouse_y = ui_.mouse_y;
  ui_.width = width;
  ui_.height = height;
  ui_.dt = dt;
  ui_.mouse_x = mouse_x;
  ui_.mouse_y = mouse_y;
  ui_.cur_box_screen_space = ui_box(0,0, ui_.width, ui_.height);
  UI_CLEAR_ZERO(ui_.last_widget);

  ui_.hovering_scope = ui_.next_hovering_scope;
  ui_.next_hovering_scope = false;

  // Window hover
  ui_.hovered_window_id = ui_.next_hovered_window_id;
  ui_.next_hovered_window_id = 0;
  ui_.next_hovered_window_priority = 0;

  arr_clear(ui_.scope_boxes);
  sb_clear(&ui_.bank);
  sb_clear(&ui_.cmd_buffer);
  arr_clear(ui_.command_lists);
  arr_clear(ui_.text_effects);

  ui_.cur_font = ui_.default_font;
  if (!ui_.default_font) {
    ui_.default_font = gfx_new_default_font();
  }

  ui_.max_priority_seen_this_frame = 0;

  ui_.is_hovering_redaction = false;
  ui_.max_redaction_priority = 0;
  if (!ui_.hovering_scope) {
    for (int i = 0; i < arr_len(ui_.redactions); i++) {
      Ui_Redaction redaction = ui_.redactions[i];
      if (redaction.priority >= ui_.max_redaction_priority && ui_hover_raw(redaction.box_in_screen_space)) {
        ui_.max_redaction_priority = redaction.priority;
        ui_.is_hovering_redaction = true;
      }
    }
  }
  arr_clear(ui_.redactions);
}

Ui_Box ui_get_current_client_box(void) {
  return ui_.cur_box_screen_space;
}

static int ui_cmp_window_command_lists(const void *va, const void *vb) {
  const Ui_Command_List *a = (const Ui_Command_List *)va;
  const Ui_Command_List *b = (const Ui_Command_List *)vb;

  if (a->priority < b->priority) {
    return -1;
  } else if (a->priority > b->priority) {
    return 1;
  }
  return 0;
}

void ui_end(void) {
  ui_.max_priority_seen_last_frame = ui_.max_priority_seen_this_frame;

  ui_.disable_input_for_non_window_widgets_above_app_bar = ui_.disable_input_for_non_window_widgets_above_app_bar_next;
  ui_.disable_input_for_non_window_widgets_above_app_bar_next = false;

  assert(ui_.started && "Called ui_end when ui is not started.");
  ui_.next_disabled = 0;
  ui_.input_cursor_flashing_t += ui_.dt;
  if (ui_.input_cursor_flashing_t > 1)
    ui_.input_cursor_flashing_t = 0;
  ui_.started = false;
  ui_.disable_input = false;

  if (ui_.orphan_cmd_buffer.size) {
    Ui_Command_List orphaned_list = {0};
    orphaned_list.begin = ui_.cmd_buffer.size;
    orphaned_list.num_bytes = ui_.orphan_cmd_buffer.size;
    sb_write(&ui_.cmd_buffer, ui_.orphan_cmd_buffer.ptr, ui_.orphan_cmd_buffer.size);
    sb_clear(&ui_.orphan_cmd_buffer);
    arr_add(ui_.command_lists, orphaned_list);
  }
  arr_sort(ui_.command_lists, &ui_cmp_window_command_lists);

  ui_.submitting_overlay = false;
  if (ui_.overlay_cmd_buffer.size) {
    Ui_Command_List overlay_list = {0};
    overlay_list.begin = ui_.cmd_buffer.size;
    overlay_list.num_bytes = ui_.overlay_cmd_buffer.size;
    sb_write(&ui_.cmd_buffer, ui_.overlay_cmd_buffer.ptr, ui_.overlay_cmd_buffer.size);
    sb_clear(&ui_.overlay_cmd_buffer);
    arr_add(ui_.command_lists, overlay_list);
  }

  if (!app_keydown(KEY_MOUSE_LEFT) ||
      ui_.active_widget_id == UI_TRANSIENT_ID)
  {
    ui_.active_widget_id = 0;
  }

  if (app_keyrelease(KEY_MOUSE_LEFT) && (!ui_.hovered_widget_id ||
      ui_.hovered_widget_id != ui_.focused_widget_id))
  {
    ui_.focused_widget_id = 0;
  }

  if (ui_.opened_dropdown_id && !ui_.dropdown_just_opened) {
    if (app_keyrelease(KEY_MOUSE_LEFT)) {
      if (ui_.hovered_window_id != ui_.dropdown_window.window_id) {
        ui_.opened_dropdown_id = 0;
      }
    }
  }
  ui_.dropdown_just_opened = false;

  ui_.last_hovered_widget_id = ui_.hovered_widget_id;
  ui_.hovered_widget_id = 0;

  assert(!ui_.cur_window_id && "ui_.cur_window_id is not null at ui_end. Did you forget to call ui_*_end?");

  switch (ui_.cursor) {
  case UI_CURSOR_WAIT:
    app_set_cursor(APP_CURSOR_WAIT);
    break;
  case UI_CURSOR_TEXT_EDIT:
    app_set_cursor(APP_CURSOR_TEXT_SELECT);
    break;
  case UI_CURSOR_HAND:
    app_set_cursor(APP_CURSOR_HAND);
    break;
  default:
    app_set_cursor(APP_CURSOR_NORMAL);
    break;
  }
  ui_.cursor = UI_CURSOR_NORMAL;
}

void ui_add_command(uint16_t op, const void *ptr, int size) {
  if (ui_.cur_window_minimized) {
    return;
  }

  String_Builder *cmd_buffer = &ui_.window_cmd_buffer;
  if (!ui_.cur_window_id) {
    if (ui_.submitting_overlay)
      cmd_buffer = &ui_.overlay_cmd_buffer;
    else
      cmd_buffer = &ui_.orphan_cmd_buffer;
  }

  assert(ui_.started && "Cannot submit command when ui is not started.");
  sb_write(cmd_buffer, &op, sizeof(op));
  sb_write(cmd_buffer, ptr, size);
}

void ui_overlay_begin(void) {
  ui_.submitting_overlay = true;
}
void ui_overlay_end(void) {
  ui_.submitting_overlay = false;
}

void ui_cmd_text_ex(uint32_t color, const char *text, float x, float y, Ui_Text_Ex ex) {
  ui_cmd_color_text_part_ex(color, text, str_size(text), x, y, ex);
}

void ui_cmd_color_text_part_ex(uint32_t color, const char *text, int text_len, float x, float y, Ui_Text_Ex ex) {
  if (ex.centered) {
    x -= ui_get_text_width_sv(sv_make(text, text_len)) / 2;
  }

  if (ex.fill_bg) {
    Ui_Box box = ui_box(x-1, y-1, ui_get_text_width(text)+1, ui_get_text_height()+1);
    ui_cmd_box(UI_COLOR_BG, box);
  }

  Ui_Cmd_Text cmd = {0};
  cmd.str_id = sb_bank_add_string_part(&ui_.bank, text, text_len);
  cmd.x = x;
  cmd.y = y;
  cmd.color = color;
  if (ex.num_text_effects) {
    cmd.text_effect_index = arr_len(ui_.text_effects);
    cmd.num_text_effects = ex.num_text_effects;
    for (int i = 0; i < ex.num_text_effects; i++) {
      arr_add(ui_.text_effects, ex.text_effects[i]);
    }
  }
  if (ui_.has_next_color) {
    cmd.color = ui_.next_color;
    ui_.has_next_color = false;
  }
  ui_add_command(UI_CMD_TEXT, &cmd, sizeof(cmd));
}

void ui_cmd_color_text_part(uint32_t color, const char *text, int text_len, float x, float y) {
  Ui_Text_Ex ex = {0};
  ui_cmd_color_text_part_ex(color, text, text_len, x, y, ex);
}

void ui_cmd_color_text(uint32_t color, const char *text, float x, float y) {
  ui_cmd_color_text_part(color, text, str_size(text), x, y);
}

void ui_cmd_text(const char *text, float x, float y) {
  ui_cmd_color_text(UI_COLOR_FG, text, x, y);
}

static void ui_limit_window_pos(Ui_Window *window) {
  Ui_Box *box = &window->box;
  if (box->x < 0) {
    box->x = 0;
  }
  if (box->x+box->w >= ui_.width) {
    box->x = ui_.width - box->w;
  }
  if (box->y < 0) {
    box->y = 0;
  }
  if (box->y+box->h >= ui_.height-UI_APP_BAR_H) {
    box->y = ui_.height-UI_APP_BAR_H - box->h;
  }
}

void ui_cmd_box_redact(uint32_t color, Ui_Box box) {
  Ui_Cmd_Box cmd = {0};
  if (box.w <= 0 || box.h <= 0)
    return;
  cmd.x = floor(box.x);
  cmd.y = floor(box.y);
  cmd.width  = box.w;
  cmd.height = box.h;
  cmd.color = color;
  ui_add_command(UI_CMD_BOX_REDACT, &cmd, sizeof(cmd));
}

void ui_cmd_box_bg(uint32_t color, Ui_Box box) {
  Ui_Cmd_Box cmd = {0};
  if (box.w <= 0 || box.h <= 0)
    return;
  cmd.x = floor(box.x);
  cmd.y = floor(box.y);
  cmd.width  = box.w;
  cmd.height = box.h;
  cmd.color = color;
  ui_add_command(UI_CMD_BOX_BG, &cmd, sizeof(cmd));
}

void ui_cmd_box_type(Ui_Box_Type type, uint32_t color, Ui_Box box) {
  switch (type) {
    case UI_BOX_FILL:
      ui_cmd_box(color, box);
      break;
    case UI_BOX_LINE:
      ui_cmd_line_box(color, box, UI_BOX_SIDE_ALL);
      break;
    case UI_BOX_CHECKERBOARD:
      ui_cmd_checkerboard_box(color, box);
      break;
  }
}

void ui_cmd_box(uint32_t color, Ui_Box box) {
  Ui_Cmd_Box cmd = {0};
  if (box.w <= 0 || box.h <= 0)
    return;
  cmd.x = floor(box.x);
  cmd.y = floor(box.y);
  cmd.width  = box.w;
  cmd.height = box.h;
  cmd.color = color;
  ui_add_command(UI_CMD_BOX, &cmd, sizeof(cmd));
}


void ui_cmd_line(uint32_t color, float x0, float y0, float x1, float y1) {
  Ui_Cmd_Line cmd = {0};
  cmd.x0 = x0;
  cmd.y0 = y0;
  cmd.x1 = x1;
  cmd.y1 = y1;
  cmd.color = color;
  ui_add_command(UI_CMD_LINE, &cmd, sizeof(cmd));
}

void ui_cmd_img_box_q(Gfx_Texture *texture, Ui_Box box, float quad[4]) {
  uint32_t color = 0xffffffff;
  if (ui_.has_next_color) {
    ui_.has_next_color = false;
    color = ui_.next_color;
  }
  ui_cmd_color_img_box_q(color, texture, box, quad);
}
void ui_cmd_color_img_box_q(uint32_t color, Gfx_Texture *texture, Ui_Box box, float quad[4]) {
  Ui_Cmd_Img_Box cmd = {0};
  cmd.x = floor(box.x);
  cmd.y = floor(box.y);
  cmd.width  = box.w;
  cmd.height = box.h;
  cmd.texture = texture;
  cmd.color = color;

  cmd.qx = quad[0];
  cmd.qy = quad[1];
  cmd.qw = quad[2];
  cmd.qh = quad[3];

  ui_add_command(UI_CMD_IMG_BOX, &cmd, sizeof(cmd));
}
void ui_cmd_img_box(Gfx_Texture *texture, Ui_Box box) {
  float quad[4] = {0.f, 0.f, (float)texture->width, (float)texture->height, };
  ui_cmd_img_box_q(texture, box, quad);
}
void ui_cmd_img(Gfx_Texture *texture, float x, float y) {
  ui_cmd_img_box(texture, ui_box(x, y, (float)texture->width, (float)texture->height));
}
void ui_cmd_color_img(uint32_t color, Gfx_Texture *texture, float x, float y) {
  float quad[4] = {0.f, 0.f, (float)texture->width, (float)texture->height, };
  ui_cmd_color_img_box_q(color, texture, ui_box(x, y, (float)texture->width, (float)texture->height), quad);
}

uint32_t ui_color(float r, float g, float b, float a) {
  return RGBA_TO_UINT32(r,g,b,a);
}

bool ui_is_current_hovered_window(void) {
  return ui_.hovered_window_id == ui_.cur_window_id || ui_.submitting_overlay;
}

bool ui_hover(Ui_Box box) {
  return
    (!ui_.is_hovering_redaction || ui_.cur_window_priority > ui_.max_redaction_priority || ui_.submitting_overlay) &&
    ui_is_current_hovered_window() &&
    ui_hover_raw(ui_.cur_box_screen_space) &&
    ui_hover_raw(box);
}

float ui_quantize_t(float t, int num_steps) {
  float step_size = 1.f / (float)num_steps;
  return floorf(t/step_size) * step_size;
}

Ui_Box ui_get_window_client_box(Ui_Window *window) {
  return  window->box;
}

void ui_disable_input_for_non_window_widgets_above_app_bar(void) {
  ui_.disable_input_for_non_window_widgets_above_app_bar_next = true;
  ui_.disable_input_for_non_window_widgets_above_app_bar = true;
}

void ui_redact(Ui_Box box) {
  ui_transform_box(&box);
  Ui_Redaction redact = {0};
  redact.priority = ui_.cur_window_priority;
  redact.box_in_screen_space = box;
  redact.window_id = ui_.cur_window_id;
  arr_add(ui_.redactions, redact);
  ui_cmd_box_redact(UI_COLOR_FG, box);
}

void ui_color_text(uint32_t color, const char *text, float x, float y) {
  Ui_Box box = {0};
  box.x = x;
  box.y = y;
  box.w = font_get_width(ui_.cur_font, text);
  box.h = font_get_height(ui_.cur_font);

  ui_transform_box(&box);

  (void)ui_widget(box);
  ui_cmd_color_text(color, text, box.x, box.y);
}

bool ui_window_is_front(Ui_Window *window) {
  return window->priority >= ui_.max_priority_seen_last_frame;
}
void ui_window_bring_to_front(Ui_Window *window) {
  window->priority = ++ui_.max_priority;
}

void ui_close_dropdown(Ui_Widget_Id id) {
  if (id && ui_.opened_dropdown_id == id) {
    ui_.opened_dropdown_id = 0;
  }
}

bool ui_dropdown(Ui_Widget_Id *id, const char *display, Ui_Box box) {
  ui_transform_box(&box);

  Ui_Widget widget = ui_widget_with_id_ptr(box, id);
  uint32_t fg_color = UI_COLOR_FG_MUTED;
  if (widget.disabled) {
    fg_color = UI_COLOR_DISABLED;
  }

  if (widget.hovered && !widget.disabled) {
    ui_cmd_line_box(fg_color, box, UI_BOX_SIDE_ALL);
  }
  else {
    ui_cmd_dotted_line_box(fg_color, box, UI_BOX_SIDE_ALL);
  }

  float triangle_x = box.x + box.w - 7;
  float triangle_y = box.y + box.h/2 - 2;
  ui_cmd_box(fg_color, ui_box(triangle_x,   triangle_y+0, 5, 1));
  ui_cmd_box(fg_color, ui_box(triangle_x+1, triangle_y+1, 3, 1));
  ui_cmd_box(fg_color, ui_box(triangle_x+2, triangle_y+2, 1, 1));

  float text_x = box.x + 2;
  float text_y = box.y + box.h/2 - ui_get_text_height()/2 + 1;
  ui_cmd_color_text(fg_color, display, text_x, text_y);

  bool is_open = ui_.opened_dropdown_id == *id;
  if (is_open) {
    Ui_Window_Options options = {0};
    options.no_title = true;
    options.is_dropdown_ = true;
    Ui_Box dropdown_box = box;
    dropdown_box.y += box.h;
    dropdown_box.h = MAX(dropdown_box.h, ui_.dropdown_content_box.h);
    dropdown_box.w = MAX(dropdown_box.w, ui_.dropdown_content_box.w);
    return ui_window(&ui_.dropdown_window, "##dropdown", dropdown_box, &options);
  }

  if (widget.clicked) {
    ui_.opened_dropdown_id = *id;
    ui_.dropdown_just_opened = true;
    ui_.dropdown_content_box = ui_box(0,0,0,0);
  }

  return false;
}

void ui_dropdown_end() {
  ui_.dropdown_content_box = ui_.content_box_local_space;
  ui_window_end(&ui_.dropdown_window);
}

bool ui_dropdown_simple(Ui_Dropdown_Simple *dropdown, int *selected_value, const Ui_Dropdown_Item *items, int num_items, Ui_Box box) {
  const char *display = NULL;
  for (int i = 0; i < num_items; i++) {
    if (items[i].value == *selected_value) {
      display = items[i].display;
      break;
    }
  }

  bool changed = false;
  if (ui_dropdown(&dropdown->id, display, box)) {
    float item_h = box.h;

    int height_in_items = 6;
    if (num_items < height_in_items)
      height_in_items = num_items;
    if (height_in_items <= 0)
      height_in_items = 1;

    ui_panel_begin(&dropdown->panel, ui_box(0,0, box.w, item_h * height_in_items + 4));
    {
      for (int i = 0; i < num_items; i++) {
        Ui_Dropdown_Item item = items[i];
        bool selected = item.value == *selected_value;
        if (ui_selectable(item.display, selected, ui_box(0,item_h*i,box.w, box.h))) {
          *selected_value = item.value;
          ui_close_dropdown(dropdown->id);
          changed = true;
        }
      }
    }
    ui_panel_end(&dropdown->panel);
    ui_dropdown_end();
  }

  return changed;
}

bool ui_dotted_button(uint32_t fg_color, Ui_Widget_Id *id, const char *label, Ui_Box box) {
  ui_transform_box(&box);

  float text_x = box.x + box.w/2 - ui_get_text_width(label)/2;
  float text_y = box.y + box.h/2 - ui_get_text_height()/2 + 1;
  Ui_Box left_box = ui_box(box.x, box.y+1, 2, box.h-2);
  Ui_Box right_box = ui_box(box.x + box.w-2, box.y+1, 2, box.h-2);
  Ui_Box middle_box = ui_box(box.x+1, box.y, box.w-2, box.h);
  Ui_Widget widget = {0};
  uint32_t color = fg_color;
  bool dotted, filled;

  dotted = 1;
  widget = ui_widget_with_id_ptr(box, id);
  if (widget.hovered) {
    dotted = 0;
  }

  if (widget.disabled)
    color = UI_COLOR_DISABLED;

  filled = widget.active && widget.hovered;

  if (filled)
    ui_cmd_box(color, middle_box);
  else if (dotted)
    ui_cmd_dotted_line_box(color, middle_box, UI_BOX_SIDE_TOP | UI_BOX_SIDE_BOTTOM);
  else
    ui_cmd_line_box(color, middle_box, UI_BOX_SIDE_TOP | UI_BOX_SIDE_BOTTOM);
  ui_cmd_box(color, left_box);
  ui_cmd_box(color, right_box);
  ui_cmd_color_text(filled ? UI_COLOR_BG : color, label, text_x, text_y);

  return widget.clicked;
}

void ui_text(const char *text, float x, float y) {
  ui_transform_pos_to_absolute(&x, &y);
  ui_cmd_text(text, x, y);
}

bool ui_window_update_minimize(Ui_Window *window, Ui_Window_Options *options) {
  move_to(&window->minimize_t, options->minimize ? 1.f : 0.f, ui_.dt / 0.20f);
  if (window->minimize_t > 0) {
    if (window->minimize_t < 1) {
      float minimize_t = window->minimize_t;
      minimize_t = ease_cos(minimize_t);
      float quantized_t = ui_quantize_t(minimize_t, 11);

      ui_overlay_begin();
      Ui_Box real_box = ui_box_lerp(window->box, options->minimize_box, quantized_t);
      ui_cmd_line_box(UI_COLOR_BG, ui_box_pad(real_box, 1), UI_BOX_SIDE_ALL);
      ui_cmd_line_box(UI_COLOR_BG, ui_box_shrink(real_box, 1), UI_BOX_SIDE_ALL);
      ui_cmd_line_box(UI_COLOR_FG, real_box, UI_BOX_SIDE_ALL);
      ui_overlay_end();
    }

    return true;
  }

  return false;
}

enum {
  UI_TITLE_BUTTON_EXIT,
  UI_TITLE_BUTTON_MINIMIZE,
};

bool ui_window_title_button(Ui_Box title_box, int type, uint32_t color, int index, bool disabled) {
  Ui_Box box = {0};
  box.w = 8;
  box.h = 8;
  box.x = title_box.x + title_box.w - (box.w + 2) * (index+1);
  box.y = title_box.y + 1;

  Ui_Box shadow_box = {0};
  shadow_box = box;
  shadow_box.x += 1;
  shadow_box.y += 1;

  if (disabled)
    ui_set_next_disabled(true);
  Ui_Widget widget = ui_widget(box);

  uint32_t fg_color = color;
  if (disabled)
    fg_color = UI_COLOR_FG;

  if (widget.active) {
    box.x += 1;
    box.y += 1;
  }
  else if (widget.hovered) {
    fg_color = UI_COLOR_FG_HL;
  }

  uint32_t bg_color = UI_COLOR_BG;
  if (disabled)
    bg_color = UI_COLOR_DISABLED;

  ui_cmd_box(bg_color, shadow_box);
  ui_cmd_box(bg_color, box);
  ui_cmd_box(fg_color, ui_box_shrink(box, 1));
  if (type == UI_TITLE_BUTTON_EXIT) {
    Ui_Box icon_box = ui_box_shrink(box, 2);
    icon_box.x = floor(icon_box.x);
    icon_box.y = floor(icon_box.y);
    float x0 = icon_box.x;
    float x1 = icon_box.x+icon_box.w;
    float y0 = icon_box.y;
    float y1 = icon_box.y+icon_box.h;
    ui_cmd_line(bg_color, x0,y0, x1,y1);
    ui_cmd_line(bg_color, x0,y1, x1,y0);
  }
  else if (type == UI_TITLE_BUTTON_MINIMIZE) {
    Ui_Box icon_box = ui_box_shrink(box, 2);
    icon_box.x = floor(icon_box.x);
    icon_box.y = floor(icon_box.y);
    float x0 = icon_box.x;
    float x1 = icon_box.x + icon_box.w;
    float y = icon_box.y+icon_box.y + 1;
    ui_cmd_box(bg_color, ui_box(x0, y, x1-x0, 1));
  }

  return widget.clicked;
}

void ui_window_title_buttons(Ui_Box title_box, Ui_Window_Options *options) {
  // Exit button
  if (!options->hide_close_button) {
    bool disabled = false;

    disabled = options->close_button_disabled;
    if (ui_window_title_button(title_box, UI_TITLE_BUTTON_EXIT, UI_COLOR_ERROR, 0, disabled)) {
      options->out_close_button_clicked = true;
    }
    disabled = false;
    if (ui_window_title_button(title_box, UI_TITLE_BUTTON_MINIMIZE, UI_COLOR_FG, 1, disabled)) {
      options->out_minimize_button_clicked = true;
    }
  }
}

bool ui_window(Ui_Window *window, const char *title, Ui_Box box, Ui_Window_Options *options) {
  Ui_Window_Options def_options = {0};

  if (!options)
    options = &def_options;

  if (!window->window_id) {
    window->window_id = ++ui_.max_window_id;
    ui_window_bring_to_front(window);
    window->box = box;
    // If this window has never been drawn before, and the caller wants it to
    // be minimized from the start, set minimize_t to 1 preemptively so it
    // doesn't start with a minimize animation.
    if (options->minimize) {
      window->minimize_t = 1;
    }
  }

  if (options->is_dropdown_) {
    window->box = box;
    if (window->priority != ui_.max_priority) {
      window->priority = ++ui_.max_priority;
    }
  }
  else if (options->force_box) {
    window->box = box;
  }

  if (!options->force_box) {
    ui_limit_window_pos(window);
  }

  if (ui_window_update_minimize(window, options)) {
    //return false;
  }

  Font *cur_font = ui_.cur_font;
  Font *title_font = ui_.cur_font;
  if (options->title_font) {
    title_font = options->title_font;
  }

  Ui_Box window_box = window->box;

  Ui_Box title_box  = window_box;
  title_box.h = font_get_height(title_font) + 3;

  Ui_Box body_box   = window_box;
  if (!options->no_title) {
    body_box.y = window_box.y+title_box.h+1;
    body_box.h = window_box.h-title_box.h-1;
  }

  Ui_Box client_box = {0};
  client_box = body_box;

  window->parent_minimized = ui_.cur_window_minimized;
  window->parent_box = ui_.cur_box_screen_space;
  window->parent_window_id = ui_.cur_window_id;
  window->parent_window_priority = ui_.cur_window_priority;
  window->parent_scroll_y = ui_.scroll_y;
  window->cmd_begin = (int)ui_.window_cmd_buffer.size;
  window->parent_content_box = ui_.content_box_local_space;
  window->parent_scissor = ui_.scissor;

  ui_.content_box_local_space = ui_box(0,0,0,0);
  ui_.cur_window_id = window->window_id;
  ui_.cur_window_priority = window->priority;
  ui_.scroll_y = 0;
  ui_.cur_window_minimized = window->minimize_t > 0;

  if (ui_.hovered_window_id == window->window_id) {
    if (app_keyrelease(KEY_MOUSE_LEFT)) {
        window->priority = ++ui_.max_priority;
    }
    if (app_keypress(KEY_MOUSE_LEFT)) {
      if (ui_hover_raw(title_box)) {
        window->priority = ++ui_.max_priority;
        ui_.drag_window_id = window->window_id;;
        ui_.drag_window_mouse_offset_x = ui_.last_mouse_x - window->box.x;
        ui_.drag_window_mouse_offset_y = ui_.last_mouse_y - window->box.y;
      }
    }
  }

  if (!app_keydown(KEY_MOUSE_LEFT)) {
    ui_.drag_window_id = 0;
  }

  if (ui_.drag_window_id == window->window_id) {
    window->box.x = ui_.last_mouse_x - ui_.drag_window_mouse_offset_x;
    window->box.y = ui_.last_mouse_y - ui_.drag_window_mouse_offset_y;
    ui_limit_window_pos(window);
  }

  uint32_t color_fg    = UI_COLOR_FG;

  // Border
  ui_cmd_box_bg(UI_COLOR_BG, ui_box_pad(window_box, 1));

  // Title
  if (!options->no_title) {
    ui_cmd_box(color_fg,    title_box);
  }

  // Title text
  if (!options->no_title) {
    ui_set_next_color(UI_COLOR_BG);
    ui_set_font(title_font);
    ui_cmd_text(title, window_box.x+3, window_box.y+2);

    // Title buttons
    ui_window_title_buttons(title_box, options);
  }

  // Body
  ui_cmd_box_bg(color_fg,    body_box);

  ui_.cur_box_screen_space = client_box;
  ui_.scroll_y = 0;
  ui_.scissor.enabled = false;

  ui_set_font(cur_font);
  ui_.max_priority_seen_this_frame = MAX(ui_.max_priority_seen_this_frame, window->priority);

  return true;
}

void ui_set_next_disabled(bool disabled) {
  ui_.next_disabled = disabled;
}
void ui_set_next_color(uint32_t color) {
  ui_.has_next_color = true;
  ui_.next_color = color;
}
void ui_set_next_cannot_hover(void) {
  ui_.next_cannot_hover = true;
}

void ui_window_end(Ui_Window *window) {
  assert(window->window_id == ui_.cur_window_id && "Called mismatched ui_window_end. Did you foreget to call ui_window_end?");

  if (!ui_window_is_minimized(window) && ui_hover_raw(window->box)) {
    if (!ui_.next_hovered_window_id || ui_.next_hovered_window_priority < window->priority) {
      bool hovering_own_scope = false;
      for (int i = 0; i < arr_len(ui_.scope_boxes); i++) {
        if (ui_.scope_boxes[i].window_id == window->window_id &&
            ui_hover_raw(ui_.scope_boxes[i].box))
        {
          hovering_own_scope = true;
          break;
        }
      }
      
      if (!hovering_own_scope) {
        ui_.next_hovered_window_id = window->window_id;
        ui_.next_hovered_window_priority = window->priority;
        ui_.next_hovering_scope = false;
      }
      else {
        ui_.next_hovering_scope = true;
      }
    }
  }

  int cmd_list_begin = ui_.cmd_buffer.size;
  int cmd_list_size  = ui_.window_cmd_buffer.size - window->cmd_begin;
  sb_write(&ui_.cmd_buffer,
      ui_.window_cmd_buffer.ptr + window->cmd_begin,
      cmd_list_size);

  sb_resize(&ui_.window_cmd_buffer, window->cmd_begin);

  Ui_Command_List cmd_list = {0};
  cmd_list.priority = window->priority;
  cmd_list.window_id = window->window_id;
  cmd_list.begin = cmd_list_begin;
  cmd_list.num_bytes = cmd_list_size;
  arr_add(ui_.command_lists, cmd_list);

  ui_.content_box_local_space = window->parent_content_box;
  ui_.scroll_y = window->parent_scroll_y;
  ui_.cur_box_screen_space = window->parent_box;
  ui_.cur_window_id = window->parent_window_id;
  ui_.cur_window_priority = window->parent_window_priority;
  ui_.scissor = window->parent_scissor;
  ui_.cur_window_minimized = window->parent_minimized;
}

void ui_transform_pos_to_absolute(float *x, float *y) {
  *x += ui_.cur_box_screen_space.x;
  *y += ui_.cur_box_screen_space.y;
  *y -= ui_.scroll_y;
}

void ui_transform_box(Ui_Box *box) {
  ui_transform_pos_to_absolute(&box->x, &box->y);
}
void ui_transform_box_inverse(Ui_Box *box) {
  box->x -= ui_.cur_box_screen_space.x;
  box->y -= ui_.cur_box_screen_space.y;
  box->y += ui_.scroll_y;
}

void ui_expand_content_box(Ui_Box box) {
  Ui_Box *cb = &ui_.content_box_local_space;
  ui_transform_box_inverse(&box);
  *cb = ui_box_expand(*cb, box);
}

Ui_Widget ui_widget_with_id(Ui_Box box, Ui_Widget_Id id) {
  Ui_Widget ret = {0};
  bool enabled = !ui_.next_disabled && !ui_.disable_input && !ui_.cur_panel_disabled;
  if (ui_.disable_input_for_non_window_widgets_above_app_bar && ui_.cur_window_id == 0) {
    if (ui_.mouse_y < ui_.height-UI_APP_BAR_H) {
      enabled = false;
    }
  }

  ui_.next_disabled = false;

  ret.disabled = !enabled;
  if (!ui_.cur_window_minimized &&
      !ui_.next_cannot_hover &&
      enabled &&
      !ui_.hovered_widget_id &&
      (!id || ui_.hovered_widget_id != id) &&
      (!ui_.opened_dropdown_id || ui_.cur_window_id == ui_.dropdown_window.window_id) &&
      (!ui_.cur_panel_id || !ui_.has_panel_loaded_box || !ui_hover(ui_.panel_loaded_box)) &&
      ui_hover(box))
  {
    ret.hovered = true;
    ui_.hovered_widget_id = id ? id : UI_TRANSIENT_ID;
  }
  ui_.next_cannot_hover = false;

  if (id && enabled) {
    if (!ui_.active_widget_id) {
      if (ret.hovered && app_keypress(KEY_MOUSE_LEFT)) {
        ui_.active_widget_id = id;
        ret.became_active = true;
      }
    }
    ret.active = ui_.active_widget_id == id;
    ret.clicked = ret.hovered && ret.active && app_keyrelease(KEY_MOUSE_LEFT);
  } else {
    ret.became_active = ret.hovered && app_keypress(KEY_MOUSE_LEFT);
    ret.active = ret.hovered && app_keydown(KEY_MOUSE_LEFT);
    ret.clicked = ret.hovered && app_keyrelease(KEY_MOUSE_LEFT);
  }

  ret.focused = (id == ui_.focused_widget_id);

  ui_expand_content_box(box);
  ui_.last_widget = ret;
  ui_.last_widget_box = box;
  return ret;
}

bool ui_just_became_active(void) {
  return ui_.last_widget.became_active;
}
bool ui_is_active(void) {
  return ui_.last_widget.active;
}
bool ui_gained_focus(void) {
  return ui_.last_widget.focused;
}

Ui_Widget ui_widget(Ui_Box box) {
  return ui_widget_with_id(box, 0);
}

void ui_progress_bar(float t, Ui_Box box) {
  ui_transform_box(&box);
 
  Ui_Box inner_box = ui_box_shrink(box,2);
  inner_box.w *= t;

  ui_cmd_box(UI_COLOR_BG, ui_box_pad(box, 1));
  ui_cmd_line_box(UI_COLOR_FG, box, UI_BOX_SIDE_ALL);
  ui_cmd_checkerboard_box(UI_COLOR_FG, inner_box);
}

Ui_Widget ui_widget_with_id_ptr(Ui_Box box, Ui_Widget_Id *id) {
  if (!id) {
    return ui_widget(box);
  }
  if (!*id) {
    *id = ui_allocate_id();
  }
  return ui_widget_with_id(box, *id);
}

bool ui_selectable(const char *text, bool selected, Ui_Box box) {
  Ui_Widget widget = {0};
  ui_transform_box(&box);
  widget = ui_widget(box);

  if (selected) {
    ui_cmd_box(UI_COLOR_FG, box);
  }
  else if (widget.active) {
    ui_cmd_line_box(UI_COLOR_FG, box, UI_BOX_SIDE_ALL);
  }
  else if (widget.hovered) {
    ui_cmd_dotted_line_box(UI_COLOR_FG, box, UI_BOX_SIDE_ALL);
  }

  if (selected)
    ui_set_next_color(UI_COLOR_BG);
  else
    ui_set_next_color(UI_COLOR_FG);
  ui_cmd_text(text, box.x + 3, floorf(box.y + box.h/2) - font_get_height(ui_.cur_font)/2 + 1);
  return widget.clicked;
}

void ui_set_font(Font *font) {
  Ui_Cmd_Font cmd = {0};
  cmd.font = font;
  ui_add_command(UI_CMD_FONT, &cmd, sizeof(cmd));
  ui_.cur_font = font;
}

Font *ui_get_font(void) {
  return ui_.cur_font;
}

float ui_get_text_width_sv(String_View sv) {
  return font_get_width_part(ui_.cur_font, (const char *)sv.ptr, sv.length);
}

float ui_get_text_width(const char *text) {
  return font_get_width(ui_.cur_font, text);
}
float ui_get_text_height(void) {
  return font_get_height(ui_.cur_font);
}

#define UI_MAX(a, b) ((a)>(b)?(a):(b))
#define UI_MIN(a, b) ((a)<(b)?(a):(b))

Ui_Box ui_box(float x, float y, float w, float h) {
  Ui_Box ret = {0};
  ret.x = x;
  ret.y = y;
  ret.w = w;
  ret.h = h;
  return ret;
}

Ui_Box ui_box_shrink(Ui_Box box, float padding) {
  box.x += padding;
  box.y += padding;
  box.w -= padding*2;
  box.h -= padding*2;
  return box;
}

Ui_Box ui_box_pad(Ui_Box box, float padding) {
  box.x -= padding;
  box.y -= padding;
  box.w += padding*2;
  box.h += padding*2;
  return box;
}

Ui_Box ui_box_set_height(Ui_Box box, float height) {
  box.h = height;
  return box;
}

Ui_Box ui_box_border_top(Ui_Box box) {
  box.h = 1;
  return box;
}
Ui_Box ui_box_border_left(Ui_Box box) {
  box.w = 1;
  return box;
}
Ui_Box ui_box_border_bottom(Ui_Box box) {
  box.y += box.h-1;
  box.h = 1;
  return box;
}
Ui_Box ui_box_border_right(Ui_Box box) {
  box.x += box.w-1;
  box.w = 1;
  return box;
}

Ui_Box ui_box_expand(Ui_Box a, Ui_Box b) {
  float left = UI_MIN(a.x, b.x);
  float top  = UI_MIN(a.y, b.y);
  float right  = UI_MAX(a.x+a.w, b.x+b.w);
  float bottom = UI_MAX(a.y+a.h, b.y+b.h);
  return ui_box(left,top, right-left, bottom-top);
}

Ui_Box ui_box_lerp(Ui_Box a, Ui_Box b, float t) {
  Ui_Box ret = {0};
  ret.x = floorf(lerp(a.x, b.x, t));
  ret.y = floorf(lerp(a.y, b.y, t));
  ret.w = floorf(lerp(a.x+a.w, b.x+b.w, t)) - ret.x;
  ret.h = floorf(lerp(a.y+a.h, b.y+b.h, t)) - ret.y;
  return ret;
}

static void ui_setup_scrollbar_textures(void) {

  if (!ui_.scroll_bar_arrow_up_texture) {
    const uint32_t p1 = -1;
    const uint32_t p0 = 0x0;
    const uint32_t pixels[] = {
      p0,p0,p1,p0,p0,
      p0,p1,p1,p1,p0,
      p1,p1,p1,p1,p1,
    };
    ui_.scroll_bar_arrow_up_texture = gfx_new_texture(pixels, 5, 3);
  }

  if (!ui_.scroll_bar_arrow_down_texture) {
    const uint32_t p1 = -1;
    const uint32_t p0 = 0x0;
    const uint32_t pixels[] = {
      p1,p1,p1,p1,p1,
      p0,p1,p1,p1,p0,
      p0,p0,p1,p0,p0,
    };
    ui_.scroll_bar_arrow_down_texture = gfx_new_texture(pixels, 5, 3);
  }
}

static Ui_Widget ui_3d_button_impl(Ui_Widget_Id id, Ui_Box box) {
  Ui_Box inner_box = {0};
  Ui_Widget widget = ui_widget_with_id(box, id);

  ui_cmd_box(UI_COLOR_BG, box);

  inner_box = ui_box_shrink(box, 1);

  if (widget.active) {
    ui_cmd_box(UI_COLOR_DISABLED, inner_box);
    ui_cmd_box(UI_COLOR_FG, ui_box_border_bottom(inner_box));
    ui_cmd_box(UI_COLOR_FG, ui_box_border_right(inner_box));
  }
  else {
    ui_cmd_box(UI_COLOR_FG, inner_box);
  }

  return widget;
}

static Ui_Widget ui_3d_img_button_impl(Ui_Widget_Id id, Texture *texture, Ui_Box box) {
  Ui_Widget widget = ui_3d_button_impl(id, box);
  Ui_Box inner_box = ui_box_shrink(box, 1);
  ui_set_next_color(UI_COLOR_BG);
  ui_cmd_img(texture, inner_box.x+1, inner_box.y+1);
  return widget;
}

void ui_cmd_checkerboard_box(uint32_t color, Ui_Box box) {
  float quad[4] = { 0.f, 0.f, box.w, box.h, };

  if (!ui_.checker_board_texture) {
    uint32_t p0 = 0;
    uint32_t p1 = -1;
    // Checkerboard pattern
    const uint32_t pixels[] = {
      p1, p0,
      p0, p1,
    };
    ui_.checker_board_texture = gfx_new_texture(pixels, 2, 2);
    gfx_set_wrap(ui_.checker_board_texture, GFX_WRAP_MODE_REPEAT, GFX_WRAP_MODE_REPEAT);
  }

  ui_cmd_color_img_box_q(color, ui_.checker_board_texture, box, quad);
}

typedef struct {
  bool needs_scrolling;
  float scroll_max_dist;
  float scroll_handle_length_pct;
} Ui_Scroll_Info;

Ui_Scroll_Info ui_get_scroll_info(Ui_Box outer, Ui_Box inner) {
  Ui_Scroll_Info ret = {0};
  ret.needs_scrolling = inner.h > outer.h;
  ret.scroll_max_dist = (inner.h - outer.h);
  ret.scroll_handle_length_pct = outer.h / inner.h;
  return ret;
}

void ui_panel_scroll_to_top(Ui_Panel *panel) {
  panel->scroll_y = 0;
}

void ui_panel_disable(void) {
  ui_.cur_panel_disabled = true;
}

void ui_panel_begin(Ui_Panel *panel, Ui_Box box) {
  if (!panel->panel_id) {
    panel->panel_id = ++ui_.max_panel_id;
  }

  panel->has_loading_t = ui_.has_next_panel_loading_t;
  panel->loading_t = ui_.next_panel_loading_t;
  ui_.has_next_panel_loading_t = false;

  ui_transform_box(&box);
  Ui_Box client_box = ui_box_shrink(box, 2);

  panel->parent_box = ui_.cur_box_screen_space;
  panel->parent_scroll_y = ui_.scroll_y;

  ui_expand_content_box(box); // This must be done before ui_.scroll_y is set

  ui_.cur_box_screen_space = client_box;
  ui_.scroll_y = panel->scroll_y;

  Ui_Box parent_content_box = ui_.content_box_local_space;
  Ui_Scroll_Info scroll_info = ui_get_scroll_info(client_box, panel->content_box_local_space);
  panel->content_box_local_space = parent_content_box;

  if (scroll_info.needs_scrolling) {
    if (panel->has_requested_scroll_y) {
      panel->has_requested_scroll_y = false;
      panel->scroll_y = panel->requested_scroll_y;
    }
    else if (ui_is_current_hovered_window() &&
        !ui_.cur_panel_disabled &&
        (!ui_.opened_dropdown_id || ui_.dropdown_window.window_id == ui_.cur_window_id) &&
        (!ui_.disable_input_for_non_window_widgets_above_app_bar || ui_.cur_window_id != 0) &&
        ui_hover_raw(client_box))
    {
      panel->scroll_y -= app_mouse_wheel_y() * 13;
    }

    panel->scroll_y = UI_MAX(panel->scroll_y, 0);
    panel->scroll_y = UI_MIN(panel->scroll_y, scroll_info.scroll_max_dist);
  }
  else {
    panel->scroll_y = 0;
  }

  ui_setup_scrollbar_textures();
  ui_cmd_box_bg(UI_COLOR_FG, box);
  ui_cmd_box_bg(UI_COLOR_BG, ui_box_shrink(box, 1));

  // Scroll bar
  const float scroll_bar_w = 8;
  const float scroll_bar_h = box.h;
  const float scroll_bar_x = box.x+box.w-scroll_bar_w;
  const float scroll_bar_y = box.y;

  const float scroll_bar_button_w = scroll_bar_w+1;
  const float scroll_bar_button_h = 7;

  const float scroll_bar_up_button_x = scroll_bar_x-1;
  const float scroll_bar_up_button_y = scroll_bar_y + scroll_bar_h - scroll_bar_button_h * 2 + 1/*So there's no two black bars between then*/;

  const float scroll_bar_down_button_x = scroll_bar_x-1;
  const float scroll_bar_down_button_y = scroll_bar_y + scroll_bar_h - scroll_bar_button_h * 1;

  const float scroll_bar_handle_area_x = scroll_bar_x;
  const float scroll_bar_handle_area_y = scroll_bar_y;
  const float scroll_bar_handle_area_h = scroll_bar_up_button_y - scroll_bar_handle_area_y;
  const float scroll_bar_handle_area_w = scroll_bar_button_w;

  const float scroll_bar_handle_w = scroll_bar_w-2;
  const float scroll_bar_handle_h = -1 + scroll_info.scroll_handle_length_pct * scroll_bar_handle_area_h;

  if (scroll_info.needs_scrolling) {
    //=====================================
    // Background and checkerboard

    Ui_Box scroll_bar_box = ui_box(scroll_bar_x, scroll_bar_y, scroll_bar_w, scroll_bar_h);
    Ui_Box scroll_bar_bg_box = scroll_bar_box;
    scroll_bar_bg_box.y += 1;
    scroll_bar_bg_box.h -= 2;
    scroll_bar_bg_box.x += 1;
    scroll_bar_bg_box.w -= 2;
    ui_cmd_box(UI_COLOR_FG, scroll_bar_box);
    ui_cmd_box(UI_COLOR_BG, scroll_bar_bg_box);

    ui_cmd_checkerboard_box(UI_COLOR_FG, ui_box_shrink(scroll_bar_bg_box, 1));

    //=====================================
    // Up and down button
    {
      Ui_Box btn_box_up   = ui_box(scroll_bar_up_button_x, scroll_bar_up_button_y, scroll_bar_button_w, scroll_bar_button_h);
      Ui_Box btn_box_down = ui_box(scroll_bar_down_button_x, scroll_bar_down_button_y, scroll_bar_button_w, scroll_bar_button_h);
      if (ui_3d_img_button_impl(0, ui_.scroll_bar_arrow_up_texture,   btn_box_up).clicked) {
        panel->has_requested_scroll_y = true;
        panel->requested_scroll_y = panel->scroll_y - 25;
      }
      if (ui_3d_img_button_impl(0, ui_.scroll_bar_arrow_down_texture, btn_box_down).clicked) {
        panel->has_requested_scroll_y = true;
        panel->requested_scroll_y = panel->scroll_y + 25;
      }
    }

    //=====================================
    // The bar
    {
      // Handle position
      float scroll_pct = panel->scroll_y / scroll_info.scroll_max_dist;
      float max_handle_pos = scroll_bar_handle_area_h - scroll_bar_handle_h;
      float handle_pos = scroll_pct * max_handle_pos;

      Ui_Box scroll_handle_box = ui_box(floor(scroll_bar_box.x+1), floor(box.y)+1 + floor(handle_pos), scroll_bar_handle_w, scroll_bar_handle_h);
      scroll_handle_box.h = ceilf(scroll_handle_box.h);
      scroll_handle_box.w = ceilf(scroll_handle_box.w);
      if (!panel->scroll_handle_id)
        panel->scroll_handle_id = ui_allocate_id();
      Ui_Widget handle_widget = ui_3d_button_impl(panel->scroll_handle_id, scroll_handle_box);

      if (handle_widget.became_active) {
        panel->scroll_mouse_offset_y = ui_.mouse_y - handle_pos;
      }
      else if (handle_widget.active) {
        float requested_handle_pos = ui_.mouse_y - panel->scroll_mouse_offset_y;
        float requested_scroll_pct = requested_handle_pos / max_handle_pos;
        panel->requested_scroll_y = requested_scroll_pct * scroll_info.scroll_max_dist;
        panel->has_requested_scroll_y = true;
      }
    }
  } // Scroll bar

  {
    // Set up the global states.
    Ui_Box scissor_box = client_box;
    if (scroll_info.needs_scrolling) {
      scissor_box.w -= scroll_bar_handle_w;
    }

    ui_cmd_scissor_begin(&panel->scissor, scissor_box);
    ui_.cur_box_screen_space = scissor_box;
    ui_.content_box_local_space = ui_box(0,0,0,0);
  }

  panel->parent_panel_id = ui_.cur_panel_id;
  panel->parent_panel_disabled = ui_.cur_panel_disabled;
  ui_.cur_panel_id = panel->panel_id;
  if (panel->has_loading_t) {
    panel->parent_loaded_box = ui_.panel_loaded_box;
    panel->parent_has_panel_loaded_box = ui_.has_panel_loaded_box;
    ui_.has_panel_loaded_box = true;
    ui_.panel_loaded_box = panel->last_loaded_box;
  }
}

void ui_panel_end(Ui_Panel *panel) {

  ui_.panel_loaded_box = panel->parent_loaded_box;
  ui_.has_panel_loaded_box = panel->parent_has_panel_loaded_box;

  if (panel->has_loading_t) {
    panel->has_loading_t = false;
    Ui_Box blocking_box = ui_.content_box_local_space;
    blocking_box.h *= 1.f - panel->loading_t;
    blocking_box.y = ui_.content_box_local_space.y + ui_.content_box_local_space.h - blocking_box.h;

    //float right = MAX(
    //    ui_.content_box_local_space.x + ui_.content_box_local_space.w,
    //    panel->scissor.box.x + panel->scissor.box.w);
    //blocking_box.w = right - blocking_box.x;

    ui_transform_box(&blocking_box);

    if (ui_.cur_box_screen_space.x + ui_.cur_box_screen_space.w > blocking_box.x+blocking_box.w) {
      blocking_box.w = ui_.cur_box_screen_space.x + ui_.cur_box_screen_space.w - blocking_box.x;
    }

    ui_cmd_box(UI_COLOR_BG, blocking_box);
    panel->last_loaded_box = blocking_box;
  }

  if (ui_.cur_panel_disabled) {
    ui_cmd_checkerboard_box(UI_COLOR_BG, ui_.cur_box_screen_space);
  }
  ui_cmd_scissor_end(&panel->scissor);

  ui_.cur_box_screen_space = panel->parent_box;
  Ui_Box parent_content_box = panel->content_box_local_space;
  panel->content_box_local_space = ui_.content_box_local_space;
  ui_.content_box_local_space = parent_content_box;
  ui_.scroll_y = panel->parent_scroll_y;
  ui_.cur_panel_id = panel->parent_panel_id;
  ui_.cur_panel_disabled = panel->parent_panel_disabled;
}

void ui_cmd_line_box(uint32_t color, Ui_Box box, uint32_t sides) {
  box.x = floorf(box.x);
  box.y = floorf(box.y);
  box.w = floorf(box.w);
  box.h = floorf(box.h);

  if (sides == 0)
    return;
  if (sides & UI_BOX_SIDE_TOP) {
    Ui_Box top_box = ui_box(box.x, box.y, box.w, 1);
    ui_cmd_box(color, top_box);
  }

  if (sides & UI_BOX_SIDE_BOTTOM) {
    Ui_Box bottom_box = ui_box(box.x, box.y + box.h - 1, box.w, 1);
    ui_cmd_box(color, bottom_box);
  }

  if (sides & UI_BOX_SIDE_LEFT) {
    Ui_Box left_box = ui_box(box.x, box.y, 1, box.h);
    ui_cmd_box(color, left_box);
  }

  if (sides & UI_BOX_SIDE_RIGHT) {
    Ui_Box right_box = ui_box(box.x + box.w-1, box.y, 1, box.h);
    ui_cmd_box(color, right_box);
  }
}

void ui_cmd_dotted_line_box(uint32_t color, Ui_Box box, uint32_t sides) {
  if (sides == 0)
    return;
  if (sides & UI_BOX_SIDE_TOP) {
    Ui_Box top_box = ui_box(box.x, box.y, box.w, 1);
    ui_cmd_checkerboard_box(color, top_box);
  }

  if (sides & UI_BOX_SIDE_BOTTOM) {
    Ui_Box bottom_box = ui_box(box.x, box.y + box.h - 1, box.w, 1);
    ui_cmd_checkerboard_box(color, bottom_box);
  }

  if (sides & UI_BOX_SIDE_LEFT) {
    Ui_Box left_box = ui_box(box.x, box.y, 1, box.h);
    ui_cmd_checkerboard_box(color, left_box);
  }

  if (sides & UI_BOX_SIDE_RIGHT) {
    Ui_Box right_box = ui_box(box.x + box.w-1, box.y, 1, box.h);
    ui_cmd_checkerboard_box(color, right_box);
  }
}

bool ui_is_focused(Ui_Widget_Id id) {
  return id == ui_.focused_widget_id;
}

bool ui_dotted_input_field(Ui_Widget_Id *id, const char *label, Ui_Box box, Ui_Input_Options options) {
  Ui_Box box_left, box_right, box_middle;
  Ui_Box cursor_box, text_box;
  uint32_t color, inverse_text_color;
  float label_x, label_y;
  Ui_Widget widget;
  bool changed = false;

  ui_transform_box(&box);
  inverse_text_color = UI_COLOR_BG;
  color = UI_COLOR_FG;

  box_left    = box;
  box_left.w  = str_empty(label) ? 3 : (MIN(25, box.w));
  box_right   = box;
  box_right.w = 3;
  box_right.x = box.x+box.w - box_right.w;
  box_middle = box;
  box_middle.x = box_left.x+box_left.w;
  box_middle.w = box_right.x-box_middle.x;

  widget = ui_widget_with_id_ptr(box, id);
  if (widget.disabled) {
    color = UI_COLOR_DISABLED;
  } else if (widget.hovered && widget.clicked) {
    ui_.input_cursor_flashing_t = 0;
    ui_.focused_widget_id = *id;
  }

  ui_cmd_box(color, box_left);
  ui_cmd_box(color, box_right);

  if (options.buffer_size) options.buffer[options.buffer_size-1] = 0;
  if (ui_is_focused(*id)) {
    for (int i = 0, j = 0, input_char = 0; (input_char = app_get_char_input(i)); i++) {
      if (!font_has_glyph(ui_.cur_font, input_char))
        continue;

      for (j = 0; j+1 < options.buffer_size; j++) {
        if (options.buffer[j] == 0) {
          if (options.correct_string) {
            if (j < str_size(options.correct_string)) {
              options.buffer[j] = options.correct_string[j];
              changed = true;
            }
          } else {
            options.buffer[j] = input_char;
            changed = true;
          }
          break;
        }
      }
    }

    if (app_keypress_allow_repeat(KEY_BACKSPACE)) {
      for (int i = options.buffer_size-1; i >= 0; i--) {
        if (options.buffer[i]) {
          options.buffer[i] = 0;
          changed = true;
          break;
        }
      }
    }
  }

  if (widget.hovered || ui_is_focused(*id)) {
    ui_cmd_line_box(color, box_middle, UI_BOX_SIDE_TOP | UI_BOX_SIDE_BOTTOM);
  } else {
    ui_cmd_dotted_line_box(color, box_middle, UI_BOX_SIDE_TOP | UI_BOX_SIDE_BOTTOM);
  }

  /* Draw the label */
  if (!str_empty(label) && label[0] != '#') {
    //ui_set_font(game_.font_square_small);
    label_x = box.x + 2;
    label_y = box.y + box.h/2 - ui_get_text_height()/2 + 1;
    ui_cmd_color_text(inverse_text_color, label, label_x, label_y);
  }

  /* Draw the actual text */
  text_box = box_middle;
  text_box.x += 2;
  text_box.y += 2;
  text_box.w = ui_get_text_width(options.buffer);
  text_box.h -= 4;
  if (!str_empty(options.buffer)) {
    Ui_Text_Ex text_ex = {0};
    text_ex.num_text_effects = options.num_text_effects;
    text_ex.text_effects = options.text_effects;
    ui_cmd_color_text_part_ex(color, options.buffer, str_len(options.buffer), text_box.x, text_box.y, text_ex);
  }

  if (ui_is_focused(*id)) {
    cursor_box = text_box;
    cursor_box.x = text_box.x + text_box.w;
    cursor_box.w = ui_get_text_width("t");
    if (ui_.input_cursor_flashing_t <= 0.65f) {
      ui_cmd_box(color, cursor_box);
    }
  }

  return changed;
}

void ui_scope(Ui_Box box) {
  ui_transform_box(&box);
  Ui_Scope_Box scope = {0};
  scope.box = box;

  if (ui_.scissor.enabled) {
    scope.box = ui_box_scissor(scope.box, ui_.scissor.box);
    if (scope.box.w == 0 || scope.box.h == 0)
      return;
  }

  scope.window_id = ui_.cur_window_id;
  scope.priority = ui_.cur_window_priority;
  arr_add(ui_.scope_boxes, scope);
  //ui_cmd_box(0xffffffff, box);
}

Ui_Box ui_box_scissor(Ui_Box box, Ui_Box scissor_box) {
  if (box.x <= scissor_box.x) {
    float diff = scissor_box.x - box.x;
    box.x += diff;
    box.w -= diff;
  }
  if (box.x+box.w > scissor_box.x+scissor_box.w) {
    float diff = (box.x+box.w) - (scissor_box.x+scissor_box.w);
    box.w -= diff;
  }
  box.w = box.w < 0 ? 0 : box.w;


  if (box.y <= scissor_box.y) {
    float diff = scissor_box.y - box.y;
    box.y += diff;
    box.h -= diff;
  }
  if (box.y+box.h > scissor_box.y+scissor_box.h) {
    float diff = (box.y+box.h) - (scissor_box.y+scissor_box.h);
    box.h -= diff;
  }
  box.h = box.h < 0 ? 0 : box.h;
  return box;
}

void ui_cmd_scissor_begin(Ui_Scissor *scissor, Ui_Box box) {
  if (ui_.scissor.enabled) {
    box = ui_box_scissor(box, ui_.scissor.box);
  }

  {
    Ui_Cmd_Scissor cmd = {0};
    cmd.box = box;
    ui_add_command(UI_CMD_SCISSOR, &cmd, sizeof(cmd));
  }

  *scissor = ui_.scissor;
  ui_.scissor.box = box;
  ui_.scissor.enabled = true;
}

void ui_cmd_scissor_end(Ui_Scissor *scissor) {
  ui_add_command(UI_CMD_UNSET_SCISSOR, NULL, 0);
  ui_.scissor = *scissor;
}

void ui_cmd_box_callback(Ui_Box box, Ui_Cmd_Box_Callback_Type *callback, void *arg) {
  Ui_Cmd_Box_Callback cmd = {0};
  cmd.box = box;
  cmd.arg = arg;
  cmd.callback = callback;
  ui_add_command(UI_CMD_BOX_CALLBACK, &cmd, sizeof(cmd));
}

void ui_draw(void) {
  int i = 0;
  gfx_push();

  gfx_set_font(ui_.cur_font);

  for (i = 0; i < arr_len(ui_.command_lists); i++) {
    Ui_Command_List cmd_list = ui_.command_lists[i];
    Data_Reader dr = make_data_reader(ui_.cmd_buffer.ptr + cmd_list.begin, cmd_list.num_bytes);
    while (!dr_reached_end(&dr)) {
      uint16_t op = 0;
      dr_read_uint16(&dr, &op);

      switch (op) {
        case UI_CMD_FONT:
        {
          Ui_Cmd_Font cmd = {0};
          dr_read(&dr, &cmd, sizeof(cmd));
          gfx_set_font(cmd.font);
        } break;

        case UI_CMD_IMG_BOX:
        {
          Ui_Cmd_Img_Box cmd = {0};
          dr_read(&dr, &cmd, sizeof(cmd));

          {
            Texture *t = cmd.texture;
            float x0 = floor(cmd.x);
            float y0 = floor(cmd.y);
            float x1 = x0+cmd.width;
            float y1 = y0+cmd.height;

            float u0 = cmd.qx / (float)t->width;
            float v0 = cmd.qy / (float)t->height;
            float u1 = (cmd.qx+cmd.qw) / (float)t->width;
            float v1 = (cmd.qy+cmd.qh) / (float)t->height;

            gfx_imm_begin_2d();
            gfx_imm_vertex_2d(x0,y0, u0,v0, cmd.color);
            gfx_imm_vertex_2d(x1,y0, u1,v0, cmd.color);
            gfx_imm_vertex_2d(x1,y1, u1,v1, cmd.color);
            gfx_imm_vertex_2d(x0,y0, u0,v0, cmd.color);
            gfx_imm_vertex_2d(x1,y1, u1,v1, cmd.color);
            gfx_imm_vertex_2d(x0,y1, u0,v1, cmd.color);
            gfx_imm_end();
            gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, t);
          }
        } break;

        case UI_CMD_LINE:
        {
          Ui_Cmd_Line cmd = {0};
          dr_read(&dr, &cmd, sizeof(cmd));

          gfx_imm_begin_2d();
          gfx_imm_vertex_2d(cmd.x0, cmd.y0, 0,0, cmd.color);
          gfx_imm_vertex_2d(cmd.x1, cmd.y1, 0,0, cmd.color);
          gfx_imm_end();
          gfx_imm_draw(GFX_PRIMITIVE_LINES, NULL);

        } break;

        case UI_CMD_BOX_REDACT:
        case UI_CMD_BOX_BG:
        case UI_CMD_BOX:
        {
          Ui_Cmd_Box cmd = {0};
          dr_read(&dr, &cmd, sizeof(cmd));

          Ui_Box box = {0};
          box.x = floor(cmd.x);
          box.y = floor(cmd.y);
          box.w = cmd.width;
          box.h = cmd.height;

          if (op == UI_CMD_BOX) {
            gfx_set_color_u(cmd.color);
            gfx_fill_rect(box.x, box.y, box.w, box.h);
          }
          else {
            Farray(Ui_Box, 4) sliced_boxes = {0};
            farr_add(sliced_boxes, box);
            for (int i = 0; i < arr_len(ui_.scope_boxes); i++) {
              Ui_Scope_Box scope = ui_.scope_boxes[i];
              if (op == UI_CMD_BOX_BG && scope.window_id != cmd_list.window_id)
                continue;
              if (op == UI_CMD_BOX_REDACT && (
                scope.window_id == cmd_list.window_id || scope.priority <= cmd_list.priority))
              {
                continue;
              }

              Ui_Box sb = scope.box;
              sb.x = floor(sb.x);
              sb.y = floor(sb.y);
              int len_left = farr_len(sliced_boxes);
              for (int j = 0; j < len_left; j++) {
                Ui_Box b = sliced_boxes.data[j];
                farr_remove_unordered(sliced_boxes, j);
                len_left--;

                bool added = false;
                if (b.x < sb.x+sb.w && b.x+b.w > sb.x) {
                  if (sb.y > b.y && sb.y < b.y+b.h) {
                    added = true;
                    farr_add(sliced_boxes, ui_box(b.x, b.y, b.w, sb.y-b.y));
                  }
                  if (sb.y+sb.h > b.y && sb.y+sb.h < b.y+b.h) {
                    added = true;
                    farr_add(sliced_boxes, ui_box(b.x, sb.y+sb.h, b.w, b.y+b.h-(sb.y+sb.h)));
                  }
                }
                if (b.y < sb.y+sb.h && b.y+b.h > sb.y) {
                  if (sb.x > b.x && sb.x < b.x+b.w) {
                    added = true;
                    farr_add(sliced_boxes, ui_box(b.x, b.y, sb.x-b.x, b.h));
                  }
                  if (sb.x+sb.w > b.x && sb.x+sb.w < b.x+b.w) {
                    added = true;
                    farr_add(sliced_boxes, ui_box(sb.x+sb.w, b.y, b.x+b.w-(sb.x+sb.w), b.h));
                  }
                }
                if (!added) {
                  farr_add(sliced_boxes, b);
                }
              }
              break;
            }
            for (int i = 0; i < farr_len(sliced_boxes); i++) {
              Ui_Box b = sliced_boxes.data[i];
              gfx_set_color_u(cmd.color);
              gfx_fill_rect(b.x, b.y, b.w, b.h);
            }
          }
        } break;

        case UI_CMD_TEXT:
        {
          Ui_Cmd_Text cmd = {0};
          dr_read(&dr, &cmd, sizeof(cmd));
          {
            const char *str = sb_bank_get(&ui_.bank, cmd.str_id);
            gfx_set_color_u(cmd.color);
            Gfx_Text_Ex ex = {0};
            if (cmd.num_text_effects) {
              ex.effects = ui_.text_effects + cmd.text_effect_index;
              ex.num_effects = cmd.num_text_effects;
            }
            gfx_text_ex(str, cmd.x, cmd.y, ex);
          }
        } break;

        case UI_CMD_VERTICES:
        {
          Ui_Cmd_Vertices cmd = {0};
          dr_read(&dr, &cmd, sizeof(cmd));

          {
            int i = 0;
            const Ui_Vertex *verts = (const Ui_Vertex *)dr_get_front(&dr);

            gfx_imm_begin_2d();
            for (i = 0; i < cmd.num_vertices; i++) {
              gfx_imm_vertex_2d(verts[i].x, verts[i].y, verts[i].u, verts[i].v, verts[i].color);
            }
            gfx_imm_end();
            gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, NULL);

            dr_advance(&dr, cmd.num_vertices * sizeof(verts[0]));
          }
        } break;

        case UI_CMD_SCISSOR:
        {
          Ui_Cmd_Scissor cmd = {0};
          dr_read(&dr, &cmd, sizeof(cmd));

          gfx_set_scissor(cmd.box.x, cmd.box.y, cmd.box.w, cmd.box.h);
        } break;

        case UI_CMD_UNSET_SCISSOR:
        {
          gfx_unset_scissor();
        } break;

        case UI_CMD_SCOPE:
        {
          Ui_Cmd_Scope cmd = {0};
          dr_read(&dr, &cmd, sizeof(cmd));
        } break;

        case UI_CMD_BOX_CALLBACK:
        {
          Ui_Cmd_Box_Callback cmd = {0};
          dr_read(&dr, &cmd, sizeof(cmd));
          cmd.callback(cmd.box, cmd.arg);
        } break;

        default:
        {
          assert(false && "Unhandled op type.");
          break;
        }
      }
    }
  }

  {
    int i = 0;
    for (i = 0; i < arr_len(ui_.debug_shapes); i++) {
      Ui_Debug_Shape shape = ui_.debug_shapes[i];
      gfx_set_color(1,0,0,1);
      gfx_trace_rect(shape.box.x, shape.box.y, shape.box.w, shape.box.h);
    }
    arr_clear(ui_.debug_shapes);
  }

  gfx_pop();
}

