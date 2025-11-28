#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

// ChipNomad library includes
#include <chipnomad_lib.h>

// Local modules
#include "audio.h"
#include "visuals.h"
#include "config.h"

#define SAMPLE_RATE 44100

struct PlayerState {
  SDL_Window* window;
  SDL_Renderer* renderer;
  ChipNomadState* chipnomadState;
  int isPlaying;
  int quit;
  AudioState audioState;
  VisualState visualState;
  VisualizerConfig visualConfig;
};

static struct PlayerState player;

int loadTrack(const char* filename) {
  // Create ChipNomad state
  player.chipnomadState = chipnomadCreate();
  if (!player.chipnomadState) {
    fprintf(stderr, "Failed to create ChipNomad state\n");
    return -1;
  }

  // Load project
  if (projectLoad(&player.chipnomadState->project, filename) != 0) {
    fprintf(stderr, "Failed to load track: %s\n", filename);
    chipnomadDestroy(player.chipnomadState);
    return -1;
  }

  // Initialize playback with the loaded project
  playbackInit(&player.chipnomadState->playbackState, &player.chipnomadState->project);

  // Initialize chips
  chipnomadInitChips(player.chipnomadState, SAMPLE_RATE, NULL);

  return 0;
}

void startPlayback() {
  audioStart(&player.audioState);
}

void render() {
  visualsRender(&player.visualState);
}

void handleEvents() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
    case SDL_QUIT:
      player.quit = 1;
      break;
    case SDL_KEYDOWN:
      switch (e.key.keysym.sym) {
      case SDLK_ESCAPE:
        player.quit = 1;
        break;
      default:
        if (!player.isPlaying) {
          startPlayback();
        }
        break;
      }
      break;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <track.cnm>\n", argv[0]);
    return 1;
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
    return 1;
  }

  // Load visualizer config from file
  configLoad(&player.visualConfig, "player_settings.txt");

  // Check display DPI to determine window size
  float ddpi;
  int displayIndex = 0;
  int windowWidth = player.visualConfig.windowWidth;
  int windowHeight = player.visualConfig.windowHeight;

  if (SDL_GetDisplayDPI(displayIndex, &ddpi, NULL, NULL) == 0) {
    // If DPI > 150, assume high-DPI display and create smaller window
    if (ddpi > 150.0f) {
      windowWidth /= 2;
      windowHeight /= 2;
    }
  }

  player.window = SDL_CreateWindow("ChipNomad Player",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    windowWidth, windowHeight,
    SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!player.window) {
      fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
      SDL_Quit();
      return 1;
    }

    player.renderer = SDL_CreateRenderer(player.window, -1, SDL_RENDERER_ACCELERATED);
    if (!player.renderer) {
      fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
      SDL_DestroyWindow(player.window);
      SDL_Quit();
      return 1;
    }

    // Always set logical size to ensure exact pixel rendering
    SDL_RenderSetLogicalSize(player.renderer, player.visualConfig.windowWidth, player.visualConfig.windowHeight);

    if (loadTrack(argv[1]) != 0) {
      SDL_DestroyRenderer(player.renderer);
      SDL_DestroyWindow(player.window);
      SDL_Quit();
      return 1;
    }

    // Initialize audio module
    player.audioState.chipnomadState = player.chipnomadState;
    player.audioState.isPlaying = &player.isPlaying;

    if (audioInit(&player.audioState) != 0) {
      SDL_DestroyRenderer(player.renderer);
      SDL_DestroyWindow(player.window);
      SDL_Quit();
      return 1;
    }

    // Initialize visual module
    player.visualState.renderer = player.renderer;
    player.visualState.project = &player.chipnomadState->project;
    player.visualState.playback = &player.chipnomadState->playbackState;
    player.visualState.isPlaying = &player.isPlaying;
    player.visualState.config = &player.visualConfig;

    if (visualsInit(&player.visualState) != 0) {
      SDL_DestroyRenderer(player.renderer);
      SDL_DestroyWindow(player.window);
      SDL_Quit();
      return 1;
    }

    while (!player.quit) {
      handleEvents();
      render();
      SDL_Delay(16); // ~60 FPS
    }

    visualsCleanup(&player.visualState);
    chipnomadDestroy(player.chipnomadState);
    SDL_DestroyRenderer(player.renderer);
    SDL_DestroyWindow(player.window);
    SDL_Quit();

    return 0;
  }