#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

// Screen state variables
int cursorRow = 0;
int cursorTrack = 0;
int topRow = 0;
int lastValue = 0;
int isSelect = 0;

void setup(int input) {
  if (input == 0x1234) { // Just a random value for now
    cursorRow = 0;
    cursorTrack = 0;
    topRow = 0;
    lastValue = 0;
    isSelect = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static void drawChain(int track, int row) {
  if (row < topRow || row >= (topRow + 16)) return; // Don't draw outside of the viewing area

  const struct ColorScheme cs = appSettings.colorScheme;
  int chain = project.song[row][track];

  if (track == cursorTrack && row == cursorRow) {
    gfxSetFgColor(cs.textDefault);
  } else if (chain == EMPTY_VALUE_16) {
    gfxSetFgColor(cs.textEmpty);
  } else if (project.chains[chain].hasNoNotes) {
    gfxSetFgColor(cs.textInfo);
  } else {
    gfxSetFgColor(cs.textValue);
  }

  gfxPrint(3 + track * 3, 3 + (row - topRow), chain == EMPTY_VALUE_16 ? "--" : byteToHex(chain));
}

static void drawAllChains() {
  for (int c = topRow; c < topRow + 16; c++) {
    for (int d = 0; d < project.tracksCount; d++) {
      drawChain(d, c);
    }
  }
}

static void drawCursor() {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxCursor(3 + cursorTrack * 3, 3 + (cursorRow - topRow), 2);

  // Row numbers
  for (int c = 0; c < 16; c++) {
    gfxSetFgColor(topRow + c == cursorRow ? cs.textDefault : cs.textInfo);
    gfxPrint(0, 3 + c, byteToHex(topRow + c));
  }

  // Track names
  char digit[2] = "0";
  for (int c = 0; c < project.tracksCount; c++) {
    gfxSetFgColor(c == cursorTrack ? cs.textDefault : cs.textInfo);
    digit[0] = c + 49;
    gfxPrint(3 + c * 3, 2, digit);
  }
}

static void fullRedraw(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrint(0, 0, "SONG");


  // Chains
  drawAllChains();
  drawCursor();
}

static void draw(void) {

}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    int chain = project.song[cursorRow][cursorTrack];
    if (chain == EMPTY_VALUE_16) {
      // If chain at cursor is empty, look up the track. If it's empty too, show message, don't let them go
      for (int c = cursorRow; c >= 0; c--) {
        chain = project.song[c][cursorTrack];
        if (chain != EMPTY_VALUE_16) break;
      }
    }

    if (chain != EMPTY_VALUE_16) {
      setupScreen(screenChain, chain);
    } else {
      screenMessage("Create a chain");
    }
    return 1;
  } else if (keys == (keyUp | keyShift)) {
    setupScreen(screenProject, 0);
    return 1;
  }
  return 0;
}

static int inputCursor(int keys, int isDoubleTap) {
  int oldCursorTrack = cursorTrack;
  int oldCursorRow = cursorRow;

  int handled = 0;
  if (keys == keyLeft) {
    cursorTrack = cursorTrack == 0 ? cursorTrack : cursorTrack - 1;
    handled = 1;
  } else if (keys == keyRight) {
    cursorTrack = cursorTrack == project.tracksCount - 1 ? cursorTrack : cursorTrack + 1;
    handled = 1;
  } else if (keys == keyUp) {
    cursorRow = cursorRow == 0 ? cursorRow : cursorRow - 1;
    if (cursorRow < topRow) {
      topRow--;
      drawAllChains();
    }
    handled = 1;
  } else if (keys == keyDown) {
    cursorRow = cursorRow == PROJECT_MAX_LENGTH - 1 ? cursorRow : cursorRow + 1;
    if (cursorRow >= topRow + 16) {
      topRow++;
      drawAllChains();
    }
    handled = 1;
  } else if (keys == (keyDown | keyOpt)) {
    if (cursorRow + 16 < PROJECT_MAX_LENGTH) {
      cursorRow += 16;
      topRow += 16;
      if (topRow + 16 >= PROJECT_MAX_LENGTH) topRow = PROJECT_MAX_LENGTH - 16;
      drawAllChains();
      handled = 1;
    }
  } else if (keys == (keyUp | keyOpt)) {
    if (cursorRow - 16 >= 0) {
      cursorRow -= 16;
      topRow -= 16;
      if (topRow < 0) topRow = 0;
      drawAllChains();
      handled = 1;
    }
  }

  if (handled) {
    drawChain(oldCursorTrack, oldCursorRow);
    drawChain(cursorTrack, cursorRow);
    drawCursor();
  }
  return handled;
}

static int inputEdit(int keys, int isDoubleTap) {
  int handled = 0;
  int current = project.song[cursorRow][cursorTrack];

  if (keys == keyEdit && isDoubleTap == 0) {
    if (current == EMPTY_VALUE_16) {
      project.song[cursorRow][cursorTrack] = lastValue;
    }
    handled = 1;
  } else if (keys == keyEdit && isDoubleTap == 1) {
    // Find the first chain with no phrases
    if (current != EMPTY_VALUE_16) {
      for (int c = current + 1; c < PROJECT_MAX_CHAINS; c++) {
        if (isChainEmpty(c)) {
          project.song[cursorRow][cursorTrack] = c;
          break;
        }
      }
    }
    handled = 1;
  } else if (keys == (keyRight | keyEdit)) {
    project.song[cursorRow][cursorTrack] = current == PROJECT_MAX_CHAINS - 1 ? current : current + 1;
    handled = 1;
  } else if (keys == (keyLeft | keyEdit)) {
    project.song[cursorRow][cursorTrack] = current == 0 ? current : current - 1;
    handled = 1;
  } else if (keys == (keyUp | keyEdit)) {
    project.song[cursorRow][cursorTrack] = current > PROJECT_MAX_CHAINS - 16 ? PROJECT_MAX_CHAINS - 1 : current + 16;
    handled = 1;
  } else if (keys == (keyDown | keyEdit)) {
    project.song[cursorRow][cursorTrack] = current < 16 ? 0 : current - 16;
    handled = 1;
  } else if (keys == (keyEdit | keyOpt)) {
    project.song[cursorRow][cursorTrack] = EMPTY_VALUE_16;
    handled = 1;
  }

  if (handled) {
    if (project.song[cursorRow][cursorTrack] != EMPTY_VALUE_16) {
      lastValue = project.song[cursorRow][cursorTrack];
    }
    drawChain(cursorTrack, cursorRow);
    drawCursor();
  }
  return handled;
}

static void onKey(int keys, int isDoubleTap) {
  //printf("%d\n", event.data.key.keys);
  if (inputScreenNavigation(keys, isDoubleTap)) return;
  if (inputCursor(keys, isDoubleTap)) return;
  if (inputEdit(keys, isDoubleTap)) return;
}

///////////////////////////////////////////////////////////////////////////////

int screenSong(struct AppEvent event) {
  switch (event.type) {
    case appEventSetup:
      setup(event.data.setup.input);
      break;
    case appEventFullRedraw:
      fullRedraw();
      break;
    case appEventDraw:
      draw();
      break;
    case appEventKey:
      onKey(event.data.key.keys, event.data.key.isDoubleTap);
      break;
  }

  return 0;
}
