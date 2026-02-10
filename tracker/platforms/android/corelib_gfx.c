#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdarg.h>
#include "version.h"
#include "corelib_gfx.h"
#include "corelib_font.h"
#include "button_icons.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PRINT_BUFFER_SIZE (256)
#define TEXT_COLS (40)
#define TEXT_ROWS (20)

#define CHAR_X(x) ((x) * charW + offsetX)
#define CHAR_Y(y) ((y) * charH + offsetY)

// Virtual gamepad constants
#define VPAD_BUTTON_SIZE 110    // Larger buttons for better touch
#define VPAD_MARGIN 30

#ifdef PORTMASTER_BUILD
#define SDL_INIT_FLAGS (SDL_INIT_EVERYTHING & ~SDL_INIT_HAPTIC)
#else
#define SDL_INIT_FLAGS (SDL_INIT_EVERYTHING)
#endif

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
static char printBuffer[PRINT_BUFFER_SIZE];
int screenW;  // Make accessible to other files
int screenH;  // Make accessible to other files
static int charW;
static int charH;
static int offsetX;
static int offsetY;
static int isDirty;
static const FontResolution* currentResolution = NULL;

// Virtual gamepad state (shared with input)
extern int vpadEnabled;
extern SDL_Rect dpadRect, aButtonRect, bButtonRect, startButtonRect, selectButtonRect;
extern SDL_Rect dpadUpRect, dpadDownRect, dpadLeftRect, dpadRightRect;

// Button press state tracking
static int buttonPressed[8] = {0}; // UP, DOWN, LEFT, RIGHT, EDIT, OPT, PLAY, SHIFT

// Font texture optimization
static SDL_Texture* fontTexture = NULL;
static SDL_Rect charRects[95];

static char charBuffer[80];

// Virtual gamepad functions
static void setupVirtualGamepad(void) {
  if (!vpadEnabled) return;

  // Calculate positions based on screen size
  int buttonGap = 15;

  // D-pad on left side, in lower half of screen
  int dpadX = VPAD_MARGIN;
  int dpadY = screenH - (VPAD_BUTTON_SIZE + VPAD_MARGIN) * 4;

  dpadUpRect = (SDL_Rect){dpadX + VPAD_BUTTON_SIZE + buttonGap, dpadY, VPAD_BUTTON_SIZE, VPAD_BUTTON_SIZE};
  dpadDownRect = (SDL_Rect){dpadX + VPAD_BUTTON_SIZE + buttonGap, dpadY + VPAD_BUTTON_SIZE + buttonGap, VPAD_BUTTON_SIZE, VPAD_BUTTON_SIZE};
  dpadLeftRect = (SDL_Rect){dpadX, dpadY + VPAD_BUTTON_SIZE + buttonGap, VPAD_BUTTON_SIZE, VPAD_BUTTON_SIZE};
  dpadRightRect = (SDL_Rect){dpadX + (VPAD_BUTTON_SIZE + buttonGap) * 2, dpadY + VPAD_BUTTON_SIZE + buttonGap, VPAD_BUTTON_SIZE, VPAD_BUTTON_SIZE};

  // Edit/Opt buttons on right side, same vertical level as d-pad center
  int rightX = screenW - VPAD_MARGIN - VPAD_BUTTON_SIZE;
  int rightY = dpadY + VPAD_BUTTON_SIZE + buttonGap;

  bButtonRect = (SDL_Rect){rightX - VPAD_BUTTON_SIZE - buttonGap, rightY, VPAD_BUTTON_SIZE, VPAD_BUTTON_SIZE};
  aButtonRect = (SDL_Rect){rightX, rightY, VPAD_BUTTON_SIZE, VPAD_BUTTON_SIZE};

  // Start/Select in middle, near bottom
  int centerX = screenW / 2;
  int bottomY = screenH - VPAD_BUTTON_SIZE - VPAD_MARGIN;

  selectButtonRect = (SDL_Rect){centerX - VPAD_BUTTON_SIZE - buttonGap, bottomY, VPAD_BUTTON_SIZE, VPAD_BUTTON_SIZE};
  startButtonRect = (SDL_Rect){centerX + buttonGap, bottomY, VPAD_BUTTON_SIZE, VPAD_BUTTON_SIZE};

  // Keep old dpadRect for compatibility
  dpadRect = (SDL_Rect){dpadX, dpadY, VPAD_BUTTON_SIZE * 3 + buttonGap * 2, VPAD_BUTTON_SIZE * 2 + buttonGap};
}

static void drawButton(SDL_Rect* buttonRect, const uint8_t* iconData, int buttonIndex) {
  // Draw button background with press state
  int bgColor = buttonPressed[buttonIndex] ? 120 : 80;
  SDL_SetRenderDrawColor(renderer, bgColor, bgColor, bgColor, 255);
  SDL_RenderFillRect(renderer, buttonRect);
  
  // Draw button border
  SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
  SDL_RenderDrawRect(renderer, buttonRect);
  
  // Draw icon
  if (iconData) {
    int iconX = buttonRect->x + (buttonRect->w - ICON_WIDTH) / 2;
    int iconY = buttonRect->y + (buttonRect->h - ICON_HEIGHT) / 2;
    
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    for (int y = 0; y < ICON_HEIGHT; y++) {
      for (int x = 0; x < ICON_WIDTH; x++) {
        int byteIndex = y * ICON_BYTES_PER_ROW + x / 8;
        int bitIndex = 7 - (x % 8);
        
        if (iconData[byteIndex] & (1 << bitIndex)) {
          SDL_RenderDrawPoint(renderer, iconX + x, iconY + y);
        }
      }
    }
  }
}

static void drawVirtualGamepad(void) {
  if (!vpadEnabled) return;

  // Draw all buttons using unified function
  drawButton(&dpadUpRect, icon_arrow_up, 0);
  drawButton(&dpadDownRect, icon_arrow_down, 1);
  drawButton(&dpadLeftRect, icon_arrow_left, 2);
  drawButton(&dpadRightRect, icon_arrow_right, 3);
  drawButton(&aButtonRect, icon_edit, 4);
  drawButton(&bButtonRect, icon_opt, 5);
  drawButton(&startButtonRect, icon_play, 6);
  drawButton(&selectButtonRect, icon_shift, 7);
}

static void createFontTexture(void) {
  if (!currentResolution || !currentResolution->data) return;

  int fontW = (currentResolution->charWidth + 7) / 8;  // Bytes per row
  uint8_t* fontData = currentResolution->data;

  fontTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, charW * 95, charH);
  SDL_SetTextureBlendMode(fontTexture, SDL_BLENDMODE_BLEND);

  SDL_SetRenderTarget(renderer, fontTexture);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);

  for (int ch = 0; ch < 95; ch++) {
    int charX = ch * charW;
    charRects[ch] = (SDL_Rect){charX, 0, charW, charH};

    for (int l = 0; l < charH; l++) {
      for (int c = 0; c < fontW; c++) {
        uint8_t fontByte = fontData[ch * fontW * charH + l * fontW + c];
        int bitsToDraw = (c == fontW - 1 && charW % 8 != 0) ? charW % 8 : 8;
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

// Override gfxSetup to add virtual gamepad
int gfxSetup(int *screenWidth, int *screenHeight) {
  if (SDL_Init(SDL_INIT_FLAGS) != 0) {
    fprintf(stderr, "SDL2 Initialization Error: %s\\n", SDL_GetError());
    return 1;
  }

  sprintf(charBuffer, "%s v%s (%s)", appTitle, appVersion, appBuild);

  // Get screen resolution
  SDL_DisplayMode dm;
  if (SDL_GetDesktopDisplayMode(0, &dm) == 0) {
    screenW = dm.w;
    screenH = dm.h;
  } else {
    screenW = 640;
    screenH = 960; // Default portrait
  }

  if (screenWidth) *screenWidth = screenW;
  if (screenHeight) *screenHeight = screenH;

  window = SDL_CreateWindow(charBuffer,
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    screenW, screenH,
    SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FULLSCREEN);

  if (!window) {
    fprintf(stderr, "SDL2 Create Window Error: %s\\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

  // Setup virtual gamepad layout
  setupVirtualGamepad();

  // Use full screen height for font selection
  currentResolution = fontSelectResolution(fontGetCurrent(), screenW, screenH);
  if (currentResolution) {
    charW = currentResolution->charWidth;
    charH = currentResolution->charHeight;
  } else {
    charW = 12;
    charH = 16;
  }

  // Position the text window at the top of the screen
  int textWindowW = TEXT_COLS * charW;
  // int textWindowH = TEXT_ROWS * charH;  // Unused for now
  offsetX = (screenW - textWindowW) / 2;  // Center horizontally
  offsetY = 20;  // Small margin from top

  createFontTexture();
  isDirty = 1;

  return 0;
}

// Override gfxUpdateScreen to draw virtual gamepad
void gfxUpdateScreen(void) {
  if (isDirty) {
    drawVirtualGamepad();
    SDL_RenderPresent(renderer);
  }
  isDirty = 0;
}

// Standard graphics functions
void gfxCleanup(void) {
  if (fontTexture) SDL_DestroyTexture(fontTexture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

void gfxSetFgColor(int rgb) { fgColor = rgb; }
void gfxSetBgColor(int rgb) { bgColor = rgb; }
void gfxSetCursorColor(int rgb) { cursorColor = rgb; }

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
  SDL_Rect rect = { CHAR_X(x), CHAR_Y(y), w * charW, h * charH };
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
      cy += charH;
      continue;
    }
    SDL_Rect bgRect = {cx, cy, charW, charH};
    SDL_RenderFillRect(renderer, &bgRect);
    cx += charW;
    if (cx >= offsetX + TEXT_COLS * charW) {
      cx = CHAR_X(x);
      cy += charH;
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
      cy += charH;
      if (cy >= offsetY + TEXT_ROWS * charH) {
        cy = CHAR_Y(y);
      }
      continue;
    }

    if (C >= 32 && C <= 126) {
      SDL_Rect dstRect = {cx, cy, charW, charH};
      SDL_RenderCopy(renderer, fontTexture, &charRects[C - 32], &dstRect);
    }

    cx += charW;
    if (cx >= offsetX + TEXT_COLS * charW) {
      cx = CHAR_X(x);
      cy += charH;
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
  SDL_Rect rect = { CHAR_X(x), CHAR_Y(y) + charH - 1, w * charW, 1 };
  setColor(cursorColor);
  SDL_RenderFillRect(renderer, &rect);
  isDirty = 1;
}

void gfxRect(int x, int y, int w, int h) {
  int cx = CHAR_X(x);
  int cy = CHAR_Y(y);
  int cw = w * charW;
  int ch = h * charH;
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

void gfxDrawCharBitmap(uint8_t* bitmap, int col, int row) {
  int cx = CHAR_X(col);
  int cy = CHAR_Y(row);

  uint8_t fgR = (fgColor >> 16) & 0xFF;
  uint8_t fgG = (fgColor >> 8) & 0xFF;
  uint8_t fgB = fgColor & 0xFF;
  uint8_t bgR = (bgColor >> 16) & 0xFF;
  uint8_t bgG = (bgColor >> 8) & 0xFF;
  uint8_t bgB = bgColor & 0xFF;

  for (int y = 0; y < charH; y++) {
    for (int x = 0; x < charW; x++) {
      uint8_t alpha = bitmap[y * charW + x];
      uint8_t r = bgR + ((fgR - bgR) * alpha) / 255;
      uint8_t g = bgG + ((fgG - bgG) * alpha) / 255;
      uint8_t b = bgB + ((fgB - bgB) * alpha) / 255;
      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_RenderDrawPoint(renderer, cx + x, cy + y);
    }
  }
  isDirty = 1;
}

int gfxGetCharWidth(void) { return charW; }
int gfxGetCharHeight(void) { return charH; }

// Update button press state for visual feedback
void gfxSetButtonPressed(int buttonIndex, int pressed) {
  if (buttonIndex >= 0 && buttonIndex < 8) {
    buttonPressed[buttonIndex] = pressed;
    isDirty = 1;
  }
}

void gfxReloadFont(void) {
  if (fontTexture) {
    SDL_DestroyTexture(fontTexture);
    fontTexture = NULL;
  }

  currentResolution = fontSelectResolution(fontGetCurrent(), screenW, screenH);
  if (currentResolution) {
    charW = currentResolution->charWidth;
    charH = currentResolution->charHeight;
  }

  int textWindowW = TEXT_COLS * charW;
  // int textWindowH = TEXT_ROWS * charH;  // Unused for now
  offsetX = (screenW - textWindowW) / 2;
  offsetY = 20;

  createFontTexture();
  isDirty = 1;
}
