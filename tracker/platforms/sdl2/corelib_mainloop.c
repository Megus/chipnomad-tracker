#include "corelib_mainloop.h"
#include <stdint.h>
#include <SDL2/SDL.h>
#include "corelib_gfx.h"
#include "corelib_input.h"
#include "../../src/corelib/corelib_assets.h"

#define FPS 60

#ifdef GAMEPAD_SUPPORT
// Gamepad support
static SDL_GameController* gameController = NULL;

static void initGamepad(void) {
#ifdef TOUCH_INPUT
  extern int vpadEnabled;
  vpadEnabled = 1;
#endif
  for (int i = 0; i < SDL_NumJoysticks(); i++) {
    if (SDL_IsGameController(i)) {
      gameController = SDL_GameControllerOpen(i);
      if (gameController) {
#ifdef TOUCH_INPUT
        vpadEnabled = 0;
#endif
        break;
      }
    }
  }
}
#endif

#ifdef TOUCH_INPUT
// Virtual gamepad state
int vpadEnabled = 0;
SDL_Rect dpadRect, aButtonRect, bButtonRect, startButtonRect, selectButtonRect;
SDL_Rect dpadUpRect, dpadDownRect, dpadLeftRect, dpadRightRect;

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
#endif

// Keyboard mapping
#if defined(DESKTOP_BUILD) || defined(PORTMASTER_BUILD)
// Desktop and PortMaster mapping
#define BTN_MENU        (SDLK_LCTRL)
#define BTN_POWER       0
#define BTN_EXIT        0
#else
// RG35xx (old) mapping
#define BTN_MENU        117
#define BTN_POWER       0
#define BTN_EXIT        0
#endif

void mainLoopRun(void (*draw)(void), void (*onEvent)(MainLoopEventData eventData)) {
  uint32_t delay = 1000 / FPS;
  uint32_t start;
  uint32_t busytime = 0;
  SDL_Event event;
  int menu = 0;
  MainLoopEventData eventData;

#ifdef MOBILE_LIFECYCLE
  int wakeRedrawFrames = 0;
#endif

  // Initialize bundled assets
  assetsInit();

#ifdef TOUCH_INPUT
  typedef struct {
    SDL_FingerID fingerId;
    int buttonIndex;
  } FingerButton;

  FingerButton activeFingers[10] = {0};
  int numActiveFingers = 0;

  extern void gfxSetButtonPressed(int buttonIndex, int pressed);
#endif

#ifdef GAMEPAD_SUPPORT
  // Initialize gamepad support
  initGamepad();
#endif

  while (1) {
    start = SDL_GetTicks();

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && (
        (event.key.keysym.sym == BTN_POWER) ||
        (event.key.keysym.sym == BTN_EXIT) ||
        (menu && event.key.keysym.sym == SDLK_x)))) {
        eventData.type = eventExit;
        eventData.data.value = 0;
        onEvent(eventData);
#ifdef GAMEPAD_SUPPORT
        if (gameController) {
          SDL_GameControllerClose(gameController);
          gameController = NULL;
        }
#endif
        return;
      }
#ifdef MOBILE_LIFECYCLE
      else if (event.type == SDL_APP_TERMINATING) {
#ifdef GAMEPAD_SUPPORT
        if (gameController) {
          SDL_GameControllerClose(gameController);
          gameController = NULL;
        }
#endif
      }
      else if (event.type == SDL_APP_WILLENTERBACKGROUND) {
        eventData.type = eventSleep;
        eventData.data.value = 0;
        onEvent(eventData);
      }
      else if (event.type == SDL_APP_DIDENTERFOREGROUND) {
        eventData.type = eventWake;
        eventData.data.value = 0;
        onEvent(eventData);
        wakeRedrawFrames = FPS;
#ifdef GAMEPAD_SUPPORT
        if (gameController && !SDL_GameControllerGetAttached(gameController)) {
          SDL_GameControllerClose(gameController);
          gameController = NULL;
        }
        if (!gameController) {
          initGamepad();
        }
#endif
      }
      else if (event.type == SDL_RENDER_TARGETS_RESET || event.type == SDL_RENDER_DEVICE_RESET) {
        wakeRedrawFrames = FPS;
      }
      else if (event.type == SDL_WINDOWEVENT) {
        if (event.window.event == SDL_WINDOWEVENT_RESTORED ||
            event.window.event == SDL_WINDOWEVENT_EXPOSED ||
            event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
          wakeRedrawFrames = FPS;
        }
      }
#endif
      else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        if (event.key.keysym.sym == BTN_MENU) {
          menu = event.type == SDL_KEYDOWN;
        } else {
          eventData.type = event.type == SDL_KEYDOWN ? eventKeyDown : eventKeyUp;
          eventData.data.input = (InputCode){inputKeyboard, event.key.keysym.sym};
          onEvent(eventData);
        }
#ifdef GAMEPAD_SUPPORT
      } else if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
        eventData.type = event.type == SDL_CONTROLLERBUTTONDOWN ? eventKeyDown : eventKeyUp;
        eventData.data.input = (InputCode){inputGamepad, event.cbutton.button};
        onEvent(eventData);
      }
      else if (event.type == SDL_CONTROLLERDEVICEADDED) {
        if (!gameController) {
          for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
              gameController = SDL_GameControllerOpen(i);
              if (gameController) break;
            }
          }
        }
      }
      else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        if (gameController && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(gameController))) {
          SDL_GameControllerClose(gameController);
          gameController = NULL;
#ifdef TOUCH_INPUT
          vpadEnabled = 1;
#endif
        }
      }
#endif
#ifdef TOUCH_INPUT
      else if (event.type == SDL_FINGERDOWN) {
        int wx, wy;
        SDL_GetWindowSize(SDL_GetWindowFromID(event.tfinger.windowID), &wx, &wy);
        int dx, dy;
        SDL_GL_GetDrawableSize(SDL_GetWindowFromID(event.tfinger.windowID), &dx, &dy);
        int x = (int)(event.tfinger.x * dx);
        int y = (int)(event.tfinger.y * dy);

        int buttonIndex = getTouchButton(x, y);
        if (buttonIndex >= 0 && numActiveFingers < 10) {
          activeFingers[numActiveFingers].fingerId = event.tfinger.fingerId;
          activeFingers[numActiveFingers].buttonIndex = buttonIndex;
          numActiveFingers++;
          gfxSetButtonPressed(buttonIndex, 1);
          eventData.type = eventKeyDown;
          eventData.data.input = (InputCode){inputLogical, buttons[buttonIndex].key};
          onEvent(eventData);
        }
      }
      else if (event.type == SDL_FINGERUP) {
        for (int i = 0; i < numActiveFingers; i++) {
          if (activeFingers[i].fingerId == event.tfinger.fingerId) {
            gfxSetButtonPressed(activeFingers[i].buttonIndex, 0);
            eventData.type = eventKeyUp;
            eventData.data.input = (InputCode){inputLogical, buttons[activeFingers[i].buttonIndex].key};
            onEvent(eventData);
            for (int j = i; j < numActiveFingers - 1; j++) {
              activeFingers[j] = activeFingers[j + 1];
            }
            numActiveFingers--;
            break;
          }
        }
      }
#endif
    }

#ifdef MOBILE_LIFECYCLE
    if (wakeRedrawFrames > 0) {
      eventData.type = eventFullRedraw;
      eventData.data.value = 0;
      onEvent(eventData);
      wakeRedrawFrames--;
    }
#endif

    eventData.type = eventTick;
    eventData.data.value = 0;
    onEvent(eventData);

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
