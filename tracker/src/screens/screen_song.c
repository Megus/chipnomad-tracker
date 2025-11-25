#include <screens.h>
#include <screen_settings.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>
#include <project_utils.h>
#include <playback.h>
#include <copy_paste.h>
#include <string.h>

// Screen state variables
static uint16_t lastChainValue = 0;

static int getColumnCount(int row);
static void drawStatic(void);
static void drawField(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static void drawSelection(int col1, int row1, int col2, int row2);
static int onEdit(int col, int row, enum CellEditAction action);

static struct ScreenData screen = {
  .rows = PROJECT_MAX_LENGTH,
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .selectMode = 0,
  .selectStartRow = 0,
  .selectStartCol = 0,
  .selectAnchorRow = 0,
  .selectAnchorCol = 0,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawSelection = drawSelection,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = drawField,
  .onEdit = onEdit,
};

void setup(int input) {
  pSongRow = &screen.cursorRow;
  pSongTrack = &screen.cursorCol;
  screen.selectMode = 0;

  if (input == 0x1234) { // Just a random value for now
    screen.cursorRow = 0;
    screen.cursorCol = 0;
    screen.topRow = 0;
    lastChainValue = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static int getColumnCount(int row) {
  return project.tracksCount;
}

static void drawStatic(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrint(0, 0, "SONG");
}

static void drawField(int col, int row, int state) {
  if (row < screen.topRow || row >= (screen.topRow + 16)) return; // Don't draw outside of the viewing area

  int chain = project.song[row][col];
  setCellColor(state, chain == EMPTY_VALUE_16, chain != EMPTY_VALUE_16 && chainHasNotes(chain));
  gfxPrint(3 + col * 3, 3 + row - screen.topRow, chain == EMPTY_VALUE_16 ? "--" : byteToHex(chain));
}

static void drawRowHeader(int row, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrint(0, 3 + row - screen.topRow, byteToHex(row));
}

static void drawColHeader(int col, int state) {
  static char digit[2] = "0";
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  digit[0] = col + 49;
  gfxPrint(3 + col * 3, 2, digit);
}

static void drawCursor(int col, int row) {
  gfxCursor(3 + col * 3, 3 + row - screen.topRow, 2);
}

static void drawSelection(int col1, int row1, int col2, int row2) {
  int x = 3 + col1 * 3;
  int y = 3 + row1 - screen.topRow;
  int w = 3 * (col2 - col1 + 1) - 1;
  int y2 = y + (row2 - row1);

  if (y < 3) y = 3; // Top row of selection is above the screen
  if (y > (3 + 15)) return; // Top row of selection is below the screen
  if (y2 < 3) return;
  if (y2 > (3 + 15)) y2 = (3 + 15);

  gfxRect(x, y, w, (y2 - y) + 1);
}

static void fullRedraw(void) {
  screenFullRedraw(&screen);
}

static void draw(void) {
  for (int c = 0; c < project.tracksCount; c++) {
    gfxClearRect(2 + c * 3, 3, 1, 16);
    if (playback.tracks[c].songRow != EMPTY_VALUE_16) {
      int row = playback.tracks[c].songRow - screen.topRow;
      if (row >= 0 && row < 16) {
        gfxSetFgColor(appSettings.colorScheme.playMarkers);
        gfxPrint(2 + c * 3, 3 + row, ">");
      }
    }
  }

  screenDrawOverlays(&screen);
}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int isDoubleTap) {
  // Go to Chain screen
  if (keys == (keyRight | keyShift)) {
    int chain = project.song[screen.cursorRow][screen.cursorCol];

    if (chain == EMPTY_VALUE_16) {
      screenMessage(0, "Enter a chain");
    } else {
      screenSetup(&screenChain, -1);
    }
    return 1;
  }

  // Go to Project screen
  if (keys == (keyUp | keyShift)) {
    screenSetup(&screenProject, 0);
    return 1;
  }

  // Go to Settings screen
  if (keys == (keyDown | keyShift)) {
    screenSetup(&screenSettings, 0);
    return 1;
  }

  return 0;
}

static int editCell(int col, int row, enum CellEditAction action) {
  if (action == editDoubleTap) {
    int current = project.song[row][col];
    if (current != EMPTY_VALUE_16) {
      int nextEmpty = findEmptyChain(current + 1);
      if (nextEmpty != EMPTY_VALUE_16) {
        project.song[row][col] = nextEmpty;
        lastChainValue = nextEmpty;
      } else {
        screenMessage(MESSAGE_TIME, "No free chains");
      }
    }
    return 1;
  } else if (action == editShallowClone) {
    int current = project.song[row][col];
    if (current != EMPTY_VALUE_16) {
      int cloned = cloneChainToNext(current);
      if (cloned != EMPTY_VALUE_16) {
        project.song[row][col] = cloned;
        lastChainValue = cloned;
        return 1;
      }
    }
    return 0;
  } else if (action == editDeepClone) {
    int current = project.song[row][col];
    if (current != EMPTY_VALUE_16) {
      int cloned = deepCloneChainToNext(current);
      if (cloned != EMPTY_VALUE_16) {
        project.song[row][col] = cloned;
        lastChainValue = cloned;
        return 1;
      }
    }
    return 0;
  }
  return edit16withLimit(action, &project.song[row][col], &lastChainValue, 16, PROJECT_MAX_CHAINS - 1);
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (action == editSwitchSelection) {
    return switchSongSelectionMode(&screen);
  } else if (action == editMultiIncreaseBig || action == editMultiDecreaseBig) {
    if (screen.selectMode != 1) return 0;
    
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);
    
    int success = 0;
    if (action == editMultiDecreaseBig) {
      success = applySongMoveDown(startCol, startRow, endCol, endRow);
      if (success) {
        screen.selectStartRow++;
        screen.cursorRow++;
        // Scroll down if selection moved below visible area
        if (screen.cursorRow >= screen.topRow + 16) {
          screen.topRow++;
        }
      }
    } else {
      success = applySongMoveUp(startCol, startRow, endCol, endRow);
      if (success) {
        screen.selectStartRow--;
        screen.cursorRow--;
        // Scroll up if selection moved above visible area
        if (screen.cursorRow < screen.topRow) {
          screen.topRow--;
        }
      }
    }
    
    if (success) {
      fullRedraw();
    }
    return success;
  } else if (action == editShallowClone || action == editDeepClone) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);
    int clonedCount = 0;
    for (int r = startRow; r <= endRow; r++) {
      for (int c = startCol; c <= endCol; c++) {
        if (editCell(c, r, action)) clonedCount++;
      }
    }
    if (clonedCount > 0) {
      const char* msg = (action == editShallowClone) ? "Shallow-cloned" : "Deep-cloned";
      screenMessage(MESSAGE_TIME, "%s %d chain%s", msg, clonedCount, clonedCount == 1 ? "" : "s");
      return 1;
    } else {
      screenMessage(MESSAGE_TIME, "No chains to clone");
      return 0;
    }
  } else if (applyMultiEdit(&screen, action, editCell)) {
    return 1;
  } else if (action == editCopy) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);
    copySong(startCol, startRow, endCol, endRow, 0);
    return 1;
  } else if (action == editCut) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);
    copySong(startCol, startRow, endCol, endRow, 1);
    return 1;
  } else if (action == editPaste) {
    pasteSong(col, row);
    fullRedraw();
    return 1;
  } else {
    return editCell(col, row, action);
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  if (screen.selectMode == 0 && inputScreenNavigation(keys, isDoubleTap)) return;
  screenInput(&screen, keys, isDoubleTap);
}

///////////////////////////////////////////////////////////////////////////////

const struct AppScreen screenSong = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
