#ifndef CONFIG_H
#define CONFIG_H

#include <SDL2/SDL.h>

struct VisualizerConfig {
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
};

void configInit(struct VisualizerConfig* config);
void configLoad(struct VisualizerConfig* config, const char* filename);

#endif