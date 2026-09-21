#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "font_manager.h"

constexpr int textCols = 40;
constexpr int textRows = 20;

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

const Font* FontManager::getDefault() {
  return &defaultFont;
}

void FontManager::setCurrent(const Font* f) {
  currentFont = f ? f : &defaultFont;
}

const Font* FontManager::getCurrent() {
  return currentFont ? currentFont : &defaultFont;
}

const FontResolution* FontManager::selectResolution(const Font* f, int screenWidth, int screenHeight) {
  if (!f || f->resolutionCount == 0) return NULL;

  // Try to find largest font that fits
  const FontResolution* best = NULL;
  for (int i = f->resolutionCount - 1; i >= 0; i--) {
    const FontResolution* res = &f->resolutions[i];
    if (screenWidth >= textCols * res->charWidth && screenHeight >= textRows * res->charHeight) {
      best = res;
      break;
    }
  }

  // If no font fits, use smallest
  if (!best) {
    best = &f->resolutions[0];
  }

  return best;
}

Font* FontManager::load(const char* path) {
  char lineBuffer[1024];

  FILE* fp = fopen(path, "r");
  if (fp == NULL) return NULL;

  Font* f = (Font*)calloc(1, sizeof(Font));
  if (!f) {
    fclose(fp);
    return NULL;
  }

  int resIdx = -1;
  int charIdx = 0;
  uint8_t* currentData = NULL;
  int bytesPerChar = 0;

  while (fgets(lineBuffer, sizeof(lineBuffer), fp) != NULL) {
    char* line = lineBuffer;
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
      strncpy(f->name, p, 15);
      f->name[15] = '\0';
      continue;
    }

    // Parse resolution
    if (strncmp(p, "resolution:", 11) == 0) {
      if (resIdx >= 0 && currentData) {
        f->resolutions[resIdx].data = currentData;
      }
      resIdx++;
      if (resIdx >= 5) break;

      p += 11;
      int w, h;
      if (sscanf(p, "%dx%d", &w, &h) == 2) {
        f->resolutions[resIdx].charWidth = w;
        f->resolutions[resIdx].charHeight = h;
        bytesPerChar = ((w + 7) / 8) * h;
        currentData = (uint8_t*)malloc(95 * bytesPerChar);
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
    f->resolutions[resIdx].data = currentData;
  }

  f->resolutionCount = resIdx + 1;
  fclose(fp);

  if (f->resolutionCount == 0) {
    free(f);
    return NULL;
  }

  return f;
}

void FontManager::freeFont(Font* f) {
  if (!f || f == &defaultFont) return;

  for (int i = 0; i < f->resolutionCount; i++) {
    free((void*)f->resolutions[i].data);
  }
  free(f);
}
