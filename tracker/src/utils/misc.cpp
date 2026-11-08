#include <string.h>
#include "misc.h"

const char* getEnvelopeShapeASCII(int shape) {
  shape = shape & 0x0F;
  if (shape == 0) return "    ";
  if (shape <= 3 || shape == 9) return "|\\__";
  if ((shape >= 4 && shape <= 7) || shape == 15) return "/|__";
  if (shape == 8) return "|\\|\\";
  if (shape == 10) return "\\/\\/";
  if (shape == 11) return "\\|^^";
  if (shape == 12) return "/|/|";
  if (shape == 13) return "/^^^";
  if (shape == 14) return "/\\/\\";
  return "    ";
}

// Trim leading and trailing spaces from string
void trimString(char* str) {
  if (!str || !str[0]) return;

  // Find first non-space character
  int start = 0;
  while (str[start] == ' ') {
    start++;
  }

  // Find last non-space character
  int end = strlen(str) - 1;
  while (end >= start && str[end] == ' ') {
    end--;
  }

  // If the entire string is spaces, empty it
  if (end < start) {
    str[0] = 0;
    return;
  }

  // Shift string to remove leading spaces
  if (start > 0) {
    int len = end - start + 1;
    memmove(str, str + start, len);
    str[len] = 0;
  } else {
    // Just trim trailing spaces
    str[end + 1] = 0;
  }
}
