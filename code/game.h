#pragma once

#include "entity.h"
#include "sequence.h"
#include "rules.h"

enum {
  WIDTH = 320,
  HEIGHT = 180,
  SCALE = 3,

  PLAYER_WIDTH = 38,
  PLAYER_HEIGHT = 64,
  PLAYER_INTERACT_BOX_WIDTH = 24,

  TV_SCREEN_X = 82,
  TV_SCREEN_Y = 23,
  TV_SCREEN_WIDTH = 166,
  TV_SCREEN_HEIGHT = 124,

#ifdef MY_RELEASE
  MY_DEBUG = 0,
#else
  MY_DEBUG = 1,
#endif
};

typedef enum Game_State {
  GAME_STATE_TITLE,
  GAME_STATE_MAIN,
} Game_State;

const static float TV_STATIC_VOLUME = 0.4;
const static float TV_STATIC_VOLUME_CLOSEUP = 0.15;
const static float AUDIO_POS_SCALE = 0.07;
const static float AUDIO_POS_Z = 0.1;

const static float CLOCK_VOLUME = 0.2;
const static float CLOCK_X = -0;
const static float CLOCK_Y = -84;

const static float BATHROOM_LIGHT_VOLUME = 0.15;
const static float BATHROOM_LIGHT_X = 5;
const static float BATHROOM_LIGHT_Y = -89;

const static float FOOTSTEP_VOLUME = 0.15;

typedef enum Room_Type {
  ROOM_NONE,
  ROOM_LIVINGROOM,
  ROOM_BEDROOM,
  ROOM_BATHROOM,
} Room_Type;


typedef struct Sequence_Arg {
  Entity_Id entity_id;
  bool locked;
  int dialog_id;
} Sequence_Arg;
SEQUENCE_CALLBACK(sequence_callback);
void s_lock(Sequence *s);
void s_say(Sequence *s, const char *content);
void run_sequence(Sequence_Proc *proc, Entity_Id entity_id);

typedef enum Day {
  DAY_1,
  DAY_2,
  DAY_3,
  DAY_4,
  DAY_5,
  DAY_6,
  DAY_7,

  NUM_DAYS,
} Day;

typedef enum Tv_Content {
  TV_CONTENT_NONE,
  TV_CONTENT_STATIC,
  TV_CONTENT_WARP,
  TV_CONTENT_TEST_SCREEN,
} Tv_Content;

typedef struct Tv_Sound {
  Tv_Content content;
  Audio_Source src;
} Tv_Sound;

typedef struct Camera2D {
  float x, y;
  float width, height;
} Camera2D;

typedef struct World {
  Room_Type room_type;
  Entity *entities;
  float margin_left;
  float margin_right;
} World;

typedef struct Game {
  float width, height;
  float scale;
  Gfx_Canvas *canvas;
  Gfx_Canvas *closeup_canvas;
  Gfx_Canvas *tv_screen_canvas;
  Font *font;

  bool loaded;
  bool started;

  World *world;
  Entity_Id max_entity_id;

  Camera2D camera;
  bool debug;
  Entity_Id hovered_entity_id;

  Sequence_Manager sequences;
  int lock_level;

  Rng rng;
  Rule_State *rule_set;

  Tv_Sound *tv_sounds;

  Audio_Source clock_src;
  Audio_Source bathroom_light_src;

  Game_State state;
  float effective_main_volume;

  struct {
    // Title
    float title_fadein_t;
    float title_fadeout_t;
    bool title_fadeout;

    // Note related data
    bool view_note;
    float view_note_t;
    float note_flip_t;
    bool note_show_back;

    // Catfood
    bool has_catfood;
    bool used_catfood;

    // Dialog
    int dialog_id;
    char *dialog_content;
    float dialog_char_timer;
    int dialog_progress;

    // Room transition
    Room_Type requested_room;
    Entity_Type requested_door_type;
    Facing requested_player_facing;
    float room_transition_t;

    // Win condition
    bool day_done;
    float day_done_fadeout_t;
    bool restart;
    float fadein_t;
    float calendar_view_t;
    Day cur_day;
    float ending_fadeout_t;

    // TV
    bool inspect_tv;
    float inspect_tv_t;
    int tv_channel;
    float tv_channel_transition_t;
    bool tv_is_on;

    // Bathroom
    bool bathroom_light_on;
    float bathroom_writing_scare_t;
    bool bathroom_writing_scare_shown;
    bool bathroom_blue;
    bool bathroom_color_swap_happened;
    bool seen_bathroom;

    // Misc scares
    bool bedroom_door_figure_appear;
    bool turned_on_bathroom_light_after_figure;
    bool should_turn_on_bathroom_light_after_figure;
    bool figure_shown;
    bool early_figure_shown;
    bool did_hand_scare;
    float hand_scare_t;
    bool show_hand_scare;
    bool its_me_writing_in_bathroom;
    bool note_garble_level;
  } misc;
} Game;

void reset_day(void);
void reset_all(void);
bool can_control(void);
void return_to_title(void);
void goto_ending(void);

World *new_world(Room_Type room_type);
Entity *world_add_entity(World *world, Entity_Type entity_type);
Entity *world_add_entity_at(World *world, Entity_Type entity_type, float x, float y);
Entity *world_find_first_of_type(World *world, Entity_Type entity_type);
Entity *world_find(World *world, Entity_Id id);
void world_apply_removes(World *world);

void world_free(World *world);
void apply_camera_to_gfx(void);
void draw_note_screen(float dt);
void update_room_transition(float dt);
void draw_room_transition();
void place_player_at_type(Entity_Type entity_type);


void draw_tv_closeup(float dt);

void apply_requested_player_facing(void);
void request_current_player_facing(void);
void finish_day(void);

Vec3 pos_to_aud_space(float x, float y);
Vec3 pos_to_aud_space_with_z(float x, float y, float z);
//void check_uncomplete_tasks(void);

extern Game the_game;
