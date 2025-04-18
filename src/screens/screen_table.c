#include <screens.h>
#include <common.h>
#include <corelib_gfx.h>
#include <utils.h>
#include <project.h>

static void setup(int input) {

}

static void fullRedraw(void) {
  const struct ColorScheme cs = appSettings.colorScheme;

  gfxSetFgColor(cs.textTitles);
  gfxPrint(0, 0, "TABLE");
}

static void draw(void) {

}

static void onInput(int keys, int isDoubleTap) {
  //printf("%d\n", event.data.key.keys);
  if (keys == (keyLeft | keyShift)) {
    screenSetup(&screenInstrument, 0);
  }
}

const struct AppScreen screenTable = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};