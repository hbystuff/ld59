#pragma once

typedef enum Rule {
  RULE_FEED_CAT,
  RULE_BATHROOM_LIGHT_OFF,
  RULE_BATHROOM_LIGHT_ON,
  RULE_TV_ON_CHANNEL_9,
  RULE_TV_ON_CHANNEL_COND,
  RULE_LET_HER_OUT,
} Rule;

typedef struct Rule_Info {
  Rule rule;
  const char *text;
} Rule_Info;

typedef struct Rule_State {
  Rule rule;
  bool complete;
  float complete_t;
  bool uncheck;
} Rule_State;

void misc_rule_updates(void);
bool is_rule_met(Rule rule);
bool is_rule_meetable(Rule rule);
Rule_Info get_rule_info(Rule rule);

