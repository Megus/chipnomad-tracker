#include <corelib_mainloop.h>
#include <stdint.h>
#include <SDL/SDL.h>

#define FPS 60

// Keyboard mapping for desktop and RG35xx
#if defined(__arm__) && defined(__linux__)
#define BTN_UP          119
#define BTN_DOWN        115
#define BTN_LEFT        113
#define BTN_RIGHT       100
#define BTN_A           97
#define BTN_B           98
#define BTN_X           120
#define BTN_Y           121
#define BTN_L1          104
#define BTN_R1          108
#define BTN_L2          106
#define BTN_R2          107
#define BTN_SELECT      110
#define BTN_START       109
#define BTN_MENU        117
#define BTN_VOLUME_UP   114
#define BTN_VOLUME_DOWN 116
#define BTN_POWER       0
#define BTN_EXIT        0
#else
#define BTN_UP          82
#define BTN_DOWN        81
#define BTN_LEFT        80
#define BTN_RIGHT       79
#define BTN_A           100
#define BTN_B           120
#define BTN_X           119
#define BTN_Y           97
#define BTN_L1          49
#define BTN_R1          48
#define BTN_L2          50
#define BTN_R2          57
#define BTN_SELECT      32
#define BTN_START       13
#define BTN_MENU        101
#define BTN_VOLUME_UP   61
#define BTN_VOLUME_DOWN 45
#define BTN_POWER       27
#define BTN_EXIT        113
#endif

extern SDL_Surface* sdlScreen;

static int decodeKey(int sym) {
  switch (sym) {
    case BTN_UP: return keyUp;
    case BTN_DOWN: return keyDown;
    case BTN_LEFT: return keyLeft;
    case BTN_RIGHT: return keyRight;
    case BTN_A: return keyA;
    case BTN_B: return keyB;
    case BTN_START: return keyStart;
    case BTN_SELECT: return keySelect;
    case BTN_VOLUME_UP: return keyVolumeUp;
    case BTN_VOLUME_DOWN: return keyVolumeDown;
    default: return -1;
  }
}

void mainLoopRun(void (*draw)(void), void (*onEvent)(enum MainLoopEvent event, int value, void* userdata)) {
  uint32_t delay = 1000 / FPS;
  uint32_t start;
  uint32_t busytime = 0;
  SDL_Event event;
  int menu = 0;

  while (1) {
    start = SDL_GetTicks();

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && (
          (event.key.keysym.sym == BTN_POWER) ||
          (event.key.keysym.sym == BTN_EXIT) ||
          (menu && event.key.keysym.sym == BTN_X)))) {
        onEvent(eventExit, 0, NULL);
        return;
      } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        if (event.type == SDL_KEYDOWN) printf("Key: %d\n", event.key.keysym.sym); // Debug
        if (event.key.keysym.sym == BTN_MENU) {
          menu = event.type == SDL_KEYDOWN;
        } else {
          enum Key key = decodeKey(event.key.keysym.sym);
          if (key != -1) onEvent(event.type == SDL_KEYDOWN ? eventKeyDown : eventKeyUp, key, NULL);
        }
      }
    }

    draw();
    SDL_Flip(sdlScreen);

    busytime = SDL_GetTicks() - start;
    if (delay > busytime) {
      SDL_Delay(delay - busytime);
    }
  }
}

void mainLoopDelay(int ms) {
  SDL_Delay(ms);
}

void mainLoopQuit(void) {
  SDL_Quit();
}