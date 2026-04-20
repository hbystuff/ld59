#pragma once

typedef enum Entity_Type {
  ENTITY_TYPE_NONE,
  ENTITY_TYPE_PLAYER,
  ENTITY_TYPE_BACKGROUND,
  ENTITY_TYPE_NOTE,
  ENTITY_TYPE_CAT_DISH,
  ENTITY_TYPE_BEDROOM_DOOR,
  ENTITY_TYPE_BATHROOM_DOOR,
  ENTITY_TYPE_FRONT_DOOR,
  ENTITY_TYPE_CAT_FOOD,
  ENTITY_TYPE_TV,
  ENTITY_TYPE_LIGHT_SWITCH,
  ENTITY_TYPE_MIRROR,
  ENTITY_TYPE_FINAL_FIGURE,
} Entity_Type;

typedef enum Facing {
  FACING_LEFT,
  FACING_RIGHT,
} Facing;

typedef uint32_t Entity_Id;
typedef struct Entity {
  Entity_Id id;
  Entity_Type type;
  Texture *texture;
  Sprite *sprite;
  float x, y;
  Facing facing;
  Sprite_Player sprite_player;
  struct World *world;
  float opacity;

  bool can_interact : 1;
  bool fg : 1;
  bool removed : 1;
} Entity;

void entity_setup(Entity *en);
void entity_update(Entity *en, float dt);
void entity_draw(Entity *en);
void entity_draw_alpha(Entity *en, float alpha);
Vec4 entity_get_box(Entity *en);
Vec4 entity_player_get_interact_box(Entity *en);
void entity_interact(Entity *en);

