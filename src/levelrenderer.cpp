#include "levelRenderer.h"
#include "arena.h"
#include "common.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_render.h"
#include "entity.h"
#include "gameState.h"
#include "levels.h"
#include "rendering.h"
#include "spriteLibrary.h"
#include <algorithm>
#include <cmath>

void RenderLevel(GameData* gameData, SDL_Renderer* renderer) {
  Gameplay* gameplay = &gameData->scenes.gameplay;
  LevelData* level = &gameplay->levels[gameplay->currentLevelIndex];

  Sprite* sprite;
  switch(level->tileset->type){
    case TILESETS::Dungeon:
      sprite = GetSprite(SPRITE_ID::dungeon_tileset, gameData->spriteBuffer);
      break;
    case TILESETS::NONE:
    case TILESETS::COUNT:
      assert(false);
      break;
    }

  constexpr uint16_t TILE_FLOOR_A = 7;
  constexpr uint16_t TILE_FLOOR_B = 8;
  constexpr uint16_t TILE_FLOOR_A_BRIGHT = 9;
  constexpr uint16_t TILE_FLOOR_B_BRIGHT = 10;

  bool checker_phase = (gameplay->player_step_count % 2) == 0;

  for(int x = 0; x < level->w; x++){
    for (int y = 0 ; y < level->h; y++) {
      uint16_t id = GetCellID(level, x, y);

      if(gameplay->is_streak_active && !gameplay->use_evolved_bass){
        if (id == TILE_FLOOR_A) {
          id = TILE_FLOOR_A_BRIGHT;
        }
        else if (id == TILE_FLOOR_B) {
          id = TILE_FLOOR_B_BRIGHT;
        }
      }

      if(gameplay->use_evolved_bass){
        if(id == TILE_FLOOR_A) {
          id = checker_phase ? TILE_FLOOR_A_BRIGHT : TILE_FLOOR_B_BRIGHT;
        }
        else if(id == TILE_FLOOR_B){
          id = checker_phase ? TILE_FLOOR_B_BRIGHT : TILE_FLOOR_A_BRIGHT;
        }
      }

      RenderTile(sprite, id, level, renderer, &gameData->camera, x, y, 1, 1);
    }
  }

  for(int i = 0; i < level->goalCount; i++){
    Goal goal = level->goals[i];
    Sprite* sprite = GetSprite(SPRITE_ID::Goal, gameData->spriteBuffer);
    int frame = (int)(goal.blink_timer / 0.2) % (sprite->sprite_count_x * sprite->sprite_count_y);
    RenderSprite_OnTile({frame, sprite}, level, renderer, &gameData->camera, goal.x, goal.y);
  }  
}

bool IsEntityBelowOtherEntity(Entity* a, Entity* b){
  if (a->y != b->y) {
    return a->y < b->y;
  }
  return a->x < b->x;
}

void RenderChaserTriangle(LevelData* lvl, GameData* data, SDL_Renderer* renderer, float x, float y){
  float world_x = x;
  float world_y = y;
  camera::GridToWorld(&world_x, &world_y, lvl);
  world_x += TILE_SIZE_PX_SCALED / 2.0f - data->camera.camera_x - data->camera.shake_x;
  world_y += TILE_SIZE_PX_SCALED / 2.0f - data->camera.camera_y - data->camera.shake_y;

  float half_width = TILE_SIZE_PX_SCALED * 0.22f;
  float half_height = TILE_SIZE_PX_SCALED * 0.24f;

  SDL_Vertex verts[3] = {
    {{world_x, world_y - half_height}, {230, 70, 220, 255}, {0.0f, 0.0f}},
    {{world_x - half_width, world_y + half_height}, {255, 130, 250, 255}, {0.0f, 0.0f}},
    {{world_x + half_width, world_y + half_height}, {190, 50, 210, 255}, {0.0f, 0.0f}},
  };

  SDL_RenderGeometry(renderer, nullptr, verts, 3, nullptr, 0);
}

void RenderEntities(GameData* data, SDL_Renderer* renderer){
  LevelData* lvl = &data->scenes.gameplay.levels[data->scenes.gameplay.currentLevelIndex];

  Entity** SortedEntities = ALLOC_ARRAY(data->arena_scratch, Entity*, lvl->entityCount);

  for (int i = 0; i < lvl->entityCount; i++) {
    SortedEntities[i] = &lvl->entityBuffer[i];
  }

  std::sort(SortedEntities, SortedEntities + lvl->entityCount, IsEntityBelowOtherEntity);

  Gameplay* gameplay = &data->scenes.gameplay;
  Entity* activeEntity = gameplay->activePlayerBuffer[gameplay->activePlayerIndex];
  
  for (int i = 0; i < lvl->entityCount; i++) {
    Entity* entity = SortedEntities[i];
    if(entity->active == false){
      continue;
    }

    SpriteRenderInfo sprite = GetSprite_FromEntityState(entity, data->spriteBuffer, data->ticks_total);
    if(HasBehaviour(entity, Behaviour::IS_PETRIFIED)){
      sprite = GetSprite(SPRITE_ID::Rock, data->spriteBuffer);
    }

    float x_animated = std::lerp(entity->x_prev, entity->x, entity->progress_01);
    float y_animated = std::lerp(entity->y_prev, entity->y, entity->progress_01);
    float ground_y = y_animated;

    if(entity->action == Actions::BUMPING){
      float bump = std::sin(entity->progress_01 * 3.14159f) * 0.2f;
      x_animated += entity->bump_x_dir * bump;
      y_animated += entity->bump_y_dir * bump;
    }

    if(entity->action == Actions::MOVING && HasBehaviour(entity, Behaviour::JUMPS) && !HasBehaviour(entity, Behaviour::IS_PUSHING)) {
     y_animated -= (data->scenes.gameplay.input_buffer_read_count % 2 == 0 ? 0.5 : 0.3) * sinf(entity->progress_01 * 3.14);
    }

    Sprite* dropshadow = &data->spriteBuffer[(int)SPRITE_ID::Dropshadow];

    RenderSprite_OnTile(dropshadow, lvl, renderer, &data->camera, x_animated, ground_y, 1, 0.4, false);

    if(entity == activeEntity){
      SpriteRenderInfo selection_marker = GetSprite(SPRITE_ID::selection_marker, data->spriteBuffer);  
      RenderSprite_OnTile(selection_marker, lvl, renderer, &data->camera, x_animated, ground_y);
    }

    float entity_scale = 1.0f;
    
    if (HasBehaviour(entity, Behaviour::IS_PLAYER)) {
      entity_scale *= GetBeatPulseScale(&data->audio, 0.15f); 
    }

    RenderSprite_OnTile(sprite, lvl, renderer, &data->camera, x_animated, y_animated, entity_scale, entity_scale, false);
  }        
}
