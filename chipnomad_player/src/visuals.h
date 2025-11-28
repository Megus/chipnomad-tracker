#ifndef VISUALS_H
#define VISUALS_H

#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <project.h>
#include <playback.h>
#include "config.h"

typedef struct VisualState {
  SDL_Renderer* renderer;
  Project* project;
  PlaybackState* playback;
  int* isPlaying;
  FT_Library ftLibrary;
  FT_Face ftFace;
  VisualizerConfig* config;
} VisualState;

int visualsInit(VisualState* visualState);
void visualsRender(VisualState* visualState);
void visualsCleanup(VisualState* visualState);

#endif