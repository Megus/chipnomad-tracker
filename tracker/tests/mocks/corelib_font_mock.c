#include "corelib_font.h"
#include <stddef.h>

// Mock font data (minimal stub)
static uint8_t mockFontData[1] = {0};

static const Font mockFont = {
  .name = "Mock",
  .resolutions = {
    {16, 24, mockFontData}
  },
  .resolutionCount = 1
};

const Font* fontGetDefault(void) {
  return &mockFont;
}

void fontSetCurrent(const Font* font) {
  // No-op
}

const Font* fontGetCurrent(void) {
  return &mockFont;
}

const FontResolution* fontSelectResolution(const Font* font, int screenWidth, int screenHeight) {
  return &mockFont.resolutions[0];
}

Font* fontLoad(const char* path) {
  return NULL;
}

void fontFree(Font* font) {
  // No-op
}
