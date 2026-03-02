// Android-specific mainloop implementation with touch support
#include "corelib_mainloop.h"
#include <stdint.h>
#include <SDL2/SDL.h>
#include "corelib_gfx.h"
#include "corelib_input.h"
#include "asset_bundling.h"
#include "common.h"
#include "../../../chipnomad_lib/corelib/corelib_file.h"

#define FPS 60

static SDL_GameController* gameController = NULL;

// Virtual gamepad state (shared with input and graphics)
extern int vpadEnabled;
extern SDL_Rect dpadRect, aButtonRect, bButtonRect, startButtonRect, selectButtonRect;
extern SDL_Rect dpadUpRect, dpadDownRect, dpadLeftRect, dpadRightRect;

// Screen dimensions (from graphics)
extern int screenW, screenH;

// Button definitions
typedef struct {
  SDL_Rect* rect;
  int key;
} Button;

static Button buttons[] = {
  {&dpadUpRect, keyUp},
  {&dpadDownRect, keyDown},
  {&dpadLeftRect, keyLeft},
  {&dpadRightRect, keyRight},
  {&aButtonRect, keyEdit},
  {&bButtonRect, keyOpt},
  {&startButtonRect, keyPlay},
  {&selectButtonRect, keyShift}
};

static int isPointInRect(int x, int y, SDL_Rect* rect) {
  return (x >= rect->x && x < rect->x + rect->w && y >= rect->y && y < rect->y + rect->h);
}

static int getTouchButton(int x, int y) {
  if (!vpadEnabled) return -1;

  for (int i = 0; i < 8; i++) {
    if (isPointInRect(x, y, buttons[i].rect)) {
      return i;
    }
  }
  return -1;
}

static int decodeGamepadButton(int button) {
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

static void initGamepad(void) {
  vpadEnabled = 1; // Enable touch controls by default
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (SDL_IsGameController(i)) {
      gameController = SDL_GameControllerOpen(i);
      if (gameController) {
        vpadEnabled = 0; // Hide touch controls when gamepad found
        break;
      }
    }
  }
}

void mainLoopRun(void (*draw)(void), void (*onEvent)(enum MainLoopEvent event, int value, void* userdata)) {
  uint32_t delay = 1000 / FPS;
  uint32_t start;
  uint32_t busytime = 0;
  SDL_Event event;
  int wakeRedrawFrames = 0;

  // Initialize asset system
  if (assetsInit() != 0) {
    // Continue anyway - app can still work without bundled assets
  }

  // Find and open first valid game controller (skip accelerometers)
  initGamepad();

  typedef struct {
    SDL_FingerID fingerId;
    int buttonIndex;
  } FingerButton;

  FingerButton activeFingers[10] = {0}; // Max 10 fingers
  int numActiveFingers = 0;

  // Declare external function for visual feedback
  extern void gfxSetButtonPressed(int buttonIndex, int pressed);

  while (1) {
    start = SDL_GetTicks();

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        onEvent(eventExit, 0, NULL);
        if (gameController) {
          SDL_GameControllerClose(gameController);
          gameController = NULL;
        }
        return;
      } else if (event.type == SDL_APP_TERMINATING) {
        if (gameController) {
          SDL_GameControllerClose(gameController);
          gameController = NULL;
        }
      } else if (event.type == SDL_APP_WILLENTERBACKGROUND) {
        onEvent(eventSleep, 0, NULL);
      } else if (event.type == SDL_APP_DIDENTERFOREGROUND) {
        onEvent(eventWake, 0, NULL);
        wakeRedrawFrames = FPS; // Redraw for 1 second
        // Health check: verify controller is still connected
        if (gameController && !SDL_GameControllerGetAttached(gameController)) {
          SDL_GameControllerClose(gameController);
          gameController = NULL;
        }
        // Check for gamepad on wake (user may have connected controller)
        if (!gameController) {
          initGamepad();
        }
      } else if (event.type == SDL_RENDER_TARGETS_RESET || event.type == SDL_RENDER_DEVICE_RESET) {
        wakeRedrawFrames = FPS;
      } else if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESTORED ||
            event.window.event == SDL_WINDOWEVENT_EXPOSED ||
            event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
          wakeRedrawFrames = FPS;
        }
      } else if (event.type == SDL_FINGERDOWN) {
        int x, y;
        SDL_GetWindowSize(SDL_GetWindowFromID(event.tfinger.windowID), &x, &y);
        x = (int)(event.tfinger.x * x);
        y = (int)(event.tfinger.y * y);

        int buttonIndex = getTouchButton(x, y);
        if (buttonIndex >= 0 && numActiveFingers < 10) {
          activeFingers[numActiveFingers].fingerId = event.tfinger.fingerId;
          activeFingers[numActiveFingers].buttonIndex = buttonIndex;
          numActiveFingers++;
          gfxSetButtonPressed(buttonIndex, 1);
          onEvent(eventKeyDown, buttons[buttonIndex].key, NULL);
        }
      } else if (event.type == SDL_FINGERUP) {
        // Find and remove this finger
        for (int i = 0; i < numActiveFingers; i++) {
          if (activeFingers[i].fingerId == event.tfinger.fingerId) {
            gfxSetButtonPressed(activeFingers[i].buttonIndex, 0);
            onEvent(eventKeyUp, buttons[activeFingers[i].buttonIndex].key, NULL);
            // Remove finger from array
            for (int j = i; j < numActiveFingers - 1; j++) {
              activeFingers[j] = activeFingers[j + 1];
            }
            numActiveFingers--;
            break;
          }
        }
      }
      else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
        if (inputRawCallback) {
          inputRawCallback((InputCode){inputGamepad, event.cbutton.button}, event.type == SDL_CONTROLLERBUTTONDOWN);
        }

        enum Key key = decodeGamepadButton(event.cbutton.button);
        if (key != 0) {
          onEvent(event.type == SDL_CONTROLLERBUTTONDOWN ? eventKeyDown : eventKeyUp, key, NULL);
        }
      }
      else if (event.type == SDL_CONTROLLERDEVICEADDED) {
        if (!gameController) {
          initGamepad();
        }
      }
      else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        if (gameController && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gameController))) {
          SDL_GameControllerClose(gameController);
          gameController = NULL;
          vpadEnabled = 1; // Show touch controls
        }
      }
    }

    // Trigger full redraw for multiple frames after app returns to foreground
    if (wakeRedrawFrames > 0) {
      onEvent(eventFullRedraw, 0, NULL);
      wakeRedrawFrames--;
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