#include "gfx.h"
#include "basic.h"

#include <math.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#define GFX_INLINE __forceinline
#else
#define GFX_INLINE inline
#endif

#define GFX_NEW(T) ((T *)calloc(1, sizeof(T)))
#define GFX_OFFSET_OF(T, member) ((size_t)(&((T *)0)->member))
#define GFX_OFFSET_OF_Obj(obj, member) ((size_t)&((obj)->member) - (size_t)(obj))

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#include "gfx_default_font.h"

#ifdef __cplusplus
extern "C" {
#endif

//========================= Font =============================
static void unload_font(Font *font);

bool font_is_valid(Font *font) {
  return !!font->texture;
}

static void add_glyph(Font *font, Font_Glyph g) {
  if (font->glyph_count >= font->glyph_capacity) {
    font->glyph_capacity = font->glyph_capacity ? font->glyph_capacity * 2 : 256;
    font->glyphs = (Font_Glyph *)malloc(font->glyph_capacity * sizeof(Font_Glyph));
  }
  font->glyphs[font->glyph_count++] = g;
}

bool font_load_ttf(Font *font, int font_size, const void *ttf_data, size_t ttf_size) {
  stbtt_pack_context ctx = {0};
  int width = 1024;
  int height = 1024;
  unsigned char *buf = NULL;

  unload_font(font);

  buf = (unsigned char *)calloc(width, height);
  if (!buf)
    return 0;

  if (1 == stbtt_PackBegin(&ctx, buf, width, height, 0, 1, NULL)) {
    const char codepoints[] = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789,.<>/?'~\"`!@#$%^&*()-=_+\\|[]{};:\t";
    int codepoints_i[_countof(codepoints)] = {0};
    stbtt_fontinfo info = {0};
    int ascent = 0, descent = 0, line_gap = 0;
    float em_scale = 0;
    int y_offset = 0;

    stbtt_packedchar char_data[_countof(codepoints_i)] = {0};
    stbtt_pack_range range = {0};

    stbtt_PackSetOversampling(&ctx, 1, 1);
    stbtt_InitFont(&info, (const unsigned char *)ttf_data, 0);
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);

    em_scale = stbtt_ScaleForPixelHeight(&info, (float)font_size);
    y_offset = (int)(em_scale * (ascent));
    
    { int i; for (i = 0; i < _countof(codepoints); i++) codepoints_i[i] = codepoints[i]; }

    range.font_size = (float)font_size;
    range.first_unicode_codepoint_in_range = 0;
    range.array_of_unicode_codepoints = codepoints_i;
    range.num_chars = _countof(codepoints_i);
    range.chardata_for_range = char_data;

    if (1 == stbtt_PackFontRanges(&ctx, (const unsigned char *)ttf_data, 0, &range, 1)) {
      unsigned char *new_buf = 0;
      int i = 0;
      for (i = 0; i < _countof(codepoints); i++) {
        Font_Glyph g = {0};
        g.quad[0] = char_data[i].x0;
        g.quad[1] = char_data[i].y0;
        g.quad[2] = (float)char_data[i].x1 - (float)char_data[i].x0;
        g.quad[3] = (float)char_data[i].y1 - (float)char_data[i].y0;
        g.x = char_data[i].xoff;
        g.y = y_offset + char_data[i].yoff;
        g.advance = char_data[i].xadvance;
        g.code = codepoints_i[i];
        add_glyph(font, g);
      }

      font->height = (float)font_size;
      font->scale = 1;
      new_buf = (unsigned char *)malloc(width * height * 4);
      for (i = 0; i < width * height; i++) {
        new_buf[i*4+0] = 255;
        new_buf[i*4+1] = 255;
        new_buf[i*4+2] = 255;
        new_buf[i*4+3] = buf[i];
      }
      font->texture = gfx_new_texture(new_buf, width, height);
      free(new_buf);
    }

    stbtt_PackEnd(&ctx);
  }

  free(buf);
  return 1;
}

void font_load_image(Font *font, const unsigned char *pixels, int w, int h, const char *characters, int spacing) {
  int last_left = 0;
  const char *c = NULL;
  int x = 0;

  unload_font(font);

  font->scale = 1;
  for (x = 0; x < w; x++) {
    unsigned char a = 0;
    if (c && !*c)
      break;

    // First row of the image is just the delimiter.
    a = pixels[x*4 + 3];
    if (a != 0) {
      if (c) {
        Font_Glyph g = {0};
        g.code = *c;
        g.quad[0] = (float)last_left+1;
        g.quad[1] = 1;
        g.quad[2] = (float)x - (float)last_left - 1;
        g.quad[3] = (float)h-1;
        g.advance = g.quad[2] + spacing;
        add_glyph(font, g);
        c++;
      }
      else {
        c = characters;
      }
      last_left = x;
    }
  }

  font->texture = gfx_new_texture(pixels, w, h);
  font->height = (float)font->texture->height-1;
}

Font_Glyph *font_find_glyph(Font *font, int c) {
  int i = 0;
  for (i = 0; i < font->glyph_count; i++)
    if (font->glyphs[i].code == c)
      return &font->glyphs[i];
  return NULL;
}


float font_get_height(Font *font) {
  return font->height * font->scale;
}

bool font_has_glyph(Font *font, int c) {
  return font_find_glyph(font, c) != NULL;
}

static bool font_property_match_(const char *text, int length, int *ptr_i, const char *match_str) {
  size_t i = (size_t)*ptr_i;
  size_t match_str_len = strlen(match_str);
  if (i + match_str_len <= length) {
    if (!memcmp(text+i, match_str, match_str_len)) {
      *ptr_i = (int)(i + match_str_len);
      return 1;
    }
  }
  return 0;
}

static bool font_eat_hex_byte_(const char *text, int i, int length, uint32_t *out_result) {
  uint32_t result = 0;
  int j = 0;
  for (j = 0; j < 2; j++,i++) {
    uint32_t digit = 0;
    if (i >= length)
      return 0;
    if (text[i] >= '0' && text[i] <= '9') {
      digit = text[i] - '0';
    } else if (text[i] >= 'a' && text[i] <= 'f') {
      digit = 10 + text[i] - 'a';
    } else if (text[i] >= 'A' && text[i] <= 'F') {
      digit = 10 + text[i] - 'A';
    } else {
      return 0;
    }

    {
      const int shift_amount = (j ^ 1)*4;
      result |= digit << shift_amount;
    }
  }
  *out_result = result;
  return 1;
}

void gfx_uint32_to_float4(uint32_t col, float v[4]) {
  v[3] = (float)((col & (0xff << 24)) >> 24) / 255.f;
  v[2] = (float)((col & (0xff << 16)) >> 16) / 255.f;
  v[1] = (float)((col & (0xff <<  8)) >>  8) / 255.f;
  v[0] = (float)((col & (0xff <<  0)) >>  0) / 255.f;
}

uint32_t gfx_uint32_set_alpha(uint32_t color, float alpha) {
  float v[4] = {0};
  gfx_uint32_to_float4(color, v);
  v[3] = alpha;
  return gfx_color(v[0], v[1], v[2], v[3]);
}

float gfx_uint32_get_alpha(uint32_t color) {
  float v[4] = {0};
  gfx_uint32_to_float4(color, v);
  return v[3];
}

// Returns 1 if reached the end of text
static bool font_eat_properties_(const char *text, int length, int *ptr_i, Text_Properties *props) {
  int i = *ptr_i;

  while (i < length) {
    int original_i = i;
    bool failed = 0;
    int local_original_i = i;

    if (font_property_match_(text, length, &i, "<#")) {
      uint32_t color = 0x00000000;
      int j = 0;
      for (j = 0; i < length ; j++,i+=2) {
        uint32_t component = 0;
        if (j == 3 && text[i] == '>') {
          component = 0xff;
        } else if (!font_eat_hex_byte_(text, i, length, &component)) {
          failed = 1;
          break;
        }

        {
          int shift_amount = j*8;
          color |= component << shift_amount;
        }

        if (j+1 >= 4) {
          break;
        }
      }

      if (!failed) {
        props->has_color = 1;
        props->color = color;
        while (i < length && text[i] != '\n') {
          if (text[i] == '>') {
            i++;
            break;
          }
          i++;
        }
      }
    } else if (font_property_match_(text, length, &i, "<>")) {
      props->has_color = 0;
      props->color = 0xffffffff;
    } else {
      break;
    }

    if (failed) {
      i = original_i;
      break;
    }
  }

  *ptr_i = i;
  return i >= length;
}

// Returns true if reached the end of text
bool gfx_skip_text_part_properties(const char *text, int length, int *ptr_i) {
  Text_Properties props = {0};
  return font_eat_properties_(text, length, ptr_i, &props);
}

void gfx_scroll_text(const char *text, int text_len, float char_time, float dt, int *progress, float *char_timer) {
  if (gfx_skip_text_part_properties(text, text_len, progress)) {
    return;
  }
  *char_timer += dt; 
  while (*char_timer >= char_time && *progress < text_len) {
    if (gfx_skip_text_part_properties(text, text_len, progress)) {
      break;
    }
    *char_timer -= char_time;
    (*progress)++;
  }
}

Font_Text_Size font_get_text_size_part(Font *font, const char *text, int length) {
  Text_Properties props = {0};

  float width = 0;
  float cur_width = 0;
  float height = font_get_height(font);
  int num_lines = 1;
  int i = 0;
  for (i = 0; i < length; i++) {
    char c = 0;
    if (font_eat_properties_(text, length, &i, &props))
      break;

    c = text[i];
    if (c == '\n') {
      width = cur_width > width ? cur_width : width;
      num_lines++;
      continue;
    }

    {
      Font_Glyph *g = font_find_glyph(font, c);
      if (g) {
        cur_width += g->advance;
      }
    }
  }

  {
    Font_Text_Size ret = {0};
    ret.width  = ceilf(cur_width) * font->scale;
    ret.height = num_lines * font_get_height(font);

    return ret;
  }
}

float font_get_height_text_part(Font *font, const char *text, int length) {
  return font_get_text_size_part(font, text, length).height;
}

float font_get_width_part(Font *font, const char *text, int length) {
  return font_get_text_size_part(font, text, length).width;
}

float font_get_width(Font *font, const char *text) {
  if (!text) return 0;
  return font_get_width_part(font, text, (int)strlen(text));
}
float font_get_gap(Font *font) {
  return 0;
}

void font_draw_text_cb(Font *font, const char *str, void *user, Font_Draw_Cb *cb) {
  int length;
  if (!str) return;
  length = (int)strlen(str);
  font_draw_text_part_cb(font, str, length, user, cb);
}

void font_draw_text_part_cb(Font *font, const char *str, int length, void *user, Font_Draw_Cb *cb) {
  Text_Properties props = {0};

  float scale = font->scale;
  float tx = 0;
  float ty = 0;
  int i = 0;

  uint32_t default_color = gfx_get_color_u();
  for (i = 0; i < length; i++) {
    char c = 0;
    if (font_eat_properties_(str, length, &i, &props))
      break;

    c = str[i];
    if (c == '\n') {
      tx = 0;
      ty += font_get_height(font);
      continue;
    }

    {
      Font_Glyph *g = font_find_glyph(font, c);
      if (g) {
        if (c != ' ') {
          uint32_t color = props.has_color ? props.color : default_color;
          color = gfx_uint32_set_alpha(color, gfx_uint32_get_alpha(default_color));
          cb(user, tx+g->x, ty+g->y, g->quad[2]*scale, g->quad[3]*scale, g->quad,
            color);
        }
        tx += g->advance;
      }
    }
  }
}

void font_imm_text_part_options(Font *font, const char *str, int length, float x, float y, Text_Options opts) {

  int i = 0;
  Text_Properties props = {0};
  if (opts.text_properties)
    props = *opts.text_properties;
  float tx = x, ty = y;
  const uint32_t default_color = gfx_get_color_u();

  tx = floor(tx);
  ty = floor(ty);

  for (i = 0; i < length; i++) {
    char c = 0;
    if (font_eat_properties_(str, length, &i, &props))
      break;

    c = str[i];
    if (c == '\n') {
      tx = 0;
      ty += font_get_height(font);
      continue;
    }

    {
      const float inv_tw = 1.f / font->texture->width;
      const float inv_th = 1.f / font->texture->height;
      Font_Glyph *g = font_find_glyph(font, c);
      if (g) {
        uint32_t color = 0;
        const float *q = g->quad;
        const float u0 = q[0] * inv_tw;
        const float v0 = q[1] * inv_th;
        const float u1 = (q[0]+q[2]) * inv_tw;
        const float v1 = (q[1]+q[3]) * inv_th;
        const float x0 = tx + g->x * font->scale;
        const float y0 = ty + g->y * font->scale;
        const float x1 = x0 + g->quad[2] * font->scale;
        const float y1 = y0 + g->quad[3] * font->scale;

        color = default_color;
        if (opts.use_color) {
          color = opts.color;
        }
        if (!opts.ignore_inline_props && props.has_color) {
          float alpha = uint32_get_alpha(color);
          color = props.color;
          color = uint32_set_alpha(color, alpha);
        }

        gfx_imm_vertex_2d(x0, y0, u0, v0, color);
        gfx_imm_vertex_2d(x1, y0, u1, v0, color);
        gfx_imm_vertex_2d(x1, y1, u1, v1, color);

        gfx_imm_vertex_2d(x0, y0, u0, v0, color);
        gfx_imm_vertex_2d(x1, y1, u1, v1, color);
        gfx_imm_vertex_2d(x0, y1, u0, v1, color);

        tx += g->advance * font->scale;
      }
    }
  }

  if (opts.text_properties)
    *opts.text_properties = props;
}

void font_imm_text_part(Font *font, const char *str, int length, float x, float y) {
  Text_Options options = {0};
  font_imm_text_part_options(font, str, length, x, y, options);
}

static void font_imm_line_(Font *font, int real_i, const char *ptr, int line_length, float tx, float ty, uint32_t default_color, bool force_default_color, Text_Properties *props, const Font_Text_Effect *effects, int num_effects) {
  for (int i = 0; i < line_length; i++) {
    if (font_eat_properties_(ptr, line_length, &i, props))
      break;

    char c = ptr[i];
    {
      const float inv_tw = 1.f / font->texture->width;
      const float inv_th = 1.f / font->texture->height;
      Font_Glyph *g = font_find_glyph(font, c);
      if (g) {
        const float *q = g->quad;
        const float u0 = q[0] * inv_tw;
        const float v0 = q[1] * inv_th;
        const float u1 = (q[0]+q[2]) * inv_tw;
        const float v1 = (q[1]+q[3]) * inv_th;
        const float x0 = tx + g->x * font->scale;
        const float y0 = ty + g->y * font->scale;
        const float x1 = x0 + g->quad[2] * font->scale;
        const float y1 = y0 + g->quad[3] * font->scale;

        uint32_t color = default_color;
        if (!force_default_color) {
          if (props->has_color) {
            color = props->color;
          }

          for (int j = 0; j < num_effects; j++) {
            if (real_i+i >= effects[j].begin &&
                real_i+i < effects[j].end)
            {
              color = effects[j].color;
            }
          }
        }
        color = gfx_uint32_set_alpha(color, gfx_uint32_get_alpha(default_color));

        gfx_imm_vertex_2d(x0, y0, u0, v0, color);
        gfx_imm_vertex_2d(x1, y0, u1, v0, color);
        gfx_imm_vertex_2d(x1, y1, u1, v1, color);

        gfx_imm_vertex_2d(x0, y0, u0, v0, color);
        gfx_imm_vertex_2d(x1, y1, u1, v1, color);
        gfx_imm_vertex_2d(x0, y1, u0, v1, color);

        tx += g->advance * font->scale;
      }
    }
  }
}

#define GFX_TEXT_LINE_PROC_(NAME) void NAME(const char *ptr, int line_start, int line_len, float x, float y, Gfx_Text_Ex ex, Text_Properties props, void *user_data)
typedef GFX_TEXT_LINE_PROC_(Gfx_Text_Line_Proc_);

static void gfx_font_for_text_line_ex_(Font *font, const char *text, float x, float y, Gfx_Text_Ex ex, Gfx_Text_Line_Proc_ *proc, void *user_data) {
  if (!text)
    return;

  int length = ex.text_length;
  if (!length)
    length = strlen(text);

  if (ex.centered) {
    if (ex.wrap_width) {
      x -= ex.wrap_width/2;
    }
    else {
      x -= font_get_width(font, text)/2;
    }
  }

  x = floor(x);
  y = floor(y);

  const char *ptr = text;
  const char *ptr_end = text + length;
  if (ptr == ptr_end)
    return;

  Text_Properties props = {0};

  float total_height = 0;
  bool end_early = false;
  while (1) {
    int line_start = ptr - text;
    int remaining_length = ptr_end - ptr;
    assert(remaining_length && "Should not get here when there's no remaining length.");

    int line_length = remaining_length;
    if (ex.wrap_width > 0) {
      line_length = font_find_next_line_break_part(font, ptr, ptr_end-ptr, (int)ex.wrap_width, ex.wrap_by_character);
      // If we couldn't find line lenght at all, then it means it's all one
      // word. Just include the whole string.
      if (line_length <= 0) {
        if (!ex.wrap_by_character)
          line_length = remaining_length;
        else
          line_length = 1;
      }
    }

    int display_line_length = line_length;
    if (ex.use_progress) {
      const char *line_end_ptr = ptr + line_length;
      if ((line_end_ptr - text) >= ex.text_progress) {
        display_line_length = text + ex.text_progress - ptr;
        if (ptr >= text+ex.text_progress) {
          end_early = true;
        }
      }
    }

    bool should_draw = !end_early;
    if (should_draw) {
      proc(ptr, line_start, display_line_length, x, y, ex, props, user_data);
    }

    ptr += line_length;

    float line_height = ex.force_line_height;
    if (line_height <= 0)
      line_height = font_get_height(font);

    if (total_height)
      total_height += ex.line_gap;
    total_height += line_height;

    y += line_height + ex.line_gap;

    if (ptr >= ptr_end)
      break;
  }

  if (ex.out_total_height) {
    *ex.out_total_height = total_height;
  }
}

//==============================
// Draw routine
static GFX_TEXT_LINE_PROC_( font_draw_text_ex2_line_proc_ ) {
  Font *font = (Font *)user_data;
  if (ex.bordered) {
    Text_Properties props_copy = props;
    bool force_default_color = true;
    uint32_t border_color = gfx_color_web(ex.border_color);
    border_color = uint32_set_alpha(border_color, gfx_get_alpha());
    font_imm_line_(font, line_start, ptr, line_len, x-1, y-1, border_color, force_default_color, &props_copy, NULL, 0);
    font_imm_line_(font, line_start, ptr, line_len, x+1, y+1, border_color, force_default_color, &props_copy, NULL, 0);
    font_imm_line_(font, line_start, ptr, line_len, x-1, y+1, border_color, force_default_color, &props_copy, NULL, 0);
    font_imm_line_(font, line_start, ptr, line_len, x+1, y-1, border_color, force_default_color, &props_copy, NULL, 0);

    font_imm_line_(font, line_start, ptr, line_len, x-1, y, border_color, force_default_color, &props_copy, NULL, 0);
    font_imm_line_(font, line_start, ptr, line_len, x+1, y, border_color, force_default_color, &props_copy, NULL, 0);
    font_imm_line_(font, line_start, ptr, line_len, x, y-1, border_color, force_default_color, &props_copy, NULL, 0);
    font_imm_line_(font, line_start, ptr, line_len, x, y+1, border_color, force_default_color, &props_copy, NULL, 0);
  }
  uint32_t color = gfx_get_color_u();
  bool force_default_color = false;
  font_imm_line_(font, line_start, ptr, line_len, x, y, color, force_default_color, &props, ex.effects, ex.num_effects);
}

void font_draw_text_ex2(Font *font, const char *text, float x, float y, Gfx_Text_Ex ex) {
  gfx_push();
  gfx_imm_begin_2d();

  gfx_font_for_text_line_ex_(font, text, x, y, ex, font_draw_text_ex2_line_proc_, font);

  gfx_imm_end();
  gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, font->texture);

  gfx_pop();
}

void font_draw_text_ex(Font *font, const char *text, float x, float y, Gfx_Text_Ex ex) {
  if (!text)
    return;
  int length = ex.text_length;
  if (!length)
    length = strlen(text);

  const char *ptr = text;
  const char *ptr_end = text + length;

  Text_Properties props = {0};

  gfx_push();
  gfx_imm_begin_2d();
  while (1) {
    int line_length = ptr_end - ptr;
    if (ex.wrap_width > 0) {
      line_length = font_find_next_line_break_part(font, ptr, ptr_end-ptr, (int)ex.wrap_width, ex.wrap_by_character);
    }

    bool end_early = false;
    if (ex.use_progress) {
      const char *line_end_ptr = ptr + line_length;
      if ((line_end_ptr - text) >= ex.text_progress) {
        line_length = text + ex.text_progress - ptr;
        end_early = true;
      }
    }

    Text_Options opts = {0};
    opts.text_properties = &props;
    if (!ex.bordered) {
      font_draw_text_part_options(font, ptr, line_length, x, y, opts);
    }
    else {
      font_draw_text_part_bordered_options(font, ptr, line_length, (int)x, (int)y, opts);
    }
    ptr += line_length;
    y += font_get_height(font) + ex.line_gap;

    if (ptr >= ptr_end || end_early)
      break;
  }
  gfx_imm_end();
  gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, font->texture);

  gfx_pop();
}


void font_draw_text_part_options(Font *font, const char *str, int length, float x, float y, Text_Options opts) {
  gfx_push();
  gfx_imm_begin_2d();
  font_imm_text_part_options(font, str, length, x, y, opts);
  gfx_imm_end();
  gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, font->texture);
  gfx_pop();
}

void font_draw_text_part(Font *font, const char *str, int length, float x, float y) {
  Text_Options opts = {0};
  font_draw_text_part_options(font, str, length, x, y, opts);
}

bool font_get_quad(Font *font, int c, float quad[4]) {
  Font_Glyph *g = font_find_glyph(font, c);
  if (!g)
    return 0;
  quad[0] = g->quad[0];
  quad[1] = g->quad[1];
  quad[2] = g->quad[2];
  quad[3] = g->quad[3];
  return 1;
}

void font_draw_text(Font *font, const char *str, float x, float y) {
  if (!str) return;
  font_draw_text_part(font, str, (int)strlen(str), x, y);
}

static bool is_white_space(char c) {
  return c == ' ' || c == '\n';
}
int font_find_next_line_break(Font *font, const char *text, int width_limit) {
  if (!text) return 0;
  int len = (int)strlen(text);
  return font_find_next_line_break_part(font, text, len, width_limit, false);
}

int font_find_next_line_break_part(Font *font, const char *text, int length, int width_limit, bool wrap_by_character) {
  float width_limit_f = (float)width_limit;
  float total_width = 0;
  int last_word_begin = -1;
  int i = 0;
  Text_Properties props = {0};

  if (!text) return 0;
  for (i = 0; i < length; i++) {
    char c = 0;
    Font_Glyph *g = 0;
    float cw = 0;
    if (font_eat_properties_(text, length, &i, &props))
      break;

    c = text[i];
    g = font_find_glyph(font, c);
    if (g)
      cw = g->advance * font->scale;

    if (c == '\n') {
      return i + 1;
    }

    if (wrap_by_character) {
      if (total_width + cw > width_limit_f) {
        return i;
      }
    }
    else {
      bool was_space = i <= 0 || is_white_space(text[i-1]);
      if (!is_white_space(c)) {
        if (total_width + cw > width_limit_f && last_word_begin != -1)
          return last_word_begin;
        if (was_space)
          last_word_begin = i;
      }
    }

    total_width += cw;
  }

  return length;
}

int gfx_get_text_length_ignore_properties(const char *text) {
  int ret = 0;
  for (int i = 0, len = str_len(text); i < len; i++) {
    if (gfx_skip_text_part_properties(text, len, &i)) {
      break;
    }
    ret++;
  }
  return ret;
}

void font_draw_text_part_bordered_options(Font *font, const char *text, int size, int x, int y, Text_Options options) {
  float color[4] = {0};
  float off = 0;
  gfx_get_color(color);

  off = (float)font->scale;
  if (off < 1) {
    off = 1;
  }

  gfx_push();
  gfx_imm_begin_2d();
  gfx_set_color(0,0,0, color[3]);
  Text_Options options_copy = options;
  options_copy.text_properties = NULL;
  options_copy.ignore_inline_props = true;
  font_imm_text_part_options(font, text, size, (float)x-off, (float)y, options_copy);
  font_imm_text_part_options(font, text, size, (float)x+off, (float)y, options_copy);
  font_imm_text_part_options(font, text, size, (float)x, (float)y-off, options_copy);
  font_imm_text_part_options(font, text, size, (float)x, (float)y+off, options_copy);

  gfx_set_color_p(color);
  options_copy = options;
  font_imm_text_part_options(font, text, size, (float)x, (float)y, options_copy);
  gfx_imm_end();

  gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, font->texture);
  gfx_pop();
}

void font_draw_text_part_bordered(Font *font, const char *text, int size, int x, int y) {
  Text_Options options = {0};
  font_draw_text_part_bordered_options(font, text, size, x, y, options);
}

void font_draw_text_bordered(Font *font, const char *text, int x, int y) {
  font_draw_text_part_bordered(font, text, (int)strlen(text), x, y);
}

static void unload_font(Font *font) {
  if (font->texture) {
    gfx_free_texture(font->texture);
    free(font->glyphs);
  }
  memset(font, 0, sizeof(*font));
}

//==================== Matrix math ======================
static GFX_INLINE void gfx_matrix_identity_(float t[16]) {
  t[0]  = 1; t[1]  = 0; t[2]  = 0; t[3]  = 0;
  t[4]  = 0; t[5]  = 1; t[6]  = 0; t[7]  = 0;
  t[8]  = 0; t[9]  = 0; t[10] = 1; t[11] = 0;
  t[12] = 0; t[13] = 0; t[14] = 0; t[15] = 1;
}
static GFX_INLINE void gfx_matrix_translate_(float t[16], float tx, float ty, float tz) {
  t[12] += t[0]*tx + t[4]*ty + t[8 ]*tz;
  t[13] += t[1]*tx + t[5]*ty + t[9 ]*tz;
  t[14] += t[2]*tx + t[6]*ty + t[10]*tz;
}
static GFX_INLINE void gfx_matrix_scale_(float t[16], float sx, float sy, float sz) {
  t[0]  *= sx;
  t[4]  *= sy;
  t[8]  *= sz;

  t[1]  *= sx;
  t[5]  *= sy;
  t[9]  *= sz;

  t[2]  *= sx;
  t[6]  *= sy;
  t[10] *= sz;
}
static GFX_INLINE void gfx_matrix_rotate(float t[16], float r) {
  r = r / 180 * 3.1415926f;
  {
    float Sin = sinf(r);
    float Cos = cosf(r);
    float t0 = t[0];
    float t4 = t[4];
    float t1 = t[1];
    float t5 = t[5];
    t[0] = t0* Cos + t4*Sin;
    t[4] = t0*-Sin + t4*Cos;
    t[1] = t1* Cos + t5*Sin;
    t[5] = t1*-Sin + t5*Cos;
  }
}

// 0 4  8 12
// 1 5  9 13
// 2 6 10 14
// 3 7 11 15

static GFX_INLINE void gfx_matrix_rotate_z_(float t[16], float r) {
  r = r / 180.f * 3.1415926f;

  {
    float Sin = sinf(r);
    float Cos = cosf(r);

    float x = Cos;
    float y = -Sin;
    float z = Sin;
    float w = Cos;

    float a = t[0];
    float b = t[4];
    float e = t[1];
    float f = t[5];
    float i = t[2];
    float j = t[6];

    t[0] = a*x+b*z;
    t[1] = e*x+f*z;
    t[2] = i*x+j*z;

    t[4] = a*y+b*w;
    t[5] = e*y+f*w;
    t[6] = i*y+j*w;
  }
}

static GFX_INLINE void gfx_matrix_rotate_x_(float t[16], float r) {
  r = r / 180.f * 3.1415926f;

  {
    float Sin = sinf(r);
    float Cos = cosf(r);

    float x = Cos;
    float y = -Sin;
    float z = Sin;
    float w = Cos;

    float b = t[4];
    float c = t[8];
    float f = t[5];
    float g = t[9];
    float j = t[6];
    float k = t[10];

    t[4]  = b*x+c*z;
    t[5]  = f*x+g*z;
    t[6]  = j*x+k*z;

    t[8]  = b*y+c*w;
    t[9]  = f*y+g*w;
    t[10] = j*y+k*w;
  }
}

static GFX_INLINE void matrix_rotate_y(float t[16], float r) {
  r = r / 180.f * 3.1415926f;

  {
    float Sin = sinf(r);
    float Cos = cosf(r);

    float x = Cos;
    float y = Sin;
    float z = -Sin;
    float w = Cos;

    float a = t[0];
    float c = t[8];
    float e = t[1];
    float g = t[9];
    float i = t[2];
    float k = t[10];

    t[0]  = a*x+c*z;
    t[1]  = e*x+g*z;
    t[2]  = i*x+k*z;

    t[8]  = a*y+c*w;
    t[9]  = e*y+g*w;
    t[10] = i*y+k*w;
  }
}

static GFX_INLINE void gfx_matrix_multiply(float t[16], const float a[16], const float b[16]) {
  t[0 ] = a[0 ]*b[0 ] + a[4 ]*b[1 ] + a[8 ]*b[2 ] + a[12]*b[3 ];
  t[1 ] = a[1 ]*b[0 ] + a[5 ]*b[1 ] + a[9 ]*b[2 ] + a[13]*b[3 ];
  t[2 ] = a[2 ]*b[0 ] + a[6 ]*b[1 ] + a[10]*b[2 ] + a[14]*b[3 ];
  t[3 ] = a[3 ]*b[0 ] + a[7 ]*b[1 ] + a[11]*b[2 ] + a[15]*b[3 ];

  t[4 ] = a[0 ]*b[4 ] + a[4 ]*b[5 ] + a[8 ]*b[6 ] + a[12]*b[7 ];
  t[5 ] = a[1 ]*b[4 ] + a[5 ]*b[5 ] + a[9 ]*b[6 ] + a[13]*b[7 ];
  t[6 ] = a[2 ]*b[4 ] + a[6 ]*b[5 ] + a[10]*b[6 ] + a[14]*b[7 ];
  t[7 ] = a[3 ]*b[4 ] + a[7 ]*b[5 ] + a[11]*b[6 ] + a[15]*b[7 ];

  t[8 ] = a[0 ]*b[8 ] + a[4 ]*b[9 ] + a[8 ]*b[10] + a[12]*b[11];
  t[9 ] = a[1 ]*b[8 ] + a[5 ]*b[9 ] + a[9 ]*b[10] + a[13]*b[11];
  t[10] = a[2 ]*b[8 ] + a[6 ]*b[9 ] + a[10]*b[10] + a[14]*b[11];
  t[11] = a[3 ]*b[8 ] + a[7 ]*b[9 ] + a[11]*b[10] + a[15]*b[11];

  t[12] = a[0 ]*b[12] + a[4 ]*b[13] + a[8 ]*b[14] + a[12]*b[15];
  t[13] = a[1 ]*b[12] + a[5 ]*b[13] + a[9 ]*b[14] + a[13]*b[15];
  t[14] = a[2 ]*b[12] + a[6 ]*b[13] + a[10]*b[14] + a[14]*b[15];
  t[15] = a[3 ]*b[12] + a[7 ]*b[13] + a[11]*b[14] + a[15]*b[15];
}
static GFX_INLINE void gfx_matrix_mulv(float t[4], const float m[16], const float v[4]) {
  t[0 ] = m[0 ]*v[0 ] + m[4 ]*v[1 ] + m[8 ]*v[2 ] + m[12]*v[3 ];
  t[1 ] = m[1 ]*v[0 ] + m[5 ]*v[1 ] + m[9 ]*v[2 ] + m[13]*v[3 ];
  t[2 ] = m[2 ]*v[0 ] + m[6 ]*v[1 ] + m[10]*v[2 ] + m[14]*v[3 ];
  t[3 ] = m[3 ]*v[0 ] + m[7 ]*v[1 ] + m[11]*v[2 ] + m[15]*v[3 ];
}
static GFX_INLINE void gfx_matrix_projection_2d(float t[16], int is_upright, int width, int height) {
  int upright = is_upright ? 1 : -1;
  float a = 1.0f / ((float)width / 2);
  float b = -1.0f / ((float)height / 2) * upright;

  memset(t, 0, 16 * sizeof(float));
  t[0] = 1;
  t[5] = 1;
  t[10] = 1;
  t[15] = 1;

  t[0]  = a;  // X scale
  t[12] = -1; // X offset

  t[5] = b;               // Y scale
  t[13] = (float)upright; // Y offset

  t[10] = 2;  // Z scale
  t[14] = -1; // Z offset
}
static GFX_INLINE void gfx_matrix_projection_perspective(float t[16], float fov_x_deg, float fov_y_deg, float near_, float far_, float zdir) {
  float px = 1.f / tanf(fov_x_deg * 0.5f * 3.1415926f / 180.f);
  float py = 1.f / tanf(fov_y_deg * 0.5f * 3.1415926f / 180.f);
  zdir = (zdir ? zdir : 1);
  {
    float pz1 = zdir * (far_ + near_) / (far_ - near_);
    float pz2 = -2   * (far_ * near_) / (far_ - near_);
    float pw = zdir;
    memset(t, 0, 16 * sizeof(float));
    t[0]  = px;
    t[5]  = py;
    t[10] = pz1;
    t[11] = pw;
    t[14] = pz2;
  }
}

void gfx_matrix_mul_v(float r[4], const float mat[16], const float i[4]) {
  float input[4] = { i[0], i[1], i[2], i[3] };
  gfx_matrix_mulv(r, mat, input);
}
void gfx_matrix_translate(float mat[16], float x, float y, float z) {
  gfx_matrix_translate_(mat, x,y,z);
}
void gfx_matrix_identity(float mat[16]) {
  gfx_matrix_identity_(mat);
}
void gfx_matrix_rotate_x(float mat[16], float r) {
  gfx_matrix_rotate_x_(mat, r);
}
void gfx_matrix_rotate_y(float mat[16], float r) {
  matrix_rotate_y(mat, r);
}
void gfx_matrix_rotate_z(float mat[16], float r) {
  gfx_matrix_rotate_z_(mat, r);
}
void gfx_matrix_scale(float mat[16], float sx, float sy, float sz) {
  gfx_matrix_scale_(mat, sx,sy,sz);
}

static GFX_INLINE void gfx_matrix_projection_orthographic(float t[16], float width, float height, float near_z, float far_z, float zdir) {
  gfx_matrix_identity_(t);
  t[0 ] = 2.f / width;
  t[5 ] = 2.f / height;
  t[10] = zdir * 2.f / (far_z - near_z);
  t[14] = -(far_z + near_z) / (far_z - near_z);
}

static bool gfx_supports_compute_;

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#include <GL/gl.h>

#ifdef _WIN64
typedef signed   long long int khronos_intptr_t;
typedef signed   long long int khronos_ssize_t;
#else
typedef signed   long  int     khronos_intptr_t;
typedef signed   long  int     khronos_ssize_t;
#endif
typedef khronos_ssize_t GLsizeiptr;
typedef khronos_intptr_t GLintptr;
typedef char GLchar;

#define GL_FRAMEBUFFER                    0x8D40
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_STATIC_DRAW                    0x88E4
#define GL_COMPILE_STATUS                 0x8B81
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_LINK_STATUS                    0x8B82
#define GL_TEXTURE0                       0x84C0
#define GL_VERTEX_SHADER                  0x8B31
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_ACTIVE_UNIFORMS                0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH      0x8B87
#define GL_FLOAT_VEC2                     0x8B50
#define GL_FLOAT_VEC4                     0x8B52
#define GL_FLOAT_MAT4                     0x8B5C
#define GL_SAMPLER_2D                     0x8B5E
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B
#define GL_RENDERBUFFER                   0x8D41
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_UNSIGNED_INT_24_8              0x84FA
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_DEPTH_STENCIL                  0x84F9
#define GL_RGB16F                         0x881B
#define GL_HALF_FLOAT                     0x140B
#define GL_TEXTURE_CUBE_MAP               0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X    0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X    0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y    0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y    0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z    0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z    0x851A
#define GL_TEXTURE_3D                     0x806F
#define GL_NUM_EXTENSIONS                 0x821D

#define GL_R32F                           0x822E
#define GL_RGBA8I                         0x8D8E
#define GL_RGBA8UI                        0x8D7C
#define GL_R32I                           0x8235
#define GL_RGB32I                         0x8D83
#define GL_RGBA32I                        0x8D82
#define GL_RED_INTEGER                    0x8D94
#define GL_RGB_INTEGER                    0x8D98
#define GL_RGBA_INTEGER                   0x8D99

#define GL_TEXTURE_COMPARE_MODE           0x884C

#define GL_MIRRORED_REPEAT                0x8370
#define GL_CLAMP_TO_BORDER                0x812D

#define GL_FRAMEBUFFER_SRGB               0x8DB9

static void (APIENTRY *glGenVertexArrays)(GLsizei n,
  	GLuint *arrays);
static void (APIENTRY *glBindVertexArray)(GLuint array);

static void (APIENTRY *glBlendFuncSeparate)(GLenum srcRGB,
   GLenum dstRGB,
   GLenum srcAlpha,
   GLenum dstAlpha);
static void (APIENTRY *glBindFramebuffer)(GLenum target,
   GLuint framebuffer);
static void (APIENTRY *glGenBuffers)(GLsizei n,
   GLuint * buffers);
static void (APIENTRY *glBindBuffer)(GLenum target,
   GLuint buffer);
static void (APIENTRY *glDeleteBuffers)(GLsizei n, const GLuint *buffers);
static void (APIENTRY *glBufferData)(GLenum target,
   GLsizeiptr size,
   const GLvoid * data,
   GLenum usage);
static void (APIENTRY *glBufferSubData)(GLenum target,
   GLintptr offset,
   GLsizeiptr size,
   const GLvoid * data);
static void (APIENTRY *glUseProgram)(GLuint program);
static void (APIENTRY *glVertexAttribPointer)(GLuint index,
   GLint size,
   GLenum type,
   GLboolean normalized,
   GLsizei stride,
   const GLvoid * pointer);
static GLuint (APIENTRY *glCreateShader)(GLenum shaderType);
static void (APIENTRY *glShaderSource)(GLuint shader,
   GLsizei count,
   const GLchar **string,
   const GLint *length);
static void (APIENTRY *glCompileShader)(GLuint shader);
static void (APIENTRY *glGetShaderiv)(GLuint shader,
   GLenum pname,
   GLint *params);
static void (APIENTRY *glGetShaderInfoLog)(GLuint shader,
   GLsizei maxLength,
   GLsizei *length,
   GLchar *infoLog);
static void (APIENTRY *glDeleteShader)(GLuint shader);
static void (APIENTRY *glEnableVertexAttribArray)(GLuint index);
static void (APIENTRY *glActiveTexture)(GLenum texture);
static GLuint (APIENTRY *glCreateProgram)(void);
static void (APIENTRY *glAttachShader)(GLuint program,
   GLuint shader);
static void (APIENTRY *glLinkProgram)(GLuint program);
static void (APIENTRY *glGetProgramiv)(GLuint program,
   GLenum pname,
   GLint *params);
static void (APIENTRY *glGetProgramInfoLog)(GLuint program,
   GLsizei maxLength,
   GLsizei *length,
   GLchar *infoLog);
static void (APIENTRY *glDeleteProgram)(GLuint program);
static GLint (APIENTRY *glGetAttribLocation)(GLuint program,
   const GLchar *name);
static void (APIENTRY *glGetActiveUniform)(GLuint program,
   GLuint index,
   GLsizei bufSize,
   GLsizei *length,
   GLint *size,
   GLenum *type,
   GLchar *name);
static GLint (APIENTRY *glGetUniformLocation)(GLuint program,
   const GLchar *name);
static void (APIENTRY *glGenFramebuffers)(GLsizei n,
   GLuint *ids);
static void (APIENTRY *glDeleteFramebuffers)(GLsizei n, const GLuint *framebuffers);
static const GLubyte *(APIENTRY *glGetStringi)(GLenum name,
 	GLuint index);

typedef void (APIENTRY *DEBUGPROC)(GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const void *userParam);
static void (APIENTRY *glFramebufferTexture2D)(GLenum target,
   GLenum attachment,
   GLenum textarget,
   GLuint texture,
   GLint level);

static void (APIENTRY *glUniform1fv)(GLint location, GLsizei count, const GLfloat *value);
static void (APIENTRY *glUniform2fv)(GLint location, GLsizei count, const GLfloat *value);
static void (APIENTRY *glUniform3fv)(GLint location, GLsizei count, const GLfloat *value);
static void (APIENTRY *glUniform4fv)(GLint location, GLsizei count, const GLfloat *value);
static void (APIENTRY *glUniform1i)(GLint location, GLint v0);
static void (APIENTRY *glUniform1iv)(GLint location, GLsizei count, const GLint *value);
//static void (APIENTRY *glUniform1uiv)(GLint location, GLsizei count, const GLuint *value);
static void (APIENTRY *glUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
static void (APIENTRY *glUniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

static void (APIENTRY *glDebugMessageCallback)(DEBUGPROC callback,
   const void * userParam);

static void (APIENTRY *glGenRenderbuffers)(GLsizei n, GLuint *renderbuffers);
static void (APIENTRY *glBindRenderbuffer)(GLenum target, GLuint renderbuffer);
static void (APIENTRY *glRenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
static void (APIENTRY *glFramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);

static void (APIENTRY *glDrawBuffers)(GLsizei n, const GLenum *bufs);

//============ Compute stuff ====================
static void (APIENTRY *glDispatchCompute)(GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);

#define load_gl_function(name) \
  *(void **)&name = (void *)gfx_load_gl_function(#name); \
  if (!name) { \
    fprintf(stderr, "Failed to load GL function %s\n", #name); \
    assert(name && "Failed to load gl function"); \
  }

static void gfx_load_gl_functions(void *(*gfx_load_gl_function)(const char *name)) {
  load_gl_function(glBlendFuncSeparate);
  load_gl_function(glBindFramebuffer);
  load_gl_function(glGenBuffers);
  load_gl_function(glBindBuffer);
  load_gl_function(glDeleteBuffers);
  load_gl_function(glBufferData);
  load_gl_function(glBufferSubData);
  load_gl_function(glUseProgram);
  load_gl_function(glCreateShader);
  load_gl_function(glShaderSource);
  load_gl_function(glCompileShader);
  load_gl_function(glGetShaderiv);
  load_gl_function(glGetShaderInfoLog);
  load_gl_function(glVertexAttribPointer);
  load_gl_function(glEnableVertexAttribArray);
  load_gl_function(glDeleteShader);
  load_gl_function(glActiveTexture);
  load_gl_function(glAttachShader);
  load_gl_function(glLinkProgram);
  load_gl_function(glCreateProgram);
  load_gl_function(glGetProgramiv);
  load_gl_function(glGetProgramInfoLog);
  load_gl_function(glDeleteProgram);
  load_gl_function(glGetAttribLocation);
  load_gl_function(glGetActiveUniform);
  load_gl_function(glGetUniformLocation);
  load_gl_function(glGenFramebuffers);
  load_gl_function(glDeleteFramebuffers);
  load_gl_function(glGetStringi);
  load_gl_function(glFramebufferTexture2D);
  load_gl_function(glUniform1fv);
  load_gl_function(glUniform2fv);
  load_gl_function(glUniform4fv);
  load_gl_function(glUniform1i);
  load_gl_function(glUniformMatrix4fv);
  load_gl_function(glUniformMatrix4x3fv);
  load_gl_function(glUniform3fv);
  load_gl_function(glUniform1iv);

  load_gl_function(glDebugMessageCallback);

  load_gl_function(glGenRenderbuffers);
  load_gl_function(glBindRenderbuffer);
  load_gl_function(glRenderbufferStorage);
  load_gl_function(glFramebufferRenderbuffer);
  load_gl_function(glDrawBuffers);

  gfx_supports_compute_ = gfx_load_gl_function("glDispatchCompute") != NULL;
  if (gfx_supports_compute_) {
    load_gl_function(glDispatchCompute);
  }

  if (1) {
    load_gl_function(glBindVertexArray)
    load_gl_function(glGenVertexArrays)
  }
}

#elif defined(__EMSCRIPTEN__)
#include <emscripten/html5.h>
#include <emscripten/emscripten.h>
#include <GLES3/gl3.h>

static void gfx_load_gl_functions(void *(*gfx_load_gl_function)(const char *name)) {}

#endif

typedef struct Gfx_Immed_Buffer_ {
  size_t capacity_in_bytes;
  size_t size_in_bytes;
  size_t gpu_capacity_in_bytes;
  unsigned num_vertices_flushed;
  void *data;

  int vertex_size;
  Gfx_Vertex_Attribute *vertex_attrs;
  GLuint handle;
  GLuint vao;
} Gfx_Immed_Buffer_;

typedef struct Gfx_Mat4_ {
  float data[16];
} Gfx_Mat4_;

typedef struct Gfx_Vertex_Buffer_Info_ {
  Gfx_Vertex_Buffer id;
  int vertex_size;
  int num_vertices;
  int index_size;
  int num_indices;
  GLuint vbo, ebo, vao;
  Gfx_Vertex_Attribute *vertex_attributes;
} Gfx_Vertex_Buffer_Info_;

typedef enum Gfx_Projection_Type_ {
  PROJECTION_2D,
  PROJECTION_PERSPECTIVE,
  PROJECTION_ORTHOGRAPHIC,
} Gfx_Projection_Type_;

typedef struct Gfx_Projection_Info_ {
  Gfx_Projection_Type_ type;
  struct {
    float fov_x_deg;
    float fov_y_deg;
    float near_plane;
    float far_plane;
  } perspective;

  struct {
    float width, height, near_z, far_z;
  } Orthographic;
} Gfx_Projection_Info_;

typedef struct Gfx_State_ {
  Gfx_Mat4_ transform;
  Gfx_Mat4_ view;
  Gfx_Canvas *canvas;
  Gfx_Shader *shader;
  Font *font;
  uint8_t blend_mode;
  uint8_t stencil_value;
  uint8_t canvas_slice;
  Gfx_Stencil_Op stencil_op;
  bool use_scissor  : 1;
  bool write_depth  : 1;
  bool is_wireframe : 1;
  bool use_viewport : 1;
  Gfx_Culling_Type culling_type : 4;
  Gfx_Depth_Mode depth_mode     : 4;
  float scissor[4];
  float color[4];
  float depth_2d;
  float viewport[4];
  Gfx_Projection_Info_ projection_info;
} Gfx_State_;

static int cur_blend_mode = -1;
static int cur_depth_mode = -1;
static int cur_culling_type = -1;
static bool cur_culling_has_canvas = false;
static bool cur_write_depth;
static bool cur_is_wireframe;
static bool cur_use_scissor;
static Gfx_Stencil_Op cur_stencil_op;
static uint8_t cur_stencil_value;

static Gfx_Shader *default_shader;
static Gfx_Texture *dummy_texture;

typedef struct Gfx_ {
  Gfx_State_ state_stack[64];
  int state_cursor;

  Font *default_font;
  Gfx_Vertex_Buffer_Info_ *vertex_buffers;

  const char **extensions;
  int num_extensions;
} Gfx_;

static Gfx_Immed_Buffer_ immed_buf;
static Gfx_ gfx_;
int gfx_width;
int gfx_height;

#define gfx_state_() gfx_.state_stack[gfx_.state_cursor]


static void gfx_log(const char *fmt) {
  fprintf(stderr, "[GFX] ");
  fprintf(stderr, "%s\n", fmt);
}

static void gfx_ibuf_init(Gfx_Immed_Buffer_ *buf, int init_capacity_in_bytes) {
  GLuint handle;
  memset(buf, 0, sizeof(*buf));
  buf->capacity_in_bytes = init_capacity_in_bytes;
  buf->data = malloc(buf->capacity_in_bytes);
  buf->gpu_capacity_in_bytes = init_capacity_in_bytes;
  glGenBuffers(1, &buf->handle);
  glBindBuffer(GL_ARRAY_BUFFER, buf->handle);
  glBufferData(GL_ARRAY_BUFFER, buf->capacity_in_bytes, NULL, GL_DYNAMIC_DRAW);
  if (glGenVertexArrays)
    glGenVertexArrays(1, &buf->vao);
}

static void gfx_ibuf_add(Gfx_Immed_Buffer_ *buf, const void *vertex, int vertex_size_in_bytes) {
  buf->vertex_size = vertex_size_in_bytes;
  if (buf->size_in_bytes + vertex_size_in_bytes >= buf->capacity_in_bytes) {
    size_t new_capacity_in_bytes = (buf->size_in_bytes + vertex_size_in_bytes) * 2;
    buf->data = realloc(buf->data, new_capacity_in_bytes);
    buf->capacity_in_bytes = new_capacity_in_bytes;
  }
  memcpy((char *)buf->data + buf->size_in_bytes, vertex, vertex_size_in_bytes);
  buf->size_in_bytes += vertex_size_in_bytes;
}

static void gfx_ibuf_flush_impl(Gfx_Immed_Buffer_ *buf, const void *data, size_t size) {
  glBindBuffer(GL_ARRAY_BUFFER, buf->handle);
  if (buf->size_in_bytes > buf->gpu_capacity_in_bytes)  {
    buf->gpu_capacity_in_bytes = buf->capacity_in_bytes;
    glBufferData(GL_ARRAY_BUFFER, buf->gpu_capacity_in_bytes, buf->data, GL_DYNAMIC_DRAW);
  }
  else {
    glBufferSubData(GL_ARRAY_BUFFER, 0, buf->size_in_bytes, buf->data);
  }
}

static void gfx_ibuf_flush(Gfx_Immed_Buffer_ *buf) {
  gfx_ibuf_flush_impl(buf, buf->data, buf->size_in_bytes);
  buf->num_vertices_flushed = buf->size_in_bytes / buf->vertex_size;
  buf->size_in_bytes = 0;
}

static void gfx_ibuf_reset(Gfx_Immed_Buffer_ *buf, int vertex_size, const Gfx_Vertex_Attribute *attrs, int num_attrs) {
  arr_clear(buf->vertex_attrs);
  for (int i = 0; i < num_attrs; i++) {
    arr_add(buf->vertex_attrs, attrs[i]);
  }
  buf->vertex_size = vertex_size;
  buf->size_in_bytes = 0;
}

void gfx_clear_depth(float depth) {
#ifdef __EMSCRIPTEN__
  glClearDepthf(depth);
#else
  glClearDepth(depth);
#endif
  glClear(GL_DEPTH_BUFFER_BIT);
}

void gfx_set_depth_write(bool write) {
  gfx_state_().write_depth = write;
  if (cur_write_depth != write) {
    cur_write_depth = write;
    if (write)
      glDepthMask(GL_TRUE);
    else
      glDepthMask(GL_FALSE);
  }
}

static void gfx_update_stencil(void) {
  if (cur_stencil_op != gfx_state_().stencil_op ||
      cur_stencil_value != gfx_state_().stencil_value)
  {
    cur_stencil_op    = gfx_state_().stencil_op;
    cur_stencil_value = gfx_state_().stencil_value;
    switch (cur_stencil_op) {
    case GFX_STENCIL_OP_NONE:
      glDisable(GL_STENCIL_TEST);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      break;
    case GFX_STENCIL_OP_EQUAL_WRITE_STENCIL_INCREMENT:
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_EQUAL, cur_stencil_value, 0xff);
      glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
      break;
    case GFX_STENCIL_OP_EQUAL_WRITE_STENCIL_ZERO:
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_EQUAL, cur_stencil_value, 0xff);
      glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
      break;
    case GFX_STENCIL_OP_EQUAL_WRITE_STENCIL:
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_EQUAL, cur_stencil_value, 0xff);
      glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
      break;
    case GFX_STENCIL_OP_ALWAYS_WRITE:
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS, cur_stencil_value, 0xff);
      glStencilOp(GL_REPLACE, GL_KEEP, GL_REPLACE);
      break;
    case GFX_STENCIL_OP_EQUAL:
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      glEnable(GL_STENCIL_TEST);
      glStencilFunc(GL_EQUAL, cur_stencil_value, 0xff);
      glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      break;
    }
  }
}

void gfx_clear_stencil(uint8_t value) {
  glClearStencil(value);
  glClear(GL_STENCIL_BUFFER_BIT);
}
void gfx_set_stencil(Gfx_Stencil_Op stencil_op, uint8_t stencil_value) {
  gfx_state_().stencil_op    = stencil_op;
  gfx_state_().stencil_value = stencil_value;
  gfx_update_stencil();
}

void gfx_set_depth_test(Gfx_Depth_Mode mode) {
  gfx_state_().depth_mode = mode;
  if (cur_depth_mode == mode)
    return;

  cur_depth_mode = mode;
  if (mode == GFX_DEPTH_NONE) {
    glDisable(GL_DEPTH_TEST);
  }
  else {
    glEnable(GL_DEPTH_TEST);
    switch (mode) {
    case GFX_DEPTH_NONE:
      break;
    case GFX_DEPTH_LT:
      glDepthFunc(GL_LESS);
      break;
    case GFX_DEPTH_LE:
      glDepthFunc(GL_LEQUAL);
      break;
    case GFX_DEPTH_GT:
      glDepthFunc(GL_GREATER);
      break;
    case GFX_DEPTH_GE:
      glDepthFunc(GL_GEQUAL);
      break;
    case GFX_DEPTH_ALWAYS:
      glDepthFunc(GL_ALWAYS);
      break;
    }
  }
}

void gfx_set_color_255(float r, float g, float b, float a) {
  gfx_set_color(r/255.f, g/255.f, b/255.f, a/255.f);
}

void gfx_set_color_p(const float *color_arg) {
  gfx_state_().color[0] = color_arg[0];
  gfx_state_().color[1] = color_arg[1];
  gfx_state_().color[2] = color_arg[2];
  gfx_state_().color[3] = color_arg[3];
}

void gfx_set_color_u(uint32_t col) {
  gfx_uint32_to_float4(col, gfx_state_().color);
}

void gfx_set_color_web(uint32_t col) {
  gfx_set_color_u(gfx_color_web(col));
}

void gfx_set_color_ignore_alpha_u(uint32_t col) {
  gfx_state_().color[2] = (float)((col & (0xff << 16)) >> 16) / 255.f;
  gfx_state_().color[1] = (float)((col & (0xff <<  8)) >>  8) / 255.f;
  gfx_state_().color[0] = (float)((col & (0xff <<  0)) >>  0) / 255.f;
}

void gfx_mult_alpha(float alpha) {
  gfx_state_().color[3] *= alpha;
}

static float gfx_clamp01(float x) {
  return x < 0 ? 0 : (x > 1 ? 1 : x);
}
void gfx_set_color(float r, float g, float b, float a) {
  gfx_state_().color[0] = gfx_clamp01(r);
  gfx_state_().color[1] = gfx_clamp01(g);
  gfx_state_().color[2] = gfx_clamp01(b);
  gfx_state_().color[3] = gfx_clamp01(a);
}

void gfx_set_alpha(float a) {
  gfx_state_().color[3] = a;
}

float gfx_get_alpha(void) {
  return gfx_state_().color[3];
}

void gfx_get_color(float out_color[4]) {
  out_color[0] = gfx_state_().color[0];
  out_color[1] = gfx_state_().color[1];
  out_color[2] = gfx_state_().color[2];
  out_color[3] = gfx_state_().color[3];
}

uint32_t gfx_get_color_u(void) {
  return gfx_color(
      gfx_state_().color[0],
      gfx_state_().color[1],
      gfx_state_().color[2],
      gfx_state_().color[3]);
}

uint32_t gfx_color_web(uint32_t c) {
  return gfx_color3_rev(c);
}
uint32_t gfx_color3_rev(uint32_t c) {
  c = (c << 8) | 0xff;
  return gfx_endian_swap_(c);
}
uint32_t gfx_color(float r, float g, float b, float a) {
  return
    ((uint32_t)(a * 255.0f) << 24) |
    ((uint32_t)(b * 255.0f) << 16) |
    ((uint32_t)(g * 255.0f) << 8 ) |
    ((uint32_t)(r * 255.0f) << 0 );
}

static void gfx_update_scissor(void) {
  cur_use_scissor = gfx_state_().use_scissor;
  if (cur_use_scissor) {
    float height = (float)gfx_height;
    float width  = (float)gfx_width;
    bool upright = 1;
    GLint h,y,x,w;

    glEnable(GL_SCISSOR_TEST);
    if (gfx_state_().canvas) {
      height = (float)gfx_state_().canvas->height;
      width  = (float)gfx_state_().canvas->width;
      upright = 0;
    }

    h = (GLint)gfx_state_().scissor[3];
    if (h < 0) h = 0;

    y = (GLint)height - (GLint)(h+gfx_state_().scissor[1]);
    if (!upright) {
      y = (GLint)gfx_state_().scissor[1];
    }
    x = (GLint)gfx_state_().scissor[0];
    w = (GLint)gfx_state_().scissor[2];

    if (x < 0) {
      w += x; x = 0;
    }
    if (x + w > width) {
      w = (GLint)(width - x);
    }
    if (y < 0) {
      h += y; y = 0;
    }
    if (y + h > height) {
      h = (GLint)(height - y);
    }
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    glScissor(x, y, w, h);
  }
  else {
    glDisable(GL_SCISSOR_TEST);
  }
}

void gfx_set_scissor(float x, float y, float w, float h) {
  gfx_state_().use_scissor = 1;
  gfx_state_().scissor[0] = x;
  gfx_state_().scissor[1] = y;
  gfx_state_().scissor[2] = w;
  gfx_state_().scissor[3] = h;
  gfx_update_scissor();
}

void gfx_set_scissor_p(const float s[4]) {
  gfx_set_scissor(s[0], s[1], s[2], s[3]);
}

void gfx_set_2d_projection() {
  gfx_state_().projection_info.type = PROJECTION_2D;
}

void gfx_set_orthographic_projection(float width, float height, float near_z, float far_z) {
  gfx_state_().projection_info.type = PROJECTION_ORTHOGRAPHIC;
  gfx_state_().projection_info.Orthographic.width  = width;
  gfx_state_().projection_info.Orthographic.height = height;
  gfx_state_().projection_info.Orthographic.near_z = near_z;
  gfx_state_().projection_info.Orthographic.far_z  = far_z;
}

void gfx_set_3d_projection(float fov_x_deg, float fov_y_deg, float near_plane, float far_plane) {
  gfx_state_().projection_info.type = PROJECTION_PERSPECTIVE;
  gfx_state_().projection_info.perspective.fov_x_deg = fov_x_deg;
  gfx_state_().projection_info.perspective.fov_y_deg = fov_y_deg;
  gfx_state_().projection_info.perspective.near_plane = near_plane;
  gfx_state_().projection_info.perspective.far_plane  = far_plane;
}

void gfx_set_all_2d(void) {
  gfx_reset_view();
  gfx_set_2d_projection();
  gfx_set_depth_test(GFX_DEPTH_NONE);
  gfx_set_depth_write(0);
}

static void gfx_get_projection_(float t[16]) {

  Gfx_Canvas *canvas = gfx_state_().canvas;
  bool upright = !canvas;
  switch (gfx_state_().projection_info.type) {
    case PROJECTION_2D:
    {
      int width  = canvas ? canvas->width : gfx_width;
      int height = canvas ? canvas->height : gfx_height;

      gfx_matrix_projection_2d(t, upright ? 1 : 0, width, height);
    } break;

    case PROJECTION_PERSPECTIVE:
    {
      gfx_matrix_projection_perspective(t,
          gfx_state_().projection_info.perspective.fov_x_deg,
          gfx_state_().projection_info.perspective.fov_y_deg * (upright ? 1 : -1),
          gfx_state_().projection_info.perspective.near_plane,
          gfx_state_().projection_info.perspective.far_plane,
          1
        );
    } break;

    case PROJECTION_ORTHOGRAPHIC:
    {
      gfx_matrix_projection_orthographic(t,
          gfx_state_().projection_info.Orthographic.width,
          gfx_state_().projection_info.Orthographic.height * (upright ? 1 : -1),
          gfx_state_().projection_info.Orthographic.near_z,
          gfx_state_().projection_info.Orthographic.far_z,
          1
        );
    } break;
  }
}

void gfx_get_projection(float t[16]) {
  gfx_get_projection_(t);
}

void gfx_get_view_projection(float t[16]) {
  float proj[16];
  gfx_get_projection_(proj);
  gfx_matrix_multiply(t, proj, gfx_state_().view.data);
}

void gfx_get_view(float t[16]) {
  memcpy(t, gfx_state_().view.data, sizeof(float)*16);
}

void gfx_get_transform(float t[16]) {
  memcpy(t, gfx_state_().transform.data, sizeof(float)*16);
}

void gfx_unset_scissor() {
  gfx_state_().use_scissor = 0;
  gfx_update_scissor();
}

void gfx_set_texture_data(Gfx_Texture *texture, const void *data, int width, int height) {
  GLenum gl_format = GL_RGBA;
  GLenum gl_internal_format = GL_RGBA;
  GLenum gl_type = GL_UNSIGNED_BYTE;
  glBindTexture(GL_TEXTURE_2D, (GLuint)texture->handle);
  glTexImage2D(GL_TEXTURE_2D, 0, gl_format, width, height, 0, gl_internal_format, gl_type, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);
}

static GLenum get_target_from_texture_dimension(Gfx_Texture_Dimension dimension) {
  GLenum target = GL_TEXTURE_2D;
  switch (dimension) {
  case GFX_TEXTURE_DIMENSION_2D:
    target = GL_TEXTURE_2D;
    break;
  case GFX_TEXTURE_DIMENSION_CUBE:
    target = GL_TEXTURE_CUBE_MAP;
    break;
  }
  return target;
}

static Gfx_Texture gfx_make_texture_impl(Gfx_Texture_Dimension dimension, const void *data, int width, int height, int depth, Gfx_Pixel_Format pixel_format) {
  GLenum gl_format = 0;
  GLenum gl_internal_format = 0;
  GLenum gl_type = 0;
  bool is_depth_stencil = 0;
  GLenum target;
  GLuint handle;

  switch (pixel_format) {
  case GFX_PIXEL_FORMAT_NONE:
  default:
  case GFX_PIXEL_FORMAT_RGBA8:
    gl_format = GL_RGBA;
    gl_internal_format = GL_RGBA;
    gl_type = GL_UNSIGNED_BYTE;
    break;
#if 0 // FIXME: Emscripten
  case GFX_PIXEL_FORMAT_R8:
    gl_format = GL_RED;
    gl_internal_format = GL_RED;
    gl_type = GL_UNSIGNED_BYTE;
    break;
#endif
  case GFX_PIXEL_FORMAT_R32F:
    gl_format = GL_RED;
    gl_internal_format = GL_R32F;
    gl_type = GL_FLOAT;
    break;
  case GFX_PIXEL_FORMAT_DEPTH16F:
    gl_format          = GL_DEPTH_COMPONENT;
    gl_internal_format = GL_DEPTH_COMPONENT16;
    gl_type            = GL_UNSIGNED_SHORT;
    is_depth_stencil = 1;
    break;
#if GFX_ENABLE_DEPTH24F
  case GFX_PIXEL_FORMAT_DEPTH24F:
    gl_format          = GL_DEPTH_COMPONENT;
    gl_internal_format = GL_DEPTH_COMPONENT24;
    gl_type            = GL_UNSIGNED_SHORT;
    is_depth_stencil = 1;
    break;
#endif
  case GFX_PIXEL_FORMAT_DEPTH24F_STENCIL8:
    gl_format          = GL_DEPTH_STENCIL;
    gl_internal_format = GL_DEPTH24_STENCIL8;
    gl_type            = GL_UNSIGNED_INT_24_8;
    is_depth_stencil = 1;
    break;
  }

  target = get_target_from_texture_dimension(dimension);
  handle = 0;
  glGenTextures(1, &handle);
  glBindTexture(target, handle);

  switch (dimension) {
  default:
  {
    assert(false && "ERROR: Unsupported texture dimension!");
    Texture ret = {0};
    return ret;
  } break;
  case GFX_TEXTURE_DIMENSION_2D:
    glTexImage2D(target, 0, gl_internal_format, width, height, 0, gl_format, gl_type, data);
    break;
  case GFX_TEXTURE_DIMENSION_CUBE:
  {
    int i = 0;
    void **faces = (void **)data;
    for (i = 0; i < 6; i++) {
      const void *face = faces ? faces[i] : NULL;
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, gl_internal_format, width, height, 0, gl_format, gl_type, face);
    }
  } break;
  }

  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  if (dimension == GFX_TEXTURE_DIMENSION_CUBE) {
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  else {
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }

#if 0 // FIXME: Emscripten
  if (is_depth_stencil) {
    glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
  }
#endif

  glBindTexture(target, 0);

  {
    Gfx_Texture t = {0};
    t.dimension = dimension;
    t.handle = handle;
    t.width = width;
    t.height = height;
    t.pixel_format = pixel_format;
    return t;
  }
}

static GLenum get_gl_filter_mode(Gfx_Filter_Mode mode) {
  switch(mode) {
    case GFX_FILTER_MODE_LINEAR:  return GL_LINEAR;
    case GFX_FILTER_MODE_NEAREST: return GL_NEAREST;
  }
  return GL_LINEAR;
}

void gfx_set_filter_mode(Gfx_Texture *texture, Gfx_Filter_Mode min_mode, Gfx_Filter_Mode mag_mode) {
  GLenum target = get_target_from_texture_dimension(texture->dimension);
  glBindTexture(target, (GLuint)texture->handle);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, get_gl_filter_mode(min_mode));
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, get_gl_filter_mode(mag_mode));
  glBindTexture(target, 0);
}

static GLenum get_gl_wrap_mode(Gfx_Wrap_Mode mode) {
  switch(mode) {
    case GFX_WRAP_MODE_CLAMP:           return GL_CLAMP_TO_EDGE;
    case GFX_WRAP_MODE_REPEAT:          return GL_REPEAT;
    case GFX_WRAP_MODE_MIRRORED_REPEAT: return GL_MIRRORED_REPEAT;
#if 0 // FIXME: Emscripten
    case GFX_WRAP_MODE_BORDER:          return GL_CLAMP_TO_BORDER;
#endif
  }
  return GL_CLAMP_TO_EDGE;
}

void gfx_set_wrap(Gfx_Texture *texture, Gfx_Wrap_Mode x_mode, Gfx_Wrap_Mode y_mode) {
  GLenum target = get_target_from_texture_dimension(texture->dimension);
  glActiveTexture(GL_TEXTURE0); // So we don't trample other textures that have been bound to shaders.
  glBindTexture(target, (GLuint)texture->handle);
  glTexParameteri(target, GL_TEXTURE_WRAP_S, get_gl_wrap_mode(x_mode));
  glTexParameteri(target, GL_TEXTURE_WRAP_T, get_gl_wrap_mode(y_mode));
  glBindTexture(target, 0);
}

Gfx_Texture *gfx_new_texture(const void *data, int width, int height) {
  return gfx_new_texture_format(data, width, height, GFX_PIXEL_FORMAT_RGBA8);
}

Gfx_Texture *gfx_new_cube_texture(const void *pixels[6], int face_width, int face_height) {
  return gfx_new_cube_texture_format(pixels, face_width, face_height, GFX_PIXEL_FORMAT_RGBA8);
}

Gfx_Texture *gfx_new_cube_texture_format(const void *pixels[6], int face_width, int face_height, Gfx_Pixel_Format format) {
  Gfx_Texture t = gfx_make_texture_impl(GFX_TEXTURE_DIMENSION_CUBE, pixels, face_width, face_height, 1, format);
  Gfx_Texture *result;
  if (!t.handle)
    return NULL;
  result = GFX_NEW(Gfx_Texture);
  if (!result)
    return NULL;
  *result = t;
  return result;
}

Gfx_Texture *gfx_new_texture_format(const void *data, int width, int height, Gfx_Pixel_Format format) {
  Gfx_Texture t = gfx_make_texture_impl(GFX_TEXTURE_DIMENSION_2D, data, width, height, 1, format);
  Gfx_Texture *result;
  if (!t.handle)
    return NULL;
  result = GFX_NEW(Gfx_Texture);
  if (!result)
    return NULL;
  *result = t;
  return result;
}

static void gfx_free_texture_impl_(Gfx_Texture *t) {
  if (t->handle) {
    GLuint handle = (GLuint)t->handle;
    glDeleteTextures(1, &handle);
    t->handle = 0;
  }
  memset(t, 0, sizeof(*t));
}

void gfx_free_texture(Gfx_Texture *t) {
  gfx_free_texture_impl_(t);
  free(t);
}

static GLuint gfx_make_gl_shader(GLenum type, const char *src, int size) {
  const char *original_src = src;
#if defined(__EMSCRIPTEN__)
  const char *prefix = "";
#else
  const char *prefix = "#version 130\n#line 0\n";
#endif
  size_t prefix_len = strlen(prefix);
  size_t concat_len = prefix_len + size;
  char *preprocessed_src = (char *)malloc(concat_len+1);
  memcpy(preprocessed_src, prefix, prefix_len);
  memcpy(preprocessed_src + prefix_len, src, size);
  preprocessed_src[concat_len] = 0;
  src = preprocessed_src;
  size = (int)concat_len;

  GLuint handle = glCreateShader(type);
  glShaderSource(handle, 1, &src, NULL);
  glCompileShader(handle);

  GLint status = 0;
  glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
  if (!status) {
    char *buffer = 0;
    GLsizei out_size = 0;
    GLint length = 0;
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);
    buffer = (char *)malloc(length+1);
    glGetShaderInfoLog(handle, length+1, &out_size, buffer);
    fprintf(stderr, "Failed to compile shader:\n\n");
    fprintf(stderr, "%.*s\n\n", size, original_src);
    fprintf(stderr, "ERROR :\n%s\n", buffer);
    glDeleteShader(handle);
    return 0;
  }

  free(preprocessed_src);
  return handle;
}

void gfx_delete_shader(Gfx_Shader *shader) {
  if (!shader)
    return;
  GLuint program = (GLuint)shader->handle;
  glDeleteProgram(program);
  free(shader);
}

static Gfx_Shader *gfx_make_shader_impl(const char *vs_src, int vs_size, const char *ps_src, int ps_size) {
  GLuint vs = 0;
  GLuint ps = 0;
  GLuint handle = 0;

  vs = gfx_make_gl_shader(GL_VERTEX_SHADER, vs_src, vs_size);
  if (vs) {
    GLint status;
    handle = glCreateProgram();
    glAttachShader(handle, vs);
    if (ps_size) {
      ps = gfx_make_gl_shader(GL_FRAGMENT_SHADER, ps_src, ps_size);
      if (ps) {
        glAttachShader(handle, ps);
      }
    }

    glLinkProgram(handle);

    status = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    if (!status) {
      GLint length = 0;
      char *buffer = 0;
      GLsizei out_size = 0;
      glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &length);
      buffer = (char *)malloc(length+1);
      glGetProgramInfoLog(handle, length+1, &out_size, buffer);
      fprintf(stderr, "%s\n", buffer);
      glDeleteProgram(handle);
      handle = 0;
    }
  }

  if (vs) glDeleteShader(vs);
  if (ps) glDeleteShader(ps);

  if (!handle)
    return NULL;

  {
    Gfx_Shader *s = GFX_NEW(Gfx_Shader);
    for (int i = 0; i < GFX_VERTEX_ATTRIBUTE_COUNT; i++) {
      char name[10];
      snprintf(name, _countof(name), "attr%d", i);
      s->attributes[i] = glGetAttribLocation(handle, name);
    }
    s->handle = handle;
    s->uniform_loc_matrix  = glGetUniformLocation(handle, "vs_matrix");
    s->uniform_loc_proj    = glGetUniformLocation(handle, "vs_proj");
    s->uniform_loc_view    = glGetUniformLocation(handle, "vs_view");
    s->uniform_loc_texture = glGetUniformLocation(handle, "ps_texture");

    return s;
  }
}

Gfx_Shader *gfx_new_default_shader(void) {
  const char ps[] = "\n"
    "precision highp float;\n" // NOTE: This newline is necessary
    "uniform sampler2D ps_texture;"

    "varying vec2 interp_uv;"
    "varying vec4 interp_color;"

    "void main(void) {"
    "  vec4 tex_color = interp_color * texture2D(ps_texture, interp_uv);"
    "  vec4 final_color = vec4(tex_color.rgb, tex_color.a);"
    "  if (abs(final_color.a - 0.0) < 0.001)"
    "    discard;"
    "  gl_FragColor = final_color;"
    "}"
  "";

  return gfx_new_shader(ps, sizeof(ps)-1);
}

Gfx_Shader *gfx_new_shader(const char *src, size_t size) {
  const char vs[] = "\n"
    "precision highp float;\n"

    "attribute vec3 attr0; /* pos */"
    "attribute vec2 attr1; /* uv */"
    "attribute vec2 attr2; /* uv2 */"
    "attribute vec4 attr3; /* color */"
    "attribute vec3 attr4; /* normal */"
    "attribute vec3 attr5; /* tangent */"
    "struct Vertex {"
    "  vec3 pos; vec2 uv; vec2 uv2; vec4 color; vec3 normal; vec3 tangent;"
    "};"
    "Vertex gfx_get_vertex() {"
    "  Vertex ret;"
    "  ret.pos = attr0;"
    "  ret.uv = attr1;"
    "  ret.uv2 = attr2;"
    "  ret.color = attr3;"
    "  ret.normal = attr4;"
    "  ret.tangent = attr5;"
    "  return ret;"
    "}"

    "uniform mat4 vs_matrix;"
    "uniform mat4 vs_view;"
    "uniform mat4 vs_proj;"

    "varying vec2 interp_uv;"
    "varying vec2 interp_uv2;"
    "varying vec4 interp_color;"
    "varying vec3 interp_world_pos;"
    "varying vec3 interp_view_pos;"
    "varying vec3 interp_world_tangent;"
    "varying vec3 interp_world_normal;"

    "void main() {"
    "  Vertex vert = gfx_get_vertex();"
    "  interp_uv  = vert.uv;"
    "  interp_uv2 = vert.uv2;"
    "  interp_color = vert.color;"
    "  interp_world_pos = (vs_matrix * vec4(vert.pos, 1.0)).xyz;"
    "  interp_world_normal = (vs_matrix * vec4(vert.normal, 0.0)).xyz;"
    "  interp_world_tangent = (vs_matrix * vec4(vert.tangent, 0.0)).xyz;"
    "  interp_view_pos = (vs_view * vec4(interp_world_pos, 1)).xyz;"
    "  gl_Position = vs_proj * vec4(interp_view_pos, 1);"
    "  gl_PointSize = 1.0;"
    "}"
  "";
  return gfx_new_shader_2(vs, sizeof(vs)-1, src, (int)size);
}

Gfx_Shader *gfx_new_shader_2(const char *vs, size_t vs_size, const char *ps, size_t ps_size) {
  return gfx_make_shader_impl(vs, (int)vs_size, ps, (int)ps_size);
}

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

static void setup_shader(Gfx_Shader *shader) {
  if (shader) {
    glUseProgram((GLuint)shader->handle);
  }
  else {
    glUseProgram((GLuint)default_shader->handle);
  }
}

static void gfx_update_viewport() {
  if (gfx_state_().use_viewport) {
    glViewport((GLint)gfx_state_().viewport[0], (GLint)gfx_state_().viewport[1], (GLint)gfx_state_().viewport[2], (GLint)gfx_state_().viewport[3]);
  }
  else if (!gfx_state_().canvas) {
    glViewport(0,0,gfx_width, gfx_height);
  }
  else {
    glViewport(0,0,gfx_state_().canvas->width,gfx_state_().canvas->height);
  }
}

static void setup_canvas() {
  if (gfx_state_().canvas) {
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)gfx_state_().canvas->handle);
  }
  else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  gfx_update_viewport();
  gfx_update_scissor();
  gfx_set_culling(gfx_state_().culling_type);
}

void gfx_set_wireframe(bool b) {
  gfx_state_().is_wireframe = b;
#ifndef __EMSCRIPTEN__
  if (gfx_state_().is_wireframe != cur_is_wireframe) {
    cur_is_wireframe = gfx_state_().is_wireframe;
    if (cur_is_wireframe) {
      glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
    }
    else {
      glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    }
  }
#endif
}
void gfx_push(void) {
  assert(gfx_.state_cursor+1 < ARRAY_SIZE(gfx_.state_stack) && "Graphics stack full");
  gfx_.state_cursor++;
  gfx_.state_stack[gfx_.state_cursor] = gfx_.state_stack[gfx_.state_cursor-1];
}
void gfx_pop(void) {
  assert(gfx_.state_cursor > 0 && "Graphics stack empty");
  gfx_.state_cursor--;
  setup_shader(gfx_state_().shader);
  setup_canvas();
  gfx_set_blend_mode(gfx_state_().blend_mode);
  gfx_set_depth_test(gfx_state_().depth_mode);
  gfx_set_depth_write(gfx_state_().write_depth);
  gfx_set_wireframe(gfx_state_().is_wireframe);
  gfx_update_stencil();
}
void gfx_reset_transform(void) {
  gfx_matrix_identity_(gfx_state_().transform.data);
}
void gfx_translate3(float tx, float ty, float tz) {
  gfx_matrix_translate(gfx_state_().transform.data, tx, ty, tz);
}
void gfx_scale3(float sx, float sy, float sz) {
  gfx_matrix_scale(gfx_state_().transform.data, sx, sy, sz);
}
void gfx_translate(float tx, float ty) {
  gfx_matrix_translate(gfx_state_().transform.data, tx, ty, 0);
}
void gfx_scale(float sx, float sy) {
  gfx_matrix_scale(gfx_state_().transform.data, sx, sy, 1);
}
void gfx_rotate(float r) {
  gfx_matrix_rotate_z(gfx_state_().transform.data, r);
}
void gfx_rotate_y(float r) {
  matrix_rotate_y(gfx_state_().transform.data, r);
}
void gfx_rotate_x(float r) {
  gfx_matrix_rotate_x_(gfx_state_().transform.data, r);
}
void gfx_apply_transform(const float mat[16]) {
  Gfx_Mat4_ tmp = gfx_state_().transform;
  gfx_matrix_multiply(gfx_state_().transform.data, tmp.data, mat);
}
void gfx_set_view(const float mat[16]) {
  memcpy(gfx_state_().view.data, mat, sizeof(float)*16);
}
void gfx_reset_view() {
  gfx_matrix_identity_(gfx_state_().view.data);
}
void gfx_set_transform(float mat[16]) {
  memcpy(gfx_state_().transform.data, mat, sizeof(float)*16);
}
void gfx_transform_point(float *x, float *y) {
  float v[4] = { *x, *y, 0.f, 1.f };
  float r[4] = { 0.f, 0.f, 0.f, 0.f };
  gfx_matrix_mulv(r, gfx_state_().transform.data, v);
  *x = r[0];
  *y = r[1];
}

void gfx_set_font(Font* font) {
  if (!font)
    font = gfx_.default_font;
  gfx_state_().font = font;
}

void gfx_text_ex(const char *text, float x, float y, Gfx_Text_Ex ex) {
  font_draw_text_ex2(gfx_state_().font, text, x, y, ex);
}

//==============================
// Total height routine
static GFX_TEXT_LINE_PROC_( gfx_get_line_height_proc_ ) {
  int *total_height = (int *)user_data;
  if (ex.force_line_height)
    *total_height += ex.force_line_height;
  else
    *total_height += gfx_get_text_height();
}

float gfx_get_text_height_ex(const char *text, Gfx_Text_Ex ex) {
  int total_height = 0;
  gfx_font_for_text_line_ex_(gfx_state_().font, text, 0,0, ex, &gfx_get_line_height_proc_, &total_height);
  return total_height;
}

void gfx_text(const char* text, float x, float y) {
  font_draw_text(gfx_state_().font, text, x, y);
}
void gfx_text_part(const char *text, int len, float x, float y) {
  font_draw_text_part(gfx_state_().font, text, len, x, y);
}
void gfx_text_part_bordered(const char *text, int len, float x, float y) {
  font_draw_text_part_bordered(gfx_state_().font, text, len, (int)x, (int)y);
}
void gfx_text_anchor(const char *text, uint32_t anchor, float x, float y) {
  float width  = gfx_get_text_width(text);
  float height = gfx_get_text_width(text);
  if (anchor & GFX_ANCHOR_CENTER_X) {
    x -= width/2;
  }
  else if (anchor & GFX_ANCHOR_RIGHT) {
    x -= width;
  }
  if (anchor & GFX_ANCHOR_CENTER_Y) {
    y -= height/2;
  }
  else if (anchor & GFX_ANCHOR_BOTTOM) {
    y -= height;
  }
  gfx_text(text, x, y);
}
float gfx_get_text_width_part(const char *text, int len) {
  return font_get_width_part(gfx_state_().font, text, len);
}
float gfx_get_text_width(const char* text) {
  return font_get_width(gfx_state_().font, text);
}
float gfx_get_text_height(void) {
  return font_get_height(gfx_state_().font);
}

void gfx_set_shader(Gfx_Shader *shader) {
  gfx_state_().shader = shader;
  setup_shader(shader);
}

Gfx_Shader *gfx_get_shader(void) {
  return gfx_state_().shader;
}

void gfx_shader_send(const char *name, int type, const void *data, size_t count) {
  Gfx_Shader *shader = 0;
  GLint loc = -1;
  int i;
  if (!gfx_state_().shader) {
    return;
  }

  shader = gfx_state_().shader;
  for (i = 0; i < shader->num_cached_uniforms; i++) {
    if (0 == strcmp(shader->cached_uniforms[i].name, name)) {
      loc = shader->cached_uniforms[i].loc;
      break;
    }
  }
  if (loc == -1) {
    loc = glGetUniformLocation((GLuint)gfx_state_().shader->handle, name);
    if (loc == -1) {
      return;
    }

    if (shader->num_cached_uniforms < _countof(shader->cached_uniforms)) {
      Gfx_Shader_Uniform uniform = {0};
      uniform.name = name;
      uniform.loc = loc;
      shader->cached_uniforms[shader->num_cached_uniforms++] = uniform;
    }
    else {
      printf("WARNING: Too many cached uniforms.\n");
    }
  }

  switch (type) {
    default:
      break;
    case GFX_UNIFORM_MATRIX4:
      glUniformMatrix4fv(loc, (GLsizei)count, GL_FALSE, (float *)data);
      break;
#if 0 // FIXME: Emscripten
    case GFX_UNIFORM_MATRIX4X3:
      glUniformMatrix4x3fv(loc, (GLsizei)count, GL_FALSE, (float *)data);
      break;
#endif

    case GFX_UNIFORM_FLOAT:
      glUniform1fv(loc, (GLsizei)count, (float *)data);
      break;
    case GFX_UNIFORM_FLOAT2:
      glUniform2fv(loc, (GLsizei)count, (float *)data);
      break;
    case GFX_UNIFORM_FLOAT3:
      glUniform3fv(loc, (GLsizei)count, (float *)data);
      break;
    case GFX_UNIFORM_FLOAT4:
      glUniform4fv(loc, (GLsizei)count, (float *)data);
      break;
    case GFX_UNIFORM_INT:
      glUniform1iv(loc, (GLsizei)count, (int32_t *)data);
      break;
    case GFX_UNIFORM_TEXTURE0:
    case GFX_UNIFORM_TEXTURE1:
    case GFX_UNIFORM_TEXTURE2:
    case GFX_UNIFORM_TEXTURE3:
    case GFX_UNIFORM_TEXTURE4:
    case GFX_UNIFORM_TEXTURE5:
    case GFX_UNIFORM_TEXTURE6:
    case GFX_UNIFORM_TEXTURE7:
    case GFX_UNIFORM_TEXTURE8:
    case GFX_UNIFORM_TEXTURE9:
    case GFX_UNIFORM_TEXTURE10:
    case GFX_UNIFORM_TEXTURE11:
    case GFX_UNIFORM_TEXTURE12:
    case GFX_UNIFORM_TEXTURE13:
    case GFX_UNIFORM_TEXTURE14:
    case GFX_UNIFORM_TEXTURE15:
    case GFX_UNIFORM_TEXTURE16:
    case GFX_UNIFORM_TEXTURE17:
    case GFX_UNIFORM_TEXTURE18:
    case GFX_UNIFORM_TEXTURE19:
    case GFX_UNIFORM_TEXTURE20:
    {
      GLint base_slot;
      GLint slot_list[32];
      int i;

      if (type + count > GFX_UNIFORM_TEXTURE_MAX) {
        gfx_log("Gfx_Texture array exceeds max texture slot.\n");
        return;
      }
      base_slot = type - GFX_UNIFORM_TEXTURE0 + 1;

      for (i = 0; i < count; i++) {
        GLint slot = base_slot + i;
        if (count > 1) {
          char real_uniform_name[32];
          snprintf(real_uniform_name, _countof(real_uniform_name), "%s[%d]", name, i);
          int slot_loc = glGetUniformLocation((GLuint)gfx_state_().shader->handle, real_uniform_name);
          glUniform1iv(slot_loc, 1, &slot);
        }

        Gfx_Texture **texture_list = (Gfx_Texture **)data;
        GLenum target = 0;
        glActiveTexture(GL_TEXTURE0 + slot);
        target = get_target_from_texture_dimension(texture_list[i]->dimension);
        glBindTexture(target, (GLuint)(texture_list[i])->handle);
        slot_list[i] = slot;
      }

      if (count <= 1)
        glUniform1iv(loc, (GLsizei)count, slot_list);
    } break;
  }
}

#if 0
void gfx_draw_vertices(const Vertex3D *vertices, int count, Gfx_Primitive_Type prim, Gfx_Texture *texture) {
  Gfx_Immed_Buffer_ *buf = &immed_buf;
  size_t size = sizeof(vertices[0]) * count;
  gfx_ibuf_flush_impl(buf, vertices, size);
  gfx_imm_draw(prim, texture);
}
#endif

void gfx_imm_draw(Gfx_Primitive_Type prim, Gfx_Texture *texture) {
  gfx_imm_draw_range(prim, texture, 0, (uint32_t)immed_buf.num_vertices_flushed);
}

static GLenum gfx_get_gl_primitive_(Gfx_Primitive_Type prim) {
  GLenum gl_prim = GL_TRIANGLES;
  switch (prim) {
    case GFX_PRIMITIVE_TRIANGLES:
      gl_prim = GL_TRIANGLES;
      break;
    case GFX_PRIMITIVE_LINES:
      gl_prim = GL_LINES;
      break;
    case GFX_PRIMITIVE_POINT:
      gl_prim = GL_POINTS;
      break;
  }
  return gl_prim;
}

static Gfx_Vertex_Buffer_Info_ *gfx_find_vertex_buffer_(Gfx_Vertex_Buffer vb);
static void gfx_setup_all_drawing_parameters_(Gfx_Vertex_Buffer_Info_ *vbi, Gfx_Primitive_Type prim, Gfx_Texture *texture);
static void gfx_setup_all_drawing_parameters2_(
    Gfx_Texture *texture,
    int vertex_size,
    const Gfx_Vertex_Attribute *attributes, int num_attributes);

static void gfx_setup_all_drawing_parameters_(Gfx_Vertex_Buffer_Info_ *vbi, Gfx_Primitive_Type prim, Gfx_Texture *texture) {
  if (glBindVertexArray)
    glBindVertexArray(vbi->vao);
  gfx_setup_all_drawing_parameters2_(
      texture,
      vbi->vertex_size,
      vbi->vertex_attributes, arr_len(vbi->vertex_attributes));
}

static void gfx_setup_all_drawing_parameters2_(
    Gfx_Texture *texture,
    int vertex_size,
    const Gfx_Vertex_Attribute *attributes, int num_attributes) {
  Gfx_Shader *shader = default_shader;
  GLuint program = (GLuint)default_shader->handle;
  GLint loc = -1;

  texture = texture ? texture : dummy_texture;
  if (gfx_state_().shader) {
    program = (GLuint)gfx_state_().shader->handle;
    shader = gfx_state_().shader;
  }

  for (int i = 0; i < num_attributes; i++) {
    Gfx_Vertex_Attribute attrib = attributes[i];
    loc = shader->attributes[attrib.id];
    if (loc != -1) {

      int num_elems = 1;
      bool normalized = false;
      int gl_elem_type = GL_FLOAT;
      switch (attrib.type) {
        case GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT:
          num_elems = 1;
          break;
        case GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT2:
          num_elems = 2;
          break;
        case GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT3:
          num_elems = 3;
          break;
        case GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT4:
          num_elems = 4;
          break;
        case GFX_VERTEX_ATTRIBUTE_TYPE_UBYTE4_NORM:
          gl_elem_type = GL_UNSIGNED_BYTE;
          num_elems = 4;
          normalized = true;
          break;
        case GFX_VERTEX_ATTRIBUTE_TYPE_UINT16:
          gl_elem_type = GL_UNSIGNED_SHORT;
          num_elems = 1;
          normalized = true;
          break;
      }

      glVertexAttribPointer(loc, num_elems, gl_elem_type, normalized,
          vertex_size,
          (void *)(size_t)attrib.offset_in_bytes);
      glEnableVertexAttribArray(loc);
    }
  }

  if (shader->uniform_loc_view != -1)
    glUniformMatrix4fv(shader->uniform_loc_view,   1, GL_FALSE, gfx_state_().view.data);
  if (shader->uniform_loc_matrix != -1)
    glUniformMatrix4fv(shader->uniform_loc_matrix, 1, GL_FALSE, gfx_state_().transform.data);

  {
    float projection[16] = {0};
    gfx_get_projection_(projection);
    if (shader->uniform_loc_proj != -1)
      glUniformMatrix4fv(shader->uniform_loc_proj,   1, GL_FALSE, projection);
  }

  if (shader->uniform_loc_texture != -1) {
    GLenum target = get_target_from_texture_dimension(texture->dimension);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(target, (GLuint)texture->handle);
    glUniform1i(shader->uniform_loc_texture, 0);
  }
}

static Gfx_Vertex_Buffer_Info_ *gfx_find_vertex_buffer_(Gfx_Vertex_Buffer vb) {
  bool index_valid = vb.handle && vb.handle <= arr_len(gfx_.vertex_buffers);
  assert(index_valid);
  if (!index_valid) return NULL;

  {
    Gfx_Vertex_Buffer_Info_ *vbi = &gfx_.vertex_buffers[vb.handle-1];
    if (!vbi->id.handle)
      return NULL;
    return vbi;
  }
}


void gfx_vertex_buffer_set_vertices(Gfx_Vertex_Buffer vb, const void *vertices, int num_vertices) {
  Gfx_Vertex_Buffer_Info_ *vbi = gfx_find_vertex_buffer_(vb);
  assert(vbi);
  if (!vbi) return;

  if (vbi->num_vertices < num_vertices) {
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vbi->vbo);
    glBufferData(GL_ARRAY_BUFFER, num_vertices * vbi->vertex_size, vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    vbi->num_vertices = num_vertices;
  }
  else {
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vbi->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_vertices * vbi->vertex_size, vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
}

void gfx_draw_vertex_buffer_range(Gfx_Vertex_Buffer handle, Gfx_Primitive_Type prim, Gfx_Texture *texture, uint32_t begin, uint32_t end) {
  Gfx_Vertex_Buffer_Info_ *vbi = gfx_find_vertex_buffer_(handle);
  if (!vbi)
    return;

  glBindBuffer(GL_ARRAY_BUFFER, vbi->vbo);
  gfx_setup_all_drawing_parameters_(vbi, prim, texture);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbi->ebo);
  glDrawElements(gfx_get_gl_primitive_(prim), end - begin, GL_UNSIGNED_SHORT/*FIXME: Should depend on the index size of the vb*/, (void *)(size_t)(begin * vbi->index_size));
}

void gfx_draw_vertex_buffer(Gfx_Vertex_Buffer handle, Gfx_Primitive_Type prim, Gfx_Texture *texture) {
  Gfx_Vertex_Buffer_Info_ *vbi = gfx_find_vertex_buffer_(handle);
  if (!vbi)
    return;

  glBindBuffer(GL_ARRAY_BUFFER, vbi->vbo);
  gfx_setup_all_drawing_parameters_(vbi, prim, texture);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbi->ebo);

  GLenum index_type = 0;
  switch (vbi->index_size) {
  case 2:
    index_type = GL_UNSIGNED_SHORT;
    break;
  case 4:
    index_type = GL_UNSIGNED_INT;
    break;
  }

  glDrawElements(gfx_get_gl_primitive_(prim), vbi->num_indices, index_type, 0);
}

void gfx_imm_draw_range(Gfx_Primitive_Type prim, Gfx_Texture *texture, uint32_t begin, uint32_t end) {
  if (glBindVertexArray)
    glBindVertexArray(immed_buf.vao);
  glBindBuffer(GL_ARRAY_BUFFER, immed_buf.handle);
  gfx_setup_all_drawing_parameters2_(texture, immed_buf.vertex_size,
      immed_buf.vertex_attrs, arr_len(immed_buf.vertex_attrs));
  glDrawArrays(gfx_get_gl_primitive_(prim), (GLint)begin, (GLsizei)(end - begin));
}

uint32_t gfx_endian_swap_(uint32_t val) {
  return
    ((val & (0xff << 24)) >> 24) |
    ((val & (0xff << 16)) >> 8 ) |
    ((val & (0xff << 8 )) << 8 ) |
    ((val & (0xff << 0 )) << 24);
}

void gfx_clear_web(uint32_t color) {
  gfx_clear_u(gfx_color_web(color));
}

void gfx_clear_u(uint32_t color) {
  color = gfx_endian_swap_(color);
  gfx_clear(
    (float)((color & (0xff << 24)) >> 24) / 255.f,
    (float)((color & (0xff << 16)) >> 16) / 255.f,
    (float)((color & (0xff <<  8)) >>  8) / 255.f,
    (float)((color & (0xff <<  0)) >>  0) / 255.f
  );
}

void gfx_clear(float r, float g, float b, float a) {
  glClearColor(r,g,b,a);
  glClear(GL_COLOR_BUFFER_BIT);

#if 0 // FIXME: Emscripten
  if (gfx_state_().canvas) {
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glClear(GL_COLOR_BUFFER_BIT);
  }
#endif
}

void gfx_clear_p(const float color[4]) {
  glClearColor(color[0],color[1],color[2],color[3]);
  glClear(GL_COLOR_BUFFER_BIT);
}

void gfx_imm_rect(float x, float y, float w, float h) {
  uint32_t color = gfx_get_color_u();
  gfx_imm_vertex_2d(x,    y,    0,0, color);
  gfx_imm_vertex_2d(x+w,  y,    1,0, color);
  gfx_imm_vertex_2d(x+w,  y+h,  1,1, color);

  gfx_imm_vertex_2d(x,    y,    0,0, color);
  gfx_imm_vertex_2d(x+w,  y+h,  1,1, color);
  gfx_imm_vertex_2d(x,    y+h,  0,1, color);
}

void gfx_fill_rect(float x, float y, float w, float h) {
  gfx_imm_begin_2d();
  gfx_imm_rect(x,y,w,h);
  gfx_imm_end();
  gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, NULL);
}

void gfx_imm_circle(float x, float y, float radius, int segments) {
  float ox = x;
  float oy = y;

  const float step = (2 * 3.1415926f) / segments;

  const float u = 0.5f;
  const float v = 0.5f;

  const uint32_t col = gfx_get_color_u();

  int i;

  for (i = 0; i < segments; i++) {
    float px = x + cosf(step * i) * radius;
    float py = y + sinf(step * i) * radius;

    float qx = x + cosf(step * (i+1)) * radius;
    float qy = y + sinf(step * (i+1)) * radius;

    float pu = u + 0.5f * cosf(step *i);
    float pv = v + 0.5f * sinf(step *i);
    float qu = u + 0.5f * cosf(step *(i+1));
    float qv = v + 0.5f * sinf(step *(i+1));

    gfx_imm_vertex_2d(px, py, pu, pv, col);
    gfx_imm_vertex_2d(qx, qy, qu, qv, col);
    gfx_imm_vertex_2d(ox, oy,  u,  v, col);
  }
}

void gfx_imm_trace_circle(float x, float y, float radius, int segments) {
  float ox = x;
  float oy = y;

  const float step = (2 * 3.1415926f) / segments;

  const float u = 0.5f;
  const float v = 0.5f;

  const uint32_t col = gfx_get_color_u();

  int i;
  for (i = 0; i < segments; i++) {
    float px = x + cosf(step * i) * radius;
    float py = y + sinf(step * i) * radius;

    float qx = x + cosf(step * (i+1)) * radius;
    float qy = y + sinf(step * (i+1)) * radius;

    float pu = u + 0.5f * cosf(step *i);
    float pv = v + 0.5f * sinf(step *i);
    float qu = u + 0.5f * cosf(step *(i+1));
    float qv = v + 0.5f * sinf(step *(i+1));

    gfx_imm_vertex_2d(px, py, pu, pv, col);
    gfx_imm_vertex_2d(qx, qy, qu, qv, col);
  }
}

void gfx_trace_circle(float x, float y, float radius) {
  gfx_trace_circle_impl(x,y,radius, 20);
}

void gfx_trace_circle_impl(float x, float y, float radius, int segments) {
  gfx_imm_begin_2d();
  gfx_imm_trace_circle(x,y,radius,segments);
  gfx_imm_end();
  gfx_imm_draw(GFX_PRIMITIVE_LINES, NULL);
}

void gfx_fill_circle_impl(float x, float y, float radius, int segments) {
  gfx_imm_begin_2d();
  gfx_imm_circle(x,y,radius,segments);
  gfx_imm_end();
  gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, NULL);
}
void gfx_fill_circle(float x, float y, float radius) {
  gfx_fill_circle_impl(x, y, radius, 20);
}

void gfx_fill_arc(float x, float y, float radius, float angle_a, float angle_b) {
  gfx_fill_arc_impl(x,y,radius,angle_a,angle_b,20);
}

void gfx_fill_arc_impl(float x, float y, float radius, float angle_a, float angle_b, int segments) {

  const float ox = x;
  const float oy = y;

  const float start = angle_a / 180 * 3.1415926f;
  const float step = ((angle_b - angle_a) / 180 * 3.1415926f) / segments;

  const float u = 0.5f;
  const float v = 0.5f;

  const uint32_t col = gfx_get_color_u();

  int i;

  gfx_imm_begin_2d();
  for (i = 0; i < segments; i++) {
    const float px = x + cosf(start + step * i) * radius;
    const float py = y + sinf(start + step * i) * radius;

    const float qx = x + cosf(start + step * (i+1)) * radius;
    const float qy = y + sinf(start + step * (i+1)) * radius;

    const float pu = u + 0.5f * cosf(start + step *i);
    const float pv = v + 0.5f * sinf(start + step *i);
    const float qu = u + 0.5f * cosf(start + step *(i+1));
    const float qv = v + 0.5f * sinf(start + step *(i+1));

    gfx_imm_vertex_2d(px, py, pu, pv, col);
    gfx_imm_vertex_2d(qx, qy, qu, qv, col);
    gfx_imm_vertex_2d(ox, oy,  u,  v, col);
  }

  gfx_imm_end();
  gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, NULL);
}

void gfx_trace_rect(float x, float y, float w, float h) {
  gfx_fill_rect(x,y, w, 1);
  gfx_fill_rect(x,y, 1, h);
  gfx_fill_rect(x,y+h-1, w, 1);
  gfx_fill_rect(x+w-1,y, 1, h);
}

void gfx_line_3d(float x0, float y0, float z0, float x1, float y1, float z1) {
  uint32_t c = gfx_get_color_u();
  gfx_imm_begin_3d();
  gfx_imm_vertex_3d(  x0,  y0,  z0,   0,0, c);
  gfx_imm_vertex_3d(  x1,  y1,  z1,   1,0, c);
  gfx_imm_end();

  gfx_imm_draw(GFX_PRIMITIVE_LINES, NULL);
}

void gfx_line(float x0, float y0, float x1, float y1) {
  uint32_t c = gfx_get_color_u();
  gfx_imm_begin_2d();
  gfx_imm_vertex_2d(  x0,  y0,    0,0, c);
  gfx_imm_vertex_2d(  x1,  y1,    1,0, c);
  gfx_imm_end();

  gfx_imm_draw(GFX_PRIMITIVE_LINES, NULL);
}

void gfx_point(float x, float y) {
  uint32_t c = gfx_get_color_u();
  gfx_imm_begin_2d();
  gfx_imm_vertex_2d(  x,  y,    0,0, c);
  gfx_imm_end();
  gfx_imm_draw(GFX_PRIMITIVE_POINT, NULL);
}

void gfx_imm_begin_2d(void) {
  Gfx_Vertex_Attribute attrs[] = {
    { GFX_VERTEX_ATTRIBUTE_POSITION, GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT3, GFX_OFFSET_OF(Gfx_Vertex_2D, pos) },
    { GFX_VERTEX_ATTRIBUTE_UV, GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT2, GFX_OFFSET_OF(Gfx_Vertex_2D, uv) },
    { GFX_VERTEX_ATTRIBUTE_COLOR, GFX_VERTEX_ATTRIBUTE_TYPE_UBYTE4_NORM, GFX_OFFSET_OF(Gfx_Vertex_2D, color) },
  };
  gfx_ibuf_reset(&immed_buf, sizeof(Gfx_Vertex_3D), attrs, _countof(attrs));
}

void gfx_imm_begin_3d(void) {
  Gfx_Vertex_Attribute attrs[] = {
    { GFX_VERTEX_ATTRIBUTE_POSITION, GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT3, GFX_OFFSET_OF(Gfx_Vertex_3D, pos) },
    { GFX_VERTEX_ATTRIBUTE_UV, GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT2, GFX_OFFSET_OF(Gfx_Vertex_3D, uv) },
    { GFX_VERTEX_ATTRIBUTE_NORMAL, GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT3, GFX_OFFSET_OF(Gfx_Vertex_3D, normal) },
    { GFX_VERTEX_ATTRIBUTE_COLOR, GFX_VERTEX_ATTRIBUTE_TYPE_UBYTE4_NORM, GFX_OFFSET_OF(Gfx_Vertex_3D, color) },
  };
  gfx_ibuf_reset(&immed_buf, sizeof(Gfx_Vertex_3D), attrs, _countof(attrs));
}

void gfx_imm_begin_custom(int vertex_size, const Gfx_Vertex_Attribute *attributes, int num_attributes) {
  gfx_ibuf_reset(&immed_buf, vertex_size, attributes, num_attributes);
}

void gfx_imm_vertex_3d_impl(Gfx_Vertex_3D vertex) {
  gfx_ibuf_add(&immed_buf, &vertex, sizeof(vertex));
}

void gfx_imm_vertex_custom(const void *data, int size) {
  assert(immed_buf.vertex_size == size);
  gfx_ibuf_add(&immed_buf, data, size);
}

void gfx_imm_vertex_3d(float x, float y, float z, float u, float v, uint32_t color) {
  Gfx_Vertex_3D vertex = {0};
  vertex.pos[0] = x;
  vertex.pos[1] = y;
  vertex.pos[2] = z;
  vertex.uv[0] = u;
  vertex.uv[1] = v;
  vertex.color = color;
  gfx_ibuf_add(&immed_buf, &vertex, sizeof(vertex));
}

void gfx_imm_vertex_2d(float x, float y, float u, float v, uint32_t color) {
  Gfx_Vertex_2D vertex = {0};
  vertex.pos[0] = x;
  vertex.pos[1] = y;
  vertex.pos[2] = gfx_state_().depth_2d;
  vertex.uv[0] = u;
  vertex.uv[1] = v;
  vertex.color = color;
  gfx_ibuf_add(&immed_buf, &vertex, sizeof(vertex));
}

void gfx_imm_vertex_2d_with_z(float x, float y, float z, float u, float v, uint32_t color) {
  Gfx_Vertex_2D vertex = {0};
  vertex.pos[0] = x;
  vertex.pos[1] = y;
  vertex.pos[2] = z;
  vertex.uv[0] = u;
  vertex.uv[1] = v;
  vertex.color = color;
  gfx_ibuf_add(&immed_buf, &vertex, sizeof(vertex));
}

void gfx_imm_end(void) {
  gfx_ibuf_flush(&immed_buf);
}

void gfx_imm_q_color_u(Gfx_Texture *t, const float quad[4], float x, float y, uint32_t color) {
  float w = quad[2];
  float h = quad[3];

  float u0 = quad[0] / t->width;
  float v0 = quad[1] / t->height;
  float u1 = (quad[0]+quad[2]) / t->width;
  float v1 = (quad[1]+quad[3]) / t->height;

  gfx_imm_vertex_2d(x,    y,    u0,v0, color);
  gfx_imm_vertex_2d(x+w,  y,    u1,v0, color);
  gfx_imm_vertex_2d(x+w,  y+h,  u1,v1, color);

  gfx_imm_vertex_2d(x,    y,    u0,v0, color);
  gfx_imm_vertex_2d(x+w,  y+h,  u1,v1, color);
  gfx_imm_vertex_2d(x,    y+h,  u0,v1, color);
}
void gfx_imm_q(Gfx_Texture *t, const float quad[4], float x, float y) {
  uint32_t color = gfx_get_color_u();
  gfx_imm_q_color_u(t, quad, x, y, color);
}

void gfx_drawq_impl(Gfx_Texture *t, const float quad[4], float x, float y, float r, float sx, float sy, float ox, float oy) {
  gfx_push();
  if (x || y)
    gfx_translate(x,y);
  if (r)
    gfx_rotate(r);
  if (sx != 1 || sy != 1)
    gfx_scale(sx,sy);
  if (ox || oy)
    gfx_translate(ox,oy);

  {
    float w = quad[2];
    float h = quad[3];

    float u0 = quad[0] / t->width;
    float v0 = quad[1] / t->height;
    float u1 = (quad[0]+quad[2]) / t->width;
    float v1 = (quad[1]+quad[3]) / t->height;

    uint32_t color = gfx_get_color_u();
    gfx_imm_begin_2d();
    gfx_imm_vertex_2d(0,    0,    u0,v0, color);
    gfx_imm_vertex_2d(0+w,  0,    u1,v0, color);
    gfx_imm_vertex_2d(0+w,  0+h,  u1,v1, color);

    gfx_imm_vertex_2d(0,    0,    u0,v0, color);
    gfx_imm_vertex_2d(0+w,  0+h,  u1,v1, color);
    gfx_imm_vertex_2d(0,    0+h,  u0,v1, color);
    gfx_imm_end();

    gfx_imm_draw(GFX_PRIMITIVE_TRIANGLES, t);
    gfx_pop();
  }
}

void gfx_draw_impl(Gfx_Texture *t, float x, float y, float r, float sx, float sy, float ox, float oy) {
  float quad[4] = {0.f, 0.f, (float)t->width, (float)t->height};
  gfx_drawq_impl(t, quad, x, y, r, sx, sy, ox, oy);
}

void gfx_draw(Gfx_Texture *t, float x, float y) {
  gfx_draw_impl(t, x, y, 0, 1,1, 0,0);
}

void gfx_drawq(Gfx_Texture *t, const float quad[4], float x, float y) {
  gfx_drawq_impl(t, quad, x, y, 0, 1,1, 0,0);
}

static bool is_color_format(Gfx_Pixel_Format format) {
  switch (format) {
#if 0 // FIXME: Emscripten
  case GFX_PIXEL_FORMAT_R8:
#endif
  case GFX_PIXEL_FORMAT_RGBA8:
    return true;
  default:
    return false;
  }
  return false;
}

static GLenum get_canvas_depth_stencil_attachment_from_pixel_format(Gfx_Pixel_Format format) {
  switch (format) {
  default:
    break;
  case GFX_PIXEL_FORMAT_DEPTH16F:
#if GFX_ENABLE_DEPTH24F
  case GFX_PIXEL_FORMAT_DEPTH24F:
#endif
    return GL_DEPTH_ATTACHMENT;
#if 1 // FIXME: Emscripten
  case GFX_PIXEL_FORMAT_DEPTH24F_STENCIL8:
    return GL_DEPTH_STENCIL_ATTACHMENT;
#endif
  }
  return GL_NONE;
}

static bool check_canvas_texture(Gfx_Texture *texture, int component) {
  if (texture->dimension != GFX_TEXTURE_DIMENSION_2D &&
      texture->dimension != GFX_TEXTURE_DIMENSION_CUBE)
  {
    gfx_log("Can only attach texture_2d and texture_cube to canvas.");
    return 0;
  }
  if (component < 0 || component >= 6) {
    gfx_log("Gfx_Canvas attachment component has to be from 0 to 5");
    return 0;
  }
  return 1;
}

void gfx_canvas_bind_color_texture(Gfx_Canvas *canvas, Gfx_Texture *texture, int component) {
  GLenum target;
  if (!check_canvas_texture(texture, component))
    return;

  if (!is_color_format(texture->pixel_format)) {
    gfx_log("Cannot attach non-color texture to canvas's color attachment");
    return;
  }

  // FIXME: This is a big of a hack. If we bind canvas textures after
  // the fact, there best be some kind of check to make sure we're not
  // assigning textures of incorrect sizes.
  canvas->width = texture->width;
  canvas->height = texture->height;

  glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)canvas->handle);
  target = GL_TEXTURE_2D;
  if (texture->dimension == GFX_TEXTURE_DIMENSION_CUBE)
    target = GL_TEXTURE_CUBE_MAP_POSITIVE_X+component;

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, (GLuint)texture->handle, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void gfx_canvas_bind_depth_stencil_texture(Gfx_Canvas *canvas, Gfx_Texture *texture, int component) {
  GLenum attachment;
  GLenum target;
  if (!check_canvas_texture(texture, component))
    return;

  attachment = get_canvas_depth_stencil_attachment_from_pixel_format(texture->pixel_format);
  if (!attachment) {
    gfx_log("Cannot attach non-depth/stencil texture to canvas's depth/stencil attachment");
    return;
  }

  // FIXME: This is a big of a hack. If we bind canvas textures after
  // the fact, there best be some kind of check to make sure we're not
  // assigning textures of incorrect sizes.
  canvas->width = texture->width;
  canvas->height = texture->height;

  glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)canvas->handle);
  target = GL_TEXTURE_2D;
  if (texture->dimension == GFX_TEXTURE_DIMENSION_CUBE)
    target = GL_TEXTURE_CUBE_MAP_POSITIVE_X+component;

  glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, target, (GLuint)texture->handle, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

float gfx_get_canvas_width(Gfx_Canvas *canvas) {
  return (float)canvas->width;
}
float gfx_get_canvas_height(Gfx_Canvas *canvas) {
  return (float)canvas->height;
}
Gfx_Texture *gfx_get_canvas_texture(Gfx_Canvas *canvas) {
  return &canvas->texture;
}
Gfx_Texture *gfx_get_canvas_depth_stencil_texture(Gfx_Canvas *canvas) {
  return &canvas->depth_stencil_texture;
}
Gfx_Canvas *gfx_new_canvas(int width, int height) {
  return gfx_new_canvas_format(width, height, GFX_PIXEL_FORMAT_RGBA8, GFX_PIXEL_FORMAT_NONE);
}

Gfx_Canvas *gfx_new_canvas_from_textures(Gfx_Texture *color_texture, int color_slice, Gfx_Texture *depth_texture, int depth_slice) {
  int width = 0;
  int height = 0;
  GLuint handle = 0;

  if (!color_texture && !depth_texture)
    return NULL;
  if (color_texture && depth_texture && (color_texture->width != depth_texture->width || color_texture->height != depth_texture->height)) {
    // TODO: log
    return NULL;
  }

  if (color_texture) {
    width = color_texture->width;
    height = color_texture->height;
  }
  else if (depth_texture) 
    width = depth_texture->width;
    height = depth_texture->height;{
  }

  glGenFramebuffers(1, &handle);

  {
    Gfx_Canvas *c = GFX_NEW(Gfx_Canvas);
    if (c) {
      c->handle = handle;
      c->width = width;
      c->height = height;

      if (color_texture)
        gfx_canvas_bind_color_texture(c, color_texture, color_slice);
      if (depth_texture)
        gfx_canvas_bind_depth_stencil_texture(c, depth_texture, depth_slice);
    }
    else {
      glDeleteFramebuffers(1, &handle);
    }
    return c;
  }
}

Gfx_Canvas *gfx_new_canvas_with_depth_stencil(int width, int height) {
  //GLenum depth_stencil_attachment = GL_DEPTH_STENCIL;
  GLuint handle = 0;
  glGenFramebuffers(1, &handle);
  Gfx_Texture texture = gfx_make_texture_impl(GFX_TEXTURE_DIMENSION_2D, NULL, width, height, 1, GFX_PIXEL_FORMAT_RGBA8);

  glBindFramebuffer(GL_FRAMEBUFFER, handle);
  GLuint render_buffer_handle = 0;
  glGenRenderbuffers(1, &render_buffer_handle);
  glBindRenderbuffer(GL_RENDERBUFFER, render_buffer_handle);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer_handle);
  if (gfx_state_().canvas) {
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)gfx_state_().canvas->handle);
  }
  else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  {
    Gfx_Canvas *c = GFX_NEW(Gfx_Canvas);
    if (c) {
      c->handle = handle;
      c->width = width;
      c->height = height;
      c->texture = texture;

      if (c->texture.handle)
        gfx_canvas_bind_color_texture(c, &c->texture, 0);
    }
    else {
      gfx_free_texture_impl_(&texture);
    }
    return c;
  }
}

Gfx_Canvas *gfx_new_canvas_format(int width, int height, Gfx_Pixel_Format color_format, Gfx_Pixel_Format depth_stencil_format) {
  GLenum depth_stencil_attachment = GL_NONE;
  GLuint handle = 0;
  Gfx_Texture texture = {0};
  Gfx_Texture depth_stencil_texture = {0};

  if (color_format && !is_color_format(color_format)) {
    gfx_log("Format for color texture of canvas must be color, not depth/stencil.");
    return NULL;
  }

  if (depth_stencil_format && !(depth_stencil_attachment = get_canvas_depth_stencil_attachment_from_pixel_format(depth_stencil_format))) {
    gfx_log("Format for depth/stencil texture of canvas must be not depth/stencil, not color.");
    return NULL;
  }
  
  glGenFramebuffers(1, &handle);

  if (color_format) {
    texture = gfx_make_texture_impl(GFX_TEXTURE_DIMENSION_2D, NULL, width, height, 1, color_format);
  }

  if (depth_stencil_format) {
    depth_stencil_texture = gfx_make_texture_impl(GFX_TEXTURE_DIMENSION_2D, NULL, width, height, 1, depth_stencil_format);
  }

  {
    Gfx_Canvas *c = GFX_NEW(Gfx_Canvas);
    if (c) {
      c->handle = handle;
      c->width = width;
      c->height = height;
      c->texture = texture;
      c->depth_stencil_texture = depth_stencil_texture;

      if (c->texture.handle)
        gfx_canvas_bind_color_texture(c, &c->texture, 0);
      if (c->depth_stencil_texture.handle)
        gfx_canvas_bind_depth_stencil_texture(c, &c->depth_stencil_texture, 0);
    }
    else {
      gfx_free_texture_impl_(&texture);
      gfx_free_texture_impl_(&depth_stencil_texture);
    }
    return c;
  }
}

void gfx_free_canvas(Gfx_Canvas *canvas) {
  if (canvas->texture.handle) {
    GLuint handle = (GLuint)canvas->texture.handle;
    glDeleteTextures(1, &handle);
  }
  gfx_free_texture_impl_(&canvas->texture);
  gfx_free_texture_impl_(&canvas->depth_stencil_texture);
  free(canvas);
}

void gfx_read_canvas_region(void *dest, int x, int y, int width, int height) {
  // @FIXME: this function fails silently
  if (!gfx_state_().canvas)
    return;
  glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dest);
}

void gfx_read_depth_canvas_region(void *dest, int x, int y, int width, int height) {
  if (!gfx_state_().canvas)
    return;
  glReadPixels(x, y, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, dest);
}

uint32_t gfx_read_canvas_pixel(int x, int y) {
  uint32_t ret = 0;
  // @FIXME: this function fails silently
  if (!gfx_state_().canvas)
    return 0;
  glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &ret);
  return ret;
}

void gfx_set_canvas(Gfx_Canvas *canvas) {
  gfx_set_canvas_slice(canvas, 0);
}

void gfx_set_canvas_slice(Gfx_Canvas *canvas, int slice) {
  gfx_state_().canvas = canvas;
  gfx_state_().canvas_slice = slice;
  setup_canvas();
}

Gfx_Canvas *gfx_get_canvas() {
  return gfx_state_().canvas;
}

void gfx_set_2d_depth(float d) {
  gfx_state_().depth_2d = d;
}

void gfx_set_blend_mode(int blend_mode) {
  gfx_state_().blend_mode = blend_mode;
  if (cur_blend_mode != blend_mode) {
    cur_blend_mode = blend_mode;
    switch (blend_mode) {
      case GFX_BLEND_NONE:
        glDisable(GL_BLEND);
        break;
      case GFX_BLEND_ALPHA:
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
      case GFX_BLEND_MULTIPLY:
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_DST_COLOR, GL_ZERO, GL_DST_COLOR, GL_ONE);
        break;
      case GFX_BLEND_ADD:
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        break;
    }
  }
}

void gfx_set_culling(Gfx_Culling_Type type) {
  gfx_state_().culling_type = type;
  bool has_canvas = !!gfx_state_().canvas;
  if (cur_culling_type != type || cur_culling_has_canvas != has_canvas) {
    cur_culling_type = type;

    // If there is canvas, in OPENGL we have to invert the projection-y in order
    // to draw everything upright. However, this causes all clockwise triangles
    // to become counter-clockwise, and vise-versa. So invert it here for convenience.
    cur_culling_has_canvas = has_canvas;
    if (has_canvas) {
      if (GFX_CULLING_CW == type) {
        type = GFX_CULLING_CCW;
      }
      else if (type == GFX_CULLING_CCW) {
        type = GFX_CULLING_CW;
      }
    }

    switch (type) {
      case GFX_CULLING_OFF:
      {
        glDisable(GL_CULL_FACE);
      } break;
      case GFX_CULLING_CW:
      {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);  
      } break;
      case GFX_CULLING_CCW:
      {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);  
      } break;
    }
  }
}

void gfx_set_smooth_polygon(int smooth) {
#if 0 // FIXME: Emscripten
  if (smooth)
    glEnable(GL_POLYGON_SMOOTH);
  else
    glDisable(GL_POLYGON_SMOOTH);
#endif
}

#ifdef GL_DEBUG_OUTPUT
static void APIENTRY gl_callback(GLenum source,
			GLenum type,
			GLuint id,
			GLenum severity,
			GLsizei length,
			const GLchar *message,
			const void *userParam)
{
  if (severity != GL_DEBUG_SEVERITY_NOTIFICATION) {
    gfx_log(message);
  }
}
#endif

bool gfx_is_vertex_buffer_valid(Gfx_Vertex_Buffer vb) {
  return vb.handle != 0;
}

static Gfx_Vertex_Buffer_Info_ *gfx_alloc_vertex_buffer_info_(void) {
  Gfx_Vertex_Buffer_Info_ *vbi = NULL;
  for (int i = 0; i < arr_len(gfx_.vertex_buffers); i++) {
    Gfx_Vertex_Buffer_Info_ *it = &gfx_.vertex_buffers[i];
    if (!it->id.handle) {
      vbi = it;
      break;
    }
  }
  if (!vbi)
    vbi = arr_add_new(gfx_.vertex_buffers);

  if (vbi) {
    vbi->id.handle = (uint16_t)(vbi - gfx_.vertex_buffers) + 1;
  }
  return vbi;
}

Gfx_Vertex_Buffer gfx_new_vertex_buffer_impl(
    const void *vertices, int vertex_size, int num_vertices,
    const void *indices, int index_size, int num_indices,
    const Gfx_Vertex_Attribute *vertex_attribs, int num_vertex_attribs)
{
  Gfx_Vertex_Buffer_Info_ *vbi = gfx_alloc_vertex_buffer_info_();
  if (vbi) {
    GLuint bufs[2] = {0};
    glGenBuffers(2, bufs);

    vbi->ebo = bufs[0];
    vbi->vbo = bufs[1];

    vbi->num_vertices = num_vertices;
    vbi->vertex_size = vertex_size;

    vbi->num_indices = num_indices;
    vbi->index_size = index_size;

    if (glGenVertexArrays)
      glGenVertexArrays(1, &vbi->vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbi->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbi->ebo);

    glBufferData(GL_ARRAY_BUFFER, num_vertices * vertex_size, vertices, vertices ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    if (num_indices) {
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * index_size, indices, indices ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
    }

    for (int i = 0; i < num_vertex_attribs; i++) {
      arr_add(vbi->vertex_attributes, vertex_attribs[i]);
    }

    return vbi->id;
  }

  Gfx_Vertex_Buffer ret = {0};
  return ret;
}

Gfx_Vertex_Buffer gfx_new_vertex_buffer(const Gfx_Vertex_3D *vertices, int num_vertices, const Gfx_Index_Type *indices, int num_indices) {
  const Gfx_Vertex_Attribute vertex_attributs[] = {
    { GFX_VERTEX_ATTRIBUTE_POSITION,  GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT3,      GFX_OFFSET_OF(Gfx_Vertex_3D, pos), },
    { GFX_VERTEX_ATTRIBUTE_UV,        GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT2,      GFX_OFFSET_OF(Gfx_Vertex_3D, uv), },
    //{ GFX_VERTEX_ATTRIBUTE_UV2,       GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT2,      GFX_OFFSET_OF(Gfx_Vertex_3D, uv2), },
    { GFX_VERTEX_ATTRIBUTE_NORMAL,    GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT3,      GFX_OFFSET_OF(Gfx_Vertex_3D, normal), },
    //{ GFX_VERTEX_ATTRIBUTE_TANGENT,   GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT3,      GFX_OFFSET_OF(Gfx_Vertex_3D, tangent), },
    //{ GFX_VERTEX_ATTRIBUTE_BITANGENT, GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT3,      GFX_OFFSET_OF(Gfx_Vertex_3D, bitangent), },
    { GFX_VERTEX_ATTRIBUTE_COLOR,     GFX_VERTEX_ATTRIBUTE_TYPE_UBYTE4_NORM, GFX_OFFSET_OF(Gfx_Vertex_3D, color), },
    //{ GFX_VERTEX_ATTRIBUTE_VERTEX_ID, GFX_VERTEX_ATTRIBUTE_TYPE_UINT16,      GFX_OFFSET_OF(Gfx_Vertex_3D, pos), },
  };

  return gfx_new_vertex_buffer_impl(
      vertices, sizeof(vertices[0]), num_vertices,
      indices, sizeof(indices[0]), num_indices,
      vertex_attributs, _countof(vertex_attributs));
}

void gfx_free_vertex_buffer(Gfx_Vertex_Buffer handle) {
  Gfx_Vertex_Buffer_Info_ *vbi = gfx_find_vertex_buffer_(handle);
  assert(vbi && "Freeing an invalid vertex buffer");
  if (!vbi) {
    return;
  }
  GLuint buffers[2];
  buffers[0] = vbi->vbo;
  buffers[1] = vbi->ebo;
  glDeleteBuffers(2, buffers);
  memset(vbi, 0, sizeof(*vbi));
}

void gfx_set_size(int width, int height) {
  gfx_width  = width;
  gfx_height = height;
  gfx_update_viewport();
}

bool gfx_supports_compute(void) {
  return gfx_supports_compute_;
}

void gfx_viewport(int x, int y, int width, int height) {
  gfx_state_().viewport[0] = (float)x;
  gfx_state_().viewport[1] = (float)y;
  gfx_state_().viewport[2] = (float)width;
  gfx_state_().viewport[3] = (float)height;
  gfx_state_().use_viewport = 1;
  gfx_update_viewport();
}

int gfx_get_viewport_width(void) {
  if (gfx_state_().use_viewport) {
    return (int)gfx_state_().viewport[2];
  }
  else if (gfx_state_().canvas) {
    return gfx_state_().canvas->width;
  }
  else {
    return gfx_width;
  }
}
int gfx_get_viewport_height(void) {
  if (gfx_state_().use_viewport) {
    return (int)gfx_state_().viewport[3];
  }
  else if (gfx_state_().canvas) {
    return gfx_state_().canvas->height;
  }
  else {
    return gfx_height;
  }
}

Font *gfx_new_default_font(void) {
  Font *font = GFX_NEW(Font);
  const char *image_bytes = GFX_DEFAULT_FONT_IMAGE;
  font_load_image(font, (const unsigned char *)image_bytes, GFX_DEFAULT_FONT_IMAGE_width, GFX_DEFAULT_FONT_IMAGE_height, " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.,!'\"", 1);
  return font;
}

bool gfx_supports_gl_extension(const char *name) {
  for (int i = 0; i < gfx_.num_extensions; i++) {
    if (str_eq(name, gfx_.extensions[i]))
      return true;
  }
  return false;
}

void gfx_setup(int width, int height, void *(*gl_loader)(const char *name)) {
  gfx_width = width;
  gfx_height = height;
  gfx_load_gl_functions(gl_loader);
  gfx_update_viewport();

#ifdef GL_DEBUG_OUTPUT
	if (glDebugMessageCallback) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(gl_callback, 0);
	}
#endif

  glGetIntegerv(GL_NUM_EXTENSIONS, &gfx_.num_extensions);
  gfx_.extensions = (const char **)malloc(gfx_.num_extensions * sizeof(const char *));
  for (int i = 0; i < gfx_.num_extensions; i++) {
    const char *extension = (const char*)glGetStringi(GL_EXTENSIONS, i);
    gfx_.extensions[i] = extension;
  }

  //gfx_set_culling(Culling_Off);
  gfx_ibuf_init(&immed_buf, sizeof(Gfx_Vertex_3D) * 2048);
  //glEnable(GL_POINT_SMOOTH);
  //glPointSize( 5.1 );

  gfx_set_depth_test(GFX_DEPTH_NONE);
  gfx_set_depth_write(1);

  glEnable(GL_BLEND);
  gfx_set_blend_mode(GFX_BLEND_ALPHA);

  gfx_set_color(1,1,1,1);
  gfx_matrix_identity_(gfx_state_().transform.data);
  gfx_matrix_identity_(gfx_state_().view.data);
  default_shader = gfx_new_default_shader();

  {
    uint32_t color = 0xffffffff;
    dummy_texture = gfx_new_texture(&color, 1,1);
    gfx_set_filter_mode(dummy_texture, GFX_FILTER_MODE_LINEAR, GFX_FILTER_MODE_LINEAR);
  }

  gfx_.default_font = gfx_new_default_font();
  gfx_.default_font->scale = 3;
  gfx_state_().font = gfx_.default_font;

  gfx_set_2d_projection();
  gfx_set_shader(0);

  gfx_set_size(width, height);
}

#ifdef __cplusplus
} // extern "C"
#endif
