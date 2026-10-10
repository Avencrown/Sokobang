#include "game.h"
#include "SDL3/SDL_blendmode.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_scancode.h"
#include "arena.h"
#include "audioSystem.h"
#include "button.h"
#include "common.h"
#include "dev_gui.h"
#include "command.h"
#include "entity.h"
#include "entityrenderer.h"
#include "fontLibrary.h"
#include "gameState.h"
#include "imgui/imgui.h"
#include "input.h"
#include "leveleditor.h"
#include "levels.h"
#include "mainmenu.h"
#include "rendering.h"
#include "spriteLibrary.h"
#include "levelRenderer.h"
#include "tilesetLibrary.h"
#include <cmath>

constexpr int TOTAL_LEVELS = 5;
constexpr int REQUIRED_BEAT_STREAK = 3;
constexpr int BASSLINE_EVOLVE_TURNS = 9;
constexpr int CHASER_RESPAWN_TURNS = 4;
constexpr float BEAT_TOLERANCE = 0.18f;
constexpr float BUMP_SPEED = 14.0f;

void StartLevel(Gameplay* gameplay, Arena* arena_commands, Arena* arena_entities);

void TriggerBump(Entity* entity, int xDir, int yDir) {
  if(entity == nullptr || !entity->active){
    return;
  }
  entity->x_prev = entity->x;
  entity->y_prev = entity->y;
  entity->bump_x_dir = xDir;
  entity->bump_y_dir = yDir;
  entity->progress_01 = 0.0f;
  entity->action = Actions::BUMPING;
}

void SetMusicFromStreak(GameData* data) {
  Gameplay* gameplay = &data->scenes.gameplay;

  if(gameplay->beat_streak < REQUIRED_BEAT_STREAK) {
    gameplay->is_streak_active = false;
    gameplay->use_evolved_bass = false;
    SetStemMutedState(&data->audio, MUSIC_STEM::BASS, true);
    SetStemMutedState(&data->audio, MUSIC_STEM::BASS2, true);
    SetStemMutedState(&data->audio, MUSIC_STEM::CHORDS, true);
    return;
  }

  gameplay->is_streak_active = true;
  gameplay->use_evolved_bass = gameplay->sustained_streak_turns >= BASSLINE_EVOLVE_TURNS;

  if (gameplay->currentLevelIndex == 0) {
    gameplay->use_evolved_bass = false;
  }

  SetStemMutedState(&data->audio, MUSIC_STEM::BASS, gameplay->use_evolved_bass);
  SetStemMutedState(&data->audio, MUSIC_STEM::BASS2, !gameplay->use_evolved_bass);
  SetStemMutedState(&data->audio, MUSIC_STEM::CHORDS, !gameplay->use_evolved_bass);
}

void ResetStreak(GameData* data, bool shake_camera) {
  Gameplay* gameplay = &data->scenes.gameplay;
  gameplay->beat_streak = 0;
  gameplay->sustained_streak_turns = 0;
  gameplay->is_streak_active = false;
  gameplay->use_evolved_bass = false;

  SetMusicFromStreak(data);

  if(shake_camera) {
    camera::StartShake(&data->camera, 0.18f, 4.0f);
  }
}

bool UndoStepContainsPlayerMove(CommandBuffer* command_buffer){
  if(command_buffer->index == 0){
    return false;
  }

  int cmd_index = command_buffer->index - 1;
  uint32_t timestamp = command_buffer->allCommands[cmd_index].command.timestamp;
  while(cmd_index >= 0 && command_buffer->allCommands[cmd_index].command.timestamp == timestamp){
    AnyCommand cmd = command_buffer->allCommands[cmd_index];
    if(cmd.command.type == CMD_TYPE::MOVE && HasBehaviour(cmd.move.entity, Behaviour::IS_PLAYER)){
      return true;
    }
    cmd_index--;
  }

  return false;
}

void UpdateChasers(GameData* data, LevelData* level) {
  Gameplay* gameplay = &data->scenes.gameplay;
  Entity* player = GetActiveEntity(gameplay);
  if(player == nullptr){
    return;
  }

  for (int i = 0; i < level->entityCount; i++) {
    Entity* enemy = &level->entityBuffer[i];
    if(enemy->id != ENTITY_ID::TRIANGLE){
      continue;
    }

    if(!enemy->active){
      if(enemy->respawn_turns_remaining > 0){
        enemy->respawn_turns_remaining--;
      }
      if(enemy->respawn_turns_remaining <= 0){
        enemy->active = true;
        enemy->x = enemy->spawn_x;
        enemy->y = enemy->spawn_y;
        enemy->x_prev = enemy->x;
        enemy->y_prev = enemy->y;
        enemy->action = Actions::NONE;
      }
      continue;
    }

    if (!data->scenes.gameplay.is_streak_active) {
      continue;
    }

    int dx = player->x - enemy->x;
    int dy = player->y - enemy->y;
    int xDir = 0;
    int yDir = 0;

    if(std::abs(dx) >= std::abs(dy)){
      xDir = dx == 0 ? 0 : (dx > 0 ? 1 : -1);
    }
    if(xDir == 0 && dy != 0){
      yDir = dy > 0 ? 1 : -1;
    }

    if(enemy->x + xDir == player->x && enemy->y + yDir == player->y){
      StartLevel(gameplay, data->arena_commands, data->arena_entities);
      SetMusicFromStreak(data);
      return;
    }

    bool moved = false;
    if(xDir != 0 || yDir != 0){
      moved = TryMove(enemy, level, gameplay->commandBuffer, xDir, yDir, enemy->strength, true, &gameplay->player_inventory, true);
    }

    if(!moved && xDir != 0 && dy != 0){
      xDir = 0;
      yDir = dy > 0 ? 1 : -1;
      TryMove(enemy, level, gameplay->commandBuffer, xDir, yDir, enemy->strength, true, &gameplay->player_inventory, true);
    }

    if(enemy->x == player->x && enemy->y == player->y){
      ResetStreak(data, false);
      enemy->active = false;
      enemy->action = Actions::NONE;
      enemy->respawn_turns_remaining = CHASER_RESPAWN_TURNS;
    }
  }
}

extern "C" {

  void InitializeGame(Gameplay* gameplay, Arena* arena_levels, Tileset* tilesetBuffer) {
    assert(gameplay->initialized == false);
    gameplay->currentLevelIndex = 0;
    CreateLevel(arena_levels, &gameplay->levels[0], &tilesetBuffer[(int)TILESETS::Dungeon], "assets/levels/Level_01.tmj");
    CreateLevel(arena_levels, &gameplay->levels[1], &tilesetBuffer[(int)TILESETS::Dungeon], "assets/levels/Level_02.tmj");
    CreateLevel(arena_levels, &gameplay->levels[2], &tilesetBuffer[(int)TILESETS::Dungeon], "assets/levels/Level_03.tmj");
    CreateLevel(arena_levels, &gameplay->levels[3], &tilesetBuffer[(int)TILESETS::Dungeon], "assets/levels/Level_04.tmj");
    CreateLevel(arena_levels, &gameplay->levels[4], &tilesetBuffer[(int)TILESETS::Dungeon], "assets/levels/Level_05.tmj");
    gameplay->initialized = true;
  }

  void Initialize(GameData* data, SDL_Window* window, SDL_Renderer* renderer){
    *data->ticks_total = 0;
    // DEV::Initialize(window, renderer);
    InitializeAudioSystem(&data->audio, data->arena_main);
    AssetManagement::LoadAllSFX(&data->audio);
    AssetManagement::LoadAllSprites(data->spriteBuffer, renderer);
    data->imGui_context = ImGui::GetCurrentContext();
    AssetManagement::LoadFont(renderer, "assets/fonts/ByteBounce.ttf", &data->font, 48);
    AssetManagement::LoadAllTilesets(data->tilesetBuffer, data->arena_images);

    SDL_Texture* blackfade = GetSprite(SPRITE_ID::black_1x1, data->spriteBuffer)->texture;
    SDL_SetTextureBlendMode(blackfade, SDL_BLENDMODE_BLEND);
    InitializeGame(&data->scenes.gameplay, data->arena_levels, data->tilesetBuffer);
    InitializeMenu(&data->scenes.mainMenu, data->spriteBuffer, &data->font, data->arena_main);
    // PlaySong(SONG_ID::THEME);

    LoadMusicStems(&data->audio, 
                "assets/audio/music/version4/drums.wav", 
                "assets/audio/music/version4/bass_first.wav", 
                "assets/audio/music/version4/bass_second.wav", 
                "assets/audio/music/version4/chords.wav");
    StartAllStemsPaused(&data->audio, 1.0f);

    SetStemMutedState(&data->audio, MUSIC_STEM::DRUMS, false);
    SetStemMutedState(&data->audio, MUSIC_STEM::BASS, true);
    SetStemMutedState(&data->audio, MUSIC_STEM::BASS2, true);
    SetStemMutedState(&data->audio, MUSIC_STEM::CHORDS, true);

    InitializeBeatClock(&data->audio, 108.0f);
    ChangeScene(data, SCENE_TYPES::MAINMENU);
  }

  void StartLevel(Gameplay* gameplay, Arena* arena_commands, Arena* arena_entities){
    ResetCommandBuffer(gameplay->commandBuffer);
    Reset(arena_commands);
    CreateEntities(&gameplay->levels[gameplay->currentLevelIndex], arena_entities);
    
    switch (gameplay->currentLevelIndex) {
      case 0:
        gameplay->current_hint = "Use the arrow keys to move to the beat. Get a streak of 3 to be able to push rocks.\nPress Z to undo your last move, and R to restart the level.";
        break;
      case 1:
        gameplay->current_hint = "Keep the streak going for longer and you'll be able to push 2 rocks!";
        break;
      case 2:
        gameplay->current_hint = "Only a key can open a locked door. Grab it and open the door!";
        break;
      case 3:
        gameplay->current_hint = "The triangle chases you if you have a streak, and it can push rocks too, but only one.\nIf it catches you when it's active, the level will reset.";
        break;
      case 4:
        gameplay->current_hint = "Press X to switch which ball you control, or G to control them all at once.";
        break;
      default:
        gameplay->current_hint = "";
        break;
    }
    
    gameplay->activePlayerIndex = 0;
    gameplay->beat_streak = 0;
    gameplay->sustained_streak_turns = 0;
    gameplay->player_step_count = 0;
    gameplay->last_processed_beat_index = 0;
    gameplay->input_queued_since_last_beat = false;
    gameplay->idle_check_pending = false;
    gameplay->is_streak_active = false;
    gameplay->use_evolved_bass = false;
    gameplay->last_credited_beat_index = -1;
    gameplay->player_inventory = {};
  }
    
  void ChangeScene(GameData* data, SCENE_TYPES new_scene) {
    assert(new_scene != data->scene_current);
    
    data->scene_previous = data->scene_current;
    data->scene_current = new_scene;
    data->transition.state = data->scene_previous == SCENE_TYPES::NONE ? Transition::FadeFrom : Transition::FadeTo;
    data->transition.fade_time_elapsed = 0;
      
    switch (data->scene_current) {
      case SCENE_TYPES::TITLESCREEN:
        data->transition.fade_time_duration = 1;
        break;          
      case SCENE_TYPES::MAINMENU:
        
        break;
      case SCENE_TYPES::GAME: {
        data->transition.fade_time_duration = 0.5f;
        Gameplay* gameplay = &data->scenes.gameplay;
        gameplay->currentLevelIndex = 0;
        assert(gameplay->initialized);
        SetStemMutedState(&data->audio, MUSIC_STEM::DRUMS, false);
        StartLevel(gameplay, data->arena_commands, data->arena_entities);
        SetMusicFromStreak(data);
        break;
      }
      case SCENE_TYPES::CREDITS:
       break;
      case SCENE_TYPES::NONE:
        assert(false);
        break;
      }
   }
   
   bool HandleEvents(GameData *data, SDL_Event event) {
    // DEV::ProcessEvents(&event);
    
    if (event.type == SDL_EVENT_QUIT) {
        return false;
    }

    if (event.type != SDL_EVENT_KEY_DOWN) {
        return true;
    }
    
    if (event.key.key == SDLK_ESCAPE) {
        return false;
    }

     return true;
  }

  void UpdateTitlescreen(TitleScreen* titlescreen, const float dt){
  }

  void UpdateGame(GameData* data, const float dt) {
    Gameplay* gameplay = &data->scenes.gameplay;
    Input* input = &data->input;
    Arena* arena_scratch = data->arena_scratch;
    Arena* arena_commands = data->arena_commands;
    Arena* arena_entities = data->arena_entities;
  
    if(KeyPressed(input, SDL_SCANCODE_R)){ // restart level on `R`
      StartLevel(gameplay, arena_commands, arena_entities);
      SetMusicFromStreak(data);
      return;
    }

    if(KeyPressed(input, SDL_SCANCODE_1)){
      gameplay->currentLevelIndex = 0;
      StartLevel(gameplay, arena_commands, arena_entities);
      SetMusicFromStreak(data);
      return;
    }

    if(KeyPressed(input, SDL_SCANCODE_2)){
      gameplay->currentLevelIndex = 1;
      StartLevel(gameplay, arena_commands, arena_entities);
      SetMusicFromStreak(data);
      return;
    }

    if(KeyPressed(input, SDL_SCANCODE_3)){
      gameplay->currentLevelIndex = 2;
      StartLevel(gameplay, arena_commands, arena_entities);
      SetMusicFromStreak(data);
      return;
    }

    if(KeyPressed(input, SDL_SCANCODE_4)){
      gameplay->currentLevelIndex = 3;
      StartLevel(gameplay, arena_commands, arena_entities);
      SetMusicFromStreak(data);
      return;
    }

    if(KeyPressed(input, SDL_SCANCODE_5)){
      gameplay->currentLevelIndex = 4;
      StartLevel(gameplay, arena_commands, arena_entities);
      SetMusicFromStreak(data);
      return;
    }

    float undo_speed_up = std::lerp(1.0, 0.15, (gameplay->commandBuffer->head - gameplay->commandBuffer->index) * (1.0/30.0));
    if(undo_speed_up < 0.15){
      undo_speed_up = 0.15;
    }

    if(KeyPressed(input, SDL_SCANCODE_Z)) { // || KeyHeld_ForTime(input, SDL_SCANCODE_Z, UNDO_REPEAT_TIME * undo_speed_up)
      ResetKeyHeldTime(input, SDL_SCANCODE_Z);
      
      if (gameplay->is_streak_active){
        ResetStreak(data, true);
      }
      
      // if(KeyHeld(input, SDL_SCANCODE_LSHIFT)){
      //   Redo(gameplay->commandBuffer, GetCurrentLevel(gameplay));
      // }
      // else{
        bool undoing_player_step = UndoStepContainsPlayerMove(gameplay->commandBuffer);
        Undo(gameplay->commandBuffer, GetCurrentLevel(gameplay));
      // }

      gameplay->input_buffer_write_count = gameplay->input_buffer_read_count;
      gameplay->last_credited_beat_index = -1;
      if(undoing_player_step && gameplay->player_step_count > 0){
        gameplay->player_step_count--;
      }
    }
   
    auto queue_input = [&](SDL_Scancode scancode, int xDir, int yDir) {
      ResetKeyHeldTime(input, scancode);
      int idx = gameplay->input_buffer_write_count % gameplay->input_buffer_capacity;
      gameplay->input_buffer[idx] = {xDir, yDir};
      gameplay->input_buffer_on_beat[idx] = IsInputOnBeat(&data->audio, BEAT_TOLERANCE);

      unsigned int target_beat = data->audio.beat_clock.beat_index;
      if (data->audio.beat_clock.beat_progress_01 > 0.5f) {
        target_beat += 1;
      }
      gameplay->input_buffer_target_beat[idx] = target_beat;

      gameplay->input_buffer_write_count++;
      gameplay->input_queued_since_last_beat = true;
    };

    if(KeyPressed(input,SDL_SCANCODE_RIGHT)) {
      queue_input(SDL_SCANCODE_RIGHT, 1, 0);
    }
    else if(KeyPressed(input,SDL_SCANCODE_LEFT)) {
      queue_input(SDL_SCANCODE_LEFT, -1, 0);
    }
    else if(KeyPressed(input,SDL_SCANCODE_UP)) {
      queue_input(SDL_SCANCODE_UP, 0, -1);
    }
    else if(KeyPressed(input,SDL_SCANCODE_DOWN)) {
      queue_input(SDL_SCANCODE_DOWN, 0, 1);
    }

    if(data->audio.beat_clock.beat_triggered_this_frame){
      bool new_beat = gameplay->last_processed_beat_index != data->audio.beat_clock.beat_index;
      if(new_beat){
        gameplay->last_processed_beat_index = data->audio.beat_clock.beat_index;
        gameplay->idle_check_pending = true;
      }
    }

    if(gameplay->idle_check_pending && data->audio.beat_clock.beat_progress_01 > BEAT_TOLERANCE){
      gameplay->idle_check_pending = false;
      bool caught_up = gameplay->input_buffer_write_count == gameplay->input_buffer_read_count;
      if(gameplay->beat_streak > 0 && !gameplay->input_queued_since_last_beat && caught_up){
        ResetStreak(data, true);
      }
      gameplay->input_queued_since_last_beat = false;
    }
    
    bool are_entities_acting = false;
    LevelData *level = GetCurrentLevel(gameplay);
    Entity* entityBuffer = level->entityBuffer;

    for (int i = 0; i < level->entityCount; i++){
      if(IsActing(&entityBuffer[i])){
        are_entities_acting = true;
        break;
      }
    }

    for (int i = 0; i < level->goalCount; i++) {
      Entity* entity = GetEntity(level, level->goals[i].x, level->goals[i].y);
      if(entity != nullptr && !IsActing(entity)){
        level->goals[i].blink_timer += dt;
      }
      else{
        level->goals[i].blink_timer = 0;
      }
    }

    if (level->goalCount > 0) {
      int goals_reached = 0;
      
      for (int i = 0; i < level->goalCount; i++) {
        Goal goal = level->goals[i];
        Entity* entity = GetEntity(level, goal.x, goal.y);
        
        if (entity == nullptr) {
          break;
        }
        else if (HasBehaviour(entity, Behaviour::IS_PLAYER)) {
          goals_reached++;
        }
      }
      
      if (goals_reached == level->goalCount) {
        gameplay->currentLevelIndex++;
        
        if (gameplay->currentLevelIndex >= TOTAL_LEVELS) {
          gameplay->currentLevelIndex = TOTAL_LEVELS - 1;
          ChangeScene(data, SCENE_TYPES::MAINMENU);
          return;
      }
  
        StartLevel(gameplay, arena_commands, arena_entities);
        SetMusicFromStreak(data);
        return;
      }
    }

    for (int i = 0; i < level->entityCount; i++){
      Entity* entity = &entityBuffer[i];
      if(!entity->active) continue;
        switch(entity->action){
        case Actions::NONE:
          continue;
        case Actions::MOVING:
          entity->progress_01 += MOVE_SPEED * dt;
            break;
        case Actions::ROTATING:
          entity->progress_01 += 8 * dt;
          break;
        case Actions::BUMPING:
          entity->progress_01 += BUMP_SPEED * dt;
          break;
      }
    }

    for (int i = 0; i < level->entityCount; i++){
      Entity* entity = &entityBuffer[i];
      if(entity->progress_01 >= 1){
        entity->x_prev = entity->x;
        entity->y_prev = entity->y;
        entity->facing_previous = entity->facing_current;
        entity->action = Actions::NONE;
        entity->progress_01 = 0;
        if(HasBehaviour(entity, Behaviour::IS_PUSHING)){
          RemoveBehaviour(entity, Behaviour::IS_PUSHING);
        }
      }
    }
    
    int player_count = 0;
    for (int i = 0; i < level->entityCount; i++) {
      if(entityBuffer[i].active == false){
        continue;
      }
      if(HasBehaviour(&level->entityBuffer[i], (Behaviour)(IS_PLAYER))){
        player_count++;
      }
    }

    int index = 0;
    gameplay->activePlayerBuffer = ALLOC_ARRAY(arena_scratch, Entity*, player_count);
    for (int i = 0; i < level->entityCount; i++) {
      if(entityBuffer[i].active == false){
        continue;
      }
      if(HasBehaviour(&level->entityBuffer[i], (Behaviour)(IS_PLAYER))){
        gameplay->activePlayerBuffer[index++] = &level->entityBuffer[i];
      }
    }
    
    if(are_entities_acting == false && KeyPressed(input, SDL_SCANCODE_G) && player_count > 0){
      SetBoolCommand set_control_all(&gameplay->control_all_players, gameplay->control_all_players ? false : true);
      Push(gameplay->commandBuffer, set_control_all, level);
    }

    if(are_entities_acting == false && KeyPressed(input, SDL_SCANCODE_X) && player_count > 0){
      if (gameplay->control_all_players){
        SetBoolCommand set_control_all(&gameplay->control_all_players, false);
        Push(gameplay->commandBuffer, set_control_all, level);
      }
      SwapActiveEntityCommand swap(&gameplay->activePlayerIndex, player_count);
      Push(gameplay->commandBuffer, swap, GetCurrentLevel(gameplay));
      gameplay->commandBuffer->timestamp += 1;
    }
        
    if(are_entities_acting){
      return;
    }
    if(gameplay->input_buffer_read_count == gameplay->input_buffer_write_count){
      return;
    }
    
    if (!gameplay->control_all_players) {
      Entity* entity = GetActiveEntity(gameplay);
      
      if(!HasBehaviour(entity, (Behaviour)(RESPOND_TO_INPUT | CAN_MOVE))){
        return;
      }
      
      if(HasBehaviour(entity, Behaviour::IS_PETRIFIED)){
        return;
      }
      
      int read_idx = gameplay->input_buffer_read_count % gameplay->input_buffer_capacity;
      int xDir = gameplay->input_buffer[read_idx].x;
      int yDir = gameplay->input_buffer[read_idx].y;
      bool was_on_beat = gameplay->input_buffer_on_beat[read_idx];
      unsigned int target_beat = gameplay->input_buffer_target_beat[read_idx];

      // Direction new_facing = DirectionFromXY(xDir, yDir);

      // if (new_facing != entity->facing_current) { 
      //   RotateCommand rotate(entity, entity->facing_current, new_facing);
      //   Push(gameplay->commandBuffer, rotate, level);
      //   return;
      // }

      if (!IsActing(entity)) {
        bool already_credited_this_beat = ((int)target_beat == gameplay->last_credited_beat_index);
        bool gains_streak_this_input = was_on_beat && !already_credited_this_beat;
        bool can_push = gameplay->beat_streak >= REQUIRED_BEAT_STREAK || (gameplay->beat_streak + (gains_streak_this_input ? 1 : 0)) >= REQUIRED_BEAT_STREAK;
        int effective_strength = entity->strength + (gameplay->use_evolved_bass ? 1 : 0);
        // bool can_enter_goal = gameplay->currentLevelIndex == 0 || gameplay->is_streak_active;
        bool can_enter_goal = true;
        TryMove(entity, level, gameplay->commandBuffer, xDir, yDir, effective_strength, can_push, &gameplay->player_inventory, can_enter_goal);

        if (already_credited_this_beat) {
          ResetStreak(data, true);
        } else if (was_on_beat) {
          gameplay->beat_streak += 1;
          gameplay->last_credited_beat_index = (int)target_beat;
        } else {
          gameplay->beat_streak = 0;
        }

        if(gameplay->beat_streak >= REQUIRED_BEAT_STREAK){
          gameplay->sustained_streak_turns += 1;
        } else {
          gameplay->sustained_streak_turns = 0;
        }

        SetMusicFromStreak(data);
        gameplay->input_buffer_read_count++;
        gameplay->player_step_count++;

        if(gameplay->player_step_count % 2 == 0){
          UpdateChasers(data, level);
        }
        gameplay->commandBuffer->timestamp += 1;
      }
    
  } else {
    int read_idx = gameplay->input_buffer_read_count % gameplay->input_buffer_capacity;
    int xDir = gameplay->input_buffer[read_idx].x;
    int yDir = gameplay->input_buffer[read_idx].y;
    bool was_on_beat = gameplay->input_buffer_on_beat[read_idx];
    unsigned int target_beat = gameplay->input_buffer_target_beat[read_idx];

    bool already_credited_this_beat = ((int)target_beat == gameplay->last_credited_beat_index);
    bool gains_streak_this_input = was_on_beat && !already_credited_this_beat;
    bool can_push = gameplay->beat_streak >= REQUIRED_BEAT_STREAK || (gameplay->beat_streak + (gains_streak_this_input ? 1 : 0)) >= REQUIRED_BEAT_STREAK;
    // bool can_enter_goal = gameplay->currentLevelIndex == 0 || gameplay->is_streak_active;
    bool can_enter_goal = true;

    bool any_player_could_act = false;

    for (int p = 0; p < player_count; p++) {
      Entity* player_entity = gameplay->activePlayerBuffer[p];

      if(!HasBehaviour(player_entity, (Behaviour)(RESPOND_TO_INPUT | CAN_MOVE))){
        continue;
      }
      if(HasBehaviour(player_entity, Behaviour::IS_PETRIFIED)){
        continue;
      }
      if(IsActing(player_entity)){
        continue;
      }

      any_player_could_act = true;
      int effective_strength = player_entity->strength + (gameplay->use_evolved_bass ? 1 : 0);
      TryMove(player_entity, level, gameplay->commandBuffer, xDir, yDir, effective_strength, can_push, &gameplay->player_inventory, can_enter_goal);
    }

    if (any_player_could_act) {
      if (already_credited_this_beat) {
        ResetStreak(data, true);
      } else if (was_on_beat) {
        gameplay->beat_streak += 1;
        gameplay->last_credited_beat_index = (int)target_beat;
      } else {
        gameplay->beat_streak = 0;
      }

      if(gameplay->beat_streak >= REQUIRED_BEAT_STREAK){
        gameplay->sustained_streak_turns += 1;
      } else {
        gameplay->sustained_streak_turns = 0;
      }

      SetMusicFromStreak(data);
      gameplay->input_buffer_read_count++;
      gameplay->player_step_count++;

      if(gameplay->player_step_count % 2 == 0){
        UpdateChasers(data, level);
      }
      gameplay->commandBuffer->timestamp += 1;
    }
  }
    
    // int xDir = gameplay->input_buffer[gameplay->input_buffer_read_count % gameplay->input_buffer_capacity].x;
    // int yDir = gameplay->input_buffer[gameplay->input_buffer_read_count % gameplay->input_buffer_capacity].y;

    // Direction new_facing = DirectionFromXY(xDir, yDir);
    
    // if(new_facing != entity->facing_current){ 
    //   RotateCommand rotate(entity, entity->facing_current, new_facing);
    //   Push(gameplay->commandBuffer, rotate, level);
    //   return;
    // }

    // if(!IsActing(entity)){
    //   bool moved = TryMove(entity, level, gameplay->commandBuffer, xDir, yDir, entity->strength);
    //   if(moved){
    //     PlaySFX(SFX_ID::JUMP);
    //   }
    //   gameplay->commandBuffer->timestamp += 1;
    //   gameplay->input_buffer_read_count++;
    // }
  }
    
  void Update(GameData* data, float dt) {
    UpdateAudioInputs(&data->input);
    UpdateBeatClock(&data->audio);
    camera::UpdateShake(&data->camera, dt);

    if (AudioActionPressed(&data->input, AudioInputAction::MUTE_ALL)) {
      ToggleMasterMute(&data->audio);
    }

    *data->ticks_total += 1;
    
    TitleScreen* titlescreen = &data->scenes.titlescreen;
    Gameplay* gameplay = &data->scenes.gameplay;
    EditorData* editorData = &data->editor_data;
    Transition* transition = &data->transition;

    // if(KeyPressed(&data->input, SDL_SCANCODE_F2)){
    //   editorData->edit_level = !editorData->edit_level;
    // }
    // if(editorData->edit_level){
    //   EDITOR::Update(&editorData->editor, &data->input, GetCurrentLevel(gameplay), gameplay->commandBuffer);
    // }
    // if(KeyPressed(&data->input, SDL_SCANCODE_5)){
    //   ChangeScene(data, SCENE_TYPES::TITLESCREEN);
    //   return;
    // }
    
    if(transition->state != Transition::Inactive){
      transition->fade_time_elapsed += dt;
      if(transition->fade_time_elapsed >= transition->fade_time_duration){
        transition->fade_time_elapsed = 0;
        switch (transition->state) {
        case Transition::Inactive:
          break;
        case Transition::FadeTo:
          transition->state = Transition::FadeFrom;
          break;
        case Transition::FadeFrom:
          transition->state = Transition::Inactive;
          break;
        }
      }
    }

    switch(data->scene_current){
    case SCENE_TYPES::TITLESCREEN:
      UpdateTitlescreen(titlescreen, dt);
      if(AnyKeyPressed(&data->input)){
        if(transition->state == Transition::FadeTo || transition->state == Transition::Inactive){
          ChangeScene(data, SCENE_TYPES::GAME);
        }
      }
      break;
    case SCENE_TYPES::MAINMENU:
      UpdateMenu(data);
      break;
    case SCENE_TYPES::GAME:
      UpdateGame(data, dt);
      break;
    case SCENE_TYPES::CREDITS:
      break;
    case SCENE_TYPES::NONE:
      assert(false);
      break;
    }
  }
  
  bool TryMove(Entity* mover, LevelData* level, CommandBuffer* commandBuffer, int xDir, int yDir, int strength, bool can_push, Inventory* inventory, bool can_enter_goal) {
    if (HasBehaviour(mover, CAN_MOVE) == false) {
      return false;
    }

    if (strength < 0) {
      return false;
    }

    int test_x = mover->x + xDir;
    int test_y = mover->y + yDir;
    Entity* stepInto_entity = GetEntity(level, test_x, test_y);

    bool mover_is_player = HasBehaviour(mover, Behaviour::IS_PLAYER);

    if (mover_is_player && !can_enter_goal && IsGoalTile(level, test_x, test_y)) {
      TriggerBump(mover, xDir, yDir);
      return false;
    }

    if (mover_is_player && stepInto_entity != nullptr && HasBehaviour(stepInto_entity, Behaviour::IS_ITEM)) {
      if (stepInto_entity->item_id == ITEM_ID::KEY) {
        if(inventory != nullptr && !inventory->has_key){
          SetBoolCommand set_key(&inventory->has_key, true);
          Push(commandBuffer, set_key, level);
        }
      }
      RemoveCommand remove(stepInto_entity);
      Push(commandBuffer, remove, level);
      stepInto_entity = nullptr;
    }

    if (stepInto_entity != nullptr && HasBehaviour(stepInto_entity, Behaviour::IS_LOCKED_DOOR)) {
      if (mover_is_player && inventory->has_key) {
        RemoveCommand remove(stepInto_entity);
        Push(commandBuffer, remove, level);
        stepInto_entity = nullptr;
      } else {
        TriggerBump(mover, xDir, yDir);
        return false;
      }
    }

    if (stepInto_entity == nullptr) {
      if (IsWalkable(test_x, test_y, level)) {
        MoveCommand mv(mover, xDir, yDir);
        Push(commandBuffer, mv, level);

        return true;
      }

      TriggerBump(mover, xDir, yDir);
      return false;
    }
      
    if (HasBehaviour(stepInto_entity, CAN_MOVE) && !HasBehaviour(stepInto_entity, UNPUSHABLE)) {
      if (!can_push) {
        TriggerBump(mover, xDir, yDir);
        TriggerBump(stepInto_entity, xDir, yDir);
        return false;
      }

      if (TryMove(stepInto_entity, level, commandBuffer, xDir, yDir, --strength, can_push, inventory, can_enter_goal)) {
        MoveCommand mv(mover, xDir, yDir);
        AddBehaviour(mover, Behaviour::IS_PUSHING);
        Push(commandBuffer, mv, level);

        return true;
      }

      TriggerBump(mover, xDir, yDir);
      TriggerBump(stepInto_entity, xDir, yDir);
      return false;
    }

    TriggerBump(mover, xDir, yDir);
    return false;
  }

  void DrawSuccessfulBeatStreak(GameData* data, SDL_Renderer* renderer) {
    int streak = data->scenes.gameplay.beat_streak;
    int max_streak = REQUIRED_BEAT_STREAK;
    float icon_size = 20.0f;
    float spacing = 5.0f;
    float total_width = (icon_size * max_streak) + (spacing * (max_streak - 1));
    float start_x = (SCREEN_WIDTH - total_width) / 2.0f;
    float y = SCREEN_HEIGHT - icon_size - 120.0f;

    for (int i = 0; i < max_streak; ++i) {
      float x = start_x + i * (icon_size + spacing);
      if (i < streak) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
      } else {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); 
      }
      SDL_FRect rect = {x, y, icon_size, icon_size};
      SDL_RenderFillRect(renderer, &rect);
    }
  }

  void DrawBeatIndicator(GameData* data, SDL_Renderer* renderer){
    AudioSystem* audio = &data->audio;
    float progress = audio->beat_clock.beat_progress_01;

    constexpr int NUM_MARKERS = 2;
    float track_y = SCREEN_HEIGHT - 60.0f;
    float center_x = SCREEN_WIDTH / 2.0f;
    float marker_width = 8.0f;
    float marker_height = 40.0f; 

    SDL_SetRenderDrawColor(renderer, 90, 70, 110, 255);
    //SDL_RenderLine(renderer, 0, track_y, (float)SCREEN_WIDTH, track_y);

    for (int k = 0; k < NUM_MARKERS; k++) {
      float progress_k = (NUM_MARKERS - 1 - k + progress) / (float)NUM_MARKERS;
      float left_x = std::lerp(0.0f, center_x, progress_k);
      float right_x = std::lerp((float)SCREEN_WIDTH, center_x, progress_k);
      float emphasis = 1.0f - ((float)k / (float)NUM_MARKERS);
      
      float current_width = marker_width * (0.5f + 0.5f * emphasis);
      float current_height = marker_height * (0.5f + 0.5f * emphasis);

      Uint8 brightness = (Uint8)(120 + 135 * emphasis);

      SDL_SetRenderDrawColor(renderer, brightness, brightness, 255, 255);

      SDL_FRect left_marker = {
        left_x - current_width / 2.0f, 
        track_y - current_height / 2.0f, 
        current_width, 
        current_height
      };
      SDL_FRect right_marker = {
        right_x - current_width / 2.0f, 
        track_y - current_height / 2.0f, 
        current_width, 
        current_height
      };

      SDL_RenderFillRect(renderer, &left_marker);
      SDL_RenderFillRect(renderer, &right_marker);
    }

    float heart_base_size = 50.0f;
    float heart_size = heart_base_size * GetBeatPulseScale(audio, 0.5f);
    bool flash = audio->beat_clock.beat_triggered_this_frame;

    if (flash) {
      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    } else {
      SDL_SetRenderDrawColor(renderer, 220, 60, 90, 255);
    }

    SDL_FRect heart = {center_x - heart_size / 2.0f, track_y - heart_size / 2.0f, heart_size, heart_size};
    SDL_RenderFillRect(renderer, &heart);
  }
  
  void DrawLevelHint(GameData* data, SDL_Renderer* renderer) {
    const char* hint = data->scenes.gameplay.current_hint;
    if (IsStringEmpty(hint)) {
      return;
    }
    float y = 60.0f;
    RenderText(&data->font, hint, renderer, nullptr, SCREEN_WIDTH / 2.0f, y, Alignment::Centered);
  }

  void DrawScene(GameData* data, SCENE_TYPES scene, SDL_Renderer* renderer) {
    switch (scene) {
      case SCENE_TYPES::TITLESCREEN:
        RenderSprite_World(data->scenes.titlescreen.background, renderer, &data->camera, 0, 0);
      break;
      case SCENE_TYPES::MAINMENU:
        DrawMenu(&data->scenes.mainMenu, renderer, data->spriteBuffer, &data->input, &data->audio);
        break;
      case SCENE_TYPES::GAME:
        RenderLevel(data, renderer);  
        RenderEntities(data, renderer);
        DrawLevelHint(data, renderer);
        DrawBeatIndicator(data, renderer);
        DrawSuccessfulBeatStreak(data, renderer);
        break;
      case SCENE_TYPES::CREDITS:
        break;
      case SCENE_TYPES::NONE:
        assert(false);
        break;
    }
  }
      
  void Draw(GameData* data, SDL_Renderer* renderer){
    // DEV::PreDraw(data->imGui_context);
    SDL_SetRenderDrawColor(renderer, 120, 70, 120, 255);
    SDL_RenderClear(renderer);

    switch(data->transition.state){
    case Transition::Inactive:
      DrawScene(data, data->scene_current, renderer);
      break;
    case Transition::FadeTo: {
        DrawScene(data, data->scene_previous, renderer);
        float alpha = data->transition.fade_time_elapsed / data->transition.fade_time_duration;
        RenderSprite_World(GetSprite(SPRITE_ID::black_1x1, data->spriteBuffer), renderer, &data->camera, 0, 0, SCREEN_WIDTH,alpha);
      break;
      }
    case Transition::FadeFrom:{
        DrawScene(data, data->scene_current, renderer);
        float alpha = 1 - data->transition.fade_time_elapsed / data->transition.fade_time_duration;
        RenderSprite_World(GetSprite(SPRITE_ID::black_1x1, data->spriteBuffer), renderer, &data->camera, 0, 0, SCREEN_WIDTH,alpha);
      break;
      }
    }
    
    // DEV::Draw(data, renderer);
    SDL_RenderPresent(renderer);
  }

  void OnQuit(SDL_Renderer* renderer){
    SDL_DestroyRenderer(renderer);
  }

}
