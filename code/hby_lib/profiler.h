#pragma once

#include <stdbool.h>
#include "basic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Profiler_Frame {
  int root_id;
  int cur_id;
  int prev_id;
  struct Profiler_Entry *list;
  String_Builder string_bank;
} Profiler_Frame;

typedef struct Profiler_Entry {
  int id, name_pos;
  const char *file;
  int line;
  int next_peer_id, first_child_id, parent_id;
  double start, end;
  double total_children_duration;
} Profiler_Entry;

#define PROFILER_NUMBER_FRAMES 128
extern int profiler_frame_index;
extern Profiler_Frame profiler_frames[PROFILER_NUMBER_FRAMES];
extern bool profiler_on;

void profiler_begin_frame(void);
void profiler_begin(const char *name, const char *file, int line);
void profiler_end(void);
double profiler_get_frame_duration(Profiler_Frame *frame);
Profiler_Entry *profiler_get_entry(Profiler_Frame *frame, int id);
const char *profiler_get_entry_name(Profiler_Frame *frame, Profiler_Entry *entry);
void profiler_draw_visualization(void);

#define PROFILER_BEGIN(name) profiler_begin(name, __FILE__, __LINE__)
#define PROFILER_BEGIN_FUNCTION() PROFILER_BEGIN(__func__)
#define PROFILER_END() profiler_end()

#ifdef __cplusplus
} // extern "C"
#endif

#ifdef __cplusplus

struct Profiler_Scope {
  ~Profiler_Scope() {
    profiler_end();
  }
};

#define PROFILER_SCOPE(name) PROFILER_BEGIN(name); Profiler_Scope profiler__scope__ ## __LINE__;
#define PROFILER_SCOPE_FUNCTION() PROFILER_SCOPE(__func__)
#endif
