#include "screens.h"
#include "common.h"
#include "corelib_gfx.h"
#include "utils.h"
#include "chipnomad_lib.h"
#include "project_utils.h"
#include "screen_instrument.h"
#include "copy_paste.h"
#include <string.h>

static int cursorRow = 0;
static int topRow = 0;

typedef enum {
  POOL_NORMAL,
  POOL_EDIT_PRESSED,
  POOL_MOVING,
} PoolState;

static PoolState poolState = POOL_NORMAL;

static void setup(int input) {
  poolState = POOL_NORMAL;
  if (input != -1) {
    cursorRow = input;
    if (cursorRow >= topRow + 16) {
      topRow = cursorRow - 15;
    } else if (cursorRow < topRow) {
      topRow = cursorRow;
    }
  }
}

static void fullRedraw(void) {
  const ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrintf(0, 0, "INSTRUMENT POOL");

  gfxSetFgColor(cs.textInfo);
  gfxPrint(0, 2, "    Name            Type");

  // Draw instruments
  int maxRow = topRow + 16;
  if (maxRow > PROJECT_MAX_INSTRUMENTS) maxRow = PROJECT_MAX_INSTRUMENTS;

  for (int i = topRow; i < maxRow; i++) {
    int y = 3 + (i - topRow);

    // Set color based on cursor position and instrument state
    if (i == cursorRow) {
      gfxSetFgColor(cs.textValue);
    } else if (instrumentIsEmpty(&chipnomadState->project, i)) {
      gfxSetFgColor(cs.textEmpty);
    } else {
      gfxSetFgColor(cs.textDefault);
    }

    // Draw cursor indicator and instrument number
    if (i == cursorRow) {
      gfxPrint(0, y, ">");
    } else {
      gfxPrint(0, y, " ");
    }
    gfxPrintf(1, y, "%02X", i);

    // Draw instrument name
    if (!instrumentIsEmpty(&chipnomadState->project, i)) {
      gfxPrintf(4, y, "%-15s", chipnomadState->project.instruments[i].name);
    } else {
      gfxPrint(4, y, "               ");
    }

    // Draw instrument type
    gfxClearRect(20, y, 14, 1);
    gfxPrint(20, y, instrumentTypeName(chipnomadState->project.instruments[i].type));
  }
}

static void draw(void) {
  // Clear playback markers
  gfxClearRect(3, 3, 1, 16);

  // Draw playback markers for currently playing instruments
  for (int track = 0; track < chipnomadState->project.tracksCount; track++) {
    PlaybackTrackState* trackState = &chipnomadState->playbackState.tracks[track];
    if (trackState->mode != playbackModeStopped && trackState->note.instrument < PROJECT_MAX_INSTRUMENTS) {
      int instrument = trackState->note.instrument;
      if (instrument >= topRow && instrument < topRow + 16) {
        int y = 3 + (instrument - topRow);
        gfxSetFgColor(appSettings.colorScheme.playMarkers);
        gfxPrint(3, y, "*");
      }
    }
  }
}

static int inputScreenNavigation(int keys) {
  if (keys == (keyUp | keyShift)) {
    screenSetup(&screenInstrument, cursorRow);
    return 1;
  } else if (keys == (keyLeft | keyShift)) {
    screenSetup(&screenPhrase, -1);
    return 1;
  } else if (keys == (keyRight | keyShift)) {
    screenSetup(&screenTable, cursorRow);
    return 1;
  }
  return 0;
}

static int onInput(int isKeyDown, int keys, int tapCount) {
  int oldCursorRow = cursorRow;
  int oldTopRow = topRow;

  // Handle key-up
  if (!isKeyDown && keys == 0) {
    playbackStopPreview(&chipnomadState->playbackState, *pSongTrack);
    if (poolState == POOL_EDIT_PRESSED) {
      poolState = POOL_NORMAL;
      screenSetup(&screenInstrument, cursorRow);
      return 1;
    } else if (poolState == POOL_MOVING) {
      poolState = POOL_NORMAL;
    }
    return 0;
  }
  if (!isKeyDown) return 0;

  if (inputScreenNavigation(keys)) return 1;

  if (keys == keyEdit) {
    if (poolState == POOL_NORMAL) {
      poolState = POOL_EDIT_PRESSED;
    }
    return 1;
  } else if (keys == keyUp) {
    if (cursorRow > 0) {
      cursorRow--;
      if (cursorRow < topRow) {
        topRow--;
        fullRedraw();
        return 1;
      }
    }
  } else if (keys == keyDown) {
    if (cursorRow < PROJECT_MAX_INSTRUMENTS - 1) {
      cursorRow++;
      if (cursorRow >= topRow + 16) {
        topRow++;
        fullRedraw();
        return 1;
      }
    }
  } else if (keys == keyLeft) {
    // Page up (16 lines)
    cursorRow -= 16;
    if (cursorRow < 0) cursorRow = 0;
    topRow -= 16;
    if (topRow < 0) topRow = 0;
    fullRedraw();
    return 1;
  } else if (keys == keyRight) {
    // Page down (16 lines)
    cursorRow += 16;
    if (cursorRow >= PROJECT_MAX_INSTRUMENTS) cursorRow = PROJECT_MAX_INSTRUMENTS - 1;
    topRow += 16;
    if (topRow + 16 >= PROJECT_MAX_INSTRUMENTS) topRow = PROJECT_MAX_INSTRUMENTS - 16;
    if (topRow < 0) topRow = 0;
    fullRedraw();
    return 1;
  } else if (keys == (keyEdit | keyUp)) {
    // Move instrument up
    if (cursorRow > 0) {
      poolState = POOL_MOVING;
      playbackStop(&chipnomadState->playbackState);
      instrumentSwap(&chipnomadState->project, cursorRow, cursorRow - 1);
      projectModified = 1;
      cursorRow--;
      if (cursorRow < topRow) {
        topRow--;
      }
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyEdit | keyDown)) {
    // Move instrument down
    if (cursorRow < PROJECT_MAX_INSTRUMENTS - 1) {
      poolState = POOL_MOVING;
      playbackStop(&chipnomadState->playbackState);
      instrumentSwap(&chipnomadState->project, cursorRow, cursorRow + 1);
      projectModified = 1;
      cursorRow++;
      if (cursorRow >= topRow + 16) {
        topRow++;
      }
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyEdit | keyPlay)) {
    // Preview instrument
    if (!instrumentIsEmpty(&chipnomadState->project, cursorRow) && !playbackIsPlaying(&chipnomadState->playbackState)) {
      uint8_t note = instrumentFirstNote(&chipnomadState->project, cursorRow);
      playbackPreviewNote(&chipnomadState->playbackState, *pSongTrack, note, cursorRow);
    }
    return 1;
  } else if (keys == (keyShift | keyOpt)) {
    // Copy instrument
    copyInstrument(cursorRow);
    screenMessage(MESSAGE_TIME, "Copied instrument");
    return 1;
  } else if (keys == (keyShift | keyEdit)) {
    // Paste instrument
    pasteInstrument(cursorRow);
    projectModified = 1;
    fullRedraw();
    return 1;
  }

  // Stop preview when cursor moves
  if (oldCursorRow != cursorRow) {
    playbackStopPreview(&chipnomadState->playbackState, *pSongTrack);
  }

  // Redraw only cursor if position changed
  if (oldCursorRow != cursorRow) {
    // Clear old cursor line
    int oldY = 3 + (oldCursorRow - oldTopRow);
    if (oldCursorRow >= oldTopRow && oldCursorRow < oldTopRow + 16) {
      if (instrumentIsEmpty(&chipnomadState->project, oldCursorRow)) {
        gfxSetFgColor(appSettings.colorScheme.textEmpty);
      } else {
        gfxSetFgColor(appSettings.colorScheme.textDefault);
      }
      gfxPrint(0, oldY, " ");
      gfxPrintf(1, oldY, "%02X", oldCursorRow);
      if (!instrumentIsEmpty(&chipnomadState->project, oldCursorRow)) {
        gfxPrintf(4, oldY, "%-15s", chipnomadState->project.instruments[oldCursorRow].name);
      } else {
        gfxPrint(4, oldY, "               ");
      }
      gfxClearRect(20, oldY, 14, 1);
      gfxPrint(20, oldY, instrumentTypeName(chipnomadState->project.instruments[oldCursorRow].type));
    }

    // Draw new cursor line
    int newY = 3 + (cursorRow - topRow);
    gfxSetFgColor(appSettings.colorScheme.textValue);
    gfxPrint(0, newY, ">");
    gfxPrintf(1, newY, "%02X", cursorRow);
    if (!instrumentIsEmpty(&chipnomadState->project, cursorRow)) {
      gfxPrintf(4, newY, "%-15s", chipnomadState->project.instruments[cursorRow].name);
    } else {
      gfxPrint(4, newY, "               ");
    }
    gfxClearRect(20, newY, 14, 1);
    gfxPrint(20, newY, instrumentTypeName(chipnomadState->project.instruments[cursorRow].type));
  }
  return 0;
}

const AppScreen screenInstrumentPool = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
