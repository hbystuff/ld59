#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GFX_ENABLE_DEPTH24F 1

enum {
  GFX_UNIFORM_FLOAT,
  GFX_UNIFORM_FLOAT2,
  GFX_UNIFORM_FLOAT3,
  GFX_UNIFORM_FLOAT4,
  GFX_UNIFORM_INT,
  GFX_UNIFORM_TEXTURE0,
  GFX_UNIFORM_TEXTURE1,
  GFX_UNIFORM_TEXTURE2,
  GFX_UNIFORM_TEXTURE3,
  GFX_UNIFORM_TEXTURE4,
  GFX_UNIFORM_TEXTURE5,
  GFX_UNIFORM_TEXTURE6,
  GFX_UNIFORM_TEXTURE7,
  GFX_UNIFORM_TEXTURE8,
  GFX_UNIFORM_TEXTURE9,
  GFX_UNIFORM_TEXTURE10,
  GFX_UNIFORM_TEXTURE11,
  GFX_UNIFORM_TEXTURE12,
  GFX_UNIFORM_TEXTURE13,
  GFX_UNIFORM_TEXTURE14,
  GFX_UNIFORM_TEXTURE15,
  GFX_UNIFORM_TEXTURE16,
  GFX_UNIFORM_TEXTURE17,
  GFX_UNIFORM_TEXTURE18,
  GFX_UNIFORM_TEXTURE19,
  GFX_UNIFORM_TEXTURE20,
  GFX_UNIFORM_MATRIX4,
#if 0 // FIXME: Emscripten
  GFX_UNIFORM_MATRIX4X3,
#endif

  GFX_UNIFORM_TEXTURE_MAX = GFX_UNIFORM_TEXTURE20,
};

enum {
  GFX_BLEND_ALPHA,
  GFX_BLEND_MULTIPLY,
  GFX_BLEND_ADD,
  GFX_BLEND_NONE,
};

typedef enum Gfx_Primitive_Type {
  GFX_PRIMITIVE_TRIANGLES,
  GFX_PRIMITIVE_LINES,
  GFX_PRIMITIVE_POINT,
} Gfx_Primitive_Type;

typedef enum Gfx_Stencil_Op {
  GFX_STENCIL_OP_NONE,
  GFX_STENCIL_OP_ALWAYS_WRITE,
  GFX_STENCIL_OP_EQUAL,
  GFX_STENCIL_OP_EQUAL_WRITE_STENCIL,
  GFX_STENCIL_OP_EQUAL_WRITE_STENCIL_ZERO,
  GFX_STENCIL_OP_EQUAL_WRITE_STENCIL_INCREMENT,
} Gfx_Stencil_Op;

typedef enum Gfx_Texture_Dimension {
  GFX_TEXTURE_DIMENSION_2D,
  GFX_TEXTURE_DIMENSION_CUBE,
} Gfx_Texture_Dimension;

typedef enum Gfx_Pixel_Format {
  GFX_PIXEL_FORMAT_NONE,
  GFX_PIXEL_FORMAT_RGBA8,
#if 0 // FIXME: Emscripten
  GFX_PIXEL_FORMAT_R8,
#endif
  GFX_PIXEL_FORMAT_DEPTH16F,
#if GFX_ENABLE_DEPTH24F // FIXME: Emscripten
  GFX_PIXEL_FORMAT_DEPTH24F,
#endif
  GFX_PIXEL_FORMAT_DEPTH24F_STENCIL8,
  GFX_PIXEL_FORMAT_R32F,
} Gfx_Pixel_Format;

typedef enum Gfx_Anchor {
  GFX_ANCHOR_NONE = 0x0,
  GFX_ANCHOR_TOP  = 0x1,
  GFX_ANCHOR_BOTTOM = 0x2,
  GFX_ANCHOR_LEFT = 0x4,
  GFX_ANCHOR_RIGHT = 0x8,
  GFX_ANCHOR_CENTER_X = 0x10,
  GFX_ANCHOR_CENTER_Y = 0x20,
} Gfx_Anchor;

typedef enum Gfx_Depth_Mode {
  GFX_DEPTH_NONE,
  GFX_DEPTH_LT,
  GFX_DEPTH_LE,
  GFX_DEPTH_GT,
  GFX_DEPTH_GE,
  GFX_DEPTH_ALWAYS,
} Gfx_Depth_Mode;

typedef enum Gfx_Culling_Type {
  GFX_CULLING_OFF,  // This means we draw everything.
  GFX_CULLING_CCW,  // This means we DON'T draw CCW faces
  GFX_CULLING_CW,   // This means we DON'T draw CW faces
} Gfx_Culling_Type;

typedef enum Gfx_Filter_Mode {
  GFX_FILTER_MODE_LINEAR,
  GFX_FILTER_MODE_NEAREST,
} Gfx_Filter_Mode;

typedef enum Gfx_Wrap_Mode {
  GFX_WRAP_MODE_CLAMP,
  GFX_WRAP_MODE_REPEAT,
  GFX_WRAP_MODE_MIRRORED_REPEAT,
#if 0 // FIXME: Emscripten
  GFX_WRAP_MODE_BORDER,
#endif
} Gfx_Wrap_Mode;

typedef struct Gfx_Texture Gfx_Texture;
typedef struct Gfx_Shader  Gfx_Shader;
typedef struct Gfx_Canvas  Gfx_Canvas;

// TODO: Delete this
typedef Gfx_Texture Texture;
typedef Gfx_Shader  Shader;
typedef Gfx_Canvas  Canvas;

typedef uint16_t Gfx_Index_Type;

typedef struct Gfx_Vertex_3D {
  float pos[3];
  float normal[3];
  float uv[2];
  uint32_t color;
} Gfx_Vertex_3D;

// 20 Bytes
typedef struct Gfx_Vertex_2D {
  float pos[3];
  float uv[2];
  uint32_t color;
} Gfx_Vertex_2D;

typedef enum {
  GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT,
  GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT2,
  GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT3,
  GFX_VERTEX_ATTRIBUTE_TYPE_FLOAT4,
  GFX_VERTEX_ATTRIBUTE_TYPE_UBYTE4_NORM,
  GFX_VERTEX_ATTRIBUTE_TYPE_UINT16,
} Gfx_Vertex_Attribute_Type;

typedef enum {
  GFX_VERTEX_ATTRIBUTE_POSITION,
  GFX_VERTEX_ATTRIBUTE_UV,
  GFX_VERTEX_ATTRIBUTE_UV2,
  GFX_VERTEX_ATTRIBUTE_COLOR,
  GFX_VERTEX_ATTRIBUTE_NORMAL,
  GFX_VERTEX_ATTRIBUTE_TANGENT,
  GFX_VERTEX_ATTRIBUTE_BITANGENT,
  GFX_VERTEX_ATTRIBUTE_VERTEX_ID,
  GFX_VERTEX_ATTRIBUTE_COUNT,
} Gfx_Vertex_Attribute_Id;

typedef struct Gfx_Vertex_Attribute {
  Gfx_Vertex_Attribute_Id   id;
  Gfx_Vertex_Attribute_Type type;
  uint32_t offset_in_bytes;
} Gfx_Vertex_Attribute;

typedef size_t Gfx_Handle; // This is for platform specific ptr/data handle

typedef struct Gfx_Texture {
  Gfx_Handle handle;
  Gfx_Texture_Dimension dimension;
  int width;
  int height;
  int format;
  bool is_integer;
  Gfx_Pixel_Format pixel_format;
  char *asset_path;
  int asset_ref_count;
} Gfx_Texture;

typedef struct Gfx_Canvas {
  Gfx_Handle handle;
  int width;
  int height;
  Gfx_Texture texture;
  Gfx_Texture depth_stencil_texture;
} Gfx_Canvas;

typedef struct Gfx_Shader_Uniform {
  const char *name;
  int loc;
} Gfx_Shader_Uniform;

typedef struct Gfx_Shader {
  Gfx_Handle handle;
  int attributes[GFX_VERTEX_ATTRIBUTE_COUNT];
  int uniform_loc_texture;
  int uniform_loc_view;
  int uniform_loc_proj;
  int uniform_loc_matrix;
  Gfx_Shader_Uniform cached_uniforms[128];
  int num_cached_uniforms;
} Gfx_Shader;

typedef struct Gfx_Vertex_Buffer {
  uint16_t handle;
} Gfx_Vertex_Buffer;

typedef struct Gfx_Index_Buffer {
  uint16_t handle;
} Gfx_Index_Buffer;

// ======================== Font ========================
typedef struct Font_Glyph {
  int code;
  float quad[4];
  float advance;
  float x, y;
} Font_Glyph;

typedef struct Font {
  struct Gfx_Texture *texture;
  float height;
  Font_Glyph *glyphs;
  int glyph_count;
  int glyph_capacity;
  float scale;
} Font;

typedef struct Text_Properties {
  uint32_t color;
  bool has_color;
} Text_Properties;

typedef struct Text_Options {
  bool ignore_inline_props : 1;
  bool use_color : 1;
  uint32_t color;
  Text_Properties *text_properties;
} Text_Options;

bool font_is_valid(Font *font);
float font_get_height(Font *font);
float font_get_width(Font *font, const char *text);
bool font_has_glyph(Font *font, int c);

typedef struct Font_Text_Size {
  float width;
  float height;
} Font_Text_Size;

Font_Text_Size font_get_text_size_part(Font *font, const char *text, int length);
bool gfx_skip_text_part_properties(const char *text, int length, int *ptr_i);
int gfx_get_text_length_ignore_properties(const char *text);
void gfx_scroll_text(const char *text, int text_len, float char_time, float dt, int *progress, float *char_timer);

// Style annotations:
// 
//    This is a normal text
//
//   declare property: <property1:value1, property2:value2>
//   reset property:   <>
//
//    This is a line with <#ff0000>red<> text.
//    This part is <#0000ff>blue but this part is <#ff0000>red<>.
//
float font_get_gap(Font *font);
float font_get_width_part(Font *font, const char *text, int length);
void font_load_image(Font *font, const unsigned char *pixels, int w, int h, const char *characters, int spacing);
bool font_load_ttf(Font *font, int font_size, const void *ttf_data, size_t ttf_size);
int font_find_next_line_break(Font *font, const char *text, int width_limit);
int font_find_next_line_break_part(Font *font, const char *text, int length, int width_limit, bool wrap_by_character);
Font_Glyph *font_find_glyph(Font *font, int c);
bool font_get_quad(Font *font, int c, float quad[4]);

void font_imm_text_part_options(Font *font, const char *str, int length, float x, float y, Text_Options opts);
void font_imm_text_part(Font *font, const char *str, int length, float x, float y);
void font_draw_text_part(Font *font, const char *str, int size, float x, float y);
void font_draw_text_part_options(Font *font, const char *str, int length, float x, float y, Text_Options opts);
void font_draw_text(Font *font, const char *text, float x, float y);
typedef void Font_Draw_Cb(void *user, float ox, float oy, float ow, float oh, const float quad[4], uint32_t color);
void font_draw_text_cb(Font *font, const char *str, void *user, Font_Draw_Cb *cb);
void font_draw_text_part_cb(Font *font, const char *str, int length, void *user, Font_Draw_Cb *cb);
void font_draw_text_bordered(Font *font, const char *text, int x, int y);
void font_draw_text_part_bordered(Font *font, const char *text, int size, int x, int y);
void font_draw_text_part_bordered_options(Font *font, const char *text, int size, int x, int y, Text_Options options);

void gfx_set_font(Font *font);
Font *gfx_new_default_font(void);

typedef struct Font_Text_Effect {
  int begin, end;
  uint32_t color;
} Font_Text_Effect;

typedef struct {
  int text_length;
  float wrap_width;
  bool wrap_by_character;
  bool bordered;
  uint32_t border_color; // web color
  bool centered;
  float line_gap;
  bool use_progress;
  int text_progress; // This is different from text_length in that it doesn't affect formatting.
  const Font_Text_Effect *effects;
  int num_effects;
  float force_line_height;

  float *out_total_height;
} Gfx_Text_Ex;

void gfx_text_ex(const char *text, float x, float y, Gfx_Text_Ex ex);
void font_draw_text_ex2(Font *font, const char *text, float x, float y, Gfx_Text_Ex ex);
void gfx_text(const char *text, float x, float y);
void gfx_text_part(const char *text, int len, float x, float y);
void gfx_text_part_bordered(const char *text, int len, float x, float y);
void gfx_text_anchor(const char *text, uint32_t anchor, float x, float y);
float gfx_get_text_width(const char *text);
float gfx_get_text_width_part(const char *text, int len);
float gfx_get_text_height(void);
float gfx_get_text_height_ex(const char *text, Gfx_Text_Ex ex);

// ============================ Gfx_Canvas =====================================
float gfx_get_canvas_width(Gfx_Canvas *canvas);
float gfx_get_canvas_height(Gfx_Canvas *canvas);
Gfx_Canvas *gfx_new_canvas(int width, int height);
Gfx_Canvas *gfx_new_canvas_with_depth_stencil(int width, int height);
Gfx_Canvas *gfx_new_canvas_format(int width, int height, Gfx_Pixel_Format color_format, Gfx_Pixel_Format depth_stencil_format);
Gfx_Texture *gfx_get_canvas_texture(Gfx_Canvas *canvas);
Gfx_Texture *gfx_get_canvas_depth_stencil_texture(Gfx_Canvas *canvas);
void gfx_canvas_bind_color_texture(Gfx_Canvas *canvas, Gfx_Texture *texture, int component);
void gfx_canvas_bind_depth_stencil_texture(Gfx_Canvas *canvas, Gfx_Texture *texture, int component);

// ============================ Depth/Stencil =====================================
void gfx_set_depth_test(Gfx_Depth_Mode mode);
void gfx_clear_depth(float d);
void gfx_set_depth_write(bool write);
void gfx_set_2d_depth(float d); // 
void gfx_clear_stencil(uint8_t value);
void gfx_set_stencil(Gfx_Stencil_Op stencil_op, uint8_t stencil_value);

// ============================ Vertex buffer =====================================
Gfx_Vertex_Buffer gfx_new_vertex_buffer(const Gfx_Vertex_3D *vertices, int num_vertices, const Gfx_Index_Type *indices, int num_indicesoo);
Gfx_Vertex_Buffer gfx_new_vertex_buffer_impl(const void *vertices, int vertex_size, int num_vertices, const void *indices, int index_size, int num_indices, const Gfx_Vertex_Attribute *vertex_attribs, int num_vertex_attribs);
bool gfx_is_vertex_buffer_valid(Gfx_Vertex_Buffer vb);
void gfx_draw_vertex_buffer(Gfx_Vertex_Buffer handle, Gfx_Primitive_Type prim, Gfx_Texture *texture);
void gfx_draw_vertex_buffer_range(Gfx_Vertex_Buffer handle, Gfx_Primitive_Type prim, Gfx_Texture *texture, uint32_t begin, uint32_t end);
void gfx_vertex_buffer_set_vertices(Gfx_Vertex_Buffer vb, const void *vertices, int num_vertices);
void gfx_free_vertex_buffer(Gfx_Vertex_Buffer handle);

// ============================ Gfx =====================================
extern int gfx_width;
extern int gfx_height;
void gfx_setup(int width, int height, void *(*gl_loader)(const char *name));
int gfx_get_viewport_width(void);
int gfx_get_viewport_height(void);
void gfx_set_size(int width, int height);

// ============================ Matrix helpers =====================================
void gfx_matrix_identity(float mat[16]);
void gfx_set_transform(float mat[16]);
void gfx_reset_transform(void);
void gfx_matrix_translate(float mat[16], float x, float y, float z);
void gfx_matrix_rotate_x(float mat[16], float r);
void gfx_matrix_rotate_y(float mat[16], float r);
void gfx_matrix_rotate_z(float mat[16], float r);
void gfx_matrix_scale(float mat[16], float sx, float sy, float sz);
void gfx_matrix_mul_v(float r[4], const float mat[16], const float i[4]);

// ============================ Color =====================================
void gfx_set_color(float r, float g, float b, float a);
void gfx_mult_alpha(float alpha);
void gfx_set_color_u(uint32_t col);
void gfx_set_color_web(uint32_t col);
void gfx_set_color_255(float r, float g, float b, float a);
void gfx_set_color_ignore_alpha_u(uint32_t col);
void gfx_set_color_p(const float *color);
void gfx_set_alpha(float a);
void gfx_get_color(float out_color[4]);
uint32_t gfx_color3_rev(uint32_t c);
uint32_t gfx_color_web(uint32_t c);
uint32_t gfx_color(float r, float g, float b, float a);
uint32_t gfx_get_color_u(void);
float gfx_get_alpha(void);
uint32_t gfx_endian_swap_(uint32_t val);

// ============================ Transform and state Stack =====================================
void gfx_push(void);
void gfx_pop(void);
void gfx_set_2d_projection(void);
void gfx_get_view_projection(float t[16]);
void gfx_get_transform(float t[16]);
void gfx_get_view(float t[16]);
void gfx_set_view(const float mat[16]);
void gfx_get_projection(float t[16]);
void gfx_set_3d_projection(float fov_x_deg, float fov_y_deg, float near, float far);
void gfx_set_all_2d(void);
void gfx_set_orthographic_projection(float width, float height, float near_z, float far_z);
void gfx_apply_transform(const float mat[16]);
void gfx_reset_view(void);
void gfx_transform_point(float *x, float *y);
void gfx_translate3(float tx, float ty, float tz);
void gfx_translate(float tx, float ty);
void gfx_scale3(float sx, float sy, float sz);
void gfx_scale(float sx, float sy);
void gfx_rotate(float r);
void gfx_rotate_y(float r);
void gfx_rotate_x(float r);

// ============================ Immediate buffer type stuff =====================================
void gfx_imm_begin_2d(void);
void gfx_imm_begin_3d(void);
void gfx_imm_begin_custom(int vertex_size, const Gfx_Vertex_Attribute *attributes, int num_attributes);
void gfx_imm_vertex_2d(float x, float y, float u, float v, uint32_t color);
void gfx_imm_vertex_2d_with_z(float x, float y, float z, float u, float v, uint32_t color);
void gfx_imm_vertex_3d(float x, float y, float z, float u, float v, uint32_t color);
void gfx_imm_vertex_3d_impl(Gfx_Vertex_3D vertex);
void gfx_imm_vertex_custom(const void *data, int size);
void gfx_imm_end(void);
void gfx_imm_draw(Gfx_Primitive_Type prim, Gfx_Texture *texture);
void gfx_imm_draw_range(Gfx_Primitive_Type prim, Gfx_Texture *texture, uint32_t begin, uint32_t end);

void gfx_imm_rect(float x, float y, float w, float h);
void gfx_imm_circle(float x, float y, float radius, int segments);
void gfx_imm_q(Gfx_Texture *t, const float quad[4], float x, float y);
void gfx_imm_q_color_u(Gfx_Texture *t, const float quad[4], float x, float y, uint32_t color);
void gfx_imm_trace_circle(float x, float y, float radius, int segments);

void gfx_clear(float r, float g, float b, float a);
void gfx_clear_u(uint32_t color);
void gfx_clear_web(uint32_t color);
void gfx_clear_p(const float color[4]);

// ============================ Misc States =====================================
void gfx_set_culling(Gfx_Culling_Type type);
void gfx_set_blend_mode(int blend_mode);
void gfx_set_wireframe(bool b);
void gfx_set_scissor(float x, float y, float w, float h);
void gfx_set_scissor_p(const float s[4]);
void gfx_unset_scissor(void);
void gfx_viewport(int x, int y, int width, int height);
void gfx_set_smooth_polygon(int smooth);

// ============================ Gfx_Canvas =====================================
void gfx_set_canvas(Gfx_Canvas* canvas);
void gfx_set_canvas_slice(Gfx_Canvas* canvas, int slice);
Gfx_Canvas *gfx_get_canvas(void);
uint32_t gfx_read_canvas_pixel(int x, int y);
void gfx_read_canvas_region(void *dest, int x, int y, int width, int height);
void gfx_read_depth_canvas_region(void *dest, int x, int y, int width, int height);

// ============================ Simple Draws =====================================
//void gfx_draw_vertices(const Gfx_Vertex_3D *vertices, int count, Gfx_Primitive_Type prim, Gfx_Texture *texture);
void gfx_drawq_impl(Gfx_Texture *t, const float quad[4], float x, float y, float r, float sx, float sy, float ox, float oy);
void gfx_drawq(Gfx_Texture *t, const float quad[4], float x, float y);
void gfx_draw_impl(Gfx_Texture *t, float x, float y, float r, float sx, float sy, float ox, float oy);
void gfx_draw(Gfx_Texture *t, float x, float y);
void gfx_line(float x0, float y0, float x1, float y1);
void gfx_line_3d(float x0, float y0, float z0, float x1, float y1, float z1);
void gfx_point(float x, float y);
void gfx_trace_rect(float x, float y, float w, float h);
void gfx_fill_rect(float x, float y, float w, float h);
static void gfx_fill_rect_p(const float data[4]) { gfx_fill_rect(data[0], data[1], data[2], data[3]); }
static void gfx_trace_rect_p(const float data[4]) { gfx_trace_rect(data[0], data[1], data[2], data[3]); }
void gfx_trace_circle_impl(float x, float y, float radius, int segments);
void gfx_trace_circle(float x, float y, float radius);
void gfx_fill_circle_impl(float x, float y, float radius, int segments);
void gfx_fill_circle(float x, float y, float radius);
void gfx_fill_arc_impl(float x, float y, float radius, float angle_a, float angle_b, int segments);
void gfx_fill_arc(float x, float y, float radius, float angle_a, float angle_b);
void gfx_trace_arc(float x, float y, float radius, float angle_a, float angle_b);

// ============================ Gfx_Shader =====================================
void gfx_set_shader(Gfx_Shader *shader);
Gfx_Shader *gfx_get_shader(void);
void gfx_shader_send(const char *name, int type, const void *data, size_t count);
Gfx_Shader *gfx_new_shader(const char *src, size_t size);
Gfx_Shader *gfx_new_default_shader(void);
Gfx_Shader *gfx_new_shader_2(const char *vs, size_t vs_size, const char *ps, size_t ps_size);
void gfx_delete_shader(Gfx_Shader *shader);

// ============================ Gfx_Texture =====================================
void gfx_set_filter_mode(Gfx_Texture *texture, Gfx_Filter_Mode min_mode, Gfx_Filter_Mode mag_mode);
void gfx_set_texture_data(Gfx_Texture *texture, const void *pixels, int width, int height);
Gfx_Texture *gfx_new_texture(const void *data, int width, int height);
void gfx_set_wrap(Gfx_Texture *texture, Gfx_Wrap_Mode x_mode, Gfx_Wrap_Mode y_mode);
// pixels for cube maps are given in the following order:
//  +x, -x, +y, -y, +z, -z
Gfx_Texture *gfx_new_cube_texture(const void *pixels[6], int face_width, int face_height);
Gfx_Texture *gfx_new_cube_texture_format(const void *pixels[6], int face_width, int face_height, Gfx_Pixel_Format format);
Gfx_Texture *gfx_new_texture_format(const void *data, int width, int height, Gfx_Pixel_Format format);
void gfx_free_texture(Gfx_Texture *texture);
void gfx_free_canvas(Gfx_Canvas *canvas);
bool gfx_supports_gl_extension(const char *name);

//======== Compute =====================
bool gfx_supports_compute(void);

#ifdef __cplusplus
} // extern "C"
#endif
