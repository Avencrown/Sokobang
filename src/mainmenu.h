#pragma once

#include "input.h"
#include "audioSystem.h"

struct GameData;
struct SDL_Renderer;
struct Button;
struct Sprite;
struct FontAtlas;
namespace Memory{
  struct Arena;
}

struct MainMenu {
  Button* buttons;
  int button_count;
  int activeButtonIndex;
  Button** activeButtons;
  int activeButtonCount;
  bool initialized;
  Sprite* bg_blue;
  Sprite* blue_ball;
  Sprite* blue_ball_sized;
  // Sprite* background_horizon;
  // Sprite* background_cloud_back;
  // Sprite* background_cloud_front;
  // Sprite* background_middle;
  // Sprite* background_front;
};

void InitializeMenu(MainMenu* mainmenu, Sprite* spriteBuffer, FontAtlas* font, Memory::Arena* arena_main);
void UpdateMenu(GameData* data);
void DrawMenu(MainMenu* mainmenu, SDL_Renderer* renderer, Sprite* spriteBuffer, Input* input, AudioSystem* audio);