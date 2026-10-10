#pragma once
#include "arena.h"
#include "audioSystem.h"
#include "camera.h"
#include "command.h"
#include "entity.h"
#include "fontLibrary.h"
#include "imgui/imgui_internal.h"
#include "input.h"
#include "leveleditor.h"
#include "levels.h"
#include "mainmenu.h"
#include "spriteLibrary.h"
#include <cstdint>


enum class SCENE_TYPES : uint8_t{
  NONE,
  TITLESCREEN,
  MAINMENU,
  GAME,
  CREDITS,
};

struct Inventory {
  bool has_key = false;
};

struct Gameplay {
  CommandBuffer* commandBuffer;
  LevelData* levels;
  int levelCount;
  int currentLevelIndex;
  float undo_timer;
  const char* current_hint;
  Position* input_buffer;
  bool* input_buffer_on_beat;
  int input_buffer_capacity;
  int input_buffer_write_count;
  int input_buffer_read_count;
  bool initialized;
  int activePlayerIndex;
  Entity** activePlayerBuffer;
  int beat_streak;
  int sustained_streak_turns;
  bool use_evolved_bass;
  int player_step_count;
  unsigned int last_processed_beat_index;
  bool input_queued_since_last_beat;
  bool idle_check_pending;
  bool is_streak_active;
  unsigned int* input_buffer_target_beat;
  int last_credited_beat_index;
  Inventory player_inventory;
};

struct Transition {
  enum States {
    Inactive,
    FadeTo,
    FadeFrom
  };
  States state;
  float fade_time_elapsed;
  float fade_time_duration = 1;
};

struct TitleScreen {
  Sprite* background;
  Sprite* ball_overlay;
  Sprite* ball_sized;
};

struct Credits {
  float scrollspeed; 
};

struct Scenes{
  Gameplay gameplay;
  MainMenu mainMenu;
  TitleScreen titlescreen;
  Credits credts;
};

struct EditorData{
  float* fps_buffer;
  int fps_buffer_count;
  bool edit_level;
  Editor editor;
  int fps_buffer_index;
};


struct GameData {
  SCENE_TYPES scene_current;
  SCENE_TYPES scene_previous;
  Scenes scenes;
  Transition transition;
  EditorData editor_data;
  Input input;
  Sprite* spriteBuffer;
  Tileset* tilesetBuffer;
  AudioSystem audio;
  uint64_t* ticks_total;
  FontAtlas font;
  
  bool running;
  Memory::Arena* arena_main;
  Memory::Arena* arena_levels;
  Memory::Arena* arena_entities;
  Memory::Arena* arena_images;
  Memory::Arena* arena_commands;
  Memory::Arena* arena_input;
  Memory::Arena* arena_scratch;
  Camera camera;
  ImGuiContext* imGui_context;
  const float* dt;  
  float* dt_scaler;
  
 };

inline LevelData* GetCurrentLevel(Gameplay* game){
    return &game->levels[game->currentLevelIndex];
}
