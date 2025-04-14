#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

static int chain = 0;
static int lastPhraseValue = 0;
static int8_t lastTransposeValue = 0;

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
      gfxPrint(2, 3 + row, "---");
    } else {
      gfxPrintf(2, 3 + row, "%03X", phrase);
    }
  } else {
    // Transpose
    setCellColor(state, 0, phrase == EMPTY_VALUE_16 || phraseHasNotes(phrase));
    gfxPrint(6, 3 + row, byteToHex(project.chains[chain].transpose[row]));
  }
}

static void drawRowHeader(int row, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);
  gfxPrintf(0, 3 + row, "%X", row);
}

static void drawColHeader(int col, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);

  if (col == 0) {
    // Phrase
    gfxPrint(2, 2, "P");
  } else {
    // Transpose
    gfxPrint(6, 2, "T");
  }
}

static void drawCursor(int col, int row) {
  if (col == 0) {
    // Phrase
    gfxCursor(2, 3 + row, 3);
  } else {
    // Transpose
    gfxCursor(6, 3 + row, 2);
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
    int current = project.chains[chain].phrases[row];
    // Phrase
    switch (action) {
      case editClear:
        project.chains[chain].phrases[row] = EMPTY_VALUE_16;
        handled = 1;
        break;
      case editTap:
        if (current == EMPTY_VALUE_16) {
          project.chains[chain].phrases[row] = lastPhraseValue;
          handled = 1;
        }
        break;
      case editDoubleTap:
        if (current != EMPTY_VALUE_16) {
          // Find first fully empty phrase
          for (int c = current + 1; c < PROJECT_MAX_PHRASES; c++) {
            if (phraseIsEmpty(c)) {
              project.chains[chain].phrases[row] = c;
              handled = 1;
              break;
            }
          }
        }
        break;
      case editIncrease:
        if (current != EMPTY_VALUE_16 && current < PROJECT_MAX_PHRASES - 1) {
          project.chains[chain].phrases[row]++;
          handled = 1;
        }
        break;
      case editDecrease:
        if (current != EMPTY_VALUE_16 && current > 0) {
          project.chains[chain].phrases[row]--;
          handled = 1;
        }
        break;
      case editIncreaseBig:
        if (current != EMPTY_VALUE_16) {
          project.chains[chain].phrases[row] = current > PROJECT_MAX_PHRASES - 16 ? PROJECT_MAX_PHRASES - 1 : current + 16;
          handled = 1;
        }
        break;
      case editDecreaseBig:
        if (current != EMPTY_VALUE_16) {
          project.chains[chain].phrases[row] = current < 16 ? 0 : current - 16;
          handled = 1;
        }
        break;
    }

    if (handled && project.chains[chain].phrases[row] != EMPTY_VALUE_16) {
      lastPhraseValue = project.chains[chain].phrases[row];
    }
  } else {
    // Transpose
    int8_t current = project.chains[chain].transpose[row];

    switch (action) {
      case editClear:
        project.chains[chain].transpose[row] = 0;
        handled = 1;
        break;
      case editTap:
        if (current == 0) project.chains[chain].transpose[row] = lastTransposeValue;
        handled = 1;
        break;
      case editIncrease:
        project.chains[chain].transpose[row]++;
        handled = 1;
        break;
      case editDecrease:
        project.chains[chain].transpose[row]--;
        handled = 1;
        break;
      case editIncreaseBig:
        project.chains[chain].transpose[row] += 12;
        handled = 1;
        break;
      case editDecreaseBig:
        project.chains[chain].transpose[row] -= 12;
        handled = 1;
        break;
      default:
        break;
    }

    if (handled) {
      lastTransposeValue = project.chains[chain].transpose[row];
    }
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