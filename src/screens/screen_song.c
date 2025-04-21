#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>
#include <playback.h>

// Screen state variables
static uint16_t lastChainValue = 0;

static void drawCell(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static int onEdit(int col, int row, enum CellEditAction action);

static struct SpreadsheetScreenData sheet = {
  .rows = PROJECT_MAX_LENGTH,
  .cols = 1, // Will overwrite in setup
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .drawCursor = drawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawCell = drawCell,
  .onEdit = onEdit,
};

void setup(int input) {
  pSongRow = &sheet.cursorRow;
  pSongTrack = &sheet.cursorCol;

  sheet.cols = project.tracksCount;

  if (input == 0x1234) { // Just a random value for now
    sheet.cursorRow = 0;
    sheet.cursorCol = 0;
    sheet.topRow = 0;
    lastChainValue = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static void drawCell(int col, int row, int state) {
  if (row < sheet.topRow || row >= (sheet.topRow + 16)) return; // Don't draw outside of the viewing area

  int chain = project.song[row][col];
  setCellColor(state, chain == EMPTY_VALUE_16, chain != EMPTY_VALUE_16 && chainHasNotes(chain));
  gfxPrint(3 + col * 3, 3 + row - sheet.topRow, chain == EMPTY_VALUE_16 ? "--" : byteToHex(chain));
}

static void drawRowHeader(int row, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrint(0, 3 + row - sheet.topRow, byteToHex(row));
}

static void drawColHeader(int col, int state) {
  static char digit[2] = "0";
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  digit[0] = col + 49;
  gfxPrint(3 + col * 3, 2, digit);
}

static void drawCursor(int col, int row) {
  gfxCursor(3 + col * 3, 3 + row - sheet.topRow, 2);
}

static void fullRedraw(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrint(0, 0, "SONG");

  spreadsheetFullRedraw(&sheet);
}

static void draw(void) {

}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int isDoubleTap) {
  // Go to Chain screen
  if (keys == (keyRight | keyShift)) {
    int chain = project.song[sheet.cursorRow][sheet.cursorCol];

    if (chain == EMPTY_VALUE_16) {
      screenMessage("Enter a chain");
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

  return 0;
}

static int onEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;

  if (action == editDoubleTap) {
    // Find the first chain with no phrases
    int current = project.song[row][col];

    if (current != EMPTY_VALUE_16) {
      for (int c = current + 1; c < PROJECT_MAX_CHAINS; c++) {
        if (chainIsEmpty(c)) {
          project.song[row][col] = c;
          lastChainValue = c;
          break;
        }
      }
    }
    handled = 1;
  } else {
    handled = edit16withLimit(action, &project.song[row][col], &lastChainValue, 16, PROJECT_MAX_CHAINS);
  }

  return handled;
}

static int inputPlayback(int keys, int isDoubleTap) {
  int handled = 0;

  if (!playbackIsPlaying(&playback) && (keys & keyPlay)) {
    playbackStartSong(&playback, sheet.cursorRow, 0);
    handled = 1;
  } else if (playbackIsPlaying(&playback) && (keys == keyPlay)) {
    playbackStop(&playback);
    handled = 1;
  }

  return handled;
}

static void onInput(int keys, int isDoubleTap) {
  if (inputScreenNavigation(keys, isDoubleTap)) return;
  if (spreadsheetInput(&sheet, keys, isDoubleTap)) return;
  inputPlayback(keys, isDoubleTap);
}

///////////////////////////////////////////////////////////////////////////////

const struct AppScreen screenSong = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
