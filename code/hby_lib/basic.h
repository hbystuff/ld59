#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "stb/stb_snprintf.h"

typedef uint8_t  u8;
typedef int8_t   i8;
typedef uint16_t u16;
typedef int16_t  i16;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint64_t u64;
typedef int64_t  i64;
typedef ptrdiff_t iptr;
typedef size_t    uptr;

typedef size_t                 uptr;
typedef ptrdiff_t              iptr;

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#define INLINE __forceinline
#else
#define INLINE inline
#endif

#ifndef _countof
#define _countof(a) ((sizeof(a) / sizeof((a)[0])))
#endif

#ifdef __cplusplus
#define STATIC_ASSERT(cond, msg) static_assert(cond, msg);
#else

#if __STDC_VERSION__ > 201112L
#define STATIC_ASSERT(...) _Static_assert(__VA_ARGS__);
#else
#define STATIC_ASSERT_JOIN2(a, b) a ## b
#define STATIC_ASSERT_JOIN(a, b) STATIC_ASSERT_JOIN2(a, b)
#define STATIC_ASSERT(cond, msg) static const char *STATIC_ASSERT_JOIN(BASIC_STATIC_ASSERT__, __LINE__)[(cond) * 2 - 1] = { msg };
#endif

#endif

#if defined(_MSC_VER)
#define PANIC(fmt, ...) (fprintf(stderr, fmt, ## __VA_ARGS__), __debugbreak());
#else
#define PANIC(fmt, ...) (fprintf(stderr, fmt, ## __VA_ARGS__), __builtin_trap());

#endif

STATIC_ASSERT(sizeof(iptr)  == sizeof(void*), "iptr's size is not pointer sized");
STATIC_ASSERT(sizeof(uptr)  == sizeof(void*), "uptr's size is not pointer sized");

#define SET_FLAG(lvalue, cond, flag) do { if (cond) { lvalue |= (flag); } else { lvalue &= ~(flag); } } while(0)
#define SET_BIT(lvalue, cond, bit) SET_FLAG(lvalue, cond, (1<<(bit)))
#define FLIP_FLAG(lvalue, flag) do { if ((lvalue) & (flag)) { lvalue &= ~(flag); } else { lvalue |= (flag); } } while(0)

#ifndef C_LINKAGE
#ifdef __cplusplus
#define C_LINKAGE extern "C"
#else
#define C_LINKAGE
#endif
#endif

#define SWAP(a, b) do { \
  char TMP__STORAGE__[sizeof(*(a)) == sizeof(*(b)) ? sizeof(*(a)) : -1]; \
  memcpy(TMP__STORAGE__, (a), sizeof(*(a))); \
  memcpy((a), (b), sizeof(*(a))); \
  memcpy((b), TMP__STORAGE__, sizeof(*(a))); \
} while(0)

#define UNREACHABLE(...) do { fprintf(stderr, "UNREACHABLE: %s(%d)\n", __FILE__ ,__LINE__); abort(); } while(0)

bool assert_impl_(bool cond);
bool assert_log_impl_(bool cond, const char *fmt, ...);
#define ASSERT(cond, ...) assert_impl_(cond)
#define ASSERT_LOG(cond, ...) assert_log_impl_(cond, __VA_ARGS__)

#define NEW_ARRAY(T, count) ((T *)calloc((count), sizeof(T)))
#define NEW(T) NEW_ARRAY(T, 1)
#define ZERO(t) memset(&(t), 0, sizeof((t)))
#define ZERO_MEMORY(t) ZERO(t)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define OFFSET_OF(T, member) ((size_t)(&((T *)0)->member))
#define SIZE_OF_MEMBER(T, member) (sizeof(((T *)0)->member))

typedef struct Allocator {
  void *(*proc)(void *ptr, iptr size, void *arg);
  void *arg;
} Allocator;

void *realloc_a(void *ptr, iptr size, Allocator a);
void free_a(void *ptr, Allocator a);

typedef struct Temp_Memory_Node {
  struct Temp_Memory_Node *next;
  iptr size;
  iptr id;
  iptr padding_;
} Temp_Memory_Node;

STATIC_ASSERT(sizeof(Temp_Memory_Node) == sizeof(iptr)*4, "sizeof(Temp_Memory_Node) incorrect.")

typedef struct Temp_Memory {
  Temp_Memory_Node *fallback_nodes;
  iptr capacity;
  iptr cursor;
  iptr max_id;
  iptr watermark;
} Temp_Memory;

#define Farray(T, count) struct { \
  T data[count]; \
  union { \
    iptr size; \
    iptr length; \
  }; \
}

#define farr_len(fa) ((fa).size)
#define farr_capacity(fa) (_countof((fa).data))
#define farr_add(fa, elem) ( \
    (fa).size < _countof((fa).data) ? \
      ((fa).data[(fa).size++] = elem,/*comma op*/ &(fa).data[(fa).size-1]) : \
      NULL \
  )

#define farr_back(fa) (arr_assert_((fa).length > 0), (fa).data[(fa).length-1])
#define farr_back_ptr(fa) (arr_assert_((fa).length > 0), &(fa).data[(fa).length-1])

INLINE static bool farr_insert_impl(void *ptr, iptr size, iptr capa, iptr index, iptr elem_size) {
  if (size < capa) {
    if (index < size) {
      uint8_t *copy_from = (uint8_t *)ptr + (index    *elem_size);
      uint8_t *copy_to   = (uint8_t *)ptr + ((index+1)*elem_size);
      memmove(copy_to, copy_from, (size - index)*elem_size);
    }
    return 1;
  }
  return 0;
}

// FIXME: i is evaluated twice. Find a clever way to not have to do that.
#define farr_insert(fa, i, elem) ( \
  farr_insert_impl((fa).data, (fa).size, _countof((fa).data), (i), sizeof((fa).data[0])) ? \
    ((fa).data[(i)] = elem, (fa).size++, 1) : \
    (0) \
)

#define farr_add_new(fa) ( \
    (fa).size < _countof((fa).data) ? \
      (memset(&(fa).data[(fa).size++], 0, sizeof((fa).data[0])),/*comma op*/ &(fa).data[(fa).size-1]) : \
      NULL \
  )
#define farr_clear(fa) do { (fa).size = 0; } while(0)
#define farr_remove_unordered(fa, index) do { \
  iptr FixedArrayIndex__ = index; \
  if (FixedArrayIndex__+1 < (fa).size) \
    (fa).data[FixedArrayIndex__] = (fa).data[(fa).size-1]; \
  (fa).size--; \
} while(0)
#define farr_remove(fa, index) do { \
  iptr FixedArrayIndex__ = index; \
  if (FixedArrayIndex__+1 < (fa).size) \
    memmove(&(fa).data[FixedArrayIndex__], &(fa).data[FixedArrayIndex__+1], ((fa).size - FixedArrayIndex__ -1) * sizeof((fa).data[0])); \
  (fa).size--; \
} while(0)

STATIC_ASSERT(sizeof(Temp_Memory) == sizeof(iptr)*5, "sizeof(Temp_Memory) incorrect.")

Temp_Memory *new_temp_memory_a(iptr capacity, Allocator allocator);
Temp_Memory *new_temp_memory(iptr capacity);
void *tm_realloc_or_free(Temp_Memory *tm, void *ptr, iptr size);
Allocator get_temporary_allocator(Temp_Memory *tm);
void tm_reset(Temp_Memory *tm);

//---------------------- Array ---------------------
typedef struct Array_Header {
  iptr capacity;
  iptr size;
  Allocator allocator;
} Array_Header;


static const Allocator default_a;

#define STATIC_FOREACH(I, ARR) for (int I = 0; I < _countof(ARR); I++)
#define ARR_FOREACH(I, ARR) for (int I = 0; I < arr_len(ARR); I++)
#define FARR_FOREACH(I, ARR) for (int I = 0; I < (ARR).size; I++)

#define arr_assert(cond)             arr_assert_(cond)
#define arr_size_lvalue(a)           (((Array_Header *)(a) - 1)->size)
#define arr_size(a)                  ((a) ? arr_size_lvalue(a) : 0)
#define arr_capacity(a)              ((a) ? ((Array_Header *)(a) - 1)->capacity : 0)
#define arr_len(a)                   arr_size(a)
#define arr_add_a(a, elem, al)       (arr_grow_((void **)&(a), sizeof(a[0]), arr_len(a), al), (a)[arr_len(a)-1] = elem, arr_back_ptr(a))
#define arr_destroy_a(a, al)         do { if (a) arr_free_(a); a = NULL; } while(0)
#define arr_insert_a(a, i, elem, al) do { iptr index__ = i; arr_grow_((void **)&a, sizeof(a[0]), index__, al); a[index__] = elem; } while(0)
#define arr_add_new_a(a, al)         (arr_grow_((void **)&(a), sizeof((a)[0]), arr_size(a), al), memset(&(a)[arr_size_lvalue(a)-1],0,sizeof((a)[0])), arr_back_ptr(a))
#define arr_add(a, elem)             arr_add_a(a, elem, default_a)
#define arr_add_uninitialized(a)     (arr_grow_((void **)&(a), sizeof(a[0]), arr_size(a)), arr_back(a))
#define arr_add_new(a)               arr_add_new_a(a, default_a)
#define arr_insert(a, i, elem)       arr_insert_a(a, i, elem, default_a)
#define arr_remove(a, i)             arr_remove_((void *)(a), sizeof(a[0]), i)
#define arr_destroy(a)               do { if (a) arr_free_(a); a = NULL; } while(0)
#define arr_free(a)                  arr_destroy(a)
#define arr_clear(a)                 do { (void)&a[0];/*Silly check that this is a pointer*/ if (a) arr_size_lvalue(a) = 0; } while(0)
#define arr_sort(a, cmp)             arr_sort_impl_(a, arr_size(a), sizeof(a[0]), (cmp))
#define arr_back_ptr(a)              (arr_assert(arr_len(a)>0), &a[arr_size_lvalue(a)-1])
#define arr_back_val(a)              (arr_assert(arr_len(a)>0),  a[arr_size_lvalue(a)-1])
#define arr_front(a)                 (arr_assert(arr_len(a)>0), &a[0])
#define arr_pop_back_val(a)          (arr_assert(arr_len(a)>0), a[--arr_size_lvalue(a)])
#define arr_pop_back(a)              (arr_assert(arr_len(a)>0), --arr_size_lvalue(a))
#define arr_pop_front(a)             arr_remove(a, 0)
#define arr_add_front(a, elem)       arr_insert(a, 0, elem)
#define arr_remove_unordered(a, i)   arr_remove_unordered_(a, sizeof(a[0]), i)
#define arr_reserve_a(a, size, al)   arr_reserve_((void **)&(a), sizeof((a)[0]), size, al)
#define arr_reserve(a, size)         arr_reserve_a((a), size, default_a)
#define arr_empty(a)                 (a == NULL || arr_size_lvalue(a) == 0)
#define arr_resize(a, new_size)      arr_resize_((void **)&(a), sizeof(a[0]), new_size, default_a)
#define arr_copy(a)                  arr_copy_(a, arr_len(a), sizeof(a[0]))

#define ARR_COPY(dst, src) do { \
  arr_clear(dst); \
  for (int i = 0; i < arr_len(src); i++) { \
    arr_add(dst, (src)[i]); \
  } \
} while(0)

typedef int Array_Cmp(const void *a, const void *b);

#define STR_AND_SIZE(str) (str), str_size(str)

Array_Header arr_debug_get_header(void *arg);
void arr_assert_(int cond);
bool arr_grow_(void **pptr, iptr elem_size, iptr i, Allocator allocator);
void arr_resize_(void **ptr, iptr elem_size, iptr new_size, Allocator allocator);
void arr_free_(void *ptr);
void arr_remove_(void* ptr, iptr elem_size, iptr i);
void arr_sort_impl_(void *ptr, iptr count, iptr elem_size, Array_Cmp *cmp);
void arr_remove_unordered_(void *ptr, iptr elem_size, iptr i);
void arr_reserve_(void **pptr, iptr elem_size, iptr capacity, Allocator allocator);

int  str_empty(const char *str);
iptr str_size(const char *str);
iptr str_len(const char *str);
iptr str_size_part(const char *str, iptr size); // This checks the strings size, but won't check past size
int  str_eq_part(const char *str, iptr asize, const char *b, iptr bsize);
int  str_eq(const char *x, const char *y);
int  str_ends_with(const char *x, const char *sub);
int  str_begins_with(const char *x, const char *prefix);
char *str_concat_a(const char *x, const char *y, Allocator a);
char *str_concat(const char *x, const char *y);
char *str_reserve_a(char *x, iptr size, Allocator a);
bool str_contains(const char *x, const char *sub);

char *str_copy_part(const char *str, iptr size);
char *str_copy_part_a(const char *str, iptr size, Allocator a);
char *str_copy_a(const char *str, Allocator a);
char *str_copy(const char *str);
char *str_reserve(char *a, int size);
char *str_concat(const char *a, const char *b);
void  str_free(char *a);

bool str_split(const char **str, const char *separator, const char **out_token, iptr *out_len);

typedef struct String {
  uint8_t *ptr;
  iptr length;
} String;

String string_from_cstr_a(const char *cstr, Allocator a);

typedef struct String_View {
  const uint8_t *ptr;
  iptr length;
} String_View;

String_View sv_make(const char *ptr, iptr length);
String_View sv_from_cstr(const char *str);
INLINE String_View svcstr(const char *str) { return sv_from_cstr(str); }
String_View sv_trim(String_View s);
String_View sv_pop_front(String_View s, iptr num);
String_View sv_pop_back(String_View s, iptr num);
static char sv_back(String_View s) {
  if (s.length) return s.ptr[s.length-1];
  return '\0';
}
String_View sv_substr(String_View s, iptr begin, iptr length);
iptr sv_find(String_View s, String_View pattern);
bool sv_contains(String_View s, String_View pattern);
bool sv_eq(String_View a, String_View b);
bool sv_starts_with(String_View s, String_View prefix);
bool sv_ends_with(String_View s, String_View suffix);
char *sv_to_cstr(String_View s);
#define sv_format(sv) ((int)(sv).length), ((sv).ptr)

#define Small_String(capacity) struct { \
  iptr length; \
  char data[capacity+1]; \
}
#define sstr_set_cstr(ss, cstr) do { \
  iptr c_str_len = str_len(cstr); \
  ss.length = MIN(_countof(ss.data)-1, c_str_len); \
  memcpy(ss.data, cstr, ss.length); \
  ss.data[ss.length] = 0; \
} while(0)
#define sstr_to_sv(ss) sv_make((ss).data, (ss).length)
#define sstr_to_cstr(ss) ((ss).length ? (ss).data : NULL)

typedef struct String_Builder {
  char *ptr;
  int32_t size, capacity;
  Allocator allocator;
} String_Builder;

void sb_write_property(String_Builder *sb, uint32_t property, const void *ptr, uint32_t size);
void sb_clear(String_Builder *sb);
bool sb_resize(String_Builder *sb, iptr size);
int sb_format(String_Builder *sb, const char *fmt, ...);
int sb_vformat(String_Builder *sb, const char *fmt, va_list args);
int sb_write(String_Builder *sb, const void *ptr, iptr size);
int sb_write_str(String_Builder *sb, const char *str);
int32_t sb_add(String_Builder *sb, char c);
char *sb_detach(String_Builder *sb);
uint32_t sb_write_u32(String_Builder *sb, uint32_t val);
uint32_t sb_write_f32(String_Builder *sb, float val);
void sb_patch_u32(String_Builder *sb, uint32_t off);
void sb_patch_u32_value(String_Builder *sb, uint32_t off, uint32_t value);
uint32_t sb_mark_u32(String_Builder *sb);
void sb_remove_at(String_Builder *sb, iptr index);
bool sb_insert_at(String_Builder *sb, iptr index, const char *buf, iptr buf_length);
void sb_destroy(String_Builder *sb);
static void sb_add_sv(String_Builder *sb, String_View sv) {
  sb_write(sb, sv.ptr, sv.length);
}
static String_View sb_get_sv(String_Builder *sb) {
  return sv_make(sb->ptr, sb->size);
}

typedef struct String_Bank_Id {
  int32_t value;
} String_Bank_Id;
String_Bank_Id sb_bank_vformat(String_Builder *sb, const char *fmt, va_list args);
String_Bank_Id sb_bank_format(String_Builder *sb, const char *fmt, ...);
String_Bank_Id sb_bank_add_string_part(String_Builder *sb, const char *str, int len);
const char *sb_bank_get(String_Builder *sb, String_Bank_Id id);
String_View sb_bank_get_sv(String_Builder *sb, String_Bank_Id id);

INLINE static String sb_detach_to_string(String_Builder *sb) { String ret = { (uint8_t *)sb->ptr, sb->size }; return ret; }

typedef enum Parse_Status {
  PARSE_SUCCESS = 0,
  PARSE_ERROR_RANGE = 1,
  PARSE_ERROR_GENERIC = 2,
} Parse_Status;
Parse_Status str_to_int64(const char *str, int64_t *out_num);
Parse_Status sv_to_int64(String_View view, int64_t *out_num);
Parse_Status sv_eat_int64(String_View view, int64_t *out_num, iptr *out_eat_length);

char *mvprintf(const char *fmt, va_list args);
char *mprintf(const char *fmt, ...);
char *vprintf_a(Allocator a, const char *fmt, va_list args);
char *printf_a(Allocator a, const char *fmt, ...);

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus
template<typename T>
struct Array {
  T *ptr;
  T &operator[](iptr index) { return ptr[index]; }
  const T &operator[](iptr index) const { return ptr[index]; }
  iptr size() const { return arr_len(ptr); }
};
template<typename T>
void array_add(Array<T> *arr, const T &elem, Allocator a={}) { arr_add_a(arr->ptr, elem, a); }
template<typename T>
void array_remove(Array<T> *arr, iptr index) { arr_remove(arr->ptr, index); }
template<typename T>
void array_remove_unordered(Array<T> *arr, iptr index) { arr_remove_unordered(arr->ptr, index); }
template<typename T>
void array_destroy(Array<T> *arr) { arr_free(arr->ptr); }
template<typename T>
void array_clear(Array<T> *arr) { arr_clear(arr->ptr); }
template<typename T>
T *array_add_new(Array<T> *arr, Allocator a={}) { return arr_add_new_a(arr->ptr, a); }
template<typename T>
iptr array_length(Array<T> arr) { return arr_size(arr.ptr); }
#endif

