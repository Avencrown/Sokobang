#include "rendering.h"
#include "SDL3/SDL_blendmode.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "button.h"
#include "camera.h"
#include "common.h"
#include "fontLibrary.h"
#include "spriteLibrary.h"
#include <cstddef>
#include <cstdint>


void RenderSprite_World(SpriteRenderInfo spriteRenderInfo, SDL_Renderer* renderer, const Camera* camera, float x, float y, float scale, float alpha, bool flipped){
  int frame = spriteRenderInfo.frame;
  Sprite* sprite = spriteRenderInfo.sprite;
  SDL_FRect tilesetRect;
  if(GetSpriteCount(sprite) > 1){
    int width = sprite->width / sprite->sprite_count_x;
    int height = sprite->height / sprite->sprite_count_y;
    tilesetRect.w = width;
    tilesetRect.h = height;
    Expand1DTo2D(frame, sprite->sprite_count_x, &tilesetRect.x, &tilesetRect.y);
    tilesetRect.x *= width;
    tilesetRect.y *= height;
  }
  else{
    tilesetRect.w = sprite->width;
    tilesetRect.h = sprite->height;
    tilesetRect.x = 0;
    tilesetRect.y = 0; 
  }
  SDL_FRect rect;
  rect.x = x;
  rect.y = y;
  float final_scale = UPSCALE_FACTOR * scale; 
  rect.w = tilesetRect.w * final_scale;
  rect.h = tilesetRect.h * final_scale;
  rect.x -= sprite->pivot_x * final_scale;
  rect.y -= sprite->pivot_y * final_scale;

  if(camera != NULL){
  rect.x -= camera->camera_x + camera->shake_x;
  rect.y -= camera->camera_y + camera->shake_y;
  }
  
  SDL_SetTextureScaleMode(sprite->texture, SDL_SCALEMODE_PIXELART);
  SDL_SetTextureAlphaModFloat(sprite->texture, alpha);
  SDL_FlipMode flip = (flipped || spriteRenderInfo.flipped_x) ? SDL_FlipMode::SDL_FLIP_HORIZONTAL : SDL_FlipMode::SDL_FLIP_NONE;
  SDL_RenderTextureRotated(renderer, sprite->texture, &tilesetRect, &rect, 0, 0, flip);
}

void RenderTile(Sprite* tileset, int cell_id, LevelData* level, SDL_Renderer* renderer, const Camera* camera, float x, float y, float scale, float alpha){
  camera::GridToWorld(&x, &y, level);
  RenderSprite_World({cell_id, tileset} ,renderer, camera, x, y, scale, alpha, false);
}

void RenderSprite_OnTile(SpriteRenderInfo spriteInfo, LevelData* level, SDL_Renderer* renderer, const Camera* camera, float x, float y, float scale, float alpha, bool flipped){
  camera::GridToWorld(&x, &y, level);
  x += TILE_SIZE_PX_SCALED / 2.0;
  y += TILE_SIZE_PX_SCALED / 2.0;
  RenderSprite_World(spriteInfo,renderer, camera, x, y, scale, alpha, flipped);
}

void RenderButton(Button* button, bool is_selected, SDL_Renderer* renderer){
  SDL_Texture* texture =button->sprite->texture;
  SDL_FRect rect = button->rect;
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);
  uint8_t colorOverlay = is_selected ? 255: 230;
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureColorMod(texture, colorOverlay, colorOverlay, colorOverlay);
  SDL_RenderTexture(renderer, button->sprite->texture, NULL, &rect);
  if(!IsStringEmpty(button->text)){
    float glyph_height = button->font->glyphs['H'].atlasPosition.h / 2.0;
    RenderText(button->font, button->text, renderer, nullptr, rect.x + (rect.w / 2.0), rect.y + (rect.h / 2.0) - glyph_height, Alignment::Centered);
  }
}

void RenderButton_Primitive(Button* button, bool is_selected, SDL_Renderer* renderer){
  SDL_FRect rect = button->rect;
  Uint8 fill = is_selected ? 40 : 20;
  Uint8 border_r = is_selected ? 255 : 140;
  Uint8 border_g = is_selected ? 60  : 40;
  Uint8 border_b = is_selected ? 220 : 160;

  SDL_SetRenderDrawColor(renderer, fill, fill, fill + 10, 255);
  SDL_RenderFillRect(renderer, &rect);

  SDL_SetRenderDrawColor(renderer, border_r, border_g, border_b, 255);
  SDL_FRect border = rect;
  for (int i = 0; i < 3; i++) {
    SDL_RenderRect(renderer, &border);
    border.x -= 1; border.y -= 1; border.w += 2; border.h += 2;
  }

  if(!IsStringEmpty(button->text)){
    float glyph_height = button->font->glyphs['H'].atlasPosition.h / 2.0;
    RenderText(button->font, button->text, renderer, nullptr, rect.x + (rect.w / 2.0), rect.y + (rect.h / 2.0) - glyph_height, Alignment::Centered);
  }
}

void RenderButton_Dynamic(Button* button, bool is_selected, SDL_Renderer* renderer){
  assert(button->sprite->sprite_count_x == 3);
  assert(button->sprite->sprite_count_y == 3);
  
  uint8_t colorOverlay = is_selected ? 255: 230;
  SDL_Texture* texture = button->sprite->texture;
  SDL_FRect rect = button->rect;

  float part_w = texture->w / 3.0;
  float part_h = texture->h / 3.0;
  float vertical_center_height  = rect.h - (part_h * 2);
  float horizontal_center_width = rect.w - (part_w * 2);

  float right_x  = rect.x + rect.w - part_w;
  float bottom_y = rect.y + rect.h - part_h;
  float center_y = rect.y + part_h;
  float center_x = rect.x + part_w;

  SDL_FRect topLeftdst       = {rect.x,    rect.y,   part_w,                   part_h                 };
  SDL_FRect topRightdst      = {right_x,   rect.y,   part_w,                   part_h                 };
  SDL_FRect topCenterdst     = {center_x,  rect.y,   horizontal_center_width,  part_h                 };
  SDL_FRect bottomLeftdst    = {rect.x,    bottom_y, part_w,                   part_h                 };
  SDL_FRect bottomRightdst   = {right_x,   bottom_y, part_w,                   part_h                 };
  SDL_FRect bottomCenterdst  = {center_x,  bottom_y, horizontal_center_width,  part_h                 };
  SDL_FRect centerLeftdst    = {rect.x,    center_y, part_w,                   vertical_center_height };
  SDL_FRect centerRightdst   = {right_x,   center_y, part_w,                   vertical_center_height };
  SDL_FRect centerdst        = {center_x,  center_y, horizontal_center_width,  vertical_center_height };

  SDL_FRect topLeftsrc      = {0,          0,          part_w, part_h};
  SDL_FRect topRightsrc     = {part_w * 2, 0,          part_w, part_h};
  SDL_FRect topCentersrc    = {part_w * 1, 0,          part_w, part_h};
  SDL_FRect bottomLeftsrc   = {0,          part_h * 2, part_w, part_h};
  SDL_FRect bottomRightsrc  = {part_w * 2, part_h * 2, part_w, part_h};
  SDL_FRect bottomCentersrc = {part_w * 1, part_h * 2, part_w, part_h};
  SDL_FRect centerLeftsrc   = {0,          part_h * 1, part_w, part_h};
  SDL_FRect centerRightsrc  = {part_w * 2, part_h * 1, part_w, part_h};
  SDL_FRect centersrc       = {part_w * 1, part_h * 1, part_w, part_h};

  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  SDL_SetTextureColorMod(texture, colorOverlay, colorOverlay, colorOverlay);

  SDL_RenderTexture(renderer, texture, &topLeftsrc,      &topLeftdst      );
  SDL_RenderTexture(renderer, texture, &topCentersrc,    &topCenterdst    );
  SDL_RenderTexture(renderer, texture, &bottomCentersrc, &bottomCenterdst );
  SDL_RenderTexture(renderer, texture, &centerLeftsrc,   &centerLeftdst   );
  SDL_RenderTexture(renderer, texture, &centerRightsrc,  &centerRightdst  );
  SDL_RenderTexture(renderer, texture, &bottomLeftsrc,   &bottomLeftdst   );
  SDL_RenderTexture(renderer, texture, &topRightsrc,     &topRightdst     );
  SDL_RenderTexture(renderer, texture, &bottomRightsrc,  &bottomRightdst  );
  SDL_RenderTexture(renderer, texture, &centersrc,       &centerdst       );

  if(!IsStringEmpty(button->text)){
    float glyph_height = button->font->glyphs['H'].atlasPosition.h / 2.0;
    RenderText(button->font, button->text, renderer, nullptr, rect.x + (rect.w / 2.0), rect.y + (rect.h / 2.0) - glyph_height, Alignment::Centered);
  }
}

void RenderText(FontAtlas* atlas, const char* text, SDL_Renderer* renderer, Camera* camera, const float x, const float y, Alignment mode) {
  assert(atlas->atlasTexture != nullptr);

  float line_height = atlas->glyphs['H'].atlasPosition.h;
  float origin_x = x;
  float origin_y = y;

  if (camera != nullptr) {
    origin_x -= camera->camera_x;
    origin_y -= camera->camera_y;
  }

  int line_start = 0;
  float current_y = origin_y;

  while (true) {
    int line_end = line_start;
    float line_width = 0.0f;

    while (text[line_end] != '\n' && text[line_end] != STOP_CHAR && text[line_end] != '\0') {
      line_width += atlas->glyphs[text[line_end]].atlasPosition.w;
      line_end++;
    }

    float current_x = origin_x;
    if (mode == Alignment::Centered) {
      current_x -= line_width / 2.0f;
    }

    for (int i = line_start; i < line_end; i++) {
      Glyph glyph = atlas->glyphs[text[i]];
      SDL_FRect renderRectangle = {current_x, current_y, glyph.atlasPosition.w, glyph.atlasPosition.h};
      SDL_RenderTexture(renderer, atlas->atlasTexture, &glyph.atlasPosition, &renderRectangle);
      current_x += glyph.atlasPosition.w;
    }

    if (text[line_end] == STOP_CHAR || text[line_end] == '\0') {
      break;
    }

    current_y += line_height;
    line_start = line_end + 1;
  }
}

void RenderTextAnimated(FontAtlas* atlas, const char* text, SDL_Renderer* renderer, Camera* camera, float x, float y, Alignment mode, float base_scale, AudioSystem* audio){
  assert(atlas->atlasTexture != nullptr);

  int text_len = 0;
  while (text[text_len] != STOP_CHAR && text[text_len] != '\0'){
    text_len++;
  }

  float total_width = 0.0f;
  for (int i = 0; i < text_len; i++){
    total_width += atlas->glyphs[text[i]].atlasPosition.w * base_scale;
  }

  float draw_x = x;
  float draw_y = y;

  if (camera != nullptr){
    draw_x -= camera->camera_x;
    draw_y -= camera->camera_y;
  }

  if (mode == Alignment::Centered){
    draw_x -= total_width / 2.0f;
  }

  float beat_duration_sec = 60.0f / audio->beat_clock.bpm;
  unsigned int current_beat = audio->beat_clock.beat_index;
  float beat_progress = audio->beat_clock.beat_progress_01; 
  float time_in_beats = (float)current_beat + beat_progress;
  float current_glyph_x = draw_x;

  for (int i = 0; i < text_len; i++){
    Glyph glyph = atlas->glyphs[text[i]];
    float glyph_w = glyph.atlasPosition.w * base_scale;
    float glyph_h = glyph.atlasPosition.h * base_scale;
    float phase = fmodf(time_in_beats - (float)i, (float)text_len);

    if (phase < 0.0f){
      phase += (float)text_len;
    }

    float pulse = 0.0f;
    float pulse_duration = 1.5f;
    
    if (phase < pulse_duration && IsDrumsAudible(audio)){
      float t = phase / pulse_duration;
      float decay = (1.0f - t);
      pulse = decay * decay;
    }

    float max_extra_scale = 0.45f;
    float letter_scale = 1.0f + (pulse * max_extra_scale);
    float scaled_w = glyph_w * letter_scale;
    float scaled_h = glyph_h * letter_scale;
    float offset_x = (scaled_w - glyph_w) * 0.5f;
    float offset_y = (scaled_h - glyph_h) * 0.5f;
    SDL_FRect renderRectangle = {current_glyph_x - offset_x, draw_y - offset_y, scaled_w,scaled_h};
    SDL_RenderTexture(renderer, atlas->atlasTexture, &glyph.atlasPosition, &renderRectangle);
    current_glyph_x += glyph_w;
  }
}