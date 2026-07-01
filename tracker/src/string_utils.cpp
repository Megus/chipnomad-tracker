#include "string_utils.h"
#include <string.h>

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
