#include <SDL/SDL.h>
#include <stdint.h>
#include "version.h"
#include "corelib_gfx.h"

#define WINDOW_WIDTH (640)
#define WINDOW_HEIGHT (480)
#define PRINT_BUFFER_SIZE (256)

#define CHAR_X(x) ((x) * fontW * 8)
#define CHAR_Y(y) ((y) * fontH)

extern uint8_t font16x24[];

SDL_Surface *sdlScreen;
static uint32_t fgColor = 0;
static uint32_t bgColor = 0;
static uint32_t cursorColor = 0;
static int fgR = 255, fgG = 255, fgB = 255;
static int bgR = 0, bgG = 0, bgB = 0;
static int cursorR = 255, cursorG = 255, cursorB = 255;
static uint8_t* font = NULL;
static char printBuffer[PRINT_BUFFER_SIZE];
static int fontH;
static int fontW;
static int isDirty;

// Font surface optimization
static SDL_Surface* charSurfaces[95]; // ASCII 32-126

static char charBuffer[80];

int gfxSetup(int *screenWidth, int *screenHeight) {
  // Currently, SDL1.2 build is only for pre-2024 RG35xx which has 640x480 screen
  // So I ignore passed screenWidth and screenHeight

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


  sprintf(charBuffer, "%s v%s (%s)", appTitle, appVersion, appBuild);
  SDL_WM_SetCaption(charBuffer, NULL);

  font = font16x24;
  fontW = 2; // In bytes
  fontH = 24;

  // Create 8-bit character surfaces
  for (int ch = 0; ch < 95; ch++) {
    charSurfaces[ch] = SDL_CreateRGBSurface(SDL_SWSURFACE, fontW * 8, fontH, 8, 0, 0, 0, 0);
    SDL_SetColorKey(charSurfaces[ch], SDL_SRCCOLORKEY, 0); // Index 0 = transparent

    for (int l = 0; l < fontH; l++) {
      for (int c = 0; c < fontW; c++) {
        uint8_t fontByte = font[ch * fontW * fontH + l * fontW + c];
        uint8_t mask = 0x80;

        for (int b = 0; b < 8; b++) {
          ((Uint8 *)charSurfaces[ch]->pixels)[l * charSurfaces[ch]->w + (c * 8 + b)] = (fontByte & mask) ? 1 : 0;
          mask >>= 1;
        }
      }
    }
  }

  isDirty = 1;

  return 0;
}

void gfxCleanup(void) {
  for (int i = 0; i < 95; i++) {
    if (charSurfaces[i]) SDL_FreeSurface(charSurfaces[i]);
  }
  SDL_FreeSurface(sdlScreen);
}

void gfxSetFgColor(int rgb) {
  fgR = (rgb & 0xff0000) >> 16;
  fgG = (rgb & 0xff00) >> 8;
  fgB = rgb & 0xff;
  fgColor = SDL_MapRGB(sdlScreen->format, fgR, fgG, fgB);
}

void gfxSetBgColor(int rgb) {
  bgR = (rgb & 0xff0000) >> 16;
  bgG = (rgb & 0xff00) >> 8;
  bgB = rgb & 0xff;
  bgColor = SDL_MapRGB(sdlScreen->format, bgR, bgG, bgB);
}

void gfxSetCursorColor(int rgb) {
  cursorR = (rgb & 0xff0000) >> 16;
  cursorG = (rgb & 0xff00) >> 8;
  cursorB = rgb & 0xff;
  cursorColor = SDL_MapRGB(sdlScreen->format, cursorR, cursorG, cursorB);
}

void gfxClear(void) {
  SDL_FillRect(sdlScreen, NULL, bgColor);
  isDirty = 1;
}

void gfxPoint(int x, int y, uint32_t color) {
  ((Uint32 *)sdlScreen->pixels)[y * sdlScreen->w + x] = color;
  isDirty = 1;
}

void gfxClearRect(int x, int y, int w, int h) {
  SDL_Rect rect = { CHAR_X(x), CHAR_Y(y), CHAR_X(w), CHAR_Y(h) };
  SDL_FillRect(sdlScreen, &rect, bgColor);
  isDirty = 1;
}

void gfxPrint(int x, int y, const char* text) {
  if (text == NULL) return;

  int cx = CHAR_X(x);
  int cy = CHAR_Y(y);
  int len = (int)strlen(text);

  // Draw background rectangles first
  for (int i = 0; i < len; i++) {
    if (text[i] == '\r' && text[i + 1] == '\n') {
      i++;
      cx = CHAR_X(x);
      cy += fontH;
      continue;
    }
    SDL_Rect bgRect = {cx, cy, fontW * 8, fontH};
    SDL_FillRect(sdlScreen, &bgRect, bgColor);
    cx += fontW * 8;
    if (cx > WINDOW_WIDTH) {
      cx = CHAR_X(x);
      cy += fontH;
    }
  }

  // Draw characters
  cx = CHAR_X(x);
  cy = CHAR_Y(y);

  for (int i = 0; i < len; i++) {
    uint8_t C = text[i];
    if (C == '\r' && text[i + 1] == '\n') {
      i++;
      cx = CHAR_X(x);
      cy += fontH;
      if (cy > WINDOW_HEIGHT) {
        cy = CHAR_Y(y);
      }
      continue;
    }

    if (C >= 32 && C <= 126) {
      SDL_Color colors[2] = {
        {0, 0, 0, 0},  // Index 0: transparent
        {fgR, fgG, fgB, 255}  // Index 1: foreground
      };
      SDL_SetColors(charSurfaces[C - 32], colors, 0, 2);

      SDL_Rect dstRect = {cx, cy, fontW * 8, fontH};
      SDL_BlitSurface(charSurfaces[C - 32], NULL, sdlScreen, &dstRect);
    }

    cx += fontW * 8;
    if (cx > WINDOW_WIDTH) {
      cx = CHAR_X(x);
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
  SDL_Rect rect = { CHAR_X(x), CHAR_Y(y) + fontH - 1, CHAR_X(w), 1 };
  SDL_FillRect(sdlScreen, &rect, cursorColor);
  isDirty = 1;
}

void gfxRect(int x, int y, int w, int h) {
  int cx = CHAR_X(x);
  int cy = CHAR_Y(y);
  int cw = CHAR_X(w);
  int ch = CHAR_Y(h);

  SDL_Rect rects[4] = {
    {cx, cy, cw, 1},           // top
    {cx, cy + ch - 1, cw, 1}, // bottom
    {cx, cy, 1, ch},          // left
    {cx + cw - 1, cy, 1, ch}  // right
  };

  for (int i = 0; i < 4; i++) {
    SDL_FillRect(sdlScreen, &rects[i], fgColor);
  }
  isDirty = 1;
}

void gfxUpdateScreen(void) {
  if (isDirty) {
    SDL_UpdateRect(sdlScreen, 0, 0, 0, 0);
  }
  isDirty = 0;
}

void gfxDrawCharBitmap(uint8_t* bitmap, int col, int row) {
  int cx = CHAR_X(col);
  int cy = CHAR_Y(row);
  int charW = fontW * 8;
  
  for (int y = 0; y < fontH; y++) {
    for (int x = 0; x < charW; x++) {
      uint8_t alpha = bitmap[y * charW + x];
      uint8_t r = bgR + ((fgR - bgR) * alpha) / 255;
      uint8_t g = bgG + ((fgG - bgG) * alpha) / 255;
      uint8_t b = bgB + ((fgB - bgB) * alpha) / 255;
      uint32_t color = SDL_MapRGB(sdlScreen->format, r, g, b);
      ((Uint32 *)sdlScreen->pixels)[(cy + y) * sdlScreen->w + (cx + x)] = color;
    }
  }
  isDirty = 1;
}

int gfxGetCharWidth(void) {
  return fontW * 8;
}

int gfxGetCharHeight(void) {
  return fontH;
}