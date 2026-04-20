#pragma once

//========================================================
// The caller should define what `struct Sequence_Arg` is.
typedef struct Sequence Sequence;
#define SEQUENCE_PROC(NAME) void NAME(Sequence *s, float dt)
typedef SEQUENCE_PROC(Sequence_Proc);

#define SEQUENCE_CALLBACK(NAME) void NAME(Sequence *s)
typedef SEQUENCE_CALLBACK(Sequence_Callback);

struct Sequence {
  float wait_t;
  int pc;
  Sequence_Proc *proc;
  bool start : 1;
  bool is_run : 1;
  bool done : 1;
  struct Sequence_Arg *arg; // Wasteful... but what are ya gonna do.

  //============================================
  // Hack. Game specific stuff here. I need a way
  // for each sequence to hold game specifc and
  // sequence specific data.
  int dialog_id;
  int dialog_prompt_result;

  //============================================
  // Wasteful per sequence runtime data.
  // But it's fine this is convenient.
  struct Sequence_Context *ctx;
};

typedef struct Sequence_Manager {
  Sequence *sequences;
  Sequence_Callback *callback;
} Sequence_Manager;

Sequence s_make(Sequence_Proc *proc, struct Sequence_Arg *arg);
void s_wait(Sequence *s, float duration, float dt);
bool s_inst(Sequence *s);
void s_goto(Sequence *s, const char *label);
void s_label(Sequence *s, const char *label);

void s_next(Sequence *s);
void s_update(Sequence *s, float dt);

void s_mgr_update(Sequence_Manager *mgr, float dt);
void s_mgr_clear(Sequence_Manager *mgr);
void s_mgr_add(Sequence_Manager *mgr, Sequence_Proc *proc, struct Sequence_Arg *arg);

