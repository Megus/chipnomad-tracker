#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

static int phrase = 0;

static uint8_t lastNote = 48;
static uint8_t lastInstrument = 0;
static uint8_t lastVolume = 15;
static uint8_t lastFX[3][2];

static void drawCell(int col, int row, int state);
static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);
static void drawCursor(int col, int row);
static int onEdit(int col, int row, enum CellEditAction action);

static struct SpreadsheetScreenData sheet = {
  .rows = 16,
  .cols = 9,
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
  phrase = input;
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static void fullRedraw(void) {
  gfxSetFgColor(appSettings.colorScheme.textTitles);
  gfxPrintf(0, 0, "PHRASE %03X", phrase);

  spreadsheetFullRedraw(&sheet);
}

static void drawCell(int col, int row, int state) {
  if (col == 0) {
    // Note
    uint8_t note = project.phrases[phrase].notes[row];
    setCellColor(state, note == EMPTY_VALUE_8, 1);
    gfxPrint(2, 3 + row, note == EMPTY_VALUE_8 ? "---" : project.pitchTable.names[note]);
  } else if (col == 1 || col == 2) {
    // Instrument and volume
    uint8_t value = (col == 1) ? project.phrases[phrase].instruments[row] : project.phrases[phrase].volumes[row];
    setCellColor(state, value == EMPTY_VALUE_8, 1);
    gfxPrint(3 + col * 3, 3 + row, value == EMPTY_VALUE_8 ? "--" : byteToHex(value));
  } else if (col == 3 || col == 5 || col == 7) {
    // FX name
    uint8_t fx = project.phrases[phrase].fx[row][(col - 3) / 2][0];
    setCellColor(state, fx == EMPTY_VALUE_8, 1);
    gfxPrint(12 + (col - 3) * 3, 3 + row, fx == EMPTY_VALUE_8 ? "---" : fxName(fx));
  } else if (col == 4 || col == 6 || col == 8) {
    // FX value
    uint8_t value = project.phrases[phrase].fx[row][(col - 4) / 2][1];
    setCellColor(state, 0, project.phrases[phrase].fx[row][(col - 3) / 2][0] != EMPTY_VALUE_8);
    gfxPrint(15 + (col - 4) * 3, 3 + row, byteToHex(value));
  }
}

static void drawRowHeader(int row, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : ((row & 3) == 0 ? cs.textValue : cs.textInfo));
  gfxPrintf(0, 3 + row, "%X", row);
}

static void drawColHeader(int col, int state) {
  const struct ColorScheme cs = appSettings.colorScheme;
  gfxSetFgColor((state & stateFocus) ? cs.textDefault : cs.textInfo);

  switch (col) {
    case 0:
      gfxPrint(2, 2, "N");
      break;
    case 1:
      gfxPrint(6, 2, "I");
      break;
    case 2:
      gfxPrint(9, 2, "V");
      break;
    case 3:
    case 4:
      gfxPrint(12, 2, "FX1");
      break;
    case 5:
    case 6:
      gfxPrint(18, 2, "FX2");
      break;
    case 7:
    case 8:
      gfxPrint(24, 2, "FX3");
      break;
    default:
      break;
  }
}

static void drawCursor(int col, int row) {

}

static int onEdit(int col, int row, enum CellEditAction action) {

  return 0;
}

static void draw(void) {

}

///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    screenSetup(&screenInstrument, 0);
    return 1;
  } else if (keys == (keyLeft | keyShift)) {
    screenSetup(&screenChain, 0);
    return 1;
  } else if (keys == (keyUp | keyShift)) {
    screenSetup(&screenGroove, 0);
    return 1;
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  if (inputScreenNavigation(keys, isDoubleTap)) return;
  if (spreadsheetInput(&sheet, keys, isDoubleTap)) return;
}

const struct AppScreen screenPhrase = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};