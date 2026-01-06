#include "corelib_mainloop.h"
#include <stdint.h>
#include <SDL2/SDL.h>
#include "corelib_gfx.h"
#include "common.h"
#include "keyboard_layout.h"

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
#define BTN_A           (SDLK_x)
#define BTN_B1          (SDLK_y)
#define BTN_B2          (SDLK_z)
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
  // If a key is not recognized, return zero
  if (sym == BTN_UP) return keyUp;
  if (sym == BTN_DOWN) return keyDown;
  if (sym == BTN_LEFT) return keyLeft;
  if (sym == BTN_RIGHT) return keyRight;
  if (sym == BTN_A) return keyEdit;
  
  // Handle keyboard layout for B button mapping
  // This ensures proper B button mapping based on current keyboard layout setting
#if defined(DESKTOP_BUILD) || defined(PORTMASTER_BUILD)
  switch (appSettings.keyboardLayout) {
    case KEYBOARD_LAYOUT_QWERTY:
      // QWERTY: Use Z key for B button (BTN_B2 is SDLK_z)
      if (sym == BTN_B2) return keyOpt;
      break;
    case KEYBOARD_LAYOUT_QWERTZ:
      // QWERTZ: Use Y key for B button (BTN_B1 is SDLK_y)
      if (sym == BTN_B1) return keyOpt;
      break;
    case KEYBOARD_LAYOUT_AZERTY:
      // AZERTY: Both Y and Z should work for compatibility
      if (sym == BTN_B1 || sym == BTN_B2) return keyOpt;
      break;
    case KEYBOARD_LAYOUT_DVORAK:
      // DVORAK: Both Y and Z should work for compatibility
      if (sym == BTN_B1 || sym == BTN_B2) return keyOpt;
      break;
    default: // AUTO
      // AUTO mode: Map both Y and Z keys to keyOpt for maximum compatibility
      if (sym == BTN_B1 || sym == BTN_B2) return keyOpt;
      break;
  }
#else
  switch (appSettings.keyboardLayout) {
    case KEYBOARD_LAYOUT_QWERTY:
    case KEYBOARD_LAYOUT_QWERTZ:
    case KEYBOARD_LAYOUT_AZERTY:
    case KEYBOARD_LAYOUT_DVORAK:
      // RG35xx: Single B key maps to keyOpt
      if (sym == BTN_B) return keyOpt;
      break;
    default: // AUTO
      // RG35xx: Single B key maps to keyOpt
      if (sym == BTN_B) return keyOpt;
      break;
  }
#endif
  
  if (sym == BTN_START) return keyPlay;
  if (sym == BTN_SELECT || sym == BTN_R1 || sym == BTN_L1) return keyShift;
  if (sym == BTN_VOLUME_UP) return keyVolumeUp;
  if (sym == BTN_VOLUME_DOWN) return keyVolumeDown;
  
  return 0;
}

#ifdef GAMEPAD_SUPPORT
static int decodeGamepadButton(int button) {
  switch (button) {
    case SDL_CONTROLLER_BUTTON_DPAD_UP: return keyUp;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return keyDown;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return keyLeft;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return keyRight;
    case SDL_CONTROLLER_BUTTON_A: return keyEdit;  // Steam Deck: A button
    case SDL_CONTROLLER_BUTTON_B: return keyOpt;   // Steam Deck: B button
    case SDL_CONTROLLER_BUTTON_START: return keyPlay;  // Steam Deck: Menu button
    case SDL_CONTROLLER_BUTTON_BACK:  // Steam Deck: View button
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
          enum Key key = decodeKey(event.key.keysym.sym);
          if (key != -1 ) onEvent(event.type == SDL_KEYDOWN ? eventKeyDown : eventKeyUp, key, NULL);
        }
#ifdef GAMEPAD_SUPPORT
      } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
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
