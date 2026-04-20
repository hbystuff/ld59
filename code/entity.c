#include "entity.h"
#include "game.h"

void entity_setup(Entity *en) {
  en->opacity = 1;
  if (en->type == ENTITY_TYPE_PLAYER) {
    en->texture = assets_get_texture("data/player.png");
    en->sprite  = assets_get_sprite ("data/player.json");
  } else if (en->type == ENTITY_TYPE_BACKGROUND) {
    if (en->world->room_type == ROOM_LIVINGROOM) {
      en->texture = assets_get_texture("data/livingroom_bg.png");
    } else if (en->world->room_type == ROOM_BEDROOM) {
      en->texture = assets_get_texture("data/bedroom_bg.png");
      en->sprite  = assets_get_sprite("data/bedroom_bg.json");
    } else if (en->world->room_type == ROOM_BATHROOM) {
      if (the_game.misc.bathroom_blue) {
        en->texture = assets_get_texture("data/bathroom_bg_blue.png");
        en->sprite  = assets_get_sprite ("data/bathroom_bg_blue.json");
      } else {
        en->texture = assets_get_texture("data/bathroom_bg_pink.png");
        en->sprite  = assets_get_sprite ("data/bathroom_bg_pink.json");
      }
    }
  } else if (en->type == ENTITY_TYPE_CAT_FOOD) {
    en->texture = assets_get_texture("data/cat_food.png");
    en->can_interact = true;
  } else if (en->type == ENTITY_TYPE_NOTE) {
    en->texture = assets_get_texture("data/note.png");
    en->can_interact = true;
  } else if (en->type == ENTITY_TYPE_CAT_DISH) {
    en->texture = assets_get_texture("data/cat_dish.png");
    en->sprite  = assets_get_sprite ("data/cat_dish.json");
    en->can_interact = true;
  } else if (en->type == ENTITY_TYPE_BEDROOM_DOOR) {
    en->texture = assets_get_texture("data/bedroom_door.png");
    en->sprite  = assets_get_sprite("data/bedroom_door.json");
    en->can_interact = true;
  } else if (en->type == ENTITY_TYPE_BATHROOM_DOOR) {
    en->can_interact = true;
  } else if (en->type == ENTITY_TYPE_FRONT_DOOR) {
    en->can_interact = true;
  } else if (en->type == ENTITY_TYPE_TV) {
    en->texture = assets_get_texture("data/tv.png");
    en->sprite  = assets_get_sprite ("data/tv.json");
    en->can_interact = true;
  } else if (en->type == ENTITY_TYPE_LIGHT_SWITCH) {
    en->texture = assets_get_texture("data/light_switch.png");
    en->sprite  = assets_get_sprite ("data/light_switch.json");
    en->can_interact = true;
  } else if (en->type == ENTITY_TYPE_FINAL_FIGURE) {
    en->texture = assets_get_texture("data/final_figure.png");
    en->sprite  = assets_get_sprite ("data/final_figure.json");
  }
}

void play_footstep(void) {
  static int footstep_index = 0;
  const char *paths[] = {
    "data/audio/footstep_0.ogg",
    "data/audio/footstep_1.ogg",
    "data/audio/footstep_2.ogg",
    "data/audio/footstep_3.ogg",
    "data/audio/footstep_4.ogg",
  };
  aud_play_oneshot(paths[footstep_index], FOOTSTEP_VOLUME);
  footstep_index++;
  footstep_index %= _countof(paths);
}

void entity_update(Entity *en, float dt) {
  if (en->type == ENTITY_TYPE_PLAYER) {
    float speed = 85;
    //if (the_game.debug && app_keydown(KEY_LSHIFT)) {
    //  speed = 400;
    //}
    bool walking = false;

    bool left  = app_keydown(KEY_LEFT)  && can_control();
    bool right = app_keydown(KEY_RIGHT) && can_control();
    if (left && !right) {
      en->x -= speed * dt;
      en->facing = FACING_LEFT;
      walking = true;
    } else if (right && !left) {
      en->x += speed * dt;
      en->facing = FACING_RIGHT;
      walking = true;
    }

    Entity *bg_en = world_find_first_of_type(en->world, ENTITY_TYPE_BACKGROUND);
    if (bg_en) {
      Vec4 bg_box = entity_get_box(bg_en);
      float left  = bg_box.x;
      float right = bg_box.x + bg_box.width;
      left  += en->world->margin_left;
      right -= en->world->margin_right;

      float player_min_x = left+PLAYER_WIDTH/2;
      float player_max_x = right-PLAYER_WIDTH/2;
      if (en->x < player_min_x)
        en->x = player_min_x;
      if (en->x > player_max_x)
        en->x = player_max_x;
    }

    if (walking) {
      int last_frame_index = 0;
      if (sprite_player_is_in_anim(&en->sprite_player, en->sprite, "walk")) {
        last_frame_index = sprite_t_to_frame_index(en->sprite, "walk", en->sprite_player.t);
      }
      sprite_player_play_repeat(&en->sprite_player,
          en->sprite, "walk", dt);
      int new_frame_index = sprite_t_to_frame_index(en->sprite, "walk", en->sprite_player.t);
      if (new_frame_index != last_frame_index) {
        if (new_frame_index == 2 || new_frame_index == 6) {
          play_footstep();
        }
      }
    } else {
      sprite_player_play_repeat(&en->sprite_player,
          en->sprite, "idle", dt);
    }
  } else if (en->type == ENTITY_TYPE_CAT_DISH) {
    if (the_game.misc.used_catfood) {
      sprite_player_play_repeat(&en->sprite_player,
          en->sprite, "full", dt);
    } else {
      sprite_player_play_repeat(&en->sprite_player,
          en->sprite, "empty", dt);
    }
  } else if (en->type == ENTITY_TYPE_TV) {
    //sprite_player_play_repeat(&en->sprite_player,
    //    en->sprite, "static", dt);
  } else if (en->type == ENTITY_TYPE_LIGHT_SWITCH) {
    if (en->world->room_type == ROOM_BATHROOM) {
      if (the_game.misc.bathroom_light_on) {
        sprite_player_play_repeat(&en->sprite_player,
            en->sprite, "on", dt);
      } else {
        sprite_player_play_repeat(&en->sprite_player,
            en->sprite, "off", dt);
      }
    }
  } else if (en->type == ENTITY_TYPE_BACKGROUND) {
    if (en->world->room_type == ROOM_BATHROOM) {
      if (the_game.misc.bathroom_light_on) {
        if (the_game.misc.bathroom_writing_scare_t > 0) {
          sprite_player_play_repeat(&en->sprite_player,
              en->sprite, "lit_wrong", dt);
        } else if (the_game.misc.its_me_writing_in_bathroom) {
          sprite_player_play_repeat(&en->sprite_player,
              en->sprite, "lit_wrong3", dt);
        } else if (the_game.misc.figure_shown) {
          sprite_player_play_repeat(&en->sprite_player,
              en->sprite, "lit_wrong2", dt);
        } else {
          sprite_player_play_repeat(&en->sprite_player,
              en->sprite, "lit", dt);
        }
      } else {
        if (the_game.misc.its_me_writing_in_bathroom) {
          sprite_player_play_repeat(&en->sprite_player,
            en->sprite, "dark_wrong3", dt);
        } else if (the_game.misc.figure_shown) {
          sprite_player_play_repeat(&en->sprite_player,
            en->sprite, "dark_wrong2", dt);
        } else {
          sprite_player_play_repeat(&en->sprite_player,
              en->sprite, "dark", dt);
        }
      }
    } else if (en->world->room_type == ROOM_BEDROOM) {
      if (the_game.misc.bathroom_light_on) {
        sprite_player_play_repeat(&en->sprite_player,
          en->sprite, "bathroom_light_on", dt);
      } else {
        sprite_player_play_repeat(&en->sprite_player,
          en->sprite, "dark", dt);
      }
    }
  } else if (en->type == ENTITY_TYPE_BEDROOM_DOOR) {
    if (the_game.misc.early_figure_shown) {
        sprite_player_play(&en->sprite_player, en->sprite, "figure_standing", dt);
    } else if (the_game.misc.bedroom_door_figure_appear) {
      Entity *player_en = world_find_first_of_type(en->world, ENTITY_TYPE_PLAYER);
      if (player_en && player_en->x >= 0) {
        sprite_player_play(&en->sprite_player, en->sprite, "figure_standing", dt);
        the_game.misc.figure_shown = true;
      } else {
        sprite_player_play(&en->sprite_player, en->sprite, "figure_walking", dt);
        the_game.misc.should_turn_on_bathroom_light_after_figure = true;
      }
    } else {
      sprite_player_play(&en->sprite_player, en->sprite, "normal", dt);
    }
  }
}

void entity_draw(Entity *en) {
  entity_draw_alpha(en, 1);
}
void entity_draw_alpha(Entity *en, float alpha) {
  if (en->texture) {
    float q[4] = {
      0.f, 0.f, (float)en->texture->width, (float)en->texture->height
    };

    if (en->sprite) {
      Sprite_Quad sq = sprite_player_get_quad(&en->sprite_player, en->sprite);
      q[0] = sq.q[0];
      q[1] = sq.q[1];
      q[2] = sq.q[2];
      q[3] = sq.q[3];
    }

    gfx_push();
    if (en->fg) {
      alpha *= 0.4;
    }
    gfx_set_color(1,1,1,alpha * en->opacity);
    gfx_translate(floor(en->x), floor(en->y));
    if (en->facing == FACING_RIGHT) {
      gfx_scale(-1, 1);
    }
    gfx_drawq(en->texture, q, -q[2]/2, -q[3]);
    gfx_pop();
  }

  if (en->type == ENTITY_TYPE_TV) {
    Texture *screen_texture = gfx_get_canvas_texture(the_game.tv_screen_canvas);
    float width = 17;
    float height = 14;
    float scale_x = width  / (float)screen_texture->width;
    float scale_y = height / (float)screen_texture->height;
    float x = floor(en->x - en->sprite->width/2 + 9);
    float y = en->y - en->sprite->height  + 8;
    gfx_push();
    gfx_translate(floor(x), floor(y));
    gfx_scale(scale_x, scale_y);
    gfx_draw(screen_texture, 0, 0);
    gfx_pop();
  } else if (en->type == ENTITY_TYPE_MIRROR) {
    Entity *player_en = world_find_first_of_type(en->world, ENTITY_TYPE_PLAYER);
    if (player_en) {
      Vec4 box = entity_get_box(en);
      box.x -= floor(the_game.camera.x - the_game.camera.width/2);
      box.y -= floor(the_game.camera.y - the_game.camera.height/2);

      gfx_push();
      gfx_set_scissor(box.x, box.y, box.width, box.height);
      gfx_translate(-4, -1);
      gfx_set_color(1,1,1,0.5);
      entity_draw_alpha(player_en, 0.5);
      gfx_pop();
    }
  }
}

Vec4 entity_get_box(Entity *en) {
  Vec4 box = {0};

  box.x = en->x;
  box.y = en->y;

  if (en->type == ENTITY_TYPE_MIRROR) {
    box.width  = 46;
    box.height = 34;
  } else if (en->type == ENTITY_TYPE_BATHROOM_DOOR || 
      en->type == ENTITY_TYPE_FRONT_DOOR)
  {
    box.width = 8;
    box.height = PLAYER_HEIGHT;
  } if (en->sprite) {
    box.width  = en->sprite->width;
    box.height = en->sprite->height;
  } else if (en->texture) {
    box.width  = en->texture->width;
    box.height = en->texture->height;
  }
  box.x -= box.width / 2;
  box.y -= box.height;
  return box;
}

Vec4 entity_player_get_interact_box(Entity *en) {
  Vec4 ret = {0};
  if (en->type == ENTITY_TYPE_PLAYER) {
    ret.p[2] = PLAYER_INTERACT_BOX_WIDTH;
    ret.p[3] = PLAYER_HEIGHT;
    float offset = 21;
    if (en->facing == FACING_LEFT) {
      ret.p[0] = en->x - ret.p[2];
    } else {
      ret.p[0] = en->x;
    }
    ret.p[1] = en->y - PLAYER_HEIGHT;
  }
  return ret;
}

void request_current_player_facing(void) {
  Entity *player_en = world_find_first_of_type(the_game.world, ENTITY_TYPE_PLAYER);
  if (player_en) {
    the_game.misc.requested_player_facing = player_en->facing;
  }
}
void apply_requested_player_facing(void) {
  Entity *player_en = world_find_first_of_type(the_game.world, ENTITY_TYPE_PLAYER);
  if (player_en) {
    player_en->facing = the_game.misc.requested_player_facing;
  }
}

SEQUENCE_PROC(sp_use_cat_dish_without_catfood) {
  s_lock(s);
  if (the_game.misc.cur_day <= DAY_1) {
    s_say(s, "A cat bowl...");
    s_say(s, "I need to go find the <#ffaacc>cat food<> first.");
  } else if (the_game.misc.cur_day == DAY_2) {
    s_say(s, "Looks like the cat ate th food!");
    s_say(s, "Let's go find some more <#ffaacc>cat food<>.");
  } else if (the_game.misc.cur_day == DAY_3) {
    s_say(s, "Someone's been hungry.");
    s_say(s, "I wish I get to see the cat though...");
  } else if (the_game.misc.cur_day >= DAY_4) {
    s_say(s, "...");
  } else if (the_game.misc.cur_day >= DAY_5) {
    s_say(s, "......");
  } else if (the_game.misc.cur_day >= DAY_6) {
    s_say(s, ".........");
  } else {
    s_say(s, "............");
  }
}
SEQUENCE_PROC(sp_interact_with_full_cat_dish) {
  s_lock(s);
  if (the_game.misc.cur_day <= DAY_3) {
    s_say(s, "Enjoy your meal, kitty.");
  } else {
    s_say(s, "...");
  }
}
SEQUENCE_PROC(sp_use_cat_food) {
  s_lock(s);
  if (s_inst(s)) {
    the_game.misc.used_catfood = true;
    aud_play_oneshot("data/audio/ld59_interaction.wav", 0.6);
    s_next(s);
  }
  if (the_game.misc.cur_day == DAY_1) {
    s_say(s, "There you go, kitty.");
  } else if (the_game.misc.cur_day == DAY_2) {
    s_say(s, "Hope you're hungry, kitty.");
  } else if (the_game.misc.cur_day == DAY_3) {
    s_say(s, "Here kitty kitty...");
  } else if (the_game.misc.cur_day >= DAY_4) {
    s_say(s, "...");
  } else if (the_game.misc.cur_day >= DAY_5) {
    s_say(s, "......");
  } else if (the_game.misc.cur_day >= DAY_6) {
    s_say(s, ".........");
  } else {
    s_say(s, "............");
  }
}

SEQUENCE_PROC(sp_picked_up_cat_food) {
  s_lock(s);
  s_say(s, "Found the <#ffaacc>cat food<>.");
}

SEQUENCE_PROC(sp_use_front_door_when_not_ready_to_leave) {
  s_lock(s);
  s_say(s, "I should check the note to make sure I did everything.");
}

bool is_rule_meetable(Rule rule) {
  return rule != RULE_LET_HER_OUT;
}

bool are_all_rules_checked_off(void) {
  for (int i = 0; i < arr_len(the_game.rule_set); i++) {
    if (!is_rule_meetable(the_game.rule_set[i].rule))
      continue;
    if (!the_game.rule_set[i].complete) {
      return false;
    }
  }
  return true;
}

void entity_interact(Entity *en) {
  if (en->type == ENTITY_TYPE_NOTE) {
    the_game.misc.view_note = true;
  } else if (en->type == ENTITY_TYPE_CAT_DISH) { 
    if (the_game.misc.used_catfood) {
      run_sequence(&sp_interact_with_full_cat_dish, 0);
    } else {
      if (!the_game.misc.has_catfood) {
        run_sequence(&sp_use_cat_dish_without_catfood, 0);
      } else {
        run_sequence(&sp_use_cat_food, 0);
      }
    }
  } else if (en->type == ENTITY_TYPE_BEDROOM_DOOR) {
    request_current_player_facing();
    the_game.misc.requested_door_type = ENTITY_TYPE_BEDROOM_DOOR;
    if (en->world->room_type == ROOM_LIVINGROOM) {
      the_game.misc.requested_room = ROOM_BEDROOM;
    } else {
      the_game.misc.requested_room = ROOM_LIVINGROOM;
    }
  } else if (en->type == ENTITY_TYPE_CAT_FOOD) {
    en->removed = true;
    the_game.misc.has_catfood = true;
    aud_play_oneshot("data/audio/ld59_interaction.wav", 0.6);
    run_sequence(&sp_picked_up_cat_food, 0);
  } else if (en->type == ENTITY_TYPE_BATHROOM_DOOR) {
    request_current_player_facing();
    the_game.misc.requested_door_type = ENTITY_TYPE_BATHROOM_DOOR;
    if (en->world->room_type == ROOM_BATHROOM) {
      the_game.misc.requested_room = ROOM_BEDROOM;
    } else {
      the_game.misc.requested_room = ROOM_BATHROOM;
    }
  } else if (en->type == ENTITY_TYPE_FRONT_DOOR) {
    if (!are_all_rules_checked_off()) {
      run_sequence(&sp_use_front_door_when_not_ready_to_leave, 0);
    } else {
      if (the_game.misc.cur_day == DAY_4) {
        the_game.misc.early_figure_shown = true;
      }
      finish_day();
    }
  } else if (en->type == ENTITY_TYPE_TV) {
    the_game.misc.inspect_tv = true;
  } else if (en->type == ENTITY_TYPE_LIGHT_SWITCH) {
    the_game.misc.bathroom_light_on = !the_game.misc.bathroom_light_on;
    aud_play_oneshot("data/audio/ld59_light_switch_1.wav", 0.4);
    if (the_game.misc.bathroom_light_on &&
        the_game.misc.cur_day == DAY_3 &&
        !the_game.misc.bathroom_writing_scare_shown)
    {
      the_game.misc.bathroom_writing_scare_shown = true;
      the_game.misc.bathroom_writing_scare_t = 1;
    }
  }
}

