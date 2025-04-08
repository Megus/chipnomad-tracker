#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

int cursorRow = 0;
int cursorTrack = 0;
int topRow = 0;

uint8_t lastValue = 0;

int isSelect = 0;

static void setup() {

}

static void drawChain(int track, int row) {
  if (row < topRow || row >= (topRow + 16)) return; // Don't draw outside of the viewing area

  const struct ColorScheme cs = appSettings.colorScheme;
  uint8_t chain = project.song[row][track];

  if (track == cursorTrack && row == cursorRow) {
    gfxSetFgColor(cs.textDefault);
  } else if (chain == EMPTY_VALUE) {
    gfxSetFgColor(cs.textEmpty);
  } else if (project.chains[chain].isEmpty) {
    gfxSetFgColor(cs.textInfo);
  } else {
    gfxSetFgColor(cs.textValue);
  }

  gfxPrint(3 + track * 3, 3 + (row - topRow), chain == PROJECT_MAX_CHAINS ? "--" : byteToHex(chain));
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


static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    setupScreen(screenChain, 0);
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

  if (keys == keyEdit && isDoubleTap == 0) {
    if (project.song[cursorRow][cursorTrack] == EMPTY_VALUE) {
      project.song[cursorRow][cursorTrack] = lastValue;
    }
    handled = 1;
  } else if (keys == keyEdit && isDoubleTap == 1) {

  } else if (keys == (keyRight | keyEdit)) {
    uint8_t c = project.song[cursorRow][cursorTrack];
    project.song[cursorRow][cursorTrack] = c == PROJECT_MAX_CHAINS - 1 ? c : c + 1;
    handled = 1;
  } else if (keys == (keyLeft | keyEdit)) {
    uint8_t c = project.song[cursorRow][cursorTrack];
    project.song[cursorRow][cursorTrack] = c == 0 ? c : c - 1;
    handled = 1;
  } else if (keys == (keyUp | keyEdit)) {
    uint8_t c = project.song[cursorRow][cursorTrack];
    project.song[cursorRow][cursorTrack] = c > PROJECT_MAX_CHAINS - 16 ? PROJECT_MAX_CHAINS - 1 : c + 16;
    handled = 1;
  } else if (keys == (keyDown | keyEdit)) {
    uint8_t c = project.song[cursorRow][cursorTrack];
    project.song[cursorRow][cursorTrack] = c < 16 ? 0 : c - 16;
    handled = 1;
  } else if (keys == (keyEdit | keyOpt)) {
    project.song[cursorRow][cursorTrack] = EMPTY_VALUE;
    handled = 1;
  }

  if (handled) {
    if (project.song[cursorRow][cursorTrack] != EMPTY_VALUE) {
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

int screenSong(struct AppEvent event) {
  switch (event.type) {
    case appEventSetup:
      setup();
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
