
#include "notes.h"
#include "game.h"

bool note_screen_open(void) {
  return the_game.misc.view_note ||
    the_game.misc.view_note_t > 0;
}

void draw_note_screen(float dt) {
  if (the_game.misc.view_note && the_game.misc.view_note_t == 0) {
    the_game.misc.note_show_back = false;
  }

  move_to(&the_game.misc.view_note_t,
      the_game.misc.view_note ? 1 : 0,
      dt / 0.2);

  float flip_speed = 0.5;
  if (the_game.misc.note_show_back) {
    if (the_game.misc.note_flip_t < 0.5) {
      move_to(&the_game.misc.note_flip_t, 0.5, dt / (flip_speed*0.5));
    } else {
      move_to(&the_game.misc.note_flip_t, 1.0, dt / (flip_speed*0.5));
    }
  } else {
    if (the_game.misc.note_flip_t > 0.5) {
      move_to(&the_game.misc.note_flip_t, 0.5, dt / (flip_speed*0.5));
    } else {
      move_to(&the_game.misc.note_flip_t, 0.0, dt / (flip_speed*0.5));
    }
  }

  if (the_game.misc.view_note_t <= 0)
    return;

  float alpha = the_game.misc.view_note_t;
  gfx_push();
  gfx_set_color(0,0,0,alpha);
  gfx_fill_rect(0,0,WIDTH,HEIGHT);
  gfx_set_color(1,1,1,alpha);

  bool any_ticking_happening = false;
  if (the_game.misc.note_flip_t < 0.5) {
    gfx_draw(assets_get_texture("data/note_closeup.png"), 0, 0);

    bool played_sound = false;
    float y = 31;
    float x = 115;
    for (int i = 0; i < arr_len(the_game.rule_set); i++) {
      Rule_State *rule_state = &the_game.rule_set[i];
      Rule rule = rule_state->rule;

      if (is_rule_met(rule_state->rule)) {
        rule_state->complete = true;
        if (rule_state->complete_t == 0) {
          played_sound = true;
          aud_play_oneshot("data/audio/ld59_interaction.wav", 0.4);
        }
        move_to(&rule_state->complete_t, 1, dt / 0.4);
      } else {
        if (rule_state->uncheck) {
          move_to(&rule_state->complete_t, 0, dt / 0.7);
          if (rule_state->complete_t <= 0) {
            rule_state->uncheck = false;
          }
        } else if (rule_state->complete_t >= 1) {
          rule_state->uncheck = true;
        }
      }

      any_ticking_happening |=
        (rule_state->complete_t > 0 && rule_state->complete_t < 1) ||
        rule_state->uncheck;

      Rule_Info rule_info = get_rule_info(rule);
      gfx_set_color(0,0,0,alpha);
      if (rule_state->uncheck) {
        Vec3 black = {0};
        Vec3 red = {1.f, 0.f, 0.f};
        Vec3 c = lerp3(black, red, ease_cos_pulse(rule_state->complete_t, 2));
        gfx_set_color(c.r, c.g, c.b, 1);
      }
      Gfx_Text_Ex ex = {0};
      ex.wrap_width = 132;

      gfx_text_ex(rule_info.text, x, y, ex);

      if (is_rule_meetable(rule)) {
        Texture *checkbox_texture = assets_get_texture("data/check_box.png");
        Sprite *checkbox_sprite   = assets_get_sprite ("data/check_box.json");
        Sprite_Quad q = sprite_get_quad_from_anim_t(checkbox_sprite, "tick", rule_state->complete_t);
        gfx_set_color(1,1,1,1);
        gfx_drawq(checkbox_texture, q.q, x - checkbox_sprite->width - 3, y);
      }

      y += gfx_get_text_height_ex(rule_info.text, ex) + 3;
    }
  } else {
    gfx_draw(assets_get_texture("data/note_closeup_back.png"), 0, 0);
  }

  gfx_pop();

  bool note_can_control =
    !any_ticking_happening &&
      (the_game.misc.note_flip_t == 0 || the_game.misc.note_flip_t == 1);

#if 0 
  // Draw the prompt and handle the flipping
  if (note_can_control) {
    if (!the_game.misc.note_show_back) {
      float x = WIDTH/6*5;
      float y = HEIGHT/2;
      Texture *texture = assets_get_texture("data/arrow_right.png");
      gfx_draw(texture,
          x-texture->width/2 + (get_wave()<0.5?-1:1),
          y-texture->height/2);

      if (app_keypress(KEY_RIGHT)) {
        the_game.misc.note_show_back = true;
      }
    } else {
      float x = WIDTH/6*1;
      float y = HEIGHT/2;
      Texture *texture = assets_get_texture("data/arrow_left.png");
      gfx_draw(texture,
          x-texture->width/2 + (get_wave()<0.5?-1:1),
          y-texture->height/2);
      if (app_keypress(KEY_LEFT)) {
        the_game.misc.note_show_back = false;
      }
    }
  }
#endif

  float flip_fade_alpha = 0;
  if (the_game.misc.note_flip_t <= 0.5) {
    flip_fade_alpha = ilerp(0, 0.5, the_game.misc.note_flip_t);
  } else {
    flip_fade_alpha = ilerp(1.0, 0.5, the_game.misc.note_flip_t);
  }
  if (flip_fade_alpha) {
    gfx_push();
    gfx_set_color(0,0,0,flip_fade_alpha);
    gfx_fill_rect(0,0,WIDTH,HEIGHT);
    gfx_pop();
  }

  if (alpha && note_can_control) {
    gfx_push();
    const char *instructions[] = {
      "<#ffff00>[Z]<> Exit",
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

  if (the_game.misc.view_note) {
    if (app_keypress(KEY_Z) && note_can_control)
    {
      the_game.misc.view_note = false;
    }
  }
}
