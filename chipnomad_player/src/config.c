#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static SDL_Color parseColor(const char* str) {
    if (str && str[0] == '0' && str[1] == 'x') {
        unsigned int hex = strtoul(str, NULL, 16);
        return (SDL_Color){(hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF, 255};
    }
    return (SDL_Color){255, 255, 255, 255};
}

void configInit(struct VisualizerConfig* config) {
    config->windowWidth = 1920;
    config->windowHeight = 1080;
    config->fontSize = 32;
    strcpy(config->fontPath, "fonts/Fragment_Mono/FragmentMono-Regular.ttf");
    config->backgroundColor = (SDL_Color){0, 0, 0, 255};
    config->currentRowColor = (SDL_Color){255, 255, 0, 255};
    config->noteColor = (SDL_Color){0, 255, 0, 255};
    config->instrumentColor = (SDL_Color){0, 255, 255, 255};
    config->volumeColor = (SDL_Color){255, 0, 255, 255};
    config->fxColor = (SDL_Color){255, 255, 255, 255};
    config->dimmedColor = (SDL_Color){64, 64, 64, 255};
}

void configLoad(struct VisualizerConfig* config, const char* filename) {
    configInit(config);
    
    FILE* file = fopen(filename, "r");
    if (!file) return;
    
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        char* colon = strchr(line, ':');
        if (!colon) continue;
        
        *colon = '\0';
        char* key = line;
        char* value = colon + 1;
        
        // Trim whitespace
        while (*value == ' ' || *value == '\t') value++;
        char* end = value + strlen(value) - 1;
        while (end > value && (*end == '\n' || *end == '\r' || *end == ' ')) *end-- = '\0';
        
        if (strcmp(key, "windowWidth") == 0) {
            config->windowWidth = atoi(value);
        } else if (strcmp(key, "windowHeight") == 0) {
            config->windowHeight = atoi(value);
        } else if (strcmp(key, "fontSize") == 0) {
            config->fontSize = atoi(value);
        } else if (strcmp(key, "fontPath") == 0) {
            strncpy(config->fontPath, value, sizeof(config->fontPath) - 1);
        } else if (strcmp(key, "backgroundColor") == 0) {
            config->backgroundColor = parseColor(value);
        } else if (strcmp(key, "currentRowColor") == 0) {
            config->currentRowColor = parseColor(value);
        } else if (strcmp(key, "noteColor") == 0) {
            config->noteColor = parseColor(value);
        } else if (strcmp(key, "instrumentColor") == 0) {
            config->instrumentColor = parseColor(value);
        } else if (strcmp(key, "volumeColor") == 0) {
            config->volumeColor = parseColor(value);
        } else if (strcmp(key, "fxColor") == 0) {
            config->fxColor = parseColor(value);
        } else if (strcmp(key, "dimmedColor") == 0) {
            config->dimmedColor = parseColor(value);
        }
    }
    
    fclose(file);
}