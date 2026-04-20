#include "profiler.h"
#include "app.h"
#include "basic.h"
#include "gfx.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

int profiler_frame_index = -1;
Profiler_Frame profiler_frames[PROFILER_NUMBER_FRAMES];
bool profiler_on = true;
static bool profiler_actually_on = false;
static int profiler_selected_index = false;
static int profiler_selected_entry_id = 0;

Profiler_Entry *profiler_get_entry(Profiler_Frame *frame, int id) {
  if (id > 0) {
    return &frame->list[id-1];
  }
  return NULL;
}

const char *profiler_get_entry_name(Profiler_Frame *frame, Profiler_Entry *entry) {
  return frame->string_bank.ptr + entry->name_pos;
}

double profiler_get_frame_duration(Profiler_Frame *frame) {
  double frame_duration = 0;
  for (Profiler_Entry *entry = profiler_get_entry(frame, frame->root_id);
      entry; entry = profiler_get_entry(frame, entry->next_peer_id))
  {
    // We're probably still in the middle of recording this frame.
    // Just return 0;
    if (entry->end < entry->start) {
      return 0;
    }
    frame_duration += entry->end - entry->start;
  }
  return frame_duration;
}

void profiler_begin_frame(void) {
  profiler_actually_on = profiler_on;
  if (profiler_actually_on) {
    profiler_frame_index++;
    if (profiler_frame_index >= PROFILER_NUMBER_FRAMES)
      profiler_frame_index = 0;
    Profiler_Frame *frame = &profiler_frames[profiler_frame_index];
    arr_clear(frame->list);
    sb_clear(&frame->string_bank);
    frame->root_id = 0;
    frame->cur_id = 0;
    frame->prev_id = 0;
  }
}

void profiler_begin(const char *name, const char *file, int line) {
  if (profiler_frame_index < 0) {
    // Not actually started profiler yet. Do not abort, since
    // we might want to call certain functions outside of a frame context.
    return;
  }
  if (!profiler_actually_on)
    return;

  if (profiler_frame_index == profiler_selected_index)
    profiler_selected_index = -1;

  Profiler_Frame *frame = &profiler_frames[profiler_frame_index];
  Profiler_Entry *entry = arr_add_new(frame->list);
  entry->id = (int)(entry - frame->list) + 1;
  entry->name_pos = frame->string_bank.size;
  entry->file = file;
  entry->line = line;
  entry->parent_id = frame->cur_id;

  sb_write(&frame->string_bank, name, str_size(name));
  sb_add(&frame->string_bank, '\0');

  Profiler_Entry *parent = profiler_get_entry(frame, entry->parent_id);
  Profiler_Entry *prev = profiler_get_entry(frame, frame->prev_id);
  if (parent) {
    if (!parent->first_child_id) {
      parent->first_child_id = entry->id;
    }
  }

  if (prev) {
    prev->next_peer_id = entry->id;
    frame->prev_id = 0;
  }

  if (!frame->root_id) {
    frame->root_id = entry->id;
  }

  frame->cur_id = entry->id;
  entry->start = app_time();
}

void profiler_end(void) {
  if (profiler_frame_index < 0)
    return;
  if (!profiler_actually_on)
    return;

  Profiler_Frame *frame = &profiler_frames[profiler_frame_index];
  Profiler_Entry *entry = profiler_get_entry(frame, frame->cur_id);
  assert(entry && "There is no currently open profiler scope!");
  if (!entry)
    return;

  frame->prev_id = entry->id;
  frame->cur_id = entry->parent_id;
  entry->end = app_time();

  Profiler_Entry *parent = profiler_get_entry(frame, entry->parent_id);
  if (parent) {
    parent->total_children_duration += entry->end - entry->start;
  }
}

static float profiler_get_index_x_(int index, float x, float w) {
  return x + (float)index / (float)(PROFILER_NUMBER_FRAMES - 1) * w;
}
static float profiler_get_duration_y_(float duration, float max_duration, float y, float h) {
  return y + h - (duration / max_duration) * h;
}

typedef struct {
  float op, ox, oy, ow, oh;
  float ip, ix, iy, iw, ih;
  float max_frame_duration;
} Profiler_Visualizer_;

typedef struct {
  Profiler_Visualizer_ vis;
  float ox, oy, ow, oh;
  float ix, iy, iw, ih;
  int line_index;
  bool any_hovered;
} Profiler_Visualizer_List_;

static float profiler_vis_get_line_height_() {
  return (2 + gfx_get_text_height());
}

static void profiler_vis_draw_list_recurse_(Profiler_Visualizer_List_ *list, Profiler_Frame *frame, int entry_id, int level) {
  float mx = app_mouse_x();
  float my = app_mouse_y();
  for (Profiler_Entry *entry = profiler_get_entry(frame, entry_id);
      entry; entry = profiler_get_entry(frame, entry->next_peer_id))
  {
    double duration = entry->end - entry->start;
    char duration_text[32];
    snprintf(duration_text, _countof(duration_text), "%.3f ms", duration * 1000);

    float th = profiler_vis_get_line_height_();
    float tx = list->ix;
    float tw = list->iw;
    float ty = list->iy + list->line_index * th;
    float tx0 = tx + level * gfx_get_text_width(" ")*2;
    float tw1 = gfx_get_text_width(duration_text);
    float tx1 = list->ix + tw - tw1;

    bool hovered =
        !list->any_hovered &&
        mx >= list->ix && mx <= list->ix+list->iw &&
        my >= ty && my <= ty+th;
    list->any_hovered |= hovered;

    if (profiler_selected_entry_id == entry->id) {
      gfx_set_color(0.4, 0.4, 0.1, 0.5);
      gfx_fill_rect(tx, ty, tw, th);
    }
    if (hovered) {
      gfx_set_color(0.4, 0.4, 0.4, 0.5);
      gfx_fill_rect(tx, ty, tw, th);
      if (app_keypress(KEY_MOUSE_LEFT)) {
        profiler_selected_entry_id = entry->id;
      }
    }

    gfx_set_color(1,1,1,1);
    gfx_text(profiler_get_entry_name(frame, entry), tx0, ty);
    gfx_set_color(1,1,0,1);
    gfx_text(duration_text, tx1, ty);

    list->line_index++;

    profiler_vis_draw_list_recurse_(list, frame, entry->first_child_id, level+1);
  }
}

static int profiler_frame_get_num_entries(Profiler_Frame *frame) {
  return arr_len(frame->list);
}

static void profiler_vis_draw_list_(Profiler_Visualizer_ vis) {
  if (profiler_selected_index >= 0) {
    Profiler_Frame *frame = &profiler_frames[profiler_selected_index];

    // Line in the graph
    {
      float duration = profiler_get_frame_duration(frame);
      float x = profiler_get_index_x_(profiler_selected_index, vis.ix, vis.iw);
      float y0 = profiler_get_duration_y_(duration, vis.max_frame_duration, vis.iy, vis.ih);
      float y1 = vis.iy + vis.ih;
      gfx_set_color(0,1,0,1);
      gfx_line(x,y0, x,y1);
    }

    Profiler_Visualizer_List_ list = {0};
    list.vis = vis;

    list.ox = vis.ox;
    list.oy = vis.oy + vis.oh + vis.op;
    list.ow = vis.ow;

    list.iw = list.ow - vis.ip*2;
    list.ih = profiler_frame_get_num_entries(frame) * profiler_vis_get_line_height_();
    list.ix = list.ox + vis.ip;
    list.iy = list.oy + vis.ip;

    list.oh = list.ih + vis.ip * 2;
    gfx_push();
    gfx_set_color(0,0,0,0.5);
    gfx_fill_rect(list.ox, list.oy, list.ow, list.oh);


    profiler_vis_draw_list_recurse_(&list, frame, frame->root_id, 0);
    gfx_pop();
  }
}

void profiler_draw_visualization(void) {
  gfx_push();
  Profiler_Visualizer_ vis = {0};
  //gfx_set_font(NULL);
  vis.op = 5;
  vis.ow = 400;
  vis.oh = 120;
  vis.ox = gfx_width - vis.ow - vis.op;
  vis.oy = vis.op;

  vis.ip = 5;
  vis.iw = vis.ow-vis.ip*2;
  vis.ih = vis.oh-vis.ip*2;
  vis.ix = vis.ox + vis.ip;
  vis.iy = vis.oy + vis.ip;

  gfx_set_color(0,0,0,0.5);
  gfx_fill_rect(vis.ox, vis.oy, vis.ow, vis.oh);
  gfx_set_color(1,1,1,0.5);
  gfx_line(vis.ix,vis.iy, vis.ix,vis.iy+vis.ih);
  gfx_line(vis.ix,vis.iy+vis.ih, vis.ix+vis.iw,vis.iy+vis.ih);

  Vec2 positions[_countof(profiler_frames)] = {0};
  for (int i = 0; i < _countof(profiler_frames); i++) {
    vis.max_frame_duration = MAX(vis.max_frame_duration, profiler_get_frame_duration(&profiler_frames[i]));
  }

  float mx = app_mouse_x();
  float my = app_mouse_y();
  bool mouse_in_box =
    mx >= vis.ox && mx <= vis.ox+vis.ow &&
    my >= vis.oy && my <= vis.oy+vis.oh;
  float nearest_mouse_dist_x = FLT_MAX;
  int hovered_index = -1;

  gfx_set_color(1,0,0,1);
  Vec2 prev_pos = {0};
  for (int i = 0; i < _countof(profiler_frames); i++) {
    float duration = profiler_get_frame_duration(&profiler_frames[i]);
    Vec2 pos = {0};
    pos.x = profiler_get_index_x_(i, vis.ix, vis.iw);
    pos.y = profiler_get_duration_y_(duration, vis.max_frame_duration, vis.iy, vis.ih);
    if (i != profiler_frame_index) {
      if (i > 0 && (i-1) != profiler_frame_index) {
        gfx_line(prev_pos.x, prev_pos.y, pos.x, pos.y);
      }

      if (mouse_in_box) {
        float mouse_dist_x = fabs(mx - pos.x);
        if (mouse_dist_x < nearest_mouse_dist_x) {
          nearest_mouse_dist_x = mouse_dist_x;
          hovered_index = i;
        }
      }
    }
    prev_pos = pos;
  }

  {
    float x = profiler_get_index_x_(profiler_frame_index, vis.ix, vis.iw);
    float y0 = vis.iy;
    float y1 = vis.iy + vis.ih;
    gfx_set_color(1,1,1,1);
    gfx_line(x,y0, x,y1);
  }

  profiler_vis_draw_list_(vis); 

  if (hovered_index >= 0) {
    Profiler_Frame *frame = &profiler_frames[hovered_index];
    float duration = profiler_get_frame_duration(frame);

    float x = profiler_get_index_x_(hovered_index, vis.ix, vis.iw);
    float y0 = vis.iy;
    float y1 = vis.iy + vis.ih;
    float yc = profiler_get_duration_y_(duration, vis.max_frame_duration, vis.iy, vis.ih);
    gfx_set_color(1,1,0,0.3);
    gfx_line(x,y0, x,yc);
    gfx_set_color(1,1,0,1);
    gfx_line(x,yc, x,y1);

    char text[32];
    snprintf(text, _countof(text), "%.3f ms", duration * 1000);
    gfx_set_color(1,1,0,1);
    float text_x = x + vis.ip;
    if (hovered_index > PROFILER_NUMBER_FRAMES/2) {
      float text_width = gfx_get_text_width(text);
      text_x = x - text_width - vis.ip;
    }
    gfx_text(text, text_x, vis.iy);

    if (app_keypress(KEY_MOUSE_LEFT)) {
      profiler_selected_index = hovered_index;
      profiler_selected_entry_id = 0;
    }
  }

  gfx_pop();
}

#ifdef __cplusplus
} // extern "C"
#endif
