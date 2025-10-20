#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>
#include <playback.h>
#include <string.h>
#include "screen_navigation.h"

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
  .isSelectMode = 0,
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

void setup(int input) {
  pSongRow = &screen.cursorRow;
  pSongTrack = &screen.cursorCol;


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
  // TODO: Proper width and height for actual selection
  gfxSelection(3 + col1 * 3, 3 + row1 - screen.topRow, 2, 1);
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
}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int isDoubleTap) {
  // Special case: Chain screen requires validation
  if (keys == (keyRight | keyShift)) {
    int chain = project.song[screen.cursorRow][screen.cursorCol];
    if (chain == EMPTY_VALUE_16) {
      screenMessage("Enter a chain");
      return 1;
    }
  }
  
  return handleScreenNavigation(&songNavigation, keys, isDoubleTap);
}

static int findEmptyChain(int startChain) {
  for (int c = startChain; c < PROJECT_MAX_CHAINS; c++) {
    if (chainIsEmpty(c)) {
      return c;
    }
  }
  return EMPTY_VALUE_16;
}

static int onEdit(int col, int row, enum CellEditAction action) {
  if (action == editDoubleTap) {
    // Find the first chain with no phrases
    int current = project.song[row][col];

    if (current != EMPTY_VALUE_16) {
      int nextEmpty = findEmptyChain(current + 1);
      if (nextEmpty != EMPTY_VALUE_16) {
        project.song[row][col] = nextEmpty;
        lastChainValue = nextEmpty;
      }
    }
    return 1;
  } else if (action == editShallowClone) {
    // Shallow chain clone
    int current = project.song[row][col];

    if (current != EMPTY_VALUE_16) {
      int nextEmpty = findEmptyChain(current + 1);
      if (nextEmpty != EMPTY_VALUE_16) {
        project.song[row][col] = nextEmpty;
        lastChainValue = nextEmpty;

        memcpy(&project.chains[nextEmpty], &project.chains[current], sizeof(struct Chain));
      }
    }
    return 1;
  } else {
    return edit16withLimit(action, &project.song[row][col], &lastChainValue, 16, PROJECT_MAX_CHAINS - 1);
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  if (inputScreenNavigation(keys, isDoubleTap)) return;
  if (screenInput(&screen, keys, isDoubleTap)) return;
}

///////////////////////////////////////////////////////////////////////////////

const struct AppScreen screenSong = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
