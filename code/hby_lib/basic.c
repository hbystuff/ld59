///////////////////////////////////////////////////////////////////////////////
// HBY's basic library.
//
// - integer typedefs
// - inline and static assert macros
// - dynamic array (arr_*)
// - string helpers (str_*)
// - string builder
// - allocator interface
// - temporary arena allocator
//
// As a rule, anything suffixed with _a takes in an allocator.
//
///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <ctype.h>
#include "basic.h"

#ifdef __cplusplus
extern "C" {
#endif

void arr_free_(void *ptr) {
  Array_Header *header = (Array_Header *)ptr - 1;
  free_a(header, header->allocator);
}

Array_Header arr_debug_get_header(void *arg) {
  Array_Header ret = {0};
  if (arg) {
    ret = *((Array_Header *)arg - 1);
  }
  return ret;
}

void arr_assert_(int cond) {
  assert(cond);
}

 void arr_remove_(void* ptr, iptr elem_size, iptr i) {
  Array_Header *header = (Array_Header *)ptr - 1;
  if (i+1 < header->size)
    memmove((char *)ptr + ((i)*elem_size), (char *)ptr + ((i+1)*elem_size), (header->size-i-1) * elem_size);
  header->size--;
}

 void arr_resize_(void **ptr, iptr elem_size, iptr new_size, Allocator allocator) {
  if (!*ptr) {
    Array_Header *header = NULL;
    iptr capacity = new_size;
    if (capacity < 10)
      capacity = 10;
    header = (Array_Header *)realloc_a(NULL, sizeof(Array_Header) + capacity * elem_size, allocator);
    header->allocator = allocator;
    header->size = 0;
    header->capacity = capacity;
    header->size = new_size;
    *ptr = header+1;
  }
  else {
    Array_Header *header = (Array_Header *)*ptr - 1;
    iptr new_capacity;
    if (header->capacity >= new_size) {
      header->size = new_size;
      return;
    }

    new_capacity = new_size;
    header = (Array_Header *)realloc_a(header, sizeof(Array_Header) + new_capacity * elem_size, header->allocator);
    header->capacity = new_capacity;
    header->size = new_size;
    *ptr = header+1;
  }
}

 void arr_reserve_(void **pptr, iptr elem_size, iptr capacity, Allocator allocator) {
  iptr new_alloc_size_in_bytes;
  void *ptr;
  Array_Header *header = NULL;
  if (capacity <= 0)
    return;

  new_alloc_size_in_bytes = capacity * elem_size + sizeof(Array_Header);

  ptr = *pptr;
  if (ptr) {
    header = (Array_Header *)ptr - 1;
    header = (Array_Header *)realloc_a(header, new_alloc_size_in_bytes, header->allocator);
    header->capacity = capacity;
  }
  else {
    header = (Array_Header *)realloc_a(header, new_alloc_size_in_bytes, allocator);
    header->capacity = capacity;
    header->size = 0;
    header->allocator = allocator;
  }

  *pptr = header+1;
}

 void arr_remove_unordered_(void *ptr, iptr elem_size, iptr i) {
  Array_Header *header = (Array_Header *)ptr - 1;
  arr_assert_(header->size > 0);
  if (i+1 < header->size)
    memcpy((char*)ptr + i*elem_size, (char*)ptr + (header->size-1)*elem_size, elem_size);
  header->size--;
}

 void arr_sort_impl_(void *ptr, iptr count, iptr elem_size, Array_Cmp *cmp) {
  if (!count)
    return;
  qsort(ptr, count, elem_size, cmp);
}

 bool arr_grow_(void **pptr, iptr elem_size, iptr i, Allocator allocator) {
  void *ptr;
  Array_Header *header;
  iptr old_capacity = arr_capacity(*pptr);
  if (arr_size(*pptr) >= old_capacity) {
    arr_reserve_(pptr, elem_size, old_capacity ? old_capacity * 2 : 10, allocator);
  }

  ptr = *pptr;
  header = (Array_Header *)ptr - 1;

  assert(i >= 0);
  ptr = header+1;
  if (i < header->size) {
    iptr src_offset_in_bytes = i     * elem_size;
    iptr dst_offset_in_bytes = (i+1) * elem_size;

    const void *src = (char *)ptr + src_offset_in_bytes;
    void       *dst = (char *)ptr + dst_offset_in_bytes;
    memmove(dst, src, (header->size-i) * elem_size);
  }
  header->size++;

  return 1;
}

static void *default_allocator_proc(void *ptr, iptr size, void *arg) {
  if (ptr && !size) {
    free(ptr);
    return NULL;
  }
  return realloc(ptr, size);
}

static void ensure_ptr_a(Allocator *a) {
  if (!a->proc) {
    a->proc = default_allocator_proc;
  }
}

void *realloc_a(void *ptr, iptr size, Allocator a) {
  ensure_ptr_a(&a);
  return a.proc(ptr, size, a.arg);
}
void free_a(void *ptr, Allocator a) {
  ensure_ptr_a(&a);
  a.proc(ptr, 0, a.arg);
}

Temp_Memory *new_temp_memory_a(iptr capacity, Allocator allocator) {
  iptr total_size = (iptr)sizeof(Temp_Memory) + capacity;
  Temp_Memory *tm = (Temp_Memory *)realloc_a(NULL, total_size, allocator);
  ZERO(*tm);
  tm->capacity = capacity;
  return tm;
}

Temp_Memory *new_temp_memory(iptr capacity) {
  return new_temp_memory_a(capacity, default_a);
}

static void *temp_allocator_proc(void *ptr, iptr size, void *arg) {
  Temp_Memory *tm = (Temp_Memory *)arg;
  return tm_realloc_or_free(tm, ptr, size);
}

Allocator get_temporary_allocator(Temp_Memory *tm) {
  Allocator ret = {0};
  ret.arg = tm;
  ret.proc = temp_allocator_proc;
  return ret;
}

INLINE static iptr align_to(iptr size, iptr alignment) {
  iptr rem = size % alignment;
  if (rem)
    size += alignment - rem;
  return size;
}

char *tm_get_ptr(Temp_Memory* tm) {
  return (char *)(&tm[1]);
}
const char *tm_get_ptr_const(Temp_Memory* tm) {
  return (const char *)(&tm[1]);
}

size_t tm_get_node_offset(Temp_Memory* tm, Temp_Memory_Node* node) {
  return (size_t)((const char *)node - tm_get_ptr_const(tm));
}

void *tm_realloc_or_free(Temp_Memory *tm, void *ptr, iptr size) {
  iptr size_needed;
  if (size == 0) {
    // TODO: try to walk back the memory if it was the last allocated.
    return NULL;
  }

  size_needed = sizeof(Temp_Memory_Node) + size;
  if (ptr) {
    Temp_Memory_Node *node = (Temp_Memory_Node *)ptr - 1;
    if (node->id > 0) {
      if (size <= node->size)
        return ptr;

      size_t new_end = align_to(tm_get_node_offset(tm, node) + size_needed, sizeof(uint64_t));
      if (node->id >= tm->max_id && new_end <= tm->capacity) {
        node->size = size;
        tm->cursor = new_end;
        return ptr;
      }
    }
  }

  tm->watermark = tm->cursor + size_needed;
  if (tm->cursor + size_needed <= tm->capacity) {
    Temp_Memory_Node *node = (Temp_Memory_Node *)((const char *)&tm[1] + tm->cursor);
    ZERO(*node);
    node->size = size;
    node->id = ++tm->max_id;
    tm->cursor += size_needed;
    tm->cursor = align_to(tm->cursor, sizeof(uint64_t));
    void *ret = node+1;
    if (ptr) {
      Temp_Memory_Node *old_node = (Temp_Memory_Node *)ptr - 1;
      assert(old_node->size <= node->size);
      memcpy(ret, ptr, old_node->size);
    }
    return ret;
  }

  {
    // Fall back to default allocator
    Temp_Memory_Node *node = (Temp_Memory_Node *)malloc(size_needed);
    fprintf(stderr, "Warning: falling back to default allocator because temporary allocator ran out of space.\n");
    if (!node)
      return NULL;
    ZERO(*node);
    node->next = tm->fallback_nodes;
    tm->fallback_nodes = node;
    return node+1;
  }
}

void tm_reset(Temp_Memory *tm) {
  Temp_Memory_Node *node = tm->fallback_nodes;
  tm->cursor = 0;
  tm->max_id = 0;
  while (node) {
    Temp_Memory_Node *next = node->next;
    free(node);
    node = next;
  }
  tm->fallback_nodes = NULL;
}

bool str_split(const char **str, const char *separator, const char **out_token, iptr *out_len) {
  iptr len;
  const char *found;
  iptr separator_len;

  if (!*str || !**str)
    return 0;

  len = str_size(*str);

  found = strstr(*str, separator);
  if (!found) {
    *out_token = *str;
    *out_len = len;
    *str = *str + len;
    return 1;
  }

  separator_len = str_size(separator);

  *out_token = *str;
  *out_len = found - *str;
  *str = found + separator_len;

  return 1;
}

char *str_copy(const char *str) {
  return (char *)str_copy_a(str, default_a);
}
void str_free(char *a) {
  if (!a)
    return;
  free_a(a, default_a);
}
char *str_concat(const char *a, const char *b) {
  return str_concat_a(a, b, default_a);
}
char *str_reserve(char *a, int size) {
  return str_reserve_a(a, size, default_a);
}

bool str_contains(const char* x, const char* sub) {
  String_View x_sv = sv_make(x, str_len(x));
  String_View sub_sv = sv_make(sub, str_len(sub));
  return sv_contains(x_sv, sub_sv);
}

char *str_reserve_a(char *x, iptr size, Allocator a) {
  char *ret = 0;
  size = size < 63 ? 63 : size;
  ret = (char *)realloc_a(x, size+1, a);
  if (!x)
    ret[0] = 0;
  return ret;
}
char *str_copy_a(const char *str, Allocator a) {
  iptr size = 0;
  char *ret = 0;
  if (!str) return NULL;
  size = strlen(str);
  ret = str_reserve_a(NULL, size, a);
  memcpy(ret, str, size);
  ret[size] = 0;
  return ret;
}
char *str_concat_a(const char *x, const char *y, Allocator a) {
  iptr size_a = str_size(x);
  iptr size_b = str_size(y);
  iptr final_size = 0;
  char *ret = 0;
  if (!size_a && !size_b)
    return NULL;

  final_size = size_a + size_b;
  ret = str_reserve_a(NULL, final_size, a);
  memcpy(ret, x, size_a);
  memcpy(ret + size_a, y, size_b);
  ret[final_size] = 0;

  return ret;
}
void str_free_a(char *x, Allocator a) {
  free_a(x, a);
}

int str_empty(const char *a) {
  return !a || a[0] == 0;
}
iptr str_size(const char *str) {
  if (!str) return 0;
  return (iptr)strlen(str);
}
iptr str_len(const char *str) {
  if (!str) return 0;
  return (iptr)strlen(str);
}
iptr str_size_part(const char *str, iptr size) {
  iptr i = 0;
  for (i = 0; i < size; i++) {
    if (!str[i])
      return i;
  }
  return size;
}

int str_eq_part(const char *x, iptr asize, const char *y, iptr bsize) {
  if (x == y) return 1;
  if (!x || !y) return 0;
  if (asize != bsize) return 0;
  return !memcmp(x, y, asize);
}

char *str_copy_part(const char *str, iptr size) {
  return str_copy_part_a(str, size, default_a);
}

char *str_copy_part_a(const char *str, iptr size, Allocator a) {
  if (size == 0)
    return NULL;
  char *buf = (char *)realloc_a(NULL, size + 1, a);
  memcpy(buf, str, size);
  buf[size] = 0;
  return buf;
}

int str_eq(const char *x, const char *y) {
  if (x == y) return 1;
  if (!x || !y) return 0;
  return !strcmp(x, y);
}
int str_ends_with(const char *x, const char *sub) {
  if (str_len(x) < str_len(sub)) return 0;
  return !memcmp(x+str_len(x)-str_len(sub), sub, str_len(sub));
}
int str_begins_with(const char *x, const char *prefix) {
  if (str_len(x) < str_len(prefix)) return 0;
  return !memcmp(x, prefix, str_len(prefix));
}


//================================ String builder ====================================

bool sb_reserve(String_Builder *sb, iptr capacity_needed) {
  if (capacity_needed > sb->capacity) {
    char *new_ptr = 0;
    while (capacity_needed > sb->capacity) {
      sb->capacity = sb->capacity ? sb->capacity * 2 : 63;
    }
    new_ptr = (char *)realloc_a(sb->ptr, sb->capacity+1, sb->allocator);
    if (!new_ptr) {
      return false;
    }
    sb->ptr = new_ptr;
  }
  return true;
}

void sb_write_property(String_Builder *sb, uint32_t property, const void *ptr, uint32_t size) {
  sb_write_u32(sb, property);
  sb_write_u32(sb, size);
  sb_write(sb, ptr, size);
}

bool sb_resize(String_Builder *sb, iptr size) {
  if (size > sb->capacity) {
    if (!sb_reserve(sb, sb->size + size))
      return false;
  }

  sb->size = size;
  sb->ptr[size] = 0;

  return true;
}

int sb_write(String_Builder *sb, const void *ptr, iptr size) {
  if (!sb_reserve(sb, sb->size + size))
    return 0;
  memcpy(sb->ptr + sb->size, ptr, size);
  sb->size += (int)size;
  sb->ptr[sb->size] = 0;
  return (int)size;
}

int sb_add(String_Builder *sb, char c) {
  if (!sb_reserve(sb, sb->size + 1))
    return 0;
  sb->ptr[sb->size] = c;
  sb->size += 1;
  sb->ptr[sb->size] = 0;
  return 1;
}

int sb_write_str(String_Builder *sb, const char *str) {
  return sb_write(sb, str, strlen(str));
}

char *sb_detach(String_Builder *sb) {
  char *ret = sb->ptr;
  sb->size = 0;
  sb->capacity = 0;
  sb->ptr = NULL;
  return ret;
}

bool sb_insert_at(String_Builder *sb, iptr index, const char *buf, iptr buf_length) {
  iptr required_capacity = 0;
  if (!buf_length)
    return 1;
  required_capacity = sb->size + buf_length;
  sb_reserve(sb, required_capacity);
  if (sb->capacity < required_capacity)
    return 0;
  memmove(sb->ptr+index+buf_length, sb->ptr+index, buf_length);
  memcpy(sb->ptr+index, buf, buf_length);
  sb->size += (int)buf_length;
  sb->ptr[sb->size] = 0;
  return 1;
}

void sb_remove_at(String_Builder *sb, iptr index) {
  if (index < 0 || index >= sb->size)
    return;
  if (index+1 < sb->size) {
    memmove(sb->ptr + index, sb->ptr+index+1, sb->size - index /*including the null terminator*/);
  }
  sb->size--;
}

void sb_destroy(String_Builder *sb) {
  str_free_a(sb->ptr, sb->allocator);
  ZERO(*sb);
}

String_Bank_Id sb_bank_add_string_part(String_Builder *sb, const char *str, int len) {
  String_Bank_Id ret = {0};
  int offset = 0;
  if (1 != sb_add(sb, '\0'))
    return ret;
  offset = sb->size;
  if (len != sb_write(sb, str, len))
    return ret;

  ret.value = offset+1;
  return ret;
}

String_Bank_Id sb_bank_vformat(String_Builder *sb, const char *fmt, va_list args) {
  String_Bank_Id ret = {0};
  int offset = 0;
  if (1 != sb_add(sb, '\0'))
    return ret;

  offset = sb->size;
  if (0 == sb_vformat(sb, fmt, args))
    return ret;

  ret.value = offset+1;
  return ret;
}

String_Bank_Id sb_bank_format(String_Builder *sb, const char *fmt, ...) {
  String_Bank_Id ret = {0};

  va_list args;
  va_start(args, fmt);
  ret = sb_bank_vformat(sb, fmt, args);
  va_end(args);

  return ret;
}

const char *sb_bank_get(String_Builder *sb, String_Bank_Id id) {
  if (sb->size <= 0)
    return NULL;
  if (id.value <= 0 || id.value-1 >= sb->size)
    return NULL;
  return sb->ptr+id.value-1;
}

String_View sb_bank_get_sv(String_Builder *sb, String_Bank_Id id) {
  return svcstr(sb_bank_get(sb, id));
}

int sb_vformat(String_Builder *sb, const char *fmt, va_list args) {
  int size = stbsp_vsnprintf(NULL, 0, fmt, args);
  if (!sb_reserve(sb, sb->size + size))
    return 0;

  stbsp_vsnprintf(sb->ptr + sb->size, size+1, fmt, args);
  sb->size += size;
  return size;
}

void sb_clear(String_Builder *sb) {
  if (sb->ptr && sb->capacity) {
    sb->ptr[0] = 0;
  }
  sb->size = 0;
}

int sb_format(String_Builder *sb, const char *fmt, ...) {
  int size = 0;

  va_list args;
  va_start(args, fmt);
  size = sb_vformat(sb, fmt, args);
  va_end(args);

  return size;
}

uint32_t sb_write_f32(String_Builder *sb, float val) {
  uint32_t off = 0;
  sb_write(sb, &val, sizeof(val));
  off = (uint32_t)sb->size;
  return off;
}
uint32_t sb_write_u32(String_Builder *sb, uint32_t val) {
  uint32_t off = 0;
  sb_write(sb, &val, sizeof(val));
  off = (uint32_t)sb->size;
  return off;
}

uint32_t sb_mark_u32(String_Builder *sb) {
  uint32_t off = 0;
  sb_write_u32(sb, -1);
  off = sb->size;
  return off;
}

void sb_patch_u32_value(String_Builder* sb, uint32_t off, uint32_t value) {
  memcpy(sb->ptr + off - sizeof(off), &value, sizeof(value));
}

void sb_patch_u32(String_Builder *sb, uint32_t off) {
  uint32_t size = sb->size - off;
  sb_patch_u32_value(sb, off, size);
}

bool assert_impl_(bool cond) {
#ifndef NDEBUG
  if (!cond) {
    fprintf(stderr, "Assertion failed!\n");
    abort();
  }
#endif
  return cond;
}

bool assert_log_impl_(bool cond, const char *fmt, ...) {
#ifndef NDEBUG
  if (!cond) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    abort();
  }
#endif
  return cond;
}

String_View sv_make(const char *ptr, iptr length) {
  String_View view = {0};
  view.ptr = (const uint8_t *)ptr;
  view.length = length;
  return view;
}
char *sv_to_cstr(String_View s) {
  return str_copy_part((char *)s.ptr, s.length);
}
String_View sv_from_cstr(const char *str) {
  String_View ret = {0};
  ret.ptr = (uint8_t *)str;
  ret.length = str_size(str);
  return ret;
}
bool sv_eq(String_View a, String_View b) {
  return str_eq_part((const char *)a.ptr, a.length, (const char *)b.ptr, b.length);
}
bool sv_starts_with(String_View s, String_View prefix) {
  if (prefix.length <= s.length) {
    return !memcmp(prefix.ptr, s.ptr, prefix.length);
  }
  return false;
}
bool sv_ends_with(String_View s, String_View suffix) {
  if (suffix.length <= s.length) {
    return !memcmp(suffix.ptr, s.ptr + s.length - suffix.length, suffix.length);
  }
  return false;
}

String_View sv_pop_back(String_View s, iptr num) {
  iptr num_to_pop = MIN(s.length, num);
  s.length -= num_to_pop;
  return s;
}
String_View sv_pop_front(String_View s, iptr num) {
  iptr num_to_pop = MIN(s.length, num);
  s.ptr += num_to_pop;
  s.length -= num_to_pop;
  return s;
}

String_View sv_trim(String_View s) {
  while (s.length && isspace((char)s.ptr[0])) {
    s.ptr++;
    s.length--;
  }
  while (s.length && isspace((char)s.ptr[s.length-1])) {
    s.length--;
  }
  return s;
}

String_View sv_substr(String_View s, iptr begin, iptr length) {
  String_View ret = {0};
  if (begin < 0 || begin >= s.length) return ret;
  if (length == 0) return ret;
  if (length > 0 && begin+length > s.length) return ret;
  ret.ptr = s.ptr + begin;
  ret.length = length < 0 ? (s.length-begin) : length;
  return ret;
}

bool sv_contains(String_View s, String_View pattern) {
  return sv_find(s, pattern) != -1;
}
iptr sv_find(String_View s, String_View pattern) {
  if (pattern.length > s.length)
    return -1;

  for (iptr i = 0; i+pattern.length <= s.length; i++) {
    if (sv_eq(sv_substr(s, i, pattern.length), pattern))
      return i;
  }
  return -1;
}

Parse_Status sv_eat_int64(String_View view, int64_t *out_num, iptr *out_eat_length) {
  char *num_end = NULL;
  int64_t num = 0;
  int i = 0;
  int is_neg = 0;
  int64_t result = 0;

  const char *str = (const char *)view.ptr;
  iptr len = view.length;

  const int64_t max_num_abs = 0x7FFFFFFFFFFFFFFF;
  const int64_t min_num_abs = 0x8000000000000000 ;

  if (!len) {
    return PARSE_ERROR_GENERIC;
  }

  i = 0;
  if (str[0] == '-') {
    is_neg = 1;
    i++;
  }
  for (; i < len; i++) {
    const char c = str[i];
    if (c >= '0' && c <= '9') {
      int increment = c - '0';

      if (!is_neg && result > max_num_abs/10) {
        return PARSE_ERROR_RANGE;
      } else if (is_neg && -result < min_num_abs/10) {
        return PARSE_ERROR_RANGE;
      }

      result *= 10;

      if (!is_neg && result > max_num_abs-increment) {
        return PARSE_ERROR_RANGE;
      } else if (is_neg && -result < min_num_abs+increment) {
        return PARSE_ERROR_RANGE;
      }
      result += increment;
    } else {
      break;
    }
  }

  if (is_neg) result *= -1;
  *out_num = result;
  *out_eat_length = i;

  return PARSE_SUCCESS;
}

Parse_Status sv_to_int64(String_View view, int64_t *out_num) {
  int64_t num = 0;
  iptr eat_length = 0;
  Parse_Status status = PARSE_SUCCESS;
  if (PARSE_SUCCESS != (status = sv_eat_int64(view, &num, &eat_length)))
    return status;

  if (eat_length != view.length)
    return PARSE_ERROR_GENERIC;

  *out_num = num;
  return PARSE_SUCCESS;
}

Parse_Status str_to_int64(const char *str, int64_t *out_num) {
  return sv_to_int64(sv_from_cstr(str), out_num);
}

String string_from_cstr_a(const char *cstr, Allocator a) {
  String ret = {0};
  ret.length = str_len(cstr);
  ret.ptr = (uint8_t *)str_copy_part_a(cstr, ret.length, a);
  return ret;
}


char *vprintf_a(Allocator a, const char *fmt, va_list args) {
  int size = stbsp_vsnprintf(NULL, 0, fmt, args);
  char *ret = (char *)realloc_a(NULL, sizeof(char) * (size+1), a);
  stbsp_vsnprintf(ret, size+1, fmt, args);
  return ret;
}
char *printf_a(Allocator a, const char *fmt, ...) {
  char *ret = NULL;
  va_list args;
  va_start(args, fmt);
  ret = vprintf_a(a, fmt, args);
  va_end(args);
  return ret;
}

char *mvprintf(const char *fmt, va_list args) {
  return vprintf_a(default_a, fmt, args);
}
char *mprintf(const char *fmt, ...) {
  char *ret = NULL;
  va_list args;
  va_start(args, fmt);
  ret = mvprintf(fmt, args);
  va_end(args);
  return ret;
}

#ifdef __cplusplus
} // extern "C"
#endif

