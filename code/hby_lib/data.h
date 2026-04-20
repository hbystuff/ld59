#pragma once

#include <stdint.h>
#include "vec.h"
#include "basic.h"

#ifdef _MSC_VER // {

#if _MSC_VER >= 1800 // {
#include <stdbool.h>
#define DATA_BOOL_SUPPORTED
#endif // }

// }
#else // {

#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#define DATA_BOOL_SUPPORTED
#endif
#endif

#endif // }

#define read_error(fmt, ...) (fprintf(stderr, fmt, ## __VA_ARGS__), 0)

#ifdef __cplusplus
extern "C" {
#endif

#define PROPERTY_OFFSET(T, MEMBER) (&((T *)0x100)->MEMBER)

typedef struct Enum_Info {
  const char *name;
  int64_t value;
} Enum_Info;

enum {
  PROPERTY_FLAG_IS_FLAG  = 0x1,
  PROPERTY_FLAG_IS_COLOR = 0x8,
  PROPERTY_FLAG_COLOR_NO_ALPHA = 0x10,

  PROPERTY_FLAG_CUSTOM_BEGIN = 0x20
};

typedef struct Property {
  const char *name;
  float     *float_offset;
  int32_t   *int32_offset;
  int16_t   *int16_offset;
  int8_t    *int8_offset;
  uint32_t  *uint32_offset;
  uint16_t  *uint16_offset;
  uint8_t   *uint8_offset;
#ifdef DATA_BOOL_SUPPORTED
  bool      *bool_offset;
#endif
  struct Vec4  *vec4_offset;
  struct Vec3  *vec3_offset;
  struct Vec2  *vec2_offset;
  char        **string_offset;

  // Other things.
  uint32_t flags;
  uint64_t flags_to_save;
  uint32_t custom_flags;
  int added_in_version;
  int removed_in_version;
  const Enum_Info *enum_values;
  int num_enum_values;
  int host_type;
  int client_type;
} Property;

typedef struct Property_Info {
  bool is_pod;
  void *offset;
  int pod_size;
} Property_Info;

Property_Info get_property_info(void *host, const Property *p);

int get_property_pod_size(const Property *p);
void *get_property_ptr(void *host, const Property *p);
bool is_property_pod(const Property *p);

Property make_property(const char *name, int host_type, int added_in_version);

typedef struct String_Builder String_Builder;

void sb_write_magic(String_Builder *sb, const char *magic);
void sb_write_string_data(String_Builder *sb, const char *string);
void sb_write_bool(String_Builder *sb, bool val);
void sb_write_uint8(String_Builder *sb, uint8_t val);
void sb_write_uint16(String_Builder *sb, uint16_t val);
void sb_write_uint32(String_Builder *sb, uint32_t val);
void sb_write_float(String_Builder *sb, float val);
void sb_write_quaternion(String_Builder *sb, Quaternion val);
void sb_write_vec2(String_Builder *sb, Vec2 val);
void sb_write_vec3(String_Builder *sb, Vec3 val);
void sb_write_vec4(String_Builder *sb, Vec4 val);

typedef struct Data_Reader {
  const char *ptr;
  const char *end;
  int line, col;
  bool plain_text;
} Data_Reader;

Data_Reader make_data_reader(const void *ptr, int size);
const char *dr_get_front(Data_Reader *dr);
bool dr_reached_end(Data_Reader *dr);
bool dr_read_magic(Data_Reader *dr, const char *magic);
bool dr_read_bool(Data_Reader *dr, bool *val);
bool dr_read_uint8(Data_Reader *dr, uint8_t *val);
bool dr_read(Data_Reader *dr, void *ptr, int size);
bool dr_read_uint16(Data_Reader *dr, uint16_t *val);
bool dr_read_uint32(Data_Reader *dr, uint32_t *val);
bool dr_read_float(Data_Reader *dr, float *val);
bool dr_read_quaternion(Data_Reader *dr, Quaternion *val);
bool dr_read_vec2(Data_Reader *dr, Vec2 *val);
bool dr_read_vec3(Data_Reader *dr, Vec3 *val);
bool dr_read_vec4(Data_Reader *dr, Vec4 *val);
bool dr_read_string_data(Data_Reader *dr, const char **out_string);
bool dr_read_data(Data_Reader *dr, int size, const char **out_data);
void dr_advance(Data_Reader *dr, int size);
iptr dr_remaining(Data_Reader *dr);

char dr_text_peek_char(Data_Reader *dr);
bool dr_text_eat_pattern(Data_Reader *dr, String_View pattern);
void dr_text_eat_white_space(Data_Reader *dr);
void dr_text_advance(Data_Reader *dr);
String_View dr_text_eat_line(Data_Reader *dr);
String_View dr_text_eat_token(Data_Reader *dr);
String_View dr_text_peak_token(Data_Reader *dr);
bool dr_text_eat_int64(Data_Reader *dr, int64_t *out_val);


#define DR_ERROR(str) do { printf("Read error: %s\n", str); return false; } while(0)
#define DR_READ_MAGIC(dr, magic) do { if (!dr_read_magic(dr, magic)) { DR_ERROR(magic); } } while(0)
#define DR_READ_UINT32(dr, ptr) do { if (!dr_read_uint32(dr, ptr)) { DR_ERROR(#ptr); } } while(0)
#define DR_READ_BOOL(dr, ptr) do { if (!dr_read_bool(dr, ptr)) { DR_ERROR(#ptr); } } while(0)
#define DR_READ_VEC2(dr, ptr) do { if (!dr_read_vec2(dr, ptr)) { DR_ERROR(#ptr); } } while(0)

void write_properties(String_Builder *sb, void *host, int type, const Property *properties, int num_properties);
bool read_properties(Data_Reader *dr, int version, void *host, int type, const Property *properties, int num_properties);

#ifdef __cplusplus
} // extern "C"
#endif
