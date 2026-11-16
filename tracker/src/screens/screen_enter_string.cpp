#include <string.h>
#include "misc.h"
#include "screen_enter_string.h"

void ScreenEnterString::setup(const ScreenData& input) {
  maxLength = input.maxLength;
  string = input.str;
  copyString(title, input.title, sizeof(title) - 1);
  copyString(prompt, input.prompt, sizeof(prompt) - 1);
  if (input.detail && strlen(input.detail) > 0) {
    copyString(detail, input.detail, sizeof(detail) - 1);
  } else {
    detail[0] = 0;
  }
  cursorRow = 0;
  cursorCol = 0;
}

int ScreenEnterString::getColumnCount(int row) {
  if (row == 0) return maxLength;
  return 2; // OK, Cancel buttons
}

void ScreenEnterString::drawStatic() {
  gfx.setFgColor(theme().textTitles);
  gfx.print(0, 0, title);

  // TODO: Draw detail string too
  //gfx.setFgColor(theme().textDefault);
  //gfx.print(0, 1, detail);

  gfx.setFgColor(theme().textDefault);
  gfx.print(0, 3, prompt);
}

void ScreenEnterString::drawCell(int col, int row, CellState state) {
  if (row == 0) {
    gfx.setFgColor(theme().textValue);
    gfx.clearRect(0, 4, 35, 1);
    gfx.print(0, 4, string);
  } else if (row == 1) {
    gfx.setFgColor(state == CellState::focus ? theme().textValue : theme().textDefault);
    if (col == 0) {
      gfx.print(0, 6, "OK");  // Buttons moved down one line
    } else {
      gfx.print(4, 6, "Cancel");
    }
  }
}

void ScreenEnterString::drawCursor(int col, int row) {
  if (row == 0) {
    gfx.cursor(col, 4, 1); // Text field
  } else if (row == 1) {
    if (col == 0) {
      gfx.cursor(0, 6, 2); // OK
    } else {
      gfx.cursor(4, 6, 6); // Cancel
    }
  }
}

bool ScreenEnterString::onEdit(int col, int row, EditCellAction action) {
  if (row == 0) {
    return enterCharacter(action, string, col, maxLength);
  } else if (row == 1) {
    int result = 0; // "Cancel" by default
    if (col == 0 && strlen(string) > 0) {
      result = 1;
    }
    events.dispatch(Event::popScreen(ScreenEnum::enterString, ScreenData::enterStringOutput(result, result ? string : nullptr)));
    return true;
  }
  return false;
}

bool ScreenEnterString::onEvent(const Event& e) {
  if (commonEventHandler(e)) return true;
  if (e.type == EventType::screenOutput && e.screen == ScreenEnum::enterCharacter) {
    if (cursorCol < getColumnCount(cursorRow) - 1) cursorCol++;
    return true;
  }
  return false;
}
