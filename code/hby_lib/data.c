#include "data.h"
#include <ctype.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

Property make_property(const char *name, int host_type, int added_in_version) {
  Property p = {0};
  p.name = name;
  p.host_type = host_type;
  p.added_in_version = added_in_version;
  return p;
}

Data_Reader make_data_reader(const void *ptr, int size) {
  Data_Reader ret = {0};
  ret.ptr = (const char *)ptr;
  ret.end = (const char *)ptr + size;
  return ret;
}

const char *dr_get_front(Data_Reader *dr) {
  return dr->ptr;
}

void dr_advance(Data_Reader *dr, int size) {
  iptr remaining = dr->end - dr->ptr;
  if (size > remaining)
    size = remaining;
  dr->ptr += size;
}

void dr_text_advance(Data_Reader *dr) {
  if (dr_reached_end(dr))
    return;

  if (*dr->ptr == '\n') {
    dr->col = 0;
    dr->line++;
  }
  else {
    dr->col++;
  }
  dr->ptr++;
}

void dr_text_advance_by(Data_Reader *dr, int count) {
  for (int i = 0; i < count; i++)
    dr_text_advance(dr);
}

void dr_text_eat_white_space(Data_Reader *dr) {
  while (!dr_reached_end(dr) && isspace(*dr->ptr)) {
    dr_text_advance(dr);
  }
}

String_View dr_text_eat_line(Data_Reader *dr) {
  String_View ret = {0};
  while (!dr_reached_end(dr)) {
    if (!ret.ptr)
      ret.ptr = (const uint8_t *)dr->ptr;

    if (*dr->ptr == '\n') {
      dr_text_advance(dr);
      break;
    }
    dr_text_advance(dr);
    ret.length++;
  }
  return ret;
}

char dr_text_peek_char(Data_Reader *dr) {
  if (!dr_reached_end(dr))
    return *dr->ptr;
  return '\0';
}

String_View dr_text_peak_token(Data_Reader *dr) {
  Data_Reader old = *dr;
  String_View sv = dr_text_eat_token(dr);
  *dr = old;
  return sv;
}

String_View dr_text_eat_token(Data_Reader *dr) {
  String_View ret = {0};
  dr_text_eat_white_space(dr);
  ret.ptr = (const uint8_t *)dr->ptr;
  while (!dr_reached_end(dr) && !isspace(*dr->ptr)) {
    dr_text_advance(dr);
    ret.length++;
  }
  return ret;
}

bool dr_text_eat_pattern(Data_Reader *dr, String_View pattern) {
  if (dr_remaining(dr) < pattern.length) {
    return false;
  }
  bool ret = str_eq_part((const char *)dr->ptr, pattern.length, (const char *)pattern.ptr, pattern.length);
  if (ret) {
    for (iptr i = 0; i < pattern.length; i++)
      dr_text_advance(dr);
  }
  return ret;
}
bool dr_text_eat_int64(Data_Reader *dr, int64_t *out_val) {
  int64_t number = 0;
  iptr eat_length = 0;
  if (PARSE_SUCCESS != sv_eat_int64(sv_make(dr->ptr, dr_remaining(dr)), &number, &eat_length)) {
    return false;
  }
  *out_val = number;
  dr_text_advance_by(dr, eat_length);
  return true;
}

bool dr_read_magic(Data_Reader *dr, const char *magic) {
  int i = 0;
  iptr magic_size = str_size(magic);
  for (i = 0; i < magic_size; i++) {
    char c = 0;
    if (!dr_read(dr, &c, sizeof(c)))
      return 0;
    if (c != magic[i])
      return 0;
  }
  return 1;
}

iptr dr_remaining(Data_Reader *dr) {
  return dr->end - dr->ptr;
}

bool dr_reached_end(Data_Reader *dr) {
  return dr_remaining(dr) == 0;
}
bool dr_read(Data_Reader *dr, void *ptr, int size) {
  assert(dr->end - dr->ptr >= size);
  if (dr->end - dr->ptr >= size) {
    if (ptr) {
      memcpy(ptr, dr->ptr, size);
    }
    dr->ptr += size;
    return 1;
  }
  return 0;
}


void sb_write_magic(String_Builder *sb, const char *magic) {
  int i = 0;
  iptr len = str_size(magic);
  for (i = 0; i < len; i++) {
    char c = 0;
    if (i < len)
      c = magic[i];
    sb_write(sb, &c, sizeof(c));
  }
}

void sb_write_string_data(String_Builder *sb, const char *string) {
  uint32_t size = (uint32_t)str_size(string);
  sb_write(sb, &size, sizeof(size));
  if (size) {
    char null_term = 0;
    sb_write(sb, string, size);
    sb_write(sb, &null_term, 1);
  }
}

void sb_write_bool(String_Builder *sb, bool val) {
  sb_write(sb, &val, sizeof(val));
}
void sb_write_uint8(String_Builder *sb, uint8_t val) {
  sb_write(sb, &val, sizeof(val));
}
void sb_write_uint16(String_Builder *sb, uint16_t val) {
  sb_write(sb, &val, sizeof(val));
}
void sb_write_uint32(String_Builder *sb, uint32_t val) {
  sb_write(sb, &val, sizeof(val));
}
void sb_write_float(String_Builder *sb, float val) {
  sb_write(sb, &val, sizeof(val));
}
void sb_write_quaternion(String_Builder *sb, Quaternion val) {
  sb_write(sb, &val, sizeof(val));
}
void sb_write_vec4(String_Builder *sb, Vec4 val) {
  sb_write(sb, &val, sizeof(val));
}
void sb_write_vec3(String_Builder *sb, Vec3 val) {
  sb_write(sb, &val, sizeof(val));
}
void sb_write_vec2(String_Builder *sb, Vec2 val) {
  sb_write(sb, &val, sizeof(val));
}

bool dr_read_bool(Data_Reader *dr, bool *val) {
  return dr_read(dr, val, sizeof(*val));
}
bool dr_read_uint8(Data_Reader *dr, uint8_t *val) {
  return dr_read(dr, val, sizeof(*val));
}
bool dr_read_uint16(Data_Reader *dr, uint16_t *val) {
  return dr_read(dr, val, sizeof(*val));
}
bool dr_read_uint32(Data_Reader *dr, uint32_t *val) {
  return dr_read(dr, val, sizeof(*val));
}
bool dr_read_float(Data_Reader *dr, float *val) {
  return dr_read(dr, val, sizeof(*val));
}
bool dr_read_quaternion(Data_Reader *dr, Quaternion *val) {
  return dr_read(dr, val, sizeof(*val));
}
bool dr_read_vec2(Data_Reader *dr, Vec2 *val) {
  return dr_read(dr, val, sizeof(*val));
}
bool dr_read_vec3(Data_Reader *dr, Vec3 *val) {
  return dr_read(dr, val, sizeof(*val));
}
bool dr_read_vec4(Data_Reader *dr, Vec4 *val) {
  return dr_read(dr, val, sizeof(*val));
}
bool dr_read_string_data(Data_Reader *dr, const char **out_string) {
  uint32_t length = 0;
  if (!dr_read(dr, &length, sizeof(length)))
    return 0;

  *out_string = NULL;
  if (!length)
    return 1;

  *out_string = dr->ptr;
  dr->ptr += length+1;
  return 1;
}
bool dr_read_data(Data_Reader *dr, int size, const char **out_data) {
  if (size > dr->end - dr->ptr) {
    return 0;
  }

  *out_data = dr->ptr;
  dr->ptr += size;
  return 1;
}

Property_Info get_property_info(void *host, const Property *p) {

#define PROPERTY_CASE(IS_POD, OFFSET_NAME) \
  else if (p->OFFSET_NAME) { \
    ret.offset = byte_ptr+(size_t)p->OFFSET_NAME; \
    ret.is_pod = IS_POD; \
    if (IS_POD) { \
      ret.pod_size = sizeof(*p->OFFSET_NAME); \
    } \
  }

  Property_Info ret = {0};
  char *byte_ptr = (char *)host;
  if (0) {}
  PROPERTY_CASE(true,  uint32_offset)
  PROPERTY_CASE(true,  uint16_offset)
  PROPERTY_CASE(true,  uint8_offset)
  PROPERTY_CASE(true,  int32_offset)
  PROPERTY_CASE(true,  int16_offset)
  PROPERTY_CASE(true,  int8_offset)
  PROPERTY_CASE(true,  float_offset)
  PROPERTY_CASE(true,  vec4_offset)
  PROPERTY_CASE(true,  vec3_offset)
  PROPERTY_CASE(true,  vec2_offset)
  PROPERTY_CASE(true,  bool_offset)
  PROPERTY_CASE(false, string_offset)
  else {
    PANIC("Unimplemented property type.");
  }
#undef PROPERTY_CASE

  ret.offset = (char *)ret.offset - 0x100;
  return ret;
}

int get_property_pod_size(const Property *p) {
  return get_property_info(NULL, p).pod_size;
}

void *get_property_ptr(void *host, const Property *p) {
  return get_property_info(host, p).offset;
}

bool is_property_pod(const Property *p) {
  return get_property_info(NULL, p).is_pod;
}

void write_properties(String_Builder *sb, void *host, int type, const Property *properties, int num_properties) {
  for (int i = 0; i < num_properties; i++) {
    Property p = properties[i];
    if (p.host_type && p.host_type != type)
      continue;
    if (p.removed_in_version)
      continue;

    if (is_property_pod(&p)) {
      sb_write(sb, get_property_ptr(host, &p), get_property_pod_size(&p));
    }
    else if (p.string_offset) {
      char **ptr = (char **)get_property_ptr(host, &p);
      sb_write_string_data(sb, *ptr);
    }
  }
}

bool read_properties(Data_Reader *dr, int version, void *host, int type, const Property *properties, int num_properties) {
  for (int j = 0; j < num_properties; j++) {
    Property p = properties[j];
    if (p.added_in_version && version < p.added_in_version)
      continue;
    if (p.removed_in_version && version >= p.removed_in_version)
      continue;
    if (p.host_type && p.host_type != type)
      continue;

    if (is_property_pod(&p)) {
      void *dst = NULL;
      if (host) dst = get_property_ptr(host, &p);
      if (!dr_read(dr, dst, get_property_pod_size(&p))) {
        return read_error("Failed to read property %s\n", p.name);
      }
    }
    else if (p.string_offset) {
      const char *string = NULL;
      if (!dr_read_string_data(dr, &string))
        return read_error("Failed to read string property %s\n", p.name);
      if (host) {
        char **ptr = (char **)get_property_ptr(host, &p);
        if (*ptr)
          str_free(*ptr);
        *ptr = str_copy(string);
      }
    }
  }
  return true;
}

#ifdef __cplusplus
} // extern "C"
#endif

