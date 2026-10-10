#include "camera.h"
#include "common.h"
#include <cstdlib>

bool camera::GetIsPointInsideGrid(float x, float y, const LevelData* lvl){
  int x_grid;
  int y_grid;
  WorldToGrid(x, y, &x_grid, &y_grid, lvl);
  return x_grid >= 0 && y_grid >= 0 && x_grid < lvl->w && y_grid < lvl->h; 
}

void camera::GridToWorld(float* x, float* y,  const LevelData* lvl){
  *x *= TILE_SIZE_PX_SCALED;
  *x += TILE_SIZE_PX_SCALED;
  *x += SCREEN_WIDTH / 2.0;
  *x -= lvl->w * TILE_SIZE_PX_SCALED / 2.0;
  *y *= TILE_SIZE_PX_SCALED;
  *y += TILE_SIZE_PX_SCALED;
  *y += SCREEN_HEIGHT / 2.0;
  *y -= lvl->h * TILE_SIZE_PX_SCALED / 2.0;
}
void camera::WorldToGrid(float x_world, float y_world, int* x, int* y, const LevelData* lvl){
  *x = x_world;
  *y = y_world;
  *x += lvl->w * TILE_SIZE_PX_SCALED / 2.0;
  *x -= SCREEN_WIDTH / 2.0;
  *x /= TILE_SIZE_PX_SCALED;
  *y += lvl->h * TILE_SIZE_PX_SCALED / 2.0;
  *y -= SCREEN_HEIGHT / 2.0;
  *y /= TILE_SIZE_PX_SCALED;
}

void camera::StartShake(Camera* camera, float duration_seconds, float magnitude_pixels){
  camera->shake_time_remaining = duration_seconds;
  camera->shake_magnitude = magnitude_pixels;
}

void camera::UpdateShake(Camera* camera, float dt){
  if(camera->shake_time_remaining <= 0.0f){
    camera->shake_x = 0.0f;
    camera->shake_y = 0.0f;
    return;
  }

  camera->shake_time_remaining -= dt;
  if(camera->shake_time_remaining < 0.0f){
    camera->shake_time_remaining = 0.0f;
  }

  float random_x = ((float)std::rand() / RAND_MAX) * 2.0f - 1.0f;
  float random_y = ((float)std::rand() / RAND_MAX) * 2.0f - 1.0f;
  float falloff = camera->shake_time_remaining > 0.0f ? 1.0f : 0.0f;
  camera->shake_x = random_x * camera->shake_magnitude * falloff;
  camera->shake_y = random_y * camera->shake_magnitude * falloff;
}
