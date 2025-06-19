#include <SDL2/SDL.h>
#include <stdint.h>
#include <version.h>
#include "corelib_gfx.h"
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH (640)
#define WINDOW_HEIGHT (480)
#define PRINT_BUFFER_SIZE (256)

#define CHAR_X(x) ((x) * fontW * 8)
#define CHAR_Y(y) ((y) * fontH)

extern uint8_t font16x24[];

SDL_Window* window = NULL;
SDL_Renderer * renderer = NULL;

static uint32_t fgColor = 0;
static uint32_t bgColor = 0;
static uint32_t cursorColor = 0;
static uint32_t selectionColor = 0;
static uint8_t* font = NULL;
static char printBuffer[PRINT_BUFFER_SIZE];
static int fontH;
static int fontW;
static int isDirty;

static char charBuffer[80];

// FIXME: On RG35XX+, the SDL2 seems to be built without haptic
// support enabled, so SDL_INIT_EVERYTHING will fail.
#ifdef RG35XX_PLUS_BUILD
#define SDL_INIT_FLAGS (SDL_INIT_EVERYTHING & ~SDL_INIT_HAPTIC)
#else
#define SDL_INIT_FLAGS (SDL_INIT_EVERYTHING)
#endif

int gfxSetup(void) {
  if (SDL_Init(SDL_INIT_FLAGS) != 0) {
    fprintf(stderr, "SDL2 Initialization Error: %s\n", SDL_GetError());
    return 1;
  }

  sprintf(charBuffer, "%s v%s (%s)", appTitle, appVersion, appBuild);

  window = SDL_CreateWindow(charBuffer,
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    WINDOW_WIDTH, WINDOW_HEIGHT,
    SDL_WINDOW_SHOWN);

  if (!window) {
    fprintf(stderr, "SDL2 Create Window Error: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

  font = font16x24;
  fontW = 2; // In bytes
  fontH = 24;

  isDirty = 1;

  return 0;
}

void gfxCleanup(void) {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void gfxSetFgColor(int rgb) {
  fgColor = rgb;
}

void gfxSetBgColor(int rgb) {
  bgColor = rgb;
}

void gfxSetCursorColor(int rgb) {
  cursorColor = rgb;
}

void gfxSetSelectionColor(int rgb) {
  selectionColor = rgb;
}

static void setColor(int rgb) {
  SDL_SetRenderDrawColor(renderer, (rgb & 0xff0000) >> 16, (rgb & 0xff00) >> 8, rgb & 0xff, 255);
}

void gfxClear(void) {
  setColor(bgColor);
  SDL_RenderClear(renderer);
  isDirty = 1;
}

void gfxPoint(int x, int y, uint32_t color) {
  setColor(color);
  SDL_RenderDrawPoint(renderer, x, y);
  isDirty = 1;
}

void gfxFillRect(int x, int y, int w, int h) {
  SDL_Rect rect = { CHAR_X(x), CHAR_Y(y), CHAR_X(w), CHAR_Y(h) };
  setColor(fgColor);
  SDL_RenderFillRect(renderer, &rect);
  isDirty = 1;
}

void gfxClearRect(int x, int y, int w, int h) {
  SDL_Rect rect = { CHAR_X(x), CHAR_Y(y), CHAR_X(w), CHAR_Y(h) };
  setColor(bgColor);
  SDL_RenderFillRect(renderer, &rect);
  isDirty = 1;
}

void gfxPrint(int x, int y, const char* text) {
  if (text == NULL) return;

  int cx = CHAR_X(x);
  int cy = CHAR_Y(y);

  int len = (int)strlen(text);

  for (int i = 0; i < len; i++) {
    uint8_t C = text[i];
    if (C == '\r' && text[i + 1] == '\n') {
      i++;
      cx = x;
      cy += fontH;
      if (cy > WINDOW_HEIGHT) {
        cy = y;
      }
      continue;
    }

    for (int l = 0; l < fontH; l++) {
      for (int c = 0; c < fontW; c++) {
        uint8_t mask = 0x80;
        int fontByte = font[(C - 32) * fontW * fontH + l * fontW + c];

        for (int b = 0; b < 8; b++) {
          gfxPoint(cx + c * 8 + b, cy + l, (fontByte & mask) ? fgColor : bgColor);
          mask = mask >> 1;
        }
      }
    }

    cx += fontW * 8;
    if (cx > WINDOW_WIDTH) {
      cx = x;
      cy += fontH;
    }
  }
  isDirty = 1;
}

void gfxPrintf(int x, int y, const char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(printBuffer, 256, format, args);
  va_end(args);
  gfxPrint(x, y, printBuffer);
}

void gfxCursor(int x, int y, int w) {
  int cx = CHAR_X(x);
  int cy = CHAR_Y(y) + fontH - 1;
  for (int c = 0; c < CHAR_X(w); c++) {
    gfxPoint(cx + c, cy, cursorColor);
  }
}

void gfxSelection(int x, int y, int w, int h) {
  int cx = CHAR_X(x);
  int cy = CHAR_Y(y);
  int cw = CHAR_X(w);
  int ch = CHAR_Y(h);
  for (int c = 0; c < cw; c++) {
    gfxPoint(cx + c, cy, selectionColor);
    gfxPoint(cx + c, cy + ch - 1, selectionColor);
  }
  for (int c = 0; c < ch; c++) {
    gfxPoint(cx, cy + c, selectionColor);
    gfxPoint(cx + cw - 1, cy + c, selectionColor);
  }
}

void gfxUpdateScreen(void) {
  if (isDirty) {
    SDL_RenderPresent(renderer);
  }
  isDirty = 0;
}
