#include "screens.h"
#include "screen_settings.h"
#include "common.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "audio_manager.h"

#include "copy_paste.h"
#include <string.h>

// Screen state variables
static uint16_t lastChainValue = 0;

// Track mute/solo state machine
typedef enum {
  MUTE_SOLO_EMPTY,
  MUTE_SOLO_OPT_PRESSED,
  MUTE_SOLO_MUTE_STATE,
  MUTE_SOLO_SOLO_STATE
} MuteSoloState;

static MuteSoloState muteSoloState = MUTE_SOLO_EMPTY;

static int getColumnCount(int row);
static void drawStatic(void);
static void drawField(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static void drawSelection(int col1, int row1, int col2, int row2);
static int onEdit(int col, int row, enum CellEditAction action);

static ScreenData screen = {
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

static void init(void) {
  lastChainValue = 0;
  screen.cursorRow = 0;
  screen.cursorCol = 0;
  screen.topRow = 0;
  screen.selectMode = 0;
  screen.selectStartRow = 0;
  screen.selectStartCol = 0;
  screen.selectAnchorRow = 0;
  screen.selectAnchorCol = 0;
  pSongRow = &screen.cursorRow;
  pSongTrack = &screen.cursorCol;
}

static void setup(int input) {
  screen.selectMode = 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static int getColumnCount(int row) {
  return chipnomadState->project.tracksCount;
}

static void drawStatic(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrint(0, 0, "SONG");
}

static void drawField(int col, int row, int state) {
  if (row < screen.topRow || row >= (screen.topRow + 16)) return; // Don't draw outside of the viewing area

  int chain = chipnomadState->project.song[row][col];
  setCellColor(state, chain == EMPTY_VALUE_16, chain != EMPTY_VALUE_16 && chainHasNotes(&chipnomadState->project, chain));
  gfxPrint(3 + col * 3, 3 + row - screen.topRow, chain == EMPTY_VALUE_16 ? "--" : byteToHex(chain));
}

static void drawRowHeader(int row, int state) {
  const ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrint(0, 3 + row - screen.topRow, byteToHex(row));
}

static void drawColHeader(int col, int state) {
  static char digit[2] = "0";
  const ColorScheme cs = appSettings.colorScheme;

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
  for (int c = 0; c < chipnomadState->project.tracksCount; c++) {
    gfxClearRect(2 + c * 3, 3, 1, 16);
    if (chipnomadState->playbackState.tracks[c].songRow != EMPTY_VALUE_16) {
      int row = chipnomadState->playbackState.tracks[c].songRow - screen.topRow;
      if (row >= 0 && row < 16) {
        gfxSetFgColor(appSettings.colorScheme.playMarkers);
        gfxPrint(2 + c * 3, 3 + row, ">");
      }
    }
    
    // Draw mute/solo indicator above track number
    gfxSetFgColor(appSettings.colorScheme.textTitles);
    if (audioManager.trackStates[c] == TRACK_MUTED) {
      gfxPrint(3 + c * 3, 1, "M");
    } else if (audioManager.trackStates[c] == TRACK_SOLO) {
      gfxPrint(3 + c * 3, 1, "S");
    } else {
      gfxPrint(3 + c * 3, 1, " "); // Clear indicator
    }
  }

  screenDrawOverlays(&screen);
}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int isDoubleTap) {
  // Fix all bare returns in this function
  // Go to Chain screen
  if (keys == (keyRight | keyShift)) {
    int chain = chipnomadState->project.song[screen.cursorRow][screen.cursorCol];

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
    int current = chipnomadState->project.song[row][col];
    if (current != EMPTY_VALUE_16) {
      int nextEmpty = findEmptyChain(&chipnomadState->project, current + 1);
      if (nextEmpty != EMPTY_VALUE_16) {
        chipnomadState->project.song[row][col] = nextEmpty;
        lastChainValue = nextEmpty;
      } else {
        screenMessage(MESSAGE_TIME, "No free chains");
      }
    }
    return 1;
  } else if (action == editClear) {
    if (chipnomadState->project.song[row][col] != EMPTY_VALUE_16) {
      // Clear the value
      chipnomadState->project.song[row][col] = EMPTY_VALUE_16;
      return 1;
    } else {
      // Value is already empty, shift column up
      shiftSongColumnUp(col, row);
      fullRedraw();
      return 1;
    }
  } else if (action == editShallowClone) {
    int current = chipnomadState->project.song[row][col];
    if (current != EMPTY_VALUE_16) {
      int cloned = cloneChainToNext(current);
      if (cloned != EMPTY_VALUE_16) {
        chipnomadState->project.song[row][col] = cloned;
        lastChainValue = cloned;
        return 1;
      }
    }
    return 0;
  } else if (action == editDeepClone) {
    int current = chipnomadState->project.song[row][col];
    if (current != EMPTY_VALUE_16) {
      int cloned = deepCloneChainToNext(current);
      if (cloned != EMPTY_VALUE_16) {
        chipnomadState->project.song[row][col] = cloned;
        lastChainValue = cloned;
        return 1;
      }
    }
    return 0;
  }
  return edit16withLimit(action, &chipnomadState->project.song[row][col], &lastChainValue, 16, PROJECT_MAX_CHAINS - 1);
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

static int onInput(int isKeyDown, int keys, int isDoubleTap) {
  int handled = 0;

  // Only handle mute/solo when not in selection mode
  if (screen.selectMode == 0) {
    switch (muteSoloState) {
      case MUTE_SOLO_EMPTY:
        if (isKeyDown && keys == keyOpt) {
          muteSoloState = MUTE_SOLO_OPT_PRESSED;
          handled = 1;
        }
        break;

      case MUTE_SOLO_OPT_PRESSED:
        if (isKeyDown && keys == (keyOpt | keyShift)) {
          audioManager.toggleTrackMute(screen.cursorCol);
          muteSoloState = MUTE_SOLO_MUTE_STATE;
          handled = 1;
        } else if (isKeyDown && keys == (keyOpt | keyPlay)) {
          audioManager.toggleTrackSolo(screen.cursorCol);
          muteSoloState = MUTE_SOLO_SOLO_STATE;
          handled = 1;
        } else if (isKeyDown && keys == keyOpt) {
          handled = 1; // Stay in OPT_PRESSED state
        } else if (keys == 0) {
          muteSoloState = MUTE_SOLO_EMPTY;
        }
        break;

      case MUTE_SOLO_MUTE_STATE:
        if (!isKeyDown && keys == keyOpt) {
          // SHIFT released first - momentary unmute
          audioManager.toggleTrackMute(screen.cursorCol);
          muteSoloState = MUTE_SOLO_OPT_PRESSED;
          handled = 1;
        } else if (!isKeyDown && keys == keyShift) {
          // OPT released first - mute sticks
          muteSoloState = MUTE_SOLO_EMPTY;
          handled = 1;
        } else if (keys == (keyOpt | keyShift)) {
          handled = 1; // Stay in mute state
        } else if (keys == 0) {
          muteSoloState = MUTE_SOLO_EMPTY;
        }
        break;

      case MUTE_SOLO_SOLO_STATE:
        if (!isKeyDown && keys == keyOpt) {
          // PLAY released first - momentary unsolo
          audioManager.toggleTrackSolo(screen.cursorCol);
          muteSoloState = MUTE_SOLO_OPT_PRESSED;
          handled = 1;
        } else if (!isKeyDown && keys == keyPlay) {
          // OPT released first - solo sticks
          muteSoloState = MUTE_SOLO_EMPTY;
          handled = 1;
        } else if (keys == (keyOpt | keyPlay)) {
          handled = 1; // Stay in solo state
        } else if (keys == 0) {
          muteSoloState = MUTE_SOLO_EMPTY;
        }
        break;
    }
  }

  // Reset state when all keys released or entering selection mode
  if (keys == 0 || screen.selectMode != 0) {
    muteSoloState = MUTE_SOLO_EMPTY;
  }

  // Only call common input handling if we didn't handle Song-specific input
  if (!handled) {
    if (isKeyDown && screen.selectMode == 0 && inputScreenNavigation(keys, isDoubleTap)) return 1;
    return screenInput(&screen, isKeyDown, keys, isDoubleTap);
  }

  return handled;
}

///////////////////////////////////////////////////////////////////////////////

const AppScreen screenSong = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput,
  .init = init
};
