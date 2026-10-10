#include "input.h"
#include "SDL3/SDL_mouse.h"
#include <cstring>

bool AnyKeyPressed(const Input *input){
  for (int i = 0; i < SDL_SCANCODE_COUNT; i++) {
    if(KeyPressed(input, (SDL_Scancode)i)){
      return true;
    }
  }
    return false;
}

bool KeyPressed(const Input* input, SDL_Scancode key){
  if(input->keys_previous == nullptr){
    return input->keys_current[key];
  }
  return input->keys_current[key] && !input->keys_previous[key];
}

bool KeyHeld(const Input* input, SDL_Scancode key){
  if(input->keys_previous == nullptr){
    return false;
  }
  return input->keys_current[key] && input->keys_previous[key];
}

bool KeyReleased(const Input* input,SDL_Scancode key){
  if(input->keys_previous == nullptr){
    return false;
  }
  return !input->keys_current[key] && input->keys_previous[key];
}

bool KeyHeld_ForTime(const Input* input, SDL_Scancode key, float min_length){
  return input->keys_held_time[key] >= min_length; 
}

void UpdateKeys(Input* input, float dt){
  for (int i = 0; i < SDL_SCANCODE_COUNT; i++) {
    if (input->keys_current[i]){
        input->keys_held_time[i] += dt;
    }
    else{
        input->keys_held_time[i] = 0;
    }
  }
  memcpy((void*)input->keys_previous, input->keys_current, SDL_SCANCODE_COUNT * sizeof(bool));
}

void ResetKeyHeldTime(Input* input,SDL_Scancode key){
  input->keys_held_time[key] = 0;
}
 
void ResetAll(Input* input){
  memset((void*)input->keys_current, 0, sizeof(bool) * SDL_SCANCODE_COUNT);
  memset((void*)input->keys_previous, 0, sizeof(bool) * SDL_SCANCODE_COUNT);
  memset((void*)input->keys_held_time, 0, sizeof(float) * SDL_SCANCODE_COUNT);
}

SDL_MouseButtonFlags ButtonToFlag(MouseButtons button){
  switch(button){
  case MouseButtons::LEFT:
    return SDL_BUTTON_LMASK;
  case MouseButtons::MIDDLE:
    return SDL_BUTTON_MMASK;
  case MouseButtons::RIGHT:
    return SDL_BUTTON_RMASK;
    break;
  case MouseButtons::COUNT:
    return -1;
    break;
  }
}
 
bool MousePressed(const Input* input, MouseButtons button){
  SDL_MouseButtonFlags flag = ButtonToFlag(button);
  return (input->mouse_current & flag) != 0 && (input->mouse_previous & flag) == 0;
}

bool MouseReleased(const Input* input, MouseButtons button){
  SDL_MouseButtonFlags flag = ButtonToFlag(button);
  return (input->mouse_current & flag) == 0 && (input->mouse_previous & flag) != 0;
}

bool MouseHeld(const Input* input, MouseButtons button){
  SDL_MouseButtonFlags flag = ButtonToFlag(button);
  return (input->mouse_current & flag) != 0 && (input->mouse_previous & flag) != 0;
}

bool MouseHeld_ForTime(const Input* input, MouseButtons button, float min_length){
  SDL_MouseButtonFlags flag = ButtonToFlag(button);
  return input->mouse_held_time[flag] >= min_length;
}

void UpdateMouse(Input* input, float dt){
  if(MouseHeld(input, MouseButtons::LEFT)){
    input->mouse_held_time[(int)MouseButtons::LEFT] += dt;
  }
  else{
    input->mouse_held_time[(int)MouseButtons::LEFT] = 0;
  }

  if(MouseHeld(input, MouseButtons::MIDDLE)){
    input->mouse_held_time[(int)MouseButtons::MIDDLE] += dt;
  }
  else{
    input->mouse_held_time[(int)MouseButtons::MIDDLE] = 0;
  }
  
  if(MouseHeld(input, MouseButtons::RIGHT)){
    input->mouse_held_time[(int)MouseButtons::RIGHT] += dt;
  }
  else{
    input->mouse_held_time[(int)MouseButtons::RIGHT] = 0;
  }

  input->mouse_previous = input->mouse_current;
}

bool IsInputOnBeat(AudioSystem* audio, float tolerance_fraction) {
  float progress = audio->beat_clock.beat_progress_01;
  float distance_to_nearest_beat = progress < (1.0f - progress) ? progress : (1.0f - progress);
  return distance_to_nearest_beat <= tolerance_fraction;
}

void UpdateAudioInputs(Input* input) {
    input->audio_actions.toggle_drums = KeyPressed(input, SDL_SCANCODE_1);
    input->audio_actions.toggle_bass = KeyPressed(input, SDL_SCANCODE_2);
    input->audio_actions.toggle_harmony = KeyPressed(input, SDL_SCANCODE_3);
    input->audio_actions.mute_all = KeyPressed(input, SDL_SCANCODE_M);
}

bool AudioActionPressed(const Input* input, AudioInputAction action) {
  switch (action) {
    case AudioInputAction::TOGGLE_DRUMS:   return input->audio_actions.toggle_drums;
    case AudioInputAction::TOGGLE_BASS:    return input->audio_actions.toggle_bass;
    case AudioInputAction::TOGGLE_HARMONY: return input->audio_actions.toggle_harmony;
    case AudioInputAction::MUTE_ALL:       return input->audio_actions.mute_all;
    default: return false;
  }
}
