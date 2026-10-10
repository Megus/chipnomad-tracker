#include "screen.h"

ColorTheme& Screen::theme() {
  return state.settings.colorTheme;
}

Rect Screen::getSelectionRange() {
  using namespace chipnomad;
  if (!supportsSelectMode() || mode != ScreenMode::select) return {0, 0, 0, 0};

  int x = min(selectStartCol, cursorCol);
  int y = min(selectStartRow, cursorRow);

  return {
    .x = x,
    .y = y,
    .w = max(selectStartCol, cursorCol) - x + 1,
    .h = max(selectStartRow, cursorRow) - y + 1,
  };
}

void Screen::drawSelectionBorder(Rect range, bool draw) {
  Rect r = getSelectionRect(range);
  if (r.x < 0 || r.y < 0 || r.w <= 0 || r.h <= 0) return; // Rect validation

  gfx.setFgColor(draw ? theme().selection : theme().background);
  gfx.rect(r);
}

bool Screen::isSingleColumnSelection() {
  if (!supportsSelectMode() || mode != ScreenMode::select) return false;
  return selectStartCol == cursorCol;
}

void Screen::validateCursorPosition() {
  // Validate row number
  int rowCount = getRowCount();
  if (cursorRow >= rowCount) {
    cursorRow = rowCount - 1;
  } else if (cursorRow < 0) {
    cursorRow = 0;
  }

  // Validate column number for the current row
  int colCount = getColumnCount(cursorRow);
  if (cursorCol >= colCount) {
    cursorCol = colCount - 1;
  } else if (cursorCol < 0) {
    cursorCol = 0;
  }

  // Validate cell. If it's invalid, jump back to the top left
  if (!isValidCell(cursorCol, cursorRow)) {
    cursorCol = 0;
    cursorRow = 0;
  }
}

void Screen::setCellColor(CellState state, int isEmpty, int hasContent) {
  const ColorTheme& t = theme();
  int color = t.textInfo;

  if (state == CellState::selected) {
    if (isEmpty) {
      color = t.textEmpty;
    } else {
      color = t.selection;
    }
  } else if (state == CellState::focus) {
    color = t.textDefault;
  } else if (isEmpty) {
    color = t.textEmpty;
  } else if (hasContent) {
    color = t.textValue;
  }

  gfx.setFgColor(color);
}

void Screen::fullRedraw() {
  validateCursorPosition();

  gfx.setBgColor(theme().background);
  gfx.clearRect(0, 0, 35, 19); // Don't clear app-wide HUD area (message, screen map, playback preview)

  // Static content
  drawStatic();

  // Cells
  Rect selection = getSelectionRange();
  int rowCount = getRowCount();

  for (int row = 0; row < rowCount; row++) {
    if (!isRowVisible(row)) continue;

    drawRowHeader(row, (cursorRow == row) ? CellState::focus : CellState::normal);

    for (int col = 0; col < getColumnCount(row); col++) {
      CellState state = CellState::normal;
      if (mode == ScreenMode::select && col >= selection.x && col < selection.x + selection.w && row >= selection.y && row < selection.y + selection.h) {
        state = CellState::selected;
      } else if (cursorCol == col && cursorRow == row) {
        state = CellState::focus;
      }

      drawCell(col, row, state);
    }
  }

  // Column headers make sense only for spreadsheet-like screens, so get the number of columns of the first row
  int colCount = getColumnCount(0);
  for (int col = 0; col < colCount; col++) {
    drawColHeader(col, (cursorCol == col) ? CellState::focus : CellState::normal);
  }

  // Cursor/selection
  if (mode == ScreenMode::select) {
    drawSelectionBorder(selection, true);
  } else {
    drawCursor(cursorCol, cursorRow);
  }
}


void Screen::moveCursorToSelectionStart() {
  if (mode != ScreenMode::select) return;

  Rect r = getSelectionRange();
  cursorCol = r.x;
  cursorRow = r.y;
}

void Screen::moveCursorBelowSelection() {
  if (mode != ScreenMode::select) return;

  Rect r = getSelectionRange();
  cursorCol = r.x;
  // Move below selection, unless last row is in selection
  cursorRow = (r.y + r.h < getRowCount()) ? (r.y + r.h) : (getRowCount() - 1);
}

void Screen::drawSelectedCells() {
  if (mode != ScreenMode::select) return;

  Rect r = getSelectionRange();

  for (int y = r.y; y < r.y + r.h; y++) {
    if (!isRowVisible(y)) continue;
    for (int x = r.x; x < r.x + r.w; x++) {
      drawCell(x, y, CellState::selected);
    }
  }
}


bool Screen::commonInputHandler(InputEventData eventData) {
  bool isKeyDown = eventData.isKeyDown;
  int keys = eventData.keys;
  int tapCount = eventData.tapCount;

  if (!isKeyDown && keys != 0) return false; // Discard key up events unless no buttons are pressed (for existing logic that expects keys == 0)
  return (mode == ScreenMode::select) ? inputSelectMode(keys, tapCount) : inputEditMode(keys, tapCount);
}

void Screen::inputCursor(int keys, bool& handled, bool& needsRedraw) {
  if (keys == keyLeft) {
    if (cursorCol > 0) {
      cursorCol--;
      // If we landed on an invalid cell, move up until valid
      while (!isValidCell(cursorCol, cursorRow) && cursorRow > 0) {
        cursorRow--;
      }
    }
    handled = true;
  } else if (keys == keyRight) {
    if (cursorCol < getColumnCount(cursorRow) - 1) {
      cursorCol++;
      // If we landed on an invalid cell, move up until valid
      while (!isValidCell(cursorCol, cursorRow) && cursorRow > 0) {
        cursorRow--;
      }
    }
    handled = true;
  } else if (keys == keyUp) {
    int origRow = cursorRow;
    while (cursorRow > 0) {
      cursorRow--;
      int columns = getColumnCount(cursorRow);
      if (cursorCol >= columns) cursorCol = columns - 1;
      if (isValidCell(cursorCol, cursorRow)) break;
    }
    if (!isValidCell(cursorCol, cursorRow)) {
      cursorRow = origRow; // Can't move
    }
    handled = true;
  } else if (keys == keyDown) {
    int origRow = cursorRow;
    while (cursorRow < getRowCount() - 1) {
      cursorRow++;
      int columns = getColumnCount(cursorRow);
      if (cursorCol >= columns) cursorCol = columns - 1;
      if (isValidCell(cursorCol, cursorRow)) break;
    }
    if (!isValidCell(cursorCol, cursorRow)) {
      cursorRow = origRow; // Can't move
    }
    handled = true;
  }

  if (adjustVerticalScroll(false)) {
    // If cursor went beyond the screen, redraw the screen
    needsRedraw = true;
  }
}

bool Screen::inputEditMode(int keys, int tapCount) {
  int oldCursorCol = cursorCol;
  int oldCursorRow = cursorRow;
  bool handled = false;
  bool needsRedraw = false;

  inputCursor(keys, handled, needsRedraw);

  if (!handled) {
    if (keys == (keyShift | keyOpt) && supportsSelectMode() && mode == ScreenMode::edit) {
      // Enter select mode
      mode == ScreenMode::select;
      selectStartRow = cursorRow;
      selectStartCol = cursorCol;
      selectAnchorRow = cursorRow;
      selectAnchorCol = cursorCol;
      handled = true;
      needsRedraw = true;
    } else if (keys == (keyDown | keyOpt)) {
      // Page down
      if (cursorRow + 16 < getRowCount()) {
        cursorRow += 16;
        adjustVerticalScroll(true);
        int columns = getColumnCount(cursorRow);
        if (cursorCol >= columns) cursorCol = columns - 1;
        needsRedraw = true;
        handled = true;
      }
    } else if (keys == (keyUp | keyOpt)) {
      // Page up
      if (cursorRow - 16 >= 0) {
        cursorRow -= 16;
        adjustVerticalScroll(true);
        int columns = getColumnCount(cursorRow);
        if (cursorCol >= columns) cursorCol = columns - 1;
        needsRedraw = true;
        handled = true;
      }
    } else if (keys == keyEdit && tapCount == 1) {
      // Edit: insert/copy value
      handled = onEdit(cursorCol, cursorRow, CellEditAction::tap);
    } else if (keys == keyEdit && tapCount == 2) {
      // Edit: double tap (usually increment to an empty value)
      handled = onEdit(cursorCol, cursorRow, CellEditAction::doubleTap);
    } else if (keys == (keyRight | keyEdit)) {
      // Edit: value small increase (usually by 1)
      handled = onEdit(cursorCol, cursorRow, CellEditAction::increase);
    } else if (keys == (keyLeft | keyEdit)) {
      // Edit: value small decrease (usually by 1)
      handled = onEdit(cursorCol, cursorRow, CellEditAction::decrease);
    } else if (keys == (keyUp | keyEdit)) {
      // Edit: value big increase
      handled = onEdit(cursorCol, cursorRow, CellEditAction::increaseBig);
    } else if (keys == (keyDown | keyEdit)) {
      // Edit: value big decrease
      handled = onEdit(cursorCol, cursorRow, CellEditAction::decreaseBig);
    } else if (keys == (keyEdit | keyOpt)) {
      // Edit: clear value
      handled = onEdit(cursorCol, cursorRow, CellEditAction::clear);
    } else if (keys == (keyShift | keyEdit)) {
      // Edit: paste
      handled = onEdit(cursorCol, cursorRow, CellEditAction::paste);
    }
  }

  validateCursorPosition();

  if (needsRedraw) {
    fullRedraw();
  } else if (handled) {
    if (oldCursorCol != cursorCol || oldCursorRow != cursorRow) {
      // Erase old cursor and headers
      drawCell(oldCursorCol, oldCursorRow, CellState::normal);
      drawRowHeader(oldCursorRow, CellState::normal);
      drawColHeader(oldCursorCol, CellState::normal);
      // Draw new headers
      drawRowHeader(cursorRow, CellState::focus);
      drawColHeader(cursorCol, CellState::focus);
    }

    // Refresh field and cursor
    drawCell(cursorCol, cursorRow, CellState::focus);
    drawCursor(cursorCol, cursorRow);
  }

  return handled;
}


bool Screen::inputSelectMode(int keys, int tapCount) {
  int oldCursorCol = cursorCol;
  int oldCursorRow = cursorRow;
  bool handled = false;
  bool needsRedraw = false;

  inputCursor(keys, handled, needsRedraw);

  if (!handled) {
    if (keys == (keyShift | keyOpt)) {
      // Switch selection mode
      bool exitSelection = onEdit(cursorCol, cursorRow, CellEditAction::switchSelection);
      if (exitSelection) {
        // Exit selection mode
        mode = ScreenMode::edit;
        cursorRow = selectAnchorRow;
        cursorCol = selectAnchorCol;
      }
      needsRedraw = true;
      handled = true;
    } else if (keys == 0 && optPressed) {
      // Copy and exit select mode on Opt release (no keys pressed)
      handled = onEdit(cursorCol, cursorRow, CellEditAction::copy);
      if (handled) {
        showMessage(true, "Copied selection");
        moveCursorBelowSelection();
      }
      mode = ScreenMode::edit;
      needsRedraw = true;
      handled = true;
      optPressed = 0;
    } else if (keys == 0 && shallowClonePressed) {
      // Exit select mode on Shift release after shallow clone
      mode = ScreenMode::edit;
      needsRedraw = 1;
      handled = true;
      shallowClonePressed = 0;
    } else if (keys == (keyEdit | keyOpt)) {
      // Cut and exit select mode
      handled = onEdit(cursorCol, cursorRow, CellEditAction::cut);
      if (handled) {
        showMessage(true, "Cut selection");
        moveCursorToSelectionStart();
      }
      mode = ScreenMode::edit;
      needsRedraw = true;
      optPressed = 0;
    } else if (keys == (keyRight | keyEdit)) {
      // Multi-edit: increase values in selection
      handled = onEdit(cursorCol, cursorRow, CellEditAction::multiIncrease);
    } else if (keys == (keyLeft | keyEdit)) {
      // Multi-edit: decrease values in selection
      handled = onEdit(cursorCol, cursorRow, CellEditAction::multiDecrease);
    } else if (keys == (keyUp | keyEdit)) {
      // Multi-edit: big increase values in selection
      handled = onEdit(cursorCol, cursorRow, CellEditAction::multiIncreaseBig);
    } else if (keys == (keyDown | keyEdit)) {
      // Multi-edit: big decrease values in selection
      handled = onEdit(cursorCol, cursorRow, CellEditAction::multiDecreaseBig);
    } else if (keys == (keyShift | keyEdit) && !shallowClonePressed) {
      // Shallow clone
      handled = onEdit(cursorCol, cursorRow, CellEditAction::shallowClone);
      shallowClonePressed = 1;
    } else if (keys == (keyShift | keyEdit) && shallowClonePressed) {
      // Deep clone (second press while Shift still held)
      handled = onEdit(cursorCol, cursorRow, CellEditAction::deepClone);
      mode = ScreenMode::edit;
      shallowClonePressed = 0;
      needsRedraw = true;
    } else if (keys & keyOpt) {
      optPressed = 1;
    }
  }

  validateCursorPosition();

  if (needsRedraw) {
    fullRedraw();
  } else if (handled) {
    if (oldCursorCol != cursorCol || oldCursorRow != cursorRow) {
      // Calculate old and new selection bounds
      using namespace chipnomad;
      int oldSelCol1 = min(selectStartCol, oldCursorCol);
      int oldSelRow1 = min(selectStartRow, oldCursorRow);
      Rect oldSel = {oldSelCol1, oldSelRow1, max(selectStartCol, oldCursorCol) - oldSelCol1 + 1, max(selectStartRow, oldCursorRow) - oldSelRow1 + 1};
      Rect newSel = getSelectionRange();

      // Erase old selection rectangle
      drawSelectionBorder(oldSel, false);

      // Re-render cells that are no longer selected
      for (int row = oldSel.y; row < oldSel.y + oldSel.h; row++) {
        if (!isRowVisible(row)) continue;
        for (int col = oldSel.x; col < oldSel.x + oldSel.w; col++) {
          if (!(col >= newSel.x && col < newSel.x + newSel.w && row >= newSel.y && row < newSel.y + newSel.h)) {
            CellState state = (col == cursorCol && row == cursorRow) ? CellState::focus : CellState::normal;
            drawCell(col, row, state);
          }
        }
      }

      // Render cells that are now selected
      for (int row = newSel.y; row < newSel.y + newSel.h; row++) {
        if (!isRowVisible(row)) continue;
        for (int col = newSel.x; col < newSel.x + newSel.w; col++) {
          if (!(col >= oldSel.x && col < oldSel.x + oldSel.w && row >= oldSel.y && row <= oldSel.y + oldSel.h)) {
            drawCell(col, row, CellState::selected);
          }
        }
      }
    } else {
      // Cursor didn't move, redraw selection for multi-edit/shallow copy
      drawSelectedCells();
    }
    // Draw selection rectangle
    Rect range = getSelectionRange();
    drawSelectionBorder(range, true);
  }

  return handled;
}

void Screen::showMessage(bool timed, const char* format, ...) {
  static char messageBuffer[41];

  va_list args;
  va_start(args, format);
  vsnprintf(messageBuffer, 41, format, args);
  va_end(args);

  events.dispatch({.type = EventType::showMessage, .stringValue = std::string(messageBuffer)});
}