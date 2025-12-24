#include "screens.h"
#include "common.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "copy_paste.h"

static int groove = 0;
static uint8_t lastValue = 0;

static int getColumnCount(int row);
static void drawStatic(void);
static void drawField(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static void drawSelection(int col1, int row1, int col2, int row2);
static int onEdit(int col, int row, enum CellEditAction action);

static ScreenData screen = {
  .rows = 16,
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

static void init(void) {
  lastValue = 0;
  screen.cursorRow = 0;
  screen.cursorCol = 0;
  screen.topRow = 0;
  screen.selectMode = 0;
  screen.selectStartRow = 0;
  screen.selectStartCol = 0;
  screen.selectAnchorRow = 0;
  screen.selectAnchorCol = 0;
}

static void setup(int input) {
  groove = input;
  screen.selectMode = 0;
}

static int getColumnCount(int row) {
  return 1;  // Only one column for groove values
}

static void drawStatic(void) {
  const ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor(cs.textTitles);
  gfxPrintf(0, 0, "GROOVE %02X", groove);
}

static void drawField(int col, int row, int state) {
  uint8_t value = chipnomadState->project.grooves[groove].speed[row];
  setCellColor(state, value == EMPTY_VALUE_8, value != 0);
  gfxPrintf(3, 3 + row, byteToHexOrEmpty(value));
}

static void drawRowHeader(int row, int state) {
  const ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrintf(1, 3 + row, "%X", row);
}

static void drawColHeader(int col, int state) {
  const ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrint(3, 2, "T");  // T for Timing
}

static void drawCursor(int col, int row) {
  gfxCursor(3, 3 + row, 2);
}

static void drawSelection(int col1, int row1, int col2, int row2) {
  gfxRect(3, 3 + row1, 2, row2 - row1 + 1);
}

static void fullRedraw(void) {
  screenFullRedraw(&screen);
}

static void draw(void) {
  // Clear the marker column
  gfxClearRect(2, 3, 1, 16);

  // Show play position if this groove is currently playing
  if (chipnomadState->playbackState.tracks[*pSongTrack].grooveIdx == groove) {
    int row = chipnomadState->playbackState.tracks[*pSongTrack].grooveRow;
    if (row >= 0 && row < 16) {
      gfxSetFgColor(appSettings.colorScheme.playMarkers);
      gfxPrint(2, 3 + row, ">");
    }
  }
}

static int editCell(int col, int row, enum CellEditAction action) {
  return edit8withLimit(action, &chipnomadState->project.grooves[groove].speed[row], &lastValue, 16, EMPTY_VALUE_8 - 1);
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (action == editSwitchSelection) {
    return switchGrooveSelectionMode(&screen);
  } else if (action == editMultiIncrease || action == editMultiDecrease) {
    if (!isSingleColumnSelection(&screen)) return 0;
    return applyMultiEdit(&screen, action, editCell);
  } else if (action == editCopy) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);
    copyGroove(groove, startRow, endRow, 0);
    return 1;
  } else if (action == editCut) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);
    copyGroove(groove, startRow, endRow, 1);
    return 1;
  } else if (action == editPaste) {
    pasteGroove(groove, row);
    fullRedraw();
    return 1;
  } else {
    return editCell(col, row, action);
  }
}

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyDown | keyShift)) {
    // To Phrase scrren
    screenSetup(&screenPhrase, 0);
    return 1;
  } else if (keys == (keyLeft | keyOpt)) {
    // To previous groove
    if (groove > 0) {
      groove--;
      fullRedraw();
      return 1;
    }
  } else if (keys == (keyRight | keyOpt)) {
    // To next groove
    if (groove < PROJECT_MAX_GROOVES) {
      groove++;
      fullRedraw();
      return 1;
    }
  } else if (keys == (keyUp | keyOpt)) {
    // +16 grooves
    groove += 16;
    if (groove >= PROJECT_MAX_GROOVES) groove = PROJECT_MAX_GROOVES - 1;
    fullRedraw();
    return 1;
  } else if (keys == (keyDown | keyOpt)) {
    // -16 grooves
    groove -= 16;
    if (groove < 0) groove = 0;
    fullRedraw();
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int isDoubleTap) {
  if (screen.selectMode == 0 && inputScreenNavigation(keys, isDoubleTap)) return 1;
  return screenInput(&screen, isKeyDown, keys, isDoubleTap);
}

const AppScreen screenGroove = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput,
  .init = init
};
