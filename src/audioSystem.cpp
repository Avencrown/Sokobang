#include "audioSystem.h"
#include "FMOD/fmod.h"
#include "FMOD/fmod_common.h"
#include "arena.h"
#include "common.h"
#include <cassert>
#include <cmath>

// global
AudioSystem* g_audioSystem;

static const SoundDataEntry all_sound_data[] = {
  {SFX_ID::FALLBACK, "assets/audio/sfx/fallback.wav"},
  {SFX_ID::JUMP, "assets/audio/sfx/fallback.wav"},
};

void InitializeAudioSystem(AudioSystem* audio, Memory::Arena* arena_main){
  assert(audio->initialized == false);

  size_t memory_size = AUDIO_MEMORY_ALLOWANCE;
  audio->fmod_memory = Memory::CreateSubArena(arena_main, memory_size);
  FMOD_RESULT memory_init_ok = FMOD_Memory_Initialize(audio->fmod_memory, memory_size, nullptr, nullptr, nullptr, FMOD_MEMORY_ALL);
  assert(memory_init_ok == FMOD_OK);

  FMOD_RESULT system_creation_ok = FMOD_System_Create(&audio->sound_system, FMOD_VERSION);
  assert(system_creation_ok == FMOD_OK);

  FMOD_RESULT system_init_ok = FMOD_System_Init(audio->sound_system, AudioSystem::CHANNEL_COUNT, FMOD_INIT_NORMAL, nullptr);
  assert(system_init_ok == FMOD_OK);

  g_audioSystem = audio;

  audio->initialized = true;
}

const int NOT_FOUND = -1;

int GetAvailableChannelIndex(AudioSystem* audio){
  for (int i = 0; i < AudioSystem::CHANNEL_COUNT; i++) {
    FMOD_CHANNEL* channel = audio->channels[i];
    if(channel == nullptr){ // never used before
      return i;
    }
    FMOD_BOOL is_playing = false;
    FMOD_Channel_IsPlaying(channel, &is_playing);
    if(is_playing == false){
      return i;
    }
  }

  return NOT_FOUND;
}

void PlaySFX(SFX_ID id, float volume){
  assert(id != SFX_ID::COUNT);
  FMOD_SOUND* sfx = g_audioSystem->soundEffects[(int)id];
  if(sfx == nullptr){
    assert(id != SFX_ID::FALLBACK);
    PlaySFX(SFX_ID::FALLBACK);
  }
  int channel_index = GetAvailableChannelIndex(g_audioSystem);
  if(channel_index == NOT_FOUND){
    return;
  }
  FMOD_CHANNEL** channel_slot = &g_audioSystem->channels[channel_index];

  FMOD_System_PlaySound(g_audioSystem->sound_system, sfx, nullptr, false, channel_slot);
  FMOD_Channel_SetVolume(*channel_slot, volume);
}

void PlaySong(SONG_ID id){
  g_audioSystem->song_id = id;

  if (g_audioSystem->song != nullptr) {
    FMOD_Channel_Stop(g_audioSystem->song_channel);
    FMOD_Sound_Release(g_audioSystem->song);
  } 

  FMOD_SYSTEM* system = g_audioSystem->sound_system;
  const char* song_name;

  switch (id) {
    case SONG_ID::THEME:
      song_name = "assets/audio/music/hellofatime.mp3";
      break;
    default:
      song_name = nullptr;
      break;
  }
  
  FMOD_System_CreateStream(system, song_name, FMOD_LOOP_NORMAL, nullptr, &g_audioSystem->song);
  int INFINITE = -1;
  FMOD_Sound_SetLoopCount(g_audioSystem->song, INFINITE);
  FMOD_System_PlaySound(system, g_audioSystem->song, nullptr, false, &g_audioSystem->song_channel);
}

void Update(AudioSystem* audio){
  if(g_audioSystem == nullptr || g_audioSystem != audio){
    g_audioSystem = audio;
  }

  assert(audio->initialized);
  FMOD_System_Update(audio->sound_system);
}

namespace AssetManagement{
  void LoadAllSFX(AudioSystem* audioSystem){
    for (const SoundDataEntry& sound_data : all_sound_data) {
      FMOD_RESULT sound_created_ok = FMOD_System_CreateSound(audioSystem->sound_system, sound_data.path, FMOD_DEFAULT, nullptr, &audioSystem->soundEffects[(int)sound_data.id]);
      assert(sound_created_ok == FMOD_OK);
    }
  }
}

void LoadMusicStems(AudioSystem* audio, const char* drums_path, const char* bass_path, const char* bass2_path, const char* harmony_path) {
  StopAllStems(audio);

  const char* paths[(int)MUSIC_STEM::COUNT] = { drums_path, bass_path, bass2_path, harmony_path };

  for (int i = 0; i < (int)MUSIC_STEM::COUNT; i++) {
    if (!paths[i])
      continue;

    FMOD_RESULT res = FMOD_System_CreateStream(
      audio->sound_system, 
      paths[i], 
      FMOD_LOOP_NORMAL | FMOD_2D, 
      nullptr, 
      &audio->stem_sounds[i]
    );

    assert(res == FMOD_OK);
    FMOD_Sound_SetLoopCount(audio->stem_sounds[i], -1);
  }
}

void StartAllStemsPaused(AudioSystem* audio, float volume) {
  for (int i = 0; i < (int)MUSIC_STEM::COUNT; i++) {
    if (audio->stem_sounds[i]) {
      FMOD_System_PlaySound(audio->sound_system, audio->stem_sounds[i], nullptr, true, &audio->stem_channels[i]);
      audio->stem_playing[i] = true;
      FMOD_Channel_SetVolume(audio->stem_channels[i], volume);
    }
  }
  for (int i = 0; i < (int)MUSIC_STEM::COUNT; i++) {
    if (audio->stem_channels[i]) {
      FMOD_Channel_SetPaused(audio->stem_channels[i], false);
    }
  }
}

void SetStemMuted(AudioSystem* audio, MUSIC_STEM stem, bool mute) {
  int idx = (int)stem;

  if (audio->stem_channels[idx]) {
    FMOD_Channel_SetMute(audio->stem_channels[idx], mute);
  }
}

void StopStem(AudioSystem* audio, MUSIC_STEM stem) {
  int idx = (int)stem;

  if (audio->stem_channels[idx]) {
    FMOD_Channel_Stop(audio->stem_channels[idx]);
    audio->stem_channels[idx] = nullptr;
  }

  if (audio->stem_sounds[idx]) {
    FMOD_Sound_Release(audio->stem_sounds[idx]);
    audio->stem_sounds[idx] = nullptr;
  }
  
  audio->stem_playing[idx] = false;
}

void StopAllStems(AudioSystem* audio) {
  for (int i = 0; i < (int)MUSIC_STEM::COUNT; i++) {
    StopStem(audio, (MUSIC_STEM)i);
  }
}

static void ApplyStemMuteState(AudioSystem* audio, MUSIC_STEM stem) {
  int idx = (int)stem;
  SetStemMuted(audio, stem, audio->stem_muted[idx] || audio->master_muted);
}

void SetStemMutedState(AudioSystem* audio, MUSIC_STEM stem, bool muted) {
  int idx = (int)stem;
  audio->stem_muted[idx] = muted;
  ApplyStemMuteState(audio, stem);
}

void ToggleStemMute(AudioSystem* audio, MUSIC_STEM stem) {
  int idx = (int)stem;
  audio->stem_muted[idx] = !audio->stem_muted[idx];
  ApplyStemMuteState(audio, stem);
}

void ToggleMasterMute(AudioSystem* audio) {
  audio->master_muted = !audio->master_muted;
  
  for (int i = 0; i < (int)MUSIC_STEM::COUNT; i++) {
    ApplyStemMuteState(audio, (MUSIC_STEM)i);
  }
}

void InitializeBeatClock(AudioSystem* audio, float bpm) {
  audio->beat_clock.bpm = bpm;
  audio->beat_clock.beat_index = 0;
  audio->beat_clock.beat_progress_01 = 0.0f;
  audio->beat_clock.beat_triggered_this_frame = false;
}

void UpdateBeatClock(AudioSystem* audio) {
  BeatClock* clock = &audio->beat_clock;
  clock->beat_triggered_this_frame = false;

  FMOD_CHANNEL* reference_channel = audio->stem_channels[(int)MUSIC_STEM::DRUMS];
  if (reference_channel == nullptr) {
    return;
  }

  unsigned int position_ms = 0;
  FMOD_RESULT result = FMOD_Channel_GetPosition(reference_channel, &position_ms, FMOD_TIMEUNIT_MS);
  if (result != FMOD_OK) {
    return;
  }

  float beat_duration_ms = 60000.0f / clock->bpm;
  unsigned int new_beat_index = (unsigned int)(position_ms / beat_duration_ms);
  clock->beat_progress_01 = fmodf((float)position_ms, beat_duration_ms) / beat_duration_ms;

  if (new_beat_index != clock->beat_index) {
    clock->beat_triggered_this_frame = true;
  }
  clock->beat_index = new_beat_index;
}

bool IsDrumsAudible(AudioSystem* audio) {
  return !audio->stem_muted[(int)MUSIC_STEM::DRUMS] && !audio->master_muted;
}

float GetBeatPulseScale(AudioSystem* audio, float intensity) {
  if (!IsDrumsAudible(audio)) {
    return 1.0f;
  }

  float decay = 1.0f - audio->beat_clock.beat_progress_01;
  return 1.0f + intensity * decay * decay;
}