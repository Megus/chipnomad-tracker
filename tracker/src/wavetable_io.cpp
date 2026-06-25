#include "wavetable_io.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Helper function to convert hex character to value
static int hexCharToValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;  // Invalid hex character
}

int wavetableSave(const char* path, uint8_t wavetables[256][32], int startIndex, int count) {
  if (!path || !wavetables) return 0;
  if (startIndex < 0 || startIndex > 255) return 0;
  if (count < 1 || count > 256) return 0;

  FILE* file = fopen(path, "w");
  if (!file) return 0;

  // Calculate actual number of wavetables to save (don't go past index 255)
  int endIndex = startIndex + count;
  if (endIndex > 256) endIndex = 256;
  int actualCount = endIndex - startIndex;

  // Write each wavetable as a line of 32 hex digits
  for (int i = 0; i < actualCount; i++) {
    int wavetableIdx = startIndex + i;
    for (int j = 0; j < 32; j++) {
      uint8_t value = wavetables[wavetableIdx][j] & 0x0F;  // Ensure 4-bit value
      fprintf(file, "%X", value);
    }
    fprintf(file, "\n");
  }

  fclose(file);
  return 1;
}

int wavetableLoad(const char* path, uint8_t wavetables[256][32], int startIndex) {
  if (!path || !wavetables) return 0;
  if (startIndex < 0 || startIndex > 255) return 0;

  FILE* file = fopen(path, "r");
  if (!file) return 0;

  char line[256];
  int wavetablesLoaded = 0;
  int currentIndex = startIndex;

  while (fgets(line, sizeof(line), file) && currentIndex < 256) {
    // Remove newline characters
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
      line[--len] = '\0';
    }

    // Skip empty lines
    if (len == 0) continue;

    // Skip comment lines (starting with '#')
    if (line[0] == '#') continue;

    // Validate line format: must be exactly 32 hex digits
    if (len != 32) {
      fclose(file);
      return 0;  // Invalid format
    }

    // Parse 32 hex digits
    int valid = 1;
    uint8_t tempWavetable[32];

    for (int i = 0; i < 32; i++) {
      int value = hexCharToValue(line[i]);
      if (value < 0 || value > 15) {
        valid = 0;
        break;
      }
      tempWavetable[i] = (uint8_t)value;
    }

    if (!valid) {
      fclose(file);
      return 0;  // Invalid hex digit found
    }

    // Copy valid wavetable to the array
    for (int i = 0; i < 32; i++) {
      wavetables[currentIndex][i] = tempWavetable[i];
    }

    wavetablesLoaded++;
    currentIndex++;
  }

  fclose(file);
  return wavetablesLoaded;
}
