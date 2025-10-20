#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>
#include <screen_instrument.h>
#include <string.h>
#include "screen_navigation.h"

static int cursorRow = 0;
static int topRow = 0;

static void onCursorChange(void);
static void onPageChange(void);

static void setup(int input) {
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
  const struct ColorScheme cs = appSettings.colorScheme;
  
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
    } else if (instrumentIsEmpty(i)) {
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
    if (!instrumentIsEmpty(i)) {
      gfxPrintf(4, y, "%-15s", project.instruments[i].name);
    } else {
      gfxPrint(4, y, "               ");
    }
    
    // Draw instrument type
    gfxClearRect(20, y, 14, 1);
    gfxPrint(20, y, instrumentTypeName(project.instruments[i].type));
  }
  

}

static void draw(void) {
  // Clear playback markers
  gfxClearRect(3, 3, 1, 16);
  
  // Draw playback markers for currently playing instruments
  for (int track = 0; track < project.tracksCount; track++) {
    struct PlaybackTrackState* trackState = &playback.tracks[track];
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

static int editPressed = 0;

static void onInput(int keys, int isDoubleTap) {
  int oldCursorRow = cursorRow;
  
  // Handle Edit button press/release for instrument selection
  if (keys == keyEdit) {
    editPressed = 1;
    return;
  } else if (keys == 0 && editPressed) {
    editPressed = 0;
    // Go to selected instrument when Edit is released
    screenSetup(&screenInstrument, cursorRow);
    return;
  } else if (keys != 0) {
    editPressed = 0;
  }
  
  // Handle cursor navigation
  struct CursorNavConfig cursorConfig = createCursorNavConfig(&cursorRow, &topRow, PROJECT_MAX_INSTRUMENTS, 16, onCursorChange, onPageChange);
  if (handleCursorNavigation(&cursorConfig, keys)) {
    return;
  } else if (keys == (keyUp | keyShift)) {
    // To Instrument screen
    screenSetup(&screenInstrument, cursorRow);
    return;
  } else if (keys == (keyRight | keyShift)) {
    // To Table screen
    screenSetup(&screenTable, cursorRow);
    return;
  }
  
  // Use common navigation for other transitions
  if (handleScreenNavigation(&instrumentPoolNavigation, keys, isDoubleTap)) {
    return;
  } else if (keys == (keyEdit | keyUp)) {
    // Move instrument up
    if (cursorRow > 0) {
      playbackStop(&playback);
      instrumentSwap(cursorRow, cursorRow - 1);
      cursorRow--;
      if (cursorRow < topRow) {
        topRow--;
      }
      fullRedraw();
    }
    return;
  } else if (keys == (keyEdit | keyDown)) {
    // Move instrument down
    if (cursorRow < PROJECT_MAX_INSTRUMENTS - 1) {
      playbackStop(&playback);
      instrumentSwap(cursorRow, cursorRow + 1);
      cursorRow++;
      if (cursorRow >= topRow + 16) {
        topRow++;
      }
      fullRedraw();
    }
    return;
  } else if (keys == (keyEdit | keyPlay)) {
    // Preview instrument
    if (!instrumentIsEmpty(cursorRow) && !playbackIsPlaying(&playback)) {
      uint8_t note = instrumentFirstNote(cursorRow);
      playbackPreviewNote(&playback, *pSongTrack, note, cursorRow);
    }
    return;
  }
  
  // Stop preview when keys are released
  if (keys == 0) {
    playbackStopPreview(&playback, *pSongTrack);
    editPressed = 0;
  }
  
  // Stop preview when cursor moves
  if (oldCursorRow != cursorRow) {
    playbackStopPreview(&playback, *pSongTrack);
    editPressed = 0;
  }
  
}

static void onCursorChange(void) {
  // Redraw only the affected cursor lines
  fullRedraw(); // For now, just do full redraw - can optimize later
}

static void onPageChange(void) {
  fullRedraw();
}

const struct AppScreen screenInstrumentPool = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};