#pragma once
#include <cassert>
#include <cstdint>

struct LevelData;
struct CommandBuffer;
struct Gameplay;

enum Behaviour : uint32_t {
  NONE = 0,
  CAN_MOVE = 1 << 0,
  IS_PLAYER = 1 << 1,
  RESPOND_TO_INPUT = 1 << 2,
  IS_PETRIFIED = 1 << 3,
  CAN_ROTATE = 1 << 4,
  UNPUSHABLE = 1 << 5,
  JUMPS = 1 << 6,
  IS_PUSHING = 1 << 7,
  IS_ITEM = 1 << 8,
  IS_LOCKED_DOOR = 1 << 9
};

enum class Actions {
  NONE = 0,
  MOVING = 1,
  ROTATING = 2,
  BUMPING = 3
};

enum class Direction {
  RIGHT,
  LEFT,
  UP,
  DOWN
};

inline Direction DirectionFromXY(int xDir, int yDir){
  assert(xDir * yDir == 0);
  if(xDir ==  1) { return Direction::RIGHT; }
  if(xDir == -1) { return Direction::LEFT;  }
  if(yDir ==  1) { return Direction::UP;    }
  else           { return Direction::DOWN;  }
}

enum class ENTITY_ID : uint8_t {
  PLAYER = 0,
  TRIANGLE = 1,
  ROCK = 2,
  SIREN = 3,
  GOLEM = 4,
  KEY = 5,
  DOOR = 6
};

struct Position{
  int x;
  int y;
};


enum class ITEM_ID : uint8_t {
  NONE,
  KEY
};

struct Entity {
  Actions action;
  ENTITY_ID id;
  bool active;
  ITEM_ID item_id;
  Direction facing_current;
  Direction facing_previous;
  int strength;
  int x;
  int y;
  int x_prev;
  int y_prev;
  int spawn_x;
  int spawn_y;
  int respawn_turns_remaining;
  int bump_x_dir;
  int bump_y_dir;
  float progress_01;
  Behaviour behaviour;
};

Entity* GetActiveEntity(Gameplay* gameplay);
bool IsMoving(Entity* entity);
bool HasBehaviour(Entity* entity, Behaviour flags);
void InitializeBaseBehaviour(Entity* entity);
void SetBehaviour(Entity* entity, Behaviour flags);
void AddBehaviour(Entity* entity, Behaviour flags);
void RemoveBehaviour(Entity* entity, Behaviour flags);
// void PostMove(Entity* entity, LevelData* level, CommandBuffer* commandBuffer);
// void PostRotation(Entity* entity, LevelData* level, CommandBuffer* commandBuffer, Direction from, Direction to);
// void PreRotation(Entity* entity, LevelData* level,CommandBuffer* commandBuffer, Direction from, Direction to);
bool IsActing(Entity* e);
// void SnapEntityToEndOfAction(Entity* entity);
