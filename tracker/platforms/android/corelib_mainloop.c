// Android-specific mainloop implementation with touch support
#include "corelib_mainloop.h"
#include <stdint.h>
#include <SDL2/SDL.h>
#include "corelib_gfx.h"
#include "../sdl2/corelib_input.h"
#include "common.h"

#define FPS 60

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

void mainLoopRun(void (*draw)(void), void (*onEvent)(enum MainLoopEvent event, int value, void* userdata)) {
  uint32_t delay = 1000 / FPS;
  uint32_t start;
  uint32_t busytime = 0;
  SDL_Event event;

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
        return;
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