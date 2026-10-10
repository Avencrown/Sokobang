#pragma once
#include "FMOD/fmod_common.h"

namespace Memory{
  struct Arena;
}

enum class MUSIC_STEM { DRUMS, BASS, BASS2, CHORDS, COUNT };

enum class SFX_ID { FALLBACK, JUMP, COUNT };

enum class SONG_ID { NONE, THEME };

struct SoundDataEntry{
  SFX_ID id;
  const char* path;
};

struct BeatClock {
  float bpm = 108.0f;
  unsigned int beat_index = 0;
  float beat_progress_01 = 0.0f;
  bool beat_triggered_this_frame = false;
};

struct AudioSystem {
  bool initialized;
  void* fmod_memory;
  FMOD_SYSTEM* sound_system;
  static const int CHANNEL_COUNT = 32;
  FMOD_CHANNEL* channels[CHANNEL_COUNT];
  FMOD_SOUND* soundEffects[(int)SFX_ID::COUNT];
  SONG_ID song_id;
  FMOD_SOUND* song;
  FMOD_CHANNEL* song_channel;
  FMOD_SOUND* stem_sounds[(int)MUSIC_STEM::COUNT];
  FMOD_CHANNEL* stem_channels[(int)MUSIC_STEM::COUNT];
  bool stem_playing[(int)MUSIC_STEM::COUNT];
  bool stem_muted[(int)MUSIC_STEM::COUNT];
  bool master_muted;
  BeatClock beat_clock;
};

// global promise
extern AudioSystem* g_audioSystem;

void PlaySong(SONG_ID id);
void PlaySFX(SFX_ID id, float volume = 1);
void InitializeAudioSystem(AudioSystem* audio, Memory::Arena* arena_main);

void LoadMusicStems(AudioSystem* audio, const char* drums_path, const char* bass_path, const char* bass2_path, const char* harmony_path);
void StartAllStemsPaused(AudioSystem* audio, float volume = 1.0f);
void ToggleStemMute(AudioSystem* audio, MUSIC_STEM stem);
void SetStemMutedState(AudioSystem* audio, MUSIC_STEM stem, bool muted);
void ToggleMasterMute(AudioSystem* audio);
void SetStemMuted(AudioSystem* audio, MUSIC_STEM stem, bool mute);
void StopStem(AudioSystem* audio, MUSIC_STEM stem);
void StopAllStems(AudioSystem* audio);
void InitializeBeatClock(AudioSystem* audio, float bpm);
void UpdateBeatClock(AudioSystem* audio);
bool IsDrumsAudible(AudioSystem* audio);
float GetBeatPulseScale(AudioSystem* audio, float intensity = 0.1f);

namespace AssetManagement{
  void LoadAllSFX(AudioSystem* audioSystem);
}
