#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>
#include <screen_instrument.h>

int instrument = 0;

static void drawRowHeader(int row, int state);
static void drawColHeader(int col, int state);

static struct ScreenData formInstrumentNone = {
  .rows = 1,
  .cursorRow = 0,
  .cursorCol = 0,
  .getColumnCount = instrumentCommonColumnCount,
  .drawStatic = instrumentCommonDrawStatic,
  .drawCursor = instrumentCommonDrawCursor,
  .drawRowHeader = drawRowHeader,
  .drawColHeader = drawColHeader,
  .drawField = instrumentCommonDrawField,
  .onEdit = instrumentCommonOnEdit,
};

static void setup(int input) {
  instrument = input;
}

static void fullRedraw(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrintf(0, 0, "INSTRUMENT %02X", instrument);
}

static void draw(void) {

}

///////////////////////////////////////////////////////////////////////////////
//
// Common part of the form
//

static void drawRowHeader(int row, int state) {

}

static void drawColHeader(int col, int state) {

}

int instrumentCommonColumnCount(int row) {
  if (row == 0) {
    return 3; // Instrument type, load, save
  } else if (row == 1) {
    return 15; // Instrument name
  } else if (row == 2) {
    return 2; // Transpose on/off, Table tic speed
  }
  return 1; // Default value
}

void instrumentCommonDrawStatic(void) {

}

void instrumentCommonDrawCursor(int col, int row) {

}

void instrumentCommonDrawField(int col, int row, int state) {

}

int instrumentCommonOnEdit(int col, int row, enum CellEditAction action) {

  return 0;
}


///////////////////////////////////////////////////////////////////////////////
//
// Input handling
//

static int inputScreenNavigation(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    screenSetup(&screenTable, instrument);
    return 1;
  } else if (keys == (keyLeft | keyShift)) {
    screenSetup(&screenPhrase, -1);
    return 1;
  }
  return 0;
}

static void onInput(int keys, int isDoubleTap) {
  if (inputScreenNavigation(keys, isDoubleTap)) return;

  struct ScreenData* form = &formInstrumentNone;
  if (project.instruments[instrument].type == instAY) {
    form = &formInstrumentAY;
  }

  if (screenInput(form, keys, isDoubleTap)) return;
}

const struct AppScreen screenInstrument = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
