#include "screens.h"
#include "common.h"
#include "gfx.h"
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
  POOL_PREVIEWING,
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
  const ColorTheme t = theme();

  gfx.setFgColor(t.textTitles);
  gfx.printf(0, 0, "INSTRUMENT POOL");

  gfx.setFgColor(t.textInfo);
  gfx.print(0, 2, "    Name            Type");

  // Draw instruments
  int maxRow = topRow + 16;
  if (maxRow > PROJECT_MAX_INSTRUMENTS) maxRow = PROJECT_MAX_INSTRUMENTS;

  for (int i = topRow; i < maxRow; i++) {
    int y = 3 + (i - topRow);

    // Set color based on cursor position and instrument state
    if (i == cursorRow) {
      gfx.setFgColor(t.textValue);
    } else if (instrumentIsEmpty(&chipnomadState->project, i)) {
      gfx.setFgColor(t.textEmpty);
    } else {
      gfx.setFgColor(t.textDefault);
    }

    // Draw cursor indicator and instrument number
    if (i == cursorRow) {
      gfx.print(0, y, ">");
    } else {
      gfx.print(0, y, " ");
    }
    gfx.printf(1, y, "%02X", i);

    // Draw instrument name
    if (!instrumentIsEmpty(&chipnomadState->project, i)) {
      gfx.printf(4, y, "%-15s", chipnomadState->project.instruments[i].name);
    } else {
      gfx.print(4, y, "               ");
    }

    // Draw instrument type
    gfx.clearRect(20, y, 14, 1);
    gfx.print(20, y, instrumentTypeName(chipnomadState->project.instruments[i].type));
  }
}

static void draw(void) {
  // Clear playback markers
  gfx.clearRect(3, 3, 1, 16);

  // Draw playback markers for currently playing instruments
  for (int track = 0; track < chipnomadState->project.tracksCount; track++) {
    PlaybackTrackState* trackState = &chipnomadState->engine->player.tracks[track];
    if (trackState->mode != PlaybackMode::stopped && trackState->note.instrument < PROJECT_MAX_INSTRUMENTS) {
      int instrument = trackState->note.instrument;
      if (instrument >= topRow && instrument < topRow + 16) {
        int y = 3 + (instrument - topRow);
        gfx.setFgColor(theme().playMarkers);
        gfx.print(3, y, "*");
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
    chipnomadState->engine->player.stopPreview(*pSongTrack);
    if (poolState == POOL_EDIT_PRESSED) {
      poolState = POOL_NORMAL;
      screenSetup(&screenInstrument, cursorRow);
      return 1;
    } else if (poolState != POOL_NORMAL) {
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
      chipnomadState->engine->player.stop();
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
      chipnomadState->engine->player.stop();
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
    poolState = POOL_PREVIEWING;
    if (!instrumentIsEmpty(&chipnomadState->project, cursorRow) && !chipnomadState->engine->player.isPlaying()) {
      uint8_t note = instrumentFirstNote(&chipnomadState->project, cursorRow);
      chipnomadState->engine->player.previewNote(*pSongTrack, note, cursorRow);
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
    chipnomadState->engine->player.stopPreview(*pSongTrack);
  }

  // Redraw only cursor if position changed
  if (oldCursorRow != cursorRow) {
    // Clear old cursor line
    int oldY = 3 + (oldCursorRow - oldTopRow);
    if (oldCursorRow >= oldTopRow && oldCursorRow < oldTopRow + 16) {
      if (instrumentIsEmpty(&chipnomadState->project, oldCursorRow)) {
        gfx.setFgColor(theme().textEmpty);
      } else {
        gfx.setFgColor(theme().textDefault);
      }
      gfx.print(0, oldY, " ");
      gfx.printf(1, oldY, "%02X", oldCursorRow);
      if (!instrumentIsEmpty(&chipnomadState->project, oldCursorRow)) {
        gfx.printf(4, oldY, "%-15s", chipnomadState->project.instruments[oldCursorRow].name);
      } else {
        gfx.print(4, oldY, "               ");
      }
      gfx.clearRect(20, oldY, 14, 1);
      gfx.print(20, oldY, instrumentTypeName(chipnomadState->project.instruments[oldCursorRow].type));
    }

    // Draw new cursor line
    int newY = 3 + (cursorRow - topRow);
    gfx.setFgColor(theme().textValue);
    gfx.print(0, newY, ">");
    gfx.printf(1, newY, "%02X", cursorRow);
    if (!instrumentIsEmpty(&chipnomadState->project, cursorRow)) {
      gfx.printf(4, newY, "%-15s", chipnomadState->project.instruments[cursorRow].name);
    } else {
      gfx.print(4, newY, "               ");
    }
    gfx.clearRect(20, newY, 14, 1);
    gfx.print(20, newY, instrumentTypeName(chipnomadState->project.instruments[cursorRow].type));
  }
  return 0;
}

static ScreenPlaybackLevel getPlaybackLevel(void) {
  return ScreenPlaybackLevel::phrase;
}

const AppScreen screenInstrumentPool = {
  .init = NULL,
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput,
  .getPlaybackLevel = getPlaybackLevel,
};
