#ifndef VISUALS_H
#define VISUALS_H

#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "project.h"
#include "playback.h"
#include "config.h"

#define MAX_HISTORY_SIZE 64

typedef struct ScrollPosition {
  int songRow;
  int chainRow;
  int phraseRow;
  int valid;
} ScrollPosition;

typedef struct TrackHistory {
  ScrollPosition positions[MAX_HISTORY_SIZE];
  int count;
  int writeIndex;
} TrackHistory;

typedef struct VisualState {
  SDL_Renderer* renderer;
  Project* project;
  PlaybackState* playback;
  int* isPlaying;
  FT_Library ftLibrary;
  FT_Face ftFace;
  VisualizerConfig* config;
  TrackHistory trackHistory[PROJECT_MAX_TRACKS];
} VisualState;

int visualsInit(VisualState* visualState);
void visualsRender(VisualState* visualState);
void visualsCleanup(VisualState* visualState);

#endif
