#include <string.h>
#include "gfx.h"
#include "misc.h"
#include "screen_edit_character.h"

// Character keyboard layout
static const char* keyboardRowsUpper[] = {
  "1234567890",
  "QWERTYUIOP",
  "ASDFGHJKL-",
  "ZXCVBNM !_",
};

static const char* keyboardRowsLower[] = {
  "1234567890",
  "qwertyuiop",
  "asdfghjkl-",
  "zxcvbnm !_",
};

void ScreenEditCharacter::setup(const ScreenData& input) {
  str = input.str;
  charIdx = input.charIdx;
  maxLength = input.maxLength;

  // Default cursor position: QWERTY row
  cursorRow = 1;
  cursorCol = 0;

  // Sanitize the string
  for (int c = strlen(str); c <= maxLength; c++) str[c] = 0;

  // Try to find the cursor position for a character
  char startChar = str[charIdx];

  if (startChar != 0 && startChar != ' ') {
    for (int row = 0; row < getRowCount(); row++) {
      const char* pos = strchr(keyboardRowsUpper[row], startChar);
      if (pos != NULL) {
        isUppercase = 1;
        cursorRow = row;
        cursorCol = pos - keyboardRowsUpper[row];
        break;
      }

      pos = strchr(keyboardRowsLower[row], startChar);
      if (pos != NULL) {
        isUppercase = 0;
        cursorRow = row;
        cursorCol = pos - keyboardRowsLower[row];
        break;
      }
    }
  }
}

void ScreenEditCharacter::drawStatic() {
  gfx.setFgColor(theme().textInfo);
  gfx.print(12, 18, isUppercase ? "SHIFT: lowercase" : "SHIFT: uppercase");
}

void ScreenEditCharacter::drawCursor(int col, int row) {
  gfx.cursor(10 + col * 2, 6 + row, 1);
}

void ScreenEditCharacter::drawCell(int col, int row, CellState state) {
  char charStr[2] = {keyboardRows()[row][col], 0};

  setCellColor(state, false, true);
  gfx.print(10 + col * 2, 6 + row, charStr);
}

bool ScreenEditCharacter::onEvent(const Event& e) {
  if (e.type != EventType::input) return false;

  bool handled = false;

  if (!e.isKeyDown) shiftFlag = false;

  if (e.keys == 0) {
    // If all keys are released then update the string and close popup
    char selectedChar = keyboardRows()[cursorRow][cursorCol];

    // Safely insert the character to the string and trim it
    str[maxLength] = 0; // EOL for safety
    if (charIdx >= strlen(str)) {
      str[charIdx + 1] = 0; // Terminate string
      // Extend string with spaces to this character
      for (int i = strlen(str); i <= charIdx; i++) {
        str[i] = ' ';
      }
    }
    str[charIdx] = selectedChar;
    trimString(str);

    events.dispatch(Event::popScreen(ScreenEnum::editCharacter));
    handled = true;
  } else if ((e.keys & keyShift) && e.isKeyDown && !shiftFlag) {
    // Toggle between uppercase and lowercase
    isUppercase = !isUppercase;
    shiftFlag = true;
    events.dispatch(Event::redrawScreen());
    handled = true;
  } else if (e.keys & keyEdit) {
    // Cursor navigation
    int oldRow = cursorRow;
    int oldCol = cursorCol;

    if (e.keys & keyUp) {
      cursorRow--;
      if (cursorRow < 0) cursorRow = getRowCount() - 1;
      handled = true;
    }
    else if (e.keys & keyDown) {
      cursorRow++;
      if (cursorRow >= getRowCount()) cursorRow = 0;
      handled = true;
    }
    else if (e.keys & keyLeft) {
      cursorCol--;
      if (cursorCol < 0) cursorCol = getColumnCount(cursorRow) - 1;
      handled = true;
    }
    else if (e.keys & keyRight) {
      cursorCol++;
      if (cursorCol >= getColumnCount(cursorRow)) cursorCol = 0;
      handled = true;
    }

    if (handled) {
      drawCell(oldCol, oldRow, CellState::normal);
      drawCell(cursorCol, cursorRow, CellState::focus);
      drawCursor(cursorCol, cursorRow);
    }
  }

  return handled;
}

const char** ScreenEditCharacter::keyboardRows() {
  return isUppercase ? keyboardRowsUpper : keyboardRowsLower;
}
