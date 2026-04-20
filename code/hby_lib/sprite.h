#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  SPRITE_FLAG_NONE    = 0,
  SPRITE_FLAG_REVERSE = 0x1 << 1,
  SPRITE_FLAG_LOOP    = 0x1 << 5,
};

typedef struct Sprite_Anim {
  char *name;
  int from, to;
  float duration;
} Sprite_Anim;

typedef struct Sprite_Frame {
  float duration;
  float quad[4];
  int fx, fy;
} Sprite_Frame;

typedef struct {
  union {
    float q[4];
    float quad[4];
  };
  int fx, fy;
} Sprite_Quad;

typedef struct Sprite {
  int flags;
  int16_t width, height;
  int16_t frame, last_frame;
  bool done;
  float frame_time, rate;
  const Sprite_Anim *cur_anim;
  Sprite_Anim *animations;
  int num_animations, capacity_animations;
  Sprite_Frame *frames;
  int num_frames, capacity_frames;
  Sprite_Anim all_frames;
} Sprite;

typedef struct Sprite_Player {
  const char *anim;
  float t;
} Sprite_Player;

typedef struct {
  float repeat;
  bool should_override_t;
  float override_t;
} Sprite_Player_Play_Ex;
void sprite_player_play_ex(Sprite_Player *sp, Sprite *sprite, const char *anim_name, float dt, Sprite_Player_Play_Ex ex);
void sprite_player_play(Sprite_Player *sp, Sprite *sprite, const char *anim_name, float dt);
void sprite_player_set_t(Sprite_Player *sp, Sprite *sprite, const char *anim_name, float t);
void sprite_player_play_repeat(Sprite_Player *sp, Sprite *sprite, const char *anim_name, float dt);
bool sprite_player_is_in_anim(Sprite_Player *sp, Sprite *sprite, const char *anim_name);
Sprite_Quad sprite_player_get_quad(Sprite_Player *sp, Sprite *sprite);

typedef struct Sprite_State {
  const Sprite_Anim *cur_anim; // TODO: use index. // 8 bytes
  float frame_time, rate; // 8 bytes
  int16_t frame, last_frame; // 4 bytes
  bool done; // 1 byte
             // 3 bytes (padding)
} Sprite_State;

void sprite_add_anim(Sprite *sprite, const char *name, int from, int to);
void sprite_add_frame(Sprite *sprite, float quad[4], float duration, int fx, int fy);
void sprite_get_quad(Sprite *sprite, int frame, float quad[4], int *fx, int *fy);
void sprite_get_quad_t(Sprite *sprite, const char *name, float t, float quad[4], int *fx, int *fy);
void sprite_get_cur_quad(Sprite *sprite, float quad[4], int *fx, int *fy);
Sprite_Quad sprite_get_quad_from_anim(Sprite *sprite, const char *name);
Sprite_Quad sprite_get_quad_from_anim_frame_index(Sprite *sprite, const char *name, int frame_index);
Sprite_Quad sprite_get_quad_from_frame(Sprite *sprite, int frame_index);
Sprite_Quad sprite_get_quad_from_anim_t(Sprite *sprite, const char *name, float t);
Sprite_Frame *sprite_get_cur_frame(Sprite *sprite);
Sprite_Frame *sprite_get_frame(Sprite *sprite, int frame);
Sprite_Anim *sprite_find_anim(Sprite *sprite, const char *name);
void sprite_play(Sprite *sprite, const char *name, float dt, int flags);
void sprite_play_duration(Sprite *sprite, const char *name, float dt, float duration, int flags);
float sprite_get_duration(Sprite *sprite, const char *name);
void sprite_play_rate(Sprite *sprite, const char *name, float dt, float rate, int flags);
int sprite_get_num_frames(Sprite *sprite);
float sprite_get_t(Sprite *sprite);
void sprite_set_t(Sprite *sprite, const char *name, float t);
int sprite_t_to_frame(Sprite *sprite, const char *name, float t);
int sprite_t_to_frame_index(Sprite *sprite, const char *name, float t);
void sprite_mirror(Sprite *sprite, const char *name, Sprite *other);
int sprite_get_rel_frame(Sprite *sprite, const char *name);
int sprite_is_playing(Sprite *sprite, const char *name);
int sprite_get_frame_count(Sprite *sprite, const char *name);
void sprite_destroy(Sprite *sprite);
int sprite_get_first_frame_of_anim(Sprite *sprite, const char *name);
int sprite_t_to_frame_from_range(Sprite *sprite, int from, int to, float t, float *out_frame_time);
void sprite_interpolate_fpos(Sprite *sprite, const char *anim_name, float t, float *fx, float *fy);

typedef struct Sprite_Texture {
  struct Gfx_Texture *texture;
  Sprite *sprite;
} Sprite_Texture;

#ifdef __cplusplus
} // extern "C"
#endif

