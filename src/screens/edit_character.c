#include <screens.h>
#include <corelib_gfx.h>
#include <string.h>

// Character keyboard layout
static const char* keyboardRows[] = {
  "1234567890",
  "QWERTYUIOP",
  "ASDFGHJKL-",
  "ZXCVBNM !_",
};

#define NUM_KEYBOARD_ROWS 4

static int currentRow = 1;  // Start at the QWERTY row
static int currentCol = 0;
static int prevRow = 1;     // Track previous cursor position
static int prevCol = 0;
static int lastSelectedChar = 0;

void updateCursorPosition();
void charEditFullDraw(char startChar);

// Trim empty characters from the end of the string
void trimStr(char* str) {
  int idx = strlen(str) - 1;
  while (idx >= 0 && str[idx] == ' ') {
    str[idx--] = 0;
  }
}

int editCharacter(enum CellEditAction action, char* str, int idx, int maxLen) {
  if (action == editClear) {
    str[idx] = ' ';
    trimStr(str);
    return 2;
  } else if (action == editTap) {
    char current = str[idx];
    if (idx  >= strlen(str)) current = -1;
    charEditFullDraw(current);
    return 1;
  }
  return 0;
}

/**
* @brief Handle input for character selection screen
*
* @param keys Currently pressed keys
* @param isDoubleTap Is it a double tap?
* @return char Selected character (0 if no selection made)
*/
char charEditInput(int keys, int isDoubleTap, char* str, int idx, int maxLen) {
  // If no keys are pressed, return the currently selected character
  if (keys == 0) {
    char selectedChar = 0;

    // Get the character at the current position
    if (currentRow >= 0 && currentRow < NUM_KEYBOARD_ROWS) {
      const char* row = keyboardRows[currentRow];
      int rowLen = strlen(row);

      if (currentCol >= 0 && currentCol < rowLen) {
        selectedChar = row[currentCol];
        lastSelectedChar = selectedChar;
      }
    }

    // Safely insert the character to the string
    str[maxLen] = 0; // EOL for safety
    if (idx >= strlen(str)) {
      str[idx + 1] = 0; // Terminate string
      // Extend string with spaces to this character
      for (int i = strlen(str); i <= idx; i++) {
        str[i] = ' ';
      }
    }
    str[idx] = selectedChar;

    return selectedChar;
  }

  // Navigation with d-pad while holding Edit button
  if (keys & keyEdit) {
    if (keys & keyUp) {
      // Move up a row
      currentRow--;
      if (currentRow < 0) {
        currentRow = NUM_KEYBOARD_ROWS - 1;
      }

      // Adjust column if needed
      const char* row = keyboardRows[currentRow];
      int rowLen = strlen(row);
      if (currentCol >= rowLen) {
        currentCol = rowLen - 1;
      }

      // Update cursor position without full redraw
      updateCursorPosition();
      return 0;
    }
    else if (keys & keyDown) {
      // Move down a row
      currentRow++;
      if (currentRow >= NUM_KEYBOARD_ROWS) {
        currentRow = 0;
      }

      // Adjust column if needed
      const char* row = keyboardRows[currentRow];
      int rowLen = strlen(row);
      if (currentCol >= rowLen) {
        currentCol = rowLen - 1;
      }

      // Update cursor position without full redraw
      updateCursorPosition();
      return 0;
    }
    else if (keys & keyLeft) {
      // Move left in the current row
      const char* row = keyboardRows[currentRow];
      int rowLen = strlen(row);

      currentCol--;
      if (currentCol < 0) {
        currentCol = rowLen - 1;
      }

      // Update cursor position without full redraw
      updateCursorPosition();
      return 0;
    }
    else if (keys & keyRight) {
      // Move right in the current row
      const char* row = keyboardRows[currentRow];
      int rowLen = strlen(row);

      currentCol++;
      if (currentCol >= rowLen) {
        currentCol = 0;
      }

      // Update cursor position without full redraw
      updateCursorPosition();
      return 0;
    }
  }

  // No character selected yet
  return 0;
}

/**
* @brief Update the cursor position without redrawing the entire screen
*/
void updateCursorPosition() {
  // Calculate positions for previous and current cursor
  const char* prevRow_str = keyboardRows[prevRow];
  const char* currRow_str = keyboardRows[currentRow];
  int prevRowLen = strlen(prevRow_str);
  int currRowLen = strlen(currRow_str);

  // Calculate x positions (with spacing between characters)
  int prevXPos = (40 - (prevRowLen * 2 - 1)) / 2 + (prevCol * 2); // Double width for spacing
  int currXPos = (40 - (currRowLen * 2 - 1)) / 2 + (currentCol * 2); // Double width for spacing

  // Clear previous cursor position and redraw character
  char prevCharStr[2] = {prevRow_str[prevCol], 0};
  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(prevXPos, 6 + prevRow, prevCharStr);

  // Draw current character with cursor
  char currCharStr[2] = {currRow_str[currentCol], 0};
  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxPrint(currXPos, 6 + currentRow, currCharStr);
  gfxCursor(currXPos, 6 + currentRow, 1);

  // Save current position as previous for next update
  prevRow = currentRow;
  prevCol = currentCol;
}

/**
* @brief Draw the character selection screen
*
* @param startChar Initial character to select
*/
void charEditFullDraw(char startChar) {
  gfxClearRect(0, 0, 40, 20);

  // Clear the screen
  gfxSetBgColor(appSettings.colorScheme.background);
  gfxClear();

  // If a valid startChar is provided, try to select it
  if (startChar != 0) {
    // Try to find the character in the keyboard layout
    for (int row = 0; row < NUM_KEYBOARD_ROWS; row++) {
      const char* keyRow = keyboardRows[row];
      char* pos = strchr(keyRow, startChar);

      if (pos != NULL) {
        // Found the character, set the current position
        currentRow = row;
        currentCol = pos - keyRow;
        prevRow = currentRow;
        prevCol = currentCol;
        break;
      }
    }
  }

  // Draw keyboard layout
  int yPos = 6;
  for (int row = 0; row < NUM_KEYBOARD_ROWS; row++) {
    const char* keyRow = keyboardRows[row];
    int rowLen = strlen(keyRow);

    // Center the row (accounting for spaces between characters)
    int xPos = (40 - (rowLen * 2 - 1)) / 2; // Double width minus 1 for spacing

    // Draw each character in the row with spacing
    for (int col = 0; col < rowLen; col++) {
      char charStr[2] = {keyRow[col], 0};
      int charXPos = xPos + (col * 2); // Double the column position for spacing

      gfxSetFgColor(appSettings.colorScheme.textDefault);
      gfxPrint(charXPos, yPos, charStr);

      // Highlight the current selection with cursor
      if (row == currentRow && col == currentCol) {
        gfxCursor(charXPos, yPos, 1);
      }
    }

    yPos++;
  }
}
