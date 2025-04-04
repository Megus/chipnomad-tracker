#include <SDL/SDL.h>
#include <stdint.h>
#include "corelib_gfx.h"
#include "font.h"

#define WINDOW_WIDTH (640)
#define WINDOW_HEIGHT (480)
#define PRINT_BUFFER_SIZE (256)

#define CHAR_X(x) ((x) * fontW * 8)
#define CHAR_Y(y) ((y) * fontH)

SDL_Surface *sdlScreen;
static uint32_t fgColor = 0;
static uint32_t bgColor = 0;
static uint8_t* font = NULL;
static char printBuffer[PRINT_BUFFER_SIZE];
static int fontH;
static int fontW;

int gfxSetup(void) {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("SDL2 Initialization Error: %s\n", SDL_GetError());
    return 1;
  }

  sdlScreen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, SDL_HWSURFACE);
  if (!sdlScreen) {
    printf("SDL1.2 Set Video Mode Error: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_WM_SetCaption("Tracker", NULL);
  SDL_ShowCursor(SDL_DISABLE);

  font = font16x24;
  fontW = 2; // In bytes
  fontH = 24;

  return 0;
}

void gfxCleanup(void) {
  SDL_FreeSurface(sdlScreen);
}

void gfxSetFgColor(int rgb) {
  fgColor = SDL_MapRGB(sdlScreen->format, (rgb & 0xff0000) >> 16, (rgb & 0xff00) >> 8, rgb & 0xff);
}

void gfxSetBgColor(int rgb) {
  bgColor = SDL_MapRGB(sdlScreen->format, (rgb & 0xff0000) >> 16, (rgb & 0xff00) >> 8, rgb & 0xff);
}

void gfxClear(void) {
  SDL_FillRect(sdlScreen, NULL, bgColor);
}

void gfxPoint(int x, int y) {
  ((Uint32 *)sdlScreen->pixels)[y * sdlScreen->w + x] = fgColor;
}

void gfxFillRect(int x, int y, int w, int h) {
  SDL_Rect rect = { CHAR_X(x), CHAR_Y(y), CHAR_X(w), CHAR_Y(h) };
  SDL_FillRect(sdlScreen, &rect, fgColor);
}

void gfxClearRect(int x, int y, int w, int h) {
  SDL_Rect rect = { CHAR_X(x), CHAR_Y(y), CHAR_X(w), CHAR_Y(h) };
  SDL_FillRect(sdlScreen, &rect, bgColor);
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
          ((Uint32 *)sdlScreen->pixels)[(cy + l) * sdlScreen->w + (cx + c * 8 + b)] = (fontByte & mask) ? fgColor : bgColor;
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
}

void gfxPrintf(int x, int y, const char * format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(printBuffer, PRINT_BUFFER_SIZE, format, args);
  va_end(args);
  gfxPrint(x, y, printBuffer);
}
