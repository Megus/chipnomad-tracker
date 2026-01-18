#include <SDL2/SDL.h>
#include <stdint.h>
#include "version.h"
#include "corelib_gfx.h"
#include <stdio.h>
#include <string.h>

#define PRINT_BUFFER_SIZE (256)
#define TEXT_COLS (40)
#define TEXT_ROWS (20)

#define CHAR_X(x) ((x) * fontPixelW + offsetX)
#define CHAR_Y(y) ((y) * fontH + offsetY)

extern uint8_t font12x16[];
extern uint8_t font16x24[];
extern uint8_t font24x36[];
extern uint8_t font32x48[];
extern uint8_t font48x54[];

SDL_Window* window = NULL;
SDL_Renderer * renderer = NULL;

static uint32_t fgColor = 0;
static uint32_t bgColor = 0;
static uint32_t cursorColor = 0;
static uint8_t* font = NULL;
static char printBuffer[PRINT_BUFFER_SIZE];
static int screenW;
static int screenH;
static int fontH;
static int fontW;
static int fontPixelW;
static int offsetX;
static int offsetY;
static int isDirty;

// Font texture optimization
static SDL_Texture* fontTexture = NULL;
static SDL_Rect charRects[95]; // ASCII 32-126

static char charBuffer[80];

static void selectFont(void) {
  if (screenW >= TEXT_COLS * 48 && screenH >= TEXT_ROWS * 54) {
    font = font48x54;
    fontW = 6; fontPixelW = 48;
    fontH = 54;
  } else if (screenW >= TEXT_COLS * 32 && screenH >= TEXT_ROWS * 48) {
    font = font32x48;
    fontW = 4; fontPixelW = 32;
    fontH = 48;
  } else if (screenW >= TEXT_COLS * 24 && screenH >= TEXT_ROWS * 36) {
    font = font24x36;
    fontW = 3; fontPixelW = 24;
    fontH = 36;
  } else if (screenW >= TEXT_COLS * 16 && screenH >= TEXT_ROWS * 24) {
    font = font16x24;
    fontW = 2; fontPixelW = 16;
    fontH = 24;
  } else {
    font = font12x16;
    fontW = 2; fontPixelW = 12;
    fontH = 16;
  }
}

static void createFontTexture(void) {
  fontTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, fontPixelW * 95, fontH);
  SDL_SetTextureBlendMode(fontTexture, SDL_BLENDMODE_BLEND);

  SDL_SetRenderTarget(renderer, fontTexture);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  for (int ch = 0; ch < 95; ch++) {
    int charX = ch * fontPixelW;
    charRects[ch] = (SDL_Rect){charX, 0, fontPixelW, fontH};

    for (int l = 0; l < fontH; l++) {
      for (int c = 0; c < fontW; c++) {
        uint8_t fontByte = font[ch * fontW * fontH + l * fontW + c];
        int bitsToDraw = (c == fontW - 1 && fontPixelW % 8 != 0) ? fontPixelW % 8 : 8;
        uint8_t mask = 0x80;

        for (int b = 0; b < bitsToDraw; b++) {
          if (fontByte & mask) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawPoint(renderer, charX + c * 8 + b, l);
          }
          mask >>= 1;
        }
      }
    }
  }

  SDL_SetRenderTarget(renderer, NULL);
}

// FIXME: On RG35XX+, the SDL2 seems to be built without haptic
// support enabled, so SDL_INIT_EVERYTHING will fail.
#ifdef PORTMASTER_BUILD
#define SDL_INIT_FLAGS (SDL_INIT_EVERYTHING & ~SDL_INIT_HAPTIC)
#else
#define SDL_INIT_FLAGS (SDL_INIT_EVERYTHING)
#endif

int gfxSetup(int *screenWidth, int *screenHeight) {
  if (SDL_Init(SDL_INIT_FLAGS) != 0) {
    fprintf(stderr, "SDL2 Initialization Error: %s\n", SDL_GetError());
    return 1;
  }

  sprintf(charBuffer, "%s v%s (%s)", appTitle, appVersion, appBuild);

  // Detect screen resolution if not provided or zero
  if (screenWidth == NULL || screenHeight == NULL || *screenWidth == 0 || *screenHeight == 0) {
    #ifdef DESKTOP_BUILD
      // Default to 640x480 on desktop platforms
      screenW = 640;
      screenH = 480;
    #else
      // For other platforms, try to get display mode
      SDL_DisplayMode dm;
      if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
        screenW = dm.w;
        screenH = dm.h;
      } else {
        // Fallback to 640x480 if detection fails
        screenW = 640;
        screenH = 480;
      }
    #endif
    // Update the pointers if they're not NULL
    if (screenWidth != NULL) *screenWidth = screenW;
    if (screenHeight != NULL) *screenHeight = screenH;
  } else {
    screenW = *screenWidth;
    screenH = *screenHeight;
  }

  window = SDL_CreateWindow(charBuffer,
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    screenW, screenH,
    SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window) {
      fprintf(stderr, "SDL2 Create Window Error: %s\n", SDL_GetError());
      SDL_Quit();
      return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    // Check for high-DPI display and get actual drawable size
    int drawableW, drawableH;
    SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);
    if (drawableW != screenW || drawableH != screenH) {
      // High-DPI display detected, use drawable size for font selection
      screenW = drawableW;
      screenH = drawableH;
    }

    selectFont();

    // Center the text window
    int textWindowW = TEXT_COLS * fontPixelW;
    int textWindowH = TEXT_ROWS * fontH;
    offsetX = (screenW - textWindowW) / 2;
    offsetY = (screenH - textWindowH) / 2;

    createFontTexture();
    isDirty = 1;

    return 0;
  }

  void gfxCleanup(void) {
    if (fontTexture) SDL_DestroyTexture(fontTexture);
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

  void gfxClearRect(int x, int y, int w, int h) {
    SDL_Rect rect = { CHAR_X(x), CHAR_Y(y), w * fontPixelW, h * fontH };
    setColor(bgColor);
    SDL_RenderFillRect(renderer, &rect);
    isDirty = 1;
  }

  void gfxPrint(int x, int y, const char* text) {
    if (text == NULL) return;

    int cx = CHAR_X(x);
    int cy = CHAR_Y(y);
    int len = (int)strlen(text);

    // Draw background rectangles first
    setColor(bgColor);
    for (int i = 0; i < len; i++) {
      if (text[i] == '\r' && text[i + 1] == '\n') {
        i++;
        cx = CHAR_X(x);
        cy += fontH;
        continue;
      }
      SDL_Rect bgRect = {cx, cy, fontPixelW, fontH};
      SDL_RenderFillRect(renderer, &bgRect);
      cx += fontPixelW;
      if (cx >= offsetX + TEXT_COLS * fontPixelW) {
        cx = CHAR_X(x);
        cy += fontH;
      }
    }

    // Draw characters
    cx = CHAR_X(x);
    cy = CHAR_Y(y);
    SDL_SetTextureColorMod(fontTexture, (fgColor >> 16) & 0xFF, (fgColor >> 8) & 0xFF, fgColor & 0xFF);

    for (int i = 0; i < len; i++) {
      uint8_t C = text[i];
      if (C == '\r' && text[i + 1] == '\n') {
        i++;
        cx = CHAR_X(x);
        cy += fontH;
        if (cy >= offsetY + TEXT_ROWS * fontH) {
          cy = CHAR_Y(y);
        }
        continue;
      }

      if (C >= 32 && C <= 126) {
        SDL_Rect dstRect = {cx, cy, fontPixelW, fontH};
        SDL_RenderCopy(renderer, fontTexture, &charRects[C - 32], &dstRect);
      }

      cx += fontPixelW;
      if (cx >= offsetX + TEXT_COLS * fontPixelW) {
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
    SDL_Rect rect = { CHAR_X(x), CHAR_Y(y) + fontH - 1, w * fontPixelW, 1 };
    setColor(cursorColor);
    SDL_RenderFillRect(renderer, &rect);
    isDirty = 1;
  }

  void gfxRect(int x, int y, int w, int h) {
    int cx = CHAR_X(x);
    int cy = CHAR_Y(y);
    int cw = w * fontPixelW;
    int ch = h * fontH;
    setColor(fgColor);

    SDL_Rect rects[4] = {
      {cx, cy, cw, 1},           // top
      {cx, cy + ch - 1, cw, 1}, // bottom
      {cx, cy, 1, ch},          // left
      {cx + cw - 1, cy, 1, ch}  // right
    };
    SDL_RenderFillRects(renderer, rects, 4);
    isDirty = 1;
  }

  void gfxUpdateScreen(void) {
    if (isDirty) {
      SDL_RenderPresent(renderer);
    }
    isDirty = 0;
  }

  void gfxDrawCharBitmap(uint8_t* bitmap, int col, int row) {
    int cx = CHAR_X(col);
    int cy = CHAR_Y(row);
    
    uint8_t fgR = (fgColor >> 16) & 0xFF;
    uint8_t fgG = (fgColor >> 8) & 0xFF;
    uint8_t fgB = fgColor & 0xFF;
    uint8_t bgR = (bgColor >> 16) & 0xFF;
    uint8_t bgG = (bgColor >> 8) & 0xFF;
    uint8_t bgB = bgColor & 0xFF;
    
    for (int y = 0; y < fontH; y++) {
      for (int x = 0; x < fontPixelW; x++) {
        uint8_t alpha = bitmap[y * fontPixelW + x];
        uint8_t r = bgR + ((fgR - bgR) * alpha) / 255;
        uint8_t g = bgG + ((fgG - bgG) * alpha) / 255;
        uint8_t b = bgB + ((fgB - bgB) * alpha) / 255;
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawPoint(renderer, cx + x, cy + y);
      }
    }
    isDirty = 1;
  }

  int gfxGetCharWidth(void) {
    return fontPixelW;
  }

  int gfxGetCharHeight(void) {
    return fontH;
  }
