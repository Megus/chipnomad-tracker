#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>
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

static struct ScreenData screen = {
  .rows = 16,
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .selectMode = 0,
  .selectStartRow = 0,
  .selectStartCol = 0,
  .getColumnCount = getColumnCount,
  .drawStatic = drawStatic,
  .drawCursor = drawCursor,
  .drawSelection = drawSelection,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = drawField,
  .onEdit = onEdit,
};

static void setup(int input) {
  pChainRow = &screen.cursorRow;
  chain = project.song[*pSongRow][*pSongTrack];
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
  uint16_t phrase = project.chains[chain].phrases[row];

  if (col == 0) {
    // Phrase
    setCellColor(state, phrase == EMPTY_VALUE_16, phrase != EMPTY_VALUE_16 && phraseHasNotes(phrase));
    if (phrase == EMPTY_VALUE_16) {
      gfxPrint(3, 3 + row, "---");
    } else {
      gfxPrintf(3, 3 + row, "%03X", phrase);
    }
  } else {
    // Transpose
    setCellColor(state, 0, phrase != EMPTY_VALUE_16 && phraseHasNotes(phrase));
    gfxPrint(7, 3 + row, byteToHex(project.chains[chain].transpose[row]));
  }
}

static void drawRowHeader(int row, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrintf(1, 3 + row, "%X", row);
}

static void drawColHeader(int col, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
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
  if (playback.tracks[*pSongTrack].songRow == *pSongRow) {
    int chainRow = playback.tracks[*pSongTrack].chainRow;
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

int findEmptyPhrase(int start) {
  for (int i = start; i < PROJECT_MAX_PHRASES; i++) {
    if (phraseIsEmpty(i)) {
      return i;
    }
  }
  return EMPTY_VALUE_16;
}

static int onEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;

  if (col == 0) {
    if (action == editDoubleTap) {
      uint16_t current = project.chains[chain].phrases[row];

      if (current != EMPTY_VALUE_16) {
        int nextEmpty = findEmptyPhrase(current + 1);
        if (nextEmpty != EMPTY_VALUE_16) {
          project.chains[chain].phrases[row] = nextEmpty;
          lastPhraseValue = nextEmpty;
          handled = 1;
        } else {
          screenMessage("No empty phrases");
        }
      }
    } else if (action == editShallowClone || action == editDeepClone) {
      // Phrase clone
      uint16_t current = project.chains[chain].phrases[row];

      if (current != EMPTY_VALUE_16) {
        int nextEmpty = findEmptyPhrase(0);
        if (nextEmpty != EMPTY_VALUE_16) {
          project.chains[chain].phrases[row] = nextEmpty;
          lastPhraseValue = nextEmpty;
          memcpy(&project.phrases[nextEmpty], &project.phrases[current], sizeof(struct Phrase));
          handled = 1;
        } else {
          screenMessage("No empty phrases");
        }
      }

    } else {
      handled = edit16withLimit(action, &project.chains[chain].phrases[row], &lastPhraseValue, 16, PROJECT_MAX_PHRASES - 1);
    }

    if (handled) {
      project.chains[chain].hasNotes = -1;
    }
  } else {
    // Transpose
    handled = edit8noLimit(action, &project.chains[chain].transpose[row], &lastTransposeValue, project.pitchTable.octaveSize);
  }

  return handled;
}

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    // To Phrase screen
    int phrase = project.chains[chain].phrases[screen.cursorRow];
    if (phrase == EMPTY_VALUE_16) {
      screenMessage("Enter a phrase");
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
    if (project.song[*pSongRow][*pSongTrack - 1] != EMPTY_VALUE_16) {
      *pSongTrack -= 1;
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyRight | keyOpt)) {
    // Next track
    if (*pSongTrack == project.tracksCount - 1) return 1;
    if (project.song[*pSongRow][*pSongTrack + 1] != EMPTY_VALUE_16) {
      *pSongTrack += 1;
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyUp | keyOpt)) {
    // Previous song row
    if (*pSongRow == 0) return 1;
    if (project.song[*pSongRow - 1][*pSongTrack] != EMPTY_VALUE_16) {
      *pSongRow -= 1;
      setup(-1);
      fullRedraw();
    }
    return 1;
  } else if (keys == (keyDown | keyOpt)) {
    // Next song row
    if (*pSongRow == PROJECT_MAX_LENGTH - 1) return 1;
    if (project.song[*pSongRow + 1][*pSongTrack] != EMPTY_VALUE_16) {
      *pSongRow += 1;
      setup(-1);
      fullRedraw();
    }
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  if (screen.selectMode == 0 && inputScreenNavigation(keys, isDoubleTap)) return;
  screenInput(&screen, keys, isDoubleTap);
}

const struct AppScreen screenChain = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
