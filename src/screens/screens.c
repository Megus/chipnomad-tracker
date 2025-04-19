#include <stdarg.h>
#include <screens.h>
#include <project.h>
#include <corelib_gfx.h>

const struct AppScreen* currentScreen = NULL;

void drawScreenMap() {
  const static int smY = 15;

  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetBgColor(cs.background);
  gfxSetFgColor(cs.textInfo);
  gfxClearRect(35, smY, 5, 4);

  // Core screens
  gfxPrint(35, smY + 1, "SCPIT");

  // Additional screens
  if (currentScreen == &screenSong || currentScreen == &screenProject) {
    gfxPrint(35, smY, "P");
  } else if (currentScreen == &screenPhrase || currentScreen == &screenGroove) {
    gfxPrint(37, smY, "G");
  }

  // Highlight current screen
  gfxSetFgColor(cs.textDefault);
  if (currentScreen == &screenSong) {
    gfxPrint(35, smY + 1, "S");
  } else if (currentScreen == &screenChain) {
    gfxPrint(36, smY + 1, "C");
  } else if (currentScreen == &screenPhrase) {
    gfxPrint(37, smY + 1, "P");
  } else if (currentScreen == &screenInstrument) {
    gfxPrint(38, smY + 1, "I");
  } else if (currentScreen == &screenTable) {
    gfxPrint(39, smY + 1, "T");
  } else if (currentScreen == &screenProject) {
    gfxPrint(35, smY, "P");
  } else if (currentScreen == &screenGroove) {
    gfxPrint(37, smY, "G");
  }
}

void screenSetup(const struct AppScreen* screen, int input) {
  currentScreen = screen;
  currentScreen->setup(input);
  gfxSetBgColor(appSettings.colorScheme.background);
  gfxSetSelectionColor(appSettings.colorScheme.selection);
  gfxSetCursorColor(appSettings.colorScheme.cursor);
  gfxClearRect(0, 0, 40, 20);
  currentScreen->fullRedraw();
  drawScreenMap();
}

void screenMessage(const char* format, ...) {
  static char buffer[42];
  gfxSetFgColor(appSettings.colorScheme.textDefault);
  gfxClearRect(0, 19, 40, 1);

  va_list args;
  va_start(args, format);
  vsnprintf(buffer, 41, format, args);
  va_end(args);
  gfxPrint(0, 19, buffer);
}


///////////////////////////////////////////////////////////////////////////////
//
// Spreadsheet screen functions
//

void spreadsheetFullRedraw(struct SpreadsheetScreenData* sheet) {
  // Cells
  for (int row = sheet->topRow; row < sheet->topRow + 16; row++) {
    for (int col = 0; col < sheet->cols; col++) {
      sheet->drawCell(col, row, (sheet->cursorCol == col && sheet->cursorRow == row) ? stateFocus : 0);
    }
  }

  // Headers
  for (int row = sheet->topRow; row < sheet->topRow + 16; row++) {
    sheet->drawRowHeader(row, (sheet->cursorRow == row) ? stateFocus : 0);
  }

  for (int col = 0; col < sheet->cols; col++) {
    sheet->drawColHeader(col, (sheet->cursorCol == col) ? stateFocus : 0);
  }

  // Cursor
  sheet->drawCursor(sheet->cursorCol, sheet->cursorRow);
}

static int spreadsheetInputCursor(struct SpreadsheetScreenData* sheet, int keys, int isDoubleTap) {
  int oldCursorCol = sheet->cursorCol;
  int oldCursorRow = sheet->cursorRow;
  int handled = 0;
  int redrawn = 0;

  if (keys == keyLeft) {
    if (sheet->cursorCol > 0) sheet->cursorCol--;
    handled = 1;
  } else if (keys == keyRight) {
    if (sheet->cursorCol < sheet->cols - 1) sheet->cursorCol++;
    handled = 1;
  } else if (keys == keyUp) {
    if (sheet->cursorRow > 0) sheet->cursorRow--;
    if (sheet->cursorRow < sheet->topRow) {
      sheet->topRow--;
      spreadsheetFullRedraw(sheet);
      redrawn = 1;
    }
    handled = 1;
  } else if (keys == keyDown) {
    if (sheet->cursorRow < sheet->rows - 1) sheet->cursorRow++;
    if (sheet->cursorRow >= sheet->topRow + 16) {
      sheet->topRow++;
      spreadsheetFullRedraw(sheet);
      redrawn = 1;
    }
    handled = 1;
  } else if (keys == (keyDown | keyOpt)) {
    if (sheet->cursorRow + 16 < sheet->rows) {
      sheet->cursorRow += 16;
      sheet->topRow += 16;
      if (sheet->topRow + 16 >= sheet->rows) sheet->topRow = sheet->rows - 16;
      spreadsheetFullRedraw(sheet);
      redrawn = 1;
      handled = 1;
    }
  } else if (keys == (keyUp | keyOpt)) {
    if (sheet->cursorRow - 16 >= 0) {
      sheet->cursorRow -= 16;
      sheet->topRow -= 16;
      if (sheet->topRow < 0) sheet->topRow = 0;
      spreadsheetFullRedraw(sheet);
      redrawn = 1;
      handled = 1;
    }
  }

  if (handled && !redrawn) {
    // Erase old cursor
    sheet->drawCell(oldCursorCol, oldCursorRow, 0);
    sheet->drawRowHeader(oldCursorRow, 0);
    sheet->drawColHeader(oldCursorCol, 0);
    // Draw new cursor
    sheet->drawCell(sheet->cursorCol, sheet->cursorRow, stateFocus);
    sheet->drawRowHeader(sheet->cursorRow, stateFocus);
    sheet->drawColHeader(sheet->cursorCol, stateFocus);
    sheet->drawCursor(sheet->cursorCol, sheet->cursorRow);
  }
  return handled;
}

static int spreadsheetInputEdit(struct SpreadsheetScreenData* sheet, int keys, int isDoubleTap) {
  int handled = 0;

  if (keys == keyEdit && isDoubleTap == 0) {
    handled = sheet->onEdit(sheet->cursorCol, sheet->cursorRow, editTap);
  } else if (keys == keyEdit && isDoubleTap == 1) {
    handled = sheet->onEdit(sheet->cursorCol, sheet->cursorRow, editDoubleTap);
  } else if (keys == (keyRight | keyEdit)) {
    handled = sheet->onEdit(sheet->cursorCol, sheet->cursorRow, editIncrease);
  } else if (keys == (keyLeft | keyEdit)) {
    handled = sheet->onEdit(sheet->cursorCol, sheet->cursorRow, editDecrease);
  } else if (keys == (keyUp | keyEdit)) {
    handled = sheet->onEdit(sheet->cursorCol, sheet->cursorRow, editIncreaseBig);
  } else if (keys == (keyDown | keyEdit)) {
    handled = sheet->onEdit(sheet->cursorCol, sheet->cursorRow, editDecreaseBig);
  } else if (keys == (keyEdit | keyOpt)) {
    handled = sheet->onEdit(sheet->cursorCol, sheet->cursorRow, editClear);
  }

  if (handled) {
    sheet->drawCell(sheet->cursorCol, sheet->cursorRow, stateFocus);
    sheet->drawCursor(sheet->cursorCol, sheet->cursorRow);
  }
  return handled;
}

int spreadsheetInput(struct SpreadsheetScreenData* sheet, int keys, int isDoubleTap) {
  if (spreadsheetInputCursor(sheet, keys, isDoubleTap)) return 1;
  return spreadsheetInputEdit(sheet, keys, isDoubleTap);
}


// Utility functions

void setCellColor(int state, int isEmpty, int hasContent) {
  const struct ColorScheme cs = appSettings.colorScheme;

  if (state & stateFocus) {
    gfxSetFgColor(cs.textDefault);
  } else if (isEmpty) {
    gfxSetFgColor(cs.textEmpty);
  } else if (hasContent) {
    gfxSetFgColor(cs.textValue);
  } else {
    gfxSetFgColor(cs.textInfo);
  }
}