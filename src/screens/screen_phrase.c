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
  phrase = input;
}

///////////////////////////////////////////////////////////////////////////////
//
// Drawing functions
//

static void fullRedraw(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrintf(0, 0, "PHRASE %03X", phrase);
}

static void drawCell(int col, int row, int state) {

}

static void drawRowHeader(int row, int state) {

}

static void drawColHeader(int col, int state) {

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