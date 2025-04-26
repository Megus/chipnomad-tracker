#include <corelib_mainloop.h>
#include <stdint.h>
#include <SDL/SDL.h>

#include <corelib_gfx.h>

#define FPS 60

// Keyboard mapping for desktop and RG35xx
#if defined(__arm__) && defined(__linux__)
// RG35xx mapping
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
// Desktop mapping
#define BTN_UP          273
#define BTN_DOWN        274
#define BTN_LEFT        276
#define BTN_RIGHT       275
#define BTN_A           120
#define BTN_B           122
#define BTN_X           113
#define BTN_Y           0
#define BTN_L1          0
#define BTN_R1          0
#define BTN_L2          0
#define BTN_R2          0
#define BTN_SELECT      304
#define BTN_START       32
#define BTN_MENU        306
#define BTN_VOLUME_UP   61
#define BTN_VOLUME_DOWN 45
#define BTN_POWER       0
#define BTN_EXIT        0
#endif

static int decodeKey(int sym) {
  // If a key is not recognized, return zero
  if (sym == BTN_UP) return keyUp;
  if (sym == BTN_DOWN) return keyDown;
  if (sym == BTN_LEFT) return keyLeft;
  if (sym == BTN_RIGHT) return keyRight;
  if (sym == BTN_A) return keyEdit;
  if (sym == BTN_B) return keyOpt;
  if (sym == BTN_START) return keyPlay;
  if (sym == BTN_SELECT || sym == BTN_R1 || sym == BTN_L1) return keyShift;
  if (sym == BTN_VOLUME_UP) return keyVolumeUp;
  if (sym == BTN_VOLUME_DOWN) return keyVolumeDown;
  return 0;
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
        if (event.key.keysym.sym == BTN_MENU) {
          menu = event.type == SDL_KEYDOWN;
        } else {
          // Debug output for key codes
          /*if (event.type == SDL_KEYDOWN) {
            gfxPrintf(35, 0, "%d", event.key.keysym.sym);
          } else {
            gfxPrint(35, 0, "     ");
          }*/
          enum Key key = decodeKey(event.key.keysym.sym);
          if (key != -1) onEvent(event.type == SDL_KEYDOWN ? eventKeyDown : eventKeyUp, key, NULL);
        }
      }
    }
    onEvent(eventTick, 0, NULL);

    draw();
    gfxUpdateScreen();

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