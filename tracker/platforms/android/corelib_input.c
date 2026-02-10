#include "../sdl2/corelib_input.h"
#include "corelib_mainloop.h"
#include <SDL2/SDL.h>
#include <stdio.h>

// Callback for raw input capture (required by SDL2 input system)
void (*inputRawCallback)(int32_t keyCode, int isDown) = NULL;

// Virtual gamepad state (shared with graphics)
int vpadEnabled = 1;
int gameAreaHeight;
SDL_Rect dpadRect, aButtonRect, bButtonRect, startButtonRect, selectButtonRect;
SDL_Rect dpadUpRect, dpadDownRect, dpadLeftRect, dpadRightRect;

// Touch state tracking
static int touchPressed[8] = {0}; // UP, DOWN, LEFT, RIGHT, A, B, START, SELECT
static int lastTouchX = -1, lastTouchY = -1;

// Virtual button hit testing
static int isPointInRect(int x, int y, SDL_Rect* rect) {
  return (x >= rect->x && x < rect->x + rect->w && y >= rect->y && y < rect->y + rect->h);
}

static int getTouchButton(int x, int y) {
  if (!vpadEnabled) return -1;

  // Check action buttons
  if (isPointInRect(x, y, &aButtonRect)) return keyEdit;
  if (isPointInRect(x, y, &bButtonRect)) return keyOpt;
  if (isPointInRect(x, y, &startButtonRect)) return keyPlay;
  if (isPointInRect(x, y, &selectButtonRect)) return keyShift;

  // Check D-pad buttons
  if (isPointInRect(x, y, &dpadUpRect)) return keyUp;
  if (isPointInRect(x, y, &dpadDownRect)) return keyDown;
  if (isPointInRect(x, y, &dpadLeftRect)) return keyLeft;
  if (isPointInRect(x, y, &dpadRightRect)) return keyRight;

  return -1;
}

int inputUpdate(void) {
  SDL_Event event;
  int result = 0;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
      result = 1;
      break;

      case SDL_FINGERDOWN:
      case SDL_MOUSEBUTTONDOWN: {
        int x, y;
        if (event.type == SDL_FINGERDOWN) {
          // Convert normalized coordinates to screen coordinates
          x = (int)(event.tfinger.x * SDL_GetWindowSurface(SDL_GetWindowFromID(event.tfinger.windowID))->w);
          y = (int)(event.tfinger.y * SDL_GetWindowSurface(SDL_GetWindowFromID(event.tfinger.windowID))->h);
        } else {
          x = event.button.x;
          y = event.button.y;
        }

        int button = getTouchButton(x, y);
        if (button >= 0) {
          touchPressed[button] = 1;
        }
        lastTouchX = x;
        lastTouchY = y;
        break;
      }

      case SDL_FINGERUP:
      case SDL_MOUSEBUTTONUP: {
        // Clear all touch states on release
        for (int i = 0; i < 8; i++) {
          touchPressed[i] = 0;
        }
        lastTouchX = lastTouchY = -1;
        break;
      }

      case SDL_FINGERMOTION:
      case SDL_MOUSEMOTION: {
        if (lastTouchX >= 0) { // Only if we have an active touch
          int x, y;
          if (event.type == SDL_FINGERMOTION) {
            x = (int)(event.tfinger.x * SDL_GetWindowSurface(SDL_GetWindowFromID(event.tfinger.windowID))->w);
            y = (int)(event.tfinger.y * SDL_GetWindowSurface(SDL_GetWindowFromID(event.tfinger.windowID))->h);
          } else {
            x = event.motion.x;
            y = event.motion.y;
          }

          // Clear previous touch states
          for (int i = 0; i < 8; i++) {
            touchPressed[i] = 0;
          }

          // Set new touch state
          int button = getTouchButton(x, y);
          if (button >= 0) {
            touchPressed[button] = 1;
          }
          lastTouchX = x;
          lastTouchY = y;
        }
        break;
      }

      case SDL_KEYDOWN:
      case SDL_KEYUP:
      // Handle physical keyboard input (for testing/debugging)
      break;
    }
  }

  return result;
}

int inputPressed(int button) {
  // Check touch input first
  if (button < 8 && touchPressed[button]) {
    return 1;
  }

  // Fall back to SDL2 keyboard input for testing
  const Uint8* keyState = SDL_GetKeyboardState(NULL);

  if (button & keyUp) return keyState[SDL_SCANCODE_UP] || keyState[SDL_SCANCODE_W];
  if (button & keyDown) return keyState[SDL_SCANCODE_DOWN] || keyState[SDL_SCANCODE_S];
  if (button & keyLeft) return keyState[SDL_SCANCODE_LEFT] || keyState[SDL_SCANCODE_A];
  if (button & keyRight) return keyState[SDL_SCANCODE_RIGHT] || keyState[SDL_SCANCODE_D];
  if (button & keyEdit) return keyState[SDL_SCANCODE_SPACE] || keyState[SDL_SCANCODE_Z];
  if (button & keyOpt) return keyState[SDL_SCANCODE_LSHIFT] || keyState[SDL_SCANCODE_X];
  if (button & keyPlay) return keyState[SDL_SCANCODE_RETURN];
  if (button & keyShift) return keyState[SDL_SCANCODE_TAB];

  return 0;
}

// Include remaining SDL2 input functions
typedef enum {
  KEYBOARD_LAYOUT_QWERTY = 0,
  KEYBOARD_LAYOUT_QWERTZ = 1
} KeyboardLayout;

KeyboardLayout detectKeyboardLayout(void) {
  return KEYBOARD_LAYOUT_QWERTY; // Default for Android
}

void inputInitDefaultKeyMapping(void) {
  // Android uses touch controls, no key mapping needed
}

const char* inputGetKeyName(int32_t keyCode) {
  if (keyCode == 0) return "---";
  return "Touch";
}