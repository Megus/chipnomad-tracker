#include "screens.h"
#include "file_browser.h"

static void setup(int input) {
}

static void fullRedraw(void) {
  fileBrowserDraw();
}

static void draw(void) {
  fileBrowserUpdate();
  fileBrowserDraw();
}

static int onInput(int isKeyDown, int keys, int isDoubleTap) {
  if (fileBrowserInput(keys, isDoubleTap)) {
    fileBrowserDraw();
    return 1;
  }
  return 0;
}

const AppScreen screenFileBrowser = {
  .setup = setup,
  .fullRedraw = fullRedraw,
  .draw = draw,
  .onInput = onInput
};
