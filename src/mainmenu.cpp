#include "mainmenu.h"
#include "SDL3/SDL_scancode.h"
#include "arena.h"
#include "button.h"
#include "common.h"
#include "fontLibrary.h"
#include "gameState.h"
#include "input.h"
#include "rendering.h"
#include "spriteLibrary.h"
#include <cassert>

 
void InitializeMenu(MainMenu* mainmenu, Sprite* spriteBuffer, FontAtlas* font, Memory::Arena* arena_main){
  assert(mainmenu->initialized == false);
  mainmenu->button_count = 2;
  mainmenu->buttons = ALLOC_ARRAY(arena_main, Button, mainmenu->button_count);
  
  SetupButton(&mainmenu->buttons[0], ButtonType::START_GAME, spriteBuffer, {SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0, 300, 110}, Alignment::Centered, font, "Start Game");
   
  mainmenu->buttons[0].is_dynamic = true;
  
  SetupButton(&mainmenu->buttons[1], ButtonType::QUIT, spriteBuffer, {SCREEN_WIDTH / 2.0, (SCREEN_HEIGHT / 2.0) + 200, 200, 80}, Alignment::Centered, font, "Quit");  

  mainmenu->bg_blue = GetSprite(SPRITE_ID::Title_Bg_Blue, spriteBuffer);
  // mainmenu->background_cloud_back = GetSprite(SPRITE_ID::Menu_Cloud_Back, spriteBuffer);
  // mainmenu->background_cloud_front = GetSprite(SPRITE_ID::Menu_Cloud_Front, spriteBuffer);
  // mainmenu->background_middle = GetSprite(SPRITE_ID::Menu_Middle, spriteBuffer);
  mainmenu->blue_ball_sized = GetSprite(SPRITE_ID::Title_Blue_Ball_Sized, spriteBuffer);

  /*
  
    data->scenes.titlescreen.background = GetSprite(SPRITE_ID::Title_Bg_Blue, data->spriteBuffer);
    data->scenes.titlescreen.ball_overlay = GetSprite(SPRITE_ID::Title_Blue_Ball, data->spriteBuffer);
    data->scenes.titlescreen.ball_sized = GetSprite(SPRITE_ID::Title_Blue_Ball_Sized, data->spriteBuffer);
    */

  mainmenu->activeButtonIndex = 0;
  mainmenu->initialized = true;
}

void UpdateMenu(GameData* data){
  MainMenu* mainmenu = &data->scenes.mainMenu;
  Input* input = &data->input;

  if(KeyPressed(input, SDL_SCANCODE_1)){
    ToggleStemMute(&data->audio, MUSIC_STEM::DRUMS);
  }
  if(KeyPressed(input, SDL_SCANCODE_2)){
    ToggleStemMute(&data->audio, MUSIC_STEM::BASS);
  }
  if(KeyPressed(input, SDL_SCANCODE_3)){
    ToggleStemMute(&data->audio, MUSIC_STEM::BASS2);
  }
  if(KeyPressed(input, SDL_SCANCODE_4)){
    ToggleStemMute(&data->audio, MUSIC_STEM::CHORDS);
  }

  mainmenu->activeButtonCount = GetActiveButtonCount(mainmenu->buttons, mainmenu->button_count);
  if(mainmenu->activeButtonCount == 0){
    return;
  }
  mainmenu->activeButtons = ALLOC_ARRAY(data->arena_scratch, Button*, mainmenu->activeButtonCount);
  
  int index = 0;
  for (int i = 0; i < mainmenu->button_count; i++) {
    Button* button = &mainmenu->buttons[i];
    if(button->is_active){
      mainmenu->activeButtons[index] = button;
      index += 1;
    }
  }

  int* buttonIndex = &mainmenu->activeButtonIndex;

  bool anyHoveredOver = false;
  bool mouseMoving = input->mouse_magnitude > 0.1;
  if(mouseMoving){
     for (int i = 0; i < mainmenu->activeButtonCount; i++) {
      Button* button = mainmenu->activeButtons[i];
      if(IsHoveredOver(button, input->mouse_x, input->mouse_y)){
        anyHoveredOver = true;
        *buttonIndex = i;
        break;
      }
    } 
  }

  bool up = KeyPressed(input, SDL_SCANCODE_UP);
  bool down = KeyPressed(input, SDL_SCANCODE_DOWN);

  if(up || down){
    int direction = up ? 1 : -1;
    *buttonIndex += direction + mainmenu->activeButtonCount;
    *buttonIndex = *buttonIndex % mainmenu->activeButtonCount;
  }
    
  Button* selected = mainmenu->activeButtons[*buttonIndex];

  if(selected != nullptr){
    if(KeyPressed(input, SDL_SCANCODE_RETURN)){
        PressButton(mainmenu->activeButtons[*buttonIndex], data);
        return;
    }

    if(IsHoveredOver(selected, input->mouse_x, input->mouse_y)){
      if(MousePressed(input, MouseButtons::LEFT)){
        PressButton(selected, data);
        return;
      }
    } 
  }
}

void DrawMenu(MainMenu* mainmenu, SDL_Renderer* renderer, Sprite* spriteBuffer, Input* input, AudioSystem* audio){
  float scale = (SCREEN_HEIGHT / ((float)mainmenu->bg_blue->height * UPSCALE_FACTOR));
  scale *= 1.15f;

  float mouse_x = input->mouse_x;
  float mouse_y = input->mouse_y;
  float center_x = SCREEN_WIDTH / 2.0f;
  float center_y = SCREEN_HEIGHT / 2.0f;
  float offset_x = center_x - mouse_x;
  float offset_y = center_y - mouse_y;

  float front_scale = scale * GetBeatPulseScale(audio);

  RenderSprite_World(GetSprite(SPRITE_ID::Title_Bg_Blue, spriteBuffer), renderer, NULL, center_x, center_y, scale);
  
  float ball_scale = front_scale * 0.7f;
  RenderSprite_World(GetSprite(SPRITE_ID::Title_Blue_Ball_Sized, spriteBuffer), renderer, NULL, center_x + 300 + (offset_x / 10.0f), center_y + 200 + (offset_y / 10.0f), ball_scale);

  float title_y = center_y - 220.0f; 
  float title_base_scale = 2.5f;

  RenderTextAnimated(
      mainmenu->buttons[0].font, 
      "SOKOBANG", 
      renderer, 
      nullptr, 
      center_x, 
      title_y, 
      Alignment::Centered, 
      title_base_scale, 
      audio
  );

  for (int i = 0; i < mainmenu->activeButtonCount; i++) {
      Button* button = mainmenu->activeButtons[i];
      RenderButton_Primitive(button, i == mainmenu->activeButtonIndex, renderer);
  }

  mainmenu->activeButtonCount = 0;
}