#include "sprite.h"
#include "basic.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void sprite_add_anim(Sprite *sprite, const char *name, int from, int to) {
  Sprite_Anim anim = {0};
  int i = 0;
  anim.name = str_copy(name);
  anim.from = from;
  anim.to = to;
  for (i = from; i <= to; i++) {
    Sprite_Frame f = sprite->frames[i];
    anim.duration += f.duration;
  }
  if (sprite->num_animations >= sprite->capacity_animations) {
    sprite->capacity_animations = sprite->capacity_animations ? sprite->capacity_animations * 2 : 64;
    sprite->animations = (Sprite_Anim *)malloc(sprite->capacity_animations * sizeof(sprite->animations[0]));
  }
  sprite->animations[sprite->num_animations++] = anim;
}

void sprite_add_frame(Sprite *sprite, float quad[4], float duration, int fx, int fy) {
  Sprite_Frame frame = {0};
  frame.quad[0] = quad[0];
  frame.quad[1] = quad[1];
  frame.quad[2] = quad[2];
  frame.quad[3] = quad[3];
  frame.duration = duration;
  frame.fx = fx;
  frame.fy = fy;
  if (sprite->num_frames >= sprite->capacity_frames) {
    sprite->capacity_frames = sprite->capacity_frames ? sprite->capacity_frames * 2 : 64;
    sprite->frames = (Sprite_Frame *)realloc(sprite->frames, sprite->capacity_frames * sizeof(sprite->frames[0]));
  }
  sprite->frames[sprite->num_frames++] = frame;
  sprite->all_frames.duration += duration;
  if (sprite->num_frames > 0)
    sprite->all_frames.to = sprite->num_frames-1;
}

Sprite_Frame sprite_get_frame_fallback(const Sprite *sprite, int frame) {
  Sprite_Frame ret = {0};
  if (frame >=0 && frame < sprite->num_frames)
    return sprite->frames[frame];
  return ret;
}

void sprite_get_quad(Sprite *sprite, int frame, float quad[4], int *fx, int *fy) {
  Sprite_Frame f = sprite_get_frame_fallback(sprite, frame);
  quad[0] = f.quad[0];
  quad[1] = f.quad[1];
  quad[2] = f.quad[2];
  quad[3] = f.quad[3];
  if (fx)
    *fx = f.fx;
  if (fy)
    *fy = f.fy;
}

void sprite_get_cur_quad(Sprite *sprite, float quad[4], int *fx, int *fy) {
  sprite_get_quad(sprite, sprite->frame, quad, fx, fy);
}

Sprite_Anim *sprite_find_anim(Sprite *sprite, const char *name) {
  int i;
  if (!name)
    return &sprite->all_frames;
  for (i = 0; i < sprite->num_animations; i++)
    if (str_eq(sprite->animations[i].name, name))
      return &sprite->animations[i];
  return NULL;
}

int sprite_t_to_frame_from_range(Sprite *sprite, int from, int to, float t, float *out_frame_time) {
  int ret = 0;
  if (t == 1) {
    ret = to;
    if (out_frame_time) {
      Sprite_Frame frame_info = sprite_get_frame_fallback(sprite, ret);
      *out_frame_time = frame_info.duration;
    }
  }
  else {
    float duration = 0;
    for (int i = from; i <= to; i++) {
      duration += sprite->frames[i].duration;
    }
    float target_time = duration * t;
    float cur_time = 0;
    for (int i = from; i <= to; i++) {
      float last_time = cur_time;
      cur_time += sprite_get_frame_fallback(sprite, i).duration;
      if (cur_time > target_time) {
        if (out_frame_time) {
          *out_frame_time = target_time - last_time;
        }
        return i;
      }
    }
  }
  return ret;
}

INLINE static int sprite_t_to_frame_impl(Sprite *sprite, Sprite_Anim *anim, float t, float *out_frame_time) {
  int ret = 0;
  if (!anim)
    return ret;
  ret = sprite_t_to_frame_from_range(sprite, anim->from, anim->to, t, out_frame_time);
  return ret;
}

void sprite_get_quad_t(Sprite *sprite, const char *name, float t, float quad[4], int *fx, int *fy) {
  Sprite_Anim *anim = sprite_find_anim(sprite, name);
  float frame_time = 0;
  int frame = sprite_t_to_frame_impl(sprite, anim, t, &frame_time);
  sprite_get_quad(sprite, frame, quad, fx, fy);
}

Sprite_Quad sprite_get_quad_from_anim_frame_index(Sprite *sprite, const char *name, int frame_index) {
  Sprite_Quad ret = {0};
  Sprite_Anim *anim = sprite_find_anim(sprite, name);
  if (anim && frame_index >= 0 && frame_index <= (anim->to-anim->from)) {
    int frame = anim->from + frame_index;
    sprite_get_quad(sprite, frame, ret.q, &ret.fx, &ret.fy);
  }
  return ret;
}

Sprite_Quad sprite_get_quad_from_frame(Sprite *sprite, int frame_index) {
  Sprite_Quad ret = {0};
  if (frame_index >= 0 && frame_index < sprite->num_frames) {
    int frame = frame_index;
    sprite_get_quad(sprite, frame, ret.q, &ret.fx, &ret.fy);
  }
  return ret;
}

Sprite_Quad sprite_get_quad_from_anim_t(Sprite *sprite, const char *name, float t) {
  Sprite_Quad ret = {0};
  sprite_get_quad_t(sprite, name, t, ret.q, &ret.fx, &ret.fy);
  return ret;
}

Sprite_Quad sprite_get_quad_from_anim(Sprite *sprite, const char *name) {
  return sprite_get_quad_from_anim_t(sprite, name, 0);
}

int sprite_get_first_frame_of_anim(Sprite *sprite, const char *name) {
  Sprite_Anim *anim = sprite_find_anim(sprite, name);
  if (!anim)
    return 0;
  return anim->from;
}

static void sprite_update_(Sprite *sprite, float dt) {
  Sprite_Frame *f;
  const Sprite_Anim *anim;

  int last_frame = sprite->frame;
  if (!sprite->cur_anim) {
    sprite->last_frame = last_frame;
    return;
  }

  f = &sprite->frames[sprite->frame];
  anim = sprite->cur_anim;

  if (sprite->flags & SPRITE_FLAG_REVERSE) {
    sprite->frame_time -= dt * sprite->rate;

    if (sprite->frame_time <= 0) {
      sprite->frame_time = 0;
      if (sprite->frame <= anim->from) {
        if (sprite->flags & SPRITE_FLAG_LOOP) {
          sprite->frame = anim->to;
          sprite->frame_time = sprite->frames[sprite->frame].duration;
        }
        else {
          sprite->done = true;
        }
      }
      else {
        sprite->frame--;
        sprite->frame_time = sprite->frames[sprite->frame].duration;
      }
    }

  }
  else {
    sprite->frame_time += dt * sprite->rate;

    if (sprite->frame_time >= f->duration) {
      sprite->frame_time = f->duration;
      if (sprite->frame >= anim->to) {
        if (sprite->flags & SPRITE_FLAG_LOOP) {
          sprite->frame = anim->from;
          sprite->frame_time = 0;
        }
        else {
          sprite->done = true;
        }
      }
      else {
        sprite->frame++;
        sprite->frame_time = 0;
      }
    }
  }
  sprite->last_frame = last_frame;
}

static void sprite_play_impl_(Sprite *sprite, const char *name, float dt, bool use_duration, float duration, bool use_rate, float rate, int flags) {
  Sprite_Anim *anim = sprite_find_anim(sprite, name);
  bool set_rate = 0;
  if (!anim)
    return;

  if (use_duration) {
    if (duration == 0) {
      if (flags & SPRITE_FLAG_REVERSE)
        sprite->frame = anim->from;
      else
        sprite->frame = anim->to;
      sprite->done = true;
      return;
    }
    sprite->rate = anim->duration / duration;
    set_rate = true;
  }
  else if (use_rate) {
    sprite->rate = rate;
    set_rate = true;
  }

  if (flags != sprite->flags)
    sprite->flags = flags;

  if (sprite->done || anim != sprite->cur_anim) {
    sprite->cur_anim = anim;
    sprite->flags = flags;
    sprite->done = false;
    if (sprite->flags & SPRITE_FLAG_REVERSE) {
      sprite->frame = anim->to;
      sprite->frame_time = sprite->frames[sprite->frame].duration;
    }
    else {
      sprite->frame = anim->from;
      sprite->frame_time = 0;
    }
    if (!set_rate) {
      sprite->rate = 1;
    }
  }
  else {
    sprite_update_(sprite, dt);
  }
}

void sprite_play_duration(Sprite *sprite, const char *name, float dt, float duration, int flags) {
  sprite_play_impl_(sprite, name, dt, true,duration, false,0, flags);
}

float sprite_get_duration(Sprite *sprite, const char *name) {
  Sprite_Anim *anim = sprite_find_anim(sprite, name);
  if (!anim)
    return 0.1; // In case the calling code might want to divide by this.
  return anim->duration;
}

void sprite_play_rate(Sprite *sprite, const char *name, float dt, float rate, int flags) {
  sprite_play_impl_(sprite, name, dt, false,0, true,rate, flags);
}

void sprite_play(Sprite *sprite, const char *name, float dt, int flags) {
  sprite_play_impl_(sprite, name, dt, false,0,false,0, flags);
}

int sprite_get_num_frames(Sprite *sprite) {
  return sprite->num_frames;
}

float sprite_get_t(Sprite *sprite) {
  float cur_time = 0;
  const Sprite_Anim *anim = sprite->cur_anim;
  int i;
  if (!anim)
    return 0;
  for (i = anim->from; i < sprite->frame; i++) {
    cur_time += sprite->frames[i].duration;
  }
  cur_time += sprite->frame_time;
  return cur_time / anim->duration;
}

int sprite_t_to_frame_index(Sprite *sprite, const char *name, float t) {
  Sprite_Anim *anim = sprite_find_anim(sprite, name);
  if (!anim)
    return 0;
  int frame = sprite_t_to_frame(sprite, name, t);
  return frame - anim->from;
}

int sprite_t_to_frame(Sprite *sprite, const char *name, float t) {
  Sprite_Anim *anim = sprite_find_anim(sprite, name);
  if (!anim)
    return 0;
  return sprite_t_to_frame_impl(sprite, anim, t, NULL);
}

void sprite_set_t(Sprite *sprite, const char *name, float t) {
  Sprite_Anim *anim = sprite_find_anim(sprite, name);
  if (!anim)
    return;

  sprite->cur_anim = anim;
  sprite->frame = sprite_t_to_frame_impl(sprite, anim, t, &sprite->frame_time);
  if (t == 1) {
    Sprite_Frame frame_info;
    sprite->frame = anim->to;
    frame_info = sprite_get_frame_fallback(sprite, sprite->frame);
    sprite->frame_time = frame_info.duration;
  }
  else {
    float target_time = anim->duration * t;
    float cur_time = 0;
    int i;
    for (i = anim->from; i <= anim->to; i++) {
      float last_time = cur_time;
      cur_time += sprite->frames[i].duration;
      if (cur_time >= target_time) {
        sprite->frame = i;
        sprite->frame_time = target_time - last_time;
        break;
      }
    }
  }
}

void sprite_mirror(Sprite *sprite, const char *name, Sprite *other) {
  float t;
  if (!other->cur_anim)
    return;

  t = sprite_get_t(other);
  sprite_set_t(sprite, name, t);
}

int sprite_is_playing(Sprite *sprite, const char *name) {
  return sprite->cur_anim && str_eq(sprite->cur_anim->name, name);
}

int sprite_get_rel_frame(Sprite *sprite, const char *name) {
  if (name) {
    Sprite_Anim *anim = sprite_find_anim(sprite, name);
    if (anim) {
      return sprite->frame - anim->from;
    }
  }
  else if (sprite->cur_anim) {
    return sprite->frame - sprite->cur_anim->from;
  }
  return 0x7fffffff;
}

int sprite_get_frame_count(Sprite *sprite, const char *name) {
  Sprite_Anim *anim = sprite_find_anim(sprite, name);
  if (anim) {
    return (anim->to - anim->from) + 1;
  }
  return 0;
}

void sprite_destroy(Sprite *sprite) {
  int i;
  for (i = 0; i < sprite->num_animations; i++)
    str_free(sprite->animations[i].name);
  free(sprite->frames);
  free(sprite->animations);
  memset(sprite, 0, sizeof(*sprite));
}

bool sprite_player_is_in_anim(Sprite_Player *sp, Sprite *sprite, const char *anim_name) {
  int frame = 0;
  if (sp->anim) {
    frame = sprite_t_to_frame(sprite, sp->anim, sp->t);
  }

  Sprite_Anim *anim = sprite_find_anim(sprite, anim_name);
  if (!anim) {
    return false;
  }

  return frame >= anim->from && frame <= anim->to;
}

Sprite_Quad sprite_player_get_quad(Sprite_Player *sp, Sprite *sprite) {
  return sprite_get_quad_from_anim_t(sprite, sp->anim, sp->t);
}

void sprite_player_play_repeat(Sprite_Player *sp, Sprite *sprite, const char *anim_name, float dt) {
  Sprite_Player_Play_Ex ex = {0};
  ex.repeat = true;
  sprite_player_play_ex(sp, sprite, anim_name, dt, ex);
}
void sprite_player_play(Sprite_Player *sp, Sprite *sprite, const char *anim_name, float dt) {
  Sprite_Player_Play_Ex ex = {0};
  sprite_player_play_ex(sp, sprite, anim_name, dt, ex);
}
void sprite_player_set_t(Sprite_Player *sp, Sprite *sprite, const char *anim_name, float t) {
  Sprite_Player_Play_Ex ex = {0};
  ex.should_override_t = true;
  ex.override_t = t;
  sprite_player_play_ex(sp, sprite, anim_name, 0, ex);
}

void sprite_interpolate_fpos(Sprite *sprite, const char *anim_name, float t, float *fx, float *fy) {
  *fx = 0;
  *fy = 0;

  Sprite_Anim *anim = sprite_find_anim(sprite, anim_name);
  if (!anim)
    return;

  Sprite_Frame *frame0 = &sprite->frames[anim->from];
  Sprite_Frame *frame1 = &sprite->frames[anim->from];
  float t_local = 0;

  if (t >= 1) {
    t_local = 0;
    frame0 = &sprite->frames[anim->to];
    frame1 = &sprite->frames[anim->to];
  }
  else if (t > 0) {
    float duration = 0;
    bool found = false;
    for (int i = anim->from; i+1 <= anim->to; i++) {
      Sprite_Frame *f0 = &sprite->frames[i];
      Sprite_Frame *f1 = &sprite->frames[i+1];
      float t0 = duration / anim->duration;
      float t1 = (duration + f0->duration) / anim->duration;
      if (t >= t0 && t <= t1) {
        t_local = (t - t0) / (t1 - t0);
        found = true;
        frame0 = f0;
        frame1 = f1;
        break;
      }
      duration += f0->duration;
    }

    if (!found) {
      frame0 = &sprite->frames[anim->to];
      frame1 = &sprite->frames[anim->to];
    }
  }

  *fx = t_local * ((float)frame1->fx - (float)frame0->fx) + (float)frame0->fx;
  *fy = t_local * ((float)frame1->fy - (float)frame0->fy) + (float)frame0->fy;

  *fx -= sprite->frames[anim->from].fx;
  *fy -= sprite->frames[anim->from].fy;
}

void sprite_player_play_ex(Sprite_Player *sp, Sprite *sprite, const char *anim_name, float dt, Sprite_Player_Play_Ex ex) {
  Sprite_Anim *anim = sprite_find_anim(sprite, anim_name);
  if (!anim)
    return;

  if (!str_eq(sp->anim, anim_name)) {
    sp->t = 0;
  }
  sp->anim = anim_name;

  sp->t += dt / anim->duration;
  if (ex.should_override_t) {
    sp->t = ex.override_t;
  }
  else if (ex.repeat) {
    while (sp->t >= 1) {
      sp->t -= 1;
    }
  }
  else {
    if (sp->t > 1)
      sp->t = 1;
  }
}

#ifdef __cplusplus
} // extern "C"
#endif

