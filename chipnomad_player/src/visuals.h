#ifndef VISUALS_H
#define VISUALS_H

#include <SDL2/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <project.h>
#include <playback.h>

struct VisualizerConfig {
    int windowWidth;
    int windowHeight;
    int fontSize;
    const char* fontPath;
    SDL_Color backgroundColor;
    SDL_Color currentRowColor;
    SDL_Color noteColor;
    SDL_Color instrumentColor;
    SDL_Color volumeColor;
    SDL_Color fxColor;
    SDL_Color dimmedColor;
};

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