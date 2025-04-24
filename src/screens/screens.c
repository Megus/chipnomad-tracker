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
  projectSave(AUTOSAVE_FILENAME); // Temporary measure against random crashes

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

void screenFullRedraw(struct ScreenData* screen) {
  // Static content
  screen->drawStatic();

  // Cells
  for (int row = screen->topRow; row < screen->topRow + 16; row++) {
    for (int col = 0; col < screen->getColumnCount(row); col++) {
      screen->drawField(col, row, (screen->cursorCol == col && screen->cursorRow == row) ? stateFocus : 0);
    }
  }

  // Row headers
  for (int row = screen->topRow; row < screen->topRow + 16; row++) {
    screen->drawRowHeader(row, (screen->cursorRow == row) ? stateFocus : 0);
  }

  // Column headers make sense only for spreadsheet-like screens, so we get the number of columns of the first row
  for (int col = 0; col < screen->getColumnCount(0); col++) {
    screen->drawColHeader(col, (screen->cursorCol == col) ? stateFocus : 0);
  }

  // Cursor
  screen->drawCursor(screen->cursorCol, screen->cursorRow);
}

static int spreadsheetInputCursor(struct ScreenData* screen, int keys, int isDoubleTap) {
  int oldCursorCol = screen->cursorCol;
  int oldCursorRow = screen->cursorRow;
  int handled = 0;
  int redrawn = 0;

  if (keys == keyLeft) {
    if (screen->cursorCol > 0) screen->cursorCol--;
    handled = 1;
  } else if (keys == keyRight) {
    if (screen->cursorCol < screen->getColumnCount(screen->cursorRow) - 1) screen->cursorCol++;
    handled = 1;
  } else if (keys == keyUp) {
    if (screen->cursorRow > 0) screen->cursorRow--;
    if (screen->cursorRow < screen->topRow) {
      screen->topRow--;
      screenFullRedraw(screen);
      redrawn = 1;
    }
    int columns = screen->getColumnCount(screen->cursorRow);
    if (screen->cursorCol >= columns) screen->cursorCol = columns - 1;
    handled = 1;
  } else if (keys == keyDown) {
    if (screen->cursorRow < screen->rows - 1) screen->cursorRow++;
    if (screen->cursorRow >= screen->topRow + 16) {
      screen->topRow++;
      screenFullRedraw(screen);
      redrawn = 1;
    }
    int columns = screen->getColumnCount(screen->cursorRow);
    if (screen->cursorCol >= columns) screen->cursorCol = columns - 1;
    handled = 1;
  } else if (keys == (keyDown | keyOpt)) {
    if (screen->cursorRow + 16 < screen->rows) {
      screen->cursorRow += 16;
      screen->topRow += 16;
      if (screen->topRow + 16 >= screen->rows) screen->topRow = screen->rows - 16;
      screenFullRedraw(screen);
      int columns = screen->getColumnCount(screen->cursorRow);
      if (screen->cursorCol >= columns) screen->cursorCol = columns - 1;
      redrawn = 1;
      handled = 1;
    }
  } else if (keys == (keyUp | keyOpt)) {
    if (screen->cursorRow - 16 >= 0) {
      screen->cursorRow -= 16;
      screen->topRow -= 16;
      if (screen->topRow < 0) screen->topRow = 0;
      screenFullRedraw(screen);
      int columns = screen->getColumnCount(screen->cursorRow);
      if (screen->cursorCol >= columns) screen->cursorCol = columns - 1;
      redrawn = 1;
      handled = 1;
    }
  }

  if (handled && !redrawn) {
    // Erase old cursor
    screen->drawField(oldCursorCol, oldCursorRow, 0);
    screen->drawRowHeader(oldCursorRow, 0);
    screen->drawColHeader(oldCursorCol, 0);
    // Draw new cursor
    screen->drawField(screen->cursorCol, screen->cursorRow, stateFocus);
    screen->drawRowHeader(screen->cursorRow, stateFocus);
    screen->drawColHeader(screen->cursorCol, stateFocus);
    screen->drawCursor(screen->cursorCol, screen->cursorRow);
  }
  return handled;
}

static int spreadsheetInputEdit(struct ScreenData* screen, int keys, int isDoubleTap) {
  int handled = 0;

  if (keys == keyEdit && isDoubleTap == 0) {
    handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editTap);
  } else if (keys == keyEdit && isDoubleTap == 1) {
    handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editDoubleTap);
  } else if (keys == (keyRight | keyEdit)) {
    handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editIncrease);
  } else if (keys == (keyLeft | keyEdit)) {
    handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editDecrease);
  } else if (keys == (keyUp | keyEdit)) {
    handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editIncreaseBig);
  } else if (keys == (keyDown | keyEdit)) {
    handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editDecreaseBig);
  } else if (keys == (keyEdit | keyOpt)) {
    handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editClear);
  }

  if (handled) {
    screen->drawField(screen->cursorCol, screen->cursorRow, stateFocus);
    screen->drawCursor(screen->cursorCol, screen->cursorRow);
  }
  return handled;
}

int screenInput(struct ScreenData* screen, int keys, int isDoubleTap) {
  if (spreadsheetInputCursor(screen, keys, isDoubleTap)) return 1;
  return spreadsheetInputEdit(screen, keys, isDoubleTap);
}


///////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//

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