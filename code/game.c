#include "assets.h"
#include "game.h"
#include "notes.h"
#include <time.h>

static float x = 10;
static float y = 10;

Game the_game;

void draw_ending_fadeout(void) {
  gfx_set_color(0,0,0, ilerp(0.0, 0.2, the_game.misc.ending_fadeout_t));
  gfx_fill_rect(0,0,WIDTH,HEIGHT);

  float alpha = 1;
  alpha *= ilerp(0.1, 0.3, the_game.misc.ending_fadeout_t);
  alpha *= 1.f - ilerp(0.8, 0.9, the_game.misc.ending_fadeout_t);

  gfx_set_color(1,1,1, alpha);
  gfx_draw(assets_get_texture("data/the_end.png"), 0,0);
}

void draw_calendar_and_fadein(float dt) {
  move_to(&the_game.misc.calendar_view_t, 1, dt / 3);
  {
    float calendar_alpha = 1;
    if (the_game.misc.calendar_view_t <= 0.1) {
      calendar_alpha = ilerp(0, 0.1, the_game.misc.calendar_view_t);
    } else if (the_game.misc.calendar_view_t >= 0.9) {
      calendar_alpha = ilerp(1, 0.9, the_game.misc.calendar_view_t);
    }

    gfx_push();
    if (the_game.misc.calendar_view_t < 1) {
      gfx_set_color(0,0,0,1);
      gfx_fill_rect(0,0,WIDTH,HEIGHT);
    }
    gfx_set_color(1,1,1,calendar_alpha);
    char text[32];
    snprintf(text, _countof(text), "Day %d / %d",
        the_game.misc.cur_day + 1, NUM_DAYS);
    gfx_text(text,
        WIDTH/2  - gfx_get_text_width(text)/2,
        HEIGHT/2 - gfx_get_text_height()/2);
    gfx_pop();
  }

  if (the_game.misc.calendar_view_t >= 1) {
    move_to(&the_game.misc.fadein_t, 1, dt / 1);
    float alpha = 1.f - the_game.misc.fadein_t;
    if (alpha > 0) {
      gfx_push();
      gfx_set_color(0,0,0,alpha);
      gfx_fill_rect(0,0,WIDTH,HEIGHT);
      gfx_pop();
    }
  }
}

static SEQUENCE_PROC(sp_ending_1) {
  s_lock(s);
  if (s_inst(s)) {
    Entity *player_en = world_find_first_of_type(the_game.world, ENTITY_TYPE_PLAYER);
    move_to(&player_en->opacity, 0, dt / 0.5);
    if (player_en->opacity <= 0) {
      s_next(s);
      aud_play_oneshot("data/audio/ld59_outro_music.wav", 0.9);
    }
  }
  // TODO: Start ending 1 music here
  s_wait(s, 1, dt);

  if (s_inst(s)) {
    Entity *door_en = world_find_first_of_type(the_game.world, ENTITY_TYPE_BEDROOM_DOOR);
    if (door_en) {
      Entity *en = world_add_entity_at(the_game.world, ENTITY_TYPE_FINAL_FIGURE, door_en->x, door_en->y);
      if (en) {
        sprite_player_play(&en->sprite_player, en->sprite, "cat_idle", dt);
      }
    }
    s_next(s);
  }

  s_wait(s, 1, dt);

  if (s_inst(s)) {
    Entity *en = world_find_first_of_type(the_game.world, ENTITY_TYPE_FINAL_FIGURE);
    if (en) {
      Entity *dish_en = world_find_first_of_type(the_game.world, ENTITY_TYPE_CAT_DISH);
      if (!dish_en) {
        s_next(s);
      } else {
        float target = dish_en->x - 5;

        move_to(&en->x, target, dt * 50.f);
        sprite_player_play_repeat(&en->sprite_player, en->sprite, "cat_walk", dt);

        if (en->x >= target) {
          sprite_player_play(&en->sprite_player, en->sprite, "cat_idle", dt);
          s_next(s);
        }
      }
    } else {
      s_next(s);
    }
  }

  s_wait(s, 2, dt);

  if (s_inst(s)) {
    the_game.misc.used_catfood = false;
    s_next(s);
  }

  s_wait(s, 2, dt);

  if (s_inst(s)) {
    Entity *en = world_find_first_of_type(the_game.world, ENTITY_TYPE_FINAL_FIGURE);
    if (en) {
      sprite_player_play(&en->sprite_player, en->sprite, "transform", dt);

      if (en->sprite_player.t >= 1) {
        s_next(s);
      }
    } else {
      s_next(s);
    }
  }

  s_wait(s, 2, dt);

  if (s_inst(s)) {
    Entity *en = world_find_first_of_type(the_game.world, ENTITY_TYPE_FINAL_FIGURE);
    if (en) {
      en->x += 80.0 * dt;
      sprite_player_play_repeat(&en->sprite_player, en->sprite, "walk", dt);

      move_to(&the_game.misc.ending_fadeout_t, 1, dt / 5.4);
      if (the_game.misc.ending_fadeout_t >= 1) {
        s_next(s);
      }
    } else {
      s_next(s);
    }
  }

  if (s_inst(s)) {
    reset_all();
    return_to_title();
    s_next(s);
  }
}

void goto_ending(void) {
  run_sequence(&sp_ending_1, 0);
}

void finish_day(void) {
  if (the_game.misc.cur_day+1 < NUM_DAYS) {
    arr_clear(the_game.rule_set);
    the_game.misc.day_done = true;
    the_game.misc.cur_day++;
  }
  else {
    goto_ending();
  }
}

static void add_rule(Rule rule) {
  Rule_State rule_state = {0};
  rule_state.rule = rule;
  arr_add(the_game.rule_set, rule_state);
}

void reset_all(void) {
  ZERO(the_game.misc);
}

void reset_day(void) {
  // World state
  the_game.misc.bathroom_light_on = false;
  the_game.misc.has_catfood = false;
  the_game.misc.used_catfood = false;

  // Win condition
  the_game.misc.day_done = false;
  the_game.misc.day_done_fadeout_t = 0;
  the_game.misc.restart = false;
  the_game.misc.calendar_view_t = 0;
  the_game.misc.fadein_t = 0;

  // Tv
  the_game.misc.tv_is_on = true;
  the_game.misc.tv_channel_transition_t = 1;
  the_game.misc.tv_channel = 5;

  // Bathroom
  //the_game.misc.bathroom_blue = (bool)(randomu(&the_game.rng) % 2);
  the_game.misc.seen_bathroom = false;

  // Scares
  the_game.misc.bedroom_door_figure_appear = false;
  the_game.misc.should_turn_on_bathroom_light_after_figure = false;
  the_game.misc.early_figure_shown = false;

  arr_clear(the_game.rule_set);
  add_rule(RULE_FEED_CAT);
  if (the_game.misc.cur_day == DAY_1) {
    the_game.misc.bathroom_blue = false;
  } else if (the_game.misc.cur_day == DAY_2) {
    the_game.misc.bathroom_blue = false;
    add_rule(RULE_BATHROOM_LIGHT_OFF);
    the_game.misc.bathroom_light_on = true;
  } else if (the_game.misc.cur_day == DAY_3) {
    add_rule(RULE_BATHROOM_LIGHT_OFF);
    add_rule(RULE_TV_ON_CHANNEL_9);
    the_game.misc.bathroom_blue = false;
    the_game.misc.bathroom_light_on = true;
  } else if (the_game.misc.cur_day == DAY_4) {
    the_game.misc.bathroom_blue = true;
    add_rule(RULE_BATHROOM_LIGHT_OFF);
    add_rule(RULE_TV_ON_CHANNEL_COND);
    the_game.misc.bathroom_light_on = true;
  } else if (the_game.misc.cur_day == DAY_5) {
    add_rule(RULE_BATHROOM_LIGHT_OFF);
    add_rule(RULE_TV_ON_CHANNEL_COND);
    the_game.misc.bathroom_light_on = true;
    the_game.misc.bathroom_blue = false;
  } else if (the_game.misc.cur_day == DAY_6) {
    add_rule(RULE_BATHROOM_LIGHT_OFF);
    add_rule(RULE_TV_ON_CHANNEL_COND);
    add_rule(RULE_LET_HER_OUT);
    the_game.misc.bathroom_light_on = true;
  } else if (the_game.misc.cur_day == DAY_7) {
    add_rule(RULE_BATHROOM_LIGHT_ON);
    add_rule(RULE_TV_ON_CHANNEL_9);
    add_rule(RULE_LET_HER_OUT);
    add_rule(RULE_LET_HER_OUT);
    add_rule(RULE_LET_HER_OUT);
    add_rule(RULE_LET_HER_OUT);
  }

  the_game.misc.note_garble_level = false;
  the_game.misc.its_me_writing_in_bathroom = false;

  if (the_game.misc.cur_day > DAY_5) {
    the_game.misc.figure_shown = true;
  }
  if (the_game.misc.cur_day > DAY_6) {
    the_game.misc.did_hand_scare = true;
    the_game.misc.its_me_writing_in_bathroom = true;
    the_game.misc.note_garble_level = 1;
  }

  if (the_game.world) {
    world_free(the_game.world);
  }
  the_game.world = new_world(ROOM_LIVINGROOM);
  place_player_at_type(ENTITY_TYPE_FRONT_DOOR);

  s_mgr_clear(&the_game.sequences);
  the_game.sequences.callback = sequence_callback;

  ZERO(the_game.camera);
  the_game.camera.width  = WIDTH;
  the_game.camera.height = HEIGHT;
}

void world_free(World *world) {
  arr_free(world->entities);
  free(world);
}
static void world_draw(World *world) {
  for (int i = 0; i < arr_len(world->entities); i++) {
    Entity *en = &world->entities[i];
    entity_draw(en);
  }
}
static void world_update(World *world, float dt) {
  for (int i = 0; i < arr_len(world->entities); i++) {
    Entity *en = &world->entities[i];
    entity_update(en, dt);
  }
}

SEQUENCE_CALLBACK(sequence_callback) {
  if (s->arg->locked) {
    the_game.lock_level--;
  }
  free(s->arg);
}
void s_lock(Sequence *s) {
  if (s_inst(s)) {
    if (!s->arg->locked) {
      the_game.lock_level++;
    }
    s->arg->locked = true;
    s_next(s);
  }
}
void s_say(Sequence *s, const char *content) {
  if (s_inst(s)) {
    if (s->start) {
      s->arg->dialog_id = ++the_game.misc.dialog_id;
      if (the_game.misc.dialog_content)
        str_free(the_game.misc.dialog_content);
      the_game.misc.dialog_content = str_copy(content);
      the_game.misc.dialog_char_timer = 0;
      the_game.misc.dialog_progress = 0;
    } else {
      if (the_game.misc.dialog_id > s->arg->dialog_id ||
          !the_game.misc.dialog_content)
      {
        s_next(s);
      }
    }
  }
}

void dialog_draw(float dt) {
  if (the_game.misc.dialog_content) {
    int content_length = str_len(the_game.misc.dialog_content);

    bool dismiss_text = false;
    if (the_game.misc.dialog_progress < content_length) {
      gfx_scroll_text(
          the_game.misc.dialog_content,
          content_length,
          0.03, dt,
          &the_game.misc.dialog_progress,
          &the_game.misc.dialog_char_timer);
      if (app_keypress(KEY_X))
        the_game.misc.dialog_progress = content_length;
    } else {
      if (app_keypress(KEY_X)) {
        dismiss_text = true;
      }
    }

    gfx_push();
    Gfx_Text_Ex ex = {0};
    float x = WIDTH/2 - gfx_get_text_width(the_game.misc.dialog_content)/2;
    float y = HEIGHT - gfx_get_text_height()*2;
    ex.bordered = true;
    ex.use_progress = true;
    ex.text_progress = the_game.misc.dialog_progress;
    gfx_text_ex(the_game.misc.dialog_content,
        x, y, ex);
    gfx_pop();

    if (dismiss_text) {
      str_free(the_game.misc.dialog_content);
      the_game.misc.dialog_content = NULL;
    }
  }
}

void run_sequence(Sequence_Proc *proc, Entity_Id entity_id) {
  Sequence_Arg *arg = NEW(Sequence_Arg);
  arg->entity_id = entity_id;
  s_mgr_add(&the_game.sequences, proc, arg);
}

static void setup(void) {
  the_game.canvas = gfx_new_canvas(WIDTH, HEIGHT);
  the_game.closeup_canvas = gfx_new_canvas(WIDTH, HEIGHT);
  the_game.tv_screen_canvas = gfx_new_canvas(TV_SCREEN_WIDTH, TV_SCREEN_HEIGHT);
  setup_rng(&the_game.rng, time(NULL));
  assets_setup();
}

static void load(void) {
  the_game.font = assets_get_ttf_font("data/LanaPixel.ttf", 13);
  gfx_set_font(the_game.font);
}

static void draw_loading_bar(int screen_w, int screen_h) {
  if (!assets_is_done()) {
    float p     = assets_progress();
    float bar_h = 12;
    float pad   = 40;
    float y     = screen_h - bar_h - 20;
    gfx_set_color(0.15f, 0.15f, 0.15f, 1);
    gfx_fill_rect(pad, y, screen_w - pad*2, bar_h);
    gfx_set_color(0.9f, 0.9f, 0.9f, 1);
    gfx_fill_rect(pad, y, (screen_w - pad*2) * p, bar_h);
  } else if (!the_game.loaded) {
    the_game.loaded = true;
    load();
  }
}


World *new_world(Room_Type room_type) {
  World *ret = NEW(World);
  ret->room_type = room_type;

  Entity *en = NULL;
  if (room_type == ROOM_LIVINGROOM) {
    ret->margin_left = 18;
    world_add_entity_at(ret, ENTITY_TYPE_FRONT_DOOR, -54+164, 0);
    world_add_entity_at(ret, ENTITY_TYPE_BACKGROUND, -54, 0);
    world_add_entity_at(ret, ENTITY_TYPE_NOTE, 40, -46);
    world_add_entity_at(ret, ENTITY_TYPE_CAT_DISH, -30, 0);
    world_add_entity_at(ret, ENTITY_TYPE_BEDROOM_DOOR, -170, 0);
    world_add_entity_at(ret, ENTITY_TYPE_TV, -105, -14);
    world_add_entity(ret, ENTITY_TYPE_PLAYER);
  } else if (room_type == ROOM_BEDROOM) {
    world_add_entity(ret, ENTITY_TYPE_BACKGROUND);
    if (!the_game.misc.has_catfood)
      world_add_entity_at(ret, ENTITY_TYPE_CAT_FOOD, 77, -28);
    world_add_entity(ret, ENTITY_TYPE_PLAYER);
    if ((en = world_add_entity_at(ret, ENTITY_TYPE_BEDROOM_DOOR, -80, 0))) {
      en->fg = true;
    }
    world_add_entity_at(ret, ENTITY_TYPE_BATHROOM_DOOR, 128, 0);
  } else if (room_type == ROOM_BATHROOM) {
    world_add_entity(ret, ENTITY_TYPE_BACKGROUND);
    world_add_entity_at(ret, ENTITY_TYPE_MIRROR, 4, -45);
    world_add_entity_at(ret, ENTITY_TYPE_LIGHT_SWITCH, -34, -48);
    world_add_entity(ret, ENTITY_TYPE_PLAYER);
    world_add_entity_at(ret, ENTITY_TYPE_BATHROOM_DOOR, -68, 0);
    ret->margin_right = 42;
  }

  return ret;
}

Entity *world_add_entity_at(World *world, Entity_Type entity_type, float x, float y) {
  Entity *en = world_add_entity(world, entity_type);
  if (en) {
    en->x = x;
    en->y = y;
  }
  return en;
}

Entity *world_add_entity(World *world, Entity_Type entity_type) {
  Entity *en = arr_add_new(world->entities);
  en->type = entity_type;
  en->id = ++the_game.max_entity_id;
  en->world = world;
  entity_setup(en);
  return en;
}

Entity *world_find_first_of_type(World *world, Entity_Type entity_type) {
  for (int i = 0; i < arr_len(world->entities); i++) {
    if (world->entities[i].type == entity_type) {
      return &world->entities[i];
    }
  }
  return NULL;
}

void world_apply_removes(World *world) {
  for (int i = 0; i < arr_len(world->entities); i++) {
    if (world->entities[i].removed) {
      arr_remove(world->entities, i);
      i--;
    }
  }
}

Entity *world_find(World *world, Entity_Id id) {
  if (id == 0)
    return NULL;
  for (int i = 0; i < arr_len(world->entities); i++) {
    if (world->entities[i].id == id) {
      return &world->entities[i];
    }
  }
  return NULL;
}

AUD_FILE_CALLBACK(my_aud_file_callback) {
  if (out_size)
    *out_size = 0;

  int size = 0;
  const char *ptr = assets_get_file(path, &size);
  if (ptr) {
    char *ret = malloc(size+1);
    memcpy(ret, ptr, size);
    ret[size] = 0;
    if (out_size)
      *out_size = size;
    return ret;
  }
  return NULL;
}

static void add_tv_sound(Tv_Content content, const char *audio_path) {
  Audio_Source src = aud_get_source(audio_path);
  aud_set_looped(src, true);
  aud_set_volume(src, 0);
  aud_play(src);
  Tv_Sound sound = {0};
  sound.content = content;
  sound.src = src;
  arr_add(the_game.tv_sounds, sound);
}

void start(void) {
  static Audio_Source src;

  Audio_Config aud_conf = {0};
  aud_conf.file_callback = &my_aud_file_callback;
  aud_setup_p(&aud_conf);

  reset_day(); 

  add_tv_sound(TV_CONTENT_STATIC, "data/audio/ld59_tv_static.wav");
  add_tv_sound(TV_CONTENT_TEST_SCREEN, "data/audio/ld59_tv_test_screen.wav");
  add_tv_sound(TV_CONTENT_WARP, "data/audio/ld59_tv_warped.wav");

  return_to_title();
}

static void update_sound_effects(void) {

  // Bathroom light
  if (!the_game.bathroom_light_src.value) {
    the_game.bathroom_light_src = aud_get_source("data/audio/ld59_fluorescent_light.wav");
    aud_set_looped(the_game.bathroom_light_src, true);
    aud_set_volume(the_game.bathroom_light_src, 0);
    aud_play(the_game.bathroom_light_src);
  }
  if (the_game.world->room_type == ROOM_BATHROOM && the_game.misc.bathroom_light_on) {
    aud_set_pos(the_game.bathroom_light_src, pos_to_aud_space(BATHROOM_LIGHT_X, BATHROOM_LIGHT_Y));
    aud_set_volume(the_game.bathroom_light_src, BATHROOM_LIGHT_VOLUME);
  } else {
    aud_set_volume(the_game.bathroom_light_src, 0);
  }

  // Clock light
  if (!the_game.clock_src.value) {
    the_game.clock_src = aud_get_source("data/audio/ld59_clock.wav");
    aud_set_looped(the_game.clock_src, true);
    aud_set_volume(the_game.clock_src, 0);
    aud_play(the_game.clock_src);
  }
  if (the_game.world->room_type == ROOM_BEDROOM) {
    aud_set_pos(the_game.clock_src, pos_to_aud_space(CLOCK_X, CLOCK_Y));
    aud_set_volume(the_game.clock_src, CLOCK_VOLUME);
  } else {
    aud_set_volume(the_game.clock_src, 0);
  }
}

static void update_camera(float dt) {
  Entity *player_en = world_find_first_of_type(the_game.world, ENTITY_TYPE_PLAYER);
  if (player_en) {
    the_game.camera.x = player_en->x;
    the_game.camera.y = player_en->y - 40;
  }

  Entity *en = world_find_first_of_type(the_game.world, ENTITY_TYPE_BACKGROUND);
  if (en && en->texture) {
    Vec4 box = entity_get_box(en);
    float left  = box.x;
    float right = box.x + box.width;
    float room_width = right - left;

    if (room_width < the_game.camera.width) {
      the_game.camera.x = left + room_width/2;
    } else {
      float margin = 20;
      float min_camera_x = left  + the_game.camera.width/2 - margin;
      float max_camera_x = right - the_game.camera.width/2 + margin;
      if (the_game.camera.x < min_camera_x) {
        the_game.camera.x = min_camera_x;
      }
      if (the_game.camera.x > max_camera_x) {
        the_game.camera.x = max_camera_x;
      }
    }
  }
}

void apply_camera_to_gfx(void) {
  Camera2D *camera = &the_game.camera;
  gfx_translate(
      floor(-(camera->x - camera->width/2)),
      floor(-(camera->y - camera->height/2)));
}

static void draw_debug() {
  if (!the_game.debug)
    return;

  if (can_control() && app_keypress(KEY_I)) {
    finish_day();
  }
  if (can_control() && app_keypress(KEY_U)) {
    goto_ending();
  }

  {
    float screen_x = app_mouse_x() / SCALE;
    float screen_y = app_mouse_y() / SCALE;
    float world_x = screen_x + the_game.camera.x - the_game.camera.width/2;
    float world_y = screen_y + the_game.camera.y - the_game.camera.height/2;
    world_x = floor(world_x); world_y = floor(world_y);
    gfx_push();
    apply_camera_to_gfx();
    gfx_set_color(1,1,1,1);
    gfx_fill_rect(world_x, world_y, 1, 1);
    char buf[128];
    snprintf(buf, _countof(buf), "%d, %d", (int)world_x, (int)world_y);
    Gfx_Text_Ex ex = {0};
    ex.bordered = true;
    gfx_text_ex(buf, world_x, world_y, ex);
    gfx_pop();
  }

  if (the_game.state == GAME_STATE_MAIN &&
      the_game.world)
  {
    World *world = the_game.world;
    gfx_push();
    apply_camera_to_gfx();

    for (int i = 0; i < arr_len(world->entities); i++) {
      Entity *en = &world->entities[i];
      if (!en->can_interact)
        continue;
      Vec4 box = entity_get_box(en);
      gfx_set_color(0.1,1,0.2, 0.3);
      if (the_game.hovered_entity_id == en->id)
        gfx_set_color(1.0,1,0.2, 0.3);
      gfx_fill_rect_p(box.p);
    }

    Entity *player_en = world_find_first_of_type(world, ENTITY_TYPE_PLAYER);
    if (player_en) {
      Vec4 box = entity_player_get_interact_box(player_en);
      gfx_set_color(0.1,0.2,1, 0.3);
      gfx_fill_rect_p(box.p);
    }
    gfx_pop();
  }
}

void draw_debug_overlay(void) {
  if (the_game.debug) {
    gfx_push();
    gfx_set_color(1,1,1,1);
    gfx_text("DEBUG", 0, 0);
    gfx_pop();
  }
}

bool can_control(void) {
  return
    (the_game.state == GAME_STATE_MAIN) &&
    (the_game.misc.calendar_view_t >= 1) &&
    (the_game.misc.fadein_t >= 1) &&
    (!the_game.misc.inspect_tv && the_game.misc.inspect_tv_t <= 0) &&
    (!the_game.misc.day_done && the_game.misc.day_done_fadeout_t <= 0) &&
    (!the_game.misc.requested_room && the_game.misc.room_transition_t <= 0) &&
    !note_screen_open() &&
    the_game.lock_level <= 0;
}

static void update_interaction(void) {
  World *world = the_game.world;
  Entity *player = world_find_first_of_type(world, ENTITY_TYPE_PLAYER);
  if (!player)
    return;

  Entity *hovered_entity = NULL;
  if (can_control()) {
    Vec4 interact_box = entity_player_get_interact_box(player);
    for (int i = 0; i < arr_len(world->entities); i++) {
      Entity *en = &world->entities[i];
      if (en == player) continue;

      if (en->can_interact) {
        Vec4 box = entity_get_box(en);
        if (util_box_overlap(interact_box, box)) {
          hovered_entity = en;
        }
      }
    }
  }

  the_game.hovered_entity_id = 0;
  if (hovered_entity) {
    the_game.hovered_entity_id = hovered_entity->id;

    if (app_keypress(KEY_X)) {
      entity_interact(hovered_entity);
    }
  }
}

static void draw_interaction(void) {
  Entity *hovered_entity = world_find(the_game.world, the_game.hovered_entity_id);
  if (hovered_entity) {
    gfx_push();
    apply_camera_to_gfx();
    Vec4 box = entity_get_box(hovered_entity);
    float x = box.x + box.width/2;
    float y = box.y;
    const char *text = "[x]";
    gfx_set_color(1,1,1,0.5);
    Gfx_Text_Ex ex = {0};
    ex.bordered = true;
    gfx_text_ex(text,
        x - gfx_get_text_width(text)/2,
        y - gfx_get_text_height() + (get_wave()>0.5?-1:0),
        ex);
    gfx_pop();
  }
}

void place_player_at_type(Entity_Type entity_type) {
  Entity *target_en = world_find_first_of_type(
      the_game.world, entity_type);
  if (target_en) {
    Entity *player_en = world_find_first_of_type(
      the_game.world, ENTITY_TYPE_PLAYER);
    if (player_en) {
      player_en->x = target_en->x;
    }
  }
}

void update_room_transition(float dt) {
  if (the_game.misc.requested_room &&
      (!the_game.world || the_game.world->room_type != the_game.misc.requested_room))
  {
    move_to(&the_game.misc.room_transition_t, 1, dt / 0.3);
    if (the_game.misc.room_transition_t >= 1) {
      if (the_game.world)
        world_free(the_game.world);
      the_game.world = new_world(the_game.misc.requested_room);

      the_game.misc.bedroom_door_figure_appear = false;

      place_player_at_type(the_game.misc.requested_door_type);
      apply_requested_player_facing();
      the_game.misc.requested_room = ROOM_NONE;
    }

  } else {
    move_to(&the_game.misc.room_transition_t, 0, dt / 0.3);
  }
}
void draw_room_transition(void) {
  gfx_push();
  gfx_set_color(0,0,0,the_game.misc.room_transition_t);
  gfx_fill_rect(0,0,WIDTH,HEIGHT);
  gfx_pop();
}

void update_day_done(float dt) {
  if (the_game.misc.day_done) {
    move_to(&the_game.misc.day_done_fadeout_t, 1, dt / 1);
    if (the_game.misc.day_done_fadeout_t >= 1)
      the_game.misc.restart = true;
  }
}

void draw_day_done(void) {
  gfx_push();
  gfx_set_color(0,0,0,the_game.misc.day_done_fadeout_t);
  gfx_fill_rect(0,0,WIDTH,HEIGHT);
  gfx_pop();
}

void draw_tv_hand_scare(float dt) {
  if (the_game.misc.show_hand_scare) {
    gfx_push();
    float last_t = the_game.misc.hand_scare_t;
    move_to(&the_game.misc.hand_scare_t, 1, dt / 1);
    if (the_game.misc.hand_scare_t >= 0.1) {
      if (last_t < 0.1)
        aud_play_oneshot("data/audio/ld59_tv_hit.wav", 0.4);
      gfx_draw(assets_get_texture("data/hand_1.png"), 4, 45);
    }
    if (the_game.misc.hand_scare_t >= 0.5) {
      if (last_t < 0.5)
        aud_play_oneshot("data/audio/ld59_tv_hit.wav", 0.4);
      gfx_draw(assets_get_texture("data/hand_2.png"), 79, 5);
    }
    gfx_pop();
  }
}

static float the_tv_seed = 0;
static float the_tv_seed_low = 0;
void draw_tv_screen_static(bool low, float alpha) {
  // Static
  gfx_push();
  gfx_set_color(1,1,1,alpha);
  static Gfx_Shader *shader;
  if (!shader) {
    int size = 0;
    const char *ptr = assets_get_file("data/shaders/static.glsl", &size);
    shader = gfx_new_shader(ptr, size);
  }
  gfx_set_shader(shader);

  float seed = the_tv_seed;
  if (low)
    seed = the_tv_seed_low * 0.1;
  gfx_shader_send("seed", GFX_UNIFORM_FLOAT, &seed, 1);
  gfx_fill_rect(0, 0, TV_SCREEN_WIDTH, TV_SCREEN_HEIGHT);
  gfx_pop();
}

void update_tv_sounds(Tv_Content content) {
  if (the_game.state == GAME_STATE_TITLE) {
    content = TV_CONTENT_STATIC;
  }

  for (int i = 0; i < arr_len(the_game.tv_sounds); i++) {
    if (the_game.tv_sounds[i].content != content) {
      aud_set_volume(the_game.tv_sounds[i].src, 0);
      continue;
    }

    Audio_Source src = the_game.tv_sounds[i].src;
    if (the_game.misc.inspect_tv) {
      aud_set_volume(src, TV_STATIC_VOLUME_CLOSEUP);
      aud_unset_pos(src);
    } else {
      aud_set_volume(src, TV_STATIC_VOLUME);
      if (the_game.world->room_type == ROOM_LIVINGROOM) {
        aud_play(src);
        Entity *tv_en = world_find_first_of_type(the_game.world, ENTITY_TYPE_TV);
        if (tv_en) {
          aud_set_pos(src,
              pos_to_aud_space(tv_en->x, tv_en->y + entity_get_box(tv_en).height/2));
        }
      } else if (the_game.world->room_type == ROOM_BEDROOM) {
        aud_play(src);
        Entity *door_en = world_find_first_of_type(the_game.world, ENTITY_TYPE_BEDROOM_DOOR);
        if (door_en) {
          aud_set_pos(src,
              pos_to_aud_space(door_en->x, door_en->y + entity_get_box(door_en).height/2 + 200));
        }
      } else {
        aud_stop(src);
      }
    }
  }
}

void draw_tv_closeup(float dt) {
  if (the_game.misc.inspect_tv) {
    move_to(&the_game.misc.inspect_tv_t, 1, dt / 0.4);
  } else {
    move_to(&the_game.misc.inspect_tv_t, 0, dt / 0.4);
  }

  bool can_control_tv = true;
  if (the_game.state == GAME_STATE_TITLE) {
    can_control_tv = false;
  }

  if (can_control_tv && app_keypress(KEY_Z) && the_game.misc.inspect_tv) {
    the_game.misc.inspect_tv = false;
    the_game.misc.show_hand_scare = false;
  }

  if (the_game.misc.inspect_tv && the_game.misc.inspect_tv_t > 0.5) {
    if (!the_game.misc.tv_is_on) {
#if 0
      if (app_keypress(KEY_X)) {
        the_game.misc.tv_is_on = true;
        the_game.misc.tv_channel_transition_t = 0;
      }
#endif
    } else {
      /*
      if (app_keypress(KEY_X)) {
        the_game.misc.tv_is_on = false;
      } else
      */

      if (can_control_tv && app_keypress(KEY_UP)) {
        the_game.misc.show_hand_scare = false;
        the_game.misc.tv_channel++;
        if (the_game.misc.tv_channel > 9) {
          the_game.misc.tv_channel = 0;
        }
        the_game.misc.tv_channel_transition_t = 0;
      } if (can_control_tv && app_keypress(KEY_DOWN)) {
        the_game.misc.show_hand_scare = false;
        the_game.misc.tv_channel--;
        if (the_game.misc.tv_channel < 0) {
          the_game.misc.tv_channel = 9;
        }
        the_game.misc.tv_channel_transition_t = 0;
      }
    }
  }

  if (the_game.misc.tv_is_on) {
    move_to(&the_game.misc.tv_channel_transition_t, 1, dt / 0.3);
  }

  bool actually_on = the_game.misc.tv_is_on && 
    the_game.misc.tv_channel_transition_t >= 1;

  float alpha = the_game.misc.inspect_tv_t;
  static float seed_timer = 0;
  move_to(&seed_timer, 1, dt / 0.03);
  if (seed_timer >= 1) {
    seed_timer = 0;
    //int rand_int = randomu(&the_game.rng) % 8388608;
    int rand_int = randomi_range(&the_game.rng, 200, 2000);
    the_tv_seed = rand_int;
    the_tv_seed_low = randomf(&the_game.rng);
  }

  Tv_Content cur_content = TV_CONTENT_NONE;
  {
    gfx_push();
    gfx_set_canvas(the_game.tv_screen_canvas);
    gfx_clear(0,0,0,1);

    if (actually_on) {
      cur_content = TV_CONTENT_STATIC;
      if (is_rule_met(RULE_TV_ON_CHANNEL_9) ||
          is_rule_met(RULE_TV_ON_CHANNEL_COND))
      {
        cur_content = TV_CONTENT_TEST_SCREEN;
      }
      if (the_game.state == GAME_STATE_TITLE) {
        cur_content = TV_CONTENT_TEST_SCREEN;
      }

      if (cur_content == TV_CONTENT_STATIC) {
        draw_tv_screen_static(false, 1);
      } else if (cur_content == TV_CONTENT_WARP) {
        draw_tv_screen_static(true, 1.0);
        draw_tv_screen_static(false, 0.2);
      } else if (cur_content == TV_CONTENT_TEST_SCREEN) {
        gfx_draw(assets_get_texture("data/tv_test_screen.png"), 0, 0);
        draw_tv_screen_static(false, 0.2);
      }

      draw_tv_hand_scare(dt);

      if (the_game.state == GAME_STATE_MAIN) {
        // Number
        gfx_push();
        char channel_text[32];
        snprintf(channel_text, _countof(channel_text), "%d", the_game.misc.tv_channel);
        Gfx_Text_Ex ex = {0};
        ex.bordered = true;
        gfx_set_color_web(0x2bdd2b);
        gfx_text_ex(channel_text, 10, 10, ex);
        gfx_pop();
      }
    } else {
      gfx_push();
      gfx_set_color_web(0x1b1f1b);
      gfx_fill_rect(0, 0, TV_SCREEN_WIDTH, TV_SCREEN_HEIGHT);
      gfx_pop();
    }

    gfx_pop();
  }


  if (alpha) {
    gfx_push();
    gfx_set_canvas(the_game.closeup_canvas);
    gfx_clear(0,0,0,1);
    gfx_set_color(1,1,1,1);
    gfx_draw(gfx_get_canvas_texture(the_game.tv_screen_canvas), TV_SCREEN_X, TV_SCREEN_Y);
    gfx_set_color(1,1,1,1);
    gfx_draw(assets_get_texture("data/tv_closeup.png"), 0, 0);
    gfx_pop();

    gfx_push();
    gfx_set_color(1,1,1, alpha);
    gfx_draw(gfx_get_canvas_texture(the_game.closeup_canvas), 0, 0);
    gfx_pop();
  }

  if (alpha && can_control_tv) {
    gfx_push();
    const char *instructions[] = {
      "<#ffff00>[Z]<> Exit",
      "<#ffff00>[DOWN]<> -channel",
      "<#ffff00>[UP]<> +channel",
    };
    for (int i = 0; i < _countof(instructions); i++) {
      Gfx_Text_Ex ex = {0};
      ex.bordered = true;
      float p = 2;
      float h = gfx_get_text_height();
      gfx_set_color(1,1,1,alpha);
      gfx_text_ex(instructions[i], 0+p, HEIGHT - (p + (h+p) * (i+1)), ex);
    }
    gfx_pop();
  }

  update_tv_sounds(cur_content); 
}

Vec3 pos_to_aud_space_with_z(float x, float y, float z) {
  return vec3(AUDIO_POS_SCALE*x, AUDIO_POS_SCALE*-y, z);
}
Vec3 pos_to_aud_space(float x, float y) {
  return pos_to_aud_space_with_z(x, y, AUDIO_POS_Z);
}

void misc_scares_update(float dt) {
  if (the_game.world->room_type == ROOM_BATHROOM &&
      the_game.misc.bathroom_light_on)
  {
    the_game.misc.seen_bathroom = true;
  }

  move_to(&the_game.misc.bathroom_writing_scare_t, 0, dt/0.2);

  if (the_game.misc.cur_day == DAY_6 &&
      is_rule_met(RULE_TV_ON_CHANNEL_COND) &&
      !the_game.misc.did_hand_scare)
  {
    the_game.misc.did_hand_scare = true;
    the_game.misc.show_hand_scare = true;
  }

  if (the_game.world->room_type == ROOM_BEDROOM &&
      the_game.misc.should_turn_on_bathroom_light_after_figure &&
      !the_game.misc.turned_on_bathroom_light_after_figure)
  {
    Entity *player_en = world_find_first_of_type(the_game.world, ENTITY_TYPE_PLAYER);
    if (player_en->x >= 1) {
      the_game.misc.turned_on_bathroom_light_after_figure = true;
      if (!the_game.misc.bathroom_light_on) {
        the_game.misc.bathroom_light_on = true;
        aud_play_oneshot_positioned("data/audio/ld59_light_switch_1.wav",
            1.0, pos_to_aud_space(player_en->x + 50, 0));
      }
    }
  }
}

static void draw_title_stuff(float dt) {
  move_to(&the_game.misc.title_fadein_t, 1, dt / 1.5);
  if (the_game.misc.title_fadeout) {
    move_to(&the_game.misc.title_fadeout_t, 1, dt / 3.5);
    if (the_game.misc.title_fadeout_t >= 1) {
      reset_all();
      reset_day();
      the_game.state = GAME_STATE_MAIN;
    }
  }

  float title_fade_t = the_game.misc.title_fadein_t * (1.f - the_game.misc.title_fadeout_t);

  gfx_push();
  gfx_set_color(1,1,1,1);
  gfx_draw(assets_get_texture("data/title.png"), 0,0);
  //gfx_text("Housesitting", 10, 10);
  Gfx_Text_Ex ex = {0};
  ex.bordered = true;


#if 1 // Taking cover image
  const char *text = "Press <#bfcc76>[x]<> to start";
  float tx = 125;
  float ty = 143;
  float tw = gfx_get_text_width(text);
  float th = gfx_get_text_height();
  float p = 5;
  gfx_set_color(0,0,0,0.7 * ilerp(0.8, 1, title_fade_t));

  gfx_fill_rect(tx-p, ty-p, tw+p*2, th+p*2);

  gfx_set_color(1,1,1,ilerp(0.8, 1, title_fade_t));
  gfx_text_ex(text, tx, ty, ex);
#endif
  gfx_pop();

  if (the_game.misc.title_fadein_t >= 1 && !the_game.misc.title_fadeout) {
    if (app_keypress(KEY_X)) {
      the_game.misc.title_fadeout = true;
    }
  }

  gfx_push();
  gfx_set_color(0,0,0,1.f - title_fade_t);
  gfx_fill_rect(0,0,WIDTH,HEIGHT);
  gfx_pop();
}

void return_to_title(void) {
  the_game.state = GAME_STATE_TITLE;
  the_game.misc.tv_is_on = true;
  the_game.misc.inspect_tv = true;
  the_game.misc.inspect_tv_t = 1;
}

static void game_title(float dt) {
  draw_tv_closeup(dt);
  draw_title_stuff(dt);
}

static void game_main_state(float dt) {
  if (the_game.misc.restart)
    reset_day(); 
  update_day_done(dt);
  world_apply_removes(the_game.world);
  update_room_transition(dt);

  world_update(the_game.world, dt);
  update_camera(dt);

  update_interaction();
  update_sound_effects();

  misc_rule_updates();
  misc_scares_update(dt);

  gfx_push();
  Camera2D *camera = &the_game.camera;
  apply_camera_to_gfx();
  world_draw(the_game.world);
  gfx_pop();

  draw_debug();

  draw_interaction();
  draw_tv_closeup(dt);
  draw_note_screen(dt);
  draw_room_transition();
  draw_day_done();
  dialog_draw(dt);
  draw_calendar_and_fadein(dt);
}

static void game_loop(float dt) {
  if (the_game.debug && app_keydown(KEY_LSHIFT)) {
    dt *= 5;
  }

  float effective_master_volume = 1;
  if (the_game.state == GAME_STATE_TITLE) {
    effective_master_volume *= the_game.misc.title_fadein_t;
    effective_master_volume *= 1.f - the_game.misc.title_fadeout_t;
  } else {
    effective_master_volume *= the_game.misc.fadein_t;
    effective_master_volume *= 1.f - ilerp(0, 0.7, the_game.misc.ending_fadeout_t);
  }
  aud_set_master_volume(effective_master_volume);

  aud_update_3d(pos_to_aud_space(the_game.camera.x, the_game.camera.y), vec3(0,0,1), dt);
  s_mgr_update(&the_game.sequences, dt);
  if (the_game.state == GAME_STATE_MAIN) {
    game_main_state(dt);
  } else {
    game_title(dt);
  }

  draw_ending_fadeout();
  draw_debug_overlay();
}

static void proc(void) {
  float dt = app_delta();

  gfx_push();
  gfx_set_canvas(the_game.canvas);
  gfx_clear(0,0,0,1);

  draw_loading_bar(WIDTH, HEIGHT);

  if (app_keypress(KEY_P) && MY_DEBUG) {
    the_game.debug = !the_game.debug;
  }

  if (the_game.loaded) { // Assets have loaded
    gfx_set_font(the_game.font);
    if (the_game.started) { // Player pressed 'x' to start
      game_loop(dt);
    }
    else {
      gfx_set_color(1,1,1,1);
      const char *text = "Press [x] to start";
      gfx_text(text,
          WIDTH /2 - gfx_get_text_width(text)/2,
          HEIGHT/2 - gfx_get_text_height()/2);
      if (app_keypress(KEY_X)) {
        the_game.started = true;
        start();
      }
    }
  }
  gfx_pop();

  gfx_clear(0,0,0,1);
  gfx_push();
  gfx_set_color(1,1,1,1);
  gfx_scale(SCALE, SCALE);
  gfx_draw(gfx_get_canvas_texture(the_game.canvas), 0,0);
  gfx_pop();
}

APP_MAIN() {
  App_Config conf = {0};
  conf.width = WIDTH * SCALE;
  conf.height = HEIGHT * SCALE;
  app_setup_p(&conf);
  setup();
  app_run(&proc);
  return 0;
}

