#pragma once

#include "levels.h"

struct Camera{
  float camera_x;
  float camera_y;
  float shake_x;
  float shake_y;
  float shake_time_remaining;
  float shake_magnitude;
};
namespace camera {
void GridToWorld(float* x, float* y, const LevelData* lvl);
void WorldToGrid(float x_world, float y_world, int* x, int* y, const LevelData* lvl);
bool GetIsPointInsideGrid(float x, float y, const LevelData* lvl);
void UpdateShake(Camera* camera, float dt);
void StartShake(Camera* camera, float duration_seconds, float magnitude_pixels);
};
