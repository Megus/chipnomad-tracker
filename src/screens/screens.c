#include <stdarg.h>
#include <screens.h>
#include <project.h>
#include <corelib_gfx.h>
#include <utils.h>

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
  } else if (currentScreen == &screenInstrument || currentScreen == &screenInstrumentPool) {
    gfxPrint(38, smY + 2, "P");
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
  } else if (currentScreen == &screenInstrumentPool) {
    gfxPrint(38, smY + 2, "P");
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

static void screenDrawSelection(struct ScreenData* screen, int drawOrErase) {
  if (drawOrErase) {
    gfxSetFgColor(appSettings.colorScheme.selection);
  } else {
    gfxSetFgColor(appSettings.colorScheme.background);
  }

  screen->drawSelection(
    min(screen->selectStartCol, screen->cursorCol), min(screen->selectStartRow, screen->cursorRow),
    max(screen->selectStartCol, screen->cursorCol), max(screen->selectStartRow, screen->cursorRow)
  );
}

void screenFullRedraw(struct ScreenData* screen) {
  gfxClearRect(0, 0, 40, 20);

  // Static content
  screen->drawStatic();

  // Cells
  int selCol1 = 0, selCol2 = 0, selRow1 = 0, selRow2 = 0;
  if (screen->selectMode == 1) {
    selCol1 = min(screen->selectStartCol, screen->cursorCol);
    selCol2 = max(screen->selectStartCol, screen->cursorCol);
    selRow1 = min(screen->selectStartRow, screen->cursorRow);
    selRow2 = max(screen->selectStartRow, screen->cursorRow);
  }

  int maxRow = screen->topRow + 16;
  if (maxRow > screen->rows) maxRow = screen->rows;

  for (int row = screen->topRow; row < maxRow; row++) {
    for (int col = 0; col < screen->getColumnCount(row); col++) {
      int state = 0;
      if (screen->selectMode == 1 && col >= selCol1 && col <= selCol2 && row >= selRow1 && row <= selRow2) {
        state = stateSelected;
      } else if (screen->cursorCol == col && screen->cursorRow == row) {
        state = stateFocus;
      }

      screen->drawField(col, row, state);
    }
  }

  // Row headers
  for (int row = screen->topRow; row < maxRow; row++) {
    screen->drawRowHeader(row, (screen->cursorRow == row) ? stateFocus : 0);
  }

  // Column headers make sense only for spreadsheet-like screens, so we get the number of columns of the first row
  for (int col = 0; col < screen->getColumnCount(0); col++) {
    screen->drawColHeader(col, (screen->cursorCol == col) ? stateFocus : 0);
  }

  // Cursor/selection
  if (screen->selectMode == 1) {
    screenDrawSelection(screen, 1);
  } else {
    screen->drawCursor(screen->cursorCol, screen->cursorRow);
  }
}

void screenDrawOverlays(struct ScreenData* screen) {
  if (screen->selectMode == 1) {
    screenDrawSelection(screen, 1);
  }
}


// Cursor navigation within a spreadhseet-like page
static void inputCursorCommon(struct ScreenData* screen, int keys, int* handled, int* redrawn) {
  if (keys == keyLeft) {
    if (screen->cursorCol > 0) screen->cursorCol--;
    *handled = 1;
  } else if (keys == keyRight) {
    if (screen->cursorCol < screen->getColumnCount(screen->cursorRow) - 1) screen->cursorCol++;
    *handled = 1;
  } else if (keys == keyUp) {
    if (screen->cursorRow > 0) screen->cursorRow--;
    if (screen->cursorRow < screen->topRow) {
      screen->topRow--;
      screenFullRedraw(screen);
      *redrawn = 1;
    }
    int columns = screen->getColumnCount(screen->cursorRow);
    if (screen->cursorCol >= columns) screen->cursorCol = columns - 1;
    *handled = 1;
  } else if (keys == keyDown) {
    if (screen->cursorRow < screen->rows - 1) screen->cursorRow++;
    if (screen->cursorRow >= screen->topRow + 16) {
      screen->topRow++;
      screenFullRedraw(screen);
      *redrawn = 1;
    }
    int columns = screen->getColumnCount(screen->cursorRow);
    if (screen->cursorCol >= columns) screen->cursorCol = columns - 1;
    *handled = 1;
  }
}

static int inputNormalMode(struct ScreenData* screen, int keys, int isDoubleTap) {
  int oldCursorCol = screen->cursorCol;
  int oldCursorRow = screen->cursorRow;
  int handled = 0;
  int redrawn = 0;

  inputCursorCommon(screen, keys, &handled, &redrawn);

  if (!handled) {
    if (keys == (keyShift | keyOpt) && screen->selectMode == 0) {
      // Enter select mode
      screen->selectMode = 1;
      screen->selectStartRow = screen->cursorRow;
      screen->selectStartCol = screen->cursorCol;
      screenFullRedraw(screen);
      handled = 1;
      redrawn = 1;
    } else if (keys == (keyDown | keyOpt)) {
      // Page down
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
      // Page up
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
    } else if (keys == keyEdit && isDoubleTap == 0) {
      // Edit: insert/copy value
      handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editTap);
    } else if (keys == keyEdit && isDoubleTap == 1) {
      // Edit: double tap (usually increment to an empty value)
      handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editDoubleTap);
    } else if (keys == (keyRight | keyEdit)) {
      // Edit: value small increase (usually by 1)
      handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editIncrease);
    } else if (keys == (keyLeft | keyEdit)) {
      // Edit: value small decrease (usually by 1)
      handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editDecrease);
    } else if (keys == (keyUp | keyEdit)) {
      // Edit: value big increase
      handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editIncreaseBig);
    } else if (keys == (keyDown | keyEdit)) {
      // Edit: value big decrease
      handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editDecreaseBig);
    } else if (keys == (keyEdit | keyOpt)) {
      // Edit: clear value
      handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editClear);
    }
  }

  if (handled && !redrawn) {
    if (oldCursorCol != screen->cursorCol || oldCursorRow != screen->cursorRow) {
      // Erase old cursor and headers
      screen->drawField(oldCursorCol, oldCursorRow, 0);
      screen->drawRowHeader(oldCursorRow, 0);
      screen->drawColHeader(oldCursorCol, 0);
      // Draw new headers
      screen->drawRowHeader(screen->cursorRow, stateFocus);
      screen->drawColHeader(screen->cursorCol, stateFocus);
    }

    // Refresh field and cursor
    screen->drawField(screen->cursorCol, screen->cursorRow, stateFocus);
    screen->drawCursor(screen->cursorCol, screen->cursorRow);
  }

  return handled;
}

static int inputSelectMode(struct ScreenData* screen, int keys, int isDoubleTap) {
  int oldCursorCol = screen->cursorCol;
  int oldCursorRow = screen->cursorRow;
  int handled = 0;
  int redrawn = 0;

  inputCursorCommon(screen, keys, &handled, &redrawn);

  if (!handled) {
    if (keys == keyOpt) {
      // Exit select mode
      screen->selectMode = 0;
      screenFullRedraw(screen);
      redrawn = 1;
      handled = 1;
    } else if (keys == (keyShift | keyEdit)) {
      // Shallow copy (to be refactored)
      handled = screen->onEdit(screen->cursorCol, screen->cursorRow, editShallowClone);
    }
  }

  if (handled && !redrawn) {
    if (oldCursorCol != screen->cursorCol || oldCursorRow != screen->cursorRow) {
      // TODO: Make optimal selection redraw
    }

    screenFullRedraw(screen);
    //screenDrawSelection(screen, 1);
  }

  return handled;
}

int screenInput(struct ScreenData* screen, int keys, int isDoubleTap) {
  return (screen->selectMode == 1) ? inputSelectMode(screen, keys, isDoubleTap) : inputNormalMode(screen, keys, isDoubleTap);
}


///////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//

void setCellColor(int state, int isEmpty, int hasContent) {
  const struct ColorScheme cs = appSettings.colorScheme;

  if (state & stateSelected) {
    if (isEmpty) {
      gfxSetFgColor(cs.textEmpty);
    } else {
      gfxSetFgColor(cs.selection);
    }
  } else if (state & stateFocus) {
    gfxSetFgColor(cs.textDefault);
  } else if (isEmpty) {
    gfxSetFgColor(cs.textEmpty);
  } else if (hasContent) {
    gfxSetFgColor(cs.textValue);
  } else {
    gfxSetFgColor(cs.textInfo);
  }
}