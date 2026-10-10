#pragma once
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_scancode.h"
#include "audioSystem.h"

enum class AudioInputAction {
    TOGGLE_DRUMS,
    TOGGLE_BASS,
    TOGGLE_HARMONY,
    MUTE_ALL,
    COUNT
};

enum class MouseButtons {
    LEFT = 0,
    MIDDLE = 1,
    RIGHT = 2,
    COUNT = 3
};

struct AudioInputState {
    bool toggle_drums;
    bool toggle_bass;
    bool toggle_harmony;
    bool mute_all;
};

struct Input {
    const bool* keys_current;
    bool*  keys_previous;
    float* keys_held_time;

    SDL_MouseButtonFlags mouse_current;
    SDL_MouseButtonFlags mouse_previous;
    float* mouse_held_time;
    float mouse_x;
    float mouse_y;
    float mouse_x_delta;
    float mouse_y_delta;
    double mouse_magnitude;

    AudioInputState audio_actions;
};

bool AnyKeyPressed(const Input* input);
bool KeyPressed(const Input* input, SDL_Scancode key);
bool KeyHeld(const Input* input, SDL_Scancode key);
bool KeyReleased(const Input* input, SDL_Scancode key);
bool KeyHeld_ForTime(const Input* input, SDL_Scancode key, float min_length);
void UpdateKeys(Input* input, float dt);
void ResetKeyHeldTime(Input* input, SDL_Scancode key);
void ResetAll(Input* input);

bool MousePressed(const Input* input, MouseButtons button);
bool MouseReleased(const Input* input, MouseButtons button);
bool MouseHeld(const Input* input, MouseButtons button);
bool MouseHeld_ForTime(const Input* input, MouseButtons button, float min_length);
void UpdateMouse(Input* input, float dt);

void UpdateAudioInputs(Input* input);
bool AudioActionPressed(const Input* input, AudioInputAction action);
bool IsInputOnBeat(AudioSystem* audio, float tolerance_fraction = 0.15f);