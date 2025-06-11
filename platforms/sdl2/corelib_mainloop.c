#include <corelib_mainloop.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <corelib_gfx.h>

#define FPS 60

// Keyboard mapping for desktop and RG35xx
#if defined(DESKTOP_BUILD)
// Desktop mapping
#define BTN_UP          (SDLK_UP)
#define BTN_DOWN        (SDLK_DOWN)
#define BTN_LEFT        (SDLK_LEFT)
#define BTN_RIGHT       (SDLK_RIGHT)
#define BTN_A           (SDLK_x)
#define BTN_B           (SDLK_z)
#define BTN_X           (SDLK_s)
#define BTN_Y           (SDLK_a)
#define BTN_L1          0
#define BTN_R1          0
#define BTN_L2          0
#define BTN_R2          0
#define BTN_SELECT      (SDLK_LSHIFT)
#define BTN_START       (SDLK_SPACE)
#define BTN_MENU        (SDLK_LCTRL)
#define BTN_VOLUME_UP   (SDLK_PLUS)
#define BTN_VOLUME_DOWN (SDLK_MINUS)
#define BTN_POWER       0
#define BTN_EXIT        0
#elif defined(RG35XX_PLUS_BUILD)
// RG35xx+ mapping
#define BTN_UP          (0x0100 + SDL_HAT_UP)
#define BTN_DOWN        (0x0100 + SDL_HAT_DOWN)
#define BTN_LEFT        (0x0100 + SDL_HAT_LEFT)
#define BTN_RIGHT       (0x0100 + SDL_HAT_RIGHT)
#define BTN_A           0
#define BTN_B           1
#define BTN_X           3
#define BTN_Y           2
#define BTN_L1          4
#define BTN_R1          5
#define BTN_L2          9
#define BTN_R2          10
#define BTN_SELECT      6
#define BTN_START       7
#define BTN_MENU        8
#define BTN_VOLUME_UP   14
#define BTN_VOLUME_DOWN 13
#define BTN_POWER       1073741926
#define BTN_EXIT        (BTN_POWER)
#else
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

#if defined(RG35XX_PLUS_BUILD)
void translateJoystickEvent(SDL_Event *event) {
  static uint8_t lastHatState = SDL_HAT_CENTERED;

  switch(event->type) {
    case SDL_JOYHATMOTION:
      if(event->jhat.hat == 0) {
        if (event->jhat.value == SDL_HAT_CENTERED) {
          event->type = SDL_KEYUP;
        } else {
          event->type = SDL_KEYDOWN;
          lastHatState = event->jhat.value;
        }
        event->key.keysym.sym = 0x0100 + lastHatState;
      }
      break;

    case SDL_JOYBUTTONUP:
      event->type = SDL_KEYUP;
      event->key.keysym.sym = event->jbutton.button;
      break;

    case SDL_JOYBUTTONDOWN:
      event->type = SDL_KEYDOWN;
      event->key.keysym.sym = event->jbutton.button;
      break;
  }
}
#endif

void mainLoopRun(void (*draw)(void), void (*onEvent)(enum MainLoopEvent event, int value, void* userdata)) {
  uint32_t delay = 1000 / FPS;
  uint32_t start;
  uint32_t busytime = 0;
  SDL_Event event;
  int menu = 0;

#if defined(RG35XX_PLUS_BUILD)
  (void)!SDL_JoystickOpen(0);
#endif

  while (1) {
    start = SDL_GetTicks();

    while (SDL_PollEvent(&event)) {
#if defined(RG35XX_PLUS_BUILD)
      translateJoystickEvent(&event);
#endif
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
            gfxPrintf(30, 0, "%d", event.key.keysym.sym);
          } else {
            gfxPrint(30, 0, "          ");
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
