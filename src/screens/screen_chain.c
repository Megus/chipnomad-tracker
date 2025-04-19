#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

static int chain = 0;
static uint16_t lastPhraseValue = 0;
static uint8_t lastTransposeValue = 0;

static void drawCell(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static int onEdit(int col, int row, enum CellEditAction action);

static struct SpreadsheetScreenData sheet = {
  .rows = 16,
  .cols = 2,
  .cursorRow = 0,
  .cursorCol = 0,
  .topRow = 0,
  .drawCursor = drawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawCell = drawCell,
  .onEdit = onEdit,
};

static void setup(int input) {
  pChainRow = &sheet.cursorRow;

  if (input != chain) {
    sheet.cursorRow = 0;
    sheet.cursorCol = 0;
  }
  chain = input;
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static void drawCell(int col, int row, int state) {
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

static void fullRedraw(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrintf(0, 0, "CHAIN %02X", chain);

  spreadsheetFullRedraw(&sheet);
}

static void draw(void) {

}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int onEdit(int col, int row, enum CellEditAction action) {
  int handled = 0;

  if (col == 0) {
    if (action == editDoubleTap) {
      uint16_t current = project.chains[chain].phrases[row];

      if (current != EMPTY_VALUE_16) {
        // Find first fully empty phrase
        for (int c = current + 1; c < PROJECT_MAX_PHRASES; c++) {
          if (phraseIsEmpty(c)) {
            project.chains[chain].phrases[row] = c;
            lastPhraseValue = c;
            handled = 1;
            break;
          }
        }
      }
    } else {
      handled = edit16withLimit(action, &project.chains[chain].phrases[row], &lastPhraseValue, 16, PROJECT_MAX_PHRASES);
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
    int phrase = project.chains[chain].phrases[sheet.cursorRow];
    if (phrase == EMPTY_VALUE_16) {
      screenMessage("Enter a phrase");
    } else {
      screenSetup(&screenPhrase, phrase);
    }
    return 1;
  } else if (keys == (keyLeft | keyShift)) {
    screenSetup(&screenSong, 0);
    return 1;
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  if (inputScreenNavigation(keys, isDoubleTap)) return;
  if (spreadsheetInput(&sheet, keys, isDoubleTap)) return;
}

const struct AppScreen screenChain = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
