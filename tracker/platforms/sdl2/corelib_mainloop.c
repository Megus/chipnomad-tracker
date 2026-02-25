#include "corelib_mainloop.h"
#include <stdint.h>
#include <SDL2/SDL.h>
#include "corelib_gfx.h"
#include "corelib_input.h"
#include "common.h"

#define FPS 60

// Enable gamepad support for desktop builds
#ifdef DESKTOP_BUILD
#define GAMEPAD_SUPPORT
#endif

#ifdef GAMEPAD_SUPPORT
// Gamepad support
static SDL_GameController* gameController = NULL;
#endif

// Keyboard mapping
#if defined(DESKTOP_BUILD) || defined(PORTMASTER_BUILD)
// Desktop and PortMaster mapping
#define BTN_UP          (SDLK_UP)
#define BTN_DOWN        (SDLK_DOWN)
#define BTN_LEFT        (SDLK_LEFT)
#define BTN_RIGHT       (SDLK_RIGHT)
#define BTN_A           (SDLK_z)
#define BTN_B           (SDLK_x)
#define BTN_X           (SDLK_c)
#define BTN_Y           (SDLK_a)
#define BTN_L1          (SDLK_LSHIFT)
#define BTN_R1          (SDLK_LSHIFT)
#define BTN_L2          0
#define BTN_R2          0
#define BTN_SELECT      (SDLK_LSHIFT)
#define BTN_START       (SDLK_SPACE)
#define BTN_MENU        (SDLK_LCTRL)
#define BTN_VOLUME_UP   0
#define BTN_VOLUME_DOWN 0
#define BTN_POWER       0
#define BTN_EXIT        0
#else
// RG35xx (old) mapping
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
  // Check custom key mappings first
  if (sym != 0) {
    for (int i = 0; i < 3; i++) {
      if (sym == appSettings.keyMapping.keyUp[i]) return keyUp;
      if (sym == appSettings.keyMapping.keyDown[i]) return keyDown;
      if (sym == appSettings.keyMapping.keyLeft[i]) return keyLeft;
      if (sym == appSettings.keyMapping.keyRight[i]) return keyRight;
      if (sym == appSettings.keyMapping.keyEdit[i]) return keyEdit;
      if (sym == appSettings.keyMapping.keyOpt[i]) return keyOpt;
      if (sym == appSettings.keyMapping.keyPlay[i]) return keyPlay;
      if (sym == appSettings.keyMapping.keyShift[i]) return keyShift;
    }
  }

  // Fallback to hardcoded mappings for unmapped keys
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

#ifdef GAMEPAD_SUPPORT
static int decodeGamepadButton(int button) {
  // Convert button to negative value for key mapping lookup
  int mappedButton = -button;

  // Check custom key mappings first
  for (int i = 0; i < 3; i++) {
    if (mappedButton == appSettings.keyMapping.keyUp[i]) return keyUp;
    if (mappedButton == appSettings.keyMapping.keyDown[i]) return keyDown;
    if (mappedButton == appSettings.keyMapping.keyLeft[i]) return keyLeft;
    if (mappedButton == appSettings.keyMapping.keyRight[i]) return keyRight;
    if (mappedButton == appSettings.keyMapping.keyEdit[i]) return keyEdit;
    if (mappedButton == appSettings.keyMapping.keyOpt[i]) return keyOpt;
    if (mappedButton == appSettings.keyMapping.keyPlay[i]) return keyPlay;
    if (mappedButton == appSettings.keyMapping.keyShift[i]) return keyShift;
  }

  // Fallback to default gamepad mappings
  switch (button) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return keyUp;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return keyDown;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return keyLeft;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return keyRight;
    case SDL_CONTROLLER_BUTTON_A: return keyEdit;
    case SDL_CONTROLLER_BUTTON_B: return keyOpt;
    case SDL_CONTROLLER_BUTTON_START: return keyPlay;
    case SDL_CONTROLLER_BUTTON_BACK:
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return keyShift;
    default: return 0;
  }
}
#endif

void mainLoopRun(void (*draw)(void), void (*onEvent)(enum MainLoopEvent event, int value, void* userdata)) {
  uint32_t delay = 1000 / FPS;
  uint32_t start;
  uint32_t busytime = 0;
  SDL_Event event;
  int menu = 0;

#ifdef GAMEPAD_SUPPORT
  // Initialize gamepad support
  if (SDL_NumJoysticks() > 0) {
    gameController = SDL_GameControllerOpen(0);
  }
#endif

  while (1) {
    start = SDL_GetTicks();

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && (
        (event.key.keysym.sym == BTN_POWER) ||
        (event.key.keysym.sym == BTN_EXIT) ||
        (menu && event.key.keysym.sym == BTN_X)))) {
        onEvent(eventExit, 0, NULL);
#ifdef GAMEPAD_SUPPORT
        if (gameController) {
          SDL_GameControllerClose(gameController);
          gameController = NULL;
        }
#endif
        return;
      } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        if (event.key.keysym.sym == BTN_MENU) {
          menu = event.type == SDL_KEYDOWN;
        } else {
          // Raw input callback for key mapping screen
          if (inputRawCallback) {
            inputRawCallback(event.key.keysym.sym, event.type == SDL_KEYDOWN);
          }

          enum Key key = decodeKey(event.key.keysym.sym);
          if (key != -1 ) onEvent(event.type == SDL_KEYDOWN ? eventKeyDown : eventKeyUp, key, NULL);
        }
#ifdef GAMEPAD_SUPPORT
      } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
        // Raw input callback for key mapping screen (negative for controller)
        if (inputRawCallback) {
          inputRawCallback(-event.cbutton.button, event.type == SDL_CONTROLLERBUTTONDOWN);
        }

        enum Key key = decodeGamepadButton(event.cbutton.button);
        if (key != 0) {
          onEvent(event.type == SDL_CONTROLLERBUTTONDOWN ? eventKeyDown : eventKeyUp, key, NULL);
        }
#endif
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

void mainLoopTriggerQuit(void) {
  SDL_Event quitEvent;
  quitEvent.type = SDL_QUIT;
  SDL_PushEvent(&quitEvent);
}
