#include "sequence.h"

typedef struct Sequence_Label {
  const char *name;
  int inst_id;
} Sequence_Label;

typedef struct Sequence_Context {
  int inst_id;
  Sequence_Label labels[32];
  int num_labels;
  const char *goto_label;
  bool next : 1;
} Sequence_Context;

Sequence s_make(Sequence_Proc *proc, struct Sequence_Arg *arg) {
  Sequence ret = {0};
  ret.proc = proc;
  ret.arg = arg;
  return ret;
}

void s_next(Sequence *s) {
  s->ctx->next = true;
}

bool s_inst(Sequence *s) {
  bool ret = s->pc == s->ctx->inst_id;
  s->ctx->inst_id++;
  return ret;
}

void s_goto(Sequence *s, const char *label) {
  s->ctx->goto_label = label;
}

void s_label(Sequence *s, const char *name) {
  if (s->ctx->num_labels <= _countof(s->ctx->labels)) {
    Sequence_Label label = {0};
    label.name = name;
    label.inst_id = s->ctx->inst_id;
    s->ctx->labels[s->ctx->num_labels++] = label;
  }
}

void s_update(Sequence *s, float dt) {

  Sequence_Context ctx = {0};
  s->ctx = &ctx;

  if (s->pc == 0) {
    if (!s->is_run) {
      s->is_run = true;
      s->start = true;
    } else {
      s->start = false;
    }
  }

  int last_pc = s->pc;
  s->proc(s, dt);

  int num_inst = ctx.inst_id;

  if (ctx.goto_label) {
    int target_inst_id = -1;
    for (int i = 0; i < ctx.num_labels; i++) {
      if (str_eq(ctx.labels[i].name, ctx.goto_label)) {
        target_inst_id = ctx.labels[i].inst_id;
        break;
      }
    }
    if (target_inst_id < 0) {
      printf("WARNING: Sequence label '%s' does not exist. Terminating sequence.\n", ctx.goto_label);
      s->done = true;
    } else {
      s->pc = target_inst_id;
    }
  } else if (s->pc >= num_inst) {
    s->done = true;
  } else if (ctx.next) {
    s->pc++;
    s->start = true;
  } else {
    s->start = false;
  }
}

void s_wait(Sequence *s, float duration, float dt) {
  if (s_inst(s)) {
    move_to(&s->wait_t, 1, dt / duration);
    if (s->wait_t >= 1) {
      s->wait_t = 0;
      s_next(s);
    }
  }
}

void s_mgr_clear(Sequence_Manager *mgr) {
  for (int i = 0; i < arr_len(mgr->sequences); i++) {
    Sequence *s = &mgr->sequences[i];
    if (mgr->callback)
      mgr->callback(s);
  }
  arr_clear(mgr->sequences);
}

void s_mgr_update(Sequence_Manager *mgr, float dt) {
  for (int i = 0; i < arr_len(mgr->sequences); i++) {
    Sequence *s = &mgr->sequences[i];
    s_update(s, dt);
    if (s->done) {
      if (mgr->callback)
        mgr->callback(s);
      arr_remove_unordered(mgr->sequences, i);
      i--;
    }
  }
}

void s_mgr_add(Sequence_Manager *mgr, Sequence_Proc *proc, struct Sequence_Arg *arg) {
  arr_add(mgr->sequences, s_make(proc, arg));
}
