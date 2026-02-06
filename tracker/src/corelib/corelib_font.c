#include "corelib_font.h"
#include "corelib_file.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define TEXT_COLS 40
#define TEXT_ROWS 20

extern uint8_t font12x16[];
extern uint8_t font16x24[];
extern uint8_t font24x36[];
extern uint8_t font32x48[];
extern uint8_t font48x54[];

static const Font defaultFont = {
  .name = "Default",
  .resolutions = {
    {12, 16, font12x16},
    {16, 24, font16x24},
    {24, 36, font24x36},
    {32, 48, font32x48},
    {48, 54, font48x54}
  },
  .resolutionCount = 5
};

static const Font* currentFont = &defaultFont;

const Font* fontGetDefault(void) {
  return &defaultFont;
}

void fontSetCurrent(const Font* font) {
  currentFont = font ? font : &defaultFont;
}

const Font* fontGetCurrent(void) {
  return currentFont;
}

const FontResolution* fontSelectResolution(const Font* font, int screenWidth, int screenHeight) {
  if (!font || font->resolutionCount == 0) return NULL;

  // Try to find largest font that fits
  const FontResolution* best = NULL;
  for (int i = font->resolutionCount - 1; i >= 0; i--) {
    const FontResolution* res = &font->resolutions[i];
    if (screenWidth >= TEXT_COLS * res->charWidth && screenHeight >= TEXT_ROWS * res->charHeight) {
      best = res;
      break;
    }
  }

  // If no font fits, use smallest
  if (!best) {
    best = &font->resolutions[0];
  }

  return best;
}

Font* fontLoad(const char* path) {
  int f = fileOpen(path, 0);
  if (f < 0) return NULL;

  Font* font = calloc(1, sizeof(Font));
  if (!font) {
    fileClose(f);
    return NULL;
  }

  int resIdx = -1;
  int charIdx = 0;
  uint8_t* currentData = NULL;
  int bytesPerChar = 0;
  char* line;

  while ((line = fileReadString(f)) != NULL) {
    // Skip comments and empty lines
    char* p = line;
    while (*p && isspace(*p)) p++;
    if (*p == '#' || *p == '\0') continue;

    // Parse name
    if (strncmp(p, "name:", 5) == 0) {
      p += 5;
      while (*p && isspace(*p)) p++;
      char* end = p + strlen(p) - 1;
      while (end > p && isspace(*end)) end--;
      *(end + 1) = '\0';
      strncpy(font->name, p, 15);
      font->name[15] = '\0';
      continue;
    }

    // Parse resolution
    if (strncmp(p, "resolution:", 11) == 0) {
      if (resIdx >= 0 && currentData) {
        font->resolutions[resIdx].data = currentData;
      }
      resIdx++;
      if (resIdx >= 5) break;

      p += 11;
      int w, h;
      if (sscanf(p, "%dx%d", &w, &h) == 2) {
        font->resolutions[resIdx].charWidth = w;
        font->resolutions[resIdx].charHeight = h;
        bytesPerChar = ((w + 7) / 8) * h;
        currentData = malloc(95 * bytesPerChar);
        charIdx = 0;
      }
      continue;
    }

    // Parse hex data
    if (resIdx >= 0 && currentData && charIdx < 95) {
      uint8_t* dest = currentData + charIdx * bytesPerChar;
      int byteIdx = 0;
      while (*p && byteIdx < bytesPerChar) {
        while (*p && isspace(*p)) p++;
        if (!*p) break;
        unsigned int byte;
        if (sscanf(p, "%2x", &byte) == 1) {
          dest[byteIdx++] = byte;
          p += 2;
        } else {
          break;
        }
      }
      charIdx++;
    }
  }

  if (resIdx >= 0 && currentData) {
    font->resolutions[resIdx].data = currentData;
  }

  font->resolutionCount = resIdx + 1;
  fileClose(f);

  if (font->resolutionCount == 0) {
    free(font);
    return NULL;
  }

  return font;
}

void fontFree(Font* font) {
  if (!font || font == &defaultFont) return;

  for (int i = 0; i < font->resolutionCount; i++) {
    free(font->resolutions[i].data);
  }
  free(font);
}
