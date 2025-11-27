#ifndef VISUALS_H
#define VISUALS_H

#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <project.h>
#include <playback.h>
#include "config.h"

struct VisualState {
    SDL_Renderer* renderer;
    struct Project* project;
    struct PlaybackState* playback;
    int* isPlaying;
    FT_Library ftLibrary;
    FT_Face ftFace;
    struct VisualizerConfig* config;
};

int visualsInit(struct VisualState* visualState);
void visualsRender(struct VisualState* visualState);
void visualsCleanup(struct VisualState* visualState);

#endif