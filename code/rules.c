#include "game.h"
#include "rules.h"

Rule_Info get_rule_info(Rule rule) {
  Rule_Info info = {0};
  if (rule == RULE_FEED_CAT) {
    if (the_game.misc.note_garble_level >= 1)
      info.text = "Feed ### ###.";
    else
      info.text = "Feed the cat.";
  } else if (rule == RULE_BATHROOM_LIGHT_OFF) {
    if (the_game.misc.note_garble_level >= 1)
      info.text = "Make #### ### ########'s ##### is <#224d0f>###<>.";
    else
      info.text = "Make sure the bathroom's light is <#224d0f>OFF<>.";
  } else if (rule == RULE_BATHROOM_LIGHT_ON) {
    if (the_game.misc.note_garble_level >= 1)
      info.text = "Make #### ### ########'s ##### is <#7a1818>##<>.";
    else
      info.text = "Make sure the bathroom's light is <#7a1818>ON<>.";
  } else if (rule == RULE_TV_ON_CHANNEL_9) {
    if (the_game.misc.note_garble_level >= 1)
      info.text = "Leave ### TV on <#224d0f>CHANNEL #<>.";
    else
      info.text = "Leave the TV on <#224d0f>CHANNEL 9<>.";
  } else if (rule == RULE_TV_ON_CHANNEL_COND) {
    if (the_game.misc.note_garble_level >= 1)
      info.text = "Leave the TV on <#224d0f>CHANNEL #<> if the ######## is <#0f184d>####<>, otherwise, leave on <#7a1857>CHANNEL #<>";
    else
      info.text = "Leave the TV on <#224d0f>CHANNEL 9<> if the bathroom is <#0f184d>BLUE<>, otherwise, leave on <#7a1857>CHANNEL 1<>";
  } else if (rule == RULE_LET_HER_OUT) {
    info.text = "<#7a1818>DO NOT LET HER OUT<>";
  }

  return info;
}

bool has_rule(Rule rule) {
  for (int i = 0; i < arr_len(the_game.rule_set); i++) {
    if (the_game.rule_set[i].rule == rule) {
      return true;
    }
  }
  return false;
}

bool is_rule_met(Rule rule) {
  if (!has_rule(rule))
    return false;
  if (rule == RULE_FEED_CAT) {
    return the_game.misc.used_catfood;
  } else if (rule == RULE_BATHROOM_LIGHT_OFF) {
    return !the_game.misc.bathroom_light_on;
  } else if (rule == RULE_BATHROOM_LIGHT_ON) {
    return the_game.misc.bathroom_light_on;
  } else if (rule == RULE_TV_ON_CHANNEL_9) {
    return the_game.misc.tv_channel == 9;
  } else if (rule == RULE_TV_ON_CHANNEL_COND) {
    return
      the_game.misc.seen_bathroom &&
      (( the_game.misc.bathroom_blue && the_game.misc.tv_channel == 9) ||
        (!the_game.misc.bathroom_blue && the_game.misc.tv_channel == 1));
  }
  return false;
}

void replace_rule(Rule a, Rule b) {
  for (int i = 0; i < arr_len(the_game.rule_set); i++) {
    if (the_game.rule_set[i].rule == a) {
      the_game.rule_set[i].rule = b;
      the_game.rule_set[i].complete = false;
      the_game.rule_set[i].complete_t = 0;
    }
  }
}

bool rule_is_ticked(Rule rule) {
  for (int i = 0; i < arr_len(the_game.rule_set); i++) {
    if (the_game.rule_set[i].rule == rule) {
      return the_game.rule_set[i].complete &&
            the_game.rule_set[i].complete_t >= 1;
    }
  }
  return false;
}

void misc_rule_updates(void) {
  // Hard coded weird stuff happening.
  // Once the player fulfils the task, swap things out
  if (the_game.misc.cur_day == DAY_3) {
    if (is_rule_met(RULE_BATHROOM_LIGHT_OFF)) {
      replace_rule(RULE_BATHROOM_LIGHT_OFF, RULE_BATHROOM_LIGHT_ON);
    }
  }

  if (the_game.misc.cur_day == DAY_6) {
    if (is_rule_met(RULE_BATHROOM_LIGHT_OFF)) {
      replace_rule(RULE_BATHROOM_LIGHT_OFF, RULE_BATHROOM_LIGHT_ON);
    }

    if (is_rule_met(RULE_BATHROOM_LIGHT_ON)) {
      the_game.misc.its_me_writing_in_bathroom = true;
    }
  }

  if (the_game.misc.cur_day == DAY_5) {
    if (rule_is_ticked(RULE_TV_ON_CHANNEL_COND) &&
        !the_game.misc.bathroom_color_swap_happened)
    {
      the_game.misc.bathroom_color_swap_happened = true;
      the_game.misc.bathroom_blue = !the_game.misc.bathroom_blue;
      the_game.misc.bedroom_door_figure_appear = true;
    }
  }

  // If the player went ahead and undid the tasks,
  // the tasks should be quietly unchecked.
  // NOTE: We only un-complete here. Marking task
  // as complete can only happen when note screen
  // is open.
  for (int i = 0; i < arr_len(the_game.rule_set); i++) {
    Rule_State *rule_state = &the_game.rule_set[i];
    if (!is_rule_met(rule_state->rule)) {
      rule_state->complete = false;
    }
  }
}
