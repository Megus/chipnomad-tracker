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
