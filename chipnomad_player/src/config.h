#ifndef CONFIG_H
#define CONFIG_H

#include <SDL2/SDL.h>

typedef enum {
  PHRASE_RENDER_MODE_FULL,
  PHRASE_RENDER_MODE_MEDIUM,
  PHRASE_RENDER_MODE_COMPACT
} PhraseRenderMode;

typedef enum {
  PLAYER_MODE_PHRASE,
  PLAYER_MODE_SCROLL
} PlayerMode;

typedef struct VisualizerConfig {
  int windowWidth;
  int windowHeight;
  int fontSize;
  char fontPath[256];
  SDL_Color backgroundColor;
  SDL_Color currentRowColor;
  SDL_Color noteColor;
  SDL_Color instrumentColor;
  SDL_Color volumeColor;
  SDL_Color fxColor;
  SDL_Color dimmedColor;
  PhraseRenderMode phraseRenderMode;
  PlayerMode playerMode;
  int scrollRows;
} VisualizerConfig;

void configInit(VisualizerConfig* config);
void configLoad(VisualizerConfig* config, const char* filename);

#endif