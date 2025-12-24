#include "screens.h"
#include "common.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "copy_paste.h"
#include <string.h>

static int chain = 0;
static uint16_t lastPhraseValue = 0;
static uint8_t lastTransposeValue = 0;

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
  lastPhraseValue = 0;
  lastTransposeValue = 0;
  screen.cursorRow = 0;
  screen.cursorCol = 0;
  screen.topRow = 0;
  screen.selectMode = 0;
  screen.selectStartRow = 0;
  screen.selectStartCol = 0;
  screen.selectAnchorRow = 0;
  screen.selectAnchorCol = 0;
  pChainRow = &screen.cursorRow;
}

static void setup(int input) {
  chain = chipnomadState->project.song[*pSongRow][*pSongTrack];
  screen.selectMode = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static int getColumnCount(int row) {
  return 2;
}

static void drawStatic(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrintf(0, 0, "CHAIN %02X", chain);
}

static void drawField(int col, int row, int state) {
  uint16_t phrase = chipnomadState->project.chains[chain].rows[row].phrase;
  int hasContent = phrase != EMPTY_VALUE_16 && phraseHasNotes(&chipnomadState->project, phrase);

  if (col == 0) {
    // Phrase
    setCellColor(state, phrase == EMPTY_VALUE_16, hasContent);
    if (phrase == EMPTY_VALUE_16) {
      gfxPrint(3, 3 + row, "---");
    } else {
      gfxPrintf(3, 3 + row, "%03X", phrase);
    }
    // Also draw transpose to keep colors synchronized (only if not in selection mode)
    if (screen.selectMode == 0) {
      setCellColor(0, 0, hasContent);
      gfxPrint(7, 3 + row, byteToHex(chipnomadState->project.chains[chain].rows[row].transpose));
    }
  } else {
    // Transpose
    setCellColor(state, 0, hasContent);
    gfxPrint(7, 3 + row, byteToHex(chipnomadState->project.chains[chain].rows[row].transpose));
  }
}

static void drawRowHeader(int row, int state) {
  const ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrintf(1, 3 + row, "%X", row);
}

static void drawColHeader(int col, int state) {
  const ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);

  if (col == 0) {
    // Phrase
    gfxPrint(3, 2, "P");
  } else {
    // Transpose
    gfxPrint(7, 2, "T");
  }
}

static void drawCursor(int col, int row) {
  if (col == 0) {
    // Phrase
    gfxCursor(3, 3 + row, 3);
  } else {
    // Transpose
    gfxCursor(7, 3 + row, 2);
  }
}

static void drawSelection(int col1, int row1, int col2, int row2) {
  int x = (col1 == 0) ? 3 : 7;
  int w = (col2 - col1 == 1) ? 6 : (col1 == 0 ? 3 : 2);
  int y = 3 + row1;
  int h = row2 - row1 + 1;
  gfxRect(x, y, w, h);
}

static void fullRedraw(void) {
  screenFullRedraw(&screen);
}

static void draw(void) {
  gfxClearRect(2, 3, 1, 16);
  if (chipnomadState && chipnomadState->playbackState.tracks[*pSongTrack].songRow == *pSongRow) {
    int chainRow = chipnomadState->playbackState.tracks[*pSongTrack].chainRow;
    if (chainRow >= 0 && chainRow < 16) {
      gfxSetFgColor(appSettings.colorScheme.playMarkers);
      gfxPrint(2, 3 + chainRow, ">");
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int editCell(int col, int row, enum CellEditAction action) {
  if (col == 0) {
    if (action == editDoubleTap) {
      uint16_t current = chipnomadState->project.chains[chain].rows[row].phrase;
      if (current != EMPTY_VALUE_16) {
        int nextEmpty = findEmptyPhrase(&chipnomadState->project, current + 1);
        if (nextEmpty != EMPTY_VALUE_16) {
          chipnomadState->project.chains[chain].rows[row].phrase = nextEmpty;
          lastPhraseValue = nextEmpty;
        } else {
          screenMessage(MESSAGE_TIME, "No empty phrases");
        }
      }
      return 1;
    } else if (action == editShallowClone || action == editDeepClone) {
      uint16_t current = chipnomadState->project.chains[chain].rows[row].phrase;
      if (current != EMPTY_VALUE_16) {
        int cloned = clonePhraseToNext(current);
        if (cloned != EMPTY_VALUE_16) {
          chipnomadState->project.chains[chain].rows[row].phrase = cloned;
          lastPhraseValue = cloned;
          return 1;
        }
      }
      return 0;
    }
    return edit16withLimit(action, &chipnomadState->project.chains[chain].rows[row].phrase, &lastPhraseValue, 16, PROJECT_MAX_PHRASES - 1);
  } else {
    return edit8noLimit(action, &chipnomadState->project.chains[chain].rows[row].transpose, &lastTransposeValue, chipnomadState->project.pitchTable.octaveSize);
  }
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (action == editSwitchSelection) {
    return switchChainSelectionMode(&screen);
  } else if (action == editMultiIncrease || action == editMultiDecrease) {
    if (!isSingleColumnSelection(&screen)) return 0;
    return applyMultiEdit(&screen, action, editCell);
  } else if (action == editMultiIncreaseBig || action == editMultiDecreaseBig) {
    if (!isSingleColumnSelection(&screen)) return 0;
    return applyMultiEdit(&screen, action, editCell);
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
      screenMessage(MESSAGE_TIME, "Cloned %d phrase%s", clonedCount, clonedCount == 1 ? "" : "s");
      return 1;
    } else {
      screenMessage(MESSAGE_TIME, "No phrases to clone");
      return 0;
    }
  } else if (action == editCopy) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);
    copyChain(chain, startCol, startRow, endCol, endRow, 0);
    return 1;
  } else if (action == editCut) {
    int startCol, startRow, endCol, endRow;
    getSelectionBounds(&screen, &startCol, &startRow, &endCol, &endRow);
    copyChain(chain, startCol, startRow, endCol, endRow, 1);
    return 1;
  } else if (action == editPaste) {
    pasteChain(chain, col, row);
    fullRedraw();
    return 1;
  } else {
    return editCell(col, row, action);
  }
}

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    // To Phrase screen
    int phrase = chipnomadState->project.chains[chain].rows[screen.cursorRow].phrase;
    if (phrase == EMPTY_VALUE_16) {
      screenMessage(0, "Enter a phrase");
    } else {
      screenSetup(&screenPhrase, -1);
    }
    return 1;
  } else if (keys == (keyLeft | keyShift)) {
    // To Song screen
    screenSetup(&screenSong, 0);
    return 1;
  } else if (keys == (keyLeft | keyOpt)) {
    // Previous track
    if (*pSongTrack == 0) return 1;
    if (chipnomadState->project.song[*pSongRow][*pSongTrack - 1] != EMPTY_VALUE_16) {
      *pSongTrack -= 1;
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyRight | keyOpt)) {
    // Next track
    if (*pSongTrack == chipnomadState->project.tracksCount - 1) return 1;
    if (chipnomadState->project.song[*pSongRow][*pSongTrack + 1] != EMPTY_VALUE_16) {
      *pSongTrack += 1;
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyUp | keyOpt)) {
    // Previous song row
    if (*pSongRow == 0) return 1;
    if (chipnomadState->project.song[*pSongRow - 1][*pSongTrack] != EMPTY_VALUE_16) {
      *pSongRow -= 1;
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyDown | keyOpt)) {
    // Next song row
    if (*pSongRow == PROJECT_MAX_LENGTH - 1) return 1;
    if (chipnomadState->project.song[*pSongRow + 1][*pSongTrack] != EMPTY_VALUE_16) {
      *pSongRow += 1;
      setup(-1);
      fullRedraw();
    }
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int isDoubleTap) {
  if (screen.selectMode == 0 && inputScreenNavigation(keys, isDoubleTap)) return 1;
  return screenInput(&screen, isKeyDown, keys, isDoubleTap);
}

const AppScreen screenChain = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput,
  .init = init
};
