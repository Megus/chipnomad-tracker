#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

int instrument = 0;

///////////////////////////////////////////////////////////////////////////////
//
// AY Instrument
//

struct FormScreenData formInstrumentAY = {

};

// Screen common code

static void setup(int input) {
  instrument = input;
}

static void fullRedraw(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "INSTRUMENT");
}

static void draw(void) {

}

static void onInput(int keys, int isDoubleTap) {
  if (keys == (keyRight | keyShift)) {
    screenSetup(&screenTable, instrument);
  } else if (keys == (keyLeft | keyShift)) {
    screenSetup(&screenPhrase, -1);
  }
}

const struct AppScreen screenInstrument = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};